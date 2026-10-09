#include "compiler.h"
#include "strres.h"
#include "dosio.h"
#include "soundmng.h"
#include "sysmng.h"
#include "timemng.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "cmt.h"
#include "memctrlva.h"
#include "cbuscore.h"
#include "mpu98ii.h"
#include "romva.h"
#include "biosmem.h"
#include "scrndraw.h"
#include "sound.h"
#include "fmboard.h"
#include "beep.h"
#include "font.h"
#include "diskdrv.h"
#include "fddfile.h"
#include "fdd_mtr.h"
#include "sxsi.h"
#include "np2ver.h"
#include "machine/calendar.h"
#include "machine/timing.h"
#include "machine/keystat.h"
#include "machine/debugsub.h"
#include "upd9002_diagnostic.h"
#include "diagnostics/upd9002_debug.h"

#include "bmsio.h"
#include "emsio.h"
#include "bkupmemva.h"

#include "../vram/maketextva.h"
#include "../vram/makesprva.h"
#include "../vram/makegrphva.h"
#include "scrnmng.h"
#include "../vram/scrndrawva.h"
#include "memoryva.h"
#include "tsp.h"
#include "sgp.h"
#include "videova.h"
#include "subsystemmx.h"
#include "va91.h"

const OEMCHAR np2version[] = OEMTEXT(VAEGREL_CORE);

#define PCBASEMULTIPLE PCCORE_STANDARD_MULTIPLE

NP2CFG np2cfg = {.KEY_MODE = 0,
                 .XSHIFT = 0,
                 .BTN_RAPID = 0,
                 .BTN_MODE = 0,
                 .dipsw = {0x3e, 0x73, 0x7b},
                 .MOUSERAPID = 0,
                 .calendar = 0,
                 .usefd144 = 0,
                 .upd8087_enable = 0,
                 .upd8087_clock_hz = UPD8087_DEFAULT_CLOCK_HZ,
                 .model = OEMTEXT("88VA2"),
                 .baseclock = PCBASECLOCK40,
                 .multiple = 2,
                 .sgp_speed_mode = 0,
                 .sgp_multiplier = 1,
                 .memsw = {0x48, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x6e},
                 .main_ram = 640,
                 .main_ram_auto = 1,
                 .v1v2_standard = 0,
                 .v1v2_nmode = 0,
                 .ITF_WORK = 1,
                 .EXTMEM = EMSIO_DEFAULT_MEGABYTES,
                 .BG_COLOR = 0x000000,
                 .FG_COLOR = 0xffffff,
                 .samplingrate = 22050,
                 .delayms = 500,
                 .SOUND_SW = FMBOARD_VA_OPNA,
                 .snd_x = 0,
                 .snd14opt = {0, 0, 0},
                 .spbopt = 0xd1,
                 .spb_vrc = 0,
                 .spb_vrl = 0,
                 .spb_x = 1,
                 .BEEP_VOL = 3,
                 .beep_level = 0xff,
                 .cmt_fast = 1,
                 .cmt_vol = 48,
                 .vol14 = {0x0c, 0x0c, 0x08, 0x06, 0x03, 0x0c},
                 .vol_fm = 64,
                 .vol_ssg = 64,
                 .vol_adpcm = 64,
                 .vol_rhythm = 64,
                 .mpuenable = 1,
                 .mpuopt = 0x82,
                 .fddequip = 3,
                 .MOTOR = 1,
                 .MOTORVOL = 80,
                 .lockedkey = 0,
                 .sasihdd = {OEMTEXT(""), OEMTEXT("")},
                 .scsihdd = {OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT(""),
                             OEMTEXT(""), OEMTEXT("")},
                 .fontfile = OEMTEXT(""),
                 .biospath = OEMTEXT("")};

PCCORE pccore = {PCBASECLOCK25,
                 PCBASEMULTIPLE,
                 0,
                 PCMODEL_VA1,
                 0,
                 0,
                 {0x3e, 0x73, 0x7b},
                 0,
                 0,
                 0,
                 PCBASECLOCK25 *PCBASEMULTIPLE};
