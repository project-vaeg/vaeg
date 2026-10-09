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
#ifndef VAEG_SDL2_NP2_H
#define VAEG_SDL2_NP2_H

enum {
	NP2OSCFG_KEYBOARD_NAME_SIZE = 16,
	NP2OSCFG_KEYBOARD_CUSTOM_MAP_SIZE = 8192,
	NP2OSCFG_OPN_BACKEND_NAME_SIZE = 8,
	NP2OSCFG_YMFM_FIDELITY_NAME_SIZE = 8,
	NP2OSCFG_SHADER_PRESET_PATH_SIZE = MAX_PATH,
	VAEG_PACING_MS_MAX = 1000
};

#define VAEG_DEFAULT_SHADER_PRESET "assets/shaders/crt/vaeg_crt_default.slangp"

enum {
	VAEG_DISPLAY_WINDOWED = 0,
	VAEG_DISPLAY_BORDERLESS,
	VAEG_DISPLAY_EXCLUSIVE,
	VAEG_DISPLAY_MODE_COUNT
};

enum {
	VAEG_DISPINFO_CPU_CLOCK = 0x01,
	VAEG_DISPINFO_FRAME = 0x02,
	VAEG_DISPINFO_SGP_CLOCK = 0x04,
	VAEG_DISPINFO_FPS = 0x08,
	VAEG_DISPINFO_VIDEO = 0x10,
	VAEG_DISPINFO_FDD = 0x20,
	VAEG_DISPINFO_FRAMEBUFFER = 0x40,
	VAEG_DISPINFO_MASK = 0x7f
};

typedef struct {
	BYTE NOWAIT;
	BYTE DRAW_SKIP;
	BYTE DISPCLK;
	BYTE F12KEY;
	BYTE resume;
	BYTE jastsnd;
	BYTE gui_scale;
	BYTE gui_aspect;
	char gui_fdd_dir[MAX_PATH];
	char gui_hdd_dir[MAX_PATH];
	char fdd_image[2][MAX_PATH];
	char keyboard_host_layout[NP2OSCFG_KEYBOARD_NAME_SIZE];
	char keyboard_kana_input[NP2OSCFG_KEYBOARD_NAME_SIZE];
	BYTE keyboard_auto_kana_lock;
	BYTE keyboard_tenkey_overlay;
	char keyboard_custom_map[NP2OSCFG_KEYBOARD_CUSTOM_MAP_SIZE];
	char opn_backend[NP2OSCFG_OPN_BACKEND_NAME_SIZE];
	char ymfm_fidelity[NP2OSCFG_YMFM_FIDELITY_NAME_SIZE];
	BYTE sound_enabled;
	BYTE gui_effect;
	BYTE gui_scaling;
	UINT16 gui_window_width;
	UINT16 gui_window_height;
	BYTE gui_display_mode;
	SINT16 gui_monitor;
	UINT16 gui_fullscreen_refresh;
	UINT16 fscrn_cx;
	UINT16 fscrn_cy;
	BYTE fscrnmod;
	BYTE MOUSE_SW;
	UINT16 pacing_ms;
	BYTE hostfat_enabled;
	char hostfat_dir[MAX_PATH];
	BYTE gui_native_crt;
	BYTE gui_native_filter;
	char gui_shader_preset[NP2OSCFG_SHADER_PRESET_PATH_SIZE];
	UINT16 gui_ui_scale;
	char gui_shader_parameters[8192];
	BYTE gui_native_mask_auto;
	/* Emulation speed in percent (10-400; 0 = 100); No Wait is the maximum. */
	UINT16 speed_percent;
} NP2OSCFG;

#if defined(SIZE_QVGA)
enum {
	FULLSCREEN_WIDTH = 320,
	FULLSCREEN_HEIGHT = 240
};
#else
enum {
	FULLSCREEN_WIDTH = 640,
	FULLSCREEN_HEIGHT = 400
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern NP2OSCFG np2oscfg;
extern BOOL np2_debug;
UINT16 np2_default_sound_for_model(const char *model);
BOOL np2_sound_hardware_valid(const char *model, UINT16 sound);
UINT16 np2_sound_for_model_selection(const char *model, UINT16 sound);
const char *np2_cli_boot_model(const char *value);
BOOL np2_select_boot_model(const char *model);
BOOL np2_cli_override_selftest(void);

#ifdef __cplusplus
}
#endif

#endif
