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
#include <cstdio>

#include "librashader/native_presenter_controller.h"

static int expect(bool condition, const char *message) {
	if (!condition) {
		std::fprintf(stderr, "fallback test failed: %s\n", message);
		return 1;
	}
	return 0;
}

int main() {
	int failures = 0;
	VAEG_FRAME_INPUT invalid_frame{};

	failures += expect(vaeg_native_presenter_is_headless_video_driver("dummy") != 0,
	                   "dummy video must bypass native presentation");
	failures += expect(vaeg_native_presenter_is_headless_video_driver("x11") == 0,
	                   "normal video must not be classified as headless");
	failures += expect(vaeg_native_presenter_is_headless_video_driver(nullptr) == 0,
	                   "unknown video driver must not be classified as headless");
	failures += expect(vaeg_native_presenter_create(nullptr, 0, 0, "missing.slangp", nullptr) == nullptr,
	                   "null window must fail closed");
	failures += expect(vaeg_native_presenter_resize(nullptr, 640, 400) ==
	                       VAEG_NATIVE_PRESENTER_FALLBACK,
	                   "resize without a presenter must fall back");
	failures += expect(vaeg_native_presenter_present(nullptr, &invalid_frame) ==
	                       VAEG_NATIVE_PRESENTER_FALLBACK,
	                   "present without a presenter must fall back");
	std::puts((failures == 0) ? "fallback checks passed" : "fallback checks failed");
	return failures == 0 ? 0 : 1;
}
