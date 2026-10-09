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
#include "profile.h"
#include "np2.h"
#include "dosio.h"
#include "ini.h"
#include "librashader/shader_config.h"
#include "machine/pccore.h"
#include "sound.h"
#include "soundopts.h"
#include "opngen.h"
#include "ymfmbridge.h"
#include "sgp.h"
#include "scrnmng.h"
#include "mouseifva.h"
#include "bmsio.h"
#include "emsio.h"

typedef struct {
	const char *title;
	const INITBL *tbl;
	const INITBL *tblterm;
	UINT count;
} _INIARG, *INIARG;

static void inirdarg8(BYTE *dst, int dsize, const char *src) {
	int i;
	BYTE val;
	BOOL set;
	char c;

	for (i = 0; i < dsize; i++) {
		val = 0;
		set = FALSE;
		while (*src == ' ') {
			src++;
		}
		while (1) {
			c = *src;
			if ((c == '\0') || (c == ' ')) {
				break;
			} else if ((c >= '0') && (c <= '9')) {
				val <<= 4;
				val += c - '0';
				set = TRUE;
			} else {
				c |= 0x20;
				if ((c >= 'a') && (c <= 'f')) {
					val <<= 4;
					val += c - 'a' + 10;
					set = TRUE;
				}
			}
			src++;
		}
		if (set == FALSE) {
			break;
		}
		dst[i] = val;
	}
}

static BOOL inireadcb(void *arg, const char *para, const char *key, const char *data) {
	const INITBL *p;

	if (arg == NULL) {
		return (FAILURE);
	}
	if (milstr_cmp(para, ((INIARG)arg)->title)) {
		return (SUCCESS);
	}
	p = ((INIARG)arg)->tbl;
	while (p < ((INIARG)arg)->tblterm) {
		if (!milstr_cmp(key, p->item)) {
			switch (p->itemtype) {
			case INITYPE_STR:
				milstr_ncpy((char *)p->value, data, p->size);
				break;

			case INITYPE_BOOL:
				*((BYTE *)p->value) = (!milstr_cmp(data, str_true)) ? 1 : 0;
				break;

			case INITYPE_BYTEARG:
				inirdarg8((BYTE *)p->value, p->size, data);
				break;

			case INITYPE_SINT8:
			case INITYPE_UINT8:
				*((BYTE *)p->value) = (BYTE)milstr_solveINT(data);
				break;

			case INITYPE_SINT16:
			case INITYPE_UINT16:
				*((UINT16 *)p->value) = (UINT16)milstr_solveINT(data);
				break;

			case INITYPE_SINT32:
			case INITYPE_UINT32:
				*((UINT32 *)p->value) = (UINT32)milstr_solveINT(data);
				break;

			case INITYPE_HEX8:
				*((BYTE *)p->value) = (BYTE)milstr_solveHEX(data);
				break;

			case INITYPE_HEX16:
				*((UINT16 *)p->value) = (UINT16)milstr_solveHEX(data);
				break;

			case INITYPE_HEX32:
				*((UINT32 *)p->value) = (UINT32)milstr_solveHEX(data);
				break;
			}
		}
		p++;
	}
	return (SUCCESS);
}

void ini_read(const char *path, const char *title, const INITBL *tbl, UINT count) {
	_INIARG iniarg;

	if (path == NULL) {
		return;
	}
	iniarg.title = title;
	iniarg.tbl = tbl;
	iniarg.tblterm = tbl + count;
	profile_enum(path, &iniarg, inireadcb);
}

// ----

static void iniwrsetstr(char *work, int size, const char *ptr) {
	int i;
	char c;

	if (ptr[0] == ' ') {
		goto iwss_extend;
	}
	i = strlen(ptr);
	if ((i) && (ptr[i - 1] == ' ')) {
		goto iwss_extend;
	}
	while (i > 0) {
		i--;
		if (ptr[i] == '\"') {
			goto iwss_extend;
		}
	}
	milstr_ncpy(work, ptr, size);
	return;

iwss_extend:
	if (size > 3) {
		size -= 3;
		*work++ = '\"';
		while (size > 0) {
			size--;
			c = *ptr++;
			if (c == '\"') {
				if (size > 0) {
					size--;
					work[0] = c;
					work[1] = c;
					work += 2;
				}
			} else {
				*work++ = c;
			}
		}
		work[0] = '\"';
		work[1] = '\0';
	}
}

