#include "compiler.h"
#include "strres.h"
#include "dosio.h"
#include "sysmng.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "sxsi.h"
#include "newdisk.h"
#include <stdio.h>
#include <limits.h>

const char sig_vhd[8] = "VHD1.00";
const char sig_nhd[15] = "T98HDDIMAGE.R0";

static UINT32 sxsi_fingerprint_update(UINT32 digest, const BYTE *data, UINT count) {
	UINT i;

	for (i = 0; i < count; i++) {
		digest ^= data[i];
		digest *= 16777619U;
	}
	return digest;
}

static UINT32 sxsi_file_fingerprint(FILEH fh, UINT32 headersize, UINT32 size) {
	BYTE sample[256];
	UINT count;
	UINT32 digest;

	digest = 2166136261U;
	count = min(headersize, (UINT32)sizeof(sample));
	if (file_seek(fh, 0, FSEEK_SET) != 0 || file_read(fh, sample, count) != count) {
		return 0;
	}
	digest = sxsi_fingerprint_update(digest, sample, count);
	count = min(size, (UINT32)sizeof(sample));
	if (file_seek(fh, (long)headersize, FSEEK_SET) != (long)headersize ||
	    file_read(fh, sample, count) != count) {
		return 0;
	}
	return sxsi_fingerprint_update(digest, sample, count);
}

const SASIHDD sasihdd[7] = {
    {33, 4, 153},  // 5MB
    {33, 4, 310},  // 10MB
    {33, 6, 310},  // 15MB
    {33, 8, 310},  // 20MB	dipswの値が 3→98のBIOSでは対応しているが、一般的には使われない
    {33, 4, 615},  // 20MB dipswの値が 4→VA/98のBIOSで対応しているタイプ
    {33, 6, 615},  // 30MB
    {33, 8, 615}}; // 40MB

#if 0
static const _SXSIDEV defide = {615*33*8, 615, 256, 33, 8,
								SXSITYPE_IDE | SXSITYPE_HDD, 256, 0, {0x00}};
static const _SXSIDEV defscsi = {40*16*32*8, 40*16, 256, 32, 8,
								SXSITYPE_SCSI | SXSITYPE_HDD, 220, 0, {0x00}};
#endif

_SXSIDEV sxsi_dev[SASIHDD_MAX + SCSIHDD_MAX];

// SASI規格HDDかチェック
static void sasihddcheck(SXSIDEV sxsi) {
	const SASIHDD *sasi;
	UINT i;

	sasi = sasihdd;
	for (i = 0; i < sizeof(sasihdd) / sizeof(SASIHDD); i++, sasi++) {
		if ((sxsi->size == 256) && (sxsi->sectors == sasi->sectors) &&
		    (sxsi->surfaces == sasi->surfaces) && (sxsi->cylinders == sasi->cylinders)) {
			sxsi->type = (UINT16)(SXSITYPE_SASI + (i << 8) + SXSITYPE_HDD);
			break;
		}
	}
}

// ----

void sxsi_initialize(void) {
	UINT i;

	ZeroMemory(sxsi_dev, sizeof(sxsi_dev));
	for (i = 0; i < (sizeof(sxsi_dev) / sizeof(_SXSIDEV)); i++) {
		sxsi_dev[i].fh = FILEH_INVALID;
	}
}

SXSIDEV sxsi_getptr(REG8 drv) {
	UINT num;

	num = drv & 0x0f;
	if (!(drv & 0x20)) { // SASI or IDE
		if (num < SASIHDD_MAX) {
			return (sxsi_dev + num);
		}
	} else {
		if (num < SCSIHDD_MAX) { // SCSI
			return (sxsi_dev + SASIHDD_MAX + num);
		}
	}
	return (NULL);
}

const char *sxsi_getname(REG8 drv) {
	SXSIDEV sxsi;

	sxsi = sxsi_getptr(drv);
	if (sxsi) {
		return (sxsi->fname);
	}
	return (NULL);
}

