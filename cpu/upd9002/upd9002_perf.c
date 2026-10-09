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
#include "upd9002_perf.h"

#if defined(VAEG_UPD9002_PERF_DIAGNOSTIC)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	FILE *stream;
	UINT64 step_count;
	UINT64 opcode[256];
	UINT64 prefix_next[256][256];
	UINT64 op0f[256];
	UINT64 exception[256];
	UINT64 interrupt[256];
	UINT64 reserved[UPD9002_PERF_RESERVED_COUNT];
	UINT32 start_cs_base;
	UINT16 start_ip;
	UINT8 start_opcode;
	UINT8 start_next_byte;
	int start_seen;
	int owns_stream;
	int atexit_registered;
} UPD9002_PERF_STATE;

static UPD9002_PERF_STATE perf_state;

static int is_prefix(UINT8 opcode) {
	switch (opcode) {
	case 0x26:
	case 0x2e:
	case 0x36:
	case 0x3e:
	case 0x64:
	case 0x65:
	case 0xf2:
	case 0xf3:
		return 1;
	default:
		return 0;
	}
}

static const char *reserved_name(UINT index) {
	switch (index) {
	case UPD9002_PERF_RESERVED_PLAIN:
		return "plain";
	case UPD9002_PERF_RESERVED_0F:
		return "0f";
	case UPD9002_PERF_RESERVED_REPNC:
		return "repnc";
	case UPD9002_PERF_RESERVED_REPC:
		return "repc";
	case UPD9002_PERF_RESERVED_REP0F_DIAGNOSTIC:
		return "rep0f-diagnostic";
	default:
		return "invalid";
	}
}

static void print_histogram_256(const char *kind, const UINT64 values[256]) {
	UINT i;

	for (i = 0; i < 256; i++) {
		if (values[i] != 0) {
			fprintf(perf_state.stream, "%s opcode=%02x count=%llu\n", kind, i,
			        (unsigned long long)values[i]);
		}
	}
}

static void print_prefix_next(void) {
	UINT prefix;
	UINT opcode;

	for (prefix = 0; prefix < 256; prefix++) {
		if (!is_prefix((UINT8)prefix)) {
			continue;
		}
		for (opcode = 0; opcode < 256; opcode++) {
			if (perf_state.prefix_next[prefix][opcode] != 0) {
				fprintf(perf_state.stream, "prefix-next prefix=%02x opcode=%02x count=%llu\n",
				        prefix, opcode, (unsigned long long)perf_state.prefix_next[prefix][opcode]);
			}
		}
	}
}

void upd9002_perf_stop(void) {
	UINT i;

	if (perf_state.stream == NULL) {
		return;
	}
	fprintf(perf_state.stream, "upd9002-perf-v1\n");
	fprintf(perf_state.stream, "steps count=%llu\n", (unsigned long long)perf_state.step_count);
	if (perf_state.start_seen) {
		fprintf(perf_state.stream, "first cs_base=%08x ip=%04x opcode=%02x next=%02x\n",
		        perf_state.start_cs_base, perf_state.start_ip, perf_state.start_opcode,
		        perf_state.start_next_byte);
	}
	print_histogram_256("opcode", perf_state.opcode);
	print_prefix_next();
	print_histogram_256("op0f", perf_state.op0f);
	print_histogram_256("exception", perf_state.exception);
	print_histogram_256("interrupt", perf_state.interrupt);
	for (i = 0; i < UPD9002_PERF_RESERVED_COUNT; i++) {
		if (perf_state.reserved[i] != 0) {
			fprintf(perf_state.stream, "reserved kind=%s count=%llu\n", reserved_name(i),
			        (unsigned long long)perf_state.reserved[i]);
		}
	}
	fflush(perf_state.stream);
	if (perf_state.owns_stream) {
		fclose(perf_state.stream);
	}
	ZeroMemory(&perf_state, sizeof(perf_state));
}

void upd9002_perf_start_from_env(void) {
	const char *path;

	if (perf_state.stream != NULL) {
		return;
	}
	path = getenv("VAEG_UPD9002_PERF_LOG");
	if ((path == NULL) || (path[0] == '\0')) {
		return;
	}
	ZeroMemory(&perf_state, sizeof(perf_state));
	if (strcmp(path, "-") == 0) {
		perf_state.stream = stderr;
	} else {
		perf_state.stream = fopen(path, "w");
		if (perf_state.stream == NULL) {
			fprintf(stderr, "VAEG_UPD9002_PERF_LOG: cannot open %s\n", path);
			return;
		}
		perf_state.owns_stream = 1;
	}
	if (!perf_state.atexit_registered) {
		atexit(upd9002_perf_stop);
		perf_state.atexit_registered = 1;
	}
}

void upd9002_perf_record_step(uint32_t cs_base, uint16_t ip, uint8_t opcode, uint8_t next_byte) {
	if (perf_state.stream == NULL) {
		return;
	}
	if (!perf_state.start_seen) {
		perf_state.start_cs_base = cs_base;
		perf_state.start_ip = ip;
		perf_state.start_opcode = opcode;
		perf_state.start_next_byte = next_byte;
		perf_state.start_seen = 1;
	}
	perf_state.step_count++;
	perf_state.opcode[opcode]++;
	if (is_prefix(opcode)) {
		perf_state.prefix_next[opcode][next_byte]++;
	}
}

void upd9002_perf_record_0f(uint8_t opcode) {
	if (perf_state.stream == NULL) {
		return;
	}
	perf_state.op0f[opcode]++;
}

void upd9002_perf_record_reserved(uint32_t kind) {
	if ((perf_state.stream == NULL) || (kind >= UPD9002_PERF_RESERVED_COUNT)) {
		return;
	}
	perf_state.reserved[kind]++;
}

void upd9002_perf_record_exception(uint8_t vect) {
	if (perf_state.stream == NULL) {
		return;
	}
	perf_state.exception[vect]++;
}

void upd9002_perf_record_interrupt(uint8_t vect) {
	if (perf_state.stream == NULL) {
		return;
	}
	perf_state.interrupt[vect]++;
}

#endif
