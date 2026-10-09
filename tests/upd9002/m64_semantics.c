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
#include "upd9002_ops.h"
#include "tests/upd9002/m64_semantics.h"

#include <stdio.h>

static void setup_instruction(const UINT8 *instruction, UINT length, UINT16 flags) {
	UINT index;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0;
	CPU_CX = 0x2468;
	CPU_DX = 0;
	CPU_BX = 0;
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
	mem[0] = 0x00;
	mem[1] = 0x04;
	mem[2] = 0x00;
	mem[3] = 0x00;
	for (index = 0; index < length; index++) {
		mem[(CS_BASE + CPU_IP + index) & CPU_ADRSMASK] = instruction[index];
	}
}

static UINT16 stack_word(UINT16 offset) {
	const UINT32 low = (SS_BASE + offset) & 0xfffff;
	const UINT32 high = (SS_BASE + LOW16(offset + 1)) & 0xfffff;

	return (UINT16)(mem[low] | (mem[high] << 8));
}

static int require_type0(UINT16 return_ip, UINT16 saved_flags, const char *name) {
	if ((CPU_CS != 0) || (CPU_IP != 0x0400) || (CPU_SP != 0x7ffa) ||
	    (stack_word(0x7ffa) != return_ip) || (stack_word(0x7ffc) != 0x2000) ||
	    (stack_word(0x7ffe) != saved_flags)) {
		fprintf(stderr, "upd9002-m64: %s type-0 frame differs\n", name);
		return FAILURE;
	}
	return SUCCESS;
}