static BOOL sxsi_hddopen_device(REG8 drv, const char *file, SXSIDEV sxsi, BOOL log_mount) {
	FILEH fh;
	const char *ext;
	UINT16 type;
	long totals;
	UINT32 headersize;
	UINT32 surfaces;
	UINT32 cylinders;
	UINT32 sectors;
	UINT32 size;

	if ((file == NULL) || (file[0] == '\0') || (sxsi == NULL)) {
		goto sxsiope_err1;
	}
	fh = file_open(file);
	if (fh == FILEH_INVALID) {
		goto sxsiope_err1;
	}
	ext = file_getext((char *)file);
	type = SXSITYPE_HDD;
	if ((!file_cmpname(ext, str_thd)) && (!(drv & 0x20))) {
		THDHDR thd; // T98 HDD (IDE)
		if (file_read(fh, &thd, sizeof(thd)) != sizeof(thd)) {
			goto sxsiope_err2;
		}
		headersize = 256;
		surfaces = 8;
		cylinders = LOADINTELWORD(thd.cylinders);
		sectors = 33;
		size = 256;
		totals = cylinders * sectors * surfaces;
	} else if ((!file_cmpname(ext, str_nhd)) && (!(drv & 0x20))) {
		NHDHDR nhd; // T98Next HDD (IDE)
		if ((file_read(fh, &nhd, sizeof(nhd)) != sizeof(nhd)) || (memcmp(nhd.sig, sig_nhd, 15))) {
			goto sxsiope_err2;
		}
		headersize = LOADINTELDWORD(nhd.headersize);
		surfaces = LOADINTELWORD(nhd.surfaces);
		cylinders = LOADINTELDWORD(nhd.cylinders);
		sectors = LOADINTELWORD(nhd.sectors);
		size = LOADINTELWORD(nhd.sectorsize);
		totals = cylinders * sectors * surfaces;
	} else if ((!file_cmpname(ext, str_hdi) || !file_cmpname(ext, str_hdd)) && (!(drv & 0x20))) {
		HDIHDR hdi; // ANEX86 HDD (SASI) thanx Mamiya
		if (file_read(fh, &hdi, sizeof(hdi)) != sizeof(hdi)) {
			goto sxsiope_err2;
		}
		headersize = LOADINTELDWORD(hdi.headersize);
		surfaces = LOADINTELDWORD(hdi.surfaces);
		cylinders = LOADINTELDWORD(hdi.cylinders);
		sectors = LOADINTELDWORD(hdi.sectors);
		size = LOADINTELDWORD(hdi.sectorsize);
		totals = cylinders * sectors * surfaces;
	} else if ((!file_cmpname(ext, str_hdd) || !file_cmpname(ext, str_hdi)) && (drv & 0x20)) {
		VHDHDR vhd; // Virtual98 HDD (SCSI)
		if ((file_read(fh, &vhd, sizeof(vhd)) != sizeof(vhd)) || (memcmp(vhd.sig, sig_vhd, 5))) {
			goto sxsiope_err2;
		}
		headersize = sizeof(vhd);
		surfaces = vhd.surfaces;
		cylinders = LOADINTELWORD(vhd.cylinders);
		sectors = vhd.sectors;
		size = LOADINTELWORD(vhd.sectorsize);
		totals = (SINT32)LOADINTELDWORD(vhd.totals);
	} else {
		goto sxsiope_err2;
	}

	// フォーマット確認～
	if ((surfaces == 0) || (surfaces >= 256) || (cylinders == 0) || (cylinders >= 65536) ||
	    (sectors == 0) || (sectors >= 256) || (size == 0) || ((size & (size - 1)) != 0) ||
	    (totals <= 0)) {
		goto sxsiope_err2;
	}
	if (!(drv & 0x20)) {
		type |= SXSITYPE_IDE;
	} else {
		UINT64 expected_size;
		UINT64 actual_size;

		type |= SXSITYPE_SCSI;
		if (!(size & 0x700)) { // not 256,512,1024
			goto sxsiope_err2;
		}
		expected_size = (UINT64)headersize + ((UINT64)totals * (UINT64)size);
		actual_size = file_getsize64(fh);
		if (actual_size != expected_size) {
			if (actual_size < expected_size) {
				fprintf(
				    stderr,
				    "Error: SCSI image truncated: %s declared_blocks=%ld block_size=%u expected_bytes=%llu actual_bytes=%llu missing_bytes=%llu\n",
				    file, totals, size, (unsigned long long)expected_size,
				    (unsigned long long)actual_size,
				    (unsigned long long)(expected_size - actual_size));
			} else {
				fprintf(
				    stderr,
				    "Error: SCSI image overlong: %s declared_blocks=%ld block_size=%u expected_bytes=%llu actual_bytes=%llu\n",
				    file, totals, size, (unsigned long long)expected_size,
				    (unsigned long long)actual_size);
			}
			goto sxsiope_err2;
		}
	}
	sxsi->totals = totals;
	sxsi->cylinders = (UINT16)cylinders;
	sxsi->size = (UINT16)size;
	sxsi->sectors = (UINT8)sectors;
	sxsi->surfaces = (UINT8)surfaces;
	sxsi->type = type;
	sxsi->headersize = headersize;
	sxsi->fh = fh;
	file_cpyname(sxsi->fname, file, sizeof(sxsi->fname));
	if (type == (SXSITYPE_IDE | SXSITYPE_HDD)) {
		sasihddcheck(sxsi);
	}
	if (log_mount) {
		const char *interface_name;
		UINT16 interface_type;
		BOOL read_only;

		interface_type = sxsi->type & SXSITYPE_IFMASK;
		if (interface_type == SXSITYPE_SASI) {
			interface_name = "SASI";
		} else if (interface_type == SXSITYPE_SCSI) {
			interface_name = "SCSI";
		} else if (interface_type == SXSITYPE_IDE) {
			interface_name = "IDE";
		} else {
			interface_name = "HDD";
		}
		read_only = ((sxsi->type & SXSITYPE_DEVMASK) == SXSITYPE_CDROM);
		fprintf(stderr,
		        "INFO: %s mount id=%u path=%s size=%llu header=%u "
		        "block_size=%u blocks=%ld read_only=%u fingerprint=%08x\n",
		        interface_name, (unsigned int)(drv & 0x0f), file,
		        (unsigned long long)file_getsize64(fh), headersize, size, totals,
		        read_only ? 1U : 0U, sxsi_file_fingerprint(fh, headersize, size));
	}
	return (SUCCESS);

sxsiope_err2:
	file_close(fh);

sxsiope_err1:
	return (FAILURE);
}

