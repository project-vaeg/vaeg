/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "sdlapi.h"
#include "strres.h"
#include "np2.h"
#include "cliopts.h"
#include "dosio.h"
#include "commng.h"
#include "inputmng.h"
#include "scrnmng.h"
#include "scrndraw.h"
#include "scrndrawva.h"
#include "soundmng.h"
#include "sysmng.h"
#include "taskmng.h"
#include "sdlkbd.h"
#include "ini.h"
#include "machine/pccore.h"
#include "sgp.h"
#include "cmt.h"
#include "diskdrv.h"
#include "sxsi.h"
#include "newdisk.h"
#include "fdc.h"
#include "scsiio.h"
#include "machine/timing.h"
#include "machine/keystat.h"
#include "bkupmemva.h"
#include "emsio.h"
#include "gui/gui.h"
#include "romcheck.h"
#include "selftest.h"
#include "upd9002_trace.h"
#include "upd9002_perf.h"
#include "upd9002_diagnostic.h"
#include "diagnostics/upd9002_debug.h"
#include "dropmedia.h"
#include "hostfat_manager.h"
#include "splash.h"
#include "np2ver.h"
#include "mousemng.h"
#include "mouseifva.h"
#include "sound.h"
#include "g75_screen.h"
#include "headless_input.h"
#include "debug_harness.h"
#include <stdlib.h>
#include "opngen.h"
#include "ymfmbridge.h"
#if defined(VAEG_UPD9002_M46_TESTING)
#include "tests/upd9002/dispatch_normalization.h"
#endif
#if defined(VAEG_UPD9002_M48_TESTING)
#include "tests/upd9002/rep0f_diagnostic_stop.h"
#endif
#if defined(VAEG_UPD9002_M60A_TESTING)
#include "tests/upd9002/flags_materialization.h"
#endif
#if defined(VAEG_UPD9002_M60E_TESTING)
#include "tests/upd9002/iret_restoration.h"
#endif
#if defined(VAEG_UPD9002_M76_TESTING)
#include "tests/upd9002/brkem_upd70008.h"
#endif
#if defined(VAEG_UPD9002_M61_TESTING)
#include "tests/upd9002/mov_imm_register.h"
#endif
#if defined(VAEG_UPD9002_M62_TESTING)
#include "tests/upd9002/semantics_bundle.h"
#endif
#if defined(VAEG_UPD9002_M64_TESTING)
#include "tests/upd9002/m64_semantics.h"
#endif
#if defined(VAEG_UPD9002_M65A_TESTING)
#include "tests/upd9002/m65a_ff7.h"
#endif
#if defined(VAEG_UPD9002_M65B_TESTING)
#include "tests/upd9002/m65b_bound.h"
#endif
#if defined(VAEG_UPD9002_M65C_TESTING)
#include "tests/upd9002/m65c_f72.h"
#endif
#if defined(VAEG_UPD9002_M65D_TESTING)
#include "tests/upd9002/m65d_ff6.h"
#endif
#if defined(VAEG_UPD9002_M65E_TESTING)
#include "tests/upd9002/m65e_tail10.h"
#endif
#if defined(VAEG_UPD9002_M96D_TESTING)
#include "tests/upd9002/m96d_nop.h"
#endif
#if defined(VAEG_UPD9002_M68_TESTING)
#include "tests/upd9002/m68_segmented_memory.h"
#endif
#if defined(VAEG_UPD9002_M70_TESTING)
#include "tests/upd9002/m70_prefix_string.h"
#endif
#if defined(VAEG_IDP_M69_TESTING)
#include "tests/idp/m69_status_composition.h"
#endif
#if defined(VAEG_UPD9002_SSTS_TESTING)
#include "tests/upd9002/ssts_worker.h"
#endif

NP2OSCFG np2oscfg = {0,
                     0,
                     VAEG_DISPINFO_CPU_CLOCK | VAEG_DISPINFO_SGP_CLOCK |
                         VAEG_DISPINFO_FDD,
                     0,
                     0,
                     0,
                     1,
                     0,
                     "",
                     "",
                     {"", ""},
                     "",
                     "",
                     0,
                     0,
                     "",
                     "ymfm",
                     "minimum",
                     1,
                     VAEG_EFFECT_UNFILTERED,
                     VAEG_SCALING_FIT,
                     640,
                     422,
                     VAEG_DISPLAY_WINDOWED,
                     0,
                     0,
                     0,
                     0,
                     2,
                     0,
                     0,
                     0,
                     "",
                     0,
                     1,
                     VAEG_DEFAULT_SHADER_PRESET,
                     0,
                     {0},
                     1};
BOOL np2_debug = FALSE;

static const UINT smoke_timeout_frames = 600;
static const UINT startup_splash_ms = 1500;
static const UINT max_catchup_frames = 15;
static const char va_backup_memory_file[] = "vabkupmem.dat";
static const char va2_backup_memory_file[] = "va2bkupmem.dat";
static BOOL backup_memory_path_ready = FALSE;
static BOOL backup_memory_path_explicit = FALSE;
static const char *rompath_override;
typedef struct {
	const char *name;
	UINT32 size;
	UINT32 crc32;
	const char *sha1;
} ROMEXPECTED;

enum {
	VA1_VAROM00_SIZE = 0x80000,
	VA1_VAROM00_BANK_SIZE = 0x10000,
	VA1_VAROM00_CHECK_BANK_COUNT = 6
};

static const char va1_varom00_reference_sha1[] =
	"e7fc344b12ab0573a5229c7b43feb64bd329e57b";

/*
 * MAME src/mame/nec/pc88va.cpp ROM_START(pc88va). Per local VA1 hardware
 * readback, MAME's vafont.rom and varom00.rom identities are incorrect;
 * keep the verified readback identities below.
 */
static const ROMEXPECTED va_required_roms[] = {
    {"vafont.rom", 0x50000, 0xb40d34e4, "a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f"},
    {"vadic.rom", 0x80000, 0xf913c605, "5ba1f3578d0aaacdaf7194a80e6d520c81ae55fb"},
    {"varom00.rom", 0x80000, 0xdf7f8a74, va1_varom00_reference_sha1},
    {"varom08.rom", 0x20000, 0x154803cc, "7e6591cd465cbb35d6d3446c5a83b46d30fafe95"},
    {"varom1.rom", 0x20000, 0x0783b16a, "54536dc03238b4668c8bb76337efade001ec7826"},
    {NULL, 0, 0, NULL}};

/* The selected VA1 readback is authoritative for populated banks 0-5.
 * Banks 6 and 7 are reserved in the technical manual and are not required
 * to match this per-bank check. */
static const char va1_varom00_bank_sha1[][41] = {
    "35d37a6a1ecf70025a9d1f9a892f3a4d1f6e1d62", "da5eaedcf34259a406946984e23e80c3fbf9d1a1",
    "e3e3ed4a7e0241dcd669b07c2d7b29a4c94744c2", "67fc27525e2ced658925289b3736002320f3dcdd",
    "463e586b9911bc6fc35f79f0a0bd3a43414460a0", "1d225f958bdc4719e83873d2a66515622d6b2dc0"};

static const char va1_varom00_uncertainty_doc[] = "docs/modernization/va-rom-dump-notes.md";

/* MAME src/mame/nec/pc88va.cpp ROM_START(pc88va2), without fallback. */
static const ROMEXPECTED va2_required_roms[] = {
    {"vafont_va2.rom", 0x50000, 0xb40d34e4, "a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f"},
    {"vadic_va2.rom", 0x80000, 0xa6108f4d, "3665db538598abb45d9dfe636423e6728a812b12"},
    {"varom00_va2.rom", 0x80000, 0x98c9959a, "bcaea28c58816602ca1e8290f534360f1ca03fe8"},
    {"varom08_va2.rom", 0x20000, 0xeef6d4a0, "47e5f89f8b0ce18ff8d5d7b7aef8ca0a2a8e3345"},
    {"varom1_va2.rom", 0x20000, 0x7e767f00, "dd4f4521bfbb068f15ab3bcdb8d47c7d82b9d1d4"},
    {NULL, 0, 0, NULL}};

/* MAME lists this ROM in a disabled FDD subsystem declaration. */
static const ROMEXPECTED extra_roms[] = {
    {"vasubsys.rom", 0x2000, 0x08962850, "a9375aa480f85e1422a0e1385acb0ea170c5c2e0"},
    {NULL, 0, 0, NULL}};

typedef struct {
	UINT32 tick;
	UINT frames_executed;
	UINT frames_skipped;
	UINT max_pending;
} PACELOG;

typedef struct {
	BOOL model_sound;
	char model[sizeof(np2cfg.model)];
	char applied_model[sizeof(np2cfg.model)];
	UINT16 sound;
	UINT16 applied_sound;
	BOOL fm_backend;
	char backend[sizeof(np2oscfg.opn_backend)];
	BOOL ymfm_fidelity;
	char fidelity[sizeof(np2oscfg.ymfm_fidelity)];
	BOOL sample_rate;
	UINT16 samplingrate;
	BOOL sound_buffer;
	UINT16 delayms;
	BOOL mute;
	BYTE sound_enabled;
	BOOL cpu_multiplier;
	UINT multiple;
	BOOL sgp;
	UINT8 sgp_mode;
	UINT8 sgp_multiplier;
	UINT8 applied_sgp_mode;
	UINT8 applied_sgp_multiplier;
	BOOL nowait;
	BYTE saved_nowait;
	BOOL frame_skip;
	BYTE draw_skip;
	BOOL display_mode;
	BYTE saved_display_mode;
	UINT16 fscrn_cx;
	UINT16 fscrn_cy;
	UINT16 fullscreen_refresh;
	UINT8 fscrnmod;
	BYTE applied_display_mode;
	UINT16 applied_fscrn_cx;
	UINT16 applied_fscrn_cy;
	UINT16 applied_fullscreen_refresh;
	UINT8 applied_fscrnmod;
	BOOL effect;
	BYTE saved_effect;
	BOOL scaling;
	BYTE saved_scaling;
	BOOL controller;
	UINT8 saved_controller;
	BOOL keyboard_layout;
	char saved_keyboard_layout[sizeof(np2oscfg.keyboard_host_layout)];
	BOOL fdd[2];
	char saved_fdd[2][MAX_PATH];
	char applied_fdd[2][MAX_PATH];
	BOOL sasi[2];
	char saved_sasi[2][MAX_PATH];
	char applied_sasi[2][MAX_PATH];
	BOOL scsi[7];
	char saved_scsi[7][MAX_PATH];
	char applied_scsi[7][MAX_PATH];
} CLI_SAVED_CONFIG;

static UINT framecnt;
static UINT waitcnt;
static UINT framemax = 1;

