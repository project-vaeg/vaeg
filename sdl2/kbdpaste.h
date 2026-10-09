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
#ifndef VAEG_SDL2_KBDPASTE_H
#define VAEG_SDL2_KBDPASTE_H

#include <stddef.h>
#include "compiler.h"

typedef struct {
	BYTE guest_code;
	BOOL shift;
} KBDPASTE_ACTION;

#ifdef __cplusplus
extern "C" {
#endif

void kbdpaste_initialize(void);
void kbdpaste_shutdown(void);
BOOL kbdpaste_start_clipboard(void);
BOOL kbdpaste_start_text(const char *text);
void kbdpaste_tick(UINT32 now, BOOL paused);
void kbdpaste_cancel(void);
BOOL kbdpaste_active(void);
const char *kbdpaste_status(void);
UINT kbdpaste_interval_ms(void);
size_t kbdpaste_map_text(const char *text, KBDPASTE_ACTION *actions, size_t capacity,
                         UINT *skipped);

#ifdef __cplusplus
}
#endif

#endif
