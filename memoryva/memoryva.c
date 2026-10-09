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
 *
 * Ported from cpuxva/memoryva.x86 for the portable i286c core.
 */

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "memoryva.h"
#include "gvramva.h"
#include "va91.h"
#include "bmsio.h"
#include "emsio.h"
#include "io/sysportva.h"

void MEMCALL gvram_wt(UINT32 address, REG8 value);
void MEMCALL gvramw_wt(UINT32 address, REG16 value);
REG8 MEMCALL gvram_rd(UINT32 address);
REG16 MEMCALL gvramw_rd(UINT32 address);

enum {
	CPUADDR_SYSM = 0x0a0000,
	CPUADDR_ROM0 = 0x0e0000,
	CPUADDR_ROM1 = 0x0f0000,
	CPUADDR_BMS = 0x080000,
	CPUADDR_BACKUP = 0x0b0000,
	/* VA1 bank 1 exposes TVRAM only at A0000-AFFFF. */
	TVRAM_VA1_SIZE = 0x10000,
	BACKUPMEMORY_SIZE = 0x04000
};

BYTE textmem[0x40000];
BYTE fontmem[0x50000];
BYTE backupmem[0x04000];
BYTE dicmem[0x80000];
BYTE rom0mem[0xa0000];
BYTE rom1mem[0x20000];

BYTE va91rom0mem[0xa0000];
BYTE va91rom1mem[0x20000];
BYTE va91dicmem[0x80000];

_MEMORYVA memoryva;
UINT8 memoryva_88_mode;
UINT8 memoryva_88_port31;
UINT8 memoryva_88_xerom = 1;
UINT8 memoryva_88_window = 0x80;
UINT8 memoryva_88_plane = 3;
_MEMORYVA88ALU memoryva_88_alu;
_MEMORYVA88ERAM memoryva_88_eram;
BYTE memoryva_n80[0x8000];
BOOL memoryva_n80_exist;
char memoryva_n80_file[32];
_MEMORYVA88DIC memoryva_88_dic = {0, 1};
BOOL textmem_dirty;

_VA91 va91;
_VA91CFG va91cfg;

typedef void(MEMCALL *MEM8WRITE)(UINT32 address, REG8 value);
typedef REG8(MEMCALL *MEM8READ)(UINT32 address);
typedef void(MEMCALL *MEM16WRITE)(UINT32 address, REG16 value);
typedef REG16(MEMCALL *MEM16READ)(UINT32 address);

static void MEMCALL mainram_wt(UINT32 address, REG8 value);
static void MEMCALL unmapped_wt(UINT32 address, REG8 value);
static void MEMCALL bms_wt_va(UINT32 address, REG8 value);
static void MEMCALL sysm_wt(UINT32 address, REG8 value);
static void MEMCALL tvram_wt(UINT32 address, REG8 value);
static void MEMCALL gvram_wt_va(UINT32 address, REG8 value);
static void MEMCALL knj2_wt(UINT32 address, REG8 value);
static void MEMCALL va91sysm_wt(UINT32 address, REG8 value);
static void MEMCALL va91knj2_wt(UINT32 address, REG8 value);

static void MEMCALL mainramw_wt(UINT32 address, REG16 value);
static void MEMCALL unmappedw_wt(UINT32 address, REG16 value);
static void MEMCALL bmsw_wt_va(UINT32 address, REG16 value);
static void MEMCALL sysmw_wt(UINT32 address, REG16 value);
static void MEMCALL tvramw_wt(UINT32 address, REG16 value);
static void MEMCALL gvramw_wt_va(UINT32 address, REG16 value);
static void MEMCALL knj2w_wt(UINT32 address, REG16 value);
static void MEMCALL va91sysmw_wt(UINT32 address, REG16 value);
static void MEMCALL va91knj2w_wt(UINT32 address, REG16 value);

static REG8 MEMCALL mainram_rd(UINT32 address);
static REG8 MEMCALL unmapped_rd(UINT32 address);
static REG8 MEMCALL bms_rd_va(UINT32 address);
static REG8 MEMCALL sysm_rd(UINT32 address);
static REG8 MEMCALL tvram_rd(UINT32 address);
static REG8 MEMCALL gvram_rd_va(UINT32 address);
static REG8 MEMCALL knj1_rd(UINT32 address);
static REG8 MEMCALL knj2_rd(UINT32 address);
static REG8 MEMCALL dic1_rd(UINT32 address);
static REG8 MEMCALL dic2_rd(UINT32 address);
static REG8 MEMCALL rom0_rd(UINT32 address);
static REG8 MEMCALL stdrom0_rd(UINT32 address);
static REG8 MEMCALL rom1_rd(UINT32 address);
static REG8 MEMCALL stdrom1_rd(UINT32 address);
static REG8 MEMCALL va91sysm_rd(UINT32 address);
static REG8 MEMCALL va91knj2_rd(UINT32 address);
static REG8 MEMCALL va91dic1_rd(UINT32 address);
static REG8 MEMCALL va91dic2_rd(UINT32 address);
static REG8 MEMCALL va91rom0_rd(UINT32 address);
static REG8 MEMCALL va91rom1_rd(UINT32 address);