static void usage(const char *progname) {
	printf("88VA Eternal Grafx %s\n", VAEGREL_CORE);
	printf("Usage: %s [options]\n", progname);
	printf("Machine and sound (session only):\n");
	printf("\t--model va|va2\n");
	printf("\t--fmbackend np2|ymfm\n");
	printf("\t--fmsound opn|opna\n");
	printf("\t--ymfm-fidelity minimum|medium|maximum\n");
	printf("\t--samplerate 11025|22050|44100\n");
	printf("\t--soundbuffer 40..1000\n");
	printf("\t--mute\n");
	printf("Media (session only; use none for an empty drive):\n");
	printf("\t--fdd1 path|none    --fdd2 path|none\n");
	printf("\t--fdd1-image N      --fdd2-image N  (disk N of a multi-image D88)\n");
	printf("\t--tape path         --tape-save path  (V1/V2 cassette: .cmt/.t88 in, raw out)\n");
	printf("\t--sasi1 path|none   --sasi2 path|none\n");
	printf("\t--scsi0 path|none   --scsi1 path|none\n");
	printf("\t--scsi2 path|none   --scsi3 path|none\n");
	printf("\t--scsi4 path|none   --scsi5 path|none   --scsi6 path|none\n");
	printf("\t--hostfat-dir path  read-only PC-Engine HOSTFAT snapshot\n");
	printf("\t--roms path         ROM directory override\n");
	printf("Persistence (paths are relative to the current directory):\n");
	printf("\t--cfg path | --no-cfg\n");
	printf("\t--bkupmem path | --no-bkupmem\n");
	printf("Execution (session only):\n");
	printf("\t--cpumult 1..32\n");
	printf("\t--sgp model|follow-cpu|1..16\n");
	printf("\t--nowait\n");
	printf("\t--frameskip auto|full|2|3|4\n");
	printf("Display and input (session only):\n");
	printf("\t--fullscreen | --windowed\n");
	printf("\t--effect unfiltered|linear|scanline|crt-lite\n");
	printf("\t--scaling native|fit|fit-8dot|integer|stretch\n");
	printf("\t--controller joystick|mouse\n");
	printf("\t--keyboard-layout jis|us|custom\n");
	printf("Diagnostics:\n");
	printf("\t--smoke --selftest --debug --fdctrace --scsitrace --pacelog\n");
	printf("\t--scsitrace-no-guest\n");
	printf("\t--scsitrace-compact\n");
	printf("\t--scsitrace-census [--scsitrace-census-only]\n");
	printf("\t--scsitrace-cmdreq-windows\n");
	printf("\t--scsitrace-limit 1..1000000\n");
	printf("\t--scsitrace-jitter-seed N [--scsitrace-jitter-span N]\n");
	printf("\t--trace-cpu 1..1000000\n");
	printf("\t--headless-input-script path\n");
	printf("\t\t@text TEXT; @key ctrl-c|ctrl-z|ctrl-left|ctrl-right|backspace|escape\n");
	printf("\t\tup|down|left|right|home|help|insert|delete|f1..f10|shift-f1..f10\n");
	printf("\t\tshift-up|shift-down|shift-left|shift-right; @enter; @wait N\n");
	printf("\t--debug-script path --debug-output-dir directory\n");
	printf("\t--screen-dump path (rendered BMP, or PNG for .png)\n");
	printf("\t--screenshot FRAME:PATH (completed guest frame; .bmp or .png; repeatable)\n");
	printf("\t--screen-tvram-dump path (raw TVRAM)\n");
	printf("\t--version --help [-h]\n");
	printf("Create image (no SDL session):\n");
	printf("\t--create-scsi-hdd --output path [--size-mib N | --block-count N]\n");
	printf("\t\t[--block-size 256|512|1024] [--force]\n");
}

static int create_scsi_hdd_cli(int argc, char **argv) {
	const char *output = NULL;
	UINT64 size_mib = 40;
	UINT64 block_count = 0;
	UINT64 block_size = 256;
	BOOL block_count_set = FALSE;
	BOOL size_set = FALSE;
	BOOL force = FALSE;
	int i;

	for (i = 2; i < argc; i++) {
		const char *arg = argv[i];
		const char *value;
		char *endp;
		UINT64 number;
		if (!strcmp(arg, "--force")) {
			force = TRUE;
			continue;
		}
		if (!strcmp(arg, "--help")) {
			printf(
			    "Usage: %s --create-scsi-hdd --output path \n\t[--size-mib N | --block-count N] [--block-size 256|512|1024] [--force]\n",
			    argv[0]);
			return (SUCCESS);
		}
		if ((i + 1 >= argc) || (strcmp(arg, "--output") && strcmp(arg, "--size-mib") &&
		                        strcmp(arg, "--block-count") && strcmp(arg, "--block-size"))) {
			fprintf(stderr, "Error: unknown or incomplete image-creation option: %s\n", arg);
			return (FAILURE);
		}
		value = argv[++i];
		if (!strcmp(arg, "--output")) {
			output = value;
			continue;
		}
		number = strtoull(value, &endp, 0);
		if ((endp == value) || (*endp != '\0')) {
			fprintf(stderr, "Error: invalid numeric image-creation value: %s\n", value);
			return (FAILURE);
		}
		if (!strcmp(arg, "--size-mib")) {
			size_mib = number;
			size_set = TRUE;
		} else if (!strcmp(arg, "--block-count")) {
			block_count = number;
			block_count_set = TRUE;
		} else {
			block_size = number;
		}
	}
	if (output == NULL || output[0] == '\0' || (size_set && block_count_set)) {
		fprintf(stderr, "Error: output and exactly one size form are required\n");
		return (FAILURE);
	}
	if (!block_count_set) {
		if ((block_size == 0) || (size_mib > ((UINT64)-1) / (1024ULL * 1024ULL)) ||
		    ((size_mib * 1024ULL * 1024ULL) % block_size) != 0) {
			fprintf(stderr, "Error: size is not representable by block size\n");
			return (FAILURE);
		}
		block_count = (size_mib * 1024ULL * 1024ULL) / block_size;
	}
	dosio_init();
	file_setcd("./");
	if (newdisk_vhd_create(output, block_count, (UINT16)block_size, force) != SUCCESS) {
		fprintf(stderr, "Error: SCSI image creation failed: %s\n", output);
		dosio_term();
		return (FAILURE);
	}
	{
		FILEH fh = file_open_rb(output);
		UINT64 logical_size = (fh != FILEH_INVALID) ? file_getsize64(fh) : 0;
		if (fh != FILEH_INVALID) {
			file_close(fh);
		}
		printf(
		    "created=%s\nheader_size=%u\nblock_size=%llu\nblock_count=%llu\nlogical_size=%llu\nsparse=platform-dependent\n",
		    output, (unsigned)sizeof(VHDHDR), (unsigned long long)block_size,
		    (unsigned long long)block_count, (unsigned long long)logical_size);
	}
	dosio_term();
	return (SUCCESS);
}

static void smoke_configure_va(const char *model) {
	file_cpyname(np2cfg.model, model, sizeof(np2cfg.model));
	np2cfg.baseclock = PCBASECLOCK40;
	np2cfg.multiple = 2;
	np2cfg.sgp_speed_mode = SGP_SPEED_MODEL_DEFAULT;
	np2cfg.sgp_multiplier = 1;
	np2cfg.ITF_WORK = 1;
	np2cfg.EXTMEM = EMSIO_DEFAULT_MEGABYTES;
	np2cfg.SOUND_SW = np2_default_sound_for_model(model);
}

UINT16 np2_default_sound_for_model(const char *model) {
	if (milstr_cmp(model, str_VA1) == 0) {
		return (FMBOARD_VA_OPN);
	}
	if (milstr_cmp(model, str_VA2) == 0) {
		return (FMBOARD_VA_OPNA);
	}
	return (FMBOARD_NONE);
}

BOOL np2_sound_hardware_valid(const char *model, UINT16 sound) {
	if (milstr_cmp(model, str_VA1) == 0) {
		return ((sound == FMBOARD_VA_OPN) || (sound == FMBOARD_VA_OPNA));
	}
	if (milstr_cmp(model, str_VA2) == 0) {
		return (sound == FMBOARD_VA_OPNA);
	}
	return (sound == FMBOARD_NONE);
}

UINT16 np2_sound_for_model_selection(const char *model, UINT16 sound) {
	if (np2_sound_hardware_valid(model, sound)) {
		return (sound);
	}
	return (np2_default_sound_for_model(model));
}

const char *np2_cli_boot_model(const char *value) {
	if (value == NULL) {
		return (NULL);
	}
	if (milstr_cmp(value, "va") == 0) {
		return (str_VA1);
	}
	if (milstr_cmp(value, "va2") == 0) {
		return (str_VA2);
	}
	return (NULL);
}

static BOOL config_selects_va(void) {
	return ((milstr_cmp(np2cfg.model, str_VA1) == 0) || (milstr_cmp(np2cfg.model, str_VA2) == 0));
}

static void warn_va_config_sanity(void) {
	UINT32 va_threshold;

	if (!config_selects_va()) {
		return;
	}
	va_threshold = (PCBASECLOCK40 + PCBASECLOCK25) / 2;
	if (np2cfg.baseclock < va_threshold) {
		fprintf(stderr,
		        "WARNING: PC-88VA config expects clk_base=3993600 "
		        "(current=%u); stale configs can run in the wrong "
		        "clock domain.\n",
		        np2cfg.baseclock);
	}
	if (!pccore_cpu_multiple_valid(np2cfg.multiple)) {
		fprintf(stderr,
		        "WARNING: PC-88VA CPU multiplier must be between 1 and 32 "
		        "(current=%u); runtime will use a safe fallback.\n",
		        np2cfg.multiple);
	}
	if (!np2_sound_hardware_valid(np2cfg.model, np2cfg.SOUND_SW)) {
		fprintf(stderr,
		        "WARNING: PC-88VA sound hardware is invalid for %s "
		        "(SNDboard=%03x; expected=%03x).\n",
		        np2cfg.model, np2cfg.SOUND_SW, np2_default_sound_for_model(np2cfg.model));
	}
}

static void make_rom_path(char *path, int size, const char *dir, const char *name) {
	if ((dir == NULL) || (dir[0] == '\0')) {
		file_cpyname(path, name, size);
		return;
	}
	file_cpyname(path, dir, size);
	file_setseparator(path, size);
	file_catname(path, name, size);
}

static void verify_va1_varom00_read(const char *dir) {
	char path[MAX_PATH];
	char full_sha1[41];
	ROMCHECKSUM full;
	BYTE *data;
	FILEH fh;
	UINT bank;
	UINT size;
	BOOL bank_mismatch;

	make_rom_path(path, sizeof(path), dir, "varom00.rom");
	if (romcheck_file(path, &full) != SUCCESS) {
		return;
	}
	romcheck_sha1_string(full.sha1, full_sha1);
	if (strcmp(full_sha1, va1_varom00_reference_sha1) == 0) {
		return;
	}

	fprintf(stderr,
	        "WARNING: VA varom00.rom full SHA-1 differs: path=%s "
	        "expected=%s actual=%s; checking populated banks 0-5.\n",
	        path, va1_varom00_reference_sha1, full_sha1);

	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		fprintf(stderr,
		        "WARNING: VA varom00.rom bank SHA-1 check could not open %s; "
		        "see %s\n",
		        path, va1_varom00_uncertainty_doc);
		return;
	}
	size = file_getsize(fh);
	if (size < VA1_VAROM00_SIZE) {
		file_close(fh);
		fprintf(stderr,
		        "WARNING: VA varom00.rom bank SHA-1 check skipped: %s is "
		        "%u bytes, expected at least %u; see %s\n",
		        path, size, VA1_VAROM00_SIZE, va1_varom00_uncertainty_doc);
		return;
	}

	data = (BYTE *)malloc(VA1_VAROM00_SIZE);
	if (data == NULL) {
		file_close(fh);
		fprintf(stderr,
		        "WARNING: VA varom00.rom bank SHA-1 check skipped: "
		        "out of memory; see %s\n",
		        va1_varom00_uncertainty_doc);
		return;
	}
	if (file_read(fh, data, VA1_VAROM00_SIZE) != VA1_VAROM00_SIZE) {
		file_close(fh);
		free(data);
		fprintf(stderr,
		        "WARNING: VA varom00.rom bank SHA-1 check failed to read %s; "
		        "see %s\n",
		        path, va1_varom00_uncertainty_doc);
		return;
	}
	file_close(fh);

	bank_mismatch = FALSE;
	for (bank = 0; bank < VA1_VAROM00_CHECK_BANK_COUNT; bank++) {
		ROMCHECKSUM bank_sum;
		char bank_sha1[41];

		romcheck_buffer(data + (bank * VA1_VAROM00_BANK_SIZE), VA1_VAROM00_BANK_SIZE, &bank_sum);
		romcheck_sha1_string(bank_sum.sha1, bank_sha1);
		if (strcmp(bank_sha1, va1_varom00_bank_sha1[bank]) != 0) {
			bank_mismatch = TRUE;
			fprintf(stderr,
			        "WARNING: VA varom00.rom bank %u SHA-1 differs: "
			        "expected=%s actual=%s\n",
			        bank, va1_varom00_bank_sha1[bank], bank_sha1);
		}
	}
	free(data);

	if (!bank_mismatch) {
		fprintf(stderr,
		        "WARNING: VA varom00.rom populated banks 0-5 match; "
		        "the full-image difference is outside those banks or is a "
		        "size/format difference. See %s\n",
		        va1_varom00_uncertainty_doc);
	} else {
		fprintf(stderr,
		        "WARNING: VA varom00.rom has one or more differing populated "
		        "banks; see %s\n",
		        va1_varom00_uncertainty_doc);
	}
}

