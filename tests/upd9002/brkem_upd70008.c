/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
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
#include "tests/upd9002/brkem_upd70008.h"
#include "cpu/upd9002_upd70008.h"
#include "machine/pccore.h"
#include "io/iocore.h"
#include "io/memctrlva.h"
#include "io/upd9002_regs.h"
#include "io/sysportva.h"
#include "memoryva/memoryva.h"

#include <stdio.h>

#if defined(VAEG_UPD9002_M76_TESTING)
static unsigned native_spy_accesses;

static REG8 IOINPCALL native_spy_in(UINT port) {
	(void)port;
	native_spy_accesses++;
	return 0x99;
}

static void IOOUTCALL native_spy_out(UINT port, REG8 dat) {
	(void)port;
	(void)dat;
	native_spy_accesses++;
}

/* Run one native I/O instruction at 2000:0100; return 1 if it trapped. */
static int native_io_case(const BYTE *code, UINT length, BYTE control, UINT16 dx, UINT vector,
                          UINT16 *saved_ip) {
	UINT i;

	for (i = 0; i < length; i++) {
		mem[0x20100 + i] = code[i];
	}
	upd9002_iotrap.control = control;
	CPU_CS = 0x2000;
	CS_BASE = 0x20000;
	CPU_SS = 0x3000;
	SS_BASE = 0x30000;
	CPU_SP = 0x0100;
	CPU_IP = 0x0100;
	CPU_DX = dx;
	CPU_AX = 0;
	CPU_FLAG = 0xf202;
	CPU_REMCLOCK = 100000;
	upd9002_core_step();
	*saved_ip = (UINT16)(mem[0x300fa] | (mem[0x300fb] << 8));
	return CPU_CS == 0x2000 && CPU_IP == (vector == 0x7c ? 0x3000 : 0x3100) && CPU_SP == 0x00fa &&
	       !(CPU_FLAG & I_FLAG);
}

static int native_iotrap_selftest(void) {
	static const BYTE in_imm[] = {0xe4, 0x50};
	static const BYTE in_imm_w[] = {0xe5, 0x5b};
	static const BYTE out_imm[] = {0xe6, 0x60};
	static const BYTE out_imm_w[] = {0xe7, 0x6f};
	static const BYTE in_imm_miss[] = {0xe4, 0x5c};
	static const BYTE in_dx[] = {0xec};
	static const BYTE in_dx_w[] = {0xed};
	static const BYTE out_dx[] = {0xee};
	static const BYTE out_dx_w[] = {0xef};
	static const BYTE prefixed[] = {0x2e, 0xe4, 0x50};
	static const BYTE ranges[8] = {0x50, 0, 0x5b, 0, 0x60, 0, 0x6f, 0};
	UINT16 saved;
	int passed;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_core_reset();
	iocore_create();
	if (iocore_build() != SUCCESS) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_regs_bind();
	iocore_attachinp(0x0050, native_spy_in);
	iocore_attachinp(0x005b, native_spy_in);
	iocore_attachinp(0x005c, native_spy_in);
	iocore_attachout(0x0060, native_spy_out);
	iocore_attachout(0x006f, native_spy_out);
	iocore_attachinp(0x1050, native_spy_in);
	iocore_attachout(0x1050, native_spy_out);
	mem[0x7c * 4 + 0] = 0x00;
	mem[0x7c * 4 + 1] = 0x30;
	mem[0x7c * 4 + 2] = 0x00;
	mem[0x7c * 4 + 3] = 0x20;
	mem[0x7d * 4 + 0] = 0x00;
	mem[0x7d * 4 + 1] = 0x31;
	mem[0x7d * 4 + 2] = 0x00;
	mem[0x7d * 4 + 3] = 0x20;
	CopyMemory(upd9002_iotrap.ranges, ranges, sizeof(ranges));
	native_spy_accesses = 0;

	/* All eight encodings trap at both range edges with no device access. */
	passed = native_io_case(in_imm, 2, 1, 0, 0x7c, &saved) && saved == 0x0100;
	passed = passed && native_io_case(in_imm_w, 2, 1, 0, 0x7c, &saved) && saved == 0x0100;
	passed = passed && native_io_case(out_imm, 2, 2, 0, 0x7d, &saved) && saved == 0x0100;
	passed = passed && native_io_case(out_imm_w, 2, 2, 0, 0x7d, &saved) && saved == 0x0100;
	passed = passed && native_io_case(in_dx, 1, 1, 0x0050, 0x7c, &saved) && saved == 0x0100;
	passed = passed && native_io_case(in_dx_w, 1, 1, 0x0050, 0x7c, &saved);
	passed = passed && native_io_case(out_dx, 1, 2, 0x006f, 0x7d, &saved);
	passed = passed && native_io_case(out_dx_w, 1, 2, 0x0060, 0x7d, &saved);
	/* A redundant segment prefix is saved like the core's fault restart. */
	passed = passed && native_io_case(prefixed, 3, 1, 0, 0x7c, &saved) && saved == 0x0100;
	passed = passed && native_spy_accesses == 0;

	/* Outside the range, direction disabled, and byte-port 16-bit compare. */
	passed = passed && !native_io_case(in_imm_miss, 2, 3, 0, 0x7c, &saved) && CPU_AL == 0x99 &&
	         native_spy_accesses == 1;
	passed = passed && !native_io_case(in_imm, 2, 2, 0, 0x7c, &saved) && native_spy_accesses == 2;
	passed =
	    passed && !native_io_case(in_dx, 1, 1, 0x1050, 0x7c, &saved) && native_spy_accesses == 3;
	/* Word-port mode (bit 4) matches the low byte only. */
	passed = passed && native_io_case(in_dx, 1, 0x11, 0x1050, 0x7c, &saved) &&
	         native_io_case(out_dx, 1, 0x12, 0x1050, 0x7d, &saved) && native_spy_accesses == 3;
	/* FFE0h-FFFFh never trap: a full-range trap still lets OUT FFEFh disable it. */
	upd9002_iotrap.ranges[2] = 0xff;
	upd9002_iotrap.ranges[3] = 0xff;
	passed = passed && !native_io_case(out_dx, 1, 3, 0xffef, 0x7d, &saved) &&
	         upd9002_iotrap.control == 0;

	ZeroMemory(&upd9002_iotrap, sizeof(upd9002_iotrap));
	iocore_destroy();
	upd9002_core_deinitialize();
	return passed ? SUCCESS : FAILURE;
}