CLOCKSCALE pccore_cpu_scale = {PCCORE_STANDARD_MULTIPLE, PCCORE_STANDARD_MULTIPLE, 0};
/* uPD70008-compatible mode: the CPU speed only (the multiplier is V30-only). */
CLOCKSCALE pccore_compat_scale = {100, 100, 0};
UPD8087_STATE upd8087;
static UINT pccore_cpu_multiple_value = PCCORE_STANDARD_MULTIPLE;
static UINT pccore_cpu_speed_value = 100;

static UINT16 active_main_ram = 640;

static UINT16 pccore_normalize_mainram(UINT16 value) {
	switch (value) {
	case 256:
	case 384:
	case 512:
	case 640:
		return value;
	default:
		return 640;
	}
}

UINT16 pccore_mainram_kb(void) {
	return pccore_normalize_mainram(np2cfg.main_ram);
}

UINT32 pccore_mainram_limit(void) {
	/* Menu changes are pending until the next machine reset. */
	return (UINT32)active_main_ram * 1024;
}

void pccore_mainram_reset(void) {
	active_main_ram = pccore_normalize_mainram(np2cfg.main_ram);
	bkupmemva_sync_mainram();
}

UINT8 screenupdate = 3; // Bit 0 requests a partial redraw.
                        // Bit 1 requests a full redraw.
int screendispflag = 1;
int soundrenewal = 0;
BOOL drawframe;
UINT drawcount = 0;
BOOL hardwarereset = FALSE;
static BOOL pccore_debug_resume = FALSE;

// ---------------------------------------------------------------------------

BOOL pccore_cpu_multiple_valid(UINT multiple) {
	return ((multiple >= 1) && (multiple <= PCCORE_CPU_MULTIPLE_MAX));
}

void pccore_clockrestore(void) {
	UINT multiple;

	multiple = np2cfg.multiple;
	if (multiple == 0) {
		multiple = 1;
	} else if (multiple > PCCORE_CPU_MULTIPLE_MAX) {
		multiple = PCCORE_CPU_MULTIPLE_MAX;
	}
	pccore.multiple = PCCORE_STANDARD_MULTIPLE;
	pccore.realclock = pccore.baseclock * pccore.multiple;
	pccore_cpu_multiple_value = multiple;
	pccore_set_cpu_speed(np2cfg.cpu_speed);
}

/* CPU speed 10-100 % (0 selects 100): cycles cost 100 / percent. */
void pccore_set_cpu_speed(UINT percent) {
	if ((percent == 0) || (percent > 100)) {
		percent = 100;
	} else if (percent < 10) {
		percent = 10;
	}
	pccore_cpu_speed_value = percent;
	clockscale_configure(&pccore_cpu_scale, PCCORE_STANDARD_MULTIPLE * 100,
	                     pccore_cpu_multiple_value * percent);
	clockscale_configure(&pccore_compat_scale, 100, percent);
}

UINT pccore_cpu_speed(void) {
	return (pccore_cpu_speed_value);
}

UINT pccore_cpu_multiple(void) {
	return (pccore_cpu_multiple_value);
}

UINT32 pccore_cpu_clock(void) {
	return (pccore.baseclock * pccore_cpu_multiple_value);
}

// ---------------------------------------------------------------------------

void getbiospath(OEMCHAR *path, const OEMCHAR *fname, int maxlen) {
	const OEMCHAR *p;

	p = np2cfg.biospath;
	if (p[0]) {
		file_cpyname(path, p, maxlen);
		file_setseparator(path, maxlen);
		file_catname(path, fname, maxlen);
	} else {
		file_cpyname(path, file_getcd(fname), maxlen);
	}
}

// ----

