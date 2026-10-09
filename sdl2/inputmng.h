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
#ifndef VAEG_SDL2_INPUTMNG_H
#define VAEG_SDL2_INPUTMNG_H

enum {
	LBUTTON_BIT = (1 << 0),
	RBUTTON_BIT = (1 << 1),
	LBUTTON_DOWNBIT = (1 << 2),
	RBUTTON_DOWNBIT = (1 << 3),
	LBUTTON_UPBIT = (1 << 4),
	RBUTTON_UPBIT = (1 << 5),
	MOUSE_MOVEBIT = (1 << 6),

	KEY_ENTER = 0x01,
	KEY_MENU = 0x02,
	KEY_SKIP = 0x04,
	KEY_EXT = 0x08,
	KEY_UP = 0x10,
	KEY_DOWN = 0x20,
	KEY_LEFT = 0x40,
	KEY_RIGHT = 0x80
};

#ifdef __cplusplus
extern "C" {
#endif

void inputmng_init(void);
void inputmng_keybind(short key, UINT bit);
UINT inputmng_getkey(short key);

#ifdef __cplusplus
}
#endif

#endif
