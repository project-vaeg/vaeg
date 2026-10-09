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
#include "kbdinject.h"
#include "kbdmap.h"
#include "kbdpaste.h"

enum {
	KBDPASTE_EVENT_INTERVAL_MS = 20,
	KBDPASTE_STATUS_SIZE = 160
};

typedef enum {
	KBDPASTE_PHASE_IDLE = 0,
	KBDPASTE_PHASE_SHIFT_DOWN,
	KBDPASTE_PHASE_KEY_DOWN,
	KBDPASTE_PHASE_KEY_UP,
	KBDPASTE_PHASE_SHIFT_UP
} KBDPASTE_PHASE;

typedef struct {
	KBDPASTE_ACTION *actions;
	size_t count;
	size_t index;
	UINT skipped;
	UINT32 next_tick;
	KBDPASTE_PHASE phase;
	BOOL shift_down;
	BOOL key_down;
	char status[KBDPASTE_STATUS_SIZE];
} KBDPASTE_STATE;

static KBDPASTE_STATE paste;

static BYTE role_code(KBDMAP_ROLE role) {
	return kbdmap_guest_code(role);
}

static BOOL map_ascii(BYTE ch, KBDPASTE_ACTION *action) {
	static const KBDMAP_ROLE digit_roles[] = {KBDROLE_0, KBDROLE_1, KBDROLE_2, KBDROLE_3,
	                                          KBDROLE_4, KBDROLE_5, KBDROLE_6, KBDROLE_7,
	                                          KBDROLE_8, KBDROLE_9};
	static const KBDMAP_ROLE letter_roles[] = {
	    KBDROLE_A, KBDROLE_B, KBDROLE_C, KBDROLE_D, KBDROLE_E, KBDROLE_F, KBDROLE_G,
	    KBDROLE_H, KBDROLE_I, KBDROLE_J, KBDROLE_K, KBDROLE_L, KBDROLE_M, KBDROLE_N,
	    KBDROLE_O, KBDROLE_P, KBDROLE_Q, KBDROLE_R, KBDROLE_S, KBDROLE_T, KBDROLE_U,
	    KBDROLE_V, KBDROLE_W, KBDROLE_X, KBDROLE_Y, KBDROLE_Z};
	KBDMAP_ROLE role;
	BOOL shift;

	shift = FALSE;
	if ((ch >= '0') && (ch <= '9')) {
		role = digit_roles[ch - '0'];
	} else if ((ch >= 'a') && (ch <= 'z')) {
		role = letter_roles[ch - 'a'];
	} else if ((ch >= 'A') && (ch <= 'Z')) {
		role = letter_roles[ch - 'A'];
		shift = TRUE;
	} else {
		switch (ch) {
		case ' ':
			role = KBDROLE_SPACE;
			break;
		case '!':
			role = KBDROLE_1;
			shift = TRUE;
			break;
		case '"':
			role = KBDROLE_2;
			shift = TRUE;
			break;
		case '#':
			role = KBDROLE_3;
			shift = TRUE;
			break;
		case '$':
			role = KBDROLE_4;
			shift = TRUE;
			break;
		case '%':
			role = KBDROLE_5;
			shift = TRUE;
			break;
		case '&':
			role = KBDROLE_6;
			shift = TRUE;
			break;
		case '\'':
			role = KBDROLE_7;
			shift = TRUE;
			break;
		case '(':
			role = KBDROLE_8;
			shift = TRUE;
			break;
		case ')':
			role = KBDROLE_9;
			shift = TRUE;
			break;
		case '*':
			role = KBDROLE_COLON;
			shift = TRUE;
			break;
		case '+':
			role = KBDROLE_SEMICOLON;
			shift = TRUE;
			break;
		case ',':
			role = KBDROLE_COMMA;
			break;
		case '-':
			role = KBDROLE_MINUS;
			break;
		case '.':
			role = KBDROLE_PERIOD;
			break;
		case '/':
			role = KBDROLE_SLASH;
			break;
		case ':':
			role = KBDROLE_COLON;
			break;
		case ';':
			role = KBDROLE_SEMICOLON;
			break;
		case '<':
			role = KBDROLE_COMMA;
			shift = TRUE;
			break;
		case '=':
			role = KBDROLE_MINUS;
			shift = TRUE;
			break;
		case '>':
			role = KBDROLE_PERIOD;
			shift = TRUE;
			break;
		case '?':
			role = KBDROLE_SLASH;
			shift = TRUE;
			break;
		case '@':
			role = KBDROLE_AT;
			break;
		case '[':
			role = KBDROLE_BRACKETLEFT;
			break;
		case '\\':
			role = KBDROLE_YEN;
			break;
		case ']':
			role = KBDROLE_BRACKETRIGHT;
			break;
		case '^':
			role = KBDROLE_CARET;
			break;
		case '_':
			role = KBDROLE_UNDERSCORE;
			shift = TRUE;
			break;
		case '`':
			role = KBDROLE_CARET;
			shift = TRUE;
			break;
		case '{':
			role = KBDROLE_BRACKETLEFT;
			shift = TRUE;
			break;
		case '|':
			role = KBDROLE_YEN;
			shift = TRUE;
			break;
		case '}':
			role = KBDROLE_BRACKETRIGHT;
			shift = TRUE;
			break;
		case '~':
			role = KBDROLE_AT;
			shift = TRUE;
			break;
		default:
			return FALSE;
		}
	}
	action->guest_code = role_code(role);
	action->shift = shift;
	return action->guest_code != KBDMAP_NC;
}