static void pccore_set(void) {
	UINT8 model;
	UINT8 extsize;

	ZeroMemory(&pccore, sizeof(pccore));
	model = PCMODEL_VA1;

	/* The active tree has only PC-88VA models.  Unknown legacy values
	 * deliberately fall back to the VA2 configuration. */
	pccore.model_va = PCMODEL_VA2;
	if (!milstr_cmp(np2cfg.model, str_VA1)) {
		pccore.model_va = PCMODEL_VA1;
	}
	pccore.model = model;

	if (np2cfg.baseclock >= ((PCBASECLOCK40 + PCBASECLOCK25) / 2)) {
		pccore.baseclock = PCBASECLOCK40; // 4.0MHz
		pccore.cpumode = CPUMODE_BASE4MHZ;
	} else if (np2cfg.baseclock >= ((PCBASECLOCK25 + PCBASECLOCK20) / 2)) {
		pccore.baseclock = PCBASECLOCK25; // 2.5MHz
		pccore.cpumode = 0;
	} else {
		pccore.baseclock = PCBASECLOCK20; // 2.0MHz
		pccore.cpumode = CPUMODE_8MHZ;
	}
	pccore_clockrestore();

	// Configure VA expansion memory.
	extsize = 0;
	if (!(np2cfg.dipsw[2] & 0x80)) {
		extsize = min(np2cfg.EXTMEM, EMSIO_MAX_MEGABYTES);
	}
	pccore.extmem = extsize;
	CopyMemory(pccore.dipsw, np2cfg.dipsw, 3);

	// Select the installed sound board.
	pccore.sound = np2cfg.SOUND_SW;

	// Select other supported expansion-bus devices.
	pccore.device = 0;
	if (np2cfg.mpuenable) {
		pccore.device |= PCCBUS_MPU98;
	}
}

// --------------------------------------------------------------------------

static void sound_init(void) {
	UINT rate;

	rate = np2cfg.samplingrate;
	if ((rate != 11025) && (rate != 22050) && (rate != 44100)) {
		rate = 0;
	}
	sound_create(rate, np2cfg.delayms);
	fddmtrsnd_initialize(rate);
	beep_initialize(rate);
	if (np2cfg.beep_level <= 128) {
		beep_setlevel(np2cfg.beep_level);
	} else {
		beep_setvol(np2cfg.BEEP_VOL);
	}
	opngen_initialize(rate);
	opngen_setvol(np2cfg.vol_fm);
	psggen_initialize(rate);
	psggen_setvol(np2cfg.vol_ssg);
	rhythm_initialize(rate);
	rhythm_setvol(np2cfg.vol_rhythm);
	adpcm_initialize(rate);
	adpcm_setvol(np2cfg.vol_adpcm);
}

static void sound_term(void) {
	soundmng_stop();
	rhythm_deinitialize();
	beep_deinitialize();
	fddmtrsnd_deinitialize();
	sound_destroy();
}

void pccore_init(void) {
	UPD8087_CONFIG upd8087_config;

	upd8087_config.enabled = np2cfg.upd8087_enable != 0;
	upd8087_config.clock_hz = np2cfg.upd8087_clock_hz;
	upd8087_initialize(&upd8087, &upd8087_config);
	CPU_INITIALIZE();

	// VA rendering owns its palette and raster conversion.
	maketextva_initialize();
	makesprva_initialize();
	makegrphva_initialize();
	subsystemmx_initialize();
	sxsi_initialize();

	font_initialize();
	font_load(np2cfg.fontfile, TRUE);
	fddfile_initialize();

	sound_init();

	rs232c_construct();
	mpu98ii_construct();
	cmt_initialize();

	iocore_create();
}

void pccore_term(void) {
	sound_term();

	fdd_eject(0);
	fdd_eject(1);
	fdd_eject(2);
	fdd_eject(3);

	iocore_destroy();

	mpu98ii_destruct();
	rs232c_destruct();
	cmt_deinitialize();

	sxsi_trash();

	CPU_DEINITIALIZE();
}

void pccore_cfgupdate(void) {
	BOOL renewal;
	int i;

	renewal = FALSE;
	for (i = 0; i < 8; i++) {
		if (np2cfg.memsw[i] != mem[MEMX_MSW + i * 4]) {
			np2cfg.memsw[i] = mem[MEMX_MSW + i * 4];
			renewal = TRUE;
		}
	}
	{
		UINT8 val;

		val = keystat_getlockedkey();
		if (np2cfg.lockedkey != val) {
			np2cfg.lockedkey = val;
			renewal = TRUE;
		}
	}
	if (renewal) {
		sysmng_update(SYS_UPDATECFG);
	}
}

