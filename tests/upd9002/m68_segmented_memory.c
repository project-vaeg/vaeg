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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "memoryva.h"
#include "bmsio.h"
#include "upd9002_ops.h"
#include "tests/upd9002/m68_segmented_memory.h"

#include <stdio.h>

enum {
	M68_NORMAL_BASE = 0x12000,
	M68_TVRAM_BASE = 0xa0000,
	M68_BMS_BASE = 0x80000,
	M68_CS_BASE = 0x20000,
	M68_ES_NORMAL_BASE = 0x30000,
	M68_BMS_SIZE = 0x20000
};

static BYTE m68_bmsmem[M68_BMS_SIZE];

static void configure_va_mapping(void) {
	pccore.model_va = PCMODEL_VA1;
	ZeroMemory(&memoryva, sizeof(memoryva));
	memoryva.sysm_bank = 1;
	memoryva.dma_sysm_bank = 0;
	memoryva.dma_access = 0;
	bmsio.bank = 1;
	bmsio.nomem = 0;
	bmsio.cfg.numbanks = 1;
	bmsiowork.bmsmem = m68_bmsmem;
	bmsiowork.bmsmemsize = sizeof(m68_bmsmem);
	textmem_dirty = FALSE;
}

static void reset_memory_fixture(void) {
	ZeroMemory(mem, 0x100000);
	ZeroMemory(textmem, 0x40000);
	ZeroMemory(m68_bmsmem, sizeof(m68_bmsmem));
	configure_va_mapping();
}

static UINT32 phys(UINT32 base, UINT16 offset) {
	return (base + offset);
}

static UINT32 bms_offset(UINT16 offset) {
	return (phys(M68_BMS_BASE, offset) - M68_BMS_BASE);
}

static void put_flat_word(UINT32 address, REG16 value) {
	mem[address] = (BYTE)value;
	mem[address + 1] = (BYTE)(value >> 8);
}

static void put_tvram_word(UINT16 offset, REG16 value) {
	textmem[offset] = (BYTE)value;
	textmem[LOW16(offset + 1)] = (BYTE)(value >> 8);
}

static REG16 flat_word(UINT32 address) {
	return ((REG16)(mem[address] | ((REG16)mem[address + 1] << 8)));
}

static REG16 tvram_word(UINT16 offset) {
	return ((REG16)(textmem[offset] | ((REG16)textmem[LOW16(offset + 1)] << 8)));
}

static REG16 bms_word(UINT16 offset) {
	UINT32 offset32;

	offset32 = bms_offset(offset);
	return ((REG16)(m68_bmsmem[offset32] | ((REG16)m68_bmsmem[offset32 + 1] << 8)));
}

static int check_word(const char *name, REG16 actual, REG16 expected) {
	if (actual != expected) {
		fprintf(stderr, "upd9002-m68-segmented-memory: %s expected=%04x actual=%04x\n", name,
		        expected, actual);
		return FAILURE;
	}
	return SUCCESS;
}

static int check_byte(const char *name, REG8 actual, REG8 expected) {
	if (actual != expected) {
		fprintf(stderr, "upd9002-m68-segmented-memory: %s expected=%02x actual=%02x\n", name,
		        expected, actual);
		return FAILURE;
	}
	return SUCCESS;
}

static int check_true(const char *name, int condition) {
	if (!condition) {
		fprintf(stderr, "upd9002-m68-segmented-memory: %s\n", name);
		return FAILURE;
	}
	return SUCCESS;
}

