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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "cpucore.h"
#include "tests/upd9002/iret_restoration.h"

#include <stdio.h>

static UINT32 physical_address(UINT16 segment, UINT16 offset) {
	return ((((UINT32)segment << 4) + offset) & 0x000fffff);
}

static void write_stack_word(UINT16 segment, UINT16 offset, UINT16 value) {
	mem[physical_address(segment, offset)] = (UINT8)value;
	mem[physical_address(segment, (UINT16)(offset + 1))] = (UINT8)(value >> 8);
}

static void setup_state(UINT16 ss, UINT16 sp, UINT16 restored_ip, UINT16 restored_cs,
                        UINT16 restored_flags) {
	static const UINT8 instruction[] = {0xcf};

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0x1357;
	CPU_BX = 0x2468;
	CPU_CX = 0x369c;
	CPU_DX = 0x48ad;
	CPU_SI = 0x55aa;
	CPU_DI = 0xaa55;
	CPU_BP = 0x7bcd;
	CPU_SP = sp;
	CPU_ES = 0x1111;
	CPU_CS = 0x2000;
	CPU_SS = ss;
	CPU_DS = 0x3333;
	CPU_IP = 0x0100;
	CPU_FLAG = 0xf002;
	ES_BASE = (UINT32)CPU_ES << 4;
	CS_BASE = (UINT32)CPU_CS << 4;
	SS_BASE = (UINT32)CPU_SS << 4;
	DS_BASE = (UINT32)CPU_DS << 4;
	upd9002_core_context.s.ss_fix = SS_BASE;
	upd9002_core_context.s.ds_fix = DS_BASE;
	CPU_ADRSMASK = 0x000fffff;
	CPU_REMCLOCK = 1000;
	CPU_BASECLOCK = 1000;
	CPU_CLOCK = 0;
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
	mem[(CS_BASE + CPU_IP) & CPU_ADRSMASK] = instruction[0];
	write_stack_word(ss, sp, restored_ip);
	write_stack_word(ss, (UINT16)(sp + 2), restored_cs);
	write_stack_word(ss, (UINT16)(sp + 4), restored_flags);
}

static int fail_case(const char *name, const char *detail) {
	fprintf(stderr, "upd9002-iret-restoration: %s: %s\n", name, detail);
	return FAILURE;
}

static int run_restoration_case(const char *name, UINT16 ss, UINT16 sp, UINT16 restored_ip,
                                UINT16 restored_cs, UINT16 stack_flags, UINT16 expected_flags) {
	setup_state(ss, sp, restored_ip, restored_cs, stack_flags);
	upd9002_core_step();
	if ((CPU_IP != restored_ip) || (CPU_CS != restored_cs)) {
		return fail_case(name, "IP or CS word restoration differs");
	}
	if (CPU_SP != (UINT16)(sp + 6)) {
		return fail_case(name, "final SP differs");
	}
	if (CPU_FLAG != expected_flags) {
		fprintf(stderr, "upd9002-iret-restoration: %s: FLAGS expected=%04x actual=%04x\n", name,
		        expected_flags, CPU_FLAG);
		return fail_case(name, "FLAGS restoration differs");
	}
	if ((CPU_AX != 0x1357) || (CPU_BX != 0x2468) || (CPU_CX != 0x369c) || (CPU_DX != 0x48ad) ||
	    (CPU_SI != 0x55aa) || (CPU_DI != 0xaa55) || (CPU_BP != 0x7bcd) || (CPU_ES != 0x1111) ||
	    (CPU_SS != ss) || (CPU_DS != 0x3333)) {
		return fail_case(name, "unrelated architectural state changed");
	}
	return SUCCESS;
}

static int test_stack_contract(void) {
	if (run_restoration_case("ordinary", 0x3000, 0x8000, 0x1234, 0xabcd, 0x0202, 0x0202) !=
	        SUCCESS ||
	    run_restoration_case("segment offset wrap", 0x2345, 0xfffc, 0x0000, 0xffff, 0x0202,
	                         0x0202) != SUCCESS ||
	    run_restoration_case("physical wrap", 0xffff, 0x000c, 0xffff, 0x0000, 0x0202, 0x0202) !=
	        SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_flags_restoration(void) {
	static const struct {
		UINT16 stack_flags;
		UINT16 expected_flags;
	} cases[] = {{0x0000, 0x0002}, {0x0001, 0x0003}, {0x0002, 0x0002}, {0x0004, 0x0006},
	             {0x0008, 0x0002}, {0x0010, 0x0012}, {0x0020, 0x0002}, {0x0040, 0x0042},
	             {0x0080, 0x0082}, {0x0200, 0x0202}, {0x0400, 0x0402}, {0x0800, 0x0802},
	             {0x1000, 0x0002}, {0x2000, 0x0002}, {0x4000, 0x0002}, {0x8000, 0x0002}};
	UINT index;

	for (index = 0; index < NELEMENTS(cases); index++) {
		if (run_restoration_case("FLAGS bit rule", 0x3000, 0x8000, 0x7654, 0x3210,
		                         cases[index].stack_flags,
		                         cases[index].expected_flags) != SUCCESS) {
			return FAILURE;
		}
	}
	return SUCCESS;
}

int upd9002_iret_restoration_main(void) {
	int result;

	upd9002_test_flat_memory_set(TRUE);
	upd9002_core_initialize();
	result = SUCCESS;
	if ((test_stack_contract() != SUCCESS) || (test_flags_restoration() != SUCCESS)) {
		result = FAILURE;
	}
	upd9002_core_deinitialize();
	upd9002_test_flat_memory_set(FALSE);
	if (result != SUCCESS) {
		return result;
	}
	fprintf(stderr, "upd9002-iret-restoration: corrected contract checks passed\n");
	return SUCCESS;
}