static void iniwrsetarg8(char *work, int size, const BYTE *ptr, int arg) {
	int i;
	char tmp[8];

	if (arg > 0) {
		SPRINTF(tmp, "%.2x", ptr[0]);
		milstr_ncpy(work, tmp, size);
	}
	for (i = 1; i < arg; i++) {
		SPRINTF(tmp, " %.2x", ptr[i]);
		milstr_ncat(work, tmp, size);
	}
}

int ini_write(const char *path, const char *title, const INITBL *tbl, UINT count) {
	FILEH fh;
	const INITBL *p;
	const INITBL *pterm;
	BOOL set;
	int result = SUCCESS;
	char work[NP2OSCFG_KEYBOARD_CUSTOM_MAP_SIZE + 512];

	fh = file_create(path);
	if (fh == FILEH_INVALID) {
		return FAILURE;
	}
	milstr_ncpy(work, "[", sizeof(work));
	milstr_ncat(work, title, sizeof(work));
	milstr_ncat(work, "]\r\n", sizeof(work));
	if (file_write(fh, work, strlen(work)) != strlen(work)) {
		result = FAILURE;
	}

	p = tbl;
	pterm = tbl + count;
	while (p < pterm) {
		work[0] = '\0';
		set = SUCCESS;
		switch (p->itemtype) {
		case INITYPE_STR:
			iniwrsetstr(work, sizeof(work), (char *)p->value);
			break;

		case INITYPE_BOOL:
			milstr_ncpy(work, (*((BYTE *)p->value)) ? str_true : str_false, sizeof(work));
			break;

		case INITYPE_BYTEARG:
			iniwrsetarg8(work, sizeof(work), (BYTE *)p->value, p->size);
			break;

		case INITYPE_SINT8:
			SPRINTF(work, str_d, *((char *)p->value));
			break;

		case INITYPE_SINT16:
			SPRINTF(work, str_d, *((SINT16 *)p->value));
			break;

		case INITYPE_SINT32:
			SPRINTF(work, str_d, *((SINT32 *)p->value));
			break;

		case INITYPE_UINT8:
			SPRINTF(work, str_u, *((BYTE *)p->value));
			break;

		case INITYPE_UINT16:
			SPRINTF(work, str_u, *((UINT16 *)p->value));
			break;

		case INITYPE_UINT32:
			SPRINTF(work, str_u, *((UINT32 *)p->value));
			break;

		case INITYPE_HEX8:
			SPRINTF(work, str_x, *((BYTE *)p->value));
			break;

		case INITYPE_HEX16:
			SPRINTF(work, str_x, *((UINT16 *)p->value));
			break;

		case INITYPE_HEX32:
			SPRINTF(work, str_x, *((UINT32 *)p->value));
			break;

		default:
			set = FAILURE;
			break;
		}
		if (set == SUCCESS) {
			if (file_write(fh, p->item, strlen(p->item)) != strlen(p->item)) {
				result = FAILURE;
			}
			if (file_write(fh, "=", 1) != 1) {
				result = FAILURE;
			}
			if (file_write(fh, work, strlen(work)) != strlen(work)) {
				result = FAILURE;
			}
			if (file_write(fh, "\r\n", 2) != 2) {
				result = FAILURE;
			}
		}
		p++;
	}
	if (file_close(fh) != 0) {
		result = FAILURE;
	}
	return result;
}

// ----

static const char ini_title[] = "NekoProjectII";
static const char config_file[] = "vaeg.cfg";
static char active_config_path[MAX_PATH];
static char configured_config_path[MAX_PATH];
static BOOL config_enabled = TRUE;
static char loaded_crt_parameters[sizeof(np2oscfg.gui_shader_parameters)];

BOOL initcrtchanged(void) {
	return config_enabled && strcmp(loaded_crt_parameters, np2oscfg.gui_shader_parameters) != 0;
}

