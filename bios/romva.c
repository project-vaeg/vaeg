/*
 * romva.c: PC-88VA ROM loading
 * TODO: Report ROM file load failures.
 */

#include "compiler.h"
#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "fontdata.h"
#include "romva.h"
#include "cgromva.h"
#include "subsystem.h"
#include "cpu/upd9002_upd70008.h"

#define VAFONTROM "vafont.rom"
#define VADICROM "vadic.rom"
#define VAROM00ROM "varom00.rom"
#define VAROM08ROM "varom08.rom"
#define VAROM1ROM "varom1.rom"
#define VAFONTROM_VA2 "vafont_va2.rom"
#define VADICROM_VA2 "vadic_va2.rom"
#define VAROM00ROM_VA2 "varom00_va2.rom"
#define VAROM08ROM_VA2 "varom08_va2.rom"
#define VAROM1ROM_VA2 "varom1_va2.rom"
#define VASUBSYSROM "vasubsys.rom"
/*
 * Optional, user-supplied PC-8001/8801 N-BASIC ROM (vaeg extension). The
 * versioned names are known dumps (identified by SHA-1 in sdl2/n80rom.c);
 * n80.rom is read whatever it holds. Without a menu choice the first file
 * found in this order is used.
 */
const char *const romva_n80_names[ROMVA_N80_NAMES] = {"n80.1.8.rom", "n80.1.2.rom", "n80.rom",
                                                      "N80.ROM"};

static BOOL romva_load_n80(const char *name) {
	char path[MAX_PATH];
	FILEH fh;
	BOOL ok;

	if ((name == NULL) || (name[0] == '\0')) {
		return FALSE;
	}
	getbiospath(path, name, sizeof(path));
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return FALSE;
	}
	ok = (file_getsize(fh) == sizeof(memoryva_n80)) &&
	     (file_read(fh, memoryva_n80, sizeof(memoryva_n80)) == sizeof(memoryva_n80));
	file_close(fh);
	if (ok) {
		milstr_ncpy(memoryva_n80_file, name, sizeof(memoryva_n80_file));
	}
	return ok;
}

#define VAFONTFILE_SIZE 0x50000
#define V98FONTFILE_SIZE 0x46800

/* VA2 names follow MAME's pc88va2 ROM set; do not fall back to VA names. */
static const char *romva_model_filename(const char *va, const char *va2) {
	return ((pccore.model_va == PCMODEL_VA2) ? va2 : va);
}

const char *romva_default_font_filename(void) {
	return romva_model_filename(VAFONTROM, VAFONTROM_VA2);
}

BOOL romva_load_default_font(void) {
	char path[MAX_PATH];
	FILEH fh;
	BOOL success;

	getbiospath(path, romva_default_font_filename(), sizeof(path));
	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return (FALSE);
	}
	if (file_getsize(fh) != VAFONTFILE_SIZE) {
		file_close(fh);
		return (FALSE);
	}
	success = (file_read(fh, fontmem, VAFONTFILE_SIZE) == VAFONTFILE_SIZE);
	file_close(fh);
	if (success) {
		memoryva.sysmromexist |= 0x300;
	}
	return (success);
}

BOOL romva_load_pc98_font(const char *filename) {
	FILEH fh;
	BYTE *v98fnt;
	UINT i;
	UINT j;
	UINT k;
	UINT16 hccode;
	BYTE *left;
	BYTE *right;
	const BYTE *src;

	fh = file_open_rb(filename);
	if (fh == FILEH_INVALID) {
		return (FALSE);
	}
	if (file_getsize(fh) != V98FONTFILE_SIZE) {
		file_close(fh);
		return (FALSE);
	}
	v98fnt = (BYTE *)_MALLOC(V98FONTFILE_SIZE, "va98font");
	if (v98fnt == NULL) {
		file_close(fh);
		return (FALSE);
	}
	if (file_read(fh, v98fnt, V98FONTFILE_SIZE) != V98FONTFILE_SIZE) {
		_MFREE(v98fnt);
		file_close(fh);
		return (FALSE);
	}
	file_close(fh);

	/* VA's fontmem is the guest-visible CGROM address space, not fontrom. */
	FillMemory(fontmem, 0x50000, 0xff);
	CopyMemory(fontmem + 0x41000, v98fnt, 0x0800);
	CopyMemory(fontmem + 0x40000, v98fnt + 0x0800, 0x1000);

	/* FONT.ROM stores 16x16 glyphs as left/right 16-byte halves. */
	for (i = 1; i <= 0x55; i++) {
		for (j = 0; j < 0x60; j++) {
			hccode = (UINT16)((j + 0x20) << 8) | i;
			left = cgromva_font(hccode);
			right = cgromva_font(hccode | 0x8000);
			src = v98fnt + 0x1800 + ((i - 1) * 0x0c00) + (j * 0x20);
			for (k = 0; k < 16; k++) {
				left[k * 2] = src[k];
				right[k * 2] = src[16 + k];
			}
		}
	}

	_MFREE(v98fnt);
	memoryva.sysmromexist |= 0x300;
	return (TRUE);
}

void romva_initialize(void) {
	char path[MAX_PATH];
	FILEH fh;
	BOOL success;
	int i;

	memoryva.rom0exist = 0;
	memoryva.rom1exist = 0;
	memoryva.sysmromexist = 0;
	subsystem.romexist = FALSE;

	romva_load_default_font();
	if (np2cfg.fontfile[0] && !file_cmpname(file_getname(np2cfg.fontfile), pc98fontromname)) {
		getbiospath(path, pc98fontromname, sizeof(path));
		romva_load_pc98_font(path);
	}

	getbiospath(path, romva_model_filename(VADICROM, VADICROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, dicmem, 0x80000) == 0x80000);
		if (success)
			memoryva.sysmromexist |= 0x3000; // bank C,D
		file_close(fh);
	}

	getbiospath(path, romva_model_filename(VAROM00ROM, VAROM00ROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom0mem, 0x80000) == 0x80000);
		if (success)
			memoryva.rom0exist |= 0xff; // bank 0-7
		file_close(fh);
	}

	getbiospath(path, romva_model_filename(VAROM08ROM, VAROM08ROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom0mem + 0x80000, 0x20000) == 0x20000);
		if (success)
			memoryva.rom0exist |= 0x300; // bank 8,9
		file_close(fh);
	}

	getbiospath(path, romva_model_filename(VAROM1ROM, VAROM1ROM_VA2), sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, rom1mem, 0x20000) == 0x20000);
		if (success)
			memoryva.rom1exist |= 0x03; // bank 0,1
		file_close(fh);
	}

	/*
	 * The VA carries no N-BASIC. A 32 KiB N-BASIC ROM placed beside the VA
	 * ROMs is shown at 0000h-7FFFh under port 31h RMODE in 88 mode, as on a
	 * PC-8801 (see memoryva_n80).
	 */
	memoryva_n80_file[0] = '\0';
	memoryva_n80_exist = romva_load_n80(np2cfg.v1v2_n80rom);
	for (i = 0; !memoryva_n80_exist && (i < ROMVA_N80_NAMES); i++) {
		memoryva_n80_exist = romva_load_n80(romva_n80_names[i]);
	}

	getbiospath(path, VASUBSYSROM, sizeof(path));
	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		success = (file_read(fh, subsystem.rom, 0x2000) == 0x2000);
		if (success)
			subsystem.romexist = TRUE;
		file_close(fh);
	}

	upd9002_upd70008_register();
}