static int iotrap_register_selftest(void) {
	static const BYTE ranges[8] = {0x50, 0, 0x5b, 0, 0x60, 0, 0x6f, 0};
	UPD9002_IOTRAP byte_state;
	int passed;
	UINT i;

	iocore_create();
	if (iocore_build() != SUCCESS) {
		iocore_destroy();
		return FAILURE;
	}
	upd9002_regs_reset();
	upd9002_regs_bind();
	/* The firmware installs ranges as descending byte writes. */
	for (i = 8; i > 0; i--) {
		iocore_out8(0xffe0 + i - 1, ranges[i - 1]);
	}
	passed = !memcmp(upd9002_iotrap.ranges, ranges, sizeof(ranges)) && upd9002_iotrap.control == 0;
	iocore_out8(0xffef, 3);
	byte_state = upd9002_iotrap;
	upd9002_regs_reset();
	passed = passed && upd9002_iotrap.control == 0;
	for (i = 0; i < 8; i++)
		passed = passed && upd9002_iotrap.ranges[i] == 0;
	for (i = 0; i < 8; i += 2) {
		iocore_out16(0xffe0 + i, ranges[i]);
	}
	iocore_out8(0xffef, 3);
	passed = passed && !memcmp(&byte_state, &upd9002_iotrap, sizeof(byte_state));
	iocore_out8(0xffef, 0);
	passed = passed && upd9002_iotrap.control == 0 &&
	         !memcmp(upd9002_iotrap.ranges, ranges, sizeof(ranges));
	upd9002_regs_reset();
	iocore_destroy();
	return passed ? SUCCESS : FAILURE;
}

