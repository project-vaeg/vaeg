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
#include "sdlapi.h"
#include "parts.h"
#include "soundmng.h"
#include "soundopts.h"
#include "sound.h"
#if defined(VERMOUTH_LIB)
#include "commng.h"
#include "cmver.h"
#endif

#define NSNDBUF 2

typedef struct {
	BOOL opened;
	BOOL enabled;
	SDL_AudioDeviceID device;
	int nsndbuf;
	int samples;
	SINT16 *buf[NSNDBUF];
} SOUNDMNG;

static SOUNDMNG soundmng;

static void sound_play_cb(void *userdata, Uint8 *stream, int len) {
	int length;
	SINT16 *dst;
	const SINT32 *src;

	length = min(len, (int)(soundmng.samples * 2 * sizeof(SINT16)));
	dst = soundmng.buf[soundmng.nsndbuf];
	src = sound_pcmlock();
	if (src) {
		satuation_s16(dst, src, length);
		sound_pcmunlock(src);
	} else {
		ZeroMemory(dst, length);
	}
	ZeroMemory(stream, len);
	SDL_MixAudioFormat(stream, (Uint8 *)dst, AUDIO_S16SYS, length, SDL_MIX_MAXVOLUME);
	soundmng.nsndbuf = (soundmng.nsndbuf + 1) % NSNDBUF;
	(void)userdata;
}

UINT soundmng_create(UINT rate, UINT ms) {
	SDL_AudioSpec want;
	SDL_AudioSpec have;
	UINT s;
	UINT samples;
	SINT16 *tmp;

	if (soundmng.opened) {
		goto smcre_err1;
	}
	if (SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
		fprintf(stderr, "Error: SDL audio init: %s\n", SDL_GetError());
		goto smcre_err1;
	}

	samples = vaeg_sound_buffer_samples(rate, ms, NSNDBUF);
	if (samples == 0) {
		goto smcre_err1;
	}
	soundmng.nsndbuf = 0;
	soundmng.samples = samples;
	for (s = 0; s < NSNDBUF; s++) {
		tmp = (SINT16 *)_MALLOC(samples * 2 * sizeof(SINT16), "buf");
		if (tmp == NULL) {
			goto smcre_err2;
		}
		soundmng.buf[s] = tmp;
		ZeroMemory(tmp, samples * 2 * sizeof(SINT16));
	}

	ZeroMemory(&want, sizeof(want));
	want.freq = rate;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = (Uint16)samples;
	want.callback = sound_play_cb;
	soundmng.device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (soundmng.device == 0) {
		fprintf(stderr, "Error: SDL_OpenAudioDevice: %s\n", SDL_GetError());
		goto smcre_err2;
	}
#if defined(VERMOUTH_LIB)
	cmvermouth_load(rate);
#endif
	soundmng.opened = TRUE;
	return (samples);

smcre_err2:
	for (s = 0; s < NSNDBUF; s++) {
		tmp = soundmng.buf[s];
		soundmng.buf[s] = NULL;
		if (tmp) {
			_MFREE(tmp);
		}
	}

smcre_err1:
	return (0);
}

void soundmng_destroy(void) {
	int i;
	SINT16 *tmp;

	if (soundmng.opened) {
		soundmng.opened = FALSE;
		SDL_PauseAudioDevice(soundmng.device, 1);
		SDL_CloseAudioDevice(soundmng.device);
		soundmng.device = 0;
		for (i = 0; i < NSNDBUF; i++) {
			tmp = soundmng.buf[i];
			soundmng.buf[i] = NULL;
			if (tmp) {
				_MFREE(tmp);
			}
		}
#if defined(VERMOUTH_LIB)
//		cmvermouth_unload();
#endif
	}
}

void soundmng_play(void) {
	if (soundmng.opened) {
		SDL_PauseAudioDevice(soundmng.device, soundmng.enabled ? 0 : 1);
	}
}

void soundmng_stop(void) {
	if (soundmng.opened) {
		SDL_PauseAudioDevice(soundmng.device, 1);
	}
}

void soundmng_setenabled(BOOL enabled) {
	soundmng.enabled = enabled ? TRUE : FALSE;
	if (soundmng.opened) {
		SDL_PauseAudioDevice(soundmng.device, soundmng.enabled ? 0 : 1);
	}
}

BOOL soundmng_isenabled(void) {
	return (soundmng.enabled);
}

void soundmng_initialize(void) {
	soundmng.enabled = TRUE;
}

void soundmng_deinitialize(void) {
#if defined(VERMOUTH_LIB)
	cmvermouth_unload();
#endif
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
