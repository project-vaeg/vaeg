/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_SDL2_VIEWPORT_H
#define VAEG_SDL2_VIEWPORT_H

enum {
	VAEG_SCALING_NATIVE = 0,
	VAEG_SCALING_FIT,
	VAEG_SCALING_FIT_8DOT,
	VAEG_SCALING_INTEGER,
	VAEG_SCALING_STRETCH,
	VAEG_SCALING_COUNT
};

typedef struct {
	int guest_width;
	int guest_height;
	int drawable_width;
	int drawable_height;
	int menu_inset;
	int scaling;
	BOOL aspect;
} VAEG_VIEWPORT_INPUT;

typedef struct {
	int x;
	int y;
	int width;
	int height;
	double scale_x;
	double scale_y;
	BOOL valid;
} VAEG_VIEWPORT;

#ifdef __cplusplus
extern "C" {
#endif

BOOL vaeg_viewport_calculate(const VAEG_VIEWPORT_INPUT *input, VAEG_VIEWPORT *viewport);
BOOL vaeg_viewport_map_point(const VAEG_VIEWPORT *viewport, int guest_width, int guest_height,
                             int drawable_x, int drawable_y, int *guest_x, int *guest_y);
UINT8 vaeg_fscrnmod_sanitize(UINT value, BOOL *masked);
void vaeg_fullscreen_size(UINT fscrn_cx, UINT fscrn_cy, UINT8 fscrnmod, int current_width,
                          int current_height, int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif
