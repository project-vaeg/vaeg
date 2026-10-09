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
#include "tests/upd9002/semantics_bundle.h"

#include <stdio.h>

typedef struct {
	UINT8 source;
	UINT8 accumulator;
	UINT8 expected_accumulator;
	UINT8 expected_destination;
} PACKED_BCD_CASE;

typedef struct {
	UINT8 radix;
	UINT8 initial_al;
	UINT8 expected_ah;
	UINT8 expected_al;
} AAM_CASE;

typedef struct {
	UINT8 opcode;
	UINT8 initial_al;
	UINT8 input_flags;
	UINT8 expected_al;
	UINT16 expected_flags;
} DECIMAL_ADJUST_CASE;

typedef struct {
	UINT8 opcode;
	UINT8 initial_al;
	UINT8 input_flags;
	UINT16 expected_ax;
	UINT16 expected_flags;
} ASCII_ADJUST_CASE;

typedef struct {
	UINT8 opcode;
	UINT8 subform;
	UINT8 count;
	UINT16 initial_destination;
	UINT16 expected_destination;
	UINT16 expected_flags;
} SHIFT_CASE;

static UINT16 aam_expected_flags(UINT16 initial_flags, UINT8 value) {
	UINT bits = value;
	UINT parity = 0;
	UINT16 flags = (UINT16)(initial_flags & 0xf700);

	while (bits) {
		parity ^= bits & 1;
		bits >>= 1;
	}
	flags |= 0x0002;
	if (!parity) {
		flags |= 0x0004;
	}
	if (!value) {
		flags |= 0x0040;
	}
	if (value & 0x80) {
		flags |= 0x0080;
	}
	return flags;
}

static void setup_instruction(const UINT8 *instruction, UINT length, UINT16 ax, UINT16 flags) {
	UINT index;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = ax;
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
	CPU_FLAG = flags;
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

static void set_byte_register(UINT code, UINT8 value) {
	UINT16 *const registers[] = {&CPU_AX, &CPU_CX, &CPU_DX, &CPU_BX};
	UINT16 *const target = registers[code & 3];

	if (code & 4) {
		*target = (UINT16)((*target & 0x00ff) | ((UINT16)value << 8));
	} else {
		*target = (UINT16)((*target & 0xff00) | value);
	}
}

static int test_aam_semantics(void) {
	static const AAM_CASE cases[] = {{0, 0x85, 0xff, 0x85},  {1, 0xff, 0xff, 0x00},
	                                 {2, 0xff, 0x7f, 0x01},  {9, 0x5a, 0x0a, 0x00},
	                                 {10, 0x63, 0x09, 0x09}, {11, 0x79, 0x0b, 0x00},
	                                 {16, 0xab, 0x0a, 0x0b}, {255, 0xfe, 0x00, 0xfe}};
	const UINT16 initial_flags = 0xfc93;
	UINT index;

	for (index = 0; index < NELEMENTS(cases); index++) {
		const AAM_CASE *const value = &cases[index];
		const UINT8 instruction[] = {0xd4, value->radix};
		const UINT16 expected_ax = (UINT16)((value->expected_ah << 8) | value->expected_al);

		setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(0x5a00 | value->initial_al),
		                  initial_flags);
		upd9002_core_step();
		if (CPU_AX != expected_ax) {
			fprintf(stderr, "upd9002-m62: AAM radix %u produced AX=%04x, expected %04x\n",
			        value->radix, CPU_AX, expected_ax);
			return FAILURE;
		}
		if (CPU_FLAG != aam_expected_flags(initial_flags, value->expected_al)) {
			fprintf(stderr, "upd9002-m62: AAM radix %u produced FLAGS=%04x\n", value->radix,
			        CPU_FLAG);
			return FAILURE;
		}
		if ((CPU_IP != 0x0102) || (CPU_CX != 0x2468) || (CPU_DX != 0x369c) || (CPU_BX != 0x48ad) ||
		    (CPU_SP != 0x8000) || (CPU_BP != 0x7bcd) || (CPU_SI != 0x55aa) || (CPU_DI != 0xaa55)) {
			fprintf(stderr, "upd9002-m62: AAM changed IP or an unrelated register\n");
			return FAILURE;
		}
	}
	return SUCCESS;
}

