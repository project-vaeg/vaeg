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
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "librashader/frame_conversion.h"

#include <cstring>

namespace {

uint8_t expand5(uint32_t value) {
	return static_cast<uint8_t>((value * 255U) / 31U);
}

uint8_t expand6(uint32_t value) {
	return static_cast<uint8_t>((value * 255U) / 63U);
}

} // namespace

extern "C" VAEG_FRAME_CONVERSION_ERROR vaeg_frame_convert_rgba8888(
	const VAEG_FRAME_INPUT *input, void *destination, uint32_t destination_pitch,
	size_t destination_size) {
	uint32_t source_bytes;
	size_t required_size;
	const uint8_t *source;
	uint8_t *output;
	uint32_t y;

	if ((input == nullptr) || (destination == nullptr)) {
		return VAEG_FRAME_CONVERSION_NULL_DESTINATION;
	}
	if (vaeg_frame_input_validate(input) != VAEG_FRAME_INPUT_OK) {
		return VAEG_FRAME_CONVERSION_SHORT_DESTINATION;
	}
	source_bytes = (input->pixel_format == VAEG_FRAME_PIXEL_RGB565) ? 2 :
	               (input->pixel_format == VAEG_FRAME_PIXEL_ARGB8888) ? 4 : 0;
	if ((source_bytes == 0) || (destination_pitch < input->width * 4U)) {
		return VAEG_FRAME_CONVERSION_SHORT_DESTINATION_PITCH;
	}
	required_size = static_cast<size_t>(destination_pitch) * input->height;
	if (destination_size < required_size) {
		return VAEG_FRAME_CONVERSION_SHORT_DESTINATION;
	}
	source = static_cast<const uint8_t *>(input->pixels);
	output = static_cast<uint8_t *>(destination);
	for (y = 0; y < input->height; y++) {
		uint32_t source_y = (input->row_origin == VAEG_FRAME_ROWS_TOP_DOWN) ? y
		                                                                    : input->height - 1 - y;
		const uint8_t *source_row = source + (static_cast<size_t>(source_y) * input->pitch_bytes);
		uint8_t *output_row = output + (static_cast<size_t>(y) * destination_pitch);
		uint32_t x;

		for (x = 0; x < input->width; x++) {
			uint8_t *pixel = output_row + (x * 4U);
			if (input->pixel_format == VAEG_FRAME_PIXEL_RGB565) {
				uint32_t value = source_row[x * 2U] | (static_cast<uint32_t>(source_row[x * 2U + 1]) << 8);
				pixel[0] = expand5((value >> 11) & 0x1fU);
				pixel[1] = expand6((value >> 5) & 0x3fU);
				pixel[2] = expand5(value & 0x1fU);
				pixel[3] = 255;
			} else {
				uint32_t value = source_row[x * 4U] |
				                 (static_cast<uint32_t>(source_row[x * 4U + 1]) << 8) |
				                 (static_cast<uint32_t>(source_row[x * 4U + 2]) << 16) |
				                 (static_cast<uint32_t>(source_row[x * 4U + 3]) << 24);
				pixel[0] = static_cast<uint8_t>((value >> 16) & 0xffU);
				pixel[1] = static_cast<uint8_t>((value >> 8) & 0xffU);
				pixel[2] = static_cast<uint8_t>(value & 0xffU);
				pixel[3] = static_cast<uint8_t>((value >> 24) & 0xffU);
			}
		}
	}
	(void)source_bytes;
	return VAEG_FRAME_CONVERSION_OK;
}
