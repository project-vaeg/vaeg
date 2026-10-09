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
#include "compiler.h"
#include "cpucore.h"
#include "upd9002_debug.h"

typedef struct {
	BOOL used;
	UINT16 cs;
	UINT16 ip;
	UINT32 count;
} UPD9002_DEBUG_COUNTER;

typedef struct {
	UPD9002_DEBUG_COUNTER counters[UPD9002_DEBUG_COUNTER_MAX];
	UINT counter_count;
	BOOL wait_armed;
	BOOL event_pending;
	BOOL resume_skip;
	UINT16 resume_cs;
	UINT16 resume_ip;
	UINT16 wait_cs;
	UINT16 wait_ip;
	UINT32 wait_ordinal;
	UINT32 wait_hits;
	UINT32 event_sequence;
	UPD9002_DEBUG_SNAPSHOT snapshot;
} UPD9002_DEBUG_STATE;

static UPD9002_DEBUG_STATE debug_state;

void upd9002_debug_reset(void) {
	ZeroMemory(&debug_state, sizeof(debug_state));
}

BOOL upd9002_debug_counter_add(UINT16 cs, UINT16 ip, UINT *index) {
	UPD9002_DEBUG_COUNTER *counter;

	if ((index == NULL) || (debug_state.counter_count >= UPD9002_DEBUG_COUNTER_MAX)) {
		return FAILURE;
	}
	counter = &debug_state.counters[debug_state.counter_count];
	counter->used = TRUE;
	counter->cs = cs;
	counter->ip = ip;
	counter->count = 0;
	*index = debug_state.counter_count++;
	return SUCCESS;
}

UINT32 upd9002_debug_counter_value(UINT index) {
	if ((index >= debug_state.counter_count) || !debug_state.counters[index].used) {
		return 0;
	}
	return debug_state.counters[index].count;
}

BOOL upd9002_debug_wait_arm(UINT16 cs, UINT16 ip, UINT32 ordinal) {
	if ((ordinal == 0) || debug_state.wait_armed || debug_state.event_pending) {
		return FAILURE;
	}
	debug_state.wait_armed = TRUE;
	debug_state.wait_cs = cs;
	debug_state.wait_ip = ip;
	debug_state.wait_ordinal = ordinal;
	debug_state.wait_hits = 0;
	return SUCCESS;
}

BOOL upd9002_debug_wait_armed(void) {
	return debug_state.wait_armed;
}

static void upd9002_debug_capture(UINT32 ordinal) {
	UPD9002_DEBUG_SNAPSHOT *snapshot;

	snapshot = &debug_state.snapshot;
	ZeroMemory(snapshot, sizeof(*snapshot));
	snapshot->sequence = ++debug_state.event_sequence;
	snapshot->ordinal = ordinal;
	snapshot->clock = (UINT32)(CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK);
	snapshot->ax = CPU_AX;
	snapshot->bx = CPU_BX;
	snapshot->cx = CPU_CX;
	snapshot->dx = CPU_DX;
	snapshot->si = CPU_SI;
	snapshot->di = CPU_DI;
	snapshot->bp = CPU_BP;
	snapshot->sp = CPU_SP;
	snapshot->es = CPU_ES;
	snapshot->cs = CPU_CS;
	snapshot->ss = CPU_SS;
	snapshot->ds = CPU_DS;
	snapshot->ip = CPU_IP;
	snapshot->flags = CPU_FLAG;
	snapshot->es_base = ES_BASE;
	snapshot->cs_base = CS_BASE;
	snapshot->ss_base = SS_BASE;
	snapshot->ds_base = DS_BASE;
}