BOOL sxsi_hddopen(REG8 drv, const char *file) {
	return (sxsi_hddopen_device(drv, file, sxsi_getptr(drv), TRUE));
}

BOOL sxsi_hddvalidate_sasi(const char *file) {
	_SXSIDEV candidate;
	UINT64 expected_size;
	BOOL result;

	ZeroMemory(&candidate, sizeof(candidate));
	candidate.fh = FILEH_INVALID;
	if (sxsi_hddopen_device(0, file, &candidate, FALSE) != SUCCESS) {
		return (FAILURE);
	}
	expected_size = (UINT64)candidate.headersize + ((UINT64)candidate.totals * candidate.size);
	result = (((candidate.type & SXSITYPE_IFMASK) == SXSITYPE_SASI) && (candidate.totals > 0) &&
	          ((UINT64)file_getsize(candidate.fh) >= expected_size))
	             ? SUCCESS
	             : FAILURE;
	if (candidate.fh != FILEH_INVALID) {
		file_close(candidate.fh);
	}
	return (result);
}

BOOL sxsi_hddvalidate_scsi(const char *file) {
	_SXSIDEV candidate;
	UINT64 expected_size;
	BOOL result;

	ZeroMemory(&candidate, sizeof(candidate));
	candidate.fh = FILEH_INVALID;
	if (sxsi_hddopen_device(0x20, file, &candidate, FALSE) != SUCCESS) {
		return (FAILURE);
	}
	expected_size = (UINT64)candidate.headersize + ((UINT64)candidate.totals * candidate.size);
	result = (((candidate.type & SXSITYPE_IFMASK) == SXSITYPE_SCSI) && (candidate.totals > 0) &&
	          (file_getsize64(candidate.fh) == expected_size))
	             ? SUCCESS
	             : FAILURE;
	if (candidate.fh != FILEH_INVALID) {
		file_close(candidate.fh);
	}
	return (result);
}