static const char *rom_model_label(const char *model) {
	return ((milstr_cmp(model, str_VA1) == 0) ? "VA" : "VA2/VA3");
}

static const ROMEXPECTED *rom_expected_set(const char *model) {
	return ((milstr_cmp(model, str_VA1) == 0) ? va_required_roms : va2_required_roms);
}

static BOOL romset_complete(const char *dir, const char *model, char *missing, int missing_size) {
	int i;
	const ROMEXPECTED *required;

	if ((dir == NULL) || (dir[0] == '\0')) {
		return (FAILURE);
	}
	required = rom_expected_set(model);
	for (i = 0; required[i].name != NULL; i++) {
		char path[MAX_PATH];
		short attr;

		make_rom_path(path, sizeof(path), dir, required[i].name);
		attr = file_attr(path);
		if ((attr == (short)-1) || (attr & FILEATTR_DIRECTORY)) {
			file_cpyname(missing, path, missing_size);
			return (FAILURE);
		}
	}
	for (i = 0; extra_roms[i].name != NULL; i++) {
		char path[MAX_PATH];
		short attr;

		make_rom_path(path, sizeof(path), dir, extra_roms[i].name);
		attr = file_attr(path);
		if ((attr == (short)-1) || (attr & FILEATTR_DIRECTORY)) {
			file_cpyname(missing, path, missing_size);
			return (FAILURE);
		}
	}
	return (SUCCESS);
}

static const char *backup_memory_filename(const char *model) {
	return ((milstr_cmp(model, str_VA1) == 0) ? va_backup_memory_file : va2_backup_memory_file);
}

static void switch_backup_memory_model(const char *model) {
	const char *path;

	if (!backup_memory_path_ready || backup_memory_path_explicit) {
		return;
	}
	/* Preserve the old model before selecting and loading the new image. */
	bkupmemva_save();
	path = backup_memory_filename(model);
	bkupmemva_setpath(path);
	bkupmemva_load();
	/* Materialize a missing model-specific image for BIOS setup immediately. */
	if (file_attr(path) < 0) {
		bkupmemva_save();
	}
	if (np2_debug) {
		SDL_Log("Backup memory model switch: %s", backup_memory_filename(model));
	}
}

static void select_backup_memory_path(const VAEG_CLI_OPTIONS *options) {
	const char *path;

	backup_memory_path_ready = TRUE;
	backup_memory_path_explicit = (options->bkupmem_path != NULL);
	if (options->no_bkupmem) {
		bkupmemva_setenabled(FALSE);
		if (np2_debug) {
			SDL_Log("Backup memory persistence disabled");
		}
		return;
	}
	bkupmemva_setenabled(TRUE);
	path = (options->bkupmem_path != NULL) ? options->bkupmem_path
	                                       : backup_memory_filename(np2cfg.model);
	bkupmemva_setpath(path);
	if (np2_debug) {
		SDL_Log("Backup memory: %s", path);
	}
}

static void verify_rom(const char *dir, const char *source, const ROMEXPECTED *expected) {
	char path[MAX_PATH];
	char sha1[41];
	ROMCHECKSUM actual;

	make_rom_path(path, sizeof(path), dir, expected->name);
	if (romcheck_file(path, &actual) != SUCCESS) {
		fprintf(stderr, "WARNING: ROM checksum read failed: %s\n", path);
		return;
	}
	romcheck_sha1_string(actual.sha1, sha1);
	if (np2_debug) {
		fprintf(stderr,
		        "INFO: ROM loaded: source=%s path=%s size=%u "
		        "crc32=%08x sha1=%s\n",
		        source, path, actual.size, actual.crc32, sha1);
	}
	if ((actual.size != expected->size) || (actual.crc32 != expected->crc32) ||
	    strcmp(sha1, expected->sha1)) {
		fprintf(stderr,
		        "WARNING: ROM differs from MAME %s: %s; "
		        "expected size=%u crc32=%08x sha1=%s; "
		        "actual size=%u crc32=%08x sha1=%s\n",
		        source, path, expected->size, expected->crc32, expected->sha1, actual.size,
		        actual.crc32, sha1);
	}
}

static void verify_romset(const char *dir, const char *model) {
	const ROMEXPECTED *required;
	const char *source;
	int i;

	required = rom_expected_set(model);
	source = (milstr_cmp(model, str_VA1) == 0) ? "pc88va" : "pc88va2";
	for (i = 0; required[i].name != NULL; i++) {
		verify_rom(dir, source, &required[i]);
	}
	for (i = 0; extra_roms[i].name != NULL; i++) {
		verify_rom(dir, "pc88va extra", &extra_roms[i]);
	}
	if (milstr_cmp(model, str_VA1) == 0) {
		verify_va1_varom00_read(dir);
	}
}

static BOOL resolve_model_rompath(char *missing, int missing_size) {
	char *base;
	char primary[MAX_PATH];
	char fallback[MAX_PATH];
	char primary_missing[MAX_PATH];
	char fallback_missing[MAX_PATH];
	BOOL primary_available;

	primary[0] = '\0';
	primary_missing[0] = '\0';
	fallback_missing[0] = '\0';
	primary_available = FALSE;
	if ((rompath_override != NULL) && (rompath_override[0] != '\0')) {
		if (romset_complete(rompath_override, np2cfg.model, primary_missing,
		                    sizeof(primary_missing)) == SUCCESS) {
			file_cpyname(np2cfg.biospath, rompath_override, sizeof(np2cfg.biospath));
			verify_romset(rompath_override, np2cfg.model);
			missing[0] = '\0';
			return (SUCCESS);
		}
		file_cpyname(np2cfg.biospath, rompath_override, sizeof(np2cfg.biospath));
		file_cpyname(missing, primary_missing, missing_size);
		return (FAILURE);
	}
	base = SDL_GetBasePath();
	if (base != NULL) {
		file_cpyname(primary, base, sizeof(primary));
		SDL_free(base);
		primary_available = TRUE;
		if (romset_complete(primary, np2cfg.model, primary_missing, sizeof(primary_missing)) ==
		    SUCCESS) {
			file_cpyname(np2cfg.biospath, primary, sizeof(np2cfg.biospath));
			verify_romset(primary, np2cfg.model);
			missing[0] = '\0';
			return (SUCCESS);
		}
	}

	file_cpyname(fallback, ".", sizeof(fallback));
	if (romset_complete(fallback, np2cfg.model, fallback_missing, sizeof(fallback_missing)) ==
	    SUCCESS) {
		file_cpyname(np2cfg.biospath, fallback, sizeof(np2cfg.biospath));
		verify_romset(fallback, np2cfg.model);
		missing[0] = '\0';
		return (SUCCESS);
	}

	if (primary_available) {
		file_cpyname(np2cfg.biospath, primary, sizeof(np2cfg.biospath));
		file_cpyname(missing, primary_missing, missing_size);
	} else {
		file_cpyname(np2cfg.biospath, fallback, sizeof(np2cfg.biospath));
		file_cpyname(missing, fallback_missing, missing_size);
	}
	return (FAILURE);
}

static void report_model_rompath(BOOL result, const char *missing) {
	if (result == SUCCESS) {
		fprintf(stderr, "INFO: PC-88VA %s ROM path: %s\n", rom_model_label(np2cfg.model),
		        np2cfg.biospath);
	} else {
		const ROMEXPECTED *required;

		required = rom_expected_set(np2cfg.model);
		fprintf(stderr,
		        "WARNING: PC-88VA %s ROM set not found or incomplete; "
		        "expected root: %s; missing: %s\n",
		        rom_model_label(np2cfg.model), np2cfg.biospath,
		        (missing[0] != '\0') ? missing : required[0].name);
	}
}

BOOL np2_select_boot_model(const char *model) {
	char missing[MAX_PATH];
	BOOL result;
	BOOL changed;

	if ((milstr_cmp(model, str_VA1) != 0) && (milstr_cmp(model, str_VA2) != 0)) {
		return (FAILURE);
	}
	changed = (milstr_cmp(np2cfg.model, model) != 0);
	if (changed) {
		switch_backup_memory_model(model);
	}
	file_cpyname(np2cfg.model, model, sizeof(np2cfg.model));
	if (changed) {
		np2cfg.SOUND_SW = np2_sound_for_model_selection(model, np2cfg.SOUND_SW);
	}
	result = resolve_model_rompath(missing, sizeof(missing));
	report_model_rompath(result, missing);
	return (result);
}

static BOOL smoke_resolve_rompath(void) {
	char missing[MAX_PATH];
	BOOL result;
	const ROMEXPECTED *required;

	required = rom_expected_set(np2cfg.model);
	result = resolve_model_rompath(missing, sizeof(missing));
	if (result == SUCCESS) {
		fprintf(stderr,
		        "smoke: PC-88VA %s ROM set found at %s; "
		        "uniform-screen detector enabled\n",
		        rom_model_label(np2cfg.model), np2cfg.biospath);
		return (SUCCESS);
	}
	fprintf(stderr,
	        "smoke: PC-88VA %s ROM-less mode: expected root %s; missing %s; "
	        "uniform-screen detector disabled\n",
	        rom_model_label(np2cfg.model), np2cfg.biospath,
	        (missing[0] != '\0') ? missing : required[0].name);
	return (FAILURE);
}

static BOOL check_fdd_image(const char *path, const char *level) {
	short attr;

	attr = file_attr(path);
	if (attr == (short)-1) {
		fprintf(stderr, "%s: FDD image not found: %s\n", level, path);
		return (FAILURE);
	}
	if (attr & FILEATTR_DIRECTORY) {
		fprintf(stderr, "%s: FDD image is a directory: %s\n", level, path);
		return (FAILURE);
	}
	return (SUCCESS);
}

/* Disk within a multi-image D88 chosen on the command line, 0-based. */
static UINT cli_fdd_image[2];

static void mount_configured_fdd_images(void) {
	int drive;

	for (drive = 0; drive < 2; drive++) {
		const char *path;

		path = np2oscfg.fdd_image[drive];
		if ((path[0] != '\0') && (check_fdd_image(path, "Warning") != SUCCESS)) {
			continue;
		}
		if (path[0] == '\0') {
			continue;
		}
		diskdrv_setfddex((REG8)drive, path, 0, cli_fdd_image[drive]);
	}
}

static const char *cli_model_name(UINT model) {
	return ((model == VAEG_CLI_MODEL_VA) ? str_VA1 : str_VA2);
}

static UINT16 cli_fm_sound(UINT sound) {
	return ((sound == VAEG_CLI_FM_SOUND_OPN) ? FMBOARD_VA_OPN : FMBOARD_VA_OPNA);
}

static UINT cli_fm_backend(UINT backend) {
	return ((backend == VAEG_CLI_FM_BACKEND_NP2) ? OPN_BACKEND_NP2 : OPN_BACKEND_YMFM);
}