static int memory_mode_selftest(void) {
	int passed;

	iocore_create();
	if (iocore_build() != SUCCESS) {
		iocore_destroy();
		return FAILURE;
	}
	memctrlva_reset();
	memctrlva_bind();
	passed = (memoryva_88_mode == 0) && (iocore_inp8(0x153) == 0x41);
	iocore_out8(0x153, 0x03);
	passed = passed && (memoryva_88_mode == 1) && (iocore_inp8(0x153) == 0x03);
	iocore_out8(0x153, 0x43);
	passed = passed && (memoryva_88_mode == 0) && (iocore_inp8(0x153) == 0x43);
	/* The ROM switches modes with a word access starting at port 152h. */
	iocore_out16(0x152, 0x0100);
	passed = passed && (memoryva_88_mode == 1) && (iocore_inp16(0x152) == 0x0100);
	iocore_out16(0x152, 0x4100);
	passed = passed && (memoryva_88_mode == 0) && (iocore_inp16(0x152) == 0x4100);
	/* Synthetic ROM/RAM sentinels test both edges and RAM under the ROM. */
	{
		const BYTE saved_first = rom1mem[0x10000];
		const BYTE saved_last = rom1mem[0x17fff];
		rom1mem[0x10000] = 0xa5;
		rom1mem[0x17fff] = 0x5a;
		upd9002_memorywrite_va(0x0ffff, 0x11);
		upd9002_memorywrite_va(0x10000, 0x22);
		upd9002_memorywrite_va(0x17fff, 0x33);
		upd9002_memorywrite_va(0x18000, 0x44);
		passed = passed && (upd9002_memoryread_va(0x10000) == 0x22);
		iocore_out8(0x153, 0x01);
		passed = passed && (upd9002_memoryread_va(0x10000) == 0xa5) &&
		         (upd9002_memoryread_va_w(0x0ffff) == 0xa511) &&
		         (upd9002_memoryread_va_w(0x17fff) == 0x445a);
		upd9002_memorywrite_va_w(0x17fff, 0x6677);
		passed =
		    passed && (upd9002_memoryread_va_w(0x17fff) == 0x665a) && (rom1mem[0x17fff] == 0x5a);
		iocore_out8(0x031, 0x02); /* MMODE: all RAM. */
		passed = passed && (upd9002_memoryread_va(0x10000) == 0x22) &&
		         (upd9002_memoryread_va_w(0x17fff) == 0x6677);
		iocore_out8(0x031, 0x00);
		passed = passed && (upd9002_memoryread_va(0x10000) == 0xa5);
		/* RMODE: the Debug 8800 bank (varom00 1E000h) at 6000h-7FFFh, with
		 * N88-BASIC kept below it; MMODE still removes all ROM. */
		{
			const BYTE saved_mon[2] = {rom0mem[0x1e000], rom0mem[0x1ffff]};
			const BYTE saved_low = rom1mem[0x15fff];
			rom0mem[0x1e000] = 0x44;
			rom0mem[0x1ffff] = 0x99;
			rom1mem[0x15fff] = 0x3c;
			iocore_out8(0x071, 0xfe); /* the extension ROM must not win */
			iocore_out8(0x031, 0x04);
			passed = passed && (upd9002_memoryread_va(0x10000) == 0xa5) &&
			         (upd9002_memoryread_va(0x16000) == 0x44) &&
			         (upd9002_memoryread_va_w(0x15fff) == 0x443c) &&
			         (upd9002_memoryread_va_w(0x17fff) == 0x6699);
			iocore_out8(0x031, 0x06);
			passed = passed && (upd9002_memoryread_va(0x16000) != 0x44) &&
			         (upd9002_memoryread_va(0x10000) == 0x22);
			iocore_out8(0x031, 0x00);
			iocore_out8(0x071, 0xff);
			passed = passed && (upd9002_memoryread_va(0x16000) == rom1mem[0x16000]);
			rom0mem[0x1e000] = saved_mon[0];
			rom0mem[0x1ffff] = saved_mon[1];
			rom1mem[0x15fff] = saved_low;
		}
		iocore_out8(0x153, 0x41);
		passed = passed && (upd9002_memoryread_va(0x10000) == 0x22);
		rom1mem[0x10000] = saved_first;
		rom1mem[0x17fff] = saved_last;
	}
	{
		BYTE saved[8];
		const BYTE saved_edge = rom1mem[0x15fff];
		const BYTE saved_bank = sysportva.port032;
		UINT bank;
		systemportva_bind();
		iocore_out8(0x153, 0x01);
		rom1mem[0x15fff] = 0xb0;
		passed = passed && iocore_inp8(0x71) == 0xff;
		for (bank = 0; bank < 4; bank++) {
			UINT offset = 0x18000 + bank * 0x2000;
			saved[bank * 2] = rom1mem[offset];
			saved[bank * 2 + 1] = rom1mem[offset + 0x1fff];
			rom1mem[offset] = 0xa0 + bank;
			rom1mem[offset + 0x1fff] = 0xc0 + bank;
		}
		iocore_out8(0x71, 0xfe);
		passed = passed && iocore_inp8(0x71) == 0xfe;
		for (bank = 0; bank < 4; bank++) {
			iocore_out8(0x32, (saved_bank & ~3) | bank);
			passed = passed && upd9002_memoryread_va(0x16000) == 0xa0 + bank &&
			         upd9002_memoryread_va_w(0x15fff) == (0xa0 + bank) * 256 + 0xb0 &&
			         upd9002_memoryread_va_w(0x17fff) == 0x6600 + 0xc0 + bank;
		}
		iocore_out8(0x71, 0xff);
		passed = passed && upd9002_memoryread_va(0x17fff) == rom1mem[0x17fff];
		for (bank = 0; bank < 4; bank++) {
			UINT offset = 0x18000 + bank * 0x2000;
			rom1mem[offset] = saved[bank * 2];
			rom1mem[offset + 0x1fff] = saved[bank * 2 + 1];
		}
		rom1mem[0x15fff] = saved_edge;
		iocore_out8(0x32, saved_bank);
	}
	iocore_out8(0x153, 0x01);
	/* TMODE 1: 88-mode F000h-FFFFh is main RAM rather than TVRAM, which
	 * these RAM-window and GVRAM checks rely on (TVRAM: vaeg --selftest). */
	iocore_out8(0x32, 0x10);
	iocore_out8(0x31, 2); /* Fill the RAM behind the ROM/window. */
	upd9002_memorywrite_va(0x1ff00, 0x91);
	upd9002_memorywrite_va(0x10000, 0x92);
	upd9002_memorywrite_va(0x102ff, 0x93);
	upd9002_memorywrite_va(0x18400, 0x94);
	iocore_out8(0x70, 0xff);
	iocore_out8(0x31, 0);
	passed = passed && iocore_inp8(0x70) == 0xff && upd9002_memoryread_va(0x18000) == 0x91 &&
	         upd9002_memoryread_va(0x18100) == 0x92 && upd9002_memoryread_va_w(0x183ff) == 0x9493;
	upd9002_memorywrite_va_w(0x183ff, 0xa2a1);
	iocore_out8(0x31, 2);
	passed =
	    passed && upd9002_memoryread_va(0x102ff) == 0xa1 && upd9002_memoryread_va(0x18400) == 0xa2;
	iocore_out8(0x31, 0);
	iocore_out8(0x78, 0x55); /* Value ignored; FF wraps to 00. */
	passed = passed && iocore_inp8(0x70) == 0 && upd9002_memoryread_va(0x18000) == 0x92;
	iocore_out8(0x78, 0xaa);
	passed = passed && iocore_inp8(0x70) == 1;
	iocore_out8(0x153, 0x41);
	passed = passed && upd9002_memoryread_va(0x18000) == 0x66;
	memctrlva_reset();
	passed = passed && iocore_inp8(0x70) == 0x80;
	{
		UINT plane;
		upd9002_memorywrite_va(0x1bfff, 0x11);
		upd9002_memorywrite_va(0x1c000, 0x22);
		upd9002_memorywrite_va(0x1ffff, 0x33);
		upd9002_memorywrite_va(0x20000, 0x44);
		passed = passed && iocore_inp8(0x5c) == 0xf8;
		iocore_out8(0x153, 0x01);
		for (plane = 0; plane < 3; plane++) {
			iocore_out8(0x5c + plane, 0xff);
			upd9002_memorywrite_va(0x1c000, 0x80 + plane);
			upd9002_memorywrite_va_w(0x1ffff, 0x4460 + plane);
		}
		for (plane = 0; plane < 3; plane++) {
			iocore_out8(0x5c + plane, 0);
			passed = passed && iocore_inp8(0x5c) == (0xf8 | (1 << plane)) &&
			         upd9002_memoryread_va_w(0x1bfff) == ((0x80 + plane) << 8 | 0x11) &&
			         upd9002_memoryread_va_w(0x1ffff) == 0x4460 + plane;
		}
		iocore_out8(0x5f, 0);
		passed = passed && iocore_inp8(0x5c) == 0xf8 && upd9002_memoryread_va(0x1c000) == 0x22 &&
		         upd9002_memoryread_va(0x1ffff) == 0x33;
		iocore_out8(0x5c, 0);
		iocore_out8(0x153, 0x41);
		passed = passed && upd9002_memoryread_va(0x1c000) == 0x22;
		memctrlva_reset();
		passed = passed && iocore_inp8(0x5c) == 0xf8;
	}
	passed = passed && (memoryva_88_mode == 0) && (iocore_inp8(0x153) == 0x41);
	iocore_destroy();
	return passed ? SUCCESS : FAILURE;
}
#endif