static REG16 MEMCALL mainramw_rd(UINT32 address);
static REG16 MEMCALL unmappedw_rd(UINT32 address);
static REG16 MEMCALL bmsw_rd_va(UINT32 address);
static REG16 MEMCALL sysmw_rd(UINT32 address);
static REG16 MEMCALL tvramw_rd(UINT32 address);
static REG16 MEMCALL gvramw_rd_va(UINT32 address);
static REG16 MEMCALL knj1w_rd(UINT32 address);
static REG16 MEMCALL knj2w_rd(UINT32 address);
static REG16 MEMCALL dic1w_rd(UINT32 address);
static REG16 MEMCALL dic2w_rd(UINT32 address);
static REG16 MEMCALL rom0w_rd(UINT32 address);
static REG16 MEMCALL stdrom0w_rd(UINT32 address);
static REG16 MEMCALL rom1w_rd(UINT32 address);
static REG16 MEMCALL stdrom1w_rd(UINT32 address);
static REG16 MEMCALL va91sysmw_rd(UINT32 address);
static REG16 MEMCALL va91knj2w_rd(UINT32 address);
static REG16 MEMCALL va91dic1w_rd(UINT32 address);
static REG16 MEMCALL va91dic2w_rd(UINT32 address);
static REG16 MEMCALL va91rom0w_rd(UINT32 address);
static REG16 MEMCALL va91rom1w_rd(UINT32 address);

/*
 * The top-level VA decoder selects one handler per 64 KiB region. Addresses
 * A0000H-DFFFFH enter the selected system-memory bank, while E0000H and
 * F0000H enter the independently banked ROM windows. The byte and word tables
 * remain parallel because VA devices define distinct word-access behavior.
 */

static MEM8WRITE membyte_write[16] = {mainram_wt, mainram_wt, mainram_wt,  mainram_wt,
                                      mainram_wt, mainram_wt, mainram_wt,  mainram_wt,
                                      bms_wt_va,  bms_wt_va,  sysm_wt,     sysm_wt,
                                      sysm_wt,    sysm_wt,    unmapped_wt, unmapped_wt};

static MEM8WRITE sysmbyte_write[16] = {unmapped_wt, tvram_wt,    unmapped_wt, unmapped_wt,
                                       gvram_wt_va, unmapped_wt, unmapped_wt, unmapped_wt,
                                       unmapped_wt, knj2_wt,     unmapped_wt, unmapped_wt,
                                       unmapped_wt, unmapped_wt, unmapped_wt, unmapped_wt};

static MEM8WRITE va91sysmbyte_write[16] = {unmapped_wt, unmapped_wt, unmapped_wt, unmapped_wt,
                                           unmapped_wt, unmapped_wt, unmapped_wt, unmapped_wt,
                                           unmapped_wt, va91knj2_wt, unmapped_wt, unmapped_wt,
                                           unmapped_wt, unmapped_wt, unmapped_wt, unmapped_wt};

static MEM16WRITE memword_write[16] = {mainramw_wt, mainramw_wt, mainramw_wt,  mainramw_wt,
                                       mainramw_wt, mainramw_wt, mainramw_wt,  mainramw_wt,
                                       bmsw_wt_va,  bmsw_wt_va,  sysmw_wt,     sysmw_wt,
                                       sysmw_wt,    sysmw_wt,    unmappedw_wt, unmappedw_wt};

static MEM16WRITE sysmword_write[16] = {unmappedw_wt, tvramw_wt,    unmappedw_wt, unmappedw_wt,
                                        gvramw_wt_va, unmappedw_wt, unmappedw_wt, unmappedw_wt,
                                        unmappedw_wt, knj2w_wt,     unmappedw_wt, unmappedw_wt,
                                        unmappedw_wt, unmappedw_wt, unmappedw_wt, unmappedw_wt};

static MEM16WRITE va91sysmword_write[16] = {unmappedw_wt, unmappedw_wt, unmappedw_wt, unmappedw_wt,
                                            unmappedw_wt, unmappedw_wt, unmappedw_wt, unmappedw_wt,
                                            unmappedw_wt, va91knj2w_wt, unmappedw_wt, unmappedw_wt,
                                            unmappedw_wt, unmappedw_wt, unmappedw_wt, unmappedw_wt};

static MEM8READ membyte_read[16] = {
    mainram_rd, mainram_rd, mainram_rd, mainram_rd, mainram_rd, mainram_rd, mainram_rd, mainram_rd,
    bms_rd_va,  bms_rd_va,  sysm_rd,    sysm_rd,    sysm_rd,    sysm_rd,    rom0_rd,    rom1_rd};

static MEM8READ sysmbyte_read[16] = {unmapped_rd, tvram_rd,    unmapped_rd, unmapped_rd,
                                     gvram_rd_va, unmapped_rd, unmapped_rd, unmapped_rd,
                                     knj1_rd,     knj2_rd,     unmapped_rd, unmapped_rd,
                                     dic1_rd,     dic2_rd,     unmapped_rd, unmapped_rd};

