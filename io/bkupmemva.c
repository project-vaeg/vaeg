/*
 *	BKMEMVA.C: PC-88VA Backup memory
 *	ToDo:
 *		ファイル読み書き失敗時のエラー通知
 */

#include "compiler.h"
#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "bkupmemva.h"

#define VABKUPMEM "vabkupmem.dat"

static char bkupmemva_path[MAX_PATH];
static BOOL bkupmemva_enabled = TRUE;

void bkupmemva_setpath(const char *path) {
	file_cpyname(bkupmemva_path, (path != NULL) ? path : "", sizeof(bkupmemva_path));
}

void bkupmemva_setenabled(BOOL enabled) {
	bkupmemva_enabled = enabled ? TRUE : FALSE;
}

static int bkupmemva_read(const char *path) {
	FILEH fh;

	fh = file_open_rb(path);
	if (fh != FILEH_INVALID) {
		int result;

		result = (file_read(fh, backupmem, 0x04000) == 0x04000) ? SUCCESS : FAILURE;
		file_close(fh);
		return result;
	}
	return FAILURE;
}

static void bkupmemva_statepath(char *path, int size) {
	if (bkupmemva_path[0] != '\0') {
		file_cpyname(path, bkupmemva_path, size);
		return;
	}
	file_cpyname(path, VABKUPMEM, size);
}

/*
 * Checksum of the memory-switch record B1FC0h-B1FC7h at B1FCDh, computed as
 * the VA2 ROM checks it at reset (F000:23B7): the ROM first sets bit 7 of
 * B1FC6h, which it later recomputes from the V1/V2 selection, so the sum
 * uses B1FC6h with bit 7 set. A mismatch makes the ROM restore defaults.
 */
static void bkupmemva_update_checksum(void) {
	UINT8 checksum = 0;
	int i;

	for (i = 0; i < 8; i++) {
		checksum = (UINT8)(checksum + backupmem[0x1fc0 + i]);
	}
	if (!(backupmem[0x1fc6] & 0x80)) {
		checksum = (UINT8)(checksum + 0x80);
	}
	backupmem[0x1fcd] = checksum;
}

void bkupmemva_sync_mainram(void) {
	if (!np2cfg.main_ram_auto) {
		return;
	}
	/* Only capacity bits and their checksum belong to automatic syncing. */
	backupmem[0x1fc4] = (BYTE)((backupmem[0x1fc4] & 0xf8) |
	                         ((pccore_mainram_kb() / 128) - 1));
	bkupmemva_update_checksum();
}

/*
 * V1/V2 system mode: B1FC5h bit 0 (1 = V1), which the VA2 ROM writes to
 * port 1C6h at reset (F000:12E4) and reflects in port 150h and in memory
 * switch MS27 (IN 31h bit 7). Takes effect at the next reset.
 */
BOOL bkupmemva_get_88v1(void) {
	return (backupmem[0x1fc5] & 0x01) ? TRUE : FALSE;
}

void bkupmemva_set_88v1(BOOL v1) {
	backupmem[0x1fc5] = (BYTE)((backupmem[0x1fc5] & 0xfe) | (v1 ? 0x01 : 0x00));
	bkupmemva_update_checksum();
}

static void bkupmemva_initialize_mainram(void) {
	ZeroMemory(backupmem, 0x04000);
	/* VA BIOS backup record: memory settings, signature, reserved bytes, checksum. */
	backupmem[0x1fc2] = 0xdb;
	backupmem[0x1fc3] = 0x02;
	backupmem[0x1fc4] = 0x60;
	backupmem[0x1fc5] = 0x04;
	backupmem[0x1fc6] = 0xf9;
	backupmem[0x1fc7] = 0x0a;
	backupmem[0x1fc8] = 0x4b;
	backupmem[0x1fc9] = 0x5a;
	backupmem[0x1fca] = 0x4d;
	backupmem[0x1fcb] = 0xff;
	backupmem[0x1fcc] = 0xff;
	bkupmemva_sync_mainram();
}

void bkupmemva_load(void) {
	char path[MAX_PATH];

	if (!bkupmemva_enabled) {
		return;
	}
	/* A missing model-specific file starts with a clean backup-RAM image. */
	ZeroMemory(backupmem, 0x04000);
	bkupmemva_statepath(path, sizeof(path));
	if (bkupmemva_read(path) != SUCCESS) {
		ZeroMemory(backupmem, 0x04000);
		if (!np2cfg.main_ram_auto) {
			return;
		}
		/* Seed the BIOS selection from the installed physical-RAM ceiling. */
		bkupmemva_initialize_mainram();
	}
}

void bkupmemva_save(void) {
	char path[MAX_PATH];
	FILEH fh;

	if (!bkupmemva_enabled) {
		return;
	}
	bkupmemva_statepath(path, sizeof(path));
	fh = file_create(path);
	if (fh != FILEH_INVALID) {
		(void)file_write(fh, backupmem, 0x04000);
		file_close(fh);
	}
}
