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
#include "upd9002_state.h"
#include "upd9002_regs.h"
#include "tests/upd9002/fixtures.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FIXTURE_LINE_CAPACITY 1024

void upd9002_m42_process_cpu_reset_request(void);

static void bytes_hex(const void *data, size_t size, char *output) {
	const uint8_t *bytes;
	size_t index;

	bytes = (const uint8_t *)data;
	for (index = 0; index < size; index++) {
		sprintf(output + index * 2, "%02x", bytes[index]);
	}
	output[size * 2] = '\0';
}

static void fixture_line(const char *scenario, char *line, size_t size) {
	Upd9002StateImage cpu;
	char cpu_hex[sizeof(cpu) * 2 + 1];
	char regs_hex[sizeof(upd9002_regs) * 2 + 1];

	upd9002_state_export(&cpu);
	bytes_hex(&cpu, sizeof(cpu), cpu_hex);
	bytes_hex(&upd9002_regs, sizeof(upd9002_regs), regs_hex);
	snprintf(line, size,
	         "%s,cpu286_size=%u,cpu286=%s,upd9002_size=%u,upd9002=%s,"
	         "ax=%04x,bx=%04x,cx=%04x,dx=%04x,sp=%04x,cs=%04x,ip=%04x,"
	         "flags=%04x,csbase=%08x,remain=%08x,base=%08x,clock=%08x,type=%02x\n",
	         scenario, (unsigned int)sizeof(cpu), cpu_hex, (unsigned int)sizeof(upd9002_regs),
	         regs_hex, CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_SP, CPU_CS, CPU_IP, CPU_FLAG, CS_BASE,
	         (uint32_t)CPU_REMCLOCK, (uint32_t)CPU_BASECLOCK, CPU_CLOCK,
	         upd9002_core_context.s.cpu_type);
}

static void prepare_executed(void) {
	static const uint8_t program[] = {0xb8, 0x34, 0x12, 0x40, 0x90};
	uint32_t index;

	ZeroMemory(&CPU_STATSAVE, sizeof(CPU_STATSAVE));
	upd9002_state_reset();
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
	CPU_FLAG = 0xf002;
	CPU_ADRSMASK = 0xfffff;
	CPU_SP = 0x8000;
	CPU_IP = 0x2000;
	CPU_REMCLOCK = 100;
	CPU_BASECLOCK = 100;
	CPU_CLOCK = 0x12345678;
	CopyMemory(mem + 0x2000, program, sizeof(program));
	for (index = 0; index < 3; index++) {
		upd9002_core_step();
	}
}

static int verify_native_reset_invariant(void) {
	Upd9002StateImage saved;

	upd9002_state_export(&saved);
	upd9002_core_context.s.cpu_type = 0;
	upd9002_core_reset();
	if ((upd9002_core_context.s.cpu_type != CPUTYPE_V30) || (CPU_CS != 0xf000) ||
	    (CPU_IP != 0xfff0) || (CS_BASE != 0x000f0000) || (CPU_FLAG != 0xf002)) {
		(void)upd9002_state_import(&saved, sizeof(saved), NULL, 0);
		return FAILURE;
	}
	return upd9002_state_import(&saved, sizeof(saved), NULL, 0);
}

int upd9002_fixture_verify(const char *path) {
	FILE *stream;
	Upd9002StateImage saved_cpu;
	UPD9002_REGS saved_regs;
	uint8_t saved_program[8];
	char actual[3][FIXTURE_LINE_CAPACITY];
	char expected[FIXTURE_LINE_CAPACITY];
	uint32_t index;

	if (verify_native_reset_invariant() != SUCCESS) {
		fprintf(stderr, "upd9002-fixture: native reset invariant failed\n");
		return FAILURE;
	}
	upd9002_state_export(&saved_cpu);
	saved_regs = upd9002_regs;
	CopyMemory(saved_program, mem + 0x2000, sizeof(saved_program));
	fixture_line("reset", actual[0], sizeof(actual[0]));
	prepare_executed();
	fixture_line("executed-3", actual[1], sizeof(actual[1]));
	CPU_RESETREQ = 1;
	upd9002_m42_process_cpu_reset_request();
	fixture_line("cpu-shut-request", actual[2], sizeof(actual[2]));
	if (upd9002_state_import(&saved_cpu, sizeof(saved_cpu), NULL, 0) != SUCCESS) {
		return FAILURE;
	}
	upd9002_regs = saved_regs;
	CopyMemory(mem + 0x2000, saved_program, sizeof(saved_program));

	stream = fopen(path, "rb");
	if (stream == NULL) {
		for (index = 0; index < 3; index++) {
			fprintf(stderr, "upd9002-fixture-generated:%s", actual[index]);
		}
		return FAILURE;
	}
	for (index = 0; index < 3; index++) {
		if ((fgets(expected, sizeof(expected), stream) == NULL) ||
		    strcmp(expected, actual[index])) {
			fprintf(stderr, "upd9002-fixture: mismatch for row %u\n", index);
			fclose(stream);
			return FAILURE;
		}
	}
	if (fgets(expected, sizeof(expected), stream) != NULL) {
		fclose(stream);
		return FAILURE;
	}
	fclose(stream);
	fprintf(stderr, "upd9002-fixture: reset, executed, and CPU_SHUT payloads passed\n");
	return SUCCESS;
}
