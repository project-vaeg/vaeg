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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_SDL2_LIBRASHADER_NATIVE_PRESENTER_CONTROLLER_H
#define VAEG_SDL2_LIBRASHADER_NATIVE_PRESENTER_CONTROLLER_H

#include <stdint.h>

#include "librashader/frame_input.h"

typedef struct VAEG_NATIVE_PRESENTER VAEG_NATIVE_PRESENTER;

typedef enum {
	VAEG_NATIVE_PRESENTER_PRESENTED = 0,
	VAEG_NATIVE_PRESENTER_NO_OUTPUT,
	VAEG_NATIVE_PRESENTER_FALLBACK
} VAEG_NATIVE_PRESENTER_RESULT;

#ifdef __cplusplus
extern "C" {
#endif

int vaeg_native_presenter_is_headless_video_driver(const char *video_driver);
const char *vaeg_native_presenter_creation_error(void);
void vaeg_native_presenter_set_output_capture(VAEG_NATIVE_PRESENTER *presenter,
                                               VAEG_OUTPUT_CAPTURE *capture);
VAEG_NATIVE_PRESENTER *vaeg_native_presenter_create(
	void *host_window, uint32_t drawable_width, uint32_t drawable_height,
	const char *preset_path, const char *parameter_state_path);
VAEG_NATIVE_PRESENTER_RESULT vaeg_native_presenter_resize(
	VAEG_NATIVE_PRESENTER *presenter, uint32_t drawable_width, uint32_t drawable_height);
VAEG_NATIVE_PRESENTER_RESULT vaeg_native_presenter_present(
	VAEG_NATIVE_PRESENTER *presenter, const VAEG_FRAME_INPUT *frame);
void vaeg_native_presenter_destroy(VAEG_NATIVE_PRESENTER *presenter);
const char *vaeg_native_presenter_backend(const VAEG_NATIVE_PRESENTER *presenter);
const char *vaeg_native_presenter_state(const VAEG_NATIVE_PRESENTER *presenter);
const char *vaeg_native_presenter_error(const VAEG_NATIVE_PRESENTER *presenter);
int vaeg_native_presenter_gui_prepare(VAEG_NATIVE_PRESENTER *presenter);
void vaeg_native_presenter_gui_shutdown(VAEG_NATIVE_PRESENTER *presenter);
void vaeg_native_presenter_set_output_viewport(VAEG_NATIVE_PRESENTER *presenter,
                                               int x, int y, int width, int height);
int vaeg_native_presenter_set_filter(VAEG_NATIVE_PRESENTER *presenter, int enabled);
int vaeg_native_presenter_set_parameter(VAEG_NATIVE_PRESENTER *presenter,
                                         const char *name, float value);

#ifdef __cplusplus
}
#endif

#endif