void pccore_reset(void) {
	int i;

	pccore_debug_resume = FALSE;
	pccore_mainram_reset();
	drawcount = 0;
	scrnmng_reset_metrics();
	soundmng_stop();
	if (soundrenewal) {
		soundrenewal = 0;
		sound_term();
		sound_init();
	}
	ZeroMemory(mem, 0x110000);
	ZeroMemory(mem + FONT_ADRS, 0x08000);

	// Copy configured memory-switch bytes into the VA work area.
	for (i = 0; i < 8; i++) {
		mem[0xa3fe2 + i * 4] = np2cfg.memsw[i];
	}

	pccore_set();
	{
		UPD8087_CONFIG upd8087_config;

		/* Configuration is intentionally applied only at machine reset. */
		upd8087_config.enabled = np2cfg.upd8087_enable != 0;
		upd8087_config.clock_hz = np2cfg.upd8087_clock_hz;
		upd8087_apply_config(&upd8087, &upd8087_config);
	}
	sgp_configure_speed();
	bmsio_set();
	keystat_setlockedkey(np2cfg.lockedkey);
	nevent_allreset();

	// Reset the CPU after deriving the VA model and expansion-memory size.

	CPU_RESET();
	CPU_SETEXTSIZE((UINT32)pccore.extmem);

	// Open configured SASI and SCSI media.
	sxsi_open();
	if (sxsi_issasi()) {
		pccore.hddif |= PCHDD_SASI;
		TRACEOUT(("supported SASI"));
	}
	if (sxsi_isscsi()) {
		pccore.hddif |= PCHDD_SCSI;
		TRACEOUT(("supported SCSI"));
	}

	sound_changeclock();
	beep_changeclock();
	sound_reset();
	fddmtrsnd_bind();

	fddfile_reset2dmode();

	iocore_reset(); // Sound-board reset calls the native PIC interface.
	cbuscore_reset();
	fmboard_reset(pccore.sound);

	upd9002_memorymap_va();
	iocore_build();
	iocore_bind();
	cbuscore_bind();
	fmboard_bind();

	fddmtr_initialize();
	calendar_initialize();

	romva_initialize();
	memctrlva_nmode_reset();
	va91_initialize();
	CS_BASE = 0xf0000;
	CPU_CS = 0xf000;
	CPU_IP = 0xfff0;

	CPU_CLEARPREFETCH();
	sysmng_cpureset();

	timing_reset();
	soundmng_play();
}

