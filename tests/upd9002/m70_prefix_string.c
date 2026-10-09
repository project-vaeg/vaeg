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
#include "machine/pccore.h"
#include "memoryva.h"
#include "bmsio.h"
#include "upd9002_ops.h"
#include "tests/upd9002/direct_harness.h"
#include "tests/upd9002/m70_prefix_string.h"

#include <stdio.h>

enum {
	M70_NORMAL_BASE = 0x12000,
	M70_TVRAM_BASE = 0xa0000,
	M70_BMS_BASE = 0x80000,
	M70_CS_BASE = 0x20000,
	M70_ES_NORMAL_BASE = 0x30000,
	M70_BMS_SIZE = 0x20000
};

static BYTE m70_bmsmem[M70_BMS_SIZE];

static void configure_va_mapping(void) {
	pccore.model_va = PCMODEL_VA1;
	ZeroMemory(&memoryva, sizeof(memoryva));
	memoryva.sysm_bank = 1;
	memoryva.dma_sysm_bank = 0;
	memoryva.dma_access = 0;
	bmsio.bank = 1;
	bmsio.nomem = 0;
	bmsio.cfg.numbanks = 1;
	bmsiowork.bmsmem = m70_bmsmem;
	bmsiowork.bmsmemsize = sizeof(m70_bmsmem);
	textmem_dirty = FALSE;
}

static void reset_memory_fixture(void) {
	ZeroMemory(&CPU_STATSAVE, sizeof(CPU_STATSAVE));
	ZeroMemory(mem, 0x100000);
	ZeroMemory(textmem, 0x40000);
	ZeroMemory(m70_bmsmem, sizeof(m70_bmsmem));
	configure_va_mapping();
}

static UINT32 phys(UINT32 base, UINT16 offset) {
	return (base + offset);
}

static UINT32 bms_offset(UINT16 offset) {
	return (phys(M70_BMS_BASE, offset) - M70_BMS_BASE);
}

static void put_flat_word(UINT32 address, REG16 value) {
	mem[address & 0xfffff] = (BYTE)value;
	mem[(address + 1) & 0xfffff] = (BYTE)(value >> 8);
}

static REG16 flat_word(UINT32 address) {
	return ((REG16)(mem[address & 0xfffff] | ((REG16)mem[(address + 1) & 0xfffff] << 8)));
}

static void put_tvram_word(UINT16 offset, REG16 value) {
	textmem[offset] = (BYTE)value;
	textmem[LOW16(offset + 1)] = (BYTE)(value >> 8);
}

static REG16 bms_word(UINT16 offset) {
	UINT32 offset32;

	offset32 = bms_offset(offset);
	return ((REG16)(m70_bmsmem[offset32] | ((REG16)m70_bmsmem[offset32 + 1] << 8)));
}

static int check_word(const char *name, REG16 actual, REG16 expected) {
	if (actual != expected) {
		fprintf(stderr, "upd9002-m70-prefix-string: %s expected=%04x actual=%04x\n", name, expected,
		        actual);
		return FAILURE;
	}
	return SUCCESS;
}

static int check_byte(const char *name, REG8 actual, REG8 expected) {
	if (actual != expected) {
		fprintf(stderr, "upd9002-m70-prefix-string: %s expected=%02x actual=%02x\n", name, expected,
		        actual);
		return FAILURE;
	}
	return SUCCESS;
}

static int check_true(const char *name, int condition) {
	if (!condition) {
		fprintf(stderr, "upd9002-m70-prefix-string: %s\n", name);
		return FAILURE;
	}
	return SUCCESS;
}

static void setup_instruction(const UINT8 *instruction, UINT length, UINT16 ds, UINT16 si,
                              UINT16 es, UINT16 di, UINT16 cx, UINT16 ax, BOOL carry,
                              BOOL direction) {
	UINT index;

	reset_memory_fixture();
	upd9002_core_reset();
	configure_va_mapping();
	CPU_AX = ax;
	CPU_BX = 0x2222;
	CPU_CX = cx;
	CPU_DX = 0x3333;
	CPU_SP = 0x8000;
	CPU_BP = 0x0200;
	CPU_SI = si;
	CPU_DI = di;
	CPU_ES = es;
	CPU_CS = (UINT16)(M70_CS_BASE >> 4);
	CPU_SS = 0x4000;
	CPU_DS = ds;
	CPU_IP = 0x0100;
	CPU_FLAG = (UINT16)(0xf002 | (carry ? C_FLAG : 0) | (direction ? D_FLAG : 0));
	ES_BASE = (UINT32)CPU_ES << 4;
	CS_BASE = (UINT32)CPU_CS << 4;
	SS_BASE = (UINT32)CPU_SS << 4;
	DS_BASE = (UINT32)CPU_DS << 4;
	SS_FIX = SS_BASE;
	DS_FIX = DS_BASE;
	CPU_ADRSMASK = 0x000fffff;
	CPU_REMCLOCK = 10000;
	CPU_BASECLOCK = 10000;
	CPU_CLOCK = 0;
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
	for (index = 0; index < length; index++) {
		mem[(CS_BASE + CPU_IP + index) & CPU_ADRSMASK] = instruction[index];
	}
}

