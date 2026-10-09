#include "compiler.h"
#include "machine/pccore.h"
#include "diskdrv.h"
#include "fdd_mtr.h"
#include "machine/timing.h"

#define MSSHIFT 16

typedef struct {
	UINT32 tick;     // 前回timing_getcount実行時のGETTICK()の値
	UINT32 msstep;   // 1msecあたりの画面表示サイクル数 << MSSHIFT
	UINT cnt;        // 経過時間を画面表示サイクル数であらわしたもの(整数部)
	UINT32 fraction; // 経過時間を画面表示サイクル数であらわしたもの(小数点以下MSSHIFTビット)
	UINT speed;      // emulation speed in percent of real time
} TIMING;

enum {
	TIMING_SPEED_MIN = 10,
	TIMING_SPEED_MAX = 400
};

static TIMING timing = {0, 0, 0, 0, 100};

void timing_reset(void) {
	timing.tick = GETTICK();
	timing.cnt = 0;
	timing.fraction = 0;
}

/*
表示周期を設定する
	IN:		lines		1画面あたりのライン数(非表示区間、垂直同期区間を含む)
			crthz		1秒当り描画ライン数
*/
void timing_setrate(UINT lines, UINT crthz) {
	timing.msstep = (crthz << (MSSHIFT - 3)) / lines / (1000 >> 3);
}

/* Emulation speed: guest frames are due at percent / 100 of real time. */
void timing_setspeed(UINT percent) {
	if (percent == 0) {
		percent = 100;
	} else if (percent < TIMING_SPEED_MIN) {
		percent = TIMING_SPEED_MIN;
	} else if (percent > TIMING_SPEED_MAX) {
		percent = TIMING_SPEED_MAX;
	}
	timing.speed = percent;
}

/* Add span milliseconds of host time; returns the frames now due. */
UINT timing_addspan(UINT32 span) {
	UINT64 fraction;

	if (span >= 1000) {
		span = 1000;
	}
	fraction = timing.fraction + ((UINT64)span * timing.msstep * timing.speed) / 100;
	timing.cnt += (UINT)(fraction >> MSSHIFT);
	timing.fraction = (UINT32)(fraction & ((1 << MSSHIFT) - 1));
	return (timing.cnt);
}

void timing_setcount(UINT value) {
	timing.cnt = value;
}

void timing_hosttick(void) {
	fddmtr_callback(GETTICK());
}

/*
経過時間を画面表示サイクル数で返却する
	この値はtiming_setcountでリセットできる。
*/
UINT timing_getcount(void) {
	UINT32 ticknow;
	UINT32 span;

	ticknow = GETTICK();
	span = ticknow - timing.tick;
	if (span) {
		timing.tick = ticknow;
		fddmtr_callback(ticknow);
		timing_addspan(span);
	}
	return (timing.cnt);
}
