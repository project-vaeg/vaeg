#include "compiler.h"
#include "soundmng.h"
#include "machine/pccore.h"
#include "fdd_mtr.h"
#include "sound.h"

enum {
	FDDMTRSND_SEEK = 0,
	FDDMTRSND_SEEK1,
	FDDMTRSND_HEADON,
	FDDMTRSND_HEADOFF,
	FDDMTRSND_TRACKS
};

static struct {
	UINT enable;
	UINT headloaded;
	UINT headoffevent;
	struct {
		PMIXHDR hdr;
		PMIXTRK trk[FDDMTRSND_TRACKS];
	} snd;
} mtrsnd;

static void fddmtrsnd_load(PMIXDAT *dat, const char *fname, UINT rate) {
	char path[MAX_PATH];

	getbiospath(path, fname, sizeof(path));
	if (pcmmix_regfile(dat, path, rate) == SUCCESS) {
		return;
	}
	pcmmix_regfile(dat, fname, rate);
}

void fddmtrsnd_initialize(UINT rate) {
	ZeroMemory(&mtrsnd, sizeof(mtrsnd));
	if (rate == 0) {
		return;
	}
	mtrsnd.enable = 1;
	mtrsnd.snd.hdr.enable = (1 << FDDMTRSND_TRACKS) - 1;
	fddmtrsnd_load(&mtrsnd.snd.trk[FDDMTRSND_SEEK].data, "seek.wav", rate);
	mtrsnd.snd.trk[FDDMTRSND_SEEK].flag = PMIXFLAG_L | PMIXFLAG_R | PMIXFLAG_LOOP;
	fddmtrsnd_load(&mtrsnd.snd.trk[FDDMTRSND_SEEK1].data, "seek1.wav", rate);
	mtrsnd.snd.trk[FDDMTRSND_SEEK1].flag = PMIXFLAG_L | PMIXFLAG_R;
	fddmtrsnd_load(&mtrsnd.snd.trk[FDDMTRSND_HEADON].data, "headon.wav", rate);
	mtrsnd.snd.trk[FDDMTRSND_HEADON].flag = PMIXFLAG_L | PMIXFLAG_R;
	fddmtrsnd_load(&mtrsnd.snd.trk[FDDMTRSND_HEADOFF].data, "headoff.wav", rate);
	mtrsnd.snd.trk[FDDMTRSND_HEADOFF].flag = PMIXFLAG_L | PMIXFLAG_R;
	fddmtrsnd_volume(np2cfg.MOTORVOL);
}

void fddmtrsnd_bind(void) {
	if (mtrsnd.enable) {
		sound_streamregist(&mtrsnd.snd, (SOUNDCB)pcmmix_getpcm);
	}
}

void fddmtrsnd_deinitialize(void) {
	int i;
	void *ptr;

	for (i = 0; i < FDDMTRSND_TRACKS; i++) {
		ptr = mtrsnd.snd.trk[i].data.sample;
		mtrsnd.snd.trk[i].data.sample = NULL;
		if (ptr) {
			_MFREE(ptr);
		}
	}
}

void fddmtrsnd_volume(UINT volume) {
	SINT32 vol;
	int i;

	vol = (SINT32)((volume << 12) / 100);
	for (i = 0; i < FDDMTRSND_TRACKS; i++) {
		mtrsnd.snd.trk[i].volume = vol;
	}
}

static void fddmtrsnd_play(UINT num, BOOL play) {
	PMIXTRK *trk;

	if ((mtrsnd.enable) && (num < FDDMTRSND_TRACKS)) {
		sound_sync();
		trk = mtrsnd.snd.trk + num;
		if (play) {
			if (trk->data.sample) {
				trk->pcm = trk->data.sample;
				trk->remain = trk->data.samples;
				mtrsnd.snd.hdr.playing |= (1 << num);
			}
		} else {
			mtrsnd.snd.hdr.playing &= ~(1 << num);
		}
	}
}

// ----

enum {
	MOVE1TCK_MS = 15,
	MOVEMOTOR1_MS = 25,
	HEADUNLOAD_MS = 2000,
	DISK1ROL_MS = 166
};

_FDDMTR fddmtr;

static void fddmtr_event(void);

void fddmtrsnd_stop(void) {
	UINT i;

	for (i = 0; i < FDDMTRSND_TRACKS; i++) {
		fddmtrsnd_play(i, FALSE);
	}
	mtrsnd.headloaded = FALSE;
	mtrsnd.headoffevent = 0;
	fddmtr.curevent = 0;
}