static int test_repnc_movsb_repeats_with_clear_carry(void) {
	static const UINT8 instruction[] = {0x64, 0xa4};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0100,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0200, 2, 0x1111, FALSE, FALSE);
	mem[phys(M70_NORMAL_BASE, 0x0100)] = 0x11;
	mem[phys(M70_NORMAL_BASE, 0x0101)] = 0x22;
	upd9002_core_step();
	if ((check_byte("REPNC MOVSB first byte", mem[phys(M70_ES_NORMAL_BASE, 0x0200)], 0x11) !=
	     SUCCESS) ||
	    (check_byte("REPNC MOVSB second byte", mem[phys(M70_ES_NORMAL_BASE, 0x0201)], 0x22) !=
	     SUCCESS) ||
	    (check_word("REPNC MOVSB CX", CPU_CX, 0) != SUCCESS) ||
	    (check_word("REPNC MOVSB SI", CPU_SI, 0x0102) != SUCCESS) ||
	    (check_word("REPNC MOVSB DI", CPU_DI, 0x0202) != SUCCESS) ||
	    (check_word("REPNC MOVSB IP", CPU_IP, 0x0102) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repc_movsw_repeats_with_set_carry(void) {
	static const UINT8 instruction[] = {0x65, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0110,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0210, 2, 0x1111, TRUE, FALSE);
	put_flat_word(phys(M70_NORMAL_BASE, 0x0110), 0x55aa);
	put_flat_word(phys(M70_NORMAL_BASE, 0x0112), 0x33cc);
	upd9002_core_step();
	if ((check_word("REPC MOVSW first word", flat_word(phys(M70_ES_NORMAL_BASE, 0x0210)), 0x55aa) !=
	     SUCCESS) ||
	    (check_word("REPC MOVSW second word", flat_word(phys(M70_ES_NORMAL_BASE, 0x0212)),
	                0x33cc) != SUCCESS) ||
	    (check_word("REPC MOVSW CX", CPU_CX, 0) != SUCCESS) ||
	    (check_word("REPC MOVSW SI", CPU_SI, 0x0114) != SUCCESS) ||
	    (check_word("REPC MOVSW DI", CPU_DI, 0x0214) != SUCCESS) ||
	    (check_true("REPC MOVSW preserves carry", (CPU_FLAG & C_FLAG) != 0) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repnc_movsb_repeats_with_set_carry(void) {
	static const UINT8 instruction[] = {0x64, 0xa4};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0118,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0218, 2, 0x1111, TRUE, FALSE);
	mem[phys(M70_NORMAL_BASE, 0x0118)] = 0x5a;
	mem[phys(M70_NORMAL_BASE, 0x0119)] = 0xa5;
	upd9002_core_step();
	if ((check_byte("REPNC MOVSB CF=1 first byte", mem[phys(M70_ES_NORMAL_BASE, 0x0218)], 0x5a) !=
	     SUCCESS) ||
	    (check_byte("REPNC MOVSB CF=1 second byte", mem[phys(M70_ES_NORMAL_BASE, 0x0219)], 0xa5) !=
	     SUCCESS) ||
	    (check_word("REPNC MOVSB CF=1 CX", CPU_CX, 0) != SUCCESS) ||
	    (check_true("REPNC MOVSB preserves carry", (CPU_FLAG & C_FLAG) != 0) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repc_movsw_repeats_with_clear_carry(void) {
	static const UINT8 instruction[] = {0x65, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x011c,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x021c, 2, 0x1111, FALSE, FALSE);
	put_flat_word(phys(M70_NORMAL_BASE, 0x011c), 0x1357);
	put_flat_word(phys(M70_NORMAL_BASE, 0x011e), 0x2468);
	upd9002_core_step();
	if ((check_word("REPC MOVSW CF=0 first word", flat_word(phys(M70_ES_NORMAL_BASE, 0x021c)),
	                0x1357) != SUCCESS) ||
	    (check_word("REPC MOVSW CF=0 second word", flat_word(phys(M70_ES_NORMAL_BASE, 0x021e)),
	                0x2468) != SUCCESS) ||
	    (check_word("REPC MOVSW CF=0 CX", CPU_CX, 0) != SUCCESS) ||
	    (check_true("REPC MOVSW preserves clear carry", (CPU_FLAG & C_FLAG) == 0) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repnc_cmpsb_stops_when_carry_set(void) {
	static const UINT8 instruction[] = {0x64, 0xa6};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0120,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0220, 3, 0x1111, FALSE, FALSE);
	mem[phys(M70_NORMAL_BASE, 0x0120)] = 0x10;
	mem[phys(M70_ES_NORMAL_BASE, 0x0220)] = 0x10;
	mem[phys(M70_NORMAL_BASE, 0x0121)] = 0x10;
	mem[phys(M70_ES_NORMAL_BASE, 0x0221)] = 0x20;
	mem[phys(M70_NORMAL_BASE, 0x0122)] = 0x30;
	mem[phys(M70_ES_NORMAL_BASE, 0x0222)] = 0x30;
	upd9002_core_step();
	if ((check_word("REPNC CMPSB CX", CPU_CX, 1) != SUCCESS) ||
	    (check_word("REPNC CMPSB SI", CPU_SI, 0x0122) != SUCCESS) ||
	    (check_word("REPNC CMPSB DI", CPU_DI, 0x0222) != SUCCESS) ||
	    (check_true("REPNC CMPSB final carry set", (CPU_FLAG & C_FLAG) != 0) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repc_scasw_stops_when_carry_clear(void) {
	static const UINT8 instruction[] = {0x65, 0xaf};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0000,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0230, 3, 0x1000, TRUE, FALSE);
	put_flat_word(phys(M70_ES_NORMAL_BASE, 0x0230), 0x2000);
	put_flat_word(phys(M70_ES_NORMAL_BASE, 0x0232), 0x1000);
	put_flat_word(phys(M70_ES_NORMAL_BASE, 0x0234), 0x3000);
	upd9002_core_step();
	if ((check_word("REPC SCASW CX", CPU_CX, 1) != SUCCESS) ||
	    (check_word("REPC SCASW DI", CPU_DI, 0x0234) != SUCCESS) ||
	    (check_true("REPC SCASW final carry clear", (CPU_FLAG & C_FLAG) == 0) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repc_stosw_direction_decrement(void) {
	static const UINT8 instruction[] = {0x65, 0xab};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0000,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0242, 2, 0x6d61, TRUE, TRUE);
	upd9002_core_step();
	if ((check_word("REPC STOSW DF=1 first word", flat_word(phys(M70_ES_NORMAL_BASE, 0x0242)),
	                0x6d61) != SUCCESS) ||
	    (check_word("REPC STOSW DF=1 second word", flat_word(phys(M70_ES_NORMAL_BASE, 0x0240)),
	                0x6d61) != SUCCESS) ||
	    (check_word("REPC STOSW DF=1 CX", CPU_CX, 0) != SUCCESS) ||
	    (check_word("REPC STOSW DF=1 DI", CPU_DI, 0x023e) != SUCCESS) ||
	    (check_true("REPC STOSW preserves carry", (CPU_FLAG & C_FLAG) != 0) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repnc_movsw_tvram_to_normal(void) {
	static const UINT8 instruction[] = {0x64, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_TVRAM_BASE >> 4), 0x0300,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0400, 1, 0x1111, FALSE, FALSE);
	put_tvram_word(0x0300, 0xa55a);
	put_flat_word(phys(M70_TVRAM_BASE, 0x0300), 0x2010);
	upd9002_core_step();
	if ((check_word("REPNC MOVSW TVRAM mapped source", flat_word(phys(M70_ES_NORMAL_BASE, 0x0400)),
	                0xa55a) != SUCCESS) ||
	    (check_true("REPNC MOVSW rejected flat TVRAM shadow",
	                flat_word(phys(M70_ES_NORMAL_BASE, 0x0400)) != 0x2010) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repc_movsw_normal_to_bms(void) {
	static const UINT8 instruction[] = {0x65, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0130,
	                  (UINT16)(M70_BMS_BASE >> 4), 0x0040, 1, 0x1111, TRUE, FALSE);
	put_flat_word(phys(M70_NORMAL_BASE, 0x0130), 0xc46d);
	put_flat_word(phys(M70_BMS_BASE, 0x0040), 0x3333);
	upd9002_core_step();
	if ((check_word("REPC MOVSW BMS mapped destination", bms_word(0x0040), 0xc46d) != SUCCESS) ||
	    (check_word("REPC MOVSW BMS flat shadow unchanged", flat_word(phys(M70_BMS_BASE, 0x0040)),
	                0x3333) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_repnc_movsw_segment_wrap(void) {
	static const UINT8 instruction[] = {0x64, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_TVRAM_BASE >> 4), 0xffff,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x04ff, 1, 0x1111, FALSE, FALSE);
	put_tvram_word(0xffff, 0x7e64);
	put_flat_word(phys(M70_TVRAM_BASE, 0xffff), 0x1122);
	upd9002_core_step();
	if ((check_word("REPNC MOVSW FFFFh-to-0000h source wrap",
	                flat_word(phys(M70_ES_NORMAL_BASE, 0x04ff)), 0x7e64) != SUCCESS) ||
	    (check_word("REPNC MOVSW wrapped SI", CPU_SI, 0x0001) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_mixed_repe_then_repnc_uses_last_repeat(void) {
	static const UINT8 instruction[] = {0xf3, 0x64, 0xa4};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0140,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0250, 2, 0x1111, FALSE, FALSE);
	mem[phys(M70_NORMAL_BASE, 0x0140)] = 0x64;
	mem[phys(M70_NORMAL_BASE, 0x0141)] = 0xa4;
	upd9002_core_step();
	if ((check_byte("F3 64 A4 first byte", mem[phys(M70_ES_NORMAL_BASE, 0x0250)], 0x64) !=
	     SUCCESS) ||
	    (check_byte("F3 64 A4 second byte", mem[phys(M70_ES_NORMAL_BASE, 0x0251)], 0xa4) !=
	     SUCCESS) ||
	    (check_word("F3 64 A4 CX", CPU_CX, 0) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_mixed_repnc_then_repe_uses_last_repeat(void) {
	static const UINT8 instruction[] = {0x64, 0xf3, 0xa4};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M70_NORMAL_BASE >> 4), 0x0148,
	                  (UINT16)(M70_ES_NORMAL_BASE >> 4), 0x0258, 2, 0x1111, TRUE, FALSE);
	mem[phys(M70_NORMAL_BASE, 0x0148)] = 0xf3;
	mem[phys(M70_NORMAL_BASE, 0x0149)] = 0xa4;
	upd9002_core_step();
	if ((check_byte("64 F3 A4 first byte", mem[phys(M70_ES_NORMAL_BASE, 0x0258)], 0xf3) !=
	     SUCCESS) ||
	    (check_byte("64 F3 A4 second byte", mem[phys(M70_ES_NORMAL_BASE, 0x0259)], 0xa4) !=
	     SUCCESS) ||
	    (check_word("64 F3 A4 CX", CPU_CX, 0) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_negative_protection_pair(UINT8 prefix, UINT8 opcode) {
	UPD9002_HARNESS_CPU_STATE cpu;
	UPD9002_SSTS_RAM_ENTRY ram[4];
	UINT8 watch_values[2];
	UINT32 watch[2];
	UPD9002_SSTS_INPUT input;
	UPD9002_SSTS_RESULT result;

	ZeroMemory(&cpu, sizeof(cpu));
	cpu.ax = 0x1234;
	cpu.bx = 0x5678;
	cpu.cx = 0x0003;
	cpu.dx = 0x0142;
	cpu.si = 0x0100;
	cpu.di = 0x0200;
	cpu.bp = 0x0300;
	cpu.sp = 0x8000;
	cpu.cs = (UINT16)(M70_CS_BASE >> 4);
	cpu.ds = (UINT16)(M70_NORMAL_BASE >> 4);
	cpu.es = (UINT16)(M70_ES_NORMAL_BASE >> 4);
	cpu.ss = 0x4000;
	cpu.cs_base = M70_CS_BASE;
	cpu.ds_base = M70_NORMAL_BASE;
	cpu.es_base = M70_ES_NORMAL_BASE;
	cpu.ss_base = 0x40000;
	cpu.ip = 0x0100;
	cpu.flags = 0xf002;
	cpu.remain_clock = 10000;
	cpu.base_clock = 10000;
	cpu.clock = 0;
	ram[0].address = M70_CS_BASE + 0x0100;
	ram[0].value = prefix;
	ram[1].address = M70_CS_BASE + 0x0101;
	ram[1].value = opcode;
	ram[2].address = M70_NORMAL_BASE + 0x0100;
	ram[2].value = 0x5a;
	ram[3].address = M70_ES_NORMAL_BASE + 0x0200;
	ram[3].value = 0xa5;
	watch[0] = M70_NORMAL_BASE + 0x0100;
	watch[1] = M70_ES_NORMAL_BASE + 0x0200;
	ZeroMemory(&input, sizeof(input));
	input.cpu = cpu;
	input.ram = ram;
	input.ram_count = NELEMENTS(ram);
	input.watch_addresses = watch;
	input.watch_count = NELEMENTS(watch);
	ZeroMemory(&result, sizeof(result));
	result.watch_values = watch_values;
	if (upd9002_harness_run_ssts(&input, &result) != SUCCESS) {
		fprintf(stderr, "upd9002-m70-prefix-string: negative pair %02x %02x did not run\n", prefix,
		        opcode);
		return FAILURE;
	}
	if ((check_word("negative pair CX", result.cpu.cx, cpu.cx) != SUCCESS) ||
	    (check_word("negative pair SI", result.cpu.si, cpu.si) != SUCCESS) ||
	    (check_word("negative pair DI", result.cpu.di, cpu.di) != SUCCESS) ||
	    (check_word("negative pair FLAGS", (REG16)(result.cpu.flags & 0x0fff),
	                (REG16)(cpu.flags & 0x0fff)) != SUCCESS) ||
	    (check_byte("negative pair source memory", watch_values[0], 0x5a) != SUCCESS) ||
	    (check_byte("negative pair destination memory", watch_values[1], 0xa5) != SUCCESS) ||
	    (check_word("negative pair IO count", (REG16)result.io_count, 0) != SUCCESS)) {
		fprintf(stderr, "upd9002-m70-prefix-string: negative pair failed: %02x %02x\n", prefix,
		        opcode);
		return FAILURE;
	}
	return SUCCESS;
}

static int test_negative_protection_6c_6f(void) {
	static const UINT8 prefixes[] = {0x64, 0x65};
	static const UINT8 opcodes[] = {0x6c, 0x6d, 0x6e, 0x6f};
	UINT prefix_index;
	UINT opcode_index;

	for (prefix_index = 0; prefix_index < NELEMENTS(prefixes); prefix_index++) {
		for (opcode_index = 0; opcode_index < NELEMENTS(opcodes); opcode_index++) {
			if (test_negative_protection_pair(prefixes[prefix_index], opcodes[opcode_index]) !=
			    SUCCESS) {
				return FAILURE;
			}
		}
	}
	return SUCCESS;
}

typedef int (*M70TEST)(void);

typedef struct {
	const char *name;
	M70TEST fn;
} M70CASE;

int upd9002_m70_prefix_string_main(void) {
	static const M70CASE cases[] = {
	    {"REPNC MOVSB repeats with CF=0", test_repnc_movsb_repeats_with_clear_carry},
	    {"REPC MOVSW repeats with CF=1", test_repc_movsw_repeats_with_set_carry},
	    {"REPNC MOVSB repeats with CF=1", test_repnc_movsb_repeats_with_set_carry},
	    {"REPC MOVSW repeats with CF=0", test_repc_movsw_repeats_with_clear_carry},
	    {"REPNC CMPSB stops when CF becomes 1", test_repnc_cmpsb_stops_when_carry_set},
	    {"REPC SCASW stops when CF becomes 0", test_repc_scasw_stops_when_carry_clear},
	    {"REPC STOSW DF=1", test_repc_stosw_direction_decrement},
	    {"REPNC MOVSW TVRAM->normal", test_repnc_movsw_tvram_to_normal},
	    {"REPC MOVSW normal->BMS", test_repc_movsw_normal_to_bms},
	    {"REPNC MOVSW FFFFh-to-0000h source wrap", test_repnc_movsw_segment_wrap},
	    {"F3 64 A4 last repeat is REPNC", test_mixed_repe_then_repnc_uses_last_repeat},
	    {"64 F3 A4 last repeat is REPE", test_mixed_repnc_then_repe_uses_last_repeat},
	    {"64/65 + 6C-6F negative protection", test_negative_protection_6c_6f}};
	UINT index;
	UINT failures;

	upd9002_core_initialize();
	failures = 0;
	for (index = 0; index < NELEMENTS(cases); index++) {
		if (cases[index].fn() != SUCCESS) {
			fprintf(stderr, "upd9002-m70-prefix-string: case failed: %s\n", cases[index].name);
			failures++;
		}
	}
	upd9002_core_deinitialize();
	if (failures) {
		fprintf(stderr, "upd9002-m70-prefix-string: %u / %u cases failed\n", failures,
		        (UINT)NELEMENTS(cases));
		return FAILURE;
	}
	puts("upd9002-m70-prefix-string: directed checks passed");
	return SUCCESS;
}
