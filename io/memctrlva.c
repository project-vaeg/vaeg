/*
 * memctrlva.c: PC-88VA memory control/memory switch
 */

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "memoryva.h"
#include "sgp.h"
#include "va91.h"
#include "cpu/upd9002/memory.h"

// ---- I/O

static void IOOUTCALL memctrlva_o05c(UINT port, REG8 dat) {
	memoryva_88_plane = (UINT8)(port - 0x5c);
	(void)dat;
}

static REG8 IOINPCALL memctrlva_i05c(UINT port) {
	(void)port;
	return 0xf8 | (memoryva_88_plane < 3 ? (1 << memoryva_88_plane) : 0);
}

static void IOOUTCALL memctrlva_o034(UINT port, REG8 dat) {
	memoryva_88_alu.port034 = (UINT8)dat;
	(void)port;
}

static void IOOUTCALL memctrlva_o035(UINT port, REG8 dat) {
	memoryva_88_alu.port035 = (UINT8)dat;
	(void)port;
}

/*
 * The manuals list 34h/35h as output ports, but the VA ROM's text trap
 * handler saves them with IN and restores them with OUT around its V3-mode
 * work, so they must read back the last value written.
 */
static REG8 IOINPCALL memctrlva_i034(UINT port) {
	(void)port;
	return memoryva_88_alu.port034;
}

static REG8 IOINPCALL memctrlva_i035(UINT port) {
	(void)port;
	return memoryva_88_alu.port035;
}

/*
 * Extended RAM. The BNN manual and the technical manual print the page/bank
 * layout under E2h and WE/RE under E3h; PC-8801 software (X88000, and a
 * PC-8801MA2 loader traced here: E3h = 03h, then E2h = 11h) writes the bank
 * to E3h and RE/WE to E2h, so the port numbers follow the PC-8801. The read
 * of the RE/WE port is inverted, as both sources state.
 */
static void IOOUTCALL memctrlva_o0e2(UINT port, REG8 dat) {
	memoryva_88_eram.mode = (UINT8)(dat & 0x11);
	(void)port;
}

static REG8 IOINPCALL memctrlva_i0e2(UINT port) {
	(void)port;
	return (REG8)(~memoryva_88_eram.mode);
}

static void IOOUTCALL memctrlva_o0e3(UINT port, REG8 dat) {
	memoryva_88_eram.bank = (UINT8)(dat & 0x0f);
	(void)port;
}

static REG8 IOINPCALL memctrlva_i0e3(UINT port) {
	(void)port;
	return (REG8)(0xf0 | memoryva_88_eram.bank);
}

/*
 * Dictionary ROM window. Not documented for the VA; a PC-8801MA2 loader
 * selects bank 0 with F0h, maps it with F1h = 00h and checks for 44h 10h at
 * C000h, which is how the VA2 dictionary ROM image begins.
 */
static void IOOUTCALL memctrlva_o0f0(UINT port, REG8 dat) {
	memoryva_88_dic.bank = (UINT8)(dat & 0x1f);
	(void)port;
}

static void IOOUTCALL memctrlva_o0f1(UINT port, REG8 dat) {
	memoryva_88_dic.enable = (UINT8)(dat & 0x01);
	(void)port;
}

static void IOOUTCALL memctrlva_o070(UINT port, REG8 dat) {
	memoryva_88_window = (UINT8)dat;
	(void)port;
}

static REG8 IOINPCALL memctrlva_i070(UINT port) {
	(void)port;
	return memoryva_88_window;
}

static void IOOUTCALL memctrlva_o078(UINT port, REG8 dat) {
	memoryva_88_window++;
	(void)port;
	(void)dat;
}

static void IOOUTCALL memctrlva_o071(UINT port, REG8 dat) {
	memoryva_88_xerom = dat & 1;
	(void)port;
}

static REG8 IOINPCALL memctrlva_i071(UINT port) {
	(void)port;
	return 0xfe | memoryva_88_xerom;
}

static void IOOUTCALL memctrlva_o031(UINT port, REG8 dat) {
	/* Retain all bits; only MMODE/RMODE affect the initial ROM overlay. */
	memoryva_88_port31 = (UINT8)dat;
	if (memoryva_88_mode) {
		/*
		 * BNN manual: port 31h bits PM00, GDEN0 and VW1 carry the names of
		 * GrRes (102h) bit 0 and GrMode (100h) bits 15 and 1, and BASIC
		 * writes them directly; treat them as aliases of those VA bits.
		 */
		videova.grmode = (WORD)((videova.grmode & ~0x8002) | ((dat & 0x08) ? 0x8000 : 0) |
		                        ((dat & 0x01) ? 0x0002 : 0));
		videova.grres = (WORD)((videova.grres & ~0x0001) | ((dat & 0x10) ? 0x0001 : 0));
	}
	(void)port;
}