static void drawscreenva(void) {
	int y;
	BOOL text200;
	BOOL grph200;
	UINT16 lines;

	//	if (videova.grmode & 0x1000) {
	//		// SYNCEN: horizontal-sync output enable.
	tsp_updateclock();
	//	}

	if (!drawframe) {
		return;
	}

	lines = tsp.screenlines;
	if (videova_hsyncmode() != VIDEOVA_24_8KHZ)
		lines *= 2;
	if (lines > SURFACE_HEIGHT)
		lines = SURFACE_HEIGHT;

	if ((tsp.flag & TSP_F_LINESCHANGED) && (videova.grmode & 0x1000)) {
		// Apply a TSP line-count change only while SYNCEN is enabled.
		scrnmng_setheight(0, lines);
		tsp.flag &= ~TSP_F_LINESCHANGED;
	}

	maketextva_begin(&text200);
	makesprva_begin();
	makegrphva_begin(&grph200);
	if (tsp.screenlines != 0) {
		VIDEOVA_GRAPHICS_WINDOW window;

		videova_graphics_window(videova_hsyncmode(), videova.grmode, tsp.syncparam, &window);
		makegrphva_setwindow(&window);
	}
	scrndrawva_compose_begin();

	if (videova_hsyncmode() != VIDEOVA_24_8KHZ) {
		// 15.7 kHz output.
		switch (videova.grmode & 0x00c0) {
		case 0x00: // Non-interlaced mode 0.
		case 0x40: // Non-interlaced mode 1.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster: a scanline gap as on a CRT, or the line
				// again as an LCD shows a 15 kHz signal (display setting).
				if (!np2cfg.monitor_15khz_fill) {
					maketextva_blankraster();
					makesprva_blankraster();
					makegrphva_blankraster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;

		case 0x80: // Interlaced mode 0.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster; reuse the previous raster where noted.
				if (text200) {
					// Reuse the previous output raster.
				} else {
					maketextva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0xc0: // Interlaced mode 1.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster.
				if (text200) {
					// Reuse the previous output raster.
				} else {
					maketextva_raster();
				}
				// Sprites are always 200-line and retain the previous output raster.
				if (grph200) {
					// Reuse the previous output raster.
				} else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		}
	} else {
		// 24.8 kHz output. The PC-8801-style V1/V2 display shows 200-line
		// graphics as non-interlaced mode 0 (odd rasters blank).
		WORD rsm = videova.grmode & 0x00c0;

		if (videova_8801_display() && (rsm == 0x40)) {
			rsm = 0x00;
		}
		switch (rsm) {
		case 0x00: // Non-interlaced mode 0.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster.
				maketextva_raster();
				makesprva_raster();
				if (grph200) {
					makegrphva_blankraster();
				} else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0x40: // Non-interlaced mode 1.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster.
				maketextva_raster();
				makesprva_raster();
				if (grph200) {
					// Reuse the previous output raster.
				} else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0x80: // Interlaced mode 0.
		case 0xc0: // Interlaced mode 1.
			// This mode combination is prohibited.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				maketextva_blankraster();
				makesprva_blankraster();
				makegrphva_blankraster();
				scrndrawva_compose_raster();
				y++;
				scrndrawva_compose_raster();
				y++;
			}
			break;
		}
	}

	screenupdate |= 2; // The VA renderer currently implements full redraws only.

	if (screenupdate) {
		screenupdate = scrndrawva_draw((BYTE)(screenupdate & 2));
		drawcount++;
	}
}

static void screendispva_setnevent() {
	nevent_set(NEVENT_FLAMES, tsp.dispclock, screenvsyncva, NEVENT_RELATIVE);
	nevent_set(NEVENT_FLAMES2, tsp.sysp4vsyncextension, sysp4vsyncend, NEVENT_RELATIVE);
}

// Begin the active-display period.
void screendispva(NEVENTITEM item) {
	/*
	PICITEM		pi;
*/
	tsp.vsync = 0;
	/*	Moved to sysp4vsyncstart.
	screendispflag = 0;
*/
	drawscreenva();
	/*
	pi = &pic.pi[0];
	if (pi->irr & PIC_CRTV) {
		pi->irr &= ~PIC_CRTV;
	}
*/
	screendispva_setnevent();
}

// Begin the vertical-retrace period.
void screenvsyncva(NEVENTITEM item) {
	//	tsp.vsync = 0x20;
	tsp.vsync = 0x40;

	videova.blinkcnt++;
	if (--tsp.blinkcnt == 0) {
		tsp.blinkcnt = tsp.blink;
		tsp.blinkcnt2++;
	}
#if 0
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock, screendispva, NEVENT_RELATIVE);
#else
	/*
	// Delay the interrupt by six clocks.
	// Software may observe VRTC at port 040H before the interrupt;
	// this ordering avoids a hang in Saishuu Heiki UPO.
	nevent_set(NEVENT_FLAMES, 6, screenvsyncva2, NEVENT_RELATIVE);
*/
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock, screendispva, NEVENT_RELATIVE);
#endif
}

#if 1
/*
void screenvsyncva2(NEVENTITEM item) {
	pic_setirq(2);
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock - 6, screendispva, NEVENT_RELATIVE);

}
*/
#endif

void sysp4vsyncint(NEVENTITEM item) {
	pic_setirq(2);
}

void sysp4vsyncstart(NEVENTITEM item) {
	tsp.sysp4vsync = 0x20;
	keyboard_matrix_tick();

	// Delay the interrupt by six clocks.
	// Software may observe VRTC at port 040H before the interrupt;
	// this ordering avoids a hang in Saishuu Heiki UPO.
	nevent_set(NEVENT_FLAMES2, 6, sysp4vsyncint, NEVENT_RELATIVE);

	screendispflag = 0;
}

void sysp4vsyncend(NEVENTITEM item) {
	PICITEM pi;

	tsp.sysp4vsync = 0;

	pi = &pic.pi[0];
	if (pi->irr & PIC_CRTV) {
		pi->irr &= ~PIC_CRTV;
	}

	nevent_set(NEVENT_FLAMES2, tsp.sysp4dispclock, sysp4vsyncstart, NEVENT_RELATIVE);
}

// ---------------------------------------------------------------------------

//@@@@@@

// Debug breakpoints.
typedef struct {
	BOOL enabled;
	UINT16 seg;
	UINT16 off;
} BREAKADDR;