static MEM8READ rom0byte_read[32] = {
    stdrom0_rd,  stdrom0_rd,  stdrom0_rd,  stdrom0_rd,  stdrom0_rd,  stdrom0_rd,  stdrom0_rd,
    stdrom0_rd,  stdrom0_rd,  stdrom0_rd,  stdrom0_rd,  stdrom0_rd,  stdrom0_rd,  stdrom0_rd,
    stdrom0_rd,  stdrom0_rd,  unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd,
    unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd,
    unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd};

static MEM8READ rom1byte_read[16] = {stdrom1_rd,  stdrom1_rd,  stdrom1_rd,  stdrom1_rd,
                                     stdrom1_rd,  stdrom1_rd,  stdrom1_rd,  stdrom1_rd,
                                     unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd,
                                     unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd};

static MEM8READ va91sysmbyte_read[16] = {unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd,
                                         unmapped_rd, unmapped_rd, unmapped_rd, unmapped_rd,
                                         unmapped_rd, va91knj2_rd, unmapped_rd, unmapped_rd,
                                         va91dic1_rd, va91dic2_rd, unmapped_rd, unmapped_rd};

static MEM16READ memword_read[16] = {mainramw_rd, mainramw_rd, mainramw_rd, mainramw_rd,
                                     mainramw_rd, mainramw_rd, mainramw_rd, mainramw_rd,
                                     bmsw_rd_va,  bmsw_rd_va,  sysmw_rd,    sysmw_rd,
                                     sysmw_rd,    sysmw_rd,    rom0w_rd,    rom1w_rd};

static MEM16READ sysmword_read[16] = {unmappedw_rd, tvramw_rd,    unmappedw_rd, unmappedw_rd,
                                      gvramw_rd_va, unmappedw_rd, unmappedw_rd, unmappedw_rd,
                                      knj1w_rd,     knj2w_rd,     unmappedw_rd, unmappedw_rd,
                                      dic1w_rd,     dic2w_rd,     unmappedw_rd, unmappedw_rd};

static MEM16READ rom0word_read[32] = {
    stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,
    stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,
    stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,  stdrom0w_rd,  unmappedw_rd, unmappedw_rd,
    unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd,
    unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd,
    unmappedw_rd, unmappedw_rd};

static MEM16READ rom1word_read[16] = {stdrom1w_rd,  stdrom1w_rd,  stdrom1w_rd,  stdrom1w_rd,
                                      stdrom1w_rd,  stdrom1w_rd,  stdrom1w_rd,  stdrom1w_rd,
                                      unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd,
                                      unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd};

static MEM16READ va91sysmword_read[16] = {unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd,
                                          unmappedw_rd, unmappedw_rd, unmappedw_rd, unmappedw_rd,
                                          unmappedw_rd, va91knj2w_rd, unmappedw_rd, unmappedw_rd,
                                          va91dic1w_rd, va91dic2w_rd, unmappedw_rd, unmappedw_rd};

static UINT32 top_index(UINT32 address) {
	return ((address >> 16) & 0x0f);
}

static UINT32 inc_cx(UINT32 address) {
	return ((address & 0xffff0000) | LOW16(address + 1));
}

static UINT sysm_bank(void) {
	UINT bank;

	bank = memoryva.dma_sysm_bank;
	if (!(bank & memoryva.dma_access)) {
		bank = memoryva.sysm_bank;
	}
	return (bank & 0x0f);
}

static BOOL ems_page_frame_active(UINT32 address) {
	return ((emsio.target != 0) && (emsio.target <= emsio.maxmem) && (address >= 0x0c0000) &&
	        (address < 0x0d0000));
}

static void ems_page_frame_write(UINT32 address, REG8 value) {
	CPU_EMSPTR[(address >> 14) & 3][LOW14(address)] = (BYTE)value;
}

static REG8 ems_page_frame_read(UINT32 address) {
	return (CPU_EMSPTR[(address >> 14) & 3][LOW14(address)]);
}

static UINT32 tvram_size(void) {
	if (pccore.model_va == PCMODEL_VA1) {
		return (TVRAM_VA1_SIZE);
	}
	return (sizeof(textmem));
}

static UINT va91sysm_bank(void) {
	return (va91.sysm_bank & 0x0f);
}

static REG16 duplicate_word(REG8 value) {
	return ((REG16)value | ((REG16)value << 8));
}

static REG16 read_pair16(MEM8READ rd, UINT32 address) {
	REG8 lo;
	REG8 hi;

	if (!(address & 1)) {
		return (duplicate_word(rd(address)));
	}
	hi = rd(inc_cx(address));
	lo = rd(address);
	return ((REG16)lo | ((REG16)hi << 8));
}

static void write_pair16(MEM8WRITE wt, UINT32 address, REG16 value) {
	if (!(address & 1)) {
		wt(address, (REG8)value);
	} else {
		wt(inc_cx(address), (REG8)(value >> 8));
		wt(address, (REG8)value);
	}
}

static REG8 rom1invalid_rd(UINT32 address) {
	REG16 ret;

	ret = (REG16)address & 0xfffe;
	if (!(address & 1)) {
		return ((REG8)(ret >> 8));
	}
	return ((REG8)ret);
}

