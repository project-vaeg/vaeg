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
#ifndef VAEG_SDL2_KBDMAP_H
#define VAEG_SDL2_KBDMAP_H

#include <stddef.h>
#include "compiler.h"
#include "sdlapi.h"

enum {
	KBDMAP_NC = 0xff
};

enum {
	KBDMAP_F12_FULL_SPEED = 6,
	KBDMAP_F12_SCREENSHOT = 7
};

typedef enum {
	KBDMAP_STATUS_IMPLEMENTED = 0,
	KBDMAP_STATUS_MAPPED_UNTESTED,
	KBDMAP_STATUS_UNRESOLVED,
	KBDMAP_STATUS_INTENTIONALLY_UNMAPPED
} KBDMAP_STATUS;

typedef enum {
	KBDROLE_STOP = 0,
	KBDROLE_COPY,
	KBDROLE_F1,
	KBDROLE_F2,
	KBDROLE_F3,
	KBDROLE_F4,
	KBDROLE_F5,
	KBDROLE_F6,
	KBDROLE_F7,
	KBDROLE_F8,
	KBDROLE_F9,
	KBDROLE_F10,
	KBDROLE_ROLLUP,
	KBDROLE_ROLLDOWN,
	KBDROLE_ESC,
	KBDROLE_1,
	KBDROLE_2,
	KBDROLE_3,
	KBDROLE_4,
	KBDROLE_5,
	KBDROLE_6,
	KBDROLE_7,
	KBDROLE_8,
	KBDROLE_9,
	KBDROLE_0,
	KBDROLE_MINUS,
	KBDROLE_CARET,
	KBDROLE_YEN,
	KBDROLE_BS,
	KBDROLE_TAB,
	KBDROLE_Q,
	KBDROLE_W,
	KBDROLE_E,
	KBDROLE_R,
	KBDROLE_T,
	KBDROLE_Y,
	KBDROLE_U,
	KBDROLE_I,
	KBDROLE_O,
	KBDROLE_P,
	KBDROLE_AT,
	KBDROLE_BRACKETLEFT,
	KBDROLE_RETURNL,
	KBDROLE_CTRL,
	KBDROLE_CAPS,
	KBDROLE_A,
	KBDROLE_S,
	KBDROLE_D,
	KBDROLE_F,
	KBDROLE_G,
	KBDROLE_H,
	KBDROLE_J,
	KBDROLE_K,
	KBDROLE_L,
	KBDROLE_SEMICOLON,
	KBDROLE_COLON,
	KBDROLE_BRACKETRIGHT,
	KBDROLE_SHIFTL,
	KBDROLE_Z,
	KBDROLE_X,
	KBDROLE_C,
	KBDROLE_V,
	KBDROLE_B,
	KBDROLE_N,
	KBDROLE_M,
	KBDROLE_COMMA,
	KBDROLE_PERIOD,
	KBDROLE_SLASH,
	KBDROLE_UNDERSCORE,
	KBDROLE_SHIFTR,
	KBDROLE_KANA,
	KBDROLE_GRAPH,
	KBDROLE_KETTEI,
	KBDROLE_SPACE,
	KBDROLE_HENKAN,
	KBDROLE_PC,
	KBDROLE_ZENKAKU,
	KBDROLE_INS,
	KBDROLE_DEL,
	KBDROLE_UP,
	KBDROLE_DOWN,
	KBDROLE_LEFT,
	KBDROLE_RIGHT,
	KBDROLE_HOME,
	KBDROLE_HELP,
	KBDROLE_KP_SUB,
	KBDROLE_KP_DIVIDE,
	KBDROLE_KP_7,
	KBDROLE_KP_8,
	KBDROLE_KP_9,
	KBDROLE_KP_MULTIPLY,
	KBDROLE_KP_4,
	KBDROLE_KP_5,
	KBDROLE_KP_6,
	KBDROLE_KP_ADD,
	KBDROLE_KP_1,
	KBDROLE_KP_2,
	KBDROLE_KP_3,
	KBDROLE_KP_EQUAL,
	KBDROLE_KP_0,
	KBDROLE_KP_COMMA,
	KBDROLE_KP_PERIOD,
	KBDROLE_RETURNR,
	KBDROLE_FAST_FORWARD,
	KBDROLE_MOUSE_CAPTURE,
	KBDROLE_SCREENSHOT,
	KBDROLE_COUNT
} KBDMAP_ROLE;

typedef enum {
	KBDMAP_SPECIAL_NONE = -1,
	KBDMAP_SPECIAL_FAST_FORWARD = 0,
	KBDMAP_SPECIAL_MOUSE_CAPTURE,
	KBDMAP_SPECIAL_SCREENSHOT
} KBDMAP_SPECIAL_ACTION;

typedef struct {
	KBDMAP_ROLE role;
	const char *id;
	const char *label;
	const char *semantic;
	BYTE guest_code;
	SDL_Scancode jis_scancode;
	SDL_Scancode us_scancode;
	KBDMAP_STATUS status;
	const char *evidence;
} KBDMAP_ENTRY;

#ifdef __cplusplus
extern "C" {
#endif

void kbdmap_initialize(void);
BOOL kbdmap_keydown(UINT scancode, SDL_Keycode keycode, UINT16 mod, BOOL repeat);
BOOL kbdmap_keyup(UINT scancode, SDL_Keycode keycode, UINT16 mod);
BOOL kbdmap_textinput(const char *text);
BYTE kbdmap_lookup(UINT scancode);
void kbdmap_reset_frontend_state(void);
void kbdmap_resetf12(void);
void kbdmap_trace_captured_key(UINT scancode, SDL_Keycode keycode, UINT16 mod, BOOL down,
                               BOOL repeat);

int kbdmap_entry_count(void);
const KBDMAP_ENTRY *kbdmap_entry(int index);
BYTE kbdmap_guest_code(KBDMAP_ROLE role);
SDL_Scancode kbdmap_binding(int index);
KBDMAP_STATUS kbdmap_binding_status(int index);
KBDMAP_SPECIAL_ACTION kbdmap_special_action(UINT scancode);
const char *kbdmap_status_name(KBDMAP_STATUS status);
const char *kbdmap_layout_name(void);
const char *kbdmap_kana_input_name(void);
BOOL kbdmap_tenkey_overlay_enabled(void);

void kbdmap_set_layout(const char *layout);
void kbdmap_set_kana_input(const char *mode);
void kbdmap_set_tenkey_overlay(BOOL enabled);
void kbdmap_reset_to_jis(void);
void kbdmap_reset_to_us(void);
BOOL kbdmap_set_binding(int index, SDL_Scancode scancode);
void kbdmap_serialize_custom(char *dst, size_t size);
void kbdmap_apply_config(void);
int kbdmap_selftest(void);

#ifdef __cplusplus
}
#endif

#endif
