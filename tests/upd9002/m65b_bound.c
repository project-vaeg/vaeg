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
#include "tests/upd9002/m65b_bound.h"

#include <stdio.h>

enum {
	kBoundOffset = 0x0200,
	kVector5Offset = 0x1234,
	kVector5Segment = 0xabcd
};

static void setup_instruction(const UINT8 *instruction, UINT length) {
	UINT index;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0x0000;
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

static void write_bounds(UINT32 address, UINT16 lower, UINT16 upper) {
	write_word(address, lower);
	write_word(address + 2, upper);
}

static void write_vector5(void) {
	write_word(5 * 4, kVector5Offset);
	write_word(5 * 4 + 2, kVector5Segment);
}

static int expect_normal(UINT16 expected_ip) {
	if ((CPU_CS != 0x2000) || (CPU_IP != expected_ip) || (CPU_SP != 0x8000) ||
	    (CPU_FLAG != 0xf046)) {
		fprintf(stderr, "upd9002-m65b-bound: normal state differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int expect_type5(UINT16 saved_ip) {
	if ((CPU_CS != kVector5Segment) || (CPU_IP != kVector5Offset) || (CPU_SP != 0x7ffa) ||
	    (read_word(SS_BASE + 0x7ffa) != saved_ip) || (read_word(SS_BASE + 0x7ffc) != 0x2000) ||
	    (read_word(SS_BASE + 0x7ffe) != 0xf046)) {
		fprintf(stderr, "upd9002-m65b-bound: type-5 frame differed\n");
		return FAILURE;
	}
	return SUCCESS;
}

static int run_direct_bound(UINT16 value, UINT16 lower, UINT16 upper, BOOL expect_event) {
	static const UINT8 instruction[] = {0x62, 0x06, 0x00, 0x02};

	setup_instruction(instruction, NELEMENTS(instruction));
	write_vector5();
	CPU_AX = value;
	write_bounds(DS_BASE + kBoundOffset, lower, upper);
	upd9002_core_step();
	if (read_word(DS_BASE + kBoundOffset) != lower ||
	    read_word(DS_BASE + kBoundOffset + 2) != upper) {
		fprintf(stderr, "upd9002-m65b-bound: bounds memory changed\n");
		return FAILURE;
	}
	if (expect_event) {
		return expect_type5(0x0104);
	}
	return expect_normal(0x0104);
}

static int test_bound_signed_range_partitions(void) {
	if ((run_direct_bound(0xfff6, 0xfff6, 0x000a, FALSE) != SUCCESS) ||
	    (run_direct_bound(0x0000, 0xfff6, 0x000a, FALSE) != SUCCESS) ||
	    (run_direct_bound(0x000a, 0xfff6, 0x000a, FALSE) != SUCCESS) ||
	    (run_direct_bound(0xfff5, 0xfff6, 0x000a, TRUE) != SUCCESS) ||
	    (run_direct_bound(0x000b, 0xfff6, 0x000a, TRUE) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_bound_signed_extremes_and_unsigned_rejections(void) {
	if ((run_direct_bound(0x8000, 0x8000, 0x7fff, FALSE) != SUCCESS) ||
	    (run_direct_bound(0x7fff, 0x8000, 0x7fff, FALSE) != SUCCESS) ||
	    (run_direct_bound(0xfffb, 0xfff6, 0xffff, FALSE) != SUCCESS) ||
	    (run_direct_bound(0x0005, 0x0001, 0x000a, FALSE) != SUCCESS) ||
	    (run_direct_bound(0xffff, 0xfffb, 0x0005, FALSE) != SUCCESS) ||
	    (run_direct_bound(0x8000, 0x0000, 0xffff, TRUE) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_bound_segment_override(void) {
	static const UINT8 instruction[] = {0x26, 0x62, 0x06, 0x00, 0x02};

	setup_instruction(instruction, NELEMENTS(instruction));
	write_vector5();
	CPU_AX = 0x0000;
	write_bounds(DS_BASE + kBoundOffset, 0x000a, 0x0014);
	write_bounds(ES_BASE + kBoundOffset, 0xffff, 0x0001);
	upd9002_core_step();
	if ((read_word(DS_BASE + kBoundOffset) != 0x000a) ||
	    (read_word(ES_BASE + kBoundOffset) != 0xffff)) {
		fprintf(stderr, "upd9002-m65b-bound: segment memory changed\n");
		return FAILURE;
	}
	return expect_normal(0x0105);
}

static int test_bound_offset_wrap(void) {
	static const UINT8 instruction[] = {0x62, 0x00};

	setup_instruction(instruction, NELEMENTS(instruction));
	write_vector5();
	CPU_AX = 0x0000;
	CPU_BX = 0xffff;
	CPU_SI = 0x0001;
	write_bounds(DS_BASE, 0xffff, 0x0001);
	upd9002_core_step();
	return expect_normal(0x0102);
}

static int test_bound_physical_wrap(void) {
	static const UINT8 instruction[] = {0x62, 0x06, 0x10, 0x00};

	setup_instruction(instruction, NELEMENTS(instruction));
	write_vector5();
	CPU_AX = 0x0000;
	CPU_DS = 0xffff;
	DS_BASE = (UINT32)CPU_DS << 4;
	upd9002_core_context.s.ds_fix = DS_BASE;
	write_bounds(0x00000, 0xffff, 0x0001);
	upd9002_core_step();
	return expect_normal(0x0104);
}

int upd9002_m65b_bound_main(void) {
	upd9002_core_initialize();
	if ((test_bound_signed_range_partitions() != SUCCESS) ||
	    (test_bound_signed_extremes_and_unsigned_rejections() != SUCCESS) ||
	    (test_bound_segment_override() != SUCCESS) || (test_bound_offset_wrap() != SUCCESS) ||
	    (test_bound_physical_wrap() != SUCCESS)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_deinitialize();
	puts("upd9002-m65b-bound: BOUND focused checks passed");
	return SUCCESS;
}