void initsetpath(const char *path) {
	file_cpyname(configured_config_path, (path != NULL) ? path : "",
	             sizeof(configured_config_path));
	active_config_path[0] = '\0';
}

void initsetenabled(BOOL enabled) {
	config_enabled = enabled ? TRUE : FALSE;
	if (!config_enabled) {
		active_config_path[0] = '\0';
	}
}

static void select_config_path(char *path, int size) {
	file_cpyname(path, (configured_config_path[0] != '\0') ? configured_config_path : config_file,
	             size);
	file_cpyname(active_config_path, path, sizeof(active_config_path));
}

static const INITBL iniitem[] = {
    {"pc_model", INITYPE_STR, &np2cfg.model, sizeof(np2cfg.model)},
    {"clk_base", INITYPE_SINT32, &np2cfg.baseclock, 0},
    {"clk_mult", INITYPE_SINT32, &np2cfg.multiple, 0},
    {"sgp_mode", INITYPE_UINT8, &np2cfg.sgp_speed_mode, 0},
    {"sgp_mult", INITYPE_UINT8, &np2cfg.sgp_multiplier, 0},

    {"DIPswtch", INITYPE_BYTEARG, np2cfg.dipsw, 3},
    {"MEMswtch", INITYPE_BYTEARG, np2cfg.memsw, 8},
    {"Main_RAM", INITYPE_UINT16, &np2cfg.main_ram, 0},
    {"Main_RAM_Auto", INITYPE_BOOL, &np2cfg.main_ram_auto, 0},
    {"V1V2_Standard", INITYPE_BOOL, &np2cfg.v1v2_standard, 0},
    {"V1V2_NMode", INITYPE_BOOL, &np2cfg.v1v2_nmode, 0},
    {"V1V2_N80ROM", INITYPE_STR, np2cfg.v1v2_n80rom, sizeof(np2cfg.v1v2_n80rom)},
    {"Monitor15kHz", INITYPE_BOOL, &np2cfg.monitor_15khz, 0},
    {"MonitorDigital", INITYPE_BOOL, &np2cfg.monitor_digital, 0},
    {"Monitor15kHzFill", INITYPE_BOOL, &np2cfg.monitor_15khz_fill, 0},
    {"V1V2_8801Display", INITYPE_BOOL, &np2cfg.v1v2_8801_display, 0},
    {"CPU_Speed", INITYPE_UINT8, &np2cfg.cpu_speed, 0},
    {"CMT_Fast", INITYPE_BOOL, &np2cfg.cmt_fast, 0},
    {"CMT_vol", INITYPE_UINT8, &np2cfg.cmt_vol, 0},
    {"ExMemory", INITYPE_UINT8, &np2cfg.EXTMEM, 0},
    {"NDP8087", INITYPE_BOOL, &np2cfg.upd8087_enable, 0},
    {"NDP8087Hz", INITYPE_UINT32, &np2cfg.upd8087_clock_hz, 0},
    {"ITF_WORK", INITYPE_BOOL, &np2cfg.ITF_WORK, 0},
    {"Use_BMS_", INITYPE_BOOL, &bmsiocfg.enabled, 0},
    {"BMS_Port", INITYPE_HEX16, &bmsiocfg.port, 0},
    {"BMS_Size", INITYPE_UINT8, &bmsiocfg.numbanks, 0},

    {"FDD1FILE", INITYPE_STR, np2oscfg.fdd_image[0], MAX_PATH},
    {"FDD2FILE", INITYPE_STR, np2oscfg.fdd_image[1], MAX_PATH},
    {"HDD1FILE", INITYPE_STR, np2cfg.sasihdd[0], MAX_PATH},
    {"HDD2FILE", INITYPE_STR, np2cfg.sasihdd[1], MAX_PATH},
    {"SCSIHDD0", INITYPE_STR, np2cfg.scsihdd[0], MAX_PATH},
    {"SCSIHDD1", INITYPE_STR, np2cfg.scsihdd[1], MAX_PATH},
    {"SCSIHDD2", INITYPE_STR, np2cfg.scsihdd[2], MAX_PATH},
    {"SCSIHDD3", INITYPE_STR, np2cfg.scsihdd[3], MAX_PATH},
    {"SCSIHDD4", INITYPE_STR, np2cfg.scsihdd[4], MAX_PATH},
    {"SCSIHDD5", INITYPE_STR, np2cfg.scsihdd[5], MAX_PATH},
    {"SCSIHDD6", INITYPE_STR, np2cfg.scsihdd[6], MAX_PATH},
    {"fontfile", INITYPE_STR, np2cfg.fontfile, MAX_PATH},

    {"SampleHz", INITYPE_UINT16, &np2cfg.samplingrate, 0},
    {"Latencys", INITYPE_UINT16, &np2cfg.delayms, 0},
    {"PacingMs", INITYPE_UINT16, &np2oscfg.pacing_ms, 0},
    {"HOSTFAT", INITYPE_BOOL, &np2oscfg.hostfat_enabled, 0},
    {"HOSTFATDIR", INITYPE_STR, np2oscfg.hostfat_dir, MAX_PATH},
    {"SNDboard", INITYPE_HEX16, &np2cfg.SOUND_SW, 0},
    {"BEEP_vol", INITYPE_UINT8, &np2cfg.BEEP_VOL, 0},
    {"BEEP_level", INITYPE_UINT8, &np2cfg.beep_level, 0},
    {"xspeaker", INITYPE_BOOL, &np2cfg.snd_x, 0},

    {"SND14vol", INITYPE_BYTEARG, np2cfg.vol14, 6},
    //	{"opt14BRD", INITYPE_BYTEARG,	np2cfg.snd14opt,		3},
    {"optSPBRD", INITYPE_HEX8, &np2cfg.spbopt, 0},
    {"optSPBVR", INITYPE_HEX8, &np2cfg.spb_vrc, 0},
    {"optSPBVL", INITYPE_UINT8, &np2cfg.spb_vrl, 0},
    {"optSPB_X", INITYPE_BOOL, &np2cfg.spb_x, 0},
    {"optMPU98", INITYPE_HEX8, &np2cfg.mpuopt, 0},

    {"volume_F", INITYPE_UINT8, &np2cfg.vol_fm, 0},
    {"volume_S", INITYPE_UINT8, &np2cfg.vol_ssg, 0},
    {"volume_A", INITYPE_UINT8, &np2cfg.vol_adpcm, 0},
    {"volume_R", INITYPE_UINT8, &np2cfg.vol_rhythm, 0},

    {"Seek_Snd", INITYPE_BOOL, &np2cfg.MOTOR, 0},
    {"Seek_Vol", INITYPE_UINT8, &np2cfg.MOTORVOL, 0},

    {"btnRAPID", INITYPE_BOOL, &np2cfg.BTN_RAPID, 0},
    {"btn_MODE", INITYPE_BOOL, &np2cfg.BTN_MODE, 0},
    {"MS_RAPID", INITYPE_BOOL, &np2cfg.MOUSERAPID, 0},
    {"Mouse_VA", INITYPE_UINT8, &mouseifvacfg.device, 0},

    {"BG_COLOR", INITYPE_HEX32, &np2cfg.BG_COLOR, 0},
    {"FG_COLOR", INITYPE_HEX32, &np2cfg.FG_COLOR, 0},
    {"calendar", INITYPE_BOOL, &np2cfg.calendar, 0},
    {"USE144FD", INITYPE_BOOL, &np2cfg.usefd144, 0},

    // OS依存～
    {"s_NOWAIT", INITYPE_BOOL, &np2oscfg.NOWAIT, 0},
    {"s_speed", INITYPE_UINT16, &np2oscfg.speed_percent, 0},
    {"SkpFrame", INITYPE_UINT8, &np2oscfg.DRAW_SKIP, 0},
    {"DspClock", INITYPE_UINT8, &np2oscfg.DISPCLK, 0},
    {"F12_bind", INITYPE_UINT8, &np2oscfg.F12KEY, 0},
    {"Mouse_sw", INITYPE_BOOL, &np2oscfg.MOUSE_SW, 0},
    {"e_resume", INITYPE_BOOL, &np2oscfg.resume, 0},
    {"jast_snd", INITYPE_BOOL, &np2oscfg.jastsnd, 0}, // ver0.73
    {"GUI_scale", INITYPE_UINT8, &np2oscfg.gui_scale, 0},
    {"GUI_aspect", INITYPE_BOOL, &np2oscfg.gui_aspect, 0},
    {"GUI_fdddir", INITYPE_STR, np2oscfg.gui_fdd_dir, MAX_PATH},
    {"GUI_hdddir", INITYPE_STR, np2oscfg.gui_hdd_dir, MAX_PATH},
    {"keyboard_host_layout", INITYPE_STR, np2oscfg.keyboard_host_layout,
     sizeof(np2oscfg.keyboard_host_layout)},
    {"keyboard_kana_input", INITYPE_STR, np2oscfg.keyboard_kana_input,
     sizeof(np2oscfg.keyboard_kana_input)},
    {"keyboard_auto_kana_lock", INITYPE_BOOL, &np2oscfg.keyboard_auto_kana_lock, 0},
    {"keyboard_tenkey_overlay", INITYPE_BOOL, &np2oscfg.keyboard_tenkey_overlay, 0},
    {"keyboard_custom_map", INITYPE_STR, np2oscfg.keyboard_custom_map,
     sizeof(np2oscfg.keyboard_custom_map)},
    {"opn_backend", INITYPE_STR, np2oscfg.opn_backend, sizeof(np2oscfg.opn_backend)},
    {"ymfm_fidelity", INITYPE_STR, np2oscfg.ymfm_fidelity, sizeof(np2oscfg.ymfm_fidelity)},
    {"sound_enabled", INITYPE_BOOL, &np2oscfg.sound_enabled, 0},
    {"GUI_effect", INITYPE_UINT8, &np2oscfg.gui_effect, 0},
    {"GUI_scaling", INITYPE_UINT8, &np2oscfg.gui_scaling, 0},
    {"GUI_win_cx", INITYPE_UINT16, &np2oscfg.gui_window_width, 0},
    {"GUI_win_cy", INITYPE_UINT16, &np2oscfg.gui_window_height, 0},
    {"display_mode", INITYPE_UINT8, &np2oscfg.gui_display_mode, 0},
    {"monitor", INITYPE_SINT16, &np2oscfg.gui_monitor, 0},
    {"fullscreen_refresh", INITYPE_UINT16, &np2oscfg.gui_fullscreen_refresh, 0},
    {"fscrn_cx", INITYPE_UINT16, &np2oscfg.fscrn_cx, 0},
    {"fscrn_cy", INITYPE_UINT16, &np2oscfg.fscrn_cy, 0},
    {"fscrnmod", INITYPE_HEX8, &np2oscfg.fscrnmod, 0},
    {"NativeCRT", INITYPE_BOOL, &np2oscfg.gui_native_crt, 0},
    {"NativeCRTFilter", INITYPE_BOOL, &np2oscfg.gui_native_filter, 0},
    {"GUI_ui_scale", INITYPE_UINT16, &np2oscfg.gui_ui_scale, 0},
    {"NativeCRTPreset", INITYPE_STR, np2oscfg.gui_shader_preset,
     sizeof(np2oscfg.gui_shader_preset)},
    {"NativeCRTParameters", INITYPE_STR, np2oscfg.gui_shader_parameters,
     sizeof(np2oscfg.gui_shader_parameters)},
    {"NativeCRTAutoMask", INITYPE_BOOL, &np2oscfg.gui_native_mask_auto, 0},
};

