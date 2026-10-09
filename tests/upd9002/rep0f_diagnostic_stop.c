/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "cpucore.h"
#include "upd9002_diagnostic.h"
#include "tests/upd9002/rep0f_diagnostic_stop.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static UINT32 memory_hash(void) {
	UINT32 hash;
	UINT index;

	hash = 2166136261u;
	for (index = 0; index < 0x100000; index++) {
		hash ^= mem[index];
		hash *= 16777619u;
	}
	return hash;
}

static void setup_state(const UINT8 *bytes, UINT size, UINT16 msw) {
	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0x1357;
	CPU_BX = 0x2468;
	CPU_CX = 0xabcd;
	CPU_DX = 0x55aa;
	CPU_SI = 0x1122;
	CPU_DI = 0x3344;
	CPU_BP = 0x5566;
	CPU_SP = 0x9000;
	CPU_ES = 0x0100;
	CPU_CS = 0x0200;
	CPU_SS = 0x0300;
	CPU_DS = 0x0400;
	ES_BASE = 0x00001000;
	CS_BASE = 0x00000000;
	SS_BASE = 0x00003000;
	DS_BASE = 0x00004000;
	upd9002_core_context.s.ss_fix = 0x00035550;
	upd9002_core_context.s.ds_fix = 0x00046660;
	CPU_IP = 0x2000;
	CPU_FLAG = 0xf8d7;
	CPU_ADRSMASK = 0x000fffff;
	CPU_REMCLOCK = 12345;
	CPU_BASECLOCK = 23456;
	CPU_CLOCK = 0x3456789a;
	upd9002_core_context.s.ovflag = 0x76543210;
	upd9002_core_context.s.MSW = msw;
	mem[0x1fff] = 0xa5;
	CopyMemory(mem + 0x2000, bytes, size);
	mem[0x2000 + size] = 0x5a;
}

static int run_unprefixed_control(void) {
	static const UINT8 bytes[] = {0x0f, 0x01, 0xf0, 0x90};

	setup_state(bytes, sizeof(bytes), 0);
	CPU_AX = 0x0001;
	upd9002_core_step();
	if (upd9002_diagnostic_pending() || (CPU_IP != 0x2002) || (CPU_MSW != 0)) {
		fprintf(stderr, "upd9002-rep0f-diagnostic: unprefixed control changed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int run_diagnostic_case(UINT8 prefix, UINT8 second, const UINT8 *leading, UINT leading_size,
                               UINT16 msw) {
	UINT8 bytes[8];
	UINT size;
	Upd9002RuntimeState state_before;
	UPD9002_DIAGNOSTIC diagnostic;
	UINT32 hash_before;

	size = 0;
	if (leading_size != 0) {
		CopyMemory(bytes, leading, leading_size);
		size = leading_size;
	}
	bytes[size++] = prefix;
	bytes[size++] = 0x0f;
	bytes[size++] = second;
	bytes[size++] = 0x90;
	setup_state(bytes, size, msw);
	state_before = upd9002_core_context.s;
	hash_before = memory_hash();

	upd9002_core_step();
	if ((upd9002_diagnostic_get(&diagnostic) != SUCCESS) ||
	    (diagnostic.reason != UPD9002_DIAGNOSTIC_REP0F) || (diagnostic.prefix != prefix) ||
	    (diagnostic.cs != 0x0200) || (diagnostic.ip != 0x2000) ||
	    memcmp(&state_before, &upd9002_core_context.s, sizeof(state_before)) ||
	    (hash_before != memory_hash())) {
		fprintf(stderr,
		        "upd9002-rep0f-diagnostic: stop mismatch prefix=%02x "
		        "second=%02x leading=%u\n",
		        prefix, second, leading_size);
		return (FAILURE);
	}

	/* A latched stop must not execute on a subsequent scheduler call. */
	upd9002_core_step();
	if (memcmp(&state_before, &upd9002_core_context.s, sizeof(state_before)) ||
	    (hash_before != memory_hash())) {
		fprintf(stderr, "upd9002-rep0f-diagnostic: latched stop resumed execution\n");
		return FAILURE;
	}
	return (SUCCESS);
}

int upd9002_rep0f_diagnostic_stop_main(void) {
	static const UINT8 segment_prefixes[] = {0x26, 0x2e, 0x36, 0x3e};
	UINT prefix_index;
	UINT second;
	UINT segment_index;
	UINT cases;

	upd9002_core_initialize();
	if (run_unprefixed_control() != SUCCESS) {
		goto failed;
	}
	cases = 0;
	for (prefix_index = 0; prefix_index < 2; prefix_index++) {
		UINT8 prefix;

		prefix = prefix_index ? 0xf3 : 0xf2;
		for (second = 0; second < 256; second++) {
			if (run_diagnostic_case(prefix, (UINT8)second, NULL, 0, 0) != SUCCESS) {
				goto failed;
			}
			cases++;
		}
		for (segment_index = 0; segment_index < NELEMENTS(segment_prefixes); segment_index++) {
			if (run_diagnostic_case(prefix, 0x01, &segment_prefixes[segment_index], 1, 0) !=
			    SUCCESS) {
				goto failed;
			}
			cases++;
		}
	}
	if (run_diagnostic_case(0xf2, 0x01, NULL, 0, MSW_PE) != SUCCESS ||
	    run_diagnostic_case(0xf3, 0x01, NULL, 0, MSW_PE) != SUCCESS) {
		goto failed;
	}
	cases += 2;
	upd9002_core_deinitialize();
	fprintf(stderr, "upd9002-rep0f-diagnostic: cases=%u state-and-memory-atomic pass\n", cases);
	return (EXIT_SUCCESS);

failed:
	upd9002_core_deinitialize();
	return (EXIT_FAILURE);
}
