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
#include "soundopts.h"

BOOL vaeg_sound_rate_valid(UINT rate) {
	return ((rate == 11025) || (rate == 22050) || (rate == 44100));
}

BOOL vaeg_sound_buffer_valid(UINT ms) {
	return ((ms >= VAEG_SOUND_BUFFER_MIN_MS) && (ms <= VAEG_SOUND_BUFFER_MAX_MS));
}

UINT vaeg_sound_buffer_clamp(UINT ms) {
	if (ms < VAEG_SOUND_BUFFER_MIN_MS) {
		return (VAEG_SOUND_BUFFER_MIN_MS);
	}
	if (ms > VAEG_SOUND_BUFFER_MAX_MS) {
		return (VAEG_SOUND_BUFFER_MAX_MS);
	}
	return (ms);
}

UINT vaeg_sound_buffer_samples(UINT rate, UINT ms, UINT buffer_count) {
	UINT64 target;
	UINT samples;

	if (!vaeg_sound_rate_valid(rate) || !vaeg_sound_buffer_valid(ms) || (buffer_count == 0)) {
		return (0);
	}
	target = ((UINT64)rate * ms) / ((UINT64)buffer_count * 1000);
	samples = 1;
	while ((UINT64)samples < target) {
		samples <<= 1;
	}
	return (samples);
}
