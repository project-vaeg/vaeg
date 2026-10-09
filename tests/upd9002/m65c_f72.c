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
#include "tests/upd9002/m65c_f72.h"

#include <stdio.h>

enum {
	kDirectOffset = 0x0200,
	kNeighborLow = 0x5a,
	kNeighborHigh = 0xa5
};

static UINT32 physical_address(UINT16 segment, UINT16 offset) {
	return ((((UINT32)segment << 4) + offset) & 0x000fffff);
}

static void setup_instruction(const UINT8 *instruction, UINT length) {
	UINT index;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0x1357;
	CPU_CX = 0x2468;
	CPU_DX = 0x369a;
	CPU_BX = 0x0200;
	CPU_SP = 0x8000;
	CPU_BP = 0x0400;
	CPU_SI = 0x0004;
	CPU_DI = 0x0010;
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

static UINT8 read_byte(UINT32 address) {
	return mem[address & CPU_ADRSMASK];
}

static UINT16 read_word_physical(UINT32 address) {
	return (UINT16)(read_byte(address) | (read_byte(address + 1) << 8));
}

static void write_word_physical(UINT32 address, UINT16 value) {
	mem[address & CPU_ADRSMASK] = (UINT8)value;
	mem[(address + 1) & CPU_ADRSMASK] = (UINT8)(value >> 8);
}

static void set_segment(UINT16 *segment, UINT32 *base, UINT16 value) {
	*segment = value;
	*base = (UINT32)value << 4;
	upd9002_core_context.s.ss_fix = SS_BASE;
	upd9002_core_context.s.ds_fix = DS_BASE;
}

static int expect_register_state(UINT16 expected_ip) {
	if ((CPU_AX != 0x1357) || (CPU_CX != 0x2468) || (CPU_DX != 0x369a) || (CPU_BX != 0x0200) ||
	    (CPU_SP != 0x8000) || (CPU_BP != 0x0400) || (CPU_SI != 0x0004) || (CPU_DI != 0x0010) ||
	    (CPU_CS != 0x2000) || (CPU_IP != expected_ip) || (CPU_FLAG != 0xf046)) {
		fprintf(stderr, "upd9002-m65c-f72: CPU state differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int run_register_not(const UINT8 *instruction, UINT length, UINT16 expected_ax,
                            UINT16 expected_bx, UINT16 expected_sp) {
	setup_instruction(instruction, length);
	upd9002_core_step();
	if ((CPU_AX != expected_ax) || (CPU_BX != expected_bx) || (CPU_SP != expected_sp) ||
	    (CPU_CX != 0x2468) || (CPU_DX != 0x369a) || (CPU_BP != 0x0400) || (CPU_SI != 0x0004) ||
	    (CPU_DI != 0x0010) || (CPU_IP != (UINT16)(0x0100 + length)) || (CPU_FLAG != 0xf046)) {
		fprintf(stderr, "upd9002-m65c-f72: register NOT result differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int run_direct_memory_not(const char *name, UINT32 physical, UINT16 initial, UINT16 expected,
                                 UINT16 expected_ip) {
	static const UINT8 instruction[] = {0xf7, 0x16, 0x00, 0x02};

	setup_instruction(instruction, NELEMENTS(instruction));
	write_word_physical(physical - 1, (UINT16)(initial << 8));
	mem[(physical - 1) & CPU_ADRSMASK] = kNeighborLow;
	write_word_physical(physical, initial);
	mem[(physical + 2) & CPU_ADRSMASK] = kNeighborHigh;
	upd9002_core_step();
	if ((read_word_physical(physical) != expected) || (read_byte(physical - 1) != kNeighborLow) ||
	    (read_byte(physical + 2) != kNeighborHigh)) {
		fprintf(stderr, "upd9002-m65c-f72: %s memory result differed\n", name);
		return FAILURE;
	}
	return expect_register_state(expected_ip);
}

static int test_register_forms_are_protected(void) {
	static const UINT8 not_ax[] = {0xf7, 0xd0};
	static const UINT8 not_bx[] = {0xf7, 0xd3};
	static const UINT8 not_sp[] = {0xf7, 0xd4};

	if ((run_register_not(not_ax, NELEMENTS(not_ax), 0xeca8, 0x0200, 0x8000) != SUCCESS) ||
	    (run_register_not(not_bx, NELEMENTS(not_bx), 0x1357, 0xfdff, 0x8000) != SUCCESS) ||
	    (run_register_not(not_sp, NELEMENTS(not_sp), 0x1357, 0x0200, 0x7fff) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_direct_even_low_memory_writes_both_bytes(void) {
	const UINT32 physical = physical_address(0x3333, kDirectOffset);

	return run_direct_memory_not("even low", physical, 0x33db, 0xcc24, 0x0104);
}

static int test_direct_low_memory_value_partitions(void) {
	const UINT32 physical = physical_address(0x3333, kDirectOffset);

	if ((run_direct_memory_not("all zero", physical, 0x0000, 0xffff, 0x0104) != SUCCESS) ||
	    (run_direct_memory_not("all one", physical, 0xffff, 0x0000, 0x0104) != SUCCESS) ||
	    (run_direct_memory_not("low changes only", physical, 0xff00, 0x00ff, 0x0104) != SUCCESS) ||
	    (run_direct_memory_not("high changes only", physical, 0x00ff, 0xff00, 0x0104) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_odd_low_memory_uses_word_path(void) {
	static const UINT8 instruction[] = {0xf7, 0x16, 0x01, 0x02};
	const UINT32 physical = physical_address(0x3333, 0x0201);

	setup_instruction(instruction, NELEMENTS(instruction));
	write_word_physical(physical, 0xa55a);
	upd9002_core_step();
	if (read_word_physical(physical) != 0x5aa5) {
		fprintf(stderr, "upd9002-m65c-f72: odd low memory result differed\n");
		return FAILURE;
	}
	return expect_register_state(0x0104);
}

static int test_segment_override_selects_es(void) {
	static const UINT8 instruction[] = {0x26, 0xf7, 0x16, 0x00, 0x02};
	const UINT32 ds_physical = physical_address(0x3333, kDirectOffset);
	const UINT32 es_physical = physical_address(0x1111, kDirectOffset);

	setup_instruction(instruction, NELEMENTS(instruction));
	write_word_physical(ds_physical, 0x1234);
	write_word_physical(es_physical, 0x55aa);
	upd9002_core_step();
	if ((read_word_physical(ds_physical) != 0x1234) ||
	    (read_word_physical(es_physical) != 0xaa55)) {
		fprintf(stderr, "upd9002-m65c-f72: segment override result differed\n");
		return FAILURE;
	}
	return expect_register_state(0x0105);
}

static int test_addressing_modes_and_displacement(void) {
	static const UINT8 instruction[] = {0xf7, 0x50, 0x10};
	const UINT16 offset = 0x0214;
	const UINT32 physical = physical_address(0x3333, offset);

	setup_instruction(instruction, NELEMENTS(instruction));
	CPU_BX = 0x0200;
	CPU_SI = 0x0004;
	write_word_physical(physical, 0x5aa5);
	upd9002_core_step();
	if ((read_word_physical(physical) != 0xa55a) || (CPU_BX != 0x0200) || (CPU_SI != 0x0004) ||
	    (CPU_IP != 0x0103) || (CPU_FLAG != 0xf046)) {
		fprintf(stderr, "upd9002-m65c-f72: indexed displacement result differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int test_offset_ffff_second_byte_address(void) {
	static const UINT8 instruction[] = {0xf7, 0x16, 0xff, 0xff};
	const UINT32 physical = physical_address(0x0000, 0xffff);

	setup_instruction(instruction, NELEMENTS(instruction));
	set_segment(&CPU_DS, &DS_BASE, 0x0000);
	write_word_physical(physical, 0xc33c);
	upd9002_core_step();
	if (read_word_physical(physical) != 0x3cc3) {
		fprintf(stderr, "upd9002-m65c-f72: offset-ffff result differed\n");
		return FAILURE;
	}
	return expect_register_state(0x0104);
}

static int test_high_region_word_path_is_unchanged(void) {
	static const UINT8 even_instruction[] = {0xf7, 0x16, 0x00, 0x00};
	static const UINT8 odd_instruction[] = {0xf7, 0x16, 0x01, 0x00};
	UINT32 physical;

	setup_instruction(even_instruction, NELEMENTS(even_instruction));
	set_segment(&CPU_DS, &DS_BASE, 0xa000);
	physical = physical_address(0xa000, 0x0000);
	write_word_physical(physical, 0x55aa);
	upd9002_core_step();
	if ((read_word_physical(physical) != 0xaa55) || (expect_register_state(0x0104) != SUCCESS)) {
		fprintf(stderr, "upd9002-m65c-f72: even high-region result differed\n");
		return FAILURE;
	}

	setup_instruction(odd_instruction, NELEMENTS(odd_instruction));
	set_segment(&CPU_DS, &DS_BASE, 0xa000);
	physical = physical_address(0xa000, 0x0001);
	write_word_physical(physical, 0xaa55);
	upd9002_core_step();
	if ((read_word_physical(physical) != 0x55aa) || (expect_register_state(0x0104) != SUCCESS)) {
		fprintf(stderr, "upd9002-m65c-f72: odd high-region result differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_m65c_f72_main(void) {
	int result;

	upd9002_test_flat_memory_set(TRUE);
	upd9002_core_initialize();
	result = SUCCESS;
	if ((test_register_forms_are_protected() != SUCCESS) ||
	    (test_direct_even_low_memory_writes_both_bytes() != SUCCESS) ||
	    (test_direct_low_memory_value_partitions() != SUCCESS) ||
	    (test_odd_low_memory_uses_word_path() != SUCCESS) ||
	    (test_segment_override_selects_es() != SUCCESS) ||
	    (test_addressing_modes_and_displacement() != SUCCESS) ||
	    (test_offset_ffff_second_byte_address() != SUCCESS) ||
	    (test_high_region_word_path_is_unchanged() != SUCCESS)) {
		result = FAILURE;
	}
	upd9002_core_deinitialize();
	upd9002_test_flat_memory_set(FALSE);
	if (result != SUCCESS) {
		return result;
	}
	puts("upd9002-m65c-f72: F7 /2 focused checks passed");
	return SUCCESS;
}