static size_t utf8_character_length(const BYTE *text) {
	BYTE ch;
	size_t expected;
	size_t length;

	ch = text[0];
	if ((ch & 0xe0) == 0xc0) {
		expected = 2;
	} else if ((ch & 0xf0) == 0xe0) {
		expected = 3;
	} else if ((ch & 0xf8) == 0xf0) {
		expected = 4;
	} else {
		return 1;
	}
	for (length = 1; length < expected; length++) {
		if ((text[length] == '\0') || ((text[length] & 0xc0) != 0x80)) {
			break;
		}
	}
	return length;
}

size_t kbdpaste_map_text(const char *text, KBDPASTE_ACTION *actions, size_t capacity,
                         UINT *skipped) {
	const BYTE *src;
	size_t count;
	UINT dropped;

	count = 0;
	dropped = 0;
	if (text == NULL) {
		if (skipped != NULL) {
			*skipped = 0;
		}
		return 0;
	}
	src = (const BYTE *)text;
	while (*src != '\0') {
		KBDPASTE_ACTION action;
		BOOL mapped;

		mapped = FALSE;
		if (*src == '\r') {
			action.guest_code = role_code(KBDROLE_RETURNL);
			action.shift = FALSE;
			mapped = action.guest_code != KBDMAP_NC;
			src++;
			if (*src == '\n') {
				src++;
			}
		} else if (*src == '\n') {
			action.guest_code = role_code(KBDROLE_RETURNL);
			action.shift = FALSE;
			mapped = action.guest_code != KBDMAP_NC;
			src++;
		} else if (*src < 0x80) {
			mapped = map_ascii(*src, &action);
			src++;
		} else {
			src += utf8_character_length(src);
		}
		if (!mapped) {
			dropped++;
			continue;
		}
		if ((actions != NULL) && (count < capacity)) {
			actions[count] = action;
		}
		count++;
	}
	if (skipped != NULL) {
		*skipped = dropped;
	}
	return count;
}

static void release_held_keys(void) {
	if (paste.key_down && (paste.index < paste.count)) {
		kbdinject_keyup(paste.actions[paste.index].guest_code);
	}
	if (paste.shift_down) {
		kbdinject_keyup(role_code(KBDROLE_SHIFTL));
	}
	paste.key_down = FALSE;
	paste.shift_down = FALSE;
}

static void clear_queue(void) {
	release_held_keys();
	if (paste.actions != NULL) {
		_MFREE(paste.actions);
	}
	paste.actions = NULL;
	paste.count = 0;
	paste.index = 0;
	paste.phase = KBDPASTE_PHASE_IDLE;
	paste.next_tick = 0;
}

void kbdpaste_initialize(void) {
	ZeroMemory(&paste, sizeof(paste));
}

void kbdpaste_shutdown(void) {
	clear_queue();
}

BOOL kbdpaste_start_text(const char *text) {
	size_t capacity;
	size_t count;
	UINT skipped;
	KBDPASTE_ACTION *actions;

	if ((text == NULL) || (text[0] == '\0')) {
		return FALSE;
	}
	capacity = strlen(text);
	if (capacity > (SIZE_MAX / sizeof(KBDPASTE_ACTION))) {
		clear_queue();
		milstr_ncpy(paste.status, "Paste failed: clipboard is too large.",
		            (int)sizeof(paste.status));
		return FALSE;
	}
	actions = (KBDPASTE_ACTION *)_MALLOC(capacity * sizeof(KBDPASTE_ACTION), "clipboard-paste");
	if (actions == NULL) {
		clear_queue();
		milstr_ncpy(paste.status, "Paste failed: out of memory.", (int)sizeof(paste.status));
		return FALSE;
	}
	count = kbdpaste_map_text(text, actions, capacity, &skipped);
	clear_queue();
	if (count == 0) {
		_MFREE(actions);
		snprintf(paste.status, sizeof(paste.status), "Paste skipped %u unsupported characters.",
		         skipped);
		return FALSE;
	}
	paste.actions = actions;
	paste.count = count;
	paste.skipped = skipped;
	paste.phase = actions[0].shift ? KBDPASTE_PHASE_SHIFT_DOWN : KBDPASTE_PHASE_KEY_DOWN;
	paste.next_tick = SDL_GetTicks();
	kbdinject_allrelease();
	if (skipped != 0) {
		snprintf(paste.status, sizeof(paste.status), "Pasting %u characters (%u skipped).",
		         (UINT)count, skipped);
	} else {
		snprintf(paste.status, sizeof(paste.status), "Pasting %u characters.", (UINT)count);
	}
	return TRUE;
}

