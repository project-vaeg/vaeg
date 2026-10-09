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
#include "mousestate.h"

static SINT64 saturating_add(SINT64 value, SINT32 delta) {
	if ((delta > 0) && (value > INT64_MAX - delta)) {
		return INT64_MAX;
	}
	if ((delta < 0) && (value < INT64_MIN - delta)) {
		return INT64_MIN;
	}
	return value + delta;
}

static SINT16 axis_value(SINT64 *value, int clear) {
	SINT64 scaled;
	SINT16 result;

	scaled = *value / 2;
	if (scaled > INT16_MAX) {
		result = INT16_MAX;
	} else if (scaled < INT16_MIN) {
		result = INT16_MIN;
	} else {
		result = (SINT16)scaled;
	}
	if (clear) {
		*value -= (SINT64)result * 2;
	}
	return result;
}

void vaeg_mouse_state_initialize(VAEG_MOUSE_STATE *state) {
	ZeroMemory(state, sizeof(*state));
	state->buttons = VAEG_MOUSE_RELEASED;
}

void vaeg_mouse_state_reset(VAEG_MOUSE_STATE *state) {
	BOOL active;

	active = state->active;
	vaeg_mouse_state_initialize(state);
	state->active = active;
}

void vaeg_mouse_state_set_active(VAEG_MOUSE_STATE *state, BOOL active) {
	if (!active) {
		vaeg_mouse_state_initialize(state);
		return;
	}
	state->active = TRUE;
}

void vaeg_mouse_state_motion(VAEG_MOUSE_STATE *state, SINT32 x, SINT32 y) {
	if (!state->active) {
		return;
	}
	state->x = saturating_add(state->x, x);
	state->y = saturating_add(state->y, y);
}

void vaeg_mouse_state_button(VAEG_MOUSE_STATE *state, UINT button, BOOL down) {
	BYTE bit;

	switch (button) {
	case VAEG_MOUSE_BUTTON_LEFT:
		bit = VAEG_MOUSE_LEFTBIT;
		break;

	case VAEG_MOUSE_BUTTON_RIGHT:
		bit = VAEG_MOUSE_RIGHTBIT;
		break;

	default:
		return;
	}
	if (down) {
		if (state->active) {
			state->buttons &= (BYTE)~bit;
		}
	} else {
		state->buttons |= bit;
	}
}

BYTE vaeg_mouse_state_getstat(VAEG_MOUSE_STATE *state, SINT16 *x, SINT16 *y, int clear) {
	*x = axis_value(&state->x, clear);
	*y = axis_value(&state->y, clear);
	return state->buttons;
}
