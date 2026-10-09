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
#include "tests/upd9002/mov_imm_register.h"

#include <stdio.h>

static UINT32 physical_address(UINT16 segment, UINT16 offset) {
	return ((((UINT32)segment << 4) + offset) & 0x000fffff);
}

static UINT16 get_word_register(UINT code) {
	switch (code) {
	case 0:
		return CPU_AX;
	case 1:
		return CPU_CX;
	case 2:
		return CPU_DX;
	case 3:
		return CPU_BX;
	case 4:
		return CPU_SP;
	case 5:
		return CPU_BP;
	case 6:
		return CPU_SI;
	default:
		return CPU_DI;
	}
}

static UINT8 get_byte_register(UINT code) {
	const UINT16 value = get_word_register(code & 3);

	return (code & 4) ? (UINT8)(value >> 8) : (UINT8)value;
}

static void setup_state(const UINT8 *instruction, UINT length) {
	UINT index;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0x1357;
	CPU_CX = 0x2468;
	CPU_DX = 0x369c;
	CPU_BX = 0x48ad;
	CPU_SP = 0x8000;
	CPU_BP = 0x7bcd;
	CPU_SI = 0x55aa;
	CPU_DI = 0xaa55;
	CPU_ES = 0x1111;
	CPU_CS = 0x2000;
	CPU_SS = 0x3000;
	CPU_DS = 0x3333;
	CPU_IP = 0x0100;
	CPU_FLAG = 0x0fd7;
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
	for (index = 0; index < length; index++) {
		mem[(CS_BASE + CPU_IP + index) & CPU_ADRSMASK] = instruction[index];
	}
}

static int fail_case(const char *name, const char *detail) {
	fprintf(stderr, "upd9002-mov-imm-register: %s: %s\n", name, detail);
	return FAILURE;
}

static int test_c6_registers(void) {
	static const UINT8 immediates[] = {0x00, 0xff, 0x5a, 0xa5, 0x11, 0xee, 0x7f, 0x80};
	UINT code;

	for (code = 0; code < 8; code++) {
		UINT16 before[8];
		UINT index;
		UINT reg_extension = (code + 3) & 7;
		UINT8 instruction[3] = {0xc6, (UINT8)(0xc0 | (reg_extension << 3) | code),
		                        immediates[code]};
		setup_state(instruction, NELEMENTS(instruction));
		for (index = 0; index < 8; index++) {
			before[index] = get_word_register(index);
		}
		upd9002_core_step();
		if (get_byte_register(code) != immediates[code]) {
			return fail_case("C6 register map", "destination byte differs");
		}
		for (index = 0; index < 8; index++) {
			UINT16 expected = before[index];
			if (index == (code & 3)) {
				if (code & 4) {
					expected = (UINT16)((expected & 0x00ff) | (immediates[code] << 8));
				} else {
					expected = (UINT16)((expected & 0xff00) | immediates[code]);
				}
			}
			if (get_word_register(index) != expected) {
				return fail_case("C6 register map", "paired or unrelated register changed");
			}
		}
		if ((CPU_FLAG != 0x0fd7) || (CPU_IP != 0x0103)) {
			return fail_case("C6 register map", "FLAGS or IP changed");
		}
	}
	return SUCCESS;
}

static int test_c7_registers(void) {
	static const UINT16 immediates[] = {0x0000, 0xffff, 0x1234, 0xabcd,
	                                    0x00ff, 0xff00, 0x5aa5, 0x8001};
	UINT code;

	for (code = 0; code < 8; code++) {
		UINT16 before[8];
		UINT index;
		UINT reg_extension = (code + 5) & 7;
		UINT8 instruction[4] = {0xc7, (UINT8)(0xc0 | (reg_extension << 3) | code),
		                        (UINT8)immediates[code], (UINT8)(immediates[code] >> 8)};
		setup_state(instruction, NELEMENTS(instruction));
		for (index = 0; index < 8; index++) {
			before[index] = get_word_register(index);
		}
		upd9002_core_step();
		for (index = 0; index < 8; index++) {
			const UINT16 expected = (index == code) ? immediates[code] : before[index];
			if (get_word_register(index) != expected) {
				return fail_case("C7 register map", "destination or unrelated register differs");
			}
		}
		if ((CPU_FLAG != 0x0fd7) || (CPU_IP != 0x0104)) {
			return fail_case("C7 register map", "FLAGS or IP changed");
		}
	}
	return SUCCESS;
}

