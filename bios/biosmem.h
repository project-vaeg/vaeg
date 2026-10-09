/*
 * Emulator-owned guest work-area offsets still consumed by active VA paths.
 * The former simulated-PC-98 bootstrap offsets were removed in M96e.
 */

enum {
	/* Emulator policy: SASI/SCSI service state is exposed in guest memory. */
	MEMB_DISK_EQUIPS = 0x00482,
	MEMW_DISK_EQUIP = 0x0055c,

	/* Emulator policy: synchronize the configured VA memory switches. */
	MEMX_MSW = 0xa3fe2
};

#if defined(BYTESEX_LITTLE)

#define GETBIOSMEM16(a) (*(UINT16 *)(mem + (a)))
#define SETBIOSMEM16(a, b) *(UINT16 *)(mem + (a)) = (b)

#define GETBIOSMEM32(a) (*(UINT32 *)(mem + (a)))
#define SETBIOSMEM32(a, b) *(UINT32 *)(mem + (a)) = (b)

#elif defined(BYTESEX_BIG)

#define GETBIOSMEM16(a) ((UINT16)(mem[(a) + 0] + (mem[(a) + 1] << 8)))
#define SETBIOSMEM16(a, b)                                                                         \
	mem[(a) + 0] = (BYTE)(b);                                                                      \
	mem[(a) + 1] = (BYTE)((b) >> 8)

#define GETBIOSMEM32(a)                                                                            \
	((UINT32)(mem[(a) + 0] + (mem[(a) + 1] << 8) + (mem[(a) + 2] << 16) + (mem[(a) + 3] << 24)))
#define SETBIOSMEM32(a, b)                                                                         \
	mem[(a) + 0] = (BYTE)(b);                                                                      \
	mem[(a) + 1] = (BYTE)((b) >> 8);                                                               \
	mem[(a) + 2] = (BYTE)((b) >> 16);                                                              \
	mem[(a) + 3] = (BYTE)((b) >> 24)

#endif
