/*
 * va91.c: PC-88VA-91 Software version-up board
 */

#include "compiler.h"
#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"

#include "va91.h"

#define VUROM00ROM "VUROM00.ROM"
#define VUROM08ROM "VUROM08.ROM"
#define VUROM1ROM "VUROM1.ROM"
#define VUDICROM "VUDIC.ROM"

// ---- I/O

static void IOOUTCALL va91_off2(UINT port, REG8 dat) {
	//	if (dat>=0x10)
	//	TRACEOUT(("va91: out %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	va91.rom0_bank = ((dat & 0x40) >> 2) | (dat & 0x0f);
	va91.rom1_bank = ((dat & 0xb0) >> 4);
}

static void IOOUTCALL va91_off3(UINT port, REG8 dat) {
	//	if (dat!=9 && dat!=0x0c && dat!=0x0d)
	//	TRACEOUT(("va91: out %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	va91.sysm_bank = dat & 0x0f;
	fdc_trace_text("banktrace port=%03x val=%02x va91_sysm_bank=%02x", port, dat, va91.sysm_bank);
}

static REG8 IOINPCALL va91_iff2(UINT port) {
	return va91.rom0_bank & 0x0f;
}

static REG8 IOINPCALL va91_iff3(UINT port) {
	return (va91.sysm_bank & 0x0f) | 0xf0;
}

REG8 va91_rombankstatus(void) {
	return (va91.cfg.enabled) ? 0x7f : 0xff;
}

// ---- I/F

void va91_reset(void) {
	// Apply configured VA91 board settings to the runtime state.
	// Called on emulator reset, but not while loading a saved state.
	va91.cfg = va91cfg;
	if (pccore.model_va != PCMODEL_VA1) {
		// The VA91 upgrade board is available only for the original VA model.
		va91.cfg.enabled = FALSE;
	}

	// Restore the board ROM-bank selection.
	va91_off2(0, 0);
	va91_off3(0, 0);
}

void va91_bind(void) {
	if (va91.cfg.enabled) {
		iocore_attachout(0xff2, va91_off2);
		iocore_attachout(0xff3, va91_off3);

		iocore_attachinp(0xff2, va91_iff2);
		iocore_attachinp(0xff3, va91_iff3);
	}
}

void va91_initialize(void) {
	char path[MAX_PATH];
	FILEH fh;
	BOOL success;

	if (!va91.cfg.enabled)
		return;

	getbiospath(path, VUDICROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, va91dicmem, 0x80000) == 0x80000);
		if (success)
			va91cfg.sysmromexist |= 0x3000; // bank C,D
		file_close(fh);
	}

	getbiospath(path, VUROM00ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, va91rom0mem, 0x80000) == 0x80000);
		if (success)
			va91cfg.rom0exist |= 0xff; // bank 0-7
		file_close(fh);
	}

	getbiospath(path, VUROM08ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, va91rom0mem + 0x80000, 0x20000) == 0x20000);
		if (success)
			va91cfg.rom0exist |= 0x0300; // bank 8-9
		file_close(fh);
	}

	getbiospath(path, VUROM1ROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, va91rom1mem, 0x20000) == 0x20000);
		if (success)
			va91cfg.rom1exist |= 0x03; // bank 0,1
		file_close(fh);
	}
}