BOOL kbdpaste_start_clipboard(void) {
	char *text;
	BOOL started;

	text = SDL_GetClipboardText();
	if (text == NULL) {
		clear_queue();
		snprintf(paste.status, sizeof(paste.status), "Paste failed: %s", SDL_GetError());
		return FALSE;
	}
	started = kbdpaste_start_text(text);
	SDL_free(text);
	return started;
}

void kbdpaste_tick(UINT32 now, BOOL paused) {
	KBDPASTE_ACTION *action;
	BOOL cleanup_only;

	if ((paste.actions == NULL) || ((SINT32)(now - paste.next_tick) < 0)) {
		return;
	}
	action = &paste.actions[paste.index];
	cleanup_only = FALSE;
	if (paused) {
		if (paste.phase == KBDPASTE_PHASE_KEY_UP) {
			kbdinject_keyup(action->guest_code);
			paste.key_down = FALSE;
			paste.phase = action->shift ? KBDPASTE_PHASE_SHIFT_UP : KBDPASTE_PHASE_IDLE;
			cleanup_only = TRUE;
		} else if (paste.phase == KBDPASTE_PHASE_SHIFT_UP) {
			kbdinject_keyup(role_code(KBDROLE_SHIFTL));
			paste.shift_down = FALSE;
			paste.phase = KBDPASTE_PHASE_IDLE;
			cleanup_only = TRUE;
		} else if ((paste.phase == KBDPASTE_PHASE_KEY_DOWN) && paste.shift_down) {
			kbdinject_keyup(role_code(KBDROLE_SHIFTL));
			paste.shift_down = FALSE;
			paste.phase = KBDPASTE_PHASE_SHIFT_DOWN;
			paste.next_tick = now + KBDPASTE_EVENT_INTERVAL_MS;
			return;
		} else {
			return;
		}
	}
	if (!cleanup_only) {
		switch (paste.phase) {
		case KBDPASTE_PHASE_SHIFT_DOWN:
			kbdinject_keydown(role_code(KBDROLE_SHIFTL));
			paste.shift_down = TRUE;
			paste.phase = KBDPASTE_PHASE_KEY_DOWN;
			break;

		case KBDPASTE_PHASE_KEY_DOWN:
			kbdinject_keydown(action->guest_code);
			paste.key_down = TRUE;
			paste.phase = KBDPASTE_PHASE_KEY_UP;
			break;

		case KBDPASTE_PHASE_KEY_UP:
			kbdinject_keyup(action->guest_code);
			paste.key_down = FALSE;
			paste.phase = action->shift ? KBDPASTE_PHASE_SHIFT_UP : KBDPASTE_PHASE_IDLE;
			break;

		case KBDPASTE_PHASE_SHIFT_UP:
			kbdinject_keyup(role_code(KBDROLE_SHIFTL));
			paste.shift_down = FALSE;
			paste.phase = KBDPASTE_PHASE_IDLE;
			break;

		default:
			break;
		}
	}
	if (paste.phase == KBDPASTE_PHASE_IDLE) {
		paste.index++;
		if (paste.index >= paste.count) {
			UINT count;
			UINT skipped;

			count = (UINT)paste.count;
			skipped = paste.skipped;
			clear_queue();
			if (skipped != 0) {
				snprintf(paste.status, sizeof(paste.status),
				         "Paste complete: %u characters, %u skipped.", count, skipped);
			} else {
				snprintf(paste.status, sizeof(paste.status), "Paste complete: %u characters.",
				         count);
			}
			return;
		}
		paste.phase =
		    paste.actions[paste.index].shift ? KBDPASTE_PHASE_SHIFT_DOWN : KBDPASTE_PHASE_KEY_DOWN;
	}
	paste.next_tick = now + KBDPASTE_EVENT_INTERVAL_MS;
}

void kbdpaste_cancel(void) {
	BOOL was_active;

	was_active = paste.actions != NULL;
	clear_queue();
	if (was_active) {
		milstr_ncpy(paste.status, "Paste cancelled.", (int)sizeof(paste.status));
	}
}

BOOL kbdpaste_active(void) {
	return paste.actions != NULL;
}

const char *kbdpaste_status(void) {
	return paste.status;
}

UINT kbdpaste_interval_ms(void) {
	return KBDPASTE_EVENT_INTERVAL_MS;
}
