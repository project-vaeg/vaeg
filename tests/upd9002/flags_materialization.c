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
#include "tests/upd9002/direct_harness.h"
#include "tests/upd9002/flags_materialization.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static UINT32 physical_address(UINT16 segment, UINT16 offset) {
	return ((((UINT32)segment << 4) + offset) & 0x000fffff);
}

static UINT16 read_segment_word(UINT16 segment, UINT16 offset) {
	UINT32 low;
	UINT32 high;

	low = physical_address(segment, offset);
	high = physical_address(segment, (UINT16)(offset + 1));
	return (UINT16)(mem[low] | ((UINT16)mem[high] << 8));
}

static void write_physical_word(UINT32 address, UINT16 value) {
	mem[address & 0x000fffff] = (UINT8)value;
	mem[(address + 1) & 0x000fffff] = (UINT8)(value >> 8);
}

static void setup_state(const UINT8 *instruction, UINT instruction_size, UINT16 flags, UINT16 ss,
                        UINT16 sp) {
	UINT index;

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
	for (index = 0; index < instruction_size; index++) {
		mem[(CS_BASE + CPU_IP + index) & 0x000fffff] = instruction[index];
	}
}

static int fail_case(const char *name, const char *detail) {
	fprintf(stderr, "upd9002-flags-materialization: %s: %s\n", name, detail);
	return FAILURE;
}

static int run_interrupt_case(const char *name, const UINT8 *instruction, UINT instruction_size,
                              UINT8 vector, UINT16 flags, UINT16 ss, UINT16 sp) {
	const UINT16 target_ip = 0x4567;
	const UINT16 target_cs = 0x1234;
	const UINT16 initial_cs = 0x2000;
	const UINT16 initial_ip = 0x0100;
	UINT16 actual_saved_flags;
	UINT16 saved_ip;

	setup_state(instruction, instruction_size, flags, ss, sp);
	write_physical_word((UINT32)vector * 4, target_ip);
	write_physical_word((UINT32)vector * 4 + 2, target_cs);
	saved_ip = (UINT16)(initial_ip + instruction_size);
	upd9002_core_step();
	if (CPU_SP != (UINT16)(sp - 6)) {
		return fail_case(name, "final SP changed");
	}
	actual_saved_flags = read_segment_word(ss, (UINT16)(sp - 2));
	if (actual_saved_flags != flags) {
		fprintf(stderr,
		        "upd9002-flags-materialization: %s: saved FLAGS expected=%04x actual=%04x\n", name,
		        flags, actual_saved_flags);
		return fail_case(name, "saved FLAGS image differs");
	}
	if (read_segment_word(ss, (UINT16)(sp - 4)) != initial_cs) {
		return fail_case(name, "saved CS differs");
	}
	if (read_segment_word(ss, (UINT16)(sp - 6)) != saved_ip) {
		return fail_case(name, "saved IP differs");
	}
	if ((CPU_CS != target_cs) || (CPU_IP != target_ip) || (CS_BASE != ((UINT32)target_cs << 4))) {
		return fail_case(name, "interrupt vector target differs");
	}
	if (CPU_REMCLOCK < 0) {
		return fail_case(name, "termination changed");
	}
	return SUCCESS;
}

