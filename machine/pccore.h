
#include "machine/nevent.h"
#include "machine/statsave.h"
#include "machine/clockscale.h"
#include "upd8087/upd8087.h"

enum {
	PCBASECLOCK40 = 3993600,
	PCBASECLOCK25 = 2457600,
	PCBASECLOCK20 = 1996800,
	PCCORE_STANDARD_MULTIPLE = 2,
	PCCORE_CPU_MULTIPLE_MAX = 32
};

enum {
	CPUMODE_8MHZ = 0x20,
	CPUMODE_BASE4MHZ = 0x40,

	PCHDD_SASI = 0x01,
	PCHDD_SCSI = 0x02,

	PCROM_BIOS = 0x01,
	PCROM_SOUND = 0x02,
	PCROM_SASI = 0x04,
	PCROM_SCSI = 0x08,

	PCSOUND_NONE = 0x00,

	PCCBUS_MPU98 = 0x0002,

	PCMODEL_VA1 = 1,
	PCMODEL_VA2 = 2,

	FMBOARD_NONE = 0x0000,
	FMBOARD_VA_OPN = 0x0100,
	FMBOARD_VA_OPNA = 0x0200,
};

typedef struct {
	UINT8 KEY_MODE;
	UINT8 XSHIFT;
	UINT8 BTN_RAPID;
	UINT8 BTN_MODE;

	UINT8 dipsw[3]; // DIP-switch values latched at reset.
	UINT8 MOUSERAPID;

	UINT8 calendar;
	UINT8 usefd144;
	UINT8 upd8087_enable;
	UINT32 upd8087_clock_hz;
	// Configuration values used during reset and infrequently thereafter.
	OEMCHAR model[8];
	UINT baseclock;
	UINT multiple;
	UINT8 sgp_speed_mode;
	UINT8 sgp_multiplier;

	UINT8 memsw[8];
	/* Installed conventional RAM ceiling; BIOS MEMswtch remains independent. */
	UINT16 main_ram;
	UINT8 main_ram_auto;
	/* vaeg extension: report PC-8801 standard speed (S) in V1/V2 mode. */
	UINT8 v1v2_standard;
	/* vaeg extension: start in N-BASIC (needs a user-supplied n80.rom). */
	UINT8 v1v2_nmode;
	/* N-BASIC ROM file chosen in the menu; empty selects the first found. */
	OEMCHAR v1v2_n80rom[32];
	/* DIP switch SW1, monitor type, read at reset: 0 = 24.8 kHz, 1 = 15 kHz. */
	UINT8 monitor_15khz;
	/* Display on the digital RGB output (8 colours); a display-side policy. */
	UINT8 monitor_digital;
	/* 15 kHz scanlines: 0 = gaps (CRT), 1 = each line repeated (LCD). */
	UINT8 monitor_15khz_fill;
	/* V1/V2 at 24.8 kHz shown as a PC-8801 (vaeg extension): 8x8 text with
	 * each row doubled and 200-line graphics with scanline gaps. */
	UINT8 v1v2_8801_display;
	/* CPU speed in percent of the configured clock (10-100; 0 = 100):
	 * instruction cycles cost 100 / speed times as much, in V3 and in the
	 * V1/V2 compatible mode; frames, timers and sound are unchanged. */
	UINT8 cpu_speed;
	/* Cassette tape: deliver bytes 16 times faster than the baud rate. */
	UINT8 cmt_fast;
	/* Cassette tape sound volume, 0-128 (independent of the master volume). */
	UINT8 cmt_vol;

	UINT8 ITF_WORK;
	UINT8 EXTMEM;
	UINT32 BG_COLOR;
	UINT32 FG_COLOR;

	UINT16 samplingrate;
	UINT16 delayms;
	UINT16 SOUND_SW;
	UINT8 snd_x;

	UINT8 snd14opt[3];
	UINT8 spbopt;
	UINT8 spb_vrc;
	UINT8 spb_vrl;
	UINT8 spb_x;

	UINT8 BEEP_VOL;
	/* Fine beeper volume 0-128 (master volume); above 128 BEEP_VOL applies. */
	UINT8 beep_level;
	UINT8 vol14[6];
	UINT8 vol_fm;
	UINT8 vol_ssg;
	UINT8 vol_adpcm;
	UINT8 vol_rhythm;

	UINT8 mpuenable;
	UINT8 mpuopt;

	UINT8 fddequip;
	UINT8 MOTOR;
	UINT8 MOTORVOL;

	UINT8 lockedkey;

	OEMCHAR sasihdd[2][MAX_PATH];
	OEMCHAR scsihdd[7][MAX_PATH];
	OEMCHAR fontfile[MAX_PATH];
	OEMCHAR biospath[MAX_PATH];
} NP2CFG;

typedef struct {
	UINT32 baseclock;
	UINT multiple;

	UINT8 cpumode;
	UINT8 model; // Serialized compatibility byte; active VA model is model_va.
	UINT8 hddif;
	UINT8 extmem;
	UINT8 dipsw[3]; // リセット時のDIPSW
	UINT8 rom;

	UINT32 sound;
	UINT32 device;

	UINT32 realclock;

	UINT8 model_va;
} PCCORE;

enum {
	COREEVENT_SHUT = 0,
	COREEVENT_RESET = 1,
	COREEVENT_EXIT = 2
};

#ifdef __cplusplus
extern "C" {
#endif

extern const OEMCHAR np2version[];

extern NP2CFG np2cfg;
extern PCCORE pccore;
extern CLOCKSCALE pccore_cpu_scale;
extern CLOCKSCALE pccore_compat_scale;
extern UPD8087_STATE upd8087;
extern UINT8 screenupdate;
extern int soundrenewal;
extern BOOL drawframe;
extern UINT drawcount;
extern BOOL hardwarereset;

void getbiospath(OEMCHAR *path, const OEMCHAR *fname, int maxlen);
void screendispva(NEVENTITEM item);
void screenvsyncva(NEVENTITEM item);
//void screenvsyncva2(NEVENTITEM item);
void sysp4vsyncint(NEVENTITEM item);
void sysp4vsyncstart(NEVENTITEM item);
void sysp4vsyncend(NEVENTITEM item);

void pccore_cfgupdate(void);
BOOL pccore_cpu_multiple_valid(UINT multiple);
void pccore_clockrestore(void);
UINT pccore_cpu_multiple(void);
UINT pccore_cpu_speed(void);
void pccore_set_cpu_speed(UINT percent);
UINT32 pccore_cpu_clock(void);
UINT16 pccore_mainram_kb(void);
UINT32 pccore_mainram_limit(void);
/* Apply pending RAM settings only while the CPU is stopped for reset. */
void pccore_mainram_reset(void);

void pccore_init(void);
void pccore_term(void);
void pccore_reset(void);
void pccore_exec(BOOL draw);
void pccore_redraw(void);

void pccore_postevent(UINT32 event);

void pccore_debugint(UINT32 no);
void pccore_debugmem(UINT32 op, UINT32 addr, UINT16 data);

#ifdef __cplusplus
}
#endif