static UINT cli_ymfm_fidelity(UINT fidelity) {
	switch (fidelity) {
	case VAEG_CLI_FIDELITY_MEDIUM:
		return (YMFMBRIDGE_FIDELITY_MEDIUM);

	case VAEG_CLI_FIDELITY_MAXIMUM:
		return (YMFMBRIDGE_FIDELITY_MAXIMUM);

	default:
		return (YMFMBRIDGE_FIDELITY_MINIMUM);
	}
}

static UINT8 cli_sgp_mode(UINT mode) {
	switch (mode) {
	case VAEG_CLI_SGP_FOLLOW_CPU:
		return (SGP_SPEED_FOLLOW_CPU);

	case VAEG_CLI_SGP_CUSTOM:
		return (SGP_SPEED_CUSTOM);

	default:
		return (SGP_SPEED_MODEL_DEFAULT);
	}
}

static BYTE cli_frame_skip(UINT value) {
	return ((BYTE)(value - VAEG_CLI_FRAMESKIP_AUTO));
}

static BYTE cli_display_mode(UINT value) {
	return ((value == VAEG_CLI_DISPLAY_FULLSCREEN) ? VAEG_DISPLAY_EXCLUSIVE
	                                               : VAEG_DISPLAY_WINDOWED);
}

static BYTE cli_effect(UINT value) {
	return ((BYTE)(value - VAEG_CLI_EFFECT_UNFILTERED));
}

static BYTE cli_scaling(UINT value) {
	return ((BYTE)(value - VAEG_CLI_SCALING_NATIVE));
}

static UINT8 cli_controller(UINT value) {
	return ((value == VAEG_CLI_CONTROLLER_MOUSE) ? MOUSEIFVA_MOUSE : MOUSEIFVA_JOYPAD);
}

static const char *cli_keyboard_layout(UINT value) {
	switch (value) {
	case VAEG_CLI_KEYBOARD_US:
		return ("us");

	case VAEG_CLI_KEYBOARD_CUSTOM:
		return ("custom");

	default:
		return ("jis");
	}
}

static BOOL check_sasi_image(const char *path, int drive) {
	short attr;

	attr = file_attr(path);
	if (attr == (short)-1) {
		fprintf(stderr, "Error: SASI-%d image not found: %s\n", drive + 1, path);
		return (FAILURE);
	}
	if (attr & FILEATTR_DIRECTORY) {
		fprintf(stderr, "Error: SASI-%d image is a directory: %s\n", drive + 1, path);
		return (FAILURE);
	}
	if (sxsi_hddvalidate_sasi(path) != SUCCESS) {
		fprintf(stderr, "Error: SASI-%d image has an unsupported format or geometry: %s\n",
		        drive + 1, path);
		return (FAILURE);
	}
	return (SUCCESS);
}

static BOOL check_scsi_image(const char *path, int drive) {
	short attr;

	attr = file_attr(path);
	if (attr == (short)-1) {
		fprintf(stderr, "Error: SCSI ID %d image not found: %s\n", drive, path);
		return (FAILURE);
	}
	if (attr & FILEATTR_DIRECTORY) {
		fprintf(stderr, "Error: SCSI ID %d image is a directory: %s\n", drive, path);
		return (FAILURE);
	}
	if (sxsi_hddvalidate_scsi(path) != SUCCESS) {
		fprintf(stderr, "Error: SCSI ID %d image has an unsupported format or geometry: %s\n",
		        drive, path);
		return (FAILURE);
	}
	return (SUCCESS);
}

static BOOL validate_cli_options(const VAEG_CLI_OPTIONS *options) {
	const char *model;
	int drive;

	model = np2cfg.model;
	if (options->smoke) {
		model = (options->model != VAEG_CLI_MODEL_UNSET) ? cli_model_name(options->model) : str_VA2;
	} else if (options->model != VAEG_CLI_MODEL_UNSET) {
		model = cli_model_name(options->model);
	}
	if ((options->fm_sound != VAEG_CLI_FM_SOUND_UNSET) &&
	    !np2_sound_hardware_valid(model, cli_fm_sound(options->fm_sound))) {
		fprintf(stderr, "Error: --fmsound opn is not valid with --model va2\n");
		return (FAILURE);
	}
	for (drive = 0; drive < 2; drive++) {
		if ((options->fdd_mode[drive] == VAEG_CLI_MEDIA_PATH) &&
		    (check_fdd_image(options->fdd_path[drive], "Error") != SUCCESS)) {
			return (FAILURE);
		}
		if ((options->sasi_mode[drive] == VAEG_CLI_MEDIA_PATH) &&
		    (check_sasi_image(options->sasi_path[drive], drive) != SUCCESS)) {
			return (FAILURE);
		}
	}
	for (drive = 0; drive < 7; drive++) {
		if ((options->scsi_mode[drive] == VAEG_CLI_MEDIA_PATH) &&
		    (check_scsi_image(options->scsi_path[drive], drive) != SUCCESS)) {
			return (FAILURE);
		}
	}
	return (SUCCESS);
}

static void save_cli_config(const VAEG_CLI_OPTIONS *options, CLI_SAVED_CONFIG *saved) {
	int drive;

	ZeroMemory(saved, sizeof(*saved));
	saved->model_sound =
	    (options->model != VAEG_CLI_MODEL_UNSET) || (options->fm_sound != VAEG_CLI_FM_SOUND_UNSET);
	if (saved->model_sound) {
		file_cpyname(saved->model, np2cfg.model, sizeof(saved->model));
		saved->sound = np2cfg.SOUND_SW;
	}
	saved->fm_backend = options->fm_backend != VAEG_CLI_FM_BACKEND_UNSET;
	if (saved->fm_backend) {
		milstr_ncpy(saved->backend, np2oscfg.opn_backend, sizeof(saved->backend));
	}
	saved->ymfm_fidelity = options->ymfm_fidelity != VAEG_CLI_FIDELITY_UNSET;
	if (saved->ymfm_fidelity) {
		milstr_ncpy(saved->fidelity, np2oscfg.ymfm_fidelity, sizeof(saved->fidelity));
	}
	saved->sample_rate = options->sample_rate != 0;
	saved->samplingrate = np2cfg.samplingrate;
	saved->sound_buffer = options->sound_buffer != 0;
	saved->delayms = np2cfg.delayms;
	saved->mute = options->mute;
	saved->sound_enabled = np2oscfg.sound_enabled;
	saved->cpu_multiplier = options->cpu_multiplier != 0;
	saved->multiple = np2cfg.multiple;
	saved->sgp = options->sgp_mode != VAEG_CLI_SGP_UNSET;
	saved->sgp_mode = np2cfg.sgp_speed_mode;
	saved->sgp_multiplier = np2cfg.sgp_multiplier;
	saved->nowait = options->nowait;
	saved->saved_nowait = np2oscfg.NOWAIT;
	saved->frame_skip = options->frame_skip != VAEG_CLI_FRAMESKIP_UNSET;
	saved->draw_skip = np2oscfg.DRAW_SKIP;
	saved->display_mode = options->display_mode != VAEG_CLI_DISPLAY_UNSET;
	saved->saved_display_mode = np2oscfg.gui_display_mode;
	saved->fscrn_cx = np2oscfg.fscrn_cx;
	saved->fscrn_cy = np2oscfg.fscrn_cy;
	saved->fullscreen_refresh = np2oscfg.gui_fullscreen_refresh;
	saved->fscrnmod = np2oscfg.fscrnmod;
	saved->effect = options->effect != VAEG_CLI_EFFECT_UNSET;
	saved->saved_effect = np2oscfg.gui_effect;
	saved->scaling = options->scaling != VAEG_CLI_SCALING_UNSET;
	saved->saved_scaling = np2oscfg.gui_scaling;
	saved->controller = options->controller != VAEG_CLI_CONTROLLER_UNSET;
	saved->saved_controller = mouseifvacfg.device;
	saved->keyboard_layout = options->keyboard_layout != VAEG_CLI_KEYBOARD_UNSET;
	if (saved->keyboard_layout) {
		milstr_ncpy(saved->saved_keyboard_layout, np2oscfg.keyboard_host_layout,
		            sizeof(saved->saved_keyboard_layout));
	}
	for (drive = 0; drive < 2; drive++) {
		saved->fdd[drive] = options->fdd_mode[drive] != VAEG_CLI_MEDIA_UNSET;
		if (saved->fdd[drive]) {
			file_cpyname(saved->saved_fdd[drive], np2oscfg.fdd_image[drive],
			             sizeof(saved->saved_fdd[drive]));
		}
		saved->sasi[drive] = options->sasi_mode[drive] != VAEG_CLI_MEDIA_UNSET;
		if (saved->sasi[drive]) {
			file_cpyname(saved->saved_sasi[drive], np2cfg.sasihdd[drive],
			             sizeof(saved->saved_sasi[drive]));
		}
	}
	for (drive = 0; drive < 7; drive++) {
		saved->scsi[drive] = options->scsi_mode[drive] != VAEG_CLI_MEDIA_UNSET;
		if (saved->scsi[drive]) {
			file_cpyname(saved->saved_scsi[drive], np2cfg.scsihdd[drive],
			             sizeof(saved->saved_scsi[drive]));
		}
	}
}

static void apply_cli_config(const VAEG_CLI_OPTIONS *options, CLI_SAVED_CONFIG *saved) {
	UINT value;
	int drive;

	if (options->fm_sound != VAEG_CLI_FM_SOUND_UNSET) {
		np2cfg.SOUND_SW = cli_fm_sound(options->fm_sound);
	}
	if (saved->model_sound) {
		file_cpyname(saved->applied_model, np2cfg.model, sizeof(saved->applied_model));
		saved->applied_sound = np2cfg.SOUND_SW;
	}
	if (saved->fm_backend) {
		value = cli_fm_backend(options->fm_backend);
		opngen_setbackend(value);
		milstr_ncpy(np2oscfg.opn_backend, opngen_backendname(value), sizeof(np2oscfg.opn_backend));
	}
	if (saved->ymfm_fidelity) {
		value = cli_ymfm_fidelity(options->ymfm_fidelity);
		ymfm_opn_setfidelity(value);
		milstr_ncpy(np2oscfg.ymfm_fidelity, ymfm_opn_fidelityname(value),
		            sizeof(np2oscfg.ymfm_fidelity));
	}
	if (saved->sample_rate) {
		np2cfg.samplingrate = (UINT16)options->sample_rate;
	}
	if (saved->sound_buffer) {
		np2cfg.delayms = (UINT16)options->sound_buffer;
	}
	if (saved->mute) {
		np2oscfg.sound_enabled = 0;
	}
	if (saved->cpu_multiplier) {
		np2cfg.multiple = options->cpu_multiplier;
	}
	if (saved->sgp) {
		np2cfg.sgp_speed_mode = cli_sgp_mode(options->sgp_mode);
		if (options->sgp_mode == VAEG_CLI_SGP_CUSTOM) {
			np2cfg.sgp_multiplier = (UINT8)options->sgp_multiplier;
		}
		saved->applied_sgp_mode = np2cfg.sgp_speed_mode;
		saved->applied_sgp_multiplier = np2cfg.sgp_multiplier;
	}
	if (saved->nowait) {
		np2oscfg.NOWAIT = 1;
	}
	if (saved->frame_skip) {
		np2oscfg.DRAW_SKIP = cli_frame_skip(options->frame_skip);
	}
	if (saved->display_mode) {
		np2oscfg.gui_display_mode = cli_display_mode(options->display_mode);
		if (np2oscfg.gui_display_mode == VAEG_DISPLAY_EXCLUSIVE) {
			np2oscfg.fscrn_cx = 0;
			np2oscfg.fscrn_cy = 0;
			np2oscfg.gui_fullscreen_refresh = 0;
			np2oscfg.fscrnmod = (np2oscfg.fscrnmod & 3) | 4;
		}
	}
	if (saved->effect) {
		np2oscfg.gui_effect = cli_effect(options->effect);
	}
	if (saved->scaling) {
		np2oscfg.gui_scaling = cli_scaling(options->scaling);
	}
	if (saved->controller) {
		mouseifvacfg.device = cli_controller(options->controller);
	}
	if (saved->keyboard_layout) {
		milstr_ncpy(np2oscfg.keyboard_host_layout, cli_keyboard_layout(options->keyboard_layout),
		            sizeof(np2oscfg.keyboard_host_layout));
	}
	for (drive = 0; drive < 2; drive++) {
		cli_fdd_image[drive] = (options->fdd_image[drive] != 0) ? options->fdd_image[drive] - 1 : 0;
		if (saved->fdd[drive]) {
			if (options->fdd_mode[drive] == VAEG_CLI_MEDIA_PATH) {
				file_cpyname(np2oscfg.fdd_image[drive], options->fdd_path[drive],
				             sizeof(np2oscfg.fdd_image[drive]));
			} else {
				np2oscfg.fdd_image[drive][0] = '\0';
			}
			file_cpyname(saved->applied_fdd[drive], np2oscfg.fdd_image[drive],
			             sizeof(saved->applied_fdd[drive]));
		}
		if (saved->sasi[drive]) {
			if (options->sasi_mode[drive] == VAEG_CLI_MEDIA_PATH) {
				file_cpyname(np2cfg.sasihdd[drive], options->sasi_path[drive],
				             sizeof(np2cfg.sasihdd[drive]));
			} else {
				np2cfg.sasihdd[drive][0] = '\0';
			}
			file_cpyname(saved->applied_sasi[drive], np2cfg.sasihdd[drive],
			             sizeof(saved->applied_sasi[drive]));
		}
	}
	for (drive = 0; drive < 7; drive++) {
		if (saved->scsi[drive]) {
			if (options->scsi_mode[drive] == VAEG_CLI_MEDIA_PATH) {
				file_cpyname(np2cfg.scsihdd[drive], options->scsi_path[drive],
				             sizeof(np2cfg.scsihdd[drive]));
			} else {
				np2cfg.scsihdd[drive][0] = '\0';
			}
			file_cpyname(saved->applied_scsi[drive], np2cfg.scsihdd[drive],
			             sizeof(saved->applied_scsi[drive]));
		}
	}
}

