// フロッピーディスクの挿入延滞用のブリッヂ

#include "compiler.h"
#include "dosio.h"
#include "sysmng.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "diskdrv.h"
#include "fddfile.h"

#if defined(SUPPORT_OPRECORD)
#include "oprecord.h"
#endif

#define DISK_DELAY 20 // (0.4sec)

int diskdrv_delay[4];
int diskdrv_ro[4];
char diskdrv_fname[4][MAX_PATH];
static UINT8 diskdrv_image[4];

void diskdrv_sethdd(REG8 drv, const char *fname) {
	UINT num;
	char *p;
	int leng;

	num = drv & 0x0f;
	p = NULL;
	leng = 0;
	if (!(drv & 0x20)) { // SASI or IDE
		if (num < 2) {
			p = np2cfg.sasihdd[num];
			leng = sizeof(np2cfg.sasihdd[0]);
		}
	} else { // SCSI
		if (num < 7) {
			p = np2cfg.scsihdd[num];
			leng = sizeof(np2cfg.scsihdd[0]);
		}
	}
	if (p) {
		if (fname) {
			file_cpyname(p, fname, leng);
		} else {
			p[0] = '\0';
		}
		sysmng_update(SYS_UPDATEHDD | SYS_UPDATECFG);
	}
}

void diskdrv_setfdd(REG8 drv, const char *fname, int readonly) {
	diskdrv_setfddex(drv, fname, readonly, 0);
}

/* `image` selects a disk inside a multi-image D88 file (0 = first). */
void diskdrv_setfddex(REG8 drv, const char *fname, int readonly, UINT image) {
	if ((drv < 4) && (fdc.equip & (1 << drv))) {
		diskdrv_image[drv] = (UINT8)image;
		fdd_eject(drv);
		diskdrv_delay[drv] = 0;
		diskdrv_fname[drv][0] = '\0';
		fdc.stat[drv] = FDCRLT_AI | FDCRLT_NR | drv;
		fdc_interrupt();

		if (fname) {
			diskdrv_delay[drv] = DISK_DELAY;
			diskdrv_ro[drv] = readonly;
			file_cpyname(diskdrv_fname[drv], fname, sizeof(diskdrv_fname[0]));
		}
		sysmng_update(SYS_UPDATEFDD);
#if defined(SUPPORT_OPRECORD)
		oprecord_record_fdd((UINT8)drv, fname, (UINT8)readonly);
#endif
	}
}

void diskdrv_callback(void) {
	REG8 drv;

	for (drv = 0; drv < 4; drv++) {
		if (diskdrv_delay[drv]) {
			diskdrv_delay[drv]--;
			if ((!diskdrv_delay[drv]) && (diskdrv_fname[drv][0])) {
				fddfile[drv].num = diskdrv_image[drv];
				fdd_set(drv, diskdrv_fname[drv], FTYPE_NONE, diskdrv_ro[drv]);
				diskdrv_fname[drv][0] = '\0';
				fdc.stat[drv] = FDCRLT_AI | drv;
				fdc_interrupt();
				sysmng_update(SYS_UPDATEFDD);
			}
		}
	}
}
