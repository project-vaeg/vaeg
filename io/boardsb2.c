/*
 *	boardsb2.c: PC-88VA Sound board 2
 */

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "cbuscore.h"
#include "boardsb2.h"
#include "sound.h"
#include "fmboard.h"

_BOARDSB2 boardsb2;

//static unsigned int wait = 160;		// 20 us; retained for timing comparison.
////static unsigned int wait = 0;			//

// ---- wait

static void startwait_addrwrite(void) {
	if (boardsb2.waitenabled) {
		boardsb2.lastaccess = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
		boardsb2.wait = boardsb2.addrwritewait;
	}
}

static void startwait_datawrite(void) {
	if (boardsb2.waitenabled) {
		boardsb2.lastaccess = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
		boardsb2.wait = boardsb2.datawritewait;
	}
}

static void checkandwait(void) {
	UINT32 current;
	UINT32 past;

	if (boardsb2.wait) {
		current = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
		past = current - boardsb2.lastaccess;
		if (past < boardsb2.wait) {
			// Charge the remaining programmed access wait.
			CPU_REMCLOCK -= boardsb2.wait - past;
		}
		boardsb2.wait = 0;
	}
}

// ---- I/O

static void IOOUTCALL sb2_o044(UINT port, REG8 dat) {
	//	CPU_REMCLOCK -= wait;

	opn.opnreg = dat;
	//TRACEOUT(("sb2: o044 <- %02x %.4x:%.4x", dat, CPU_CS, CPU_IP));

	checkandwait();
	startwait_addrwrite();
}

static void IOOUTCALL sb2_o045(UINT port, REG8 dat) {
	//TRACEOUT(("sb2: o045 (%02x) <- %02x %.4x:%.4x", opn.opnreg, dat, CPU_CS, CPU_IP));
	if (opn.opnreg < 0x10) {
		if (opn.opnreg != 0x0e) {
			psggen_setreg(&psg1, opn.opnreg, dat);
		}
	} else {
		if ((opn.opnreg < 0x20) && (opn.channels > 3)) {
			rhythm_setreg(&rhythm, opn.opnreg, dat);
		} else if ((opn.opnreg < 0x30) && ((opn.channels <= 3) || (opn.opnreg >= 0x20))) {
			if (opn.opnreg == 0x28) {
				if ((dat & 0x0f) < 3) {
					opngen_keyon(dat & 0x0f, dat);
				} else if ((opn.channels > 3) && ((dat & 0x0f) != 3) && ((dat & 0x0f) < 7)) {
					opngen_keyon((dat & 0x0f) - 1, dat);
				}
			} else {
				fmtimer_setreg(opn.opnreg, dat);
				opngen_setcontrol(0, opn.opnreg, dat);
			}
		} else if (opn.opnreg < 0xc0) {
			opngen_setreg(0, opn.opnreg, dat);
		}
		opn.reg[opn.opnreg] = dat;
	}
	(void)port;

	//	CPU_REMCLOCK -= wait;
	checkandwait();
	startwait_datawrite();
}

static void IOOUTCALL sb2_o046(UINT port, REG8 dat) {
	//	CPU_REMCLOCK -= wait;

	opn.extreg = dat;
	(void)port;

	checkandwait();
	startwait_addrwrite();
}

static void IOOUTCALL sb2_o047(UINT port, REG8 dat) {
	//TRACEOUT(("sb2: o047 (%02x) <- %02x %.4x:%.4x", opn.extreg, dat, CPU_CS, CPU_IP));
	opn.reg[opn.extreg + 0x100] = dat;
	if (opn.extreg >= 0x30) {
		opngen_setreg(3, opn.extreg, dat);
	} else if (opn.extreg < 0x12) {
		adpcm_setreg(&adpcm, opn.extreg, dat);
	}
	(void)port;

	//	CPU_REMCLOCK -= wait;
	checkandwait();
	startwait_datawrite();
}

static REG8 IOINPCALL sb2_i044(UINT port) {
	(void)port;
	checkandwait();
	if (opn.channels > 3) {
		return ((fmtimer.status & 3) | adpcm_status(&adpcm));
	}
	return (fmtimer.status & 3);
}