#define INIITEMS (sizeof(iniitem) / sizeof(INITBL))

void initload(void) {
	char path[MAX_PATH];

	if (config_enabled) {
		select_config_path(path, sizeof(path));
		if (np2_debug) {
			SDL_Log("Config load: %s", path);
		}
		ini_read(path, ini_title, iniitem, INIITEMS);
	} else if (np2_debug) {
		SDL_Log("Config persistence disabled");
	}
	if (!config_enabled) {
		np2oscfg.gui_shader_parameters[0] = '\0';
	}
	milstr_ncpy(loaded_crt_parameters, np2oscfg.gui_shader_parameters,
	            sizeof(loaded_crt_parameters));
	if (!vaeg_shader_config_bind(np2oscfg.gui_shader_parameters,
	                             sizeof(np2oscfg.gui_shader_parameters))) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
		            "CRT parameter config storage is invalid");
	}
	if (np2oscfg.pacing_ms > VAEG_PACING_MS_MAX) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid PacingMs=%u; using %u",
		            np2oscfg.pacing_ms, VAEG_PACING_MS_MAX);
		np2oscfg.pacing_ms = VAEG_PACING_MS_MAX;
	}
	if (np2cfg.EXTMEM > EMSIO_MAX_MEGABYTES) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid ExMemory=%u; using %u", np2cfg.EXTMEM,
		            EMSIO_MAX_MEGABYTES);
		np2cfg.EXTMEM = EMSIO_MAX_MEGABYTES;
	}
	if ((np2cfg.main_ram != 256) && (np2cfg.main_ram != 384) && (np2cfg.main_ram != 512) &&
	    (np2cfg.main_ram != 640)) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid Main_RAM=%u; using 640",
		            np2cfg.main_ram);
		np2cfg.main_ram = 640;
	}
	if (!upd8087_clock_valid(np2cfg.upd8087_clock_hz)) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid NDP8087Hz=%u; using %u",
		            np2cfg.upd8087_clock_hz, UPD8087_DEFAULT_CLOCK_HZ);
		np2cfg.upd8087_clock_hz = UPD8087_DEFAULT_CLOCK_HZ;
	}
	np2cfg.upd8087_enable = np2cfg.upd8087_enable ? 1 : 0;

	bmsiocfg.enabled = bmsiocfg.enabled ? TRUE : FALSE;
	if ((bmsiocfg.port != BMSIO_PORT_DEFAULT) && (bmsiocfg.port != BMSIO_PORT_COMPAT)) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid BMS_Port=%04x; using %04x",
		            bmsiocfg.port, BMSIO_PORT_DEFAULT);
		bmsiocfg.port = BMSIO_PORT_DEFAULT;
	}
	if (bmsiocfg.numbanks == 0) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid BMS_Size=0; using %u",
		            BMSIO_DEFAULT_BANKS);
		bmsiocfg.numbanks = BMSIO_DEFAULT_BANKS;
	}
	bmsiocfg.portmask = BMSIO_PORT_MASK;
	if (!vaeg_sound_rate_valid(np2cfg.samplingrate)) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid SampleHz=%u; using %u",
		            np2cfg.samplingrate, VAEG_SOUND_RATE_DEFAULT);
		np2cfg.samplingrate = VAEG_SOUND_RATE_DEFAULT;
	}
	if (!vaeg_sound_buffer_valid(np2cfg.delayms)) {
		const UINT delayms = vaeg_sound_buffer_clamp(np2cfg.delayms);

		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid Latencys=%u; using %u", np2cfg.delayms,
		            delayms);
		np2cfg.delayms = (UINT16)delayms;
	}
	np2oscfg.DISPCLK &= VAEG_DISPINFO_MASK;
	if (np2oscfg.gui_scale > VAEG_WINDOW_SCALE_MAX) {
		np2oscfg.gui_scale = 1;
	}
	np2oscfg.gui_aspect = np2oscfg.gui_aspect ? 1 : 0;
	np2oscfg.gui_native_crt = np2oscfg.gui_native_crt ? 1 : 0;
	np2oscfg.gui_native_filter = np2oscfg.gui_native_filter ? 1 : 0;
	np2oscfg.gui_native_mask_auto = np2oscfg.gui_native_mask_auto ? 1 : 0;
	if (np2oscfg.gui_shader_preset[0] == '\0') {
		milstr_ncpy(np2oscfg.gui_shader_preset, VAEG_DEFAULT_SHADER_PRESET,
		            sizeof(np2oscfg.gui_shader_preset));
	}
	if (np2oscfg.gui_effect >= VAEG_EFFECT_COUNT) {
		np2oscfg.gui_effect = VAEG_EFFECT_UNFILTERED;
	}
	if (np2oscfg.gui_scaling >= VAEG_SCALING_COUNT) {
		np2oscfg.gui_scaling = VAEG_SCALING_FIT;
	}
	if (np2oscfg.gui_window_width < 320) {
		np2oscfg.gui_window_width = 640;
	}
	if (np2oscfg.gui_window_height < 240) {
		np2oscfg.gui_window_height = 422;
	}
	if ((np2oscfg.gui_display_mode != VAEG_DISPLAY_WINDOWED) &&
	    (np2oscfg.gui_display_mode != VAEG_DISPLAY_EXCLUSIVE)) {
		np2oscfg.gui_display_mode = VAEG_DISPLAY_WINDOWED;
	}
	if (np2oscfg.gui_monitor < 0) {
		np2oscfg.gui_monitor = 0;
	}
	{
		BOOL masked;
		UINT8 fscrnmod;

		fscrnmod = vaeg_fscrnmod_sanitize(np2oscfg.fscrnmod, &masked);
		if (masked) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
			            "Unsupported fscrnmod bits in %02x; using %02x", np2oscfg.fscrnmod,
			            fscrnmod);
		}
		np2oscfg.fscrnmod = fscrnmod;
	}
	if (np2oscfg.gui_display_mode == VAEG_DISPLAY_EXCLUSIVE) {
		np2oscfg.fscrn_cx = 0;
		np2oscfg.fscrn_cy = 0;
		np2oscfg.gui_fullscreen_refresh = 0;
		np2oscfg.fscrnmod = (np2oscfg.fscrnmod & 3) | 4;
	}
	if (!sgp_speed_mode_valid(np2cfg.sgp_speed_mode)) {
		np2cfg.sgp_speed_mode = SGP_SPEED_MODEL_DEFAULT;
	}
	if (!sgp_speed_multiplier_valid(np2cfg.sgp_multiplier)) {
		np2cfg.sgp_multiplier = 1;
	}
	if (!np2_sound_hardware_valid(np2cfg.model, np2cfg.SOUND_SW)) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
		            "Invalid SNDboard=%03x for %s; using model default %03x", np2cfg.SOUND_SW,
		            np2cfg.model, np2_default_sound_for_model(np2cfg.model));
		np2cfg.SOUND_SW = np2_default_sound_for_model(np2cfg.model);
	}
	np2oscfg.sound_enabled = np2oscfg.sound_enabled ? 1 : 0;
	np2oscfg.MOUSE_SW = np2oscfg.MOUSE_SW ? 1 : 0;
	if (mouseifvacfg.device > MOUSEIFVA_MOUSE) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid Mouse_VA=%u; using joystick",
		            mouseifvacfg.device);
		mouseifvacfg.device = MOUSEIFVA_JOYPAD;
	}
	np2oscfg.keyboard_auto_kana_lock = np2oscfg.keyboard_auto_kana_lock ? 1 : 0;
	np2oscfg.keyboard_tenkey_overlay = np2oscfg.keyboard_tenkey_overlay ? 1 : 0;
	opngen_setbackend(opngen_parsebackend(np2oscfg.opn_backend));
	milstr_ncpy(np2oscfg.opn_backend, opngen_backendname(opngen_getbackend()),
	            sizeof(np2oscfg.opn_backend));
	ymfm_opn_setfidelity(ymfm_opn_parsefidelity(np2oscfg.ymfm_fidelity));
	milstr_ncpy(np2oscfg.ymfm_fidelity, ymfm_opn_fidelityname(ymfm_opn_getfidelity()),
	            sizeof(np2oscfg.ymfm_fidelity));
}

int initsave_checked(void) {
	char temporary[MAX_PATH];
	if (!config_enabled) {
		return FAILURE;
	}
	if (active_config_path[0] == '\0') {
		select_config_path(active_config_path, sizeof(active_config_path));
	}
	if (strlen(active_config_path) + sizeof(".tmp") > sizeof(temporary)) {
		return FAILURE;
	}
	strcpy(temporary, active_config_path);
	strcat(temporary, ".tmp");
	if (ini_write(temporary, ini_title, iniitem, INIITEMS) != SUCCESS ||
	    file_rename_atomic(temporary, active_config_path, TRUE) != 0) {
		file_delete(temporary);
		return FAILURE;
	}
	return SUCCESS;
}

void initsave(void) {
	if (config_enabled && initsave_checked() != SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not save configuration: %s",
		             active_config_path);
	}
}