static REG16 rom1invalidw_rd(UINT32 address) {
	REG16 ret;

	ret = (REG16)address & 0xfffe;
	if (!(address & 1)) {
		ret = (REG16)(((ret & 0x00ff) << 8) | (((ret >> 8) + 2) & 0x00ff));
	}
	return (ret);
}

static void MEMCALL mainram_wt(UINT32 address, REG8 value) {
	upd9002_mainram_write(address, value);
}

static void MEMCALL unmapped_wt(UINT32 address, REG8 value) {
	(void)address;
	(void)value;
}

static void MEMCALL bms_wt_va(UINT32 address, REG8 value) {
	UINT32 offset;

	if (bmsio.bank == 0) {
		mainram_wt(address, value);
		return;
	}
	if (bmsio.nomem || (bmsiowork.bmsmem == NULL)) {
		return;
	}
	offset = ((UINT32)(bmsio.bank - 1) << 17) + address - CPUADDR_BMS;
	if (offset >= bmsiowork.bmsmemsize) {
		return;
	}
	bmsiowork.bmsmem[offset] = (BYTE)value;
}

static void MEMCALL sysm_wt(UINT32 address, REG8 value) {
	if (ems_page_frame_active(address)) {
		ems_page_frame_write(address, value);
		return;
	}
	sysmbyte_write[sysm_bank()](address, value);
}

static void MEMCALL tvram_wt(UINT32 address, REG8 value) {
	if ((address - CPUADDR_SYSM) >= tvram_size()) {
		return;
	}
	textmem[address - CPUADDR_SYSM] = (BYTE)value;
	textmem_dirty = TRUE;
}

static void MEMCALL gvram_wt_va(UINT32 address, REG8 value) {
	gvram_wt(address - CPUADDR_SYSM, value);
}

static void MEMCALL knj2_wt(UINT32 address, REG8 value) {
	UINT32 offset;

	if ((address >= 0x0b1fc0) && (address < 0x0b2000) && memoryva.backupmem_wp) {
		return;
	}
	if (address < CPUADDR_BACKUP) {
		return;
	}
	offset = address - CPUADDR_BACKUP;
	if (offset >= BACKUPMEMORY_SIZE) {
		return;
	}
	backupmem[offset] = (BYTE)value;
}

static void MEMCALL va91sysm_wt(UINT32 address, REG8 value) {
	va91sysmbyte_write[va91sysm_bank()](address, value);
}

static void MEMCALL va91knj2_wt(UINT32 address, REG8 value) {
	UINT32 offset;

	if (address < (CPUADDR_BACKUP + 0x2000)) {
		return;
	}
	offset = address - (CPUADDR_BACKUP + 0x2000);
	if (offset >= (BACKUPMEMORY_SIZE - 0x2000)) {
		return;
	}
	backupmem[0x2000 + offset] = (BYTE)value;
}

static void MEMCALL mainramw_wt(UINT32 address, REG16 value) {
	upd9002_mainram_write_w(address, value);
}

static void MEMCALL unmappedw_wt(UINT32 address, REG16 value) {
	(void)address;
	(void)value;
}

static void MEMCALL bmsw_wt_va(UINT32 address, REG16 value) {
	UINT32 offset;

	if (bmsio.bank == 0) {
		mainramw_wt(address, value);
		return;
	}
	if (bmsio.nomem || (bmsiowork.bmsmem == NULL)) {
		return;
	}
	offset = ((UINT32)(bmsio.bank - 1) << 17) + address - CPUADDR_BMS;
	if ((offset >= bmsiowork.bmsmemsize) || ((bmsiowork.bmsmemsize - offset) < 2)) {
		return;
	}
	STOREINTELWORD(bmsiowork.bmsmem + offset, value);
}

static void MEMCALL sysmw_wt(UINT32 address, REG16 value) {
	if (ems_page_frame_active(address) || ems_page_frame_active(address + 1)) {
		sysm_wt(address, (REG8)value);
		sysm_wt(address + 1, (REG8)(value >> 8));
		return;
	}
	sysmword_write[sysm_bank()](address, value);
}

static void MEMCALL tvramw_wt(UINT32 address, REG16 value) {
	UINT32 offset;

	offset = address - CPUADDR_SYSM;
	if (offset >= tvram_size()) {
		return;
	}
	textmem[offset] = (BYTE)value;
	if ((offset + 1) < tvram_size()) {
		textmem[offset + 1] = (BYTE)(value >> 8);
	}
	textmem_dirty = TRUE;
}

static void MEMCALL gvramw_wt_va(UINT32 address, REG16 value) {
	gvramw_wt(address - CPUADDR_SYSM, value);
}

static void MEMCALL knj2w_wt(UINT32 address, REG16 value) {
	write_pair16(knj2_wt, address, value);
}

static void MEMCALL va91sysmw_wt(UINT32 address, REG16 value) {
	va91sysmword_write[va91sysm_bank()](address, value);
}

static void MEMCALL va91knj2w_wt(UINT32 address, REG16 value) {
	write_pair16(va91knj2_wt, address, value);
}

static REG8 MEMCALL mainram_rd(UINT32 address) {
	return (upd9002_mainram_read(address));
}