static void IOOUTCALL memctrlva_o152(UINT port, REG8 dat) {
	if (pccore.model_va == PCMODEL_VA1) {
		memoryva.rom0_bank = (dat & 0x0f);
		memoryva.rom1_bank = ((dat & 0xf0) >> 4);
	} else {
		memoryva.rom0_bank = ((dat & 0x40) >> 2) | (dat & 0x0f);
		memoryva.rom1_bank = ((dat & 0xb0) >> 4);
	}
	(void)port;
}

static void IOOUTCALL memctrlva_o153(UINT port, REG8 dat) {
	if ((dat & 0x0f) == 0x0f)
		TRACEOUT(("memctrlva: out %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	memoryva.sysm_bank = dat & 0x0f;
	memoryva_88_mode = (dat & 0x40) ? 0 : 1;
	fdc_trace_text("banktrace port=%03x val=%02x sysm_bank=%02x", port, dat, memoryva.sysm_bank);
	if ((dat ^ gactrlva.gmsp) & 0x10) {
		// Reset access state when GMSP changes between multi- and single-plane modes.
		gactrlva_reset();
		if (dat & 0x10) {
			sgp_reset();
		}
	}
	gactrlva.gmsp = dat & 0x10;
	(void)port;
}

static REG8 IOINPCALL memctrlva_i152(UINT port) {
	(void)port;
	if (pccore.model_va == PCMODEL_VA1) {
		return (memoryva.rom0_bank & 0x0f) | ((memoryva.rom1_bank & 0x0f) << 4);
	} else {
		return (memoryva.rom0_bank & 0x0f) | ((memoryva.rom0_bank & 0x10) << 2) |
		       ((memoryva.rom1_bank & 0x0b) << 4);
	}
}

static REG8 IOINPCALL memctrlva_i153(UINT port) {
	(void)port;
	return (memoryva.sysm_bank & 0x0f) | gactrlva.gmsp | (memoryva_88_mode ? 0 : 0x40);
}

static REG8 IOINPCALL memctrlva_i156(UINT port) {
	// Return the VA91 ROM-bank presence status on the shared bank-status port.
	REG8 dat = 0xff;
	dat = ~(~dat | ~va91_rombankstatus());
	return dat;
}

static void IOOUTCALL memctrlva_o180(UINT port, REG8 dat) {
	memoryva.dma_sysm_bank = dat & 0x8f;
}

static REG8 IOINPCALL memctrlva_i180(UINT port) {
	return memoryva.dma_sysm_bank;
}

static void IOOUTCALL memctrlva_o198(UINT port, REG8 dat) {
	memoryva.backupmem_wp = 1;
	(void)port;
}

static void IOOUTCALL memctrlva_o19a(UINT port, REG8 dat) {
	memoryva.backupmem_wp = 0;
	(void)port;
}

static REG8 IOINPCALL memctrlva_i030(UINT port) {
	return backupmem[0x1fc2];
}

/*
 * IN 31h reads memory switch B1FC6h. Bit 6 (MS26) is unused and always 1 on
 * the VA; on the PC-8801 it reports high speed (H). As a vaeg extension,
 * V1/V2 mode can report standard speed (S, bit 6 clear), which N-88 BASIC's
 * NEW ON 1 requires (with V1). CPU timing is not changed.
 */
static REG8 IOINPCALL memctrlva_i031(UINT port) {
	if (memoryva_88_mode && np2cfg.v1v2_standard) {
		return (REG8)(backupmem[0x1fc6] & ~0x40);
	}
	return backupmem[0x1fc6];
}

/*
 * N mode (vaeg extension; see docs/modernization/v1v2-n-basic-mode.md).
 * With a user-supplied N-BASIC ROM and the N setting, the machine starts as
 * a PC-8801 in N mode: RMODE is set before the VA ROM hands off to
 * compatible code at 0000h, port 40h bit 3 lets the ROM enter V1/V2 without
 * a disk, and at the first compatible-mode entry the VA ROM's emulated port
 * 53h is put into the PC-8801 reset state (text on).
 */
static BOOL nmode_entry_pending;

BOOL memctrlva_nmode_active(void) {
	return (np2cfg.v1v2_nmode && memoryva_n80_exist) ? TRUE : FALSE;
}

/*
 * TRUE from reset until the first compatible-mode entry: the VA ROM's boot
 * search, which port 40h bit 3 must skip. N-BASIC reads the same bit as
 * "no disk unit" (00CBh) and then skips its disk boot, so it must read 0.
 */
BOOL memctrlva_nmode_boot_search(void) {
	return nmode_entry_pending;
}

/* Called after the ROMs are loaded (romva_initialize) at every reset. */
void memctrlva_nmode_reset(void) {
	nmode_entry_pending = memctrlva_nmode_active();
	if (nmode_entry_pending) {
		memoryva_88_port31 = 0x04; /* RMODE: N-BASIC at 0000h-7FFFh */
	}
}

/*
 * Both VA ROMs start V1/V2 with port 148h bit 7 set (text off), keep that
 * value at 0040:00E8h, and clear bit 7 only when compatible code writes
 * port 53h with bit 0 clear (VA2 F000:1D9D, VA F000:1459). N-88 BASIC does;
 * N-BASIC, written for the PC-8001, never writes 53h, and a PC-8801 resets
 * 53h to 0. Clear bit 7 in both places once, as that reset would.
 */
void memctrlva_nmode_compat_entry(void) {
	if (!nmode_entry_pending) {
		return;
	}
	nmode_entry_pending = FALSE;
	upd9002_mainram_write(0x4e8, (REG8)(upd9002_mainram_read(0x4e8) & 0x7f));
	iocore_out8(0x148, (REG8)(videova.txtmode & 0x7f));
}

// ---- I/F

void memctrlva_reset(void) {
	memoryva_88_port31 = 0;
	memoryva_88_xerom = 1;
	memoryva_88_window = 0x80;
	memoryva_88_plane = 3;
	ZeroMemory(&memoryva_88_alu, sizeof(memoryva_88_alu));
	ZeroMemory(&memoryva_88_eram, sizeof(memoryva_88_eram));
	memoryva_88_dic.bank = 0;
	memoryva_88_dic.enable = 1;
	memctrlva_o152(0, 0);
	memctrlva_o153(0, 0x41);
	memctrlva_o198(0, 0);
}

void memctrlva_bind(void) {
	iocore_attachout(0x031, memctrlva_o031);
	iocore_attachout(0x05c, memctrlva_o05c);
	iocore_attachout(0x05d, memctrlva_o05c);
	iocore_attachout(0x05e, memctrlva_o05c);
	iocore_attachout(0x05f, memctrlva_o05c);
	iocore_attachinp(0x05c, memctrlva_i05c);
	iocore_attachout(0x034, memctrlva_o034);
	iocore_attachout(0x035, memctrlva_o035);
	iocore_attachinp(0x034, memctrlva_i034);
	iocore_attachinp(0x035, memctrlva_i035);
	iocore_attachout(0x0f0, memctrlva_o0f0);
	iocore_attachout(0x0f1, memctrlva_o0f1);
	iocore_attachout(0x0e2, memctrlva_o0e2);
	iocore_attachinp(0x0e2, memctrlva_i0e2);
	iocore_attachout(0x0e3, memctrlva_o0e3);
	iocore_attachinp(0x0e3, memctrlva_i0e3);
	iocore_attachout(0x070, memctrlva_o070);
	iocore_attachinp(0x070, memctrlva_i070);
	iocore_attachout(0x078, memctrlva_o078);
	iocore_attachout(0x071, memctrlva_o071);
	iocore_attachinp(0x071, memctrlva_i071);
	iocore_attachout(0x152, memctrlva_o152);
	iocore_attachout(0x153, memctrlva_o153);
	iocore_attachout(0x180, memctrlva_o180);
	iocore_attachout(0x198, memctrlva_o198);
	iocore_attachout(0x19a, memctrlva_o19a);

	iocore_attachinp(0x152, memctrlva_i152);
	iocore_attachinp(0x153, memctrlva_i153);
	iocore_attachinp(0x156, memctrlva_i156);
	iocore_attachinp(0x180, memctrlva_i180);

	iocore_attachinp(0x030, memctrlva_i030);
	iocore_attachinp(0x031, memctrlva_i031);
}