void sxsi_open(void) {
	int i;
	REG8 drv;

	sxsi_trash();
	drv = 0;
	for (i = 0; i < 2; i++) {
		if (sxsi_hddopen(drv, np2cfg.sasihdd[i]) == SUCCESS) {
			drv++;
		}
	}
	drv = 0x20;
	for (i = 0; i < SCSIHDD_MAX; i++) {
		sxsi_hddopen((REG8)(0x20 + i), np2cfg.scsihdd[i]);
	}
}

void sxsi_flash(void) {
	SXSIDEV sxsi;
	SXSIDEV sxsiterm;

	sxsi = sxsi_dev;
	sxsiterm = sxsi + (sizeof(sxsi_dev) / sizeof(_SXSIDEV));
	while (sxsi < sxsiterm) {
		if (sxsi->fh != FILEH_INVALID) {
			file_close(sxsi->fh);
			sxsi->fh = FILEH_INVALID;
		}
		sxsi++;
	}
}

void sxsi_trash(void) {
	SXSIDEV sxsi;
	SXSIDEV sxsiterm;

	sxsi = sxsi_dev;
	sxsiterm = sxsi + (sizeof(sxsi_dev) / sizeof(_SXSIDEV));
	while (sxsi < sxsiterm) {
		if (sxsi->fh != FILEH_INVALID) {
			file_close(sxsi->fh);
		}
		ZeroMemory(sxsi, sizeof(_SXSIDEV));
		sxsi->fh = FILEH_INVALID;
		sxsi++;
	}
}

static SXSIDEV getdrive(REG8 drv) {
	SXSIDEV ret;

	ret = sxsi_getptr(drv);
	if ((ret == NULL) || (ret->fname[0] == '\0')) {
		return (NULL);
	}
	if (ret->fh == FILEH_INVALID) {
		ret->fh = file_open(ret->fname);
		if (ret->fh == FILEH_INVALID) {
			ret->fname[0] = '\0';
			return (NULL);
		}
	}
	sysmng_hddaccess(drv);
	return (ret);
}

BOOL sxsi_issasi(void) {
	REG8 drv;
	SXSIDEV sxsi;
	BOOL ret;
	UINT sxsiif;

	ret = FALSE;
	for (drv = 0x00; drv < 0x04; drv++) {
		sxsi = sxsi_getptr(drv);
		if (sxsi) {
			sxsiif = sxsi->type & SXSITYPE_IFMASK;
			if (sxsiif == SXSITYPE_SASI) {
				ret = TRUE;
			} else if (sxsiif == SXSITYPE_IDE) {
				ret = FALSE;
				break;
			}
		}
	}
	return (ret);
}

BOOL sxsi_isscsi(void) {
	REG8 drv;
	SXSIDEV sxsi;

	for (drv = 0x20; drv < 0x28; drv++) {
		sxsi = sxsi_getptr(drv);
		if ((sxsi) && (sxsi->type)) {
			return (TRUE);
		}
	}
	return (FALSE);
}

BOOL sxsi_iside(void) {
	REG8 drv;
	SXSIDEV sxsi;

	for (drv = 0x00; drv < 0x04; drv++) {
		sxsi = sxsi_getptr(drv);
		if ((sxsi) && (sxsi->type)) {
			return (TRUE);
		}
	}
	return (FALSE);
}

REG8 sxsi_read(REG8 drv, long pos, BYTE *buf, UINT size) {
	const _SXSIDEV *sxsi;
	UINT64 blocks;
	UINT64 offset;
	long r;
	UINT rsize;

	sxsi = getdrive(drv);
	if (sxsi == NULL) {
		return (0x60);
	}
	if ((pos < 0) || (sxsi->size == 0) || (size % sxsi->size) != 0) {
		return (0x40);
	}
	blocks = size / sxsi->size;
	if (((UINT64)pos >= (UINT64)sxsi->totals) || (blocks > ((UINT64)sxsi->totals - (UINT64)pos))) {
		return (0x40);
	}
	offset = (UINT64)sxsi->headersize + ((UINT64)pos * (UINT64)sxsi->size);
	if (offset > (UINT64)LONG_MAX) {
		return (0xd0);
	}
	r = file_seek(sxsi->fh, (long)offset, FSEEK_SET);
	if ((UINT64)r != offset) {
		return (0xd0);
	}
	while (size) {
		rsize = min(size, sxsi->size);
		CPU_REMCLOCK -= rsize;
		if (file_read(sxsi->fh, buf, rsize) != rsize) {
			return (0xd0);
		}
		buf += rsize;
		size -= rsize;
	}
	return (0x00);
}