int upd9002_brkem_upd70008_main(void) {
#if defined(VAEG_UPD9002_M76_TESTING)
	if (upd9002_upd70008_compat_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: production uPD70008-compatible bridge failed\n");
		return FAILURE;
	}
	if (upd9002_upd70008_alt_regs_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: alternate register storage failed\n");
		return FAILURE;
	}
	if (upd9002_upd70008_cpu_speed_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: CPU speed in compatible mode failed\n");
		return FAILURE;
	}
	if (native_iotrap_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: native I/O trap failed\n");
		return FAILURE;
	}
	fprintf(stderr, "upd9002-brkem-upd70008: native I/O trap forms, ranges and exemption passed\n");
	if (iotrap_register_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: I/O trap register byte/word writes failed\n");
		return FAILURE;
	}
	if (memory_mode_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: memory-mode latch failed\n");
		return FAILURE;
	}
	fprintf(stderr, "upd9002-brkem-upd70008: memory-mode byte/word I/O and reset passed\n");
	fprintf(
	    stderr,
	    "upd9002-brkem-upd70008: BRKEM/BRKEM2, Z80 JR/IX/IY, CALLN/IRET, LD HL, RETEM passed\n");
	fprintf(stderr, "upd9002-brkem-upd70008: alternate register storage, state authority and "
	                "load-before-enter vector reader passed\n");
	return SUCCESS;
#else
	return FAILURE;
#endif
}
