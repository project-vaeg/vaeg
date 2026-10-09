/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_SDL2_HEADLESS_INPUT_H
#define VAEG_SDL2_HEADLESS_INPUT_H

#include "compiler.h"

typedef struct {
	char *text;
	UINT wait_frames;
	UINT disk_drive;
	BOOL wait;
	BOOL disk_swap;
	BOOL key_press;
	BOOL key_hold;
	UINT key_role;
	UINT modifier_role;
} HEADLESS_INPUT_COMMAND;

typedef struct {
	UINT command_index;
	UINT command_count;
	UINT32 next_frame;
	BOOL completed;
	UINT key_phase;
	BYTE held_key;
	BYTE held_modifier;
	BOOL key_down;
	BOOL modifier_down;
	HEADLESS_INPUT_COMMAND *commands;
} HEADLESS_INPUT_SCRIPT;

#ifdef __cplusplus
extern "C" {
#endif

BOOL headless_input_script_load(HEADLESS_INPUT_SCRIPT *script, const char *path);
void headless_input_script_clear(HEADLESS_INPUT_SCRIPT *script);
void headless_input_script_initialize(HEADLESS_INPUT_SCRIPT *script);
BOOL headless_input_script_after_frame(HEADLESS_INPUT_SCRIPT *script, UINT frames);

#ifdef __cplusplus
}
#endif

#endif