static void update_applied_display(CLI_SAVED_CONFIG *saved) {
	if (!saved->display_mode) {
		return;
	}
	saved->applied_display_mode = np2oscfg.gui_display_mode;
	saved->applied_fscrn_cx = np2oscfg.fscrn_cx;
	saved->applied_fscrn_cy = np2oscfg.fscrn_cy;
	saved->applied_fullscreen_refresh = np2oscfg.gui_fullscreen_refresh;
	saved->applied_fscrnmod = np2oscfg.fscrnmod;
}

static void restore_cli_config(const VAEG_CLI_OPTIONS *options, const CLI_SAVED_CONFIG *saved) {
	int drive;

	if (saved->model_sound && !strcmp(np2cfg.model, saved->applied_model) &&
	    (np2cfg.SOUND_SW == saved->applied_sound)) {
		file_cpyname(np2cfg.model, saved->model, sizeof(np2cfg.model));
		np2cfg.SOUND_SW = saved->sound;
	}
	if (saved->fm_backend &&
	    !strcmp(np2oscfg.opn_backend, opngen_backendname(cli_fm_backend(options->fm_backend)))) {
		milstr_ncpy(np2oscfg.opn_backend, saved->backend, sizeof(np2oscfg.opn_backend));
		opngen_setbackend(opngen_parsebackend(saved->backend));
	}
	if (saved->ymfm_fidelity &&
	    !strcmp(np2oscfg.ymfm_fidelity,
	            ymfm_opn_fidelityname(cli_ymfm_fidelity(options->ymfm_fidelity)))) {
		milstr_ncpy(np2oscfg.ymfm_fidelity, saved->fidelity, sizeof(np2oscfg.ymfm_fidelity));
		ymfm_opn_setfidelity(ymfm_opn_parsefidelity(saved->fidelity));
	}
	if (saved->sample_rate && (np2cfg.samplingrate == (UINT16)options->sample_rate)) {
		np2cfg.samplingrate = saved->samplingrate;
	}
	if (saved->sound_buffer && (np2cfg.delayms == (UINT16)options->sound_buffer)) {
		np2cfg.delayms = saved->delayms;
	}
	if (saved->mute && (np2oscfg.sound_enabled == 0)) {
		np2oscfg.sound_enabled = saved->sound_enabled;
	}
	if (saved->cpu_multiplier && (np2cfg.multiple == options->cpu_multiplier)) {
		np2cfg.multiple = saved->multiple;
	}
	if (saved->sgp && (np2cfg.sgp_speed_mode == saved->applied_sgp_mode) &&
	    (np2cfg.sgp_multiplier == saved->applied_sgp_multiplier)) {
		np2cfg.sgp_speed_mode = saved->sgp_mode;
		np2cfg.sgp_multiplier = saved->sgp_multiplier;
	}
	if (saved->nowait && (np2oscfg.NOWAIT == 1)) {
		np2oscfg.NOWAIT = saved->saved_nowait;
	}
	if (saved->frame_skip && (np2oscfg.DRAW_SKIP == cli_frame_skip(options->frame_skip))) {
		np2oscfg.DRAW_SKIP = saved->draw_skip;
	}
	if (saved->display_mode && (np2oscfg.gui_display_mode == saved->applied_display_mode) &&
	    (np2oscfg.fscrn_cx == saved->applied_fscrn_cx) &&
	    (np2oscfg.fscrn_cy == saved->applied_fscrn_cy) &&
	    (np2oscfg.gui_fullscreen_refresh == saved->applied_fullscreen_refresh) &&
	    (np2oscfg.fscrnmod == saved->applied_fscrnmod)) {
		np2oscfg.gui_display_mode = saved->saved_display_mode;
		np2oscfg.fscrn_cx = saved->fscrn_cx;
		np2oscfg.fscrn_cy = saved->fscrn_cy;
		np2oscfg.gui_fullscreen_refresh = saved->fullscreen_refresh;
		np2oscfg.fscrnmod = saved->fscrnmod;
	}
	if (saved->effect && (np2oscfg.gui_effect == cli_effect(options->effect))) {
		np2oscfg.gui_effect = saved->saved_effect;
	}
	if (saved->scaling && (np2oscfg.gui_scaling == cli_scaling(options->scaling))) {
		np2oscfg.gui_scaling = saved->saved_scaling;
	}
	if (saved->controller && (mouseifvacfg.device == cli_controller(options->controller))) {
		mouseifvacfg.device = saved->saved_controller;
	}
	if (saved->keyboard_layout &&
	    !strcmp(np2oscfg.keyboard_host_layout, cli_keyboard_layout(options->keyboard_layout))) {
		milstr_ncpy(np2oscfg.keyboard_host_layout, saved->saved_keyboard_layout,
		            sizeof(np2oscfg.keyboard_host_layout));
	}
	for (drive = 0; drive < 2; drive++) {
		if (saved->fdd[drive] && !strcmp(np2oscfg.fdd_image[drive], saved->applied_fdd[drive])) {
			file_cpyname(np2oscfg.fdd_image[drive], saved->saved_fdd[drive],
			             sizeof(np2oscfg.fdd_image[drive]));
		}
		if (saved->sasi[drive] && !strcmp(np2cfg.sasihdd[drive], saved->applied_sasi[drive])) {
			file_cpyname(np2cfg.sasihdd[drive], saved->saved_sasi[drive],
			             sizeof(np2cfg.sasihdd[drive]));
		}
	}
	for (drive = 0; drive < 7; drive++) {
		if (saved->scsi[drive] && !strcmp(np2cfg.scsihdd[drive], saved->applied_scsi[drive])) {
			file_cpyname(np2cfg.scsihdd[drive], saved->saved_scsi[drive],
			             sizeof(np2cfg.scsihdd[drive]));
		}
	}
}

