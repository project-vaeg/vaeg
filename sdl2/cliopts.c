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
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "cliopts.h"
#include "machine/pccore.h"
#include "sgp.h"
#include "soundopts.h"

static BOOL name_equal(const char *left, const char *right) {
	unsigned char l;
	unsigned char r;

	if ((left == NULL) || (right == NULL)) {
		return (FALSE);
	}
	while ((*left != '\0') && (*right != '\0')) {
		l = (unsigned char)*left++;
		r = (unsigned char)*right++;
		if ((l >= 'A') && (l <= 'Z')) {
			l = (unsigned char)(l - 'A' + 'a');
		}
		if ((r >= 'A') && (r <= 'Z')) {
			r = (unsigned char)(r - 'A' + 'a');
		}
		if (l != r) {
			return (FALSE);
		}
	}
	return ((*left == '\0') && (*right == '\0'));
}

static BOOL set_error(char *error, UINT error_size, const char *message, const char *value) {
	if ((error != NULL) && (error_size != 0)) {
		if (value != NULL) {
			snprintf(error, error_size, "%s: %s", message, value);
		} else {
			snprintf(error, error_size, "%s", message);
		}
	}
	return (FAILURE);
}

static BOOL is_scsi_media_option(const char *argument) {
	return ((argument != NULL) && !strncmp(argument, "--scsi", 6) && (argument[6] >= '0') &&
	        (argument[6] <= '6') && (argument[7] == '\0'));
}

static const char *option_value(int argc, char **argv, int *position, const char *option,
                                char *error, UINT error_size) {
	if (*position >= argc) {
		set_error(error, error_size, "option requires a value", option);
		return (NULL);
	}
	return (argv[(*position)++]);
}

static BOOL parse_uint(const char *value, UINT *result) {
	char *end;
	unsigned long parsed;

	if ((value == NULL) || (value[0] == '\0') || (value[0] == '-')) {
		return (FAILURE);
	}
	errno = 0;
	end = NULL;
	parsed = strtoul(value, &end, 10);
	if ((errno != 0) || (end == value) || (*end != '\0') || (parsed > UINT_MAX)) {
		return (FAILURE);
	}
	*result = (UINT)parsed;
	return (SUCCESS);
}

static BOOL parse_screenshot(const char *value, VAEG_SCREENSHOT_REQUEST *request,
                             char *error, UINT error_size) {
	char *end;
	const char *path;
	unsigned long frame;

	if ((value == NULL) || (value[0] < '0') || (value[0] > '9')) {
		return (set_error(error, error_size,
		                  "--screenshot requires a decimal FRAME:PATH with FRAME >= 1", value));
	}
	errno = 0;
	end = NULL;
	frame = strtoul(value, &end, 10);
	if ((errno != 0) || (end == value) || (*end != ':') || (frame == 0) ||
	    (frame > UINT32_MAX)) {
		return (set_error(error, error_size,
		                  "--screenshot requires a decimal FRAME:PATH with FRAME >= 1", value));
	}
	path = end + 1;
	if (path[0] == '\0') {
		return (set_error(error, error_size, "--screenshot requires a non-empty output path", value));
	}
	if (strrchr(path, '.') != NULL) {
		const char *extension = strrchr(path, '.');
		if (name_equal(extension, ".bmp")) {
			request->format = VAEG_SCREENSHOT_FORMAT_BMP;
		} else if (name_equal(extension, ".png")) {
			request->format = VAEG_SCREENSHOT_FORMAT_PNG;
		} else {
			return (set_error(error, error_size,
			                  "--screenshot supports only .bmp and .png paths", value));
		}
	} else {
		return (set_error(error, error_size,
		                  "--screenshot supports only .bmp and .png paths", value));
	}
	request->frame = (UINT32)frame;
	request->path = path;
	return SUCCESS;
}

static BOOL parse_media(const char *option, const char *value, UINT *mode, const char **path,
                        char *error, UINT error_size) {
	if ((value == NULL) || (value[0] == '\0')) {
		return (set_error(error, error_size, "media path is empty", option));
	}
	if (name_equal(value, "none")) {
		*mode = VAEG_CLI_MEDIA_NONE;
		*path = NULL;
		return (SUCCESS);
	}
	if (strlen(value) >= MAX_PATH) {
		return (set_error(error, error_size, "media path is too long", option));
	}
	*mode = VAEG_CLI_MEDIA_PATH;
	*path = value;
	return (SUCCESS);
}