static int test_ror4_registers(void) {
	static const UINT8 sources[] = {0x12, 0xab, 0xf0, 0x5e, 0x87, 0xd5, 0x09, 0xff};
	const UINT16 initial_flags = 0xfcd7;
	UINT code;

	for (code = 0; code < NELEMENTS(sources); code++) {
		const UINT8 instruction[] = {0x0f, 0x2a, (UINT8)(0xc0 | code)};
		UINT16 before[8];
		UINT8 accumulator;
		UINT8 expected_destination;
		UINT index;

		setup_instruction(instruction, NELEMENTS(instruction), 0x7a34, initial_flags);
		set_byte_register(code, sources[code]);
		accumulator = CPU_AL;
		expected_destination = (UINT8)((sources[code] >> 4) | ((accumulator & 0x0f) << 4));
		for (index = 0; index < 8; index++) {
			before[index] = get_word_register(index);
		}
		upd9002_core_step();
		if (CPU_AL != sources[code]) {
			fprintf(stderr, "upd9002-m62: ROR4 register %u did not load AL\n", code);
			return FAILURE;
		}
		if ((code != 0) && (get_byte_register(code) != expected_destination)) {
			fprintf(stderr, "upd9002-m62: ROR4 register %u destination differs\n", code);
			return FAILURE;
		}
		for (index = 0; index < 8; index++) {
			if ((index > 3) && (get_word_register(index) != before[index])) {
				fprintf(stderr, "upd9002-m62: ROR4 changed an unrelated word register\n");
				return FAILURE;
			}
		}
		if ((CPU_FLAG != initial_flags) || (CPU_IP != 0x0103)) {
			fprintf(stderr, "upd9002-m62: ROR4 changed FLAGS or IP\n");
			return FAILURE;
		}
	}
	return SUCCESS;
}

