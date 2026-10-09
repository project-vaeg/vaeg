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
#ifndef VAEG_SDL2_SOUNDOPTS_H
#define VAEG_SDL2_SOUNDOPTS_H

enum {
	VAEG_SOUND_RATE_DEFAULT = 22050,
	VAEG_SOUND_BUFFER_DEFAULT_MS = 500,
	VAEG_SOUND_BUFFER_MIN_MS = 40,
	VAEG_SOUND_BUFFER_MAX_MS = 1000
};

#ifdef __cplusplus
extern "C" {
#endif

BOOL vaeg_sound_rate_valid(UINT rate);
BOOL vaeg_sound_buffer_valid(UINT ms);
UINT vaeg_sound_buffer_clamp(UINT ms);
UINT vaeg_sound_buffer_samples(UINT rate, UINT ms, UINT buffer_count);

#ifdef __cplusplus
}
#endif

#endif