static REG8 MEMCALL unmapped_rd(UINT32 address) {
	(void)address;
	return (0xff);
}

static REG8 MEMCALL bms_rd_va(UINT32 address) {
	UINT32 offset;

	if (bmsio.bank == 0) {
		return (mainram_rd(address));
	}
	if (bmsio.nomem || (bmsiowork.bmsmem == NULL)) {
		return (0xff);
	}
	offset = ((UINT32)(bmsio.bank - 1) << 17) + address - CPUADDR_BMS;
	if (offset >= bmsiowork.bmsmemsize) {
		return (0xff);
	}
	return (bmsiowork.bmsmem[offset]);
}

static REG8 MEMCALL sysm_rd(UINT32 address) {
	if (ems_page_frame_active(address)) {
		return (ems_page_frame_read(address));
	}
	return (sysmbyte_read[sysm_bank()](address));
}

static REG8 MEMCALL tvram_rd(UINT32 address) {
	if ((address - CPUADDR_SYSM) >= tvram_size()) {
		return (0xff);
	}
	return (textmem[address - CPUADDR_SYSM]);
}

static REG8 MEMCALL gvram_rd_va(UINT32 address) {
	return (gvram_rd(address - CPUADDR_SYSM));
}

static REG8 MEMCALL knj1_rd(UINT32 address) {
	return (fontmem[address - CPUADDR_SYSM]);
}

static REG8 MEMCALL knj2_rd(UINT32 address) {
	if (address >= (CPUADDR_BACKUP + BACKUPMEMORY_SIZE)) {
		return (0xff);
	}
	if (address >= CPUADDR_BACKUP) {
		return (backupmem[address - CPUADDR_BACKUP]);
	}
	return (fontmem[address - (CPUADDR_SYSM - 0x40000)]);
}

static REG8 MEMCALL dic1_rd(UINT32 address) {
	return (dicmem[address - CPUADDR_SYSM]);
}

static REG8 MEMCALL dic2_rd(UINT32 address) {
	return (dicmem[address - (CPUADDR_SYSM - 0x40000)]);
}

static REG8 MEMCALL rom0_rd(UINT32 address) {
	return (rom0byte_read[memoryva.rom0_bank & 0x1f](address));
}

