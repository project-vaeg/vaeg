#ifndef VAEG_MEMORYVA_MEMORYVA_H
#define VAEG_MEMORYVA_MEMORYVA_H

#ifndef MEMCALL
#define MEMCALL
#endif

/*
 * Runtime state for the native VA banked-memory decoder. Bank registers are
 * restored from save states before upd9002_memorymap_va() rebuilds optional
 * VA91 entries.
 */
typedef struct {
	UINT8 sysm_bank;
	UINT8 rom0_bank;
	UINT8 rom1_bank;
	UINT8 dma_sysm_bank;
	UINT8 dma_access;
	UINT8 backupmem_wp;
	UINT8 dmy0;
	UINT8 dmy1;

	UINT32 rom0exist;
	UINT32 rom1exist;
	UINT32 sysmromexist;
} _MEMORYVA;

#ifdef __cplusplus
extern "C" {
#endif

extern BYTE textmem[0x40000];
extern BYTE fontmem[0x50000];
extern BYTE backupmem[0x04000];
extern BYTE dicmem[0x80000];
extern BYTE rom0mem[0xa0000];
extern BYTE rom1mem[0x20000];

extern _MEMORYVA memoryva;
/* Independent of CPU execution mode: zero = V3, one = 88-mode request. */
extern UINT8 memoryva_88_mode;
extern UINT8 memoryva_88_port31;
extern UINT8 memoryva_88_xerom;  /* 71h bit 0: one disables extension ROM. */
extern UINT8 memoryva_88_plane;  /* 0..2 selected, 3 disables GVRAM access. */

/*
 * V1/V2-mode GVRAM extended access (BNN manual and tekumani, ports 32h,
 * 34h, 35h). Port 32h bit 6 (GVAM) selects it; port 35h bit 7 (GAM) maps
 * GVRAM at C000h-FFFFh, bits 5-4 (GDM) select the write data and bits 2-0
 * the read comparison colour; port 34h holds a two-bit ALU operation per
 * plane. Reads latch all three planes.
 */
typedef struct {
	UINT8 gvam;     /* port 32h bit 6 */
	UINT8 port034;  /* last value written to port 34h */
	UINT8 port035;  /* last value written to port 35h */
	UINT8 latch[3]; /* planes 0-2 latched by the last read */
	UINT8 padding[2];
} _MEMORYVA88ALU;

extern _MEMORYVA88ALU memoryva_88_alu;

/*
 * V1/V2 extended RAM (PC-8801-02N compatible). E2h: bit 0 RE, bit 4 WE;
 * E3h: bits 3-2 page, bits 1-0 bank. Page p, bank b appears at 0000h-7FFFh
 * from VA main RAM 20000h + p * 20000h + b * 8000h.
 */
typedef struct {
	UINT8 mode; /* E2h */
	UINT8 bank; /* E3h */
} _MEMORYVA88ERAM;

extern _MEMORYVA88ERAM memoryva_88_eram;

/* Optional user-supplied N-BASIC ROM (vaeg extension, bios/romva.c). */
extern BYTE memoryva_n80[0x8000];
extern BOOL memoryva_n80_exist;
extern char memoryva_n80_file[32]; /* file loaded, empty without a ROM */

/*
 * V1/V2 dictionary ROM window (PC-8801MA style, not in the VA manuals).
 * F0h selects a 16 KiB bank of the VA dictionary ROM; F1h bit 0 clear maps
 * it for reads at C000h-FFFFh.
 */
typedef struct {
	UINT8 bank;   /* F0h */
	UINT8 enable; /* F1h, bit 0 clear maps the window */
} _MEMORYVA88DIC;

extern _MEMORYVA88DIC memoryva_88_dic;
extern UINT8 memoryva_88_window; /* 70h: high byte of the 1KiB RAM-window origin. */
extern BOOL textmem_dirty;

void MEMCALL upd9002_memorymap_va(void);
void MEMCALL upd9002_memorywrite_va(UINT32 address, REG8 value);
void MEMCALL upd9002_memorywrite_va_w(UINT32 address, REG16 value);
REG8 MEMCALL upd9002_memoryread_va(UINT32 address);
REG16 MEMCALL upd9002_memoryread_va_w(UINT32 address);

#ifdef __cplusplus
}
#endif

#endif
