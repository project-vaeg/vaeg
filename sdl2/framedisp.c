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
#include "framedisp.h"

enum {
	VAEG_FRAMEDISP_INTERVAL_MS = 2000
};

void vaeg_framedisp_reset(VAEG_FRAMEDISP *state, UINT32 tick, UINT32 draws) {
	if (state == NULL) {
		return;
	}
	state->tick = tick;
	state->draws = draws;
	state->fps_tenths = 0;
}

BOOL vaeg_framedisp_update(VAEG_FRAMEDISP *state, UINT32 tick, UINT32 draws) {
	UINT32 elapsed;
	UINT32 draw_delta;
	UINT64 fps_tenths;

	if (state == NULL) {
		return (FALSE);
	}
	elapsed = tick - state->tick;
	if (elapsed < VAEG_FRAMEDISP_INTERVAL_MS) {
		return (FALSE);
	}
	draw_delta = draws - state->draws;
	fps_tenths = ((UINT64)draw_delta * 10000) / elapsed;
	if (fps_tenths > 0xffffffffULL) {
		fps_tenths = 0xffffffffULL;
	}
	state->tick = tick;
	state->draws = draws;
	state->fps_tenths = (UINT32)fps_tenths;
	return (TRUE);
}
