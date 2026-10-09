#include "compiler.h"
#include "dosio.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "sysportva.h"
#include "sound.h"
#include "fmboard.h"
#include "beep.h"

_BEEP beep;
/* Port sound: 40h bit 7 (FBEEP) drives the speaker directly (VA technical
 * manual 5.10.4), mixed with the buzzer as its own one-shot level stream. */
_BEEP fbeep;
BEEPCFG beepcfg;

// #define	BEEPLOG

#if defined(BEEPLOG)
static struct {
	FILEH fh;
	UINT events;
	UINT32 event[0x10000];
} bplog;

static void beeplogflash(void) {
	if ((bplog.fh != FILEH_INVALID) && (bplog.events)) {
		file_write(bplog.fh, bplog.event, bplog.events * sizeof(UINT32));
		bplog.events = 0;
	}
}
#endif

void beep_initialize(UINT rate) {
	beepcfg.rate = rate;
	beepcfg.vol = 2 << BEEPVOL_SHIFT;
#if defined(BEEPLOG)
	bplog.fh = file_create("beeplog");
	bplog.events = 0;
#endif
}

void beep_deinitialize(void) {
#if defined(BEEPLOG)
	beeplogflash();
	if (bplog.fh != FILEH_INVALID) {
		file_close(bplog.fh);
		bplog.fh = FILEH_INVALID;
	}
#endif
}

/* Legacy volume, 0-3 (configuration key BEEP_vol). */
void beep_setvol(UINT vol) {
	beepcfg.vol = (vol & 3) << BEEPVOL_SHIFT;
}

/* Fine volume, 0-128, where 128 is the legacy maximum 3. */
void beep_setlevel(UINT level) {
	if (level > 128) {
		level = 128;
	}
	beepcfg.vol = (level * (3 << BEEPVOL_SHIFT) + 64) / 128;
}

void beep_changeclock(void) {
	UINT32 hz;
	UINT rate;

	hz = pccore.realclock / 25;
	rate = beepcfg.rate / 25;
	beepcfg.samplebase = (1 << 16) * rate / hz;
}

void beep_reset(void) {
	beep_changeclock();
	ZeroMemory(&beep, sizeof(beep));
	beep.mode = 1;
	ZeroMemory(&fbeep, sizeof(fbeep));
	fbeep.mode = 0;
}

void beep_hzset(UINT16 cnt, UINT beepclock) {
	double hz;

	sound_sync();
	beep.hz = 0;
	if ((cnt & 0xff80) && (beepcfg.rate)) {
		hz = 65536.0 / 4.0 * beepclock / beepcfg.rate / cnt;
		if (hz < 0x8000) {
			beep.hz = (UINT16)hz;
			return;
		}
	}
}

void beep_modeset(void) {
	UINT8 newmode;

	newmode = (pit.ch[1].ctrl >> 2) & 3;
	if (beep.mode != newmode) {
		sound_sync();
		beep.mode = newmode;
		beep_eventinit();
	}
}

static void beep_streamevent(BEEP bp, int enable) {
	BPEVENT *evt;
	SINT32 clock;

	if (bp->enable != enable) {
		if (bp->events >= (BEEPEVENT_MAX / 2)) {
			sound_sync();
		}
		bp->enable = enable;
		if (bp->events < BEEPEVENT_MAX) {
			clock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
			evt = bp->event + bp->events;
			bp->events++;
			evt->clock = (clock - bp->clock) * beepcfg.samplebase;
			evt->enable = enable;
			bp->clock = clock;
		}
	}
}

static void beep_eventset(void) {
	BPEVENT *evt;
	int enable;
	SINT32 clock;

	enable = beep.low & beep.buz;
	if (beep.enable != enable) {
#if defined(BEEPLOG)
		UINT32 tmp;
		tmp = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
		if (enable) {
			tmp |= 0x80000000;
		} else {
			tmp &= ~0x80000000;
		}
		bplog.event[bplog.events++] = tmp;
		if (bplog.events >= (sizeof(bplog.event) / sizeof(UINT32))) {
			beeplogflash();
		}
#endif
		if (beep.events >= (BEEPEVENT_MAX / 2)) {
			sound_sync();
		}
		beep.enable = enable;
		if (beep.events < BEEPEVENT_MAX) {
			clock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
			evt = beep.event + beep.events;
			beep.events++;
			evt->clock = (clock - beep.clock) * beepcfg.samplebase;
			evt->enable = enable;
			beep.clock = clock;
		}
	}
}

void beep_eventinit(void) {
	beep.low = 0;
	beep.enable = 0;
	beep.lastenable = 0;
	beep.clock = soundcfg.lastclock;
	beep.events = 0;
}

void beep_eventreset(void) {
	beep.lastenable = beep.enable;
	beep.clock = soundcfg.lastclock;
	beep.events = 0;
	fbeep.lastenable = fbeep.enable;
	fbeep.clock = soundcfg.lastclock;
	fbeep.events = 0;
}

void beep_lheventset(int low) {
	if (beep.low != low) {
		beep.low = low;
		beep_eventset();
	}
}

/*
 * Buzzer on when either 1CDh bit 3 (XBEEP, active low) or 40h bit 5 (BEEP,
 * the 88-mode control) is on (VA technical manual 5.10.3).
 */
void beep_oneventset(void) {
	int buz;

	buz = (!(sysportva.c & 8) || (sysportva.port040 & 0x20)) ? 1 : 0;
	if (beep.buz != buz) {
		beep.buz = buz;
		beep_eventset();
	}
}

/* Port sound output: 40h bit 7 (FBEEP), enabled by 190h bit 4 (FBEN). */
void beep_portsoundset(void) {
	beep_streamevent(&fbeep, ((sysportva.port040 & 0x80) && (sysportva.port190 & 0x10)) ? 1 : 0);
}