static REG8 MEMCALL stdrom0_rd(UINT32 address) {
	UINT bank;
	UINT32 offset;

	bank = memoryva.rom0_bank & 0x1f;
	if (bank >= 0x0a) {
		return (0xff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM0;
	return (rom0mem[offset]);
}

static REG8 MEMCALL rom1_rd(UINT32 address) {
	return (rom1byte_read[memoryva.rom1_bank & 0x0f](address));
}

static REG8 MEMCALL stdrom1_rd(UINT32 address) {
	UINT bank;
	UINT32 offset;

	bank = memoryva.rom1_bank & 0x03;
	if (bank & 0x02) {
		return (rom1invalid_rd(address));
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM1;
	return (rom1mem[offset]);
}

static REG8 MEMCALL va91sysm_rd(UINT32 address) {
	return (va91sysmbyte_read[va91sysm_bank()](address));
}

static REG8 MEMCALL va91knj2_rd(UINT32 address) {
	if ((address < (CPUADDR_BACKUP + 0x2000)) ||
	    (address >= (CPUADDR_BACKUP + BACKUPMEMORY_SIZE))) {
		return (0xff);
	}
	return (backupmem[address - CPUADDR_BACKUP]);
}

static REG8 MEMCALL va91dic1_rd(UINT32 address) {
	return (va91dicmem[address - CPUADDR_SYSM]);
}

static REG8 MEMCALL va91dic2_rd(UINT32 address) {
	return (va91dicmem[address - (CPUADDR_SYSM - 0x40000)]);
}

static REG8 MEMCALL va91rom0_rd(UINT32 address) {
	UINT bank;
	UINT32 offset;

	bank = va91.rom0_bank;
	if (bank >= 0x0a) {
		return (0xff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM0;
	return (va91rom0mem[offset]);
}

static REG8 MEMCALL va91rom1_rd(UINT32 address) {
	UINT bank;
	UINT32 offset;

	bank = va91.rom1_bank;
	if (bank >= 0x02) {
		return (0xff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM1;
	return (va91rom1mem[offset]);
}

static REG16 MEMCALL mainramw_rd(UINT32 address) {
	return (upd9002_mainram_read_w(address));
}

static REG16 MEMCALL unmappedw_rd(UINT32 address) {
	(void)address;
	return (0xffff);
}

static REG16 MEMCALL bmsw_rd_va(UINT32 address) {
	UINT32 offset;

	if (bmsio.bank == 0) {
		return (mainramw_rd(address));
	}
	if (bmsio.nomem || (bmsiowork.bmsmem == NULL)) {
		return (0xffff);
	}
	offset = ((UINT32)(bmsio.bank - 1) << 17) + address - CPUADDR_BMS;
	if ((offset >= bmsiowork.bmsmemsize) || ((bmsiowork.bmsmemsize - offset) < 2)) {
		return (0xffff);
	}
	return (LOADINTELWORD(bmsiowork.bmsmem + offset));
}

static REG16 MEMCALL sysmw_rd(UINT32 address) {
	if (ems_page_frame_active(address) || ems_page_frame_active(address + 1)) {
		return ((REG16)(sysm_rd(address) | (sysm_rd(address + 1) << 8)));
	}
	return (sysmword_read[sysm_bank()](address));
}

static REG16 MEMCALL tvramw_rd(UINT32 address) {
	UINT32 offset;
	REG16 ret;

	offset = address - CPUADDR_SYSM;
	if (offset >= tvram_size()) {
		return (0xffff);
	}
	ret = textmem[offset];
	if ((offset + 1) < tvram_size()) {
		ret |= (REG16)textmem[offset + 1] << 8;
	} else {
		ret |= 0xff00;
	}
	return (ret);
}

static REG16 MEMCALL gvramw_rd_va(UINT32 address) {
	return (gvramw_rd(address - CPUADDR_SYSM));
}

static REG16 MEMCALL knj1w_rd(UINT32 address) {
	return (read_pair16(knj1_rd, address));
}

static REG16 MEMCALL knj2w_rd(UINT32 address) {
	return (read_pair16(knj2_rd, address));
}

static REG16 MEMCALL dic1w_rd(UINT32 address) {
	return (LOADINTELWORD(dicmem + address - CPUADDR_SYSM));
}

static REG16 MEMCALL dic2w_rd(UINT32 address) {
	return (LOADINTELWORD(dicmem + address - (CPUADDR_SYSM - 0x40000)));
}

static REG16 MEMCALL rom0w_rd(UINT32 address) {
	return (rom0word_read[memoryva.rom0_bank & 0x1f](address));
}

static REG16 MEMCALL stdrom0w_rd(UINT32 address) {
	UINT bank;
	UINT32 offset;

	bank = memoryva.rom0_bank & 0x1f;
	if (bank >= 0x0a) {
		return (0xffff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM0;
	return (LOADINTELWORD(rom0mem + offset));
}

static REG16 MEMCALL rom1w_rd(UINT32 address) {
	return (rom1word_read[memoryva.rom1_bank & 0x0f](address));
}

static REG16 MEMCALL stdrom1w_rd(UINT32 address) {
	UINT bank;
	UINT32 offset;

	bank = memoryva.rom1_bank & 0x03;
	if (bank & 0x02) {
		return (rom1invalidw_rd(address));
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM1;
	return (LOADINTELWORD(rom1mem + offset));
}

static REG16 MEMCALL va91sysmw_rd(UINT32 address) {
	return (va91sysmword_read[va91sysm_bank()](address));
}

static REG16 MEMCALL va91knj2w_rd(UINT32 address) {
	return (read_pair16(va91knj2_rd, address));
}

static REG16 MEMCALL va91dic1w_rd(UINT32 address) {
	return (LOADINTELWORD(va91dicmem + address - CPUADDR_SYSM));
}

static REG16 MEMCALL va91dic2w_rd(UINT32 address) {
	return (LOADINTELWORD(va91dicmem + address - (CPUADDR_SYSM - 0x40000)));
}

static REG16 MEMCALL va91rom0w_rd(UINT32 address) {
	UINT bank;
	UINT32 offset;

	bank = va91.rom0_bank;
	if (bank >= 0x0a) {
		return (0xffff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM0;
	return (LOADINTELWORD(va91rom0mem + offset));
}

static REG16 MEMCALL va91rom1w_rd(UINT32 address) {
	UINT bank;
	UINT32 offset;

	bank = va91.rom1_bank;
	if (bank >= 0x02) {
		return (0xffff);
	}
	offset = (((UINT32)bank) << 16) + address - CPUADDR_ROM1;
	return (LOADINTELWORD(va91rom1mem + offset));
}

/* Independent-plane access (port 32h GVAM clear), selected by ports 5Ch-5Fh. */
static BOOL n88_gvram_selected(UINT32 address) {
	return memoryva_88_mode && !memoryva_88_alu.gvam && memoryva_88_plane < 3 &&
	       address >= 0x1c000 && address < 0x20000;
}

/*
 * Extended (ALU) access, after X88000 1.5.3 (public domain),
 * PC88Z80Main.cpp ReadMemoryGVRamEx / WriteMemoryGVRamEx*. Re-implemented.
 */
static BOOL n88_gvram_ex_selected(UINT32 address) {
	return memoryva_88_mode && memoryva_88_alu.gvam && (memoryva_88_alu.port035 & 0x80) &&
	       address >= 0x1c000 && address < 0x20000;
}

static UINT32 n88_plane_offset(UINT plane, UINT32 address) {
	return ((UINT32)plane << 16) + 0x4000 + (address & 0x3fff);
}

static REG8 n88_gvram_ex_read(UINT32 address) {
	REG8 result = 0xff;
	UINT plane;

	for (plane = 0; plane < 3; plane++) {
		const BYTE dat = grphmem[n88_plane_offset(plane, address)];
		memoryva_88_alu.latch[plane] = dat;
		result &= (REG8)(dat ^ ((memoryva_88_alu.port035 & (1 << plane)) ? 0x00 : 0xff));
	}
	return result;
}

static void n88_gvram_ex_write(UINT32 address, REG8 value) {
	UINT plane;

	switch ((memoryva_88_alu.port035 >> 4) & 3) {
	case 0:
		for (plane = 0; plane < 3; plane++) {
			BYTE *p = grphmem + n88_plane_offset(plane, address);
			const UINT op = ((memoryva_88_alu.port034 >> plane) & 1) |
			                (((memoryva_88_alu.port034 >> (plane + 4)) & 1) << 1);

			switch (op) {
			case 0:
				*p &= (BYTE)~value;
				break;
			case 1:
				*p |= (BYTE)value;
				break;
			case 2:
				*p ^= (BYTE)value;
				break;
			}
		}
		break;
	case 1:
		for (plane = 0; plane < 3; plane++) {
			grphmem[n88_plane_offset(plane, address)] = memoryva_88_alu.latch[plane];
		}
		break;
	case 2:
		grphmem[n88_plane_offset(0, address)] = memoryva_88_alu.latch[1];
		break;
	case 3:
		grphmem[n88_plane_offset(1, address)] = memoryva_88_alu.latch[0];
		break;
	}
}

static UINT32 n88_gvram_address(UINT32 address) {
	return ((UINT32)memoryva_88_plane << 16) + 0x4000 + (address & 0x3fff);
}

/*
 * 88-mode TVRAM (BNN manual 8.2.1): with no GVRAM plane selected and port
 * 32h TMODE (bit 4) clear, F000h-FFFFh is the 4 KiB of V3 TVRAM at A6000h.
 */
static BOOL n88_tvram_selected(UINT32 address) {
	const BOOL gvram =
	    memoryva_88_alu.gvam ? (memoryva_88_alu.port035 & 0x80) != 0 : (memoryva_88_plane < 3);
	return memoryva_88_mode && !gvram && !(sysportva.port032 & 0x10) && address >= 0x1f000 &&
	       address < 0x20000;
}

static UINT32 n88_tvram_offset(UINT32 address) {
	return 0x6000 + (address & 0x0fff);
}

/* Provisional 88-mode RAM backing remains physical 10000h..1FFFFh. */
static BOOL n88_ram_window_selected(UINT32 address) {
	return memoryva_88_mode && !(memoryva_88_port31 & 0x06) && address >= 0x18000 &&
	       address < 0x18400;
}

static BOOL n88_dic_selected(UINT32 address) {
	return memoryva_88_mode && !(memoryva_88_dic.enable & 0x01) && address >= 0x1c000 &&
	       address < 0x20000;
}

static REG8 n88_dic_read(UINT32 address) {
	return dicmem[(((UINT32)memoryva_88_dic.bank << 14) + (address & 0x3fff)) &
	              (sizeof(dicmem) - 1)];
}

static BOOL n88_eram_selected(UINT32 address, UINT8 enable) {
	return memoryva_88_mode && (memoryva_88_eram.mode & enable) && address >= 0x10000 &&
	       address < 0x18000;
}

static UINT32 n88_eram_address(UINT32 address) {
	return 0x20000 + ((UINT32)(memoryva_88_eram.bank & 0x0f) << 15) + (address & 0x7fff);
}

static UINT32 n88_ram_window_address(UINT32 address) {
	return 0x10000 + (((memoryva_88_window << 8) + (address & 0x3ff)) & 0xffff);
}

void MEMCALL upd9002_memorywrite_va(UINT32 address, REG8 value) {
	pccore_debugmem(0, address, value);
	if (n88_gvram_ex_selected(address)) {
		n88_gvram_ex_write(address, value);
		return;
	}
	if (n88_tvram_selected(address)) {
		textmem[n88_tvram_offset(address)] = (BYTE)value;
		textmem_dirty = TRUE;
		return;
	}
	if (n88_gvram_selected(address)) {
		grphmem[n88_gvram_address(address)] = (BYTE)value;
		return;
	}
	if (n88_ram_window_selected(address)) {
		upd9002_mainram_write(n88_ram_window_address(address), value);
		return;
	}
	if (n88_eram_selected(address, 0x10)) {
		upd9002_mainram_write(n88_eram_address(address), value);
		return;
	}
	membyte_write[top_index(address)](address, value);
}

void MEMCALL upd9002_memorywrite_va_w(UINT32 address, REG16 value) {
	UINT32 next;

	pccore_debugmem(1, address, value);
	next = address + 1;
	if (n88_ram_window_selected(address) || n88_ram_window_selected(next) ||
	    n88_gvram_selected(address) || n88_gvram_selected(next) || n88_tvram_selected(address) ||
	    n88_tvram_selected(next) || n88_gvram_ex_selected(address) || n88_gvram_ex_selected(next) ||
	    n88_eram_selected(address, 0x10) || n88_eram_selected(next, 0x10)) {
		upd9002_memorywrite_va(address, (REG8)value);
		upd9002_memorywrite_va(next, (REG8)(value >> 8));
		return;
	}
	if (next) {
		memword_write[top_index(next)](address, value);
	} else {
		membyte_write[0](next, (REG8)(value >> 8));
		membyte_write[0x0f](address, (REG8)value);
	}
}

/* N88 ROM overlay and extension banks. Monitor ROM is pending.
 * RAM writes still use the existing physical main-RAM backing. */
/* ROM/RAM mode (port 31h MMODE, bit 1, clear): ROM over 0000h-7FFFh. */
static BOOL n88_rom_selected(UINT32 address) {
	return memoryva_88_mode && !(memoryva_88_port31 & 0x02) && address >= 0x10000 &&
	       address < 0x18000;
}

/*
 * Port 31h RMODE (bit 2) selects the "monitor ROM" (BNN manual, port 31h).
 * The VA has no N-BASIC; the monitor is the Debug 8800 bank at varom00
 * offset 1E000h, appearing at 6000h-7FFFh (CPU document 4.4, Appendix C).
 * `[DERIVED]` 0000h-5FFFh stays N88-BASIC: its MON statement (02E4h) sets
 * RMODE and keeps executing there before checking the "DB" signature at
 * 6000h and jumping to 6002h.
 */
static BOOL n88_monitor_selected(UINT32 address) {
	return (memoryva_88_port31 & 0x04) && (address >= 0x16000);
}

REG8 MEMCALL upd9002_memoryread_va(UINT32 address) {
	if (n88_dic_selected(address)) {
		return n88_dic_read(address);
	}
	if (n88_gvram_ex_selected(address)) {
		return n88_gvram_ex_read(address);
	}
	if (n88_tvram_selected(address)) {
		return textmem[n88_tvram_offset(address)];
	}
	if (n88_gvram_selected(address)) {
		return grphmem[n88_gvram_address(address)];
	}
	if (n88_ram_window_selected(address)) {
		return upd9002_mainram_read(n88_ram_window_address(address));
	}
	if (n88_eram_selected(address, 0x01)) {
		return upd9002_mainram_read(n88_eram_address(address));
	}
	if (n88_rom_selected(address)) {
		/*
		 * vaeg extension: with an N-BASIC ROM supplied, RMODE selects it for
		 * all of 0000h-7FFFh, as on a PC-8801 (N mode). Its top 8 KiB is the
		 * same Debug 8800 bank the VA shows at 6000h-7FFFh under RMODE.
		 */
		if (memoryva_n80_exist && (memoryva_88_port31 & 0x04)) {
			return memoryva_n80[address & 0x7fff];
		}
		if (n88_monitor_selected(address)) {
			return rom0mem[0x1e000 + (address & 0x1fff)];
		}
		if (address >= 0x16000 && !memoryva_88_xerom) {
			return rom1mem[0x18000 + ((sysportva.port032 & 3) << 13) + (address & 0x1fff)];
		}
		return rom1mem[address]; /* N88 payload begins at ROM-image offset 10000h. */
	}
	return (membyte_read[top_index(address)](address));
}

REG16 MEMCALL upd9002_memoryread_va_w(UINT32 address) {
	UINT32 next;
	REG8 lo;
	REG8 hi;

	next = address + 1;
	if (n88_rom_selected(address) || n88_rom_selected(next) || n88_ram_window_selected(address) ||
	    n88_ram_window_selected(next) || n88_gvram_selected(address) || n88_gvram_selected(next) ||
	    n88_tvram_selected(address) || n88_tvram_selected(next) || n88_gvram_ex_selected(address) ||
	    n88_gvram_ex_selected(next) || n88_eram_selected(address, 0x01) ||
	    n88_eram_selected(next, 0x01) || n88_dic_selected(address) || n88_dic_selected(next)) {
		lo = upd9002_memoryread_va(address);
		hi = upd9002_memoryread_va(next);
		return (REG16)lo | ((REG16)hi << 8);
	}
	if (next) {
		return (memword_read[top_index(next)](address));
	}
	hi = membyte_read[0](next);
	lo = membyte_read[0x0f](address);
	return ((REG16)lo | ((REG16)hi << 8));
}

void MEMCALL upd9002_memorymap_va(void) {
	if (va91.cfg.enabled & 1) {
		sysmbyte_write[0x0f] = va91sysm_wt;
		sysmword_write[0x0f] = va91sysmw_wt;
		sysmbyte_read[0x0f] = va91sysm_rd;
		sysmword_read[0x0f] = va91sysmw_rd;
		rom0byte_read[0x0f] = va91rom0_rd;
		rom0word_read[0x0f] = va91rom0w_rd;
		rom1byte_read[0x0f] = va91rom1_rd;
		rom1word_read[0x0f] = va91rom1w_rd;
	} else {
		sysmbyte_write[0x0f] = unmapped_wt;
		sysmword_write[0x0f] = unmappedw_wt;
		sysmbyte_read[0x0f] = unmapped_rd;
		sysmword_read[0x0f] = unmappedw_rd;
		rom0byte_read[0x0f] = stdrom0_rd;
		rom0word_read[0x0f] = stdrom0w_rd;
		rom1byte_read[0x0f] = stdrom1_rd;
		rom1word_read[0x0f] = stdrom1w_rd;
	}
}
