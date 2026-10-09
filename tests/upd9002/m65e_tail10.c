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
#include "tests/upd9002/m65e_tail10.h"

#include <stdio.h>

static void setup_instruction(const UINT8 *instruction, UINT length) {
	UINT index;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0x1111;
	CPU_CX = 0x2222;
	CPU_DX = 0x3333;
	CPU_BX = 0x0100;
	CPU_SP = 0x8000;
	CPU_BP = 0x0200;
	CPU_SI = 0x0300;
	CPU_DI = 0x0400;
	CPU_ES = 0x3000;
	CPU_CS = 0x2000;
	CPU_SS = 0x4000;
	CPU_DS = 0x5000;
	CPU_IP = 0x0100;
	CPU_FLAG = 0xf002;
	ES_BASE = (UINT32)CPU_ES << 4;
	CS_BASE = (UINT32)CPU_CS << 4;
	SS_BASE = (UINT32)CPU_SS << 4;
	DS_BASE = (UINT32)CPU_DS << 4;
	upd9002_core_context.s.ss_fix = SS_BASE;
	upd9002_core_context.s.ds_fix = DS_BASE;
	CPU_ADRSMASK = 0x000fffff;
	CPU_REMCLOCK = 10000;
	CPU_BASECLOCK = 10000;
	CPU_CLOCK = 0;
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
	for (index = 0; index < length; index++) {
		mem[(CS_BASE + CPU_IP + index) & CPU_ADRSMASK] = instruction[index];
	}
}

static UINT8 read_byte(UINT32 base, UINT16 offset) {
	return mem[(base + offset) & CPU_ADRSMASK];
}

static void write_byte(UINT32 base, UINT16 offset, UINT8 value) {
	mem[(base + offset) & CPU_ADRSMASK] = value;
}

static int test_popa_reads_wrapped_stack_words(void) {
	static const UINT8 instruction[] = {0x61};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_SP = 0xfff1;
	write_byte(SS_BASE, 0xfff1, 0x34);
	write_byte(SS_BASE, 0xfff2, 0x12);
	write_byte(SS_BASE, 0xffff, 0xcd);
	write_byte(SS_BASE, 0x0000, 0xab);
	upd9002_core_step();
	if ((CPU_DI != 0x1234) || (CPU_AX != 0xabcd) || (CPU_SP != 0x0001) || (CPU_IP != 0x0101)) {
		fprintf(stderr, "upd9002-m65e-tail10: POPA wrap differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_pushf_writes_wrapped_stack_word(void) {
	static const UINT8 instruction[] = {0x9c};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_SP = 0x0001;
	CPU_FLAG = 0xf0d6;
	upd9002_core_step();
	if ((CPU_SP != 0xffff) || (read_byte(SS_BASE, 0xffff) != 0xd6) ||
	    (read_byte(SS_BASE, 0x0000) != 0xf0)) {
		fprintf(stderr, "upd9002-m65e-tail10: PUSHF wrap differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_les_reads_wrapped_pointer_word(void) {
	static const UINT8 instruction[] = {0xc4, 0x06, 0xff, 0xff};

	setup_instruction(instruction, NELEMENTS(instruction));
	write_byte(DS_BASE, 0xffff, 0x83);
	write_byte(DS_BASE, 0x0000, 0x79);
	write_byte(DS_BASE, 0x0001, 0x16);
	write_byte(DS_BASE, 0x0002, 0x5b);
	upd9002_core_step();
	if ((CPU_AX != 0x7983) || (CPU_ES != 0x5b16) || (CPU_IP != 0x0104)) {
		fprintf(stderr, "upd9002-m65e-tail10: LES wrap differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_rep_movsw_writes_wrapped_destination_word(void) {
	static const UINT8 instruction[] = {0xf3, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_CX = 1;
	CPU_SI = 0x0100;
	CPU_DI = 0xffff;
	write_byte(DS_BASE, 0x0100, 0x5a);
	write_byte(DS_BASE, 0x0101, 0xa5);
	upd9002_core_step();
	if ((CPU_CX != 0) || (read_byte(ES_BASE, 0xffff) != 0x5a) ||
	    (read_byte(ES_BASE, 0x0000) != 0xa5)) {
		fprintf(stderr, "upd9002-m65e-tail10: REP MOVSW wrap differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_xor_word_rmw_wraps_second_byte(void) {
	static const UINT8 instruction[] = {0x81, 0x36, 0xff, 0xff, 0x09, 0x70};

	setup_instruction(instruction, NELEMENTS(instruction));
	write_byte(DS_BASE, 0xffff, 0xcc);
	write_byte(DS_BASE, 0x0000, 0x84);
	upd9002_core_step();
	if ((read_byte(DS_BASE, 0xffff) != 0xc5) || (read_byte(DS_BASE, 0x0000) != 0xf4)) {
		fprintf(stderr, "upd9002-m65e-tail10: XOR word wrap differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_shift_word_rmw_wraps_second_byte(void) {
	static const UINT8 instruction[] = {0xd1, 0x36, 0xff, 0xff};

	setup_instruction(instruction, NELEMENTS(instruction));
	write_byte(DS_BASE, 0xffff, 0x8a);
	write_byte(DS_BASE, 0x0000, 0x84);
	upd9002_core_step();
	if ((read_byte(DS_BASE, 0xffff) != 0x14) || (read_byte(DS_BASE, 0x0000) != 0x09)) {
		fprintf(stderr, "upd9002-m65e-tail10: shift word wrap differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_m65e_tail10_main(void) {
	upd9002_core_initialize();
	if ((test_popa_reads_wrapped_stack_words() != SUCCESS) ||
	    (test_pushf_writes_wrapped_stack_word() != SUCCESS) ||
	    (test_les_reads_wrapped_pointer_word() != SUCCESS) ||
	    (test_rep_movsw_writes_wrapped_destination_word() != SUCCESS) ||
	    (test_xor_word_rmw_wraps_second_byte() != SUCCESS) ||
	    (test_shift_word_rmw_wraps_second_byte() != SUCCESS)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_deinitialize();
	puts("upd9002-m65e-tail10: wrapped word focused checks passed");
	return SUCCESS;
}
