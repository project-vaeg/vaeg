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
#include "cpucore.h"
#if !defined(VAEG_M44_RAW_COMPAT_PROBE)
#include "upd9002_state.h"
#endif
#include "upd9002_regs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROBE_LINE_SIZE 2048
#define CPU_PAYLOAD_SIZE 112
#define UPD9002_PAYLOAD_SIZE 16

Upd9002CoreContext upd9002_core_context;
UPD9002_REGS upd9002_regs;

static int hex_value(int ch) {
	if ((ch >= '0') && (ch <= '9')) {
		return ch - '0';
	}
	if ((ch >= 'a') && (ch <= 'f')) {
		return ch - 'a' + 10;
	}
	return -1;
}

static int decode_hex(const char *text, UINT8 *output, size_t size) {
	size_t index;

	for (index = 0; index < size; index++) {
		int high;
		int low;

		high = hex_value((unsigned char)text[index * 2]);
		low = hex_value((unsigned char)text[index * 2 + 1]);
		if ((high < 0) || (low < 0)) {
			return FAILURE;
		}
		output[index] = (UINT8)((high << 4) | low);
	}
	return SUCCESS;
}

static int parse_payload(const char *line, const char *key, UINT8 *output, size_t size) {
	const char *text;

	text = strstr(line, key);
	if (text == NULL) {
		return FAILURE;
	}
	text += strlen(key);
	if ((strlen(text) < size * 2) ||
	    ((text[size * 2] != ',') && (text[size * 2] != '\n') && (text[size * 2] != '\0'))) {
		return FAILURE;
	}
	return decode_hex(text, output, size);
}

static int import_export_cpu(const UINT8 *input, UINT8 *output) {
#if defined(VAEG_M44_RAW_COMPAT_PROBE)
	memcpy(&upd9002_core_context.s, input, sizeof(upd9002_core_context.s));
	memcpy(output, &upd9002_core_context.s, sizeof(upd9002_core_context.s));
	return SUCCESS;
#else
	Upd9002StateImage state;

	if (upd9002_state_import(input, CPU_PAYLOAD_SIZE, NULL, 0) != SUCCESS) {
		return FAILURE;
	}
	upd9002_state_export(&state);
	memcpy(output, &state, sizeof(state));
	return SUCCESS;
#endif
}

static int verify_record(const char *line) {
	UINT8 cpu_input[CPU_PAYLOAD_SIZE];
	UINT8 cpu_output[CPU_PAYLOAD_SIZE];
	UINT8 registers_input[UPD9002_PAYLOAD_SIZE];
	UINT8 registers_output[UPD9002_PAYLOAD_SIZE];

	if ((parse_payload(line, "cpu286=", cpu_input, sizeof(cpu_input)) != SUCCESS) ||
	    (parse_payload(line, "upd9002=", registers_input, sizeof(registers_input)) != SUCCESS) ||
	    (import_export_cpu(cpu_input, cpu_output) != SUCCESS)) {
		return FAILURE;
	}
	memcpy(&upd9002_regs, registers_input, sizeof(upd9002_regs));
	memcpy(registers_output, &upd9002_regs, sizeof(registers_output));
	if (memcmp(cpu_input, cpu_output, sizeof(cpu_input)) ||
	    memcmp(registers_input, registers_output, sizeof(registers_input))) {
		return FAILURE;
	}
	return SUCCESS;
}

static int verify_opaque_record(const char *line) {
	UINT8 cpu_input[CPU_PAYLOAD_SIZE];
	UINT8 cpu_output[CPU_PAYLOAD_SIZE];

	if (parse_payload(line, "cpu286=", cpu_input, sizeof(cpu_input)) != SUCCESS) {
		return FAILURE;
	}
	cpu_input[94] = 0xa5;
	cpu_input[95] = 0x5a;
	if ((import_export_cpu(cpu_input, cpu_output) != SUCCESS) ||
	    memcmp(cpu_input, cpu_output, sizeof(cpu_input))) {
		return FAILURE;
	}
	return SUCCESS;
}

int main(int argc, char **argv) {
	FILE *stream;
	char line[PROBE_LINE_SIZE];
	char first[PROBE_LINE_SIZE];
	unsigned int rows;

	if (argc != 2) {
		return 2;
	}
	if ((sizeof(upd9002_core_context.s) != CPU_PAYLOAD_SIZE) ||
	    (sizeof(upd9002_regs) != UPD9002_PAYLOAD_SIZE)) {
		return 3;
	}
#if !defined(VAEG_M44_RAW_COMPAT_PROBE)
	upd9002_state_initialize();
#endif
	stream = fopen(argv[1], "rb");
	if (stream == NULL) {
		return 4;
	}
	rows = 0;
	first[0] = '\0';
	while (fgets(line, sizeof(line), stream) != NULL) {
		if (rows == 0) {
			strncpy(first, line, sizeof(first) - 1);
			first[sizeof(first) - 1] = '\0';
		}
		if (verify_record(line) != SUCCESS) {
			fclose(stream);
			return 5;
		}
		rows++;
	}
	if (ferror(stream) || (rows != 3) || (verify_opaque_record(first) != SUCCESS)) {
		fclose(stream);
		return 6;
	}
	fclose(stream);
#if defined(VAEG_M44_RAW_COMPAT_PROBE)
	fprintf(stderr, "upd9002-state-payload-probe: mode=g41-raw rows=3 opaque=pass\n");
#else
	fprintf(stderr, "upd9002-state-payload-probe: mode=m44-adapter rows=3 opaque=pass\n");
#endif
	return 0;
}