static int test_div8(void) {
	static const UINT8 instruction[] = {0xf6, 0xf3};

	setup_instruction(instruction, NELEMENTS(instruction), 0xf002);
	CPU_AX = 0x1234;
	CPU_BL = 0x56;
	upd9002_core_step();
	if ((CPU_AX != 0x1036) || (CPU_IP != 0x0102) || (CPU_FLAG != 0xf093)) {
		fprintf(stderr, "upd9002-m64: F6 /6 normal result differs\n");
		return FAILURE;
	}

	setup_instruction(instruction, NELEMENTS(instruction), 0xf002);
	CPU_AX = 0x5634;
	CPU_BL = 0x56;
	upd9002_core_step();
	if (require_type0(0x0102, 0xf046, "F6 /6 overflow") != SUCCESS) {
		return FAILURE;
	}

	setup_instruction(instruction, NELEMENTS(instruction), 0xf002);
	CPU_AX = 0x1234;
	CPU_BL = 0;
	upd9002_core_step();
	if (require_type0(0x0102, 0xf006, "F6 /6 zero divisor") != SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_idiv8(void) {
	static const UINT8 instruction[] = {0xf6, 0xfb};

	setup_instruction(instruction, NELEMENTS(instruction), 0xf002);
	CPU_AX = 0xff81;
	CPU_BL = 2;
	upd9002_core_step();
	if ((CPU_AL != 0xc1) || (CPU_AH != 0xff) || (CPU_IP != 0x0102) || (CPU_FLAG != 0xf006)) {
		fprintf(stderr, "upd9002-m64: F6 /7 signed result differs\n");
		return FAILURE;
	}

	setup_instruction(instruction, NELEMENTS(instruction), 0xf002);
	CPU_AX = 0xff80;
	CPU_BL = 1;
	upd9002_core_step();
	if (require_type0(0x0102, 0xf082, "F6 /7 minimum quotient") != SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_div16(void) {
	static const UINT8 instruction[] = {0xf7, 0xf3};
	static const UINT8 wrapped[] = {0x26, 0xf7, 0xb4, 0xa0, 0xc2};

	setup_instruction(instruction, NELEMENTS(instruction), 0xf002);
	CPU_DX = 1;
	CPU_AX = 0;
	CPU_BX = 3;
	upd9002_core_step();
	if ((CPU_AX != 0x5555) || (CPU_DX != 1) || (CPU_IP != 0x0102) || (CPU_FLAG != 0xf093)) {
		fprintf(stderr, "upd9002-m64: F7 /6 normal result differs\n");
		return FAILURE;
	}

	setup_instruction(wrapped, NELEMENTS(wrapped), 0xfc16);
	CPU_ES = 0x8bb0;
	ES_BASE = (UINT32)CPU_ES << 4;
	CPU_SI = 0x3d5f;
	CPU_DX = 0x3ef8;
	CPU_AX = 0x2e13;
	mem[(ES_BASE + 0xffff) & 0xfffff] = 0xc5;
	mem[ES_BASE & 0xfffff] = 0x84;
	upd9002_core_step();
	if ((CPU_AX != 0x796a) || (CPU_DX != 0x1781) || (CPU_IP != 0x0105)) {
		fprintf(stderr, "upd9002-m64: F7 /6 segment-wrap read differs\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_idiv16(void) {
	static const UINT8 instruction[] = {0xf7, 0xfb};

	setup_instruction(instruction, NELEMENTS(instruction), 0xf002);
	CPU_DX = 0x8000;
	CPU_AX = 0;
	CPU_BX = 0xffff;
	upd9002_core_step();
	if (require_type0(0x0102, 0xf816, "F7 /7 widened overflow") != SUCCESS) {
		return FAILURE;
	}

	setup_instruction(instruction, NELEMENTS(instruction), 0xf002);
	CPU_DX = 0xffff;
	CPU_AX = 0x8000;
	CPU_BX = 1;
	upd9002_core_step();
	if (require_type0(0x0102, 0xf086, "F7 /7 minimum quotient") != SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_packed_decimal_strings(void) {
	static const UINT8 add4s[] = {0x0f, 0x20};
	static const UINT8 sub4s[] = {0x0f, 0x22};
	static const UINT8 cmp4s[] = {0x0f, 0x26};
	UINT32 src0;
	UINT32 src1;
	UINT32 dst0;
	UINT32 dst1;

	setup_instruction(add4s, NELEMENTS(add4s), 0xf002);
	CPU_CL = 3;
	CPU_SI = 0xffff;
	CPU_DI = 0xffff;
	src0 = (DS_BASE + 0xffff) & 0xfffff;
	src1 = DS_BASE & 0xfffff;
	dst0 = (ES_BASE + 0xffff) & 0xfffff;
	dst1 = ES_BASE & 0xfffff;
	mem[src0] = 0x5f;
	mem[src1] = 0x87;
	mem[dst0] = 0x3e;
	mem[dst1] = 0x58;
	upd9002_core_step();
	if ((mem[dst0] != 0xa3) || (mem[dst1] != 0x45) || (CPU_FLAG != 0xf093) || (CPU_IP != 0x0102)) {
		fprintf(stderr, "upd9002-m64: ADD4S result or wrap differs\n");
		return FAILURE;
	}

	setup_instruction(sub4s, NELEMENTS(sub4s), 0xf002);
	CPU_CL = 3;
	CPU_SI = 0x1234;
	CPU_DI = 0x5678;
	src0 = (DS_BASE + CPU_SI) & 0xfffff;
	src1 = (src0 + 1) & 0xfffff;
	dst0 = (ES_BASE + CPU_DI) & 0xfffff;
	dst1 = (dst0 + 1) & 0xfffff;
	mem[src0] = 0x12;
	mem[src1] = 0x34;
	mem[dst0] = 0x56;
	mem[dst1] = 0x78;
	upd9002_core_step();
	if ((mem[dst0] != 0x44) || (mem[dst1] != 0x44) || (CPU_FLAG != 0xf002) || (CPU_IP != 0x0102)) {
		fprintf(stderr, "upd9002-m64: SUB4S result differs\n");
		return FAILURE;
	}

	setup_instruction(cmp4s, NELEMENTS(cmp4s), 0xf002);
	CPU_CL = 3;
	CPU_SI = 0x1234;
	CPU_DI = 0x5678;
	src0 = (DS_BASE + CPU_SI) & 0xfffff;
	src1 = (src0 + 1) & 0xfffff;
	dst0 = (ES_BASE + CPU_DI) & 0xfffff;
	dst1 = (dst0 + 1) & 0xfffff;
	mem[src0] = 0x12;
	mem[src1] = 0x34;
	mem[dst0] = 0x56;
	mem[dst1] = 0x78;
	upd9002_core_step();
	if ((mem[dst0] != 0x56) || (mem[dst1] != 0x78) || (CPU_FLAG != 0xf002) || (CPU_IP != 0x0102)) {
		fprintf(stderr, "upd9002-m64: CMP4S result or write protection differs\n");
		return FAILURE;
	}
	return SUCCESS;
}

typedef struct {
	UINT8 opcode;
	UINT8 bit;
	UINT16 initial_ax;
	UINT16 expected_ax;
	UINT16 expected_flag_mask;
	BOOL immediate;
	BOOL test_only;
} BIT_OPERATION_CASE;

static int test_bit_operations(void) {
	static const BIT_OPERATION_CASE cases[] = {{0x10, 7, 0x55aa, 0x55aa, 0x0080, FALSE, TRUE},
	                                           {0x11, 15, 0xd5aa, 0xd5aa, 0x0084, FALSE, TRUE},
	                                           {0x12, 7, 0x55aa, 0x552a, 0x00d5, FALSE, FALSE},
	                                           {0x13, 15, 0xd5aa, 0x55aa, 0x00d5, FALSE, FALSE},
	                                           {0x14, 7, 0x552a, 0x55aa, 0x00d5, FALSE, FALSE},
	                                           {0x15, 15, 0x55aa, 0xd5aa, 0x00d5, FALSE, FALSE},
	                                           {0x16, 7, 0x55aa, 0x552a, 0x00d5, FALSE, FALSE},
	                                           {0x17, 15, 0xd5aa, 0x55aa, 0x00d5, FALSE, FALSE},
	                                           {0x18, 7, 0x55aa, 0x55aa, 0x0080, TRUE, TRUE},
	                                           {0x19, 15, 0xd5aa, 0xd5aa, 0x0084, TRUE, TRUE},
	                                           {0x1a, 7, 0x55aa, 0x552a, 0x00d5, TRUE, FALSE},
	                                           {0x1b, 15, 0xd5aa, 0x55aa, 0x00d5, TRUE, FALSE},
	                                           {0x1c, 7, 0x552a, 0x55aa, 0x00d5, TRUE, FALSE},
	                                           {0x1d, 15, 0x55aa, 0xd5aa, 0x00d5, TRUE, FALSE},
	                                           {0x1e, 7, 0x55aa, 0x552a, 0x00d5, TRUE, FALSE},
	                                           {0x1f, 15, 0xd5aa, 0x55aa, 0x00d5, TRUE, FALSE}};
	UINT index;

	for (index = 0; index < NELEMENTS(cases); index++) {
		const BIT_OPERATION_CASE *const value = &cases[index];
		UINT8 instruction[] = {0x0f, value->opcode, 0xc0, value->bit};
		const UINT length = value->immediate ? 4 : 3;

		setup_instruction(instruction, length, 0xfcd7);
		CPU_AX = value->initial_ax;
		if (!value->immediate) {
			CPU_CL = value->bit;
		}
		upd9002_core_step();
		if ((CPU_AX != value->expected_ax) || (CPU_IP != (UINT16)(0x0100 + length)) ||
		    ((CPU_FLAG & 0x00d5) != value->expected_flag_mask) ||
		    (value->test_only && UPD9002_OV) || (!value->test_only && (CPU_FLAG != 0xfcd7))) {
			fprintf(stderr, "upd9002-m64: bit operation 0F%02X differs\n", value->opcode);
			return FAILURE;
		}
	}
	return SUCCESS;
}

int upd9002_m64_semantics_main(void) {
	upd9002_core_initialize();
	if ((test_div8() != SUCCESS) || (test_idiv8() != SUCCESS) || (test_div16() != SUCCESS) ||
	    (test_idiv16() != SUCCESS) || (test_packed_decimal_strings() != SUCCESS) ||
	    (test_bit_operations() != SUCCESS)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_deinitialize();
	puts("upd9002-m64-semantics: arithmetic and 0F checks passed");
	return SUCCESS;
}