REG8 sxsi_write(REG8 drv, long pos, const BYTE *buf, UINT size) {
	const _SXSIDEV *sxsi;
	UINT64 blocks;
	UINT64 offset;
	long r;
	UINT wsize;

	sxsi = getdrive(drv);
	if (sxsi == NULL) {
		return (0x60);
	}
	if ((pos < 0) || (sxsi->size == 0) || (size % sxsi->size) != 0) {
		return (0x40);
	}
	blocks = size / sxsi->size;
	if (((UINT64)pos >= (UINT64)sxsi->totals) || (blocks > ((UINT64)sxsi->totals - (UINT64)pos))) {
		return (0x40);
	}
	offset = (UINT64)sxsi->headersize + ((UINT64)pos * (UINT64)sxsi->size);
	if (offset > (UINT64)LONG_MAX) {
		return (0xd0);
	}
	r = file_seek(sxsi->fh, (long)offset, FSEEK_SET);
	if ((UINT64)r != offset) {
		return (0xd0);
	}
	while (size) {
		wsize = min(size, sxsi->size);
		CPU_REMCLOCK -= wsize;
		if (file_write(sxsi->fh, buf, wsize) != wsize) {
			return (0x70);
		}
		buf += wsize;
		size -= wsize;
	}
	if (file_flush(sxsi->fh) != 0) {
		return (0x70);
	}
	return (0x00);
}

REG8 sxsi_format(REG8 drv, long pos) {
	const _SXSIDEV *sxsi;
	long r;
	UINT16 i;
	BYTE work[256];
	UINT size;
	UINT wsize;

	sxsi = getdrive(drv);
	if (sxsi == NULL) {
		return (0x60);
	}
	if ((pos < 0) || (sxsi->size == 0) || ((UINT64)pos >= (UINT64)sxsi->totals) ||
	    ((UINT64)sxsi->sectors > ((UINT64)sxsi->totals - (UINT64)pos))) {
		return (0x40);
	}
	if (((UINT64)sxsi->headersize + ((UINT64)pos * (UINT64)sxsi->size)) > (UINT64)LONG_MAX) {
		return (0xd0);
	}
	pos = pos * sxsi->size + sxsi->headersize;
	r = file_seek(sxsi->fh, pos, FSEEK_SET);
	if (pos != r) {
		return (0xd0);
	}
	FillMemory(work, sizeof(work), 0xe5);
	for (i = 0; i < sxsi->sectors; i++) {
		size = sxsi->size;
		while (size) {
			wsize = min(size, sizeof(work));
			size -= wsize;
			CPU_REMCLOCK -= wsize;
			if (file_write(sxsi->fh, work, wsize) != wsize) {
				return (0x70);
			}
		}
	}
	if (file_flush(sxsi->fh) != 0) {
		return (0x70);
	}
	return (0x00);
}