static REG8 IOINPCALL sb2_i045(UINT port) {
	REG8 dat;
	UINT8 data4;
	UINT8 data2;

	if (opn.opnreg == 0x0e) {
		mouseifva_indata(&data4, &data2);
		dat = data4 | 0xf0;
	} else if (opn.opnreg == 0x0f) { // VA-specific joypad button register.
		mouseifva_indata(&data4, &data2);
		dat = data2 | 0xfc;
	} else if (opn.opnreg < 0x10) {
		dat = psggen_getreg(&psg1, opn.opnreg);
	} else {
		dat = opn.reg[opn.opnreg];
	}
	//TRACEOUT(("sb2: i045 (%02x) -> %02x %.4x:%.4x", opn.opnreg, dat, CPU_CS, CPU_IP));
	checkandwait();
	return (dat);
}

static REG8 IOINPCALL sb2_i047(UINT port) {
	if (opn.extreg == 0x08) {
		return (adpcm_readsample(&adpcm));
	}
	(void)port;
	checkandwait();
	return (opn.reg[opn.opnreg]);
}

static void IOOUTCALL sb2_o19c(UINT port, REG8 dat) {
	// Enable OPNA access waits.
	boardsb2.waitenabled = TRUE;
}

static void IOOUTCALL sb2_o19e(UINT port, REG8 dat) {
	// Disable OPNA access waits.
	boardsb2.waitenabled = FALSE;
	boardsb2.wait = 0;
}

// ----

static void boardsb_reset(void) {
	ZeroMemory(&boardsb2, sizeof(boardsb2));
	boardsb2.waitenabled = TRUE;
	boardsb2.addrwritewait = pccore.realclock * 17 / 4000000;
	/* 17/4000000 = 4.25μ */
	boardsb2.datawritewait = pccore.realclock * 83 / 4000000;
	/* 83/4000000 = 20.75μ */

	psggen_setreg(&psg1, 0x07, 0);
	/* VA-specific reset value:
											   psggen_reset(), called by fmboard_reset(),
											   initializes this register to BFH,
											   whereas the VA reset value appears to be zero.
											*/
	fmtimer_reset(0xc0);
}

void boardopnva_reset(void) {
	boardsb_reset();
	opn.channels = 3;
	opngen_setcfg(3, OPN_MONORAL | 0x007);
}

void boardsb2_reset(void) {
	boardsb_reset();
	opn.channels = 6;
	opngen_setcfg(6, OPN_STEREO | 0x03f);
}

static void boardsb_bind(BOOL opna) {
	fmboard_fmrestore(0, 0);
	psggen_restore(&psg1);
	if (opna) {
		fmboard_fmrestore(3, 1);
		fmboard_rhyrestore(&rhythm, 0);
		sound_streamregist(&opngen, (SOUNDCB)opngen_getpcmvr);
		sound_streamregist(&psg1, (SOUNDCB)psggen_getpcm);
		rhythm_bind(&rhythm);
		sound_streamregist(&adpcm, (SOUNDCB)adpcm_getpcm);
	} else {
		sound_streamregist(&opngen, (SOUNDCB)opngen_getpcm);
		sound_streamregist(&psg1, (SOUNDCB)psggen_getpcm);
	}

	iocore_attachinp(0x044, sb2_i044);
	iocore_attachinp(0x045, sb2_i045);
	iocore_attachout(0x044, sb2_o044);
	iocore_attachout(0x045, sb2_o045);
	if (opna) {
		iocore_attachinp(0x046, sb2_i044);
		iocore_attachinp(0x047, sb2_i047);
		iocore_attachout(0x046, sb2_o046);
		iocore_attachout(0x047, sb2_o047);
	}

	iocore_attachout(0x19c, sb2_o19c);
	iocore_attachout(0x19e, sb2_o19e);
}

void boardopnva_bind(void) {
	boardsb_bind(FALSE);
}

void boardsb2_bind(void) {
	boardsb_bind(TRUE);
}