BOOL np2_cli_override_selftest(void) {
	NP2CFG original_cfg;
	NP2CFG baseline_cfg;
	NP2OSCFG original_oscfg;
	NP2OSCFG baseline_oscfg;
	VAEG_CLI_OPTIONS options;
	CLI_SAVED_CONFIG saved;
	UINT8 original_controller;
	UINT8 baseline_controller;
	UINT original_backend;
	UINT original_fidelity;
	BOOL result;

	original_cfg = np2cfg;
	original_oscfg = np2oscfg;
	original_controller = mouseifvacfg.device;
	original_backend = opngen_getbackend();
	original_fidelity = ymfm_opn_getfidelity();
	file_cpyname(np2cfg.model, str_VA2, sizeof(np2cfg.model));
	np2cfg.SOUND_SW = FMBOARD_VA_OPNA;
	np2cfg.samplingrate = 22050;
	np2cfg.delayms = 500;
	np2cfg.multiple = 2;
	np2cfg.sgp_speed_mode = SGP_SPEED_MODEL_DEFAULT;
	np2cfg.sgp_multiplier = 1;
	file_cpyname(np2cfg.sasihdd[0], "saved1.hdi", sizeof(np2cfg.sasihdd[0]));
	file_cpyname(np2cfg.sasihdd[1], "saved2.hdi", sizeof(np2cfg.sasihdd[1]));
	file_cpyname(np2cfg.scsihdd[0], "saved0.hdd", sizeof(np2cfg.scsihdd[0]));
	file_cpyname(np2cfg.scsihdd[1], "saved1.hdd", sizeof(np2cfg.scsihdd[1]));
	file_cpyname(np2cfg.scsihdd[2], "saved2.hdd", sizeof(np2cfg.scsihdd[2]));
	file_cpyname(np2cfg.scsihdd[3], "saved3.hdd", sizeof(np2cfg.scsihdd[3]));
	file_cpyname(np2cfg.scsihdd[4], "saved4.hdd", sizeof(np2cfg.scsihdd[4]));
	file_cpyname(np2cfg.scsihdd[5], "saved5.hdd", sizeof(np2cfg.scsihdd[5]));
	file_cpyname(np2cfg.scsihdd[6], "saved6.hdd", sizeof(np2cfg.scsihdd[6]));
	milstr_ncpy(np2oscfg.opn_backend, "ymfm", sizeof(np2oscfg.opn_backend));
	milstr_ncpy(np2oscfg.ymfm_fidelity, "minimum", sizeof(np2oscfg.ymfm_fidelity));
	np2oscfg.sound_enabled = 1;
	np2oscfg.NOWAIT = 0;
	np2oscfg.DRAW_SKIP = 0;
	np2oscfg.gui_display_mode = VAEG_DISPLAY_WINDOWED;
	np2oscfg.fscrn_cx = 640;
	np2oscfg.fscrn_cy = 480;
	np2oscfg.gui_fullscreen_refresh = 60;
	np2oscfg.fscrnmod = 2;
	np2oscfg.gui_effect = VAEG_EFFECT_UNFILTERED;
	np2oscfg.gui_scaling = VAEG_SCALING_FIT;
	milstr_ncpy(np2oscfg.keyboard_host_layout, "jis", sizeof(np2oscfg.keyboard_host_layout));
	file_cpyname(np2oscfg.fdd_image[0], "saved1.d88", sizeof(np2oscfg.fdd_image[0]));
	file_cpyname(np2oscfg.fdd_image[1], "saved2.d88", sizeof(np2oscfg.fdd_image[1]));
	mouseifvacfg.device = MOUSEIFVA_JOYPAD;
	opngen_setbackend(OPN_BACKEND_YMFM);
	ymfm_opn_setfidelity(YMFMBRIDGE_FIDELITY_MINIMUM);
	baseline_cfg = np2cfg;
	baseline_oscfg = np2oscfg;
	baseline_controller = mouseifvacfg.device;
	vaeg_cli_options_init(&options);
	options.model = VAEG_CLI_MODEL_VA;
	options.fm_backend = VAEG_CLI_FM_BACKEND_NP2;
	options.fm_sound = VAEG_CLI_FM_SOUND_OPNA;
	options.ymfm_fidelity = VAEG_CLI_FIDELITY_MAXIMUM;
	options.sample_rate = 44100;
	options.sound_buffer = 40;
	options.mute = TRUE;
	options.cpu_multiplier = 32;
	options.sgp_mode = VAEG_CLI_SGP_CUSTOM;
	options.sgp_multiplier = 16;
	options.nowait = TRUE;
	options.frame_skip = VAEG_CLI_FRAMESKIP_QUARTER;
	options.display_mode = VAEG_CLI_DISPLAY_FULLSCREEN;
	options.effect = VAEG_CLI_EFFECT_CRT_LITE;
	options.scaling = VAEG_CLI_SCALING_STRETCH;
	options.controller = VAEG_CLI_CONTROLLER_MOUSE;
	options.keyboard_layout = VAEG_CLI_KEYBOARD_US;
	options.fdd_mode[0] = VAEG_CLI_MEDIA_PATH;
	options.fdd_path[0] = "cli1.d88";
	options.fdd_mode[1] = VAEG_CLI_MEDIA_NONE;
	options.sasi_mode[0] = VAEG_CLI_MEDIA_PATH;
	options.sasi_path[0] = "cli1.hdi";
	options.sasi_mode[1] = VAEG_CLI_MEDIA_NONE;
	options.scsi_mode[0] = VAEG_CLI_MEDIA_PATH;
	options.scsi_path[0] = "cli0.hdd";
	options.scsi_mode[1] = VAEG_CLI_MEDIA_NONE;
	options.scsi_mode[2] = VAEG_CLI_MEDIA_UNSET;
	options.scsi_mode[3] = VAEG_CLI_MEDIA_UNSET;
	options.scsi_mode[4] = VAEG_CLI_MEDIA_UNSET;
	options.scsi_mode[5] = VAEG_CLI_MEDIA_UNSET;
	options.scsi_mode[6] = VAEG_CLI_MEDIA_PATH;
	options.scsi_path[6] = "cli6.hdd";
	save_cli_config(&options, &saved);
	file_cpyname(np2cfg.model, str_VA1, sizeof(np2cfg.model));
	apply_cli_config(&options, &saved);
	update_applied_display(&saved);
	result =
	    !strcmp(backup_memory_filename(str_VA1), "vabkupmem.dat") &&
	    !strcmp(backup_memory_filename(str_VA2), "va2bkupmem.dat") &&
	    (np2cfg.SOUND_SW == FMBOARD_VA_OPNA) && (opngen_getbackend() == OPN_BACKEND_NP2) &&
	    (ymfm_opn_getfidelity() == YMFMBRIDGE_FIDELITY_MAXIMUM) && (np2cfg.samplingrate == 44100) &&
	    (np2cfg.delayms == 40) && (np2oscfg.sound_enabled == 0) && (np2cfg.multiple == 32) &&
	    (np2cfg.sgp_speed_mode == SGP_SPEED_CUSTOM) && (np2cfg.sgp_multiplier == 16) &&
	    (np2oscfg.NOWAIT == 1) && (np2oscfg.DRAW_SKIP == 4) &&
	    (np2oscfg.gui_display_mode == VAEG_DISPLAY_EXCLUSIVE) &&
	    (np2oscfg.gui_effect == VAEG_EFFECT_CRT_LITE) &&
	    (np2oscfg.gui_scaling == VAEG_SCALING_STRETCH) &&
	    (mouseifvacfg.device == MOUSEIFVA_MOUSE) && !strcmp(np2oscfg.keyboard_host_layout, "us") &&
	    !strcmp(np2oscfg.fdd_image[0], "cli1.d88") && (np2oscfg.fdd_image[1][0] == '\0') &&
	    !strcmp(np2cfg.sasihdd[0], "cli1.hdi") && (np2cfg.sasihdd[1][0] == '\0') &&
	    !strcmp(np2cfg.scsihdd[0], "cli0.hdd") && (np2cfg.scsihdd[1][0] == '\0') &&
	    !strcmp(np2cfg.scsihdd[2], "saved2.hdd") && !strcmp(np2cfg.scsihdd[3], "saved3.hdd") &&
	    !strcmp(np2cfg.scsihdd[4], "saved4.hdd") && !strcmp(np2cfg.scsihdd[5], "saved5.hdd") &&
	    !strcmp(np2cfg.scsihdd[6], "cli6.hdd");
	restore_cli_config(&options, &saved);
	result = result && !memcmp(&np2cfg, &baseline_cfg, sizeof(np2cfg)) &&
	         !memcmp(&np2oscfg, &baseline_oscfg, sizeof(np2oscfg)) &&
	         (mouseifvacfg.device == baseline_controller) &&
	         (opngen_getbackend() == OPN_BACKEND_YMFM) &&
	         (ymfm_opn_getfidelity() == YMFMBRIDGE_FIDELITY_MINIMUM);
	np2cfg = original_cfg;
	np2oscfg = original_oscfg;
	mouseifvacfg.device = original_controller;
	opngen_setbackend(original_backend);
	ymfm_opn_setfidelity(original_fidelity);
	return (result);
}

static BOOL smoke_check_screen(UINT frames, BOOL *done) {
	BOOL uniform;

	*done = FALSE;
	if (scrnmng_texture_uniform(&uniform) != SUCCESS) {
		fprintf(stderr, "Error: smoke screen detector could not read guest texture\n");
		return (FAILURE);
	}
	if (!uniform) {
		*done = TRUE;
		return (SUCCESS);
	}
	if (frames >= smoke_timeout_frames) {
		fprintf(stderr,
		        "Error: smoke screen detector: guest texture is uniform "
		        "after %u frames\n",
		        frames);
		return (FAILURE);
	}
	return (SUCCESS);
}

static void pacelog_initialize(PACELOG *log) {
	ZeroMemory(log, sizeof(*log));
	log->tick = GETTICK();
}

static void pacelog_update(PACELOG *log, BOOL enabled, UINT executed, UINT skipped, UINT pending) {
	UINT32 now;

	if (!enabled) {
		return;
	}
	log->frames_executed += executed;
	log->frames_skipped += skipped;
	if (log->max_pending < pending) {
		log->max_pending = pending;
	}
	now = GETTICK();
	if ((UINT32)(now - log->tick) >= 1000) {
		fprintf(stderr,
		        "pacelog frames_executed=%u frames_skipped=%u "
		        "max_pending=%u renderer=%s\n",
		        log->frames_executed, log->frames_skipped, log->max_pending,
		        scrnmng_get_renderer_backend());
		log->tick = now;
		log->frames_executed = 0;
		log->frames_skipped = 0;
		log->max_pending = 0;
	}
}

static void framereset(UINT cnt) {
	framecnt = 0;
	(void)cnt;
}

static void service_host_idle(void) {
	taskmng_rol();
	timing_hosttick();
	if (taskmng_isavail()) {
		SDL_Delay(1);
	}
}

static void processwait(UINT cnt, PACELOG *pacelog, BOOL pacelog_enabled) {
	UINT pending;

	pending = timing_getcount();
	pacelog_update(pacelog, pacelog_enabled, 0, 0, pending);
	if (pending >= cnt) {
		timing_setcount(0);
		framereset(cnt);
	} else {
		service_host_idle();
	}
	soundmng_sync();
}

static BOOL initialize_gui_after_native_fallback(void) {
	if (!scrnmng_take_native_fallback()) {
		return scrnmng_apply_native_crt_request();
	}
	if (gui_initialize(scrnmng_get_window(), scrnmng_get_renderer(), NULL) != SUCCESS) {
		fprintf(stderr, "Error: failed to initialize SDL GUI after Native CRT fallback\n");
		return FAILURE;
	}
	return scrnmng_apply_native_crt_request();
}

static BOOL run_guest_frame(BOOL draw, UINT32 frames) {
	UPD9002_DIAGNOSTIC diagnostic;

	if (draw) {
		gui_new_frame();
	}
	for (;;) {
		pccore_exec(draw);
		if (upd9002_diagnostic_get(&diagnostic) == SUCCESS) {
			fprintf(stderr,
			        "Error: uPD9002 fail-closed diagnostic stop at %04x:%04x: "
			        "%02x 0f was not executed because its semantics are unresolved\n",
			        diagnostic.cs, diagnostic.ip, diagnostic.prefix);
			taskmng_exit();
			return FAILURE;
		}
		if (!upd9002_debug_event_pending()) {
			break;
		}
		if (debug_harness_handle_pc_event(frames) != SUCCESS) {
			fprintf(stderr, "Error: debug PC-event action failed\n");
			taskmng_exit();
			return FAILURE;
		}
		if (debug_harness_exit_requested()) {
			taskmng_exit();
			break;
		}
	}
	if (draw) {
		gui_draw();
		scrnmng_present_begin();
		gui_render();
		scrnmng_present_end();
		if (initialize_gui_after_native_fallback() != SUCCESS) {
			taskmng_exit();
			return FAILURE;
		}
	}
	scrnmng_framedisp_tick(SDL_GetTicks(), drawcount, frames);
	return SUCCESS;
}

static BOOL capture_screenshot_request(UINT32 frame, const VAEG_SCREENSHOT_REQUEST *request,
                                       void *opaque) {
	(void)opaque;
	if ((request == NULL) || (scrnmng_save_rendered_frame(request->path) != SUCCESS)) {
		fprintf(stderr, "Error: screenshot capture failed at completed guest frame %u\n", frame);
		return FAILURE;
	}
	fprintf(stderr, "screenshot frame=%u path=%s\n", frame, request->path);
	return SUCCESS;
}

static BOOL screenshot_frame_needs_draw(const VAEG_SCREENSHOT_SCHEDULER *scheduler,
                                         UINT32 frame) {
	return vaeg_screenshot_scheduler_wants_frame(scheduler, frame);
}

static void render_host_ui_only(void) {
	gui_new_frame();
	gui_draw();
	scrnmng_present_begin();
	gui_render();
	scrnmng_present_end();
	if (initialize_gui_after_native_fallback() != SUCCESS) {
		taskmng_exit();
	}
}

static BOOL smoke_after_frame(BOOL smoke, UINT frames, BOOL detect_screen, BOOL headless_input,
                              HEADLESS_INPUT_SCRIPT *input_script,
                              VAEG_SCREENSHOT_SCHEDULER *screenshots) {
	BOOL done;
	BOOL ret;

	if (headless_input && (headless_input_script_after_frame(input_script, frames) != SUCCESS)) {
		taskmng_exit();
		return (FAILURE);
	}
	if (debug_harness_active() && (debug_harness_after_frame(frames) != SUCCESS)) {
		fprintf(stderr, "Error: debug frame action failed\n");
		taskmng_exit();
		return (FAILURE);
	}
	/* Completed-frame order is deterministic: automation, screenshots, then exit checks. */
	if ((screenshots != NULL) &&
	    (vaeg_screenshot_scheduler_after_frame(screenshots, frames, capture_screenshot_request,
	                                            NULL) != SUCCESS)) {
		taskmng_exit();
		return (FAILURE);
	}
	if (debug_harness_exit_requested()) {
		taskmng_exit();
	}
	if ((screenshots != NULL) && vaeg_screenshot_scheduler_done(screenshots)) {
		taskmng_exit();
	}
	if (!smoke) {
		return (SUCCESS);
	}
	if (!detect_screen) {
		if (frames >= 1) {
			taskmng_exit();
		}
		return (SUCCESS);
	}
	ret = smoke_check_screen(frames, &done);
	if ((ret != SUCCESS) || done) {
		taskmng_exit();
		return (ret);
	}
	return (SUCCESS);
}

