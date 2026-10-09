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
#include "upd9002_ops.h"
#include "memoryva.h"
#include "tests/upd9002/m96d_nop.h"

#include <stdio.h>

enum {
	M96D_MSW5_OFFSET = 0xa3ff2
};

static void setup_nop_at_physical_address(void) {
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
	CPU_CS = 0xfffe;
	CPU_SS = 0x4000;
	CPU_DS = 0x5000;
	CPU_IP = 0x0008;
	CPU_FLAG = 0xf002;
	ES_BASE = (UINT32)CPU_ES << 4;
	CS_BASE = (UINT32)CPU_CS << 4;
	SS_BASE = (UINT32)CPU_SS << 4;
	DS_BASE = (UINT32)CPU_DS << 4;
	upd9002_core_context.s.ss_fix = SS_BASE;
	upd9002_core_context.s.ds_fix = DS_BASE;
	CPU_ADRSMASK = 0x000fffff;
	CPU_REMCLOCK = 5000;
	CPU_BASECLOCK = 10000;
	CPU_CLOCK = 0;
	CPU_ITFBANK = 0;
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;

	/* 0xfffe8 is the historical simulated-BIOS hook address. */
	/* VA ROM1 backing supplies the F0000H-FFFFFH window. Evidence: M96 report section 11. */
	rom1mem[0xffe8] = 0x90;
	/* Select the ROM/default path if the old hook is accidentally called. */
	mem[M96D_MSW5_OFFSET] = 0xf0;
}

static int test_nop_has_no_physical_bios_side_channel(void) {
	UINT32 original_es_base;
	UINT32 original_cs_base;
	UINT32 original_ss_base;
	UINT32 original_ds_base;

	setup_nop_at_physical_address();
	original_es_base = ES_BASE;
	original_cs_base = CS_BASE;
	original_ss_base = SS_BASE;
	original_ds_base = DS_BASE;
	upd9002_core_step();

	if ((CPU_REMCLOCK != 4997) || (CPU_CS != 0xfffe) || (CPU_IP != 0x0009) ||
	    (ES_BASE != original_es_base) || (CS_BASE != original_cs_base) ||
	    (SS_BASE != original_ss_base) || (DS_BASE != original_ds_base)) {
		fprintf(
		    stderr,
		    "upd9002-m96d-nop: side channel check failed (clock=%d cs=%04x ip=%04x bases=%08x/%08x/%08x/%08x)\n",
		    CPU_REMCLOCK, CPU_CS, CPU_IP, (unsigned)ES_BASE, (unsigned)CS_BASE, (unsigned)SS_BASE,
		    (unsigned)DS_BASE);
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_m96d_nop_main(void) {
	upd9002_core_initialize();
	if (test_nop_has_no_physical_bios_side_channel() != SUCCESS) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_deinitialize();
	puts("upd9002-m96d-nop: plain NOP check passed");
	return SUCCESS;
}