static int run_memory_case(const char *name, const UINT8 *instruction, UINT length, UINT16 segment,
                           UINT16 offset, UINT16 expected, UINT width) {
	const UINT32 address = physical_address(segment, offset);
	UINT16 before[8];
	UINT index;

	setup_state(instruction, length);
	if (segment == 0xffff) {
		CPU_ES = segment;
		ES_BASE = (UINT32)CPU_ES << 4;
	}
	if ((offset == 0x0001) && (instruction[1] == 0x00)) {
		CPU_BX = 0xffff;
		CPU_SI = 0x0002;
	}
	for (index = 0; index < 8; index++) {
		before[index] = get_word_register(index);
	}
	mem[address] = 0x3c;
	mem[(address + 1) & 0x000fffff] = 0xc3;
	upd9002_core_step();
	if ((mem[address] != (UINT8)expected) ||
	    ((width == 16) && (mem[(address + 1) & 0x000fffff] != (UINT8)(expected >> 8)))) {
		return fail_case(name, "memory destination differs");
	}
	for (index = 0; index < 8; index++) {
		if (get_word_register(index) != before[index]) {
			return fail_case(name, "register changed");
		}
	}
	if ((CPU_FLAG != 0x0fd7) || (CPU_IP != (UINT16)(0x0100 + length))) {
		return fail_case(name, "FLAGS or IP changed");
	}
	return SUCCESS;
}

static int test_memory_forms(void) {
	static const UINT8 c6_direct[] = {0xc6, 0x06, 0x34, 0x12, 0xa5};
	static const UINT8 c6_disp8[] = {0xc6, 0x40, 0x7f, 0x5a};
	static const UINT8 c7_disp16[] = {0xc7, 0x80, 0x34, 0x12, 0x34, 0x12};
	static const UINT8 c6_physical_wrap[] = {0x26, 0xc6, 0x06, 0x10, 0x00, 0xee};
	static const UINT8 c7_offset_wrap[] = {0xc7, 0x00, 0xcd, 0xab};

	if (run_memory_case("C6 direct", c6_direct, NELEMENTS(c6_direct), 0x3333, 0x1234, 0x00a5, 8) !=
	    SUCCESS) {
		return FAILURE;
	}
	if (run_memory_case("C6 disp8", c6_disp8, NELEMENTS(c6_disp8), 0x3333,
	                    (UINT16)(0x48ad + 0x55aa + 0x7f), 0x005a, 8) != SUCCESS) {
		return FAILURE;
	}
	if (run_memory_case("C7 disp16", c7_disp16, NELEMENTS(c7_disp16), 0x3333,
	                    (UINT16)(0x48ad + 0x55aa + 0x1234), 0x1234, 16) != SUCCESS) {
		return FAILURE;
	}
	if (run_memory_case("C6 physical wrap", c6_physical_wrap, NELEMENTS(c6_physical_wrap), 0xffff,
	                    0x0010, 0x00ee, 8) != SUCCESS) {
		return FAILURE;
	}
	if (run_memory_case("C7 offset wrap", c7_offset_wrap, NELEMENTS(c7_offset_wrap), 0x3333, 0x0001,
	                    0xabcd, 16) != SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_mov_imm_register_main(void) {
	upd9002_test_flat_memory_set(TRUE);

	upd9002_core_initialize();
	if ((test_c6_registers() != SUCCESS) || (test_c7_registers() != SUCCESS) ||
	    (test_memory_forms() != SUCCESS)) {
		upd9002_core_deinitialize();
		upd9002_test_flat_memory_set(FALSE);
		return FAILURE;
	}
	upd9002_core_deinitialize();
	upd9002_test_flat_memory_set(FALSE);
	fprintf(stderr, "upd9002-mov-imm-register: register and memory checks passed\n");
	return SUCCESS;
}