static BOOL runloop(BOOL smoke, BOOL pacelog_enabled, BOOL detect_screen, BOOL headless_input,
                    HEADLESS_INPUT_SCRIPT *input_script,
                    VAEG_SCREENSHOT_SCHEDULER *screenshots) {
	UINT frames;
	PACELOG pacelog;
	UINT32 next_guest_tick;
	UINT32 harness_started;

	frames = 0;
	framecnt = 0;
	waitcnt = 0;
	framemax = 1;
	next_guest_tick = 0;
	harness_started = SDL_GetTicks();
	pacelog_initialize(&pacelog);
	if (headless_input) {
		headless_input_script_initialize(input_script);
	}
	while (taskmng_isavail()) {
		BOOL effective_nowait;
		UINT effective_drawskip;

		taskmng_rol();
		if (taskmng_ispaused()) {
			render_host_ui_only();
			SDL_Delay(16);
			continue;
		}
		if (g75_screen_harness_exit_requested(SDL_GetTicks() - harness_started)) {
			taskmng_exit();
			break;
		}
		if (scsiio_trace_stop_requested()) {
			taskmng_exit();
			break;
		}
		if ((np2oscfg.pacing_ms != 0) && ((SINT32)(next_guest_tick - SDL_GetTicks()) > 0)) {
			render_host_ui_only();
			SDL_Delay(1);
			continue;
		}
		timing_hosttick();
		effective_nowait = taskmng_effective_nowait(np2oscfg.NOWAIT ? TRUE : FALSE);
		effective_drawskip = taskmng_effective_drawskip(np2oscfg.DRAW_SKIP);
		if (effective_nowait) {
			BOOL draw;

			draw = screenshot_frame_needs_draw(screenshots, frames + 1) ||
			       (headless_input ? FALSE : (framecnt == 0));
			if (run_guest_frame(draw, frames) != SUCCESS) {
				return FAILURE;
			}
			next_guest_tick = SDL_GetTicks() + np2oscfg.pacing_ms;
			frames++;
			pacelog_update(&pacelog, pacelog_enabled, 1, draw ? 0 : 1, 0);
			if (smoke_after_frame(smoke, frames, detect_screen, headless_input, input_script,
			                       screenshots) !=
			    SUCCESS) {
				return (FAILURE);
			}
			if (effective_drawskip) {
				framecnt++;
				if (framecnt >= effective_drawskip) {
					processwait(0, &pacelog, pacelog_enabled);
				}
			} else {
				UINT cnt;

				framecnt = 1;
				cnt = timing_getcount();
				pacelog_update(&pacelog, pacelog_enabled, 0, 0, cnt);
				if (cnt) {
					processwait(0, &pacelog, pacelog_enabled);
				}
			}
		} else if (effective_drawskip) {
			if (framecnt < effective_drawskip) {
				BOOL draw;

				draw = screenshot_frame_needs_draw(screenshots, frames + 1) ||
				       (headless_input ? FALSE : (framecnt == 0));
				if (run_guest_frame(draw, frames) != SUCCESS) {
					return FAILURE;
				}
				next_guest_tick = SDL_GetTicks() + np2oscfg.pacing_ms;
				frames++;
				pacelog_update(&pacelog, pacelog_enabled, 1, draw ? 0 : 1, 0);
				if (smoke_after_frame(smoke, frames, detect_screen, headless_input, input_script,
				                       screenshots) !=
				    SUCCESS) {
					return (FAILURE);
				}
				framecnt++;
			} else {
				processwait(effective_drawskip, &pacelog, pacelog_enabled);
			}
		} else {
			if (!waitcnt) {
				BOOL draw;
				UINT cnt;

				draw = screenshot_frame_needs_draw(screenshots, frames + 1) ||
				       (headless_input ? FALSE : (framecnt == 0));
				if (run_guest_frame(draw, frames) != SUCCESS) {
					return FAILURE;
				}
				next_guest_tick = SDL_GetTicks() + np2oscfg.pacing_ms;
				frames++;
				pacelog_update(&pacelog, pacelog_enabled, 1, draw ? 0 : 1, 0);
				if (smoke_after_frame(smoke, frames, detect_screen, headless_input, input_script,
				                       screenshots) !=
				    SUCCESS) {
					return (FAILURE);
				}
				framecnt++;
				cnt = timing_getcount();
				pacelog_update(&pacelog, pacelog_enabled, 0, 0, cnt);
				if (framecnt > cnt) {
					waitcnt = framecnt;
					if (framemax > 1) {
						framemax--;
					}
				} else if (framecnt >= framemax) {
					if (framemax < 12) {
						framemax++;
					}
					if (cnt >= max_catchup_frames) {
						timing_reset();
					} else {
						timing_setcount(cnt - framecnt);
					}
					framereset(0);
				}
			} else {
				processwait(waitcnt, &pacelog, pacelog_enabled);
				waitcnt = framecnt;
			}
		}
	}
	return (SUCCESS);
}

static void wait_startup_splash(UINT32 started) {
	while (taskmng_isavail()) {
		UINT32 elapsed;
		UINT32 remaining;

		elapsed = GETTICK() - started;
		if (elapsed >= startup_splash_ms) {
			break;
		}
		taskmng_rol();
		remaining = startup_splash_ms - elapsed;
		SDL_Delay((remaining < 10) ? remaining : 10);
	}
}

int main(int argc, char **argv) {
	BOOL smoke_detect_screen;
	BOOL splash_visible;
	BOOL run_ok;
	UINT32 splash_started;
	VAEG_VIEWPORT startup_viewport;
	HEADLESS_INPUT_SCRIPT input_script;
	VAEG_CLI_OPTIONS options;
	CLI_SAVED_CONFIG saved_cli;
	VAEG_SCREENSHOT_SCHEDULER screenshots;
	char cli_error[256];
	const char *cli_model;

#if defined(VAEG_UPD9002_M46_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m46-dispatch-qa")) {
		return upd9002_dispatch_normalization_main();
	}
#endif
#if defined(VAEG_UPD9002_M48_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m48-rep0f-diagnostic")) {
		return upd9002_rep0f_diagnostic_stop_main();
	}
#endif
#if defined(VAEG_UPD9002_M60A_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m60a-flags-materialization")) {
		return upd9002_flags_materialization_main();
	}
#endif
#if defined(VAEG_UPD9002_M60E_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m60e-iret-restoration")) {
		return upd9002_iret_restoration_main();
	}
#endif
#if defined(VAEG_UPD9002_M76_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-brkem-upd70008")) {
		return upd9002_brkem_upd70008_main();
	}
#endif
#if defined(VAEG_UPD9002_M61_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m61-mov-imm-register")) {
		return upd9002_mov_imm_register_main();
	}
#endif
#if defined(VAEG_UPD9002_M62_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m62-semantics-bundle")) {
		return upd9002_semantics_bundle_main();
	}
#endif
#if defined(VAEG_UPD9002_M64_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m64-semantics")) {
		return upd9002_m64_semantics_main();
	}
#endif
#if defined(VAEG_UPD9002_M65A_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m65a-ff7")) {
		return upd9002_m65a_ff7_main();
	}
#endif
#if defined(VAEG_UPD9002_M65B_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m65b-bound")) {
		return upd9002_m65b_bound_main();
	}
#endif
#if defined(VAEG_UPD9002_M65C_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m65c-f72")) {
		return upd9002_m65c_f72_main();
	}
#endif
#if defined(VAEG_UPD9002_M65D_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m65d-ff6")) {
		return upd9002_m65d_ff6_main();
	}
#endif
#if defined(VAEG_UPD9002_M65E_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m65e-tail10")) {
		return upd9002_m65e_tail10_main();
	}
#endif
#if defined(VAEG_UPD9002_M96D_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m96d-nop")) {
		return upd9002_m96d_nop_main();
	}
#endif
#if defined(VAEG_UPD9002_M68_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m68-segmented-memory")) {
		return upd9002_m68_segmented_memory_main();
	}
#endif
#if defined(VAEG_IDP_M69_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--idp-m69-status-composition")) {
		return idp_m69_status_composition_main();
	}
#endif
#if defined(VAEG_UPD9002_M70_TESTING)
	if ((argc == 2) && !strcmp(argv[1], "--upd9002-m70-prefix-string")) {
		return upd9002_m70_prefix_string_main();
	}
#endif
#if defined(VAEG_UPD9002_SSTS_TESTING)
	if ((argc > 1) && !strcmp(argv[1], "--upd9002-ssts-worker")) {
		return upd9002_ssts_worker_main(argc - 1, argv + 1);
	}
#endif

	if ((argc > 1) && !strcmp(argv[1], "--create-scsi-hdd")) {
		return (create_scsi_hdd_cli(argc, argv));
	}

	smoke_detect_screen = FALSE;
	splash_visible = FALSE;
	run_ok = SUCCESS;
	splash_started = 0;
	cli_model = NULL;
	ZeroMemory(&input_script, sizeof(input_script));
	if (vaeg_cli_parse(argc, argv, &options, cli_error, sizeof(cli_error)) != SUCCESS) {
		fprintf(stderr, "Error: %s\n", cli_error);
		fprintf(stderr, "Try '%s --help' for available options.\n", argv[0]);
		return (FAILURE);
	}
	if (options.help) {
		usage(argv[0]);
		return (SUCCESS);
	}
	if (options.version) {
		printf("88VA Eternal Grafx %s\n", VAEGREL_CORE);
		return (SUCCESS);
	}
	if (options.screenshot_count != 0) {
		vaeg_screenshot_scheduler_init(&screenshots, options.screenshot_requests,
		                               options.screenshot_count);
	}
	if (options.smoke && (options.screenshot_count != 0)) {
		fprintf(stderr, "Error: --screenshot cannot be combined with --smoke\n");
		return (FAILURE);
	}
	if (((options.debug_script == NULL) != (options.debug_output_dir == NULL)) ||
	    ((options.debug_script != NULL) &&
	     ((options.trace_cpu != 0) || (options.headless_input_script != NULL)))) {
		fprintf(stderr, "Error: --debug-script requires --debug-output-dir and cannot be "
		                "combined with --trace-cpu or --headless-input-script\n");
		return (FAILURE);
	}
	if ((options.headless_input_script != NULL) || (options.debug_script != NULL)) {
		options.mute = TRUE;
		options.nowait = TRUE;
		SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
		SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	}
	np2_debug = options.debug;

	SDL_SetMainReady();
