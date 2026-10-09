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
#include <cstring>

#include "librashader/frame_input.h"

int main() {
	std::uint8_t pixels[12] = {0x00, 0x00, 0x34, 0x12, 0xcd, 0xab,
	                           0xff, 0xff, 0x5a, 0xa5, 0xc3, 0x3c};
	std::uint8_t before[sizeof(pixels)];
	VAEG_FRAME_INPUT frame;

	std::memcpy(before, pixels, sizeof(pixels));
	vaeg_frame_input_initialize(&frame, pixels, 2, 2, 6, VAEG_FRAME_PIXEL_RGB565,
	                            VAEG_FRAME_ROWS_TOP_DOWN, 4, 3, 60, 1, 19, 16666667);
	assert(frame.pixels == pixels);
	assert(frame.pitch_bytes == 6);
	assert(frame.pixel_format == VAEG_FRAME_PIXEL_RGB565);
	assert(frame.frame_number == 19);
	assert(vaeg_frame_input_validate(&frame) == VAEG_FRAME_INPUT_OK);
	assert(std::memcmp(before, pixels, sizeof(pixels)) == 0);
	return 0;
}
