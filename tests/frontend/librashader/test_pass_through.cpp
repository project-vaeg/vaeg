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
#include <vector>

#include "compiler.h"
#undef max
#undef min
#include "librashader/frame_conversion.h"
#include "librashader/presenter_factory.h"
#include "viewport.h"

using namespace vaeg::librashader;

int main() {
	const std::uint8_t source[4] = {0x00, 0xf8, 0xe0, 0x07};
	std::uint8_t output[8] = {};
	VAEG_FRAME_INPUT frame;
	VAEG_VIEWPORT_INPUT viewport_input = {640, 400, 1920, 1080, 0, VAEG_SCALING_FIT, TRUE};
	VAEG_VIEWPORT viewport;
	int guest_x;
	int guest_y;

	vaeg_frame_input_initialize(&frame, source, 2, 1, 4, VAEG_FRAME_PIXEL_RGB565,
	                            VAEG_FRAME_ROWS_TOP_DOWN, 4, 3, 60, 1, 1, 16666667);
	assert(vaeg_frame_convert_rgba8888(&frame, output, 8, sizeof(output)) ==
	       VAEG_FRAME_CONVERSION_OK);
	assert(output[0] == 255 && output[1] == 0 && output[2] == 0 && output[3] == 255);
	assert(output[4] == 0 && output[5] == 255 && output[6] == 0 && output[7] == 255);
	assert(vaeg_frame_convert_rgba8888(&frame, output, 4, sizeof(output)) ==
	       VAEG_FRAME_CONVERSION_SHORT_DESTINATION_PITCH);
	assert(vaeg_frame_convert_rgba8888(&frame, output, 8, 4) ==
	       VAEG_FRAME_CONVERSION_SHORT_DESTINATION);

	assert(vaeg_viewport_calculate(&viewport_input, &viewport) == SUCCESS);
	assert(viewport.valid && viewport.x == 240 && viewport.y == 0 && viewport.width == 1440 &&
	       viewport.height == 1080);
	assert(vaeg_viewport_map_point(&viewport, 640, 400, 960, 540, &guest_x, &guest_y) == SUCCESS);
	assert(guest_x == 320 && guest_y == 200);

	auto presenter = create_native_presenter(PresenterBackend::Automatic);
	assert(presenter->initialize({nullptr, 640, 400, PresenterBackend::Automatic, false, nullptr}) ==
	       PresenterResult::Fallback);
	assert(presenter->set_filter_enabled(false) == PresenterResult::Fallback);
	assert(presenter->present(frame) == PresenterResult::Fallback);
	presenter->shutdown();
	presenter->shutdown();
	assert(presenter->recover() == PresenterResult::Fallback);
	return 0;
}
