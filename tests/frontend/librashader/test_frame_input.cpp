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
#include <cassert>
#include <cstdint>

#include "librashader/frame_input.h"

int main() {
	std::uint16_t pixels[4] = {0x0000, 0x1234, 0xabcd, 0xffff};
	VAEG_FRAME_INPUT frame = {pixels, 2, 2, 4, VAEG_FRAME_PIXEL_RGB565,
	                          VAEG_FRAME_ROWS_TOP_DOWN, 4, 3, 60, 1, 7, 16666667};

	assert(vaeg_frame_input_validate(&frame) == VAEG_FRAME_INPUT_OK);
	assert(vaeg_frame_input_error_name(VAEG_FRAME_INPUT_OK) != nullptr);
	frame.pitch_bytes = 2;
	assert(vaeg_frame_input_validate(&frame) == VAEG_FRAME_INPUT_SHORT_PITCH);
	frame.pitch_bytes = 4;
	frame.frame_time_delta_ns = 0;
	assert(vaeg_frame_input_validate(&frame) == VAEG_FRAME_INPUT_INVALID_DELTA);
	frame.frame_time_delta_ns = 16666667;
	frame.source_aspect_height = 0;
	assert(vaeg_frame_input_validate(&frame) == VAEG_FRAME_INPUT_INVALID_ASPECT);
	return 0;
}
