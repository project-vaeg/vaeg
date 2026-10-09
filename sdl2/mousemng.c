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
#include "mousemng.h"
#include "mousestate.h"

typedef struct {
	VAEG_MOUSE_STATE state;
	BOOL initialized;
	BOOL requested;
	BOOL focused;
	BOOL gui_blocked;
	BOOL relative;
	BOOL retry_blocked;
	char status[256];
} MOUSEMNG;

static MOUSEMNG mousemng;

static BOOL update_capture(BOOL retry) {
	BOOL desired;

	desired =
	    mousemng.initialized && mousemng.requested && mousemng.focused && !mousemng.gui_blocked;
	if (!desired) {
		if (mousemng.relative) {
			if (SDL_SetRelativeMouseMode(SDL_FALSE) < 0) {
				SPRINTF(mousemng.status, "Mouse release failed: %.220s", SDL_GetError());
				SDL_CaptureMouse(SDL_FALSE);
				SDL_ShowCursor(SDL_ENABLE);
			}
			mousemng.relative = FALSE;
		}
		vaeg_mouse_state_set_active(&mousemng.state, FALSE);
		return SUCCESS;
	}
	if (mousemng.relative) {
		return SUCCESS;
	}
	if (mousemng.retry_blocked && !retry) {
		return FAILURE;
	}
	if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0) {
		SPRINTF(mousemng.status, "Mouse capture failed: %.220s", SDL_GetError());
		mousemng.retry_blocked = TRUE;
		vaeg_mouse_state_set_active(&mousemng.state, FALSE);
		return FAILURE;
	}
	mousemng.relative = TRUE;
	mousemng.retry_blocked = FALSE;
	mousemng.status[0] = '\0';
	vaeg_mouse_state_set_active(&mousemng.state, TRUE);
	return SUCCESS;
}

BYTE mousemng_getstat(SINT16 *x, SINT16 *y, int clear) {
	return vaeg_mouse_state_getstat(&mousemng.state, x, y, clear);
}

void mousemng_initialize(void) {
	ZeroMemory(&mousemng, sizeof(mousemng));
	vaeg_mouse_state_initialize(&mousemng.state);
	mousemng.initialized = TRUE;
	mousemng.focused = (SDL_GetKeyboardFocus() != NULL) ? TRUE : FALSE;
}

void mousemng_shutdown(void) {
	if (!mousemng.initialized) {
		return;
	}
	mousemng.requested = FALSE;
	mousemng.gui_blocked = TRUE;
	update_capture(FALSE);
	mousemng.initialized = FALSE;
}

void mousemng_reset(void) {
	vaeg_mouse_state_reset(&mousemng.state);
}

BOOL mousemng_setcapture(BOOL capture) {
	mousemng.requested = capture ? TRUE : FALSE;
	mousemng.retry_blocked = FALSE;
	if (!mousemng.requested) {
		mousemng.status[0] = '\0';
	}
	return update_capture(TRUE);
}

BOOL mousemng_togglecapture(void) {
	mousemng_setcapture(mousemng.requested ? FALSE : TRUE);
	return mousemng.requested;
}

BOOL mousemng_getcapture(void) {
	return mousemng.requested;
}

BOOL mousemng_iscaptured(void) {
	return mousemng.relative;
}

void mousemng_setfocus(BOOL focused) {
	focused = focused ? TRUE : FALSE;
	if (mousemng.focused == focused) {
		return;
	}
	mousemng.focused = focused;
	mousemng.retry_blocked = FALSE;
	update_capture(focused);
}

void mousemng_setguiblocked(BOOL blocked) {
	blocked = blocked ? TRUE : FALSE;
	if (mousemng.gui_blocked == blocked) {
		update_capture(FALSE);
		return;
	}
	mousemng.gui_blocked = blocked;
	mousemng.retry_blocked = FALSE;
	update_capture(blocked ? FALSE : TRUE);
}

void mousemng_motion(SINT32 x, SINT32 y) {
	vaeg_mouse_state_motion(&mousemng.state, x, y);
}

void mousemng_button(UINT button, BOOL down) {
	vaeg_mouse_state_button(&mousemng.state, button, down);
}

const char *mousemng_status(void) {
	return mousemng.status;
}
