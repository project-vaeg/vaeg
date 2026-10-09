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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "tests/upd9002/m65d_ff6.h"

#include <stdio.h>

enum {
	kMemoryOperandOffset = 0x0120
};

static void setup_instruction(const UINT8 *instruction, UINT length) {
	UINT index;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0x3456;
	CPU_CX = 0x789a;
	CPU_DX = 0xbcde;
	CPU_BX = 0x1357;
	CPU_SP = 0x8000;
	CPU_BP = 0x2468;
	CPU_SI = 0x0123;
	CPU_DI = 0x4567;
	CPU_ES = 0x1111;
	CPU_CS = 0x2000;
	CPU_SS = 0x3000;
	CPU_DS = 0x3333;
	CPU_IP = 0x0100;
	CPU_FLAG = 0xf046;
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

static UINT16 read_word(UINT32 address) {
	return (UINT16)(mem[address & CPU_ADRSMASK] | (mem[(address + 1) & CPU_ADRSMASK] << 8));
}

static void write_word(UINT32 address, UINT16 value) {
	mem[address & CPU_ADRSMASK] = (UINT8)value;
	mem[(address + 1) & CPU_ADRSMASK] = (UINT8)(value >> 8);
}

static int expect_common_state(UINT16 expected_ip, UINT16 expected_sp) {
	if ((CPU_AX != 0x3456) || (CPU_CX != 0x789a) || (CPU_DX != 0xbcde) || (CPU_BX != 0x1357) ||
	    (CPU_BP != 0x2468) || (CPU_SI != 0x0123) || (CPU_DI != 0x4567) || (CPU_CS != 0x2000) ||
	    (CPU_IP != expected_ip) || (CPU_SP != expected_sp) || (CPU_FLAG != 0xf046)) {
		fprintf(stderr, "upd9002-m65d-ff6: CPU state differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_ff6_sp_alias_pushes_decremented_sp(void) {
	static const UINT8 instruction[] = {0xff, 0xf4};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_SP = 0x6000;
	upd9002_core_step();
	if ((read_word(SS_BASE + 0x5ffe) != 0x5ffe) ||
	    (expect_common_state(0x0102, 0x5ffe) != SUCCESS)) {
		fprintf(stderr, "upd9002-m65d-ff6: SP alias did not push decremented SP\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_ff6_sp_alias_prefix_does_not_change_stack_segment(void) {
	static const UINT8 instruction[] = {0x26, 0xff, 0xf4};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_SP = 0x7000;
	write_word(ES_BASE + 0x6ffe, 0xa55a);
	upd9002_core_step();
	if ((read_word(SS_BASE + 0x6ffe) != 0x6ffe) || (read_word(ES_BASE + 0x6ffe) != 0xa55a) ||
	    (expect_common_state(0x0103, 0x6ffe) != SUCCESS)) {
		fprintf(stderr, "upd9002-m65d-ff6: prefixed SP alias stack result differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_ff6_other_register_uses_source_value(void) {
	static const UINT8 instruction[] = {0xff, 0xf0};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_SP = 0x6000;
	upd9002_core_step();
	if ((read_word(SS_BASE + 0x5ffe) != 0x3456) ||
	    (expect_common_state(0x0102, 0x5ffe) != SUCCESS)) {
		fprintf(stderr, "upd9002-m65d-ff6: non-SP register push differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_ff6_memory_operand_pushes_source_without_writing_operand(void) {
	static const UINT8 instruction[] = {0xff, 0x36, 0x20, 0x01};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_SP = 0x6000;
	write_word(DS_BASE + kMemoryOperandOffset, 0xbeef);
	upd9002_core_step();
	if ((read_word(SS_BASE + 0x5ffe) != 0xbeef) ||
	    (read_word(DS_BASE + kMemoryOperandOffset) != 0xbeef) ||
	    (expect_common_state(0x0104, 0x5ffe) != SUCCESS)) {
		fprintf(stderr, "upd9002-m65d-ff6: memory source push differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_ff6_sp_alias_stack_wrap(void) {
	static const UINT8 instruction[] = {0xff, 0xf4};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_SP = 0x0000;
	upd9002_core_step();
	if ((read_word(SS_BASE + 0xfffe) != 0xfffe) ||
	    (expect_common_state(0x0102, 0xfffe) != SUCCESS)) {
		fprintf(stderr, "upd9002-m65d-ff6: SP alias stack wrap differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_m65d_ff6_main(void) {
	upd9002_core_initialize();
	if ((test_ff6_sp_alias_pushes_decremented_sp() != SUCCESS) ||
	    (test_ff6_sp_alias_prefix_does_not_change_stack_segment() != SUCCESS) ||
	    (test_ff6_other_register_uses_source_value() != SUCCESS) ||
	    (test_ff6_memory_operand_pushes_source_without_writing_operand() != SUCCESS) ||
	    (test_ff6_sp_alias_stack_wrap() != SUCCESS)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_deinitialize();
	puts("upd9002-m65d-ff6: FF /6 focused checks passed");
	return SUCCESS;
}
