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
#include "librashader/frame_input.h"

namespace {

uint32_t bytes_per_pixel(VAEG_FRAME_PIXEL_FORMAT format) {
	switch (format) {
	case VAEG_FRAME_PIXEL_RGB565:
		return 2;
	case VAEG_FRAME_PIXEL_ARGB8888:
		return 4;
	default:
		return 0;
	}
}

} // namespace

extern "C" void vaeg_frame_input_initialize(
	VAEG_FRAME_INPUT *input, const void *pixels, uint32_t width, uint32_t height,
	uint32_t pitch_bytes, VAEG_FRAME_PIXEL_FORMAT pixel_format, VAEG_FRAME_ROW_ORIGIN row_origin,
	uint32_t source_aspect_width, uint32_t source_aspect_height,
	uint32_t source_frame_rate_numerator, uint32_t source_frame_rate_denominator,
	uint64_t frame_number, uint64_t frame_time_delta_ns) {
	if (input == nullptr) {
		return;
	}
	input->pixels = pixels;
	input->width = width;
	input->height = height;
	input->pitch_bytes = pitch_bytes;
	input->pixel_format = pixel_format;
	input->row_origin = row_origin;
	input->source_aspect_width = source_aspect_width;
	input->source_aspect_height = source_aspect_height;
	input->source_frame_rate_numerator = source_frame_rate_numerator;
	input->source_frame_rate_denominator = source_frame_rate_denominator;
	input->frame_number = frame_number;
	input->frame_time_delta_ns = frame_time_delta_ns;
}

extern "C" VAEG_FRAME_INPUT_ERROR vaeg_frame_input_validate(const VAEG_FRAME_INPUT *input) {
	uint32_t pixel_bytes;
	uint64_t minimum_pitch;

	if ((input == nullptr) || (input->pixels == nullptr)) {
		return VAEG_FRAME_INPUT_NULL_PIXELS;
	}
	if ((input->width == 0) || (input->height == 0)) {
		return VAEG_FRAME_INPUT_ZERO_EXTENT;
	}
	pixel_bytes = bytes_per_pixel(input->pixel_format);
	if (pixel_bytes == 0) {
		return VAEG_FRAME_INPUT_UNKNOWN_FORMAT;
	}
	minimum_pitch = static_cast<uint64_t>(input->width) * pixel_bytes;
	if ((minimum_pitch > UINT32_MAX) || (input->pitch_bytes < minimum_pitch)) {
		return VAEG_FRAME_INPUT_SHORT_PITCH;
	}
	if ((input->source_aspect_width == 0) || (input->source_aspect_height == 0)) {
		return VAEG_FRAME_INPUT_INVALID_ASPECT;
	}
	if ((input->source_frame_rate_numerator == 0) ||
	    (input->source_frame_rate_denominator == 0)) {
		return VAEG_FRAME_INPUT_INVALID_RATE;
	}
	if (input->frame_time_delta_ns == 0) {
		return VAEG_FRAME_INPUT_INVALID_DELTA;
	}
	return VAEG_FRAME_INPUT_OK;
}

extern "C" const char *vaeg_frame_input_error_name(VAEG_FRAME_INPUT_ERROR error) {
	switch (error) {
	case VAEG_FRAME_INPUT_OK:
		return "ok";
	case VAEG_FRAME_INPUT_NULL_PIXELS:
		return "null_pixels";
	case VAEG_FRAME_INPUT_ZERO_EXTENT:
		return "zero_extent";
	case VAEG_FRAME_INPUT_UNKNOWN_FORMAT:
		return "unknown_format";
	case VAEG_FRAME_INPUT_SHORT_PITCH:
		return "short_pitch";
	case VAEG_FRAME_INPUT_INVALID_ASPECT:
		return "invalid_aspect";
	case VAEG_FRAME_INPUT_INVALID_RATE:
		return "invalid_rate";
	case VAEG_FRAME_INPUT_INVALID_DELTA:
		return "invalid_delta";
	default:
		return "unknown";
	}
}
