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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "tests/upd9002/m65a_ff7.h"

#include <stdio.h>

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

static int test_ff7_register_pushes_source(void) {
	static const UINT8 instruction[] = {0xff, 0xf8};

	setup_instruction(instruction, NELEMENTS(instruction));
	upd9002_core_step();
	if ((CPU_SP != 0x7ffe) || (read_word(SS_BASE + 0x7ffe) != 0x3456) || (CPU_AX != 0x3456) ||
	    (CPU_IP != 0x0102) || (CPU_FLAG != 0xf046)) {
		fprintf(stderr, "upd9002-m65a-ff7: register source push differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_ff7_sp_alias_pushes_decremented_sp(void) {
	static const UINT8 instruction[] = {0xff, 0xfc};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_SP = 0x6000;
	upd9002_core_step();
	if ((CPU_SP != 0x5ffe) || (read_word(SS_BASE + 0x5ffe) != 0x5ffe) || (CPU_IP != 0x0102) ||
	    (CPU_FLAG != 0xf046)) {
		fprintf(stderr, "upd9002-m65a-ff7: SP alias did not push decremented SP\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_ff7_memory_pushes_source_without_writing_operand(void) {
	static const UINT8 instruction[] = {0xff, 0x3e, 0x20, 0x01};
	const UINT32 operand = 0x0120;

	setup_instruction(instruction, NELEMENTS(instruction));
	write_word(DS_BASE + operand, 0xbeef);
	upd9002_core_step();
	if ((CPU_SP != 0x7ffe) || (read_word(SS_BASE + 0x7ffe) != 0xbeef) ||
	    (read_word(DS_BASE + operand) != 0xbeef) || (CPU_IP != 0x0104) || (CPU_FLAG != 0xf046)) {
		fprintf(stderr, "upd9002-m65a-ff7: memory source push differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_m65a_ff7_main(void) {
	upd9002_core_initialize();
	if ((test_ff7_register_pushes_source() != SUCCESS) ||
	    (test_ff7_sp_alias_pushes_decremented_sp() != SUCCESS) ||
	    (test_ff7_memory_pushes_source_without_writing_operand() != SUCCESS)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_deinitialize();
	puts("upd9002-m65a-ff7: FF /7 focused checks passed");
	return SUCCESS;
}
