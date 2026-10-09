
#ifdef __cplusplus
extern "C" {
#endif

void newdisk_fdd(const char *fname, REG8 type, const char *label);

enum {
	NEWDISK_FDD_MSDOS_2HD = 0,
	NEWDISK_FDD_MSDOS_2DD,
	/* Physical layout only (sectors filled with E5h, no file system), D88. */
	NEWDISK_FDD_BLANK_2DD_720,
	NEWDISK_FDD_BLANK_2D_320,
	NEWDISK_FDD_BLANK_2D_360,
	NEWDISK_FDD_MSDOS_COUNT
};

enum {
	NEWDISK_FDD_CONTAINER_D88 = 0,
	NEWDISK_FDD_CONTAINER_RAW,
	NEWDISK_FDD_CONTAINER_COUNT
};

BOOL newdisk_fdd_msdos(const char *fname, UINT format);
BOOL newdisk_fdd_has_filesystem(UINT format);
BOOL newdisk_fdd_msdos_ex(const char *fname, UINT format, UINT container);

void newdisk_thd(const char *fname, UINT hddsize);
void newdisk_nhd(const char *fname, UINT hddsize);
void newdisk_hdi(const char *fname, UINT hddtype);
BOOL newdisk_vhd_create(const char *fname, UINT64 block_count, UINT16 block_size, BOOL force);
void newdisk_vhd(const char *fname, UINT hddsize);

#ifdef __cplusplus
}
#endif