static int test_interrupt_frames(void) {
	static const UINT8 cc[] = {0xcc};
	static const UINT8 cd[] = {0xcd, 0x73};
	static const UINT8 ce[] = {0xce};
	static const UINT16 copied_flags[] = {0x0002, 0x1237, 0xa9cf, 0xffff};
	UINT index;

	for (index = 0; index < NELEMENTS(copied_flags); index++) {
		if (run_interrupt_case("CC copied FLAGS", cc, sizeof(cc), 3, copied_flags[index], 0x3100,
		                       0x8000) != SUCCESS) {
			return FAILURE;
		}
	}
	if (run_interrupt_case("CD non-boundary", cd, sizeof(cd), 0x73, 0xb6d7, 0x4100, 0x9000) !=
	    SUCCESS) {
		return FAILURE;
	}
	if (run_interrupt_case("CE interrupting", ce, sizeof(ce), 4, 0xf8d7, 0x5100, 0xa000) !=
	    SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_interrupt_physical_wrap(void) {
	static const char *const name =
	    "CC physical wrap 0199d4e94995854d9b05d3a14ea0edd88ee43a00c9a9664a82bec8855e5e978e";
	static const UPD9002_SSTS_RAM_ENTRY ram[] = {
	    {0x0000c, 0xbf}, {0x0000d, 0x5a}, {0x0000e, 0xdb}, {0x0000f, 0x73}, {0x09468, 0xcc}};
	static const uint32_t watch_addresses[] = {0x02a0f, 0x02a10, 0x02a11,
	                                           0x02a12, 0x02a13, 0x02a14};
	static const uint8_t expected_frame[] = {0x49, 0xb2, 0x22, 0xfe, 0xc3, 0xf0};
	UPD9002_SSTS_INPUT input;
	UPD9002_SSTS_RESULT result;
	uint8_t watch_values[NELEMENTS(watch_addresses)];

	ZeroMemory(&input, sizeof(input));
	ZeroMemory(&result, sizeof(result));
	input.cpu.ax = 0x30cb;
	input.cpu.bx = 0x9728;
	input.cpu.cx = 0x0b79;
	input.cpu.dx = 0x325c;
	input.cpu.si = 0x9331;
	input.cpu.di = 0x2578;
	input.cpu.bp = 0x90bf;
	input.cpu.sp = 0x76b5;
	input.cpu.es = 0x250d;
	input.cpu.cs = 0xfe22;
	input.cpu.ss = 0xfb36;
	input.cpu.ds = 0xfef7;
	input.cpu.ip = 0xb248;
	input.cpu.flags = 0xf0c3;
	input.cpu.es_base = 0x250d0;
	input.cpu.cs_base = 0xfe220;
	input.cpu.ss_base = 0xfb360;
	input.cpu.ds_base = 0xfef70;
	input.cpu.remain_clock = 1000;
	input.cpu.base_clock = 1000;
	input.ram = ram;
	input.ram_count = NELEMENTS(ram);
	input.watch_addresses = watch_addresses;
	input.watch_count = NELEMENTS(watch_addresses);
	result.watch_values = watch_values;
	if (upd9002_harness_run_ssts(&input, &result) != SUCCESS) {
		return fail_case(name, "SST replay failed");
	}
	if ((result.cpu.sp != 0x76af) || (result.cpu.cs != 0x73db) || (result.cpu.ip != 0x5abf) ||
	    (result.termination != UPD9002_SSTS_TERMINATION_NORMAL) || (result.interrupt_count != 1) ||
	    (result.last_interrupt_vector != 3) ||
	    (memcmp(watch_values, expected_frame, sizeof(expected_frame)) != 0)) {
		return fail_case(name, "approved physical-wrap frame differs");
	}
	return SUCCESS;
}

static int test_interrupt_segment_wrap(void) {
	static const char *const name =
	    "CC segment wrap 97656ccbabdcb985a47ad03aa0f348b7ba5a9228a730f3a807b2fd58fd2216c9";
	static const UPD9002_SSTS_RAM_ENTRY ram[] = {
	    {0x0000c, 0xf4}, {0x0000d, 0x66}, {0x0000e, 0x9b}, {0x0000f, 0xef}, {0x4e441, 0xcc}};
	static const uint32_t watch_addresses[] = {0xfae3e, 0xfae3f, 0xeae40,
	                                           0xeae41, 0xeae42, 0xeae43};
	static const uint8_t expected_frame[] = {0x62, 0x11, 0x2e, 0x4d, 0x16, 0xfc};
	UPD9002_SSTS_INPUT input;
	UPD9002_SSTS_RESULT result;
	uint8_t watch_values[NELEMENTS(watch_addresses)];

	ZeroMemory(&input, sizeof(input));
	ZeroMemory(&result, sizeof(result));
	input.cpu.ax = 0x990c;
	input.cpu.bx = 0x347e;
	input.cpu.cx = 0x0507;
	input.cpu.dx = 0x1dc2;
	input.cpu.si = 0x65db;
	input.cpu.di = 0xa398;
	input.cpu.bp = 0x185f;
	input.cpu.sp = 0x0004;
	input.cpu.es = 0x69b2;
	input.cpu.cs = 0x4d2e;
	input.cpu.ss = 0xeae4;
	input.cpu.ds = 0xb9a6;
	input.cpu.ip = 0x1161;
	input.cpu.flags = 0xfc16;
	input.cpu.es_base = 0x69b20;
	input.cpu.cs_base = 0x4d2e0;
	input.cpu.ss_base = 0xeae40;
	input.cpu.ds_base = 0xb9a60;
	input.cpu.remain_clock = 1000;
	input.cpu.base_clock = 1000;
	input.ram = ram;
	input.ram_count = NELEMENTS(ram);
	input.watch_addresses = watch_addresses;
	input.watch_count = NELEMENTS(watch_addresses);
	result.watch_values = watch_values;
	if (upd9002_harness_run_ssts(&input, &result) != SUCCESS) {
		return fail_case(name, "SST replay failed");
	}
	if ((result.cpu.sp != 0xfffe) || (result.cpu.cs != 0xef9b) || (result.cpu.ip != 0x66f4) ||
	    (result.termination != UPD9002_SSTS_TERMINATION_NORMAL) || (result.interrupt_count != 1) ||
	    (result.last_interrupt_vector != 3) ||
	    (memcmp(watch_values, expected_frame, sizeof(expected_frame)) != 0)) {
		return fail_case(name, "approved segment-wrap frame differs");
	}
	return SUCCESS;
}

static int run_pushf_case(const char *name, UINT16 flags, UINT16 ss, UINT16 sp) {
	static const UINT8 instruction[] = {0x9c};
	UINT16 initial_ip;

	setup_state(instruction, sizeof(instruction), flags, ss, sp);
	initial_ip = CPU_IP;
	upd9002_core_step();
	if ((CPU_SP != (UINT16)(sp - 2)) || (CPU_IP != (UINT16)(initial_ip + 1)) ||
	    (read_segment_word(ss, (UINT16)(sp - 2)) != flags) || (CPU_AX != 0x1357) ||
	    (CPU_BX != 0x2468) || (CPU_REMCLOCK < 0)) {
		return fail_case(name, "ordinary PUSHF state differs");
	}
	return SUCCESS;
}

static int test_pushf_physical_wrap(void) {
	static const char *const name =
	    "PUSHF physical wrap 007e62c57c1a718b7fb18abf4309759fdd45984163c2d22893ce9bffac585421";
	static const UPD9002_SSTS_RAM_ENTRY ram[] = {{0xddb0c, 0x9c}};
	static const uint32_t watch_addresses[] = {0x04f84, 0x04f85};
	static const uint8_t expected_stack[] = {0x87, 0xfc};
	UPD9002_SSTS_INPUT input;
	UPD9002_SSTS_RESULT result;
	uint8_t watch_values[NELEMENTS(watch_addresses)];

	ZeroMemory(&input, sizeof(input));
	ZeroMemory(&result, sizeof(result));
	input.cpu.ax = 0xea6f;
	input.cpu.bx = 0xd926;
	input.cpu.cx = 0x896f;
	input.cpu.dx = 0xbf14;
	input.cpu.si = 0x1965;
	input.cpu.di = 0xbd97;
	input.cpu.bp = 0xf752;
	input.cpu.sp = 0xcf06;
	input.cpu.es = 0xab18;
	input.cpu.cs = 0xd692;
	input.cpu.ss = 0xf808;
	input.cpu.ds = 0xe1e6;
	input.cpu.ip = 0x71ec;
	input.cpu.flags = 0xfc87;
	input.cpu.es_base = 0xab180;
	input.cpu.cs_base = 0xd6920;
	input.cpu.ss_base = 0xf8080;
	input.cpu.ds_base = 0xe1e60;
	input.cpu.remain_clock = 1000;
	input.cpu.base_clock = 1000;
	input.ram = ram;
	input.ram_count = NELEMENTS(ram);
	input.watch_addresses = watch_addresses;
	input.watch_count = NELEMENTS(watch_addresses);
	result.watch_values = watch_values;
	if (upd9002_harness_run_ssts(&input, &result) != SUCCESS) {
		return fail_case(name, "SST replay failed");
	}
	if ((result.cpu.sp != 0xcf04) || (result.cpu.cs != 0xd692) || (result.cpu.ip != 0x71ed) ||
	    (result.cpu.flags != 0xfc87) || (result.termination != UPD9002_SSTS_TERMINATION_NORMAL) ||
	    (result.interrupt_count != 0) ||
	    (memcmp(watch_values, expected_stack, sizeof(expected_stack)) != 0)) {
		fprintf(
		    stderr,
		    "upd9002-flags-materialization: %s: sp=%04x cs=%04x ip=%04x flags=%04x termination=%u interrupts=%u stack=%02x%02x\n",
		    name, result.cpu.sp, result.cpu.cs, result.cpu.ip, result.cpu.flags, result.termination,
		    result.interrupt_count, watch_values[1], watch_values[0]);
		return fail_case(name, "approved physical-wrap stack image differs");
	}
	return SUCCESS;
}

static int test_pushf(void) {
	static const UINT8 instruction[] = {0x9c};
	UINT32 linear_high;
	UINT32 wrapped_high;

	if (run_pushf_case("PUSHF ordinary", 0x35ad, 0x3100, 0x8000) != SUCCESS) {
		return FAILURE;
	}
	if (test_pushf_physical_wrap() != SUCCESS) {
		return FAILURE;
	}
	setup_state(instruction, sizeof(instruction), 0xf0d6, 0x3da8, 0x0001);
	upd9002_core_step();
	linear_high = (((UINT32)0x3da8 << 4) + 0x10000) & 0x000fffff;
	wrapped_high = physical_address(0x3da8, 0x0000);
	if ((CPU_SP != 0xffff) || (mem[physical_address(0x3da8, 0xffff)] != 0xd6) ||
	    (mem[linear_high] != 0x00) || (mem[wrapped_high] != 0xf0)) {
		return fail_case("PUSHF segment wrap", "the M65e segment-wrap stack image differs");
	}
	return SUCCESS;
}

static int run_popf_case(const char *name, UINT16 stack_flags, UINT16 expected_flags) {
	static const UINT8 instruction[] = {0x9d};
	const UINT16 ss = 0x3000;
	const UINT16 sp = 0x8000;
	UINT16 initial_ip;

	setup_state(instruction, sizeof(instruction), 0xf246, ss, sp);
	write_physical_word(physical_address(ss, sp), stack_flags);
	initial_ip = CPU_IP;
	upd9002_core_step();
	if ((CPU_FLAG != expected_flags) || (CPU_SP != (UINT16)(sp + 2)) ||
	    (CPU_IP != (UINT16)(initial_ip + 1)) || (CPU_AX != 0x1357) || (CPU_BX != 0x2468) ||
	    (read_segment_word(ss, sp) != stack_flags) || (CPU_REMCLOCK < 0)) {
		return fail_case(name, "POPF load rule differs");
	}
	return SUCCESS;
}

static int test_popf(void) {
	if (run_popf_case("POPF all bits", 0xffff, 0xfed7) != SUCCESS ||
	    run_popf_case("POPF zero", 0x0000, 0xf002) != SUCCESS ||
	    run_popf_case("POPF mixed", 0x0a6d, 0xfa47) != SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

static int run_sahf_case(const char *name, UINT16 initial_flags, UINT8 ah, UINT16 expected_flags) {
	static const UINT8 instruction[] = {0x9e};
	UINT16 initial_ip;

	setup_state(instruction, sizeof(instruction), initial_flags, 0x3000, 0x8000);
	CPU_AH = ah;
	initial_ip = CPU_IP;
	upd9002_core_step();
	if ((CPU_FLAG != expected_flags) || (CPU_AH != ah) || (CPU_AL != 0x57) ||
	    (CPU_IP != (UINT16)(initial_ip + 1)) || (CPU_SP != 0x8000) || (CPU_REMCLOCK < 0)) {
		return fail_case(name, "SAHF load rule differs");
	}
	return SUCCESS;
}

static int test_sahf(void) {
	if (run_sahf_case("SAHF all bits", 0xffff, 0xff, 0xffd7) != SUCCESS ||
	    run_sahf_case("SAHF reserved bits", 0xf402, 0x28, 0xf402) != SUCCESS ||
	    run_sahf_case("SAHF copied bits", 0xfa02, 0xd5, 0xfad7) != SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

static int run_lahf_case(const char *name, UINT16 flags, UINT8 expected_ah) {
	static const UINT8 instruction[] = {0x9f};
	UINT16 initial_ip;

	setup_state(instruction, sizeof(instruction), flags, 0x3000, 0x8000);
	initial_ip = CPU_IP;
	upd9002_core_step();
	if ((CPU_AH != expected_ah) || (CPU_AL != 0x57) || (CPU_FLAG != flags) ||
	    (CPU_IP != (UINT16)(initial_ip + 1)) || (CPU_SP != 0x8000) || (CPU_REMCLOCK < 0)) {
		return fail_case(name, "LAHF image differs");
	}
	return SUCCESS;
}

static int test_lahf(void) {
	if (run_lahf_case("LAHF fixed bits", 0xf002, 0x02) != SUCCESS ||
	    run_lahf_case("LAHF copied bits", 0xf0d7, 0xd7) != SUCCESS) {
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_flags_materialization_main(void) {
	UINT groups;

	upd9002_core_initialize();
	groups = 0;
	if (test_interrupt_frames() != SUCCESS) {
		goto failed;
	}
	groups++;
	if (test_interrupt_physical_wrap() != SUCCESS) {
		goto failed;
	}
	groups++;
	if (test_interrupt_segment_wrap() != SUCCESS) {
		goto failed;
	}
	groups++;
	if (test_pushf() != SUCCESS) {
		goto failed;
	}
	groups++;
	if (test_popf() != SUCCESS) {
		goto failed;
	}
	groups++;
	if (test_sahf() != SUCCESS) {
		goto failed;
	}
	groups++;
	if (test_lahf() != SUCCESS) {
		goto failed;
	}
	groups++;
	upd9002_core_deinitialize();
	fprintf(stderr, "upd9002-flags-materialization: groups=%u deterministic checks passed\n",
	        groups);
	return EXIT_SUCCESS;

failed:
	upd9002_core_deinitialize();
	return EXIT_FAILURE;
}
