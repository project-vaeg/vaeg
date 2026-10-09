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
#ifndef VAEG_UPD9002_DEBUG_H
#define VAEG_UPD9002_DEBUG_H

#include "compiler.h"

enum {
	UPD9002_DEBUG_COUNTER_MAX = 32
};

typedef struct {
	UINT32 sequence;
	UINT32 ordinal;
	UINT32 clock;
	UINT16 ax;
	UINT16 bx;
	UINT16 cx;
	UINT16 dx;
	UINT16 si;
	UINT16 di;
	UINT16 bp;
	UINT16 sp;
	UINT16 es;
	UINT16 cs;
	UINT16 ss;
	UINT16 ds;
	UINT16 ip;
	UINT16 flags;
	UINT32 es_base;
	UINT32 cs_base;
	UINT32 ss_base;
	UINT32 ds_base;
} UPD9002_DEBUG_SNAPSHOT;

#ifdef __cplusplus
extern "C" {
#endif

void upd9002_debug_reset(void);
BOOL upd9002_debug_counter_add(UINT16 cs, UINT16 ip, UINT *index);
UINT32 upd9002_debug_counter_value(UINT index);
BOOL upd9002_debug_wait_arm(UINT16 cs, UINT16 ip, UINT32 ordinal);
BOOL upd9002_debug_wait_armed(void);
BOOL upd9002_debug_step_begin(void);
BOOL upd9002_debug_event_pending(void);
BOOL upd9002_debug_event_snapshot(UPD9002_DEBUG_SNAPSHOT *snapshot);
void upd9002_debug_event_resume(void);
BOOL upd9002_debug_selftest(void);

#ifdef __cplusplus
}
#endif

#endif