void vaeg_cli_options_init(VAEG_CLI_OPTIONS *options) {
	if (options != NULL) {
		ZeroMemory(options, sizeof(*options));
	}
}

BOOL vaeg_cli_parse(int argc, char **argv, VAEG_CLI_OPTIONS *options, char *error,
                    UINT error_size) {
	int position;
	const char *argument;
	const char *value;
	UINT number;

	if ((options == NULL) || (argc < 0) || ((argc != 0) && (argv == NULL))) {
		return (set_error(error, error_size, "invalid CLI parser input", NULL));
	}
	vaeg_cli_options_init(options);
	if ((error != NULL) && (error_size != 0)) {
		error[0] = '\0';
	}
	position = 1;
	while (position < argc) {
		argument = argv[position++];
		if ((!strcmp(argument, "-h")) || (!strcmp(argument, "--help"))) {
			options->help = TRUE;
			return (SUCCESS);
		}
		if (!strcmp(argument, "--version")) {
			options->version = TRUE;
			return (SUCCESS);
		}
		if (!strcmp(argument, "--smoke")) {
			options->smoke = TRUE;
		} else if (!strcmp(argument, "--selftest")) {
			options->selftest = TRUE;
		} else if (!strcmp(argument, "--debug")) {
			options->debug = TRUE;
		} else if (!strcmp(argument, "--fdctrace")) {
			options->fdctrace = TRUE;
		} else if (!strcmp(argument, "--scsitrace")) {
			options->scsitrace = TRUE;
			options->scsitrace_guest = TRUE;
		} else if (!strcmp(argument, "--scsitrace-no-guest")) {
			options->scsitrace = TRUE;
			options->scsitrace_guest = FALSE;
		} else if (!strcmp(argument, "--scsitrace-compact")) {
			options->scsitrace = TRUE;
			options->scsitrace_compact = TRUE;
		} else if (!strcmp(argument, "--scsitrace-census")) {
			options->scsitrace = TRUE;
			options->scsitrace_compact = TRUE;
			options->scsitrace_census = TRUE;
		} else if (!strcmp(argument, "--scsitrace-census-only")) {
			options->scsitrace = TRUE;
			options->scsitrace_compact = TRUE;
			options->scsitrace_census = TRUE;
			options->scsitrace_census_only = TRUE;
		} else if (!strcmp(argument, "--scsitrace-cmdreq-windows")) {
			options->scsitrace = TRUE;
			options->scsitrace_guest = TRUE;
			options->scsitrace_cmdreq_windows = TRUE;
		} else if (!strcmp(argument, "--scsitrace-jitter-seed")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (parse_uint(value, &number) != SUCCESS)) {
				return (set_error(error, error_size,
				                  "--scsitrace-jitter-seed accepts an unsigned seed", value));
			}
			options->scsitrace = TRUE;
			options->scsitrace_jitter = TRUE;
			options->scsitrace_jitter_seed = number;
		} else if (!strcmp(argument, "--scsitrace-jitter-span")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (parse_uint(value, &number) != SUCCESS) || (number > 100000)) {
				return (set_error(error, error_size,
				                  "--scsitrace-jitter-span accepts 0 through 100000 clocks",
				                  value));
			}
			options->scsitrace = TRUE;
			options->scsitrace_jitter = TRUE;
			options->scsitrace_jitter_span = number;
		} else if (!strcmp(argument, "--scsitrace-limit")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (parse_uint(value, &number) != SUCCESS) || (number == 0) ||
			    (number > 1000000)) {
				return (set_error(error, error_size,
				                  "--scsitrace-limit accepts 1 through 1000000 transfers", value));
			}
			options->scsitrace_limit = number;
		} else if (!strcmp(argument, "--headless-input-script")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (value[0] == '\0')) {
				return (
				    set_error(error, error_size, "--headless-input-script requires a path", value));
			}
			options->headless_input_script = value;
		} else if (!strcmp(argument, "--debug-script")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (value[0] == '\0')) {
				return (set_error(error, error_size, "--debug-script requires a path", value));
			}
			options->debug_script = value;
		} else if (!strcmp(argument, "--debug-output-dir")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (value[0] == '\0')) {
				return (set_error(error, error_size, "--debug-output-dir requires a path", value));
			}
			options->debug_output_dir = value;
		} else if (!strcmp(argument, "--screen-dump")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (value[0] == '\0')) {
				return (set_error(error, error_size, "--screen-dump requires a path", value));
			}
			options->screen_dump_path = value;
		} else if (!strcmp(argument, "--screenshot")) {
			VAEG_SCREENSHOT_REQUEST request;

			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return FAILURE;
			}
			if (options->screenshot_count >= VAEG_SCREENSHOT_MAX_REQUESTS) {
				return (set_error(error, error_size,
				                  "--screenshot accepts at most 64 requests", value));
			}
			if (parse_screenshot(value, &request, error, error_size) != SUCCESS) {
				return FAILURE;
			}
			options->screenshot_requests[options->screenshot_count++] = request;
		} else if (!strcmp(argument, "--screen-tvram-dump")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (value[0] == '\0')) {
				return (set_error(error, error_size, "--screen-tvram-dump requires a path", value));
			}
			options->screen_tvram_dump_path = value;
		} else if (!strcmp(argument, "--pacelog")) {
			options->pacelog = TRUE;
		} else if (!strcmp(argument, "--mute")) {
			options->mute = TRUE;
		} else if (!strcmp(argument, "--nowait")) {
			options->nowait = TRUE;
		} else if (!strcmp(argument, "--no-cfg")) {
			options->no_config = TRUE;
		} else if (!strcmp(argument, "--no-bkupmem")) {
			options->no_bkupmem = TRUE;
		} else if (!strcmp(argument, "--trace-cpu")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (parse_uint(value, &number) != SUCCESS) || (number == 0) ||
			    (number > 1000000)) {
				return (set_error(error, error_size, "--trace-cpu accepts 1 through 1000000 steps",
				                  value));
			}
			options->trace_cpu = number;
		} else if (!strcmp(argument, "--fullscreen")) {
			options->display_mode = VAEG_CLI_DISPLAY_FULLSCREEN;
		} else if (!strcmp(argument, "--windowed")) {
			options->display_mode = VAEG_CLI_DISPLAY_WINDOWED;
		} else if (!strcmp(argument, "--model")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "va")) {
				options->model = VAEG_CLI_MODEL_VA;
			} else if (name_equal(value, "va2")) {
				options->model = VAEG_CLI_MODEL_VA2;
			} else {
				return (set_error(error, error_size, "--model accepts only va or va2", value));
			}
		} else if (!strcmp(argument, "--fmbackend")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "np2")) {
				options->fm_backend = VAEG_CLI_FM_BACKEND_NP2;
			} else if (name_equal(value, "ymfm")) {
				options->fm_backend = VAEG_CLI_FM_BACKEND_YMFM;
			} else {
				return (
				    set_error(error, error_size, "--fmbackend accepts only np2 or ymfm", value));
			}
		} else if (!strcmp(argument, "--fmsound")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "opn")) {
				options->fm_sound = VAEG_CLI_FM_SOUND_OPN;
			} else if (name_equal(value, "opna")) {
				options->fm_sound = VAEG_CLI_FM_SOUND_OPNA;
			} else {
				return (set_error(error, error_size, "--fmsound accepts only opn or opna", value));
			}
		} else if (!strcmp(argument, "--ymfm-fidelity")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "minimum")) {
				options->ymfm_fidelity = VAEG_CLI_FIDELITY_MINIMUM;
			} else if (name_equal(value, "medium")) {
				options->ymfm_fidelity = VAEG_CLI_FIDELITY_MEDIUM;
			} else if (name_equal(value, "maximum")) {
				options->ymfm_fidelity = VAEG_CLI_FIDELITY_MAXIMUM;
			} else {
				return (set_error(error, error_size,
				                  "--ymfm-fidelity accepts minimum, medium, or maximum", value));
			}
		} else if (!strcmp(argument, "--samplerate")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (parse_uint(value, &number) != SUCCESS) ||
			    !vaeg_sound_rate_valid(number)) {
				return (set_error(error, error_size, "--samplerate accepts 11025, 22050, or 44100",
				                  value));
			}
			options->sample_rate = number;
		} else if (!strcmp(argument, "--soundbuffer")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (parse_uint(value, &number) != SUCCESS) ||
			    !vaeg_sound_buffer_valid(number)) {
				return (set_error(error, error_size,
				                  "--soundbuffer accepts 40 through 1000 milliseconds", value));
			}
			options->sound_buffer = number;
		} else if (!strcmp(argument, "--cpumult")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (parse_uint(value, &number) != SUCCESS) ||
			    !pccore_cpu_multiple_valid(number)) {
				return (set_error(error, error_size, "--cpumult accepts 1 through 32", value));
			}
			options->cpu_multiplier = number;
		} else if (!strcmp(argument, "--sgp")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "model")) {
				options->sgp_mode = VAEG_CLI_SGP_MODEL;
			} else if (name_equal(value, "follow-cpu")) {
				options->sgp_mode = VAEG_CLI_SGP_FOLLOW_CPU;
			} else if ((parse_uint(value, &number) == SUCCESS) &&
			           sgp_speed_multiplier_valid(number)) {
				options->sgp_mode = VAEG_CLI_SGP_CUSTOM;
				options->sgp_multiplier = number;
			} else {
				return (set_error(error, error_size,
				                  "--sgp accepts model, follow-cpu, or 1 through 16", value));
			}
		} else if (!strcmp(argument, "--frameskip")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "auto")) {
				options->frame_skip = VAEG_CLI_FRAMESKIP_AUTO;
			} else if (name_equal(value, "full")) {
				options->frame_skip = VAEG_CLI_FRAMESKIP_FULL;
			} else if (!strcmp(value, "2")) {
				options->frame_skip = VAEG_CLI_FRAMESKIP_HALF;
			} else if (!strcmp(value, "3")) {
				options->frame_skip = VAEG_CLI_FRAMESKIP_THIRD;
			} else if (!strcmp(value, "4")) {
				options->frame_skip = VAEG_CLI_FRAMESKIP_QUARTER;
			} else {
				return (set_error(error, error_size, "--frameskip accepts auto, full, 2, 3, or 4",
				                  value));
			}
		} else if (!strcmp(argument, "--effect")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "unfiltered")) {
				options->effect = VAEG_CLI_EFFECT_UNFILTERED;
			} else if (name_equal(value, "linear")) {
				options->effect = VAEG_CLI_EFFECT_LINEAR;
			} else if (name_equal(value, "scanline")) {
				options->effect = VAEG_CLI_EFFECT_SCANLINE;
			} else if (name_equal(value, "crt-lite")) {
				options->effect = VAEG_CLI_EFFECT_CRT_LITE;
			} else {
				return (set_error(error, error_size,
				                  "--effect accepts unfiltered, linear, scanline, or crt-lite",
				                  value));
			}
		} else if (!strcmp(argument, "--scaling")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "native")) {
				options->scaling = VAEG_CLI_SCALING_NATIVE;
			} else if (name_equal(value, "fit")) {
				options->scaling = VAEG_CLI_SCALING_FIT;
			} else if (name_equal(value, "fit-8dot")) {
				options->scaling = VAEG_CLI_SCALING_FIT_8DOT;
			} else if (name_equal(value, "integer")) {
				options->scaling = VAEG_CLI_SCALING_INTEGER;
			} else if (name_equal(value, "stretch")) {
				options->scaling = VAEG_CLI_SCALING_STRETCH;
			} else {
				return (set_error(error, error_size,
				                  "--scaling accepts native, fit, fit-8dot, integer, or stretch",
				                  value));
			}
		} else if (!strcmp(argument, "--controller")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "joystick")) {
				options->controller = VAEG_CLI_CONTROLLER_JOYSTICK;
			} else if (name_equal(value, "mouse")) {
				options->controller = VAEG_CLI_CONTROLLER_MOUSE;
			} else {
				return (set_error(error, error_size, "--controller accepts only joystick or mouse",
				                  value));
			}
		} else if (!strcmp(argument, "--keyboard-layout")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (name_equal(value, "jis")) {
				options->keyboard_layout = VAEG_CLI_KEYBOARD_JIS;
			} else if (name_equal(value, "us")) {
				options->keyboard_layout = VAEG_CLI_KEYBOARD_US;
			} else if (name_equal(value, "custom")) {
				options->keyboard_layout = VAEG_CLI_KEYBOARD_CUSTOM;
			} else {
				return (set_error(error, error_size, "--keyboard-layout accepts jis, us, or custom",
				                  value));
			}
		} else if ((!strcmp(argument, "--tape")) || (!strcmp(argument, "--tape-save"))) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (argument[6] == '\0') {
				options->tape_path = value;
			} else {
				options->tape_save_path = value;
			}
		} else if ((!strcmp(argument, "--fdd1-image")) || (!strcmp(argument, "--fdd2-image"))) {
			const int drive = argument[5] - '1';

			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (parse_uint(value, &number) != SUCCESS) || (number < 1) ||
			    (number > 255)) {
				return (set_error(error, error_size,
				                  "--fdd1-image and --fdd2-image accept 1 through 255", value));
			}
			options->fdd_image[drive] = number;
		} else if ((!strcmp(argument, "--fdd1")) || (!strcmp(argument, "--fdd2")) ||
		           (!strcmp(argument, "--sasi1")) || (!strcmp(argument, "--sasi2")) ||
		           is_scsi_media_option(argument)) {
			const BOOL scsi = !strncmp(argument, "--scsi", 6);
			const int drive = argument[strlen(argument) - 1] - (scsi ? '0' : '1');
			const BOOL fdd = !strncmp(argument, "--fdd", 5);

			value = option_value(argc, argv, &position, argument, error, error_size);
			if (value == NULL) {
				return (FAILURE);
			}
			if (fdd) {
				if (parse_media(argument, value, &options->fdd_mode[drive],
				                &options->fdd_path[drive], error, error_size) != SUCCESS) {
					return (FAILURE);
				}
			} else if (scsi) {
				if (parse_media(argument, value, &options->scsi_mode[drive],
				                &options->scsi_path[drive], error, error_size) != SUCCESS) {
					return (FAILURE);
				}
			} else if (parse_media(argument, value, &options->sasi_mode[drive],
			                       &options->sasi_path[drive], error, error_size) != SUCCESS) {
				return (FAILURE);
			}
		} else if (!strcmp(argument, "--hostfat-dir")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (value[0] == '\0')) {
				return (
				    set_error(error, error_size, "--hostfat-dir requires a non-empty path", value));
			}
			if (strlen(value) >= MAX_PATH) {
				return (set_error(error, error_size, "HOSTFAT directory path is too long", value));
			}
			options->hostfat_path = value;
		} else if (!strcmp(argument, "--roms")) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (value[0] == '\0')) {
				return (set_error(error, error_size, "--roms requires a non-empty path", value));
			}
			if (strlen(value) >= MAX_PATH) {
				return (set_error(error, error_size, "ROM directory path is too long", value));
			}
			options->roms_path = value;
		} else if ((!strcmp(argument, "--cfg")) || (!strcmp(argument, "--bkupmem"))) {
			value = option_value(argc, argv, &position, argument, error, error_size);
			if ((value == NULL) || (value[0] == '\0')) {
				return (set_error(error, error_size, "persistence option requires a non-empty path",
				                  argument));
			}
			if (strlen(value) >= MAX_PATH) {
				return (set_error(error, error_size, "persistence path is too long", argument));
			}
			if (!strcmp(argument, "--cfg")) {
				options->config_path = value;
			} else {
				options->bkupmem_path = value;
			}
		} else if (argument[0] == '-') {
			return (set_error(error, error_size, "unknown option", argument));
		} else {
			return (set_error(error, error_size,
			                  "positional FDD arguments were removed; use --fdd1 or --fdd2",
			                  argument));
		}
	}
	if (options->no_config && (options->config_path != NULL)) {
		return (set_error(error, error_size, "--cfg cannot be combined with --no-cfg", NULL));
	}
	if (options->no_bkupmem && (options->bkupmem_path != NULL)) {
		return (
		    set_error(error, error_size, "--bkupmem cannot be combined with --no-bkupmem", NULL));
	}
	return (SUCCESS);
}
