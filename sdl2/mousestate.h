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
#ifndef VAEG_SDL2_MOUSESTATE_H
#define VAEG_SDL2_MOUSESTATE_H

enum {
	VAEG_MOUSE_BUTTON_LEFT = 0,
	VAEG_MOUSE_BUTTON_RIGHT
};

enum {
	VAEG_MOUSE_LEFTBIT = 0x80,
	VAEG_MOUSE_RIGHTBIT = 0x20,
	VAEG_MOUSE_RELEASED = VAEG_MOUSE_LEFTBIT | VAEG_MOUSE_RIGHTBIT
};

typedef struct {
	SINT64 x;
	SINT64 y;
	BYTE buttons;
	BOOL active;
} VAEG_MOUSE_STATE;

#ifdef __cplusplus
extern "C" {
#endif

void vaeg_mouse_state_initialize(VAEG_MOUSE_STATE *state);
void vaeg_mouse_state_reset(VAEG_MOUSE_STATE *state);
void vaeg_mouse_state_set_active(VAEG_MOUSE_STATE *state, BOOL active);
void vaeg_mouse_state_motion(VAEG_MOUSE_STATE *state, SINT32 x, SINT32 y);
void vaeg_mouse_state_button(VAEG_MOUSE_STATE *state, UINT button, BOOL down);
BYTE vaeg_mouse_state_getstat(VAEG_MOUSE_STATE *state, SINT16 *x, SINT16 *y, int clear);

#ifdef __cplusplus
}
#endif

#endif
