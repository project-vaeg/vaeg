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
#ifndef VAEG_SDL2_LIBRASHADER_FRAME_INPUT_H
#define VAEG_SDL2_LIBRASHADER_FRAME_INPUT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VAEG_FRAME_PIXEL_RGB565 = 1,
	VAEG_FRAME_PIXEL_ARGB8888 = 2
} VAEG_FRAME_PIXEL_FORMAT;

typedef enum {
	VAEG_FRAME_ROWS_TOP_DOWN = 0,
	VAEG_FRAME_ROWS_BOTTOM_UP = 1
} VAEG_FRAME_ROW_ORIGIN;

typedef enum {
	VAEG_FRAME_INPUT_OK = 0,
	VAEG_FRAME_INPUT_NULL_PIXELS,
	VAEG_FRAME_INPUT_ZERO_EXTENT,
	VAEG_FRAME_INPUT_UNKNOWN_FORMAT,
	VAEG_FRAME_INPUT_SHORT_PITCH,
	VAEG_FRAME_INPUT_INVALID_ASPECT,
	VAEG_FRAME_INPUT_INVALID_RATE,
	VAEG_FRAME_INPUT_INVALID_DELTA
} VAEG_FRAME_INPUT_ERROR;

typedef struct {
	const void *pixels;
	uint32_t width;
	uint32_t height;
	uint32_t pitch_bytes;
	VAEG_FRAME_PIXEL_FORMAT pixel_format;
	VAEG_FRAME_ROW_ORIGIN row_origin;
	uint32_t source_aspect_width;
	uint32_t source_aspect_height;
	uint32_t source_frame_rate_numerator;
	uint32_t source_frame_rate_denominator;
	uint64_t frame_number;
	uint64_t frame_time_delta_ns;
} VAEG_FRAME_INPUT;

/* Optional one-shot output readback, RGBA bytes, top-down. Not a raw frame. */
typedef struct {
	void *pixels;
	uint32_t width, height, pitch_bytes;
	int complete;
} VAEG_OUTPUT_CAPTURE;

void vaeg_frame_input_initialize(VAEG_FRAME_INPUT *input, const void *pixels, uint32_t width,
                                 uint32_t height, uint32_t pitch_bytes,
                                 VAEG_FRAME_PIXEL_FORMAT pixel_format,
                                 VAEG_FRAME_ROW_ORIGIN row_origin,
                                 uint32_t source_aspect_width, uint32_t source_aspect_height,
                                 uint32_t source_frame_rate_numerator,
                                 uint32_t source_frame_rate_denominator, uint64_t frame_number,
                                 uint64_t frame_time_delta_ns);
VAEG_FRAME_INPUT_ERROR vaeg_frame_input_validate(const VAEG_FRAME_INPUT *input);
const char *vaeg_frame_input_error_name(VAEG_FRAME_INPUT_ERROR error);

#ifdef __cplusplus
}
#endif

#endif