enum {
	BREAKADDR_MAX = 16,
};

BOOL stopexec = FALSE;      // Stop execution.
BOOL singlestep = FALSE;    // Execute one instruction.
BOOL breakpointflag = TRUE; //FALSE;				// Enable breakpoint checks.
BREAKADDR breakaddrx[BREAKADDR_MAX] = {
    // Debug breakpoints.
    {FALSE, 0xe000, 0x9213},
    {FALSE, 0xe000, 0xb577},
};

void pccore_debugmem(UINT32 op, UINT32 addr, UINT16 data) {
	/*
	int	x = 0;

    if (addr == 0x2d830+0xb955) {
		x=op+addr+data;
	}
*/
	(void)op;
	(void)addr;
	(void)data;
}

void pccore_debugint(UINT32 no) {
	if (no != 0x82 && !(no == 0x83 && CPU_AX == 0x2e00) && no != 0x96) {
		TRACEOUT((
		    "cpu: int 0x%02x %04x:%04x rom0=%02x AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x SP=%04x DS=%04x ES=%04x SS=%04x",
		    no, CPU_CS, CPU_IP, memoryva.rom0_bank, CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_SI, CPU_DI,
		    CPU_BP, CPU_SP, CPU_DS, CPU_ES, CPU_SS));
	}
	/*
	if (no == 0x8b && CPU_AH == 0x17) {
		int i;
		TRACEOUT(("Music BIOS (17h): CH=%x, DL=%x, ES:BP=%.4x:%.4x",CPU_CH, CPU_DL, CPU_ES, CPU_BP));
		for (i = 0; i < CPU_DL; i++) {
			TRACEOUT(("  %.2x",mem[(CPU_ES << 4) + CPU_BP + i]));
		}
	}
	*/
}

#if defined(TRACE)
static int resetcnt = 0;
static int execcnt = 0;
int piccnt = 0;
int tr = 0;
UINT cflg;
#endif

void pccore_postevent(UINT32 event) { // yet!

	(void)event;
}

static void pccore_process_cpu_reset_request(void) {
	if (CPU_RESETREQ) {
		CPU_RESETREQ = 0;
		CPU_SHUT();
	}
}

#if defined(VAEG_UPD9002_M42_TESTING)
void upd9002_m42_process_cpu_reset_request(void) {
	pccore_process_cpu_reset_request();
}
#endif

void pccore_exec(BOOL draw) {
	if (upd9002_diagnostic_pending() || upd9002_debug_event_pending()) {
		return;
	}
	if (pccore_debug_resume) {
		pccore_debug_resume = FALSE;
		goto debug_resume_cpu;
	}

	drawframe = draw;
	//	keystat_sync();
	soundmng_sync();
	mouseif_sync();

	screendispflag = 1;
	if (!nevent_iswork(NEVENT_FLAMES)) {
		screendispva_setnevent();
	}

	while (screendispflag) {
#if defined(TRACE)
		resetcnt++;
#endif
		pic_irq();
		pccore_process_cpu_reset_request();

		while (CPU_REMCLOCK > 0) {
		debug_resume_cpu:
			if (upd9002_debug_step_begin()) {
				pccore_debug_resume = TRUE;
				return;
			}
			//TRACEOUT(("%.4x:%.4x", CPU_CS, CPU_IP));
			upd9002_core_step();
			if (upd9002_diagnostic_pending()) {
				return;
			}
			subsystemmx_exec();
			sgp_step();
		}

		nevent_progress();
	}
	mpu98ii_callback();
	diskdrv_callback();
	calendar_inc();
	sound_sync(); // happy!

	if (hardwarereset) {
		hardwarereset = FALSE;
		pccore_cfgupdate();
		pccore_reset();
	}

#if defined(TRACE)
	execcnt++;
	if (execcnt >= 60) {
		//		TRACEOUT(("resetcnt = %d / pic %d", resetcnt, piccnt));
		execcnt = 0;
		resetcnt = 0;
		piccnt = 0;
	}
#endif
}

void pccore_redraw(void) {
	BOOL saved_drawframe;

	saved_drawframe = drawframe;
	drawframe = TRUE;
	drawscreenva();
	drawframe = saved_drawframe;
}