static BOOL upd9002_debug_observe(UINT16 cs, UINT16 ip, BOOL capture) {
	UINT index;
	UPD9002_DEBUG_COUNTER *counter;

	if (debug_state.event_pending) {
		return TRUE;
	}
	if (debug_state.resume_skip) {
		debug_state.resume_skip = FALSE;
		if ((debug_state.resume_cs == cs) && (debug_state.resume_ip == ip)) {
			return FALSE;
		}
	}
	for (index = 0; index < debug_state.counter_count; index++) {
		counter = &debug_state.counters[index];
		if (counter->used && (counter->cs == cs) && (counter->ip == ip)) {
			counter->count++;
		}
	}
	if (!debug_state.wait_armed || (debug_state.wait_cs != cs) || (debug_state.wait_ip != ip)) {
		return FALSE;
	}
	debug_state.wait_hits++;
	if (debug_state.wait_hits != debug_state.wait_ordinal) {
		return FALSE;
	}
	if (capture) {
		upd9002_debug_capture(debug_state.wait_hits);
	}
	debug_state.event_pending = TRUE;
	return TRUE;
}

BOOL upd9002_debug_step_begin(void) {
	return upd9002_debug_observe(CPU_CS, CPU_IP, TRUE);
}

BOOL upd9002_debug_event_pending(void) {
	return debug_state.event_pending;
}

BOOL upd9002_debug_event_snapshot(UPD9002_DEBUG_SNAPSHOT *snapshot) {
	if (!debug_state.event_pending || (snapshot == NULL)) {
		return FAILURE;
	}
	*snapshot = debug_state.snapshot;
	return SUCCESS;
}

void upd9002_debug_event_resume(void) {
	if (!debug_state.event_pending) {
		return;
	}
	debug_state.event_pending = FALSE;
	debug_state.wait_armed = FALSE;
	debug_state.wait_hits = 0;
	debug_state.resume_skip = TRUE;
	debug_state.resume_cs = debug_state.snapshot.cs;
	debug_state.resume_ip = debug_state.snapshot.ip;
}

BOOL upd9002_debug_selftest(void) {
	UPD9002_DEBUG_STATE saved;
	Upd9002CoreContext core_saved;
	UPD9002_DEBUG_SNAPSHOT snapshot;
	UINT first;
	UINT second;
	BOOL passed;

	saved = debug_state;
	core_saved = upd9002_core_context;
	upd9002_debug_reset();
	CPU_CS = 0x1234;
	CPU_IP = 0x5678;
	CPU_AX = 0x9abc;
	passed = (upd9002_debug_counter_add(0x1234, 0x5678, &first) == SUCCESS) &&
	         (upd9002_debug_counter_add(0x1234, 0x5679, &second) == SUCCESS) &&
	         (upd9002_debug_wait_arm(0x1234, 0x5678, 2) == SUCCESS) &&
	         !upd9002_debug_observe(0x1234, 0x5679, FALSE) && !upd9002_debug_step_begin() &&
	         upd9002_debug_step_begin() && upd9002_debug_event_pending() &&
	         (upd9002_debug_event_snapshot(&snapshot) == SUCCESS) && (snapshot.cs == 0x1234) &&
	         (snapshot.ip == 0x5678) && (snapshot.ax == 0x9abc) && (snapshot.ordinal == 2) &&
	         (upd9002_debug_counter_value(first) == 2) &&
	         (upd9002_debug_counter_value(second) == 1);
	passed = passed && upd9002_debug_step_begin() && (CPU_CS == 0x1234) && (CPU_IP == 0x5678) &&
	         (CPU_AX == 0x9abc) && upd9002_debug_event_pending();
	upd9002_debug_event_resume();
	passed = passed && !upd9002_debug_event_pending() && !upd9002_debug_wait_armed() &&
	         !upd9002_debug_step_begin() && (upd9002_debug_counter_value(first) == 2) &&
	         !upd9002_debug_step_begin() && (upd9002_debug_counter_value(first) == 3) &&
	         (upd9002_debug_wait_arm(0, 0, 0) == FAILURE);
	upd9002_core_context = core_saved;
	debug_state = saved;
	return passed ? SUCCESS : FAILURE;
}