static int test_segmented_word_read_routes_tvram(void) {
	REG16 value;

	reset_memory_fixture();
	put_tvram_word(0x0100, 0xa55a);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0100), 0x2010);
	value = upd9002_memoryread_seg_w(M68_TVRAM_BASE, 0x0100);
	if ((check_word("TVRAM read returned mapped textmem", value, 0xa55a) != SUCCESS) ||
	    (check_true("TVRAM read returned flat shadow", value != 0x2010) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_segmented_word_read_routes_bms(void) {
	REG16 value;

	reset_memory_fixture();
	m68_bmsmem[bms_offset(0x0020)] = 0x6d;
	m68_bmsmem[bms_offset(0x0021)] = 0xb5;
	put_flat_word(phys(M68_BMS_BASE, 0x0020), 0x4321);
	value = upd9002_memoryread_seg_w(M68_BMS_BASE, 0x0020);
	if ((check_word("BMS read returned mapped bank memory", value, 0xb56d) != SUCCESS) ||
	    (check_true("BMS read returned flat shadow", value != 0x4321) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_segmented_word_write_routes_tvram(void) {
	reset_memory_fixture();
	put_flat_word(phys(M68_TVRAM_BASE, 0x0101), 0x2211);
	upd9002_memorywrite_seg_w(M68_TVRAM_BASE, 0x0101, 0xc35a);
	if ((check_word("TVRAM unaligned write updated textmem", tvram_word(0x0101), 0xc35a) !=
	     SUCCESS) ||
	    (check_word("TVRAM unaligned write left flat shadow",
	                flat_word(phys(M68_TVRAM_BASE, 0x0101)), 0x2211) != SUCCESS) ||
	    (check_true("TVRAM write did not set dirty notification", textmem_dirty) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_segmented_word_write_routes_bms(void) {
	reset_memory_fixture();
	put_flat_word(phys(M68_BMS_BASE, 0x0030), 0x3412);
	upd9002_memorywrite_seg_w(M68_BMS_BASE, 0x0030, 0xd17e);
	if ((check_word("BMS write updated mapped bank memory", bms_word(0x0030), 0xd17e) != SUCCESS) ||
	    (check_word("BMS write left flat shadow", flat_word(phys(M68_BMS_BASE, 0x0030)), 0x3412) !=
	     SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_segmented_word_preserves_normal_ram(void) {
	REG16 value;

	reset_memory_fixture();
	put_flat_word(phys(M68_NORMAL_BASE, 0x0100), 0x55aa);
	value = upd9002_memoryread_seg_w(M68_NORMAL_BASE, 0x0100);
	upd9002_memorywrite_seg_w(M68_NORMAL_BASE, 0x0102, 0x33cc);
	if ((check_word("normal RAM read", value, 0x55aa) != SUCCESS) ||
	    (check_word("normal RAM write", flat_word(phys(M68_NORMAL_BASE, 0x0102)), 0x33cc) !=
	     SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_segmented_word_offsets_and_wrap(void) {
	REG16 value;

	reset_memory_fixture();
	put_tvram_word(0xfffe, 0x7e68);
	value = upd9002_memoryread_seg_w(M68_TVRAM_BASE, 0xfffe);
	if (check_word("TVRAM FFFEh read", value, 0x7e68) != SUCCESS) {
		return FAILURE;
	}
	put_tvram_word(0xffff, 0x5ab4);
	put_flat_word(phys(M68_TVRAM_BASE, 0xffff), 0x6655);
	value = upd9002_memoryread_seg_w(M68_TVRAM_BASE, 0xffff);
	if ((check_word("TVRAM FFFFh-to-0000h wrap read", value, 0x5ab4) != SUCCESS) ||
	    (check_true("TVRAM wrap read returned flat shadow", value != 0x6655) != SUCCESS)) {
		return FAILURE;
	}
	upd9002_memorywrite_seg_w(M68_TVRAM_BASE, 0xffff, 0x9c81);
	if ((check_byte("TVRAM wrap low byte", textmem[0xffff], 0x81) != SUCCESS) ||
	    (check_byte("TVRAM wrap high byte", textmem[0x0000], 0x9c) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static void setup_instruction(const UINT8 *instruction, UINT length, UINT16 ds, UINT16 si,
                              UINT16 es, UINT16 di, UINT16 cx, BOOL direction) {
	UINT index;

	reset_memory_fixture();
	upd9002_core_reset();
	configure_va_mapping();
	CPU_AX = 0x1111;
	CPU_BX = 0x2222;
	CPU_CX = cx;
	CPU_DX = 0x3333;
	CPU_SP = 0x8000;
	CPU_BP = 0x0200;
	CPU_SI = si;
	CPU_DI = di;
	CPU_ES = es;
	CPU_CS = (UINT16)(M68_CS_BASE >> 4);
	CPU_SS = 0x4000;
	CPU_DS = ds;
	CPU_IP = 0x0100;
	CPU_FLAG = (UINT16)(0xf002 | (direction ? D_FLAG : 0));
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

static void put_normal_source(UINT16 offset, REG16 value) {
	put_flat_word(phys(M68_NORMAL_BASE, offset), value);
}

static REG16 normal_word(UINT32 base, UINT16 offset) {
	return (flat_word(phys(base, offset)));
}

static int test_movsw_tvram_to_normal_nonrep(void) {
	static const UINT8 instruction[] = {0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M68_TVRAM_BASE >> 4), 0x0100,
	                  (UINT16)(M68_ES_NORMAL_BASE >> 4), 0x0200, 0x7777, FALSE);
	put_tvram_word(0x0100, 0xa55a);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0100), 0x2010);
	upd9002_core_step();
	if ((check_word("non-REP MOVSW TVRAM->normal", normal_word(M68_ES_NORMAL_BASE, 0x0200),
	                0xa55a) != SUCCESS) ||
	    (check_true("non-REP MOVSW copied flat TVRAM shadow",
	                normal_word(M68_ES_NORMAL_BASE, 0x0200) != 0x2010) != SUCCESS) ||
	    (check_word("non-REP MOVSW CX unchanged", CPU_CX, 0x7777) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_movsw_tvram_to_normal_rep(void) {
	static const UINT8 instruction[] = {0xf3, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M68_TVRAM_BASE >> 4), 0x0100,
	                  (UINT16)(M68_ES_NORMAL_BASE >> 4), 0x0200, 2, FALSE);
	put_tvram_word(0x0100, 0xa55a);
	put_tvram_word(0x0102, 0xc33c);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0100), 0x2010);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0102), 0x4030);
	upd9002_core_step();
	if ((check_word("REP MOVSW TVRAM->normal word 0", normal_word(M68_ES_NORMAL_BASE, 0x0200),
	                0xa55a) != SUCCESS) ||
	    (check_word("REP MOVSW TVRAM->normal word 1", normal_word(M68_ES_NORMAL_BASE, 0x0202),
	                0xc33c) != SUCCESS) ||
	    (check_word("REP MOVSW CX final", CPU_CX, 0) != SUCCESS) ||
	    (check_word("REP MOVSW SI final", CPU_SI, 0x0104) != SUCCESS) ||
	    (check_word("REP MOVSW DI final", CPU_DI, 0x0204) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_movsw_normal_to_tvram_rep(void) {
	static const UINT8 instruction[] = {0xf3, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M68_NORMAL_BASE >> 4), 0x0110,
	                  (UINT16)(M68_TVRAM_BASE >> 4), 0x0210, 2, FALSE);
	put_normal_source(0x0110, 0x6d61);
	put_normal_source(0x0112, 0x7032);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0210), 0x1111);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0212), 0x2222);
	upd9002_core_step();
	if ((check_word("REP MOVSW normal->TVRAM word 0", tvram_word(0x0210), 0x6d61) != SUCCESS) ||
	    (check_word("REP MOVSW normal->TVRAM word 1", tvram_word(0x0212), 0x7032) != SUCCESS) ||
	    (check_word("REP MOVSW normal->TVRAM flat shadow 0",
	                flat_word(phys(M68_TVRAM_BASE, 0x0210)), 0x1111) != SUCCESS) ||
	    (check_true("REP MOVSW normal->TVRAM dirty notification", textmem_dirty) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_movsw_tvram_to_tvram_rep(void) {
	static const UINT8 instruction[] = {0xf3, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M68_TVRAM_BASE >> 4), 0x0300,
	                  (UINT16)(M68_TVRAM_BASE >> 4), 0x0310, 2, FALSE);
	put_tvram_word(0x0300, 0x815a);
	put_tvram_word(0x0302, 0xa59c);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0300), 0x1010);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0302), 0x2020);
	upd9002_core_step();
	if ((check_word("REP MOVSW TVRAM->TVRAM word 0", tvram_word(0x0310), 0x815a) != SUCCESS) ||
	    (check_word("REP MOVSW TVRAM->TVRAM word 1", tvram_word(0x0312), 0xa59c) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_movsw_bms_to_normal_rep(void) {
	static const UINT8 instruction[] = {0xf3, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M68_BMS_BASE >> 4), 0x0040,
	                  (UINT16)(M68_ES_NORMAL_BASE >> 4), 0x0240, 2, FALSE);
	m68_bmsmem[bms_offset(0x0040)] = 0x42;
	m68_bmsmem[bms_offset(0x0041)] = 0x24;
	m68_bmsmem[bms_offset(0x0042)] = 0x68;
	m68_bmsmem[bms_offset(0x0043)] = 0x86;
	put_flat_word(phys(M68_BMS_BASE, 0x0040), 0xaaaa);
	put_flat_word(phys(M68_BMS_BASE, 0x0042), 0xbbbb);
	upd9002_core_step();
	if ((check_word("REP MOVSW BMS->normal word 0", normal_word(M68_ES_NORMAL_BASE, 0x0240),
	                0x2442) != SUCCESS) ||
	    (check_word("REP MOVSW BMS->normal word 1", normal_word(M68_ES_NORMAL_BASE, 0x0242),
	                0x8668) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_movsw_normal_to_bms_rep(void) {
	static const UINT8 instruction[] = {0xf3, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M68_NORMAL_BASE >> 4), 0x0120,
	                  (UINT16)(M68_BMS_BASE >> 4), 0x0060, 2, FALSE);
	put_normal_source(0x0120, 0x61c4);
	put_normal_source(0x0122, 0x68c8);
	put_flat_word(phys(M68_BMS_BASE, 0x0060), 0x5555);
	put_flat_word(phys(M68_BMS_BASE, 0x0062), 0x7777);
	upd9002_core_step();
	if ((check_word("REP MOVSW normal->BMS word 0", bms_word(0x0060), 0x61c4) != SUCCESS) ||
	    (check_word("REP MOVSW normal->BMS word 1", bms_word(0x0062), 0x68c8) != SUCCESS) ||
	    (check_word("REP MOVSW normal->BMS flat shadow 0", flat_word(phys(M68_BMS_BASE, 0x0060)),
	                0x5555) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

static int test_movsw_rep_direction_decrement(void) {
	static const UINT8 instruction[] = {0xf3, 0xa5};

	setup_instruction(instruction, NELEMENTS(instruction), (UINT16)(M68_TVRAM_BASE >> 4), 0x0402,
	                  (UINT16)(M68_ES_NORMAL_BASE >> 4), 0x0502, 2, TRUE);
	put_tvram_word(0x0400, 0x0400);
	put_tvram_word(0x0402, 0x0402);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0400), 0x4000);
	put_flat_word(phys(M68_TVRAM_BASE, 0x0402), 0x4002);
	upd9002_core_step();
	if ((check_word("REP MOVSW DF=1 first destination", normal_word(M68_ES_NORMAL_BASE, 0x0502),
	                0x0402) != SUCCESS) ||
	    (check_word("REP MOVSW DF=1 second destination", normal_word(M68_ES_NORMAL_BASE, 0x0500),
	                0x0400) != SUCCESS) ||
	    (check_word("REP MOVSW DF=1 SI final", CPU_SI, 0x03fe) != SUCCESS) ||
	    (check_word("REP MOVSW DF=1 DI final", CPU_DI, 0x04fe) != SUCCESS)) {
		return FAILURE;
	}
	return SUCCESS;
}

typedef int (*M68TEST)(void);

typedef struct {
	const char *name;
	M68TEST fn;
} M68CASE;

int upd9002_m68_segmented_memory_main(void) {
	static const M68CASE cases[] = {
	    {"segmented word read TVRAM", test_segmented_word_read_routes_tvram},
	    {"segmented word read BMS", test_segmented_word_read_routes_bms},
	    {"segmented word write TVRAM", test_segmented_word_write_routes_tvram},
	    {"segmented word write BMS", test_segmented_word_write_routes_bms},
	    {"segmented word normal RAM", test_segmented_word_preserves_normal_ram},
	    {"segmented word offsets/wrap", test_segmented_word_offsets_and_wrap},
	    {"MOVSW TVRAM->normal non-REP", test_movsw_tvram_to_normal_nonrep},
	    {"MOVSW TVRAM->normal REP", test_movsw_tvram_to_normal_rep},
	    {"MOVSW normal->TVRAM REP", test_movsw_normal_to_tvram_rep},
	    {"MOVSW TVRAM->TVRAM REP", test_movsw_tvram_to_tvram_rep},
	    {"MOVSW BMS->normal REP", test_movsw_bms_to_normal_rep},
	    {"MOVSW normal->BMS REP", test_movsw_normal_to_bms_rep},
	    {"MOVSW REP DF=1", test_movsw_rep_direction_decrement}};
	UINT index;
	UINT failures;

	upd9002_core_initialize();
	failures = 0;
	for (index = 0; index < NELEMENTS(cases); index++) {
		if (cases[index].fn() != SUCCESS) {
			fprintf(stderr, "upd9002-m68-segmented-memory: case failed: %s\n", cases[index].name);
			failures++;
		}
	}
	upd9002_core_deinitialize();
	if (failures) {
		fprintf(stderr, "upd9002-m68-segmented-memory: %u / %u cases failed\n", failures,
		        (UINT)NELEMENTS(cases));
		return FAILURE;
	}
	puts("upd9002-m68-segmented-memory: mapped dispatch checks passed");
	return SUCCESS;
}