static int test_ror4_memory(void) {
	static const UINT8 instruction[] = {0x0f, 0x2a, 0x06, 0x34, 0x12};
	const UINT16 initial_flags = 0xfcd7;
	const UINT32 address = (((UINT32)0x3333 << 4) + 0x1234) & 0xfffff;

	setup_instruction(instruction, NELEMENTS(instruction), 0x7a34, initial_flags);
	mem[address] = 0xab;
	upd9002_core_step();
	if ((CPU_AL != 0xab) || (mem[address] != 0x4a) || (CPU_AH != 0x7a) ||
	    (CPU_FLAG != initial_flags) || (CPU_IP != 0x0105)) {
		fprintf(stderr, "upd9002-m62: ROR4 memory result differs\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_evidence_oracles(void) {
	static const PACKED_BCD_CASE ror4_cases[] = {
	    {0x12, 0x34, 0x12, 0x41}, {0xab, 0xcd, 0xab, 0xda}, {0xf0, 0x0f, 0xf0, 0xff}};
	static const PACKED_BCD_CASE rol4_cases[] = {
	    {0x12, 0x34, 0x41, 0x24}, {0xab, 0xcd, 0xda, 0xbd}, {0xf0, 0x0f, 0xff, 0x0f}};
	UINT index;

	for (index = 0; index < NELEMENTS(ror4_cases); index++) {
		const PACKED_BCD_CASE *const value = &ror4_cases[index];
		if ((value->expected_accumulator != value->source) ||
		    (value->expected_destination !=
		     (UINT8)((value->source >> 4) | ((value->accumulator & 0x0f) << 4)))) {
			fprintf(stderr, "upd9002-m62: ROR4 evidence oracle differs\n");
			return FAILURE;
		}
	}
	for (index = 0; index < NELEMENTS(rol4_cases); index++) {
		const PACKED_BCD_CASE *const value = &rol4_cases[index];
		if ((value->expected_accumulator !=
		     (UINT8)((value->accumulator << 4) | (value->source >> 4))) ||
		    (value->expected_destination !=
		     (UINT8)((value->source << 4) | (value->accumulator & 0x0f)))) {
			fprintf(stderr, "upd9002-m62: ROL4 evidence oracle differs\n");
			return FAILURE;
		}
	}
	return SUCCESS;
}

static int test_rol4_registers(void) {
	static const UINT8 sources[] = {0x12, 0xab, 0xf0, 0x5e, 0x87, 0xd5, 0x09, 0xff};
	const UINT16 initial_flags = 0xfcd7;
	UINT code;

	for (code = 0; code < NELEMENTS(sources); code++) {
		const UINT8 instruction[] = {0x0f, 0x28, (UINT8)(0xc0 | code)};
		UINT8 accumulator;
		UINT8 expected_accumulator;
		UINT8 expected_destination;

		setup_instruction(instruction, NELEMENTS(instruction), 0x7a34, initial_flags);
		set_byte_register(code, sources[code]);
		accumulator = CPU_AL;
		expected_accumulator = (UINT8)((accumulator << 4) | (sources[code] >> 4));
		expected_destination = (UINT8)((sources[code] << 4) | (accumulator & 0x0f));
		upd9002_core_step();
		if (CPU_AL != expected_accumulator) {
			fprintf(stderr, "upd9002-m62: ROL4 register %u accumulator differs\n", code);
			return FAILURE;
		}
		if ((code != 0) && (get_byte_register(code) != expected_destination)) {
			fprintf(stderr, "upd9002-m62: ROL4 register %u destination differs\n", code);
			return FAILURE;
		}
		if ((CPU_FLAG != initial_flags) || (CPU_IP != 0x0103)) {
			fprintf(stderr, "upd9002-m62: ROL4 changed FLAGS or IP\n");
			return FAILURE;
		}
	}
	return SUCCESS;
}

static int test_rol4_memory(void) {
	static const UINT8 instruction[] = {0x3e, 0x0f, 0x28, 0x06, 0x34, 0x12};
	const UINT16 initial_flags = 0xfcd7;
	const UINT32 address = (((UINT32)0x3333 << 4) + 0x1234) & 0xfffff;

	setup_instruction(instruction, NELEMENTS(instruction), 0x7acd, initial_flags);
	mem[address] = 0xab;
	upd9002_core_step();
	if ((CPU_AL != 0xda) || (mem[address] != 0xbd) || (CPU_AH != 0x7a) ||
	    (CPU_FLAG != initial_flags) || (CPU_IP != 0x0106)) {
		fprintf(stderr, "upd9002-m62: ROL4 memory result differs\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_daa_das_semantics(void) {
	static const DECIMAL_ADJUST_CASE cases[] = {
	    {0x27, 0x09, 0x00, 0x09, 0xf406},   {0x27, 0x0a, 0x00, 0x10, 0xf412},
	    {0x27, 0x7a, 0x00, 0x80, 0xfc92},   {0x27, 0x99, 0x00, 0x99, 0xf486},
	    {0x27, 0x9a, 0x00, 0x00, 0xf457},   {0x27, 0x9a, A_FLAG, 0xa0, 0xf496},
	    {0x27, 0xa0, A_FLAG, 0x06, 0xf417}, {0x2f, 0x00, 0x00, 0x00, 0xf446},
	    {0x2f, 0x0a, 0x00, 0x04, 0xf412},   {0x2f, 0x7a, 0x00, 0x74, 0xf416},
	    {0x2f, 0x99, 0x00, 0x99, 0xf486},   {0x2f, 0x9a, 0x00, 0x34, 0xfc13},
	    {0x2f, 0x9a, A_FLAG, 0x94, 0xf492}, {0x2f, 0xa0, A_FLAG, 0x3a, 0xfc17}};
	UINT index;

	for (index = 0; index < NELEMENTS(cases); index++) {
		const DECIMAL_ADJUST_CASE *const value = &cases[index];
		const UINT8 instruction[] = {value->opcode};

		setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(0x5a00 | value->initial_al),
		                  (UINT16)(0xf402 | value->input_flags));
		upd9002_core_step();
		if ((CPU_AL != value->expected_al) || (CPU_AH != 0x5a) ||
		    (CPU_FLAG != value->expected_flags) || (CPU_IP != 0x0101)) {
			fprintf(stderr, "upd9002-m62: decimal adjust %02x case %u differs\n", value->opcode,
			        index);
			return FAILURE;
		}
	}
	return SUCCESS;
}

static int test_aaa_aas_semantics(void) {
	static const ASCII_ADJUST_CASE cases[] = {
	    {0x37, 0x09, 0x00, 0x5a09, 0xf406},   {0x37, 0x0a, 0x00, 0x5b00, 0xf413},
	    {0x37, 0x40, 0x00, 0x5a00, 0xf402},   {0x37, 0x7a, 0x00, 0x5b00, 0xfc93},
	    {0x37, 0x7f, 0x00, 0x5b05, 0xfc93},   {0x37, 0xfa, 0x00, 0x5b00, 0xf457},
	    {0x37, 0x18, A_FLAG, 0x5b0e, 0xf417}, {0x3f, 0x09, 0x00, 0x5a09, 0xf406},
	    {0x3f, 0x0a, 0x00, 0x5904, 0xf413},   {0x3f, 0x56, 0x00, 0x5a06, 0xf406},
	    {0x3f, 0x7c, 0x00, 0x5906, 0xf413},   {0x3f, 0x80, A_FLAG, 0x590a, 0xfc13},
	    {0x3f, 0x85, A_FLAG, 0x590f, 0xfc13}, {0x3f, 0x00, A_FLAG, 0x590a, 0xf497},
	    {0x3f, 0x56, A_FLAG, 0x5900, 0xf417}};
	UINT index;

	for (index = 0; index < NELEMENTS(cases); index++) {
		const ASCII_ADJUST_CASE *const value = &cases[index];
		const UINT8 instruction[] = {value->opcode};

		setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(0x5a00 | value->initial_al),
		                  (UINT16)(0xf402 | value->input_flags));
		upd9002_core_step();
		if ((CPU_AX != value->expected_ax) || (CPU_FLAG != value->expected_flags) ||
		    (CPU_IP != 0x0101)) {
			fprintf(stderr, "upd9002-m62: ASCII adjust %02x case %u differs\n", value->opcode,
			        index);
			return FAILURE;
		}
	}
	return SUCCESS;
}

static int test_shift_registers(void) {
	static const SHIFT_CASE cases[] = {
	    {0xc0, 4, 0, 0x0095, 0x0095, 0xfcd7},   {0xc0, 4, 1, 0x0095, 0x002a, 0xfc03},
	    {0xc0, 4, 2, 0x0095, 0x0054, 0xf402},   {0xc0, 4, 7, 0x0095, 0x0080, 0xfc82},
	    {0xc0, 4, 8, 0x0095, 0x0000, 0xfc47},   {0xc0, 4, 9, 0x0095, 0x0000, 0xf446},
	    {0xc0, 4, 31, 0x0095, 0x0000, 0xf446},  {0xc0, 4, 32, 0x0095, 0x0000, 0xf446},
	    {0xc0, 4, 33, 0x0095, 0x0000, 0xf446},  {0xc0, 4, 255, 0x0095, 0x0000, 0xf446},
	    {0xc0, 5, 1, 0x0095, 0x004a, 0xfc03},   {0xc0, 5, 2, 0x0095, 0x0025, 0xf402},
	    {0xc0, 5, 8, 0x0095, 0x0000, 0xf447},   {0xc0, 5, 9, 0x0095, 0x0000, 0xf446},
	    {0xc0, 6, 1, 0x0095, 0x002a, 0xfc03},   {0xc0, 6, 9, 0x0095, 0x0000, 0xf446},
	    {0xc0, 7, 1, 0x0095, 0x00ca, 0xf487},   {0xc0, 7, 2, 0x0095, 0x00e5, 0xf482},
	    {0xc0, 7, 8, 0x0095, 0x00ff, 0xf487},   {0xc0, 7, 9, 0x0095, 0x00ff, 0xf487},
	    {0xc1, 4, 0, 0x95a5, 0x95a5, 0xfcd7},   {0xc1, 4, 1, 0x95a5, 0x2b4a, 0xfc03},
	    {0xc1, 4, 2, 0x95a5, 0x5694, 0xf402},   {0xc1, 4, 15, 0x95a5, 0x8000, 0xfc86},
	    {0xc1, 4, 16, 0x95a5, 0x0000, 0xfc47},  {0xc1, 4, 17, 0x95a5, 0x0000, 0xf446},
	    {0xc1, 4, 31, 0x95a5, 0x0000, 0xf446},  {0xc1, 4, 32, 0x95a5, 0x0000, 0xf446},
	    {0xc1, 4, 33, 0x95a5, 0x0000, 0xf446},  {0xc1, 4, 255, 0x95a5, 0x0000, 0xf446},
	    {0xc1, 5, 1, 0x95a5, 0x4ad2, 0xfc07},   {0xc1, 5, 2, 0x95a5, 0x2569, 0xf406},
	    {0xc1, 5, 16, 0x95a5, 0x0000, 0xf447},  {0xc1, 5, 17, 0x95a5, 0x0000, 0xf446},
	    {0xc1, 6, 1, 0x95a5, 0x2b4a, 0xfc03},   {0xc1, 6, 17, 0x95a5, 0x0000, 0xf446},
	    {0xc1, 7, 1, 0x95a5, 0xcad2, 0xf487},   {0xc1, 7, 2, 0x95a5, 0xe569, 0xf486},
	    {0xc1, 7, 16, 0x95a5, 0xffff, 0xf487},  {0xc1, 7, 17, 0x95a5, 0xffff, 0xf487},
	    {0xd2, 4, 33, 0x00d3, 0x0000, 0xf446},  {0xd2, 5, 31, 0x00d3, 0x0000, 0xf446},
	    {0xd2, 6, 255, 0x00d3, 0x0000, 0xf446}, {0xd2, 7, 32, 0x00d3, 0x00ff, 0xf487},
	    {0xd2, 7, 9, 0x0035, 0x0000, 0xf446},   {0xd3, 4, 33, 0xd357, 0x0000, 0xf446},
	    {0xd3, 5, 31, 0xd357, 0x0000, 0xf446},  {0xd3, 6, 255, 0xd357, 0x0000, 0xf446},
	    {0xd3, 7, 32, 0xd357, 0xffff, 0xf487},  {0xd3, 7, 17, 0x3579, 0x0000, 0xf446}};
	UINT index;

	for (index = 0; index < NELEMENTS(cases); index++) {
		const SHIFT_CASE *const value = &cases[index];
		UINT8 instruction[3] = {value->opcode, (UINT8)(0xc0 | (value->subform << 3)), value->count};
		const BOOL immediate = ((value->opcode == 0xc0) || (value->opcode == 0xc1));
		const BOOL word = ((value->opcode == 0xc1) || (value->opcode == 0xd3));
		const UINT length = immediate ? 3 : 2;

		setup_instruction(instruction, length, value->initial_destination, 0xfcd7);
		if (!immediate) {
			CPU_CL = value->count;
		}
		upd9002_core_step();
		if (((word ? CPU_AX : CPU_AL) != value->expected_destination) ||
		    (!word && (CPU_AH != (UINT8)(value->initial_destination >> 8))) ||
		    (CPU_FLAG != value->expected_flags) || (CPU_IP != (UINT16)(0x0100 + length))) {
			fprintf(stderr, "upd9002-m62: shift register case %u differs\n", index);
			return FAILURE;
		}
	}
	return SUCCESS;
}

static int test_shift_memory_and_rotate_protection(void) {
	static const UINT8 c0_memory[] = {0xc0, 0x26, 0x34, 0x12, 0x09};
	static const UINT8 c1_memory[] = {0xc1, 0x3e, 0x34, 0x12, 0x11};
	static const UINT8 d2_physical_wrap[] = {0x26, 0xd2, 0x2e, 0x10, 0x00};
	static const UINT8 d3_offset_wrap[] = {0xd3, 0x20};
	const UINT32 direct = (((UINT32)0x3333 << 4) + 0x1234) & 0xfffff;
	UINT subform;

	setup_instruction(c0_memory, NELEMENTS(c0_memory), 0x5a00, 0xfcd7);
	mem[direct] = 0x95;
	upd9002_core_step();
	if ((mem[direct] != 0x00) || (CPU_FLAG != 0xf446) || (CPU_IP != 0x0105)) {
		fprintf(stderr, "upd9002-m62: C0 memory shift differs\n");
		return FAILURE;
	}

	setup_instruction(c1_memory, NELEMENTS(c1_memory), 0x5a00, 0xfcd7);
	mem[direct] = 0xa5;
	mem[(direct + 1) & 0xfffff] = 0x95;
	upd9002_core_step();
	if ((mem[direct] != 0xff) || (mem[(direct + 1) & 0xfffff] != 0xff) || (CPU_FLAG != 0xf487) ||
	    (CPU_IP != 0x0105)) {
		fprintf(stderr, "upd9002-m62: C1 memory shift differs: %02x %02x %04x %04x\n", mem[direct],
		        mem[(direct + 1) & 0xfffff], CPU_FLAG, CPU_IP);
		return FAILURE;
	}

	setup_instruction(d2_physical_wrap, NELEMENTS(d2_physical_wrap), 0x5a00, 0xfcd7);
	CPU_ES = 0xffff;
	ES_BASE = (UINT32)CPU_ES << 4;
	CPU_CL = 8;
	mem[0] = 0x95;
	upd9002_core_step();
	if ((mem[0] != 0x00) || (CPU_FLAG != 0xf447) || (CPU_IP != 0x0105)) {
		fprintf(stderr, "upd9002-m62: D2 physical-wrap shift differs\n");
		return FAILURE;
	}

	setup_instruction(d3_offset_wrap, NELEMENTS(d3_offset_wrap), 0x5a00, 0xfcd7);
	CPU_BX = 0xffff;
	CPU_SI = 0x0002;
	CPU_CL = 16;
	mem[(DS_BASE + 1) & 0xfffff] = 0xa5;
	mem[(DS_BASE + 2) & 0xfffff] = 0x95;
	upd9002_core_step();
	if ((mem[(DS_BASE + 1) & 0xfffff] != 0x00) || (mem[(DS_BASE + 2) & 0xfffff] != 0x00) ||
	    (CPU_FLAG != 0xfc47) || (CPU_IP != 0x0102)) {
		fprintf(stderr, "upd9002-m62: D3 offset-wrap shift differs\n");
		return FAILURE;
	}

	for (subform = 0; subform < 4; subform++) {
		const UINT8 instruction[] = {0xc0, (UINT8)(0xc0 | (subform << 3)), 0x00};

		setup_instruction(instruction, NELEMENTS(instruction), 0x5a95, 0xfcd7);
		upd9002_core_step();
		if ((CPU_AX != 0x5a95) || (CPU_FLAG != 0xfcd7) || (CPU_IP != 0x0103)) {
			fprintf(stderr, "upd9002-m62: rotate subform %u count-zero changed\n", subform);
			return FAILURE;
		}
	}
	return SUCCESS;
}

int upd9002_semantics_bundle_main(void) {
	upd9002_test_flat_memory_set(TRUE);

	upd9002_core_initialize();
	if ((test_evidence_oracles() != SUCCESS) || (test_aam_semantics() != SUCCESS) ||
	    (test_ror4_registers() != SUCCESS) || (test_ror4_memory() != SUCCESS) ||
	    (test_rol4_registers() != SUCCESS) || (test_rol4_memory() != SUCCESS) ||
	    (test_daa_das_semantics() != SUCCESS) || (test_aaa_aas_semantics() != SUCCESS) ||
	    (test_shift_registers() != SUCCESS) ||
	    (test_shift_memory_and_rotate_protection() != SUCCESS)) {
		upd9002_core_deinitialize();
		upd9002_test_flat_memory_set(FALSE);
		return FAILURE;
	}
	upd9002_core_deinitialize();
	upd9002_test_flat_memory_set(FALSE);
	puts("upd9002-m62-semantics-bundle: audit infrastructure passed");
	return SUCCESS;
}
