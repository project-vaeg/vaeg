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
#ifndef VAEG_SDL2_CLIOPTS_H
#define VAEG_SDL2_CLIOPTS_H

#include "compiler.h"
#include "screenshot.h"

enum {
	VAEG_CLI_MODEL_UNSET = 0,
	VAEG_CLI_MODEL_VA,
	VAEG_CLI_MODEL_VA2
};

enum {
	VAEG_CLI_FM_BACKEND_UNSET = 0,
	VAEG_CLI_FM_BACKEND_NP2,
	VAEG_CLI_FM_BACKEND_YMFM
};

enum {
	VAEG_CLI_FM_SOUND_UNSET = 0,
	VAEG_CLI_FM_SOUND_OPN,
	VAEG_CLI_FM_SOUND_OPNA
};

enum {
	VAEG_CLI_FIDELITY_UNSET = 0,
	VAEG_CLI_FIDELITY_MINIMUM,
	VAEG_CLI_FIDELITY_MEDIUM,
	VAEG_CLI_FIDELITY_MAXIMUM
};

enum {
	VAEG_CLI_SGP_UNSET = 0,
	VAEG_CLI_SGP_MODEL,
	VAEG_CLI_SGP_FOLLOW_CPU,
	VAEG_CLI_SGP_CUSTOM
};

enum {
	VAEG_CLI_FRAMESKIP_UNSET = 0,
	VAEG_CLI_FRAMESKIP_AUTO,
	VAEG_CLI_FRAMESKIP_FULL,
	VAEG_CLI_FRAMESKIP_HALF,
	VAEG_CLI_FRAMESKIP_THIRD,
	VAEG_CLI_FRAMESKIP_QUARTER
};

enum {
	VAEG_CLI_DISPLAY_UNSET = 0,
	VAEG_CLI_DISPLAY_WINDOWED,
	VAEG_CLI_DISPLAY_FULLSCREEN
};

enum {
	VAEG_CLI_EFFECT_UNSET = 0,
	VAEG_CLI_EFFECT_UNFILTERED,
	VAEG_CLI_EFFECT_LINEAR,
	VAEG_CLI_EFFECT_SCANLINE,
	VAEG_CLI_EFFECT_CRT_LITE
};

enum {
	VAEG_CLI_SCALING_UNSET = 0,
	VAEG_CLI_SCALING_NATIVE,
	VAEG_CLI_SCALING_FIT,
	VAEG_CLI_SCALING_FIT_8DOT,
	VAEG_CLI_SCALING_INTEGER,
	VAEG_CLI_SCALING_STRETCH
};

enum {
	VAEG_CLI_CONTROLLER_UNSET = 0,
	VAEG_CLI_CONTROLLER_JOYSTICK,
	VAEG_CLI_CONTROLLER_MOUSE
};

enum {
	VAEG_CLI_KEYBOARD_UNSET = 0,
	VAEG_CLI_KEYBOARD_JIS,
	VAEG_CLI_KEYBOARD_US,
	VAEG_CLI_KEYBOARD_CUSTOM
};

enum {
	VAEG_CLI_MEDIA_UNSET = 0,
	VAEG_CLI_MEDIA_PATH,
	VAEG_CLI_MEDIA_NONE
};

typedef struct {
	BOOL help;
	BOOL version;
	BOOL smoke;
	BOOL selftest;
	BOOL debug;
	BOOL fdctrace;
	BOOL scsitrace;
	BOOL scsitrace_guest;
	BOOL scsitrace_compact;
	BOOL scsitrace_census;
	BOOL scsitrace_census_only;
	BOOL scsitrace_cmdreq_windows;
	BOOL scsitrace_jitter;
	BOOL pacelog;
	BOOL mute;
	BOOL nowait;
	BOOL no_config;
	BOOL no_bkupmem;
	UINT trace_cpu;
	UINT scsitrace_limit;
	UINT scsitrace_jitter_seed;
	UINT scsitrace_jitter_span;
	UINT model;
	UINT fm_backend;
	UINT fm_sound;
	UINT ymfm_fidelity;
	UINT sample_rate;
	UINT sound_buffer;
	UINT cpu_multiplier;
	UINT sgp_mode;
	UINT sgp_multiplier;
	UINT frame_skip;
	UINT display_mode;
	UINT effect;
	UINT scaling;
	UINT controller;
	UINT keyboard_layout;
	UINT fdd_mode[2];
	const char *fdd_path[2];
	UINT fdd_image[2]; /* 1-based disk within a multi-image D88; 0 = unset */
	const char *tape_path;      /* cassette tape image to load (V1/V2) */
	const char *tape_save_path; /* raw image recording the saved tape */
	UINT sasi_mode[2];
	const char *sasi_path[2];
	UINT scsi_mode[7];
	const char *scsi_path[7];
	const char *hostfat_path;
	const char *roms_path;
	const char *config_path;
	const char *bkupmem_path;
	const char *headless_input_script;
	const char *debug_script;
	const char *debug_output_dir;
	const char *screen_dump_path;
	const char *screen_tvram_dump_path;
	VAEG_SCREENSHOT_REQUEST screenshot_requests[VAEG_SCREENSHOT_MAX_REQUESTS];
	UINT screenshot_count;
} VAEG_CLI_OPTIONS;

#ifdef __cplusplus
extern "C" {
#endif

void vaeg_cli_options_init(VAEG_CLI_OPTIONS *options);
BOOL vaeg_cli_parse(int argc, char **argv, VAEG_CLI_OPTIONS *options, char *error, UINT error_size);

#ifdef __cplusplus
}
#endif

#endif