void fddmtrsnd_seek(BOOL one_track, UINT duration_ms) {
	if (one_track) {
		if (fddmtr.curevent < 80) {
			fddmtr_event();
			fddmtrsnd_play(FDDMTRSND_SEEK1, TRUE);
			fddmtr.curevent = 80;
			fddmtr.nextevent = GETTICK() + MOVEMOTOR1_MS;
		}
	} else if (fddmtr.curevent < 100) {
		fddmtr_event();
		fddmtrsnd_play(FDDMTRSND_SEEK, TRUE);
		fddmtr.curevent = 100;
		fddmtr.nextevent = GETTICK() + duration_ms;
	}
}

void fddmtrsnd_headload(BOOL one_track) {
	if (mtrsnd.headloaded) {
		mtrsnd.headoffevent = 0;
		return;
	}
	fddmtrsnd_play(FDDMTRSND_HEADOFF, FALSE);
	if (one_track) {
		fddmtrsnd_play(FDDMTRSND_SEEK1, FALSE);
		fddmtrsnd_play(FDDMTRSND_SEEK1, TRUE);
	} else {
		fddmtrsnd_play(FDDMTRSND_HEADON, FALSE);
		fddmtrsnd_play(FDDMTRSND_HEADON, TRUE);
	}
	mtrsnd.headloaded = TRUE;
	mtrsnd.headoffevent = 0;
}

void fddmtrsnd_headunload(void) {
	if (mtrsnd.headloaded) {
		mtrsnd.headoffevent = GETTICK() + HEADUNLOAD_MS;
	}
}

static void fddmtr_event(void) {
	switch (fddmtr.curevent) {
	case 100:
		fddmtrsnd_play(FDDMTRSND_SEEK, FALSE);
		fddmtr.curevent = 0;
		break;

	default:
		fddmtr.curevent = 0;
		break;
	}
}

void fddmtr_initialize(void) {
	fddmtrsnd_stop();
	ZeroMemory(&fddmtr, sizeof(fddmtr));
	FillMemory(fddmtr.head, sizeof(fddmtr.head), 42);
}

void fddmtr_callback(UINT time) {
	if ((fddmtr.curevent) && (time >= fddmtr.nextevent)) {
		fddmtr_event();
	}
	if ((mtrsnd.headloaded) && (mtrsnd.headoffevent) && (time >= mtrsnd.headoffevent)) {
		fddmtrsnd_play(FDDMTRSND_HEADOFF, FALSE);
		fddmtrsnd_play(FDDMTRSND_HEADOFF, TRUE);
		mtrsnd.headloaded = FALSE;
		mtrsnd.headoffevent = 0;
	}
}

void fdbiosout(NEVENTITEM item) {
	fddmtr.busy = 0;
	(void)item;
}

void fddmtr_seek(REG8 drv, REG8 c, UINT size) {
	int regmove;
	SINT32 waitcnt;

	drv &= 3;
	regmove = c - fddmtr.head[drv];
	fddmtr.head[drv] = c;

	if (!np2cfg.MOTOR) {
		if (size) {
			fddmtr.busy = 1;
			nevent_set(NEVENT_FDBIOSBUSY, size * pccore.multiple, fdbiosout, NEVENT_ABSOLUTE);
		}
		return;
	}

	waitcnt = (size * DISK1ROL_MS) / (1024 * 8);
	if (regmove < 0) {
		regmove = 0 - regmove;
	}
	if (regmove == 1) {
		if (fddmtr.curevent < 80) {
			fddmtr_event();
			fddmtrsnd_play(FDDMTRSND_SEEK1, TRUE);
			fddmtr.curevent = 80;
			fddmtr.nextevent = GETTICK() + MOVEMOTOR1_MS;
		}
	} else if (regmove) {
		if (fddmtr.curevent < 100) {
			fddmtr_event();
			fddmtrsnd_play(FDDMTRSND_SEEK, TRUE);
			fddmtr.curevent = 100;
			fddmtr.nextevent = GETTICK() + (regmove * MOVE1TCK_MS);
		}
		if (regmove >= 32) {
			waitcnt += DISK1ROL_MS;
		}
	}
	if (waitcnt) {
		fddmtr.busy = 1;
		nevent_setbyms(NEVENT_FDBIOSBUSY, waitcnt, fdbiosout, NEVENT_ABSOLUTE);
	}
	(void)drv;
}