#if defined(_WIN32)
	SDL_SetHintWithPriority(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2", SDL_HINT_DEFAULT);
#endif
	if (SDL_Init(0) < 0) {
		fprintf(stderr, "Error: SDL_Init: %s\n", SDL_GetError());
		return (FAILURE);
	}
	if (options.screen_dump_path != NULL) {
		(void)SDL_setenv("VAEG_SCREEN_DUMP", options.screen_dump_path, 1);
	}
	if (options.screen_tvram_dump_path != NULL) {
		(void)SDL_setenv("VAEG_SCREEN_TVRAM_DUMP", options.screen_tvram_dump_path, 1);
	}

	dosio_init();
	file_setcd("./");
	initsetpath(options.config_path);
	initsetenabled(options.no_config ? FALSE : TRUE);
	rompath_override = options.roms_path;
	if (options.trace_cpu != 0) {
		upd9002_trace_start(stderr, options.trace_cpu);
	}
	if (options.scsitrace && options.scsitrace_guest) {
		if (options.scsitrace_cmdreq_windows) {
			upd9002_guest_trace_start_cmdreq_windows(stderr);
		} else {
			upd9002_guest_trace_start(stderr);
		}
	}
	upd9002_perf_start_from_env();
	if (options.selftest) {
		run_ok = vaeg_selftest_run();
		upd9002_perf_stop();
		upd9002_guest_trace_stop();
		upd9002_trace_stop();
		SDL_Quit();
		dosio_term();
		return (run_ok);
	}
	initload();
	timing_setspeed(np2oscfg.speed_percent);
	if (validate_cli_options(&options) != SUCCESS) {
		SDL_Quit();
		dosio_term();
		return (FAILURE);
	}
	if ((options.headless_input_script != NULL) &&
	    (headless_input_script_load(&input_script, options.headless_input_script) != SUCCESS)) {
		SDL_Quit();
		dosio_term();
		return (FAILURE);
	}
	if ((options.debug_script != NULL) &&
	    (debug_harness_load(options.debug_script, options.debug_output_dir) != SUCCESS)) {
		headless_input_script_clear(&input_script);
		SDL_Quit();
		dosio_term();
		return (FAILURE);
	}
	if (hostfat_manager_initialize() != SUCCESS) {
		fprintf(stderr, "Error: cannot initialize HOSTFAT manager: %s\n", SDL_GetError());
		debug_harness_clear();
		headless_input_script_clear(&input_script);
		SDL_Quit();
		dosio_term();
		return (FAILURE);
	}
	const char *hostfat_path = options.hostfat_path;
	const BOOL hostfat_configured = (hostfat_path == NULL) && (np2oscfg.hostfat_enabled != 0);
	if ((hostfat_path == NULL) && np2oscfg.hostfat_enabled) {
		if (np2oscfg.hostfat_dir[0] == '\0') {
			fprintf(stderr, "Warning: HOSTFAT is enabled but HOSTFATDIR is empty; "
			                "disabling HOSTFAT for recovery\n");
			np2oscfg.hostfat_enabled = 0;
			sysmng_update(SYS_UPDATEOSCFG);
			if (!options.smoke) {
				initsave();
			}
		} else {
			hostfat_path = np2oscfg.hostfat_dir;
		}
	}
	if (hostfat_path != NULL) {
		HOSTFAT_SNAPSHOT_INFO hostfat_info;
		char hostfat_error[256];

		if (hostfat_manager_mount_startup(hostfat_path, &hostfat_info, hostfat_error,
		                                  sizeof(hostfat_error)) != SUCCESS) {
			fprintf(stderr, "Error: cannot create HOSTFAT snapshot: %s\n", hostfat_error);
			if (!hostfat_configured) {
				hostfat_manager_shutdown();
				headless_input_script_clear(&input_script);
				SDL_Quit();
				dosio_term();
				return (FAILURE);
			}
			np2oscfg.hostfat_enabled = 0;
			sysmng_update(SYS_UPDATEOSCFG);
			if (!options.smoke) {
				initsave();
			}
			fprintf(stderr, "Warning: disabling HOSTFAT in configuration for recovery\n");
		} else {
			fprintf(stderr,
			        "HOSTFAT: read-only snapshot ready: %u files, %u directories, "
			        "%llu source bytes, digest %08x\n",
			        hostfat_info.files, hostfat_info.directories,
			        (unsigned long long)hostfat_info.source_bytes, hostfat_info.digest);
		}
	}
	save_cli_config(&options, &saved_cli);
	dropmedia_initialize();
	dropmedia_set_session_fdd_references(saved_cli.fdd[0] ? saved_cli.saved_fdd[0] : NULL,
	                                     saved_cli.fdd[1] ? saved_cli.saved_fdd[1] : NULL);
	if (options.model != VAEG_CLI_MODEL_UNSET) {
		cli_model = cli_model_name(options.model);
	}
	if (options.smoke) {
		smoke_configure_va((cli_model != NULL) ? cli_model : str_VA2);
		apply_cli_config(&options, &saved_cli);
		smoke_detect_screen = (smoke_resolve_rompath() == SUCCESS);
	} else {
		if (cli_model != NULL) {
			(void)np2_select_boot_model(cli_model);
			if (np2_debug) {
				fprintf(stderr,
				        "INFO: CLI boot model override: %s "
				        "(session only)\n",
				        (milstr_cmp(cli_model, str_VA1) == 0) ? "va" : "va2");
			}
		} else {
			char missing[MAX_PATH];
			BOOL result;

			result = resolve_model_rompath(missing, sizeof(missing));
			report_model_rompath(result, missing);
		}
		apply_cli_config(&options, &saved_cli);
	}
	select_backup_memory_path(&options);
	warn_va_config_sanity();
	if (np2_debug) {
		fprintf(stderr,
		        "INFO: Machine config: pc_model=%s clk_base=%u "
		        "clk_mult=%u Main_RAM=%u SNDboard=%03x sound_enabled=%u biospath=%s\n",
		        np2cfg.model, np2cfg.baseclock, np2cfg.multiple, pccore_mainram_kb(),
		        np2cfg.SOUND_SW, np2oscfg.sound_enabled, np2cfg.biospath);
		fprintf(stderr,
		        "INFO: Runtime config: opn_backend=%s ymfm_fidelity=%s "
		        "SampleHz=%u Latencys=%u sgp_mode=%u sgp_mult=%u\n",
		        np2oscfg.opn_backend, np2oscfg.ymfm_fidelity, np2cfg.samplingrate, np2cfg.delayms,
		        np2cfg.sgp_speed_mode, np2cfg.sgp_multiplier);
		fprintf(stderr,
		        "INFO: Media config: FDD1=%s FDD2=%s SASI1=%s SASI2=%s "
		        "SCSI0=%s SCSI1=%s SCSI2=%s SCSI3=%s "
		        "SCSI4=%s SCSI5=%s SCSI6=%s\n",
		        np2oscfg.fdd_image[0], np2oscfg.fdd_image[1], np2cfg.sasihdd[0], np2cfg.sasihdd[1],
		        np2cfg.scsihdd[0], np2cfg.scsihdd[1], np2cfg.scsihdd[2], np2cfg.scsihdd[3],
		        np2cfg.scsihdd[4], np2cfg.scsihdd[5], np2cfg.scsihdd[6]);
	}
	if (options.smoke) {
		np2oscfg.NOWAIT = 1;
		np2oscfg.resume = 0;
	}

	TRACEINIT();
	fdc_trace_enable(options.fdctrace);
	scsiio_trace_enable(options.scsitrace);
	scsiio_trace_compact(options.scsitrace_compact);
	scsiio_trace_census_only(options.scsitrace_census_only);
	scsiio_trace_limit(options.scsitrace_limit);
	scsiio_trace_jitter(options.scsitrace_jitter, options.scsitrace_jitter_seed,
	                    options.scsitrace_jitter_span);
	sdlkbd_initialize();
	inputmng_init();
	keystat_initialize();

	scrnmng_initialize();
	scrnmng_set_display(np2oscfg.gui_scale, np2oscfg.gui_aspect);
	scrnmng_set_scaling(np2oscfg.gui_scaling);
	scrnmng_set_effect(np2oscfg.gui_effect);
	SDL_Log("Display config: window=%ux%u mode=%u monitor=%d scaling=%u effect=%u",
	        np2oscfg.gui_window_width, np2oscfg.gui_window_height, np2oscfg.gui_display_mode,
	        np2oscfg.gui_monitor, np2oscfg.gui_scaling, np2oscfg.gui_effect);
	if (scrnmng_create(np2oscfg.gui_window_width, np2oscfg.gui_window_height) != SUCCESS) {
		goto np2main_err2;
	}
	scrnmng_set_framedisp((np2oscfg.DISPCLK & VAEG_DISPINFO_FRAME) ? TRUE : FALSE);
	/* Dear ImGui has a native-presenter backend as well as the SDL renderer
	 * backend. Initialize it for every presentation path; otherwise a Windows
	 * NativeCRT session has no menu even when --windowed is used. */
	if (gui_initialize(scrnmng_get_window(), scrnmng_get_renderer(), argv[0]) != SUCCESS) {
		if (!scrnmng_native_active() || scrnmng_fallback_to_sdl() != SUCCESS ||
		    gui_initialize(scrnmng_get_window(), scrnmng_get_renderer(), argv[0]) != SUCCESS) {
			goto np2main_err3;
		}
		(void)scrnmng_take_native_fallback();
	}
	if ((np2oscfg.gui_display_mode != VAEG_DISPLAY_WINDOWED) &&
	    (scrnmng_set_display_mode(np2oscfg.gui_display_mode, np2oscfg.gui_monitor,
	                              np2oscfg.fscrn_cx, np2oscfg.fscrn_cy,
	                              np2oscfg.gui_fullscreen_refresh, np2oscfg.fscrnmod) != SUCCESS)) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Saved fullscreen mode failed; using Windowed");
		np2oscfg.gui_display_mode = VAEG_DISPLAY_WINDOWED;
	}
	if (scrnmng_get_viewport(&startup_viewport) != SUCCESS) {
		fprintf(stderr, "Error: cannot calculate a valid startup display viewport\n");
		goto np2main_err3;
	}
	update_applied_display(&saved_cli);
	scrnmng_show();
	SDL_PumpEvents();
	if ((!options.smoke) && (splash_show() == SUCCESS)) {
		splash_visible = TRUE;
		splash_started = GETTICK();
	}

	soundmng_initialize();
	soundmng_setenabled(np2oscfg.sound_enabled ? TRUE : FALSE);
	commng_initialize();
	sysmng_initialize();
	taskmng_initialize();
	pccore_init();
	bkupmemva_load();
	if (splash_visible) {
		wait_startup_splash(splash_started);
	}

	if (taskmng_isavail()) {
		pccore_reset();
		sdlkbd_reset_state();
		scrndrawva_redraw();
		mount_configured_fdd_images();
		if ((options.tape_path != NULL) && (cmt_open(options.tape_path) != SUCCESS)) {
			fprintf(stderr, "WARNING: cassette tape image not loaded: %s\n", options.tape_path);
		}
		if (options.tape_save_path != NULL) {
			cmt_save_begin(options.tape_save_path);
		}
		dropmedia_prune_storage();
		if ((debug_harness_initialize() != SUCCESS) || (debug_harness_after_frame(0) != SUCCESS)) {
			fprintf(stderr, "Error: debug initialization action failed\n");
			run_ok = FAILURE;
			taskmng_exit();
		} else {
			run_ok = runloop(options.smoke, options.pacelog, smoke_detect_screen,
			                 options.headless_input_script != NULL, &input_script,
			                 (options.screenshot_count != 0) ? &screenshots : NULL);
		}
		g75_screen_capture();
	}

	debug_harness_clear();
	headless_input_script_clear(&input_script);
	pccore_cfgupdate();
	restore_cli_config(&options, &saved_cli);
	if ((!options.smoke) &&
	    ((sys_updates & (SYS_UPDATECFG | SYS_UPDATEOSCFG)) || initcrtchanged())) {
		initsave();
	}
	bkupmemva_save();
	pccore_term();
	scsiio_trace_census_report();
	hostfat_manager_shutdown();
	dropmedia_shutdown();
	soundmng_deinitialize();
	mousemng_shutdown();
	gui_shutdown();
	scrnmng_destroy();
	TRACETERM();
	upd9002_perf_stop();
	upd9002_trace_stop();
	upd9002_guest_trace_stop();
	SDL_Quit();
	dosio_term();
	return (run_ok);

np2main_err3:
	gui_shutdown();
	scrnmng_destroy();

np2main_err2:
	debug_harness_clear();
	headless_input_script_clear(&input_script);
	hostfat_manager_shutdown();
	TRACETERM();
	upd9002_perf_stop();
	upd9002_trace_stop();
	upd9002_guest_trace_stop();
	SDL_Quit();
	dosio_term();
	return (FAILURE);
}