int sxsi_image_selftest(void) {
	_SXSIDEV saved_slots[SCSIHDD_MAX];
	SXSIDEV slot;
	BYTE pattern[256];
	BYTE readback[256];
	BYTE zeroes[256];
	FILEH fh;
	UINT64 expected_size;
	UINT id;
	UINT i;
	long saved_remclock;
	BOOL ok = TRUE;
	const char *path = "m75-sxsi-image-selftest.hdd";

	for (id = 0; id < SCSIHDD_MAX; id++) {
		slot = sxsi_getptr((REG8)(0x20 + id));
		saved_slots[id] = *slot;
	}
	if (file_attr(path) != (short)-1) {
		return (FAILURE);
	}
	saved_remclock = CPU_REMCLOCK;
	if (newdisk_vhd_create(path, 163840, 256, FALSE) != SUCCESS) {
		ok = FALSE;
		goto image_selftest_cleanup;
	}
	expected_size = (UINT64)sizeof(VHDHDR) + 163840ULL * 256ULL;
	fh = file_open_rb(path);
	ok = (fh != FILEH_INVALID) && (file_getsize64(fh) == expected_size);
	if (fh != FILEH_INVALID) {
		file_close(fh);
	}
	if (!ok || (sxsi_hddvalidate_scsi(path) != SUCCESS)) {
		goto image_selftest_cleanup;
	}
	if (newdisk_vhd_create(path, 163840, 256, FALSE) == SUCCESS) {
		ok = FALSE;
		goto image_selftest_cleanup;
	}
	slot = sxsi_getptr(0x20);
	ZeroMemory(slot, sizeof(*slot));
	slot->fh = FILEH_INVALID;
	if (sxsi_hddopen(0x20, path) != SUCCESS) {
		ok = FALSE;
		goto image_selftest_cleanup;
	}
	ZeroMemory(zeroes, sizeof(zeroes));
	for (i = 1; i < 3; i++) {
		if ((sxsi_read(0x20, (long)(i * 1000), readback, sizeof(readback)) != 0) ||
		    (memcmp(readback, zeroes, sizeof(readback)) != 0)) {
			ok = FALSE;
		}
	}
	if (sxsi_read(0x20, 163840 - 1, readback, sizeof(readback)) != 0 ||
	    memcmp(readback, zeroes, sizeof(readback)) != 0) {
		ok = FALSE;
	}
	for (i = 0; i < sizeof(pattern); i++) {
		pattern[i] = (BYTE)(i ^ 0x5a);
	}
	if ((sxsi_write(0x20, 0, pattern, sizeof(pattern)) != 0) ||
	    (sxsi_read(0x20, 0, readback, sizeof(readback)) != 0) ||
	    (memcmp(pattern, readback, sizeof(pattern)) != 0) ||
	    (sxsi_write(0x20, 163840 - 1, pattern, sizeof(pattern)) != 0) ||
	    (sxsi_read(0x20, 163840 - 1, readback, sizeof(readback)) != 0) ||
	    (memcmp(pattern, readback, sizeof(pattern)) != 0) ||
	    (sxsi_read(0x20, 163840, readback, sizeof(readback)) != 0x40) ||
	    (sxsi_write(0x20, 163840, pattern, sizeof(pattern)) != 0x40) ||
	    (sxsi_read(0x20, 163839, readback, 255) != 0x40) ||
	    (file_getsize64(slot->fh) != expected_size)) {
		ok = FALSE;
	}
	file_close(slot->fh);
	slot->fh = FILEH_INVALID;
	if (sxsi_hddopen(0x20, path) != SUCCESS ||
	    sxsi_read(0x20, 0, readback, sizeof(readback)) != 0 ||
	    memcmp(pattern, readback, sizeof(pattern)) != 0 ||
	    sxsi_read(0x20, 163839, readback, sizeof(readback)) != 0 ||
	    memcmp(pattern, readback, sizeof(pattern)) != 0) {
		ok = FALSE;
	}
	if (slot->fh != FILEH_INVALID) {
		file_close(slot->fh);
		slot->fh = FILEH_INVALID;
	}
	{
		const char *truncated = "m75-sxsi-image-selftest-truncated.hdd";
		if (file_attr(truncated) == (short)-1) {
			if (newdisk_vhd_create(truncated, 163840, 256, FALSE) == SUCCESS) {
				fh = file_open(truncated);
				if ((fh == FILEH_INVALID) || (file_setsize(fh, expected_size - 1) != 0)) {
					ok = FALSE;
				}
				if (fh != FILEH_INVALID) {
					file_close(fh);
				}
				if (sxsi_hddvalidate_scsi(truncated) == SUCCESS) {
					ok = FALSE;
				}
				file_delete(truncated);
			} else {
				ok = FALSE;
			}
		}
	}

image_selftest_cleanup:
	for (id = 0; id < SCSIHDD_MAX; id++) {
		slot = sxsi_getptr((REG8)(0x20 + id));
		if (slot->fh != FILEH_INVALID && slot->fh != saved_slots[id].fh) {
			file_close(slot->fh);
		}
		*slot = saved_slots[id];
	}
	file_delete(path);
	CPU_REMCLOCK = saved_remclock;
	if (ok) {
		fprintf(stderr, "selftest: SCSI image creation/backing validation ok\n");
	}
	return (ok ? SUCCESS : FAILURE);
}
