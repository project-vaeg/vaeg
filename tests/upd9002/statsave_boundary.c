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
#include "dosio.h"
#include "cpucore.h"
#include "upd9002_state.h"
#include "machine/pccore.h"
#include "upd9002_regs.h"
#include "tests/upd9002/statsave_boundary.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define STATE_FILE_HEADER_SIZE 0x30
#define STATE_SECTION_HEADER_SIZE 16
#define STATE_MEMORY_SIZE 0x110000

typedef struct {
	Upd9002RuntimeState runtime;
	Upd9002StateImage state_image;
	PCCORE core;
	UPD9002_REGS registers;
	UINT32 memory_hash;
} STATE_SNAPSHOT;

static UINT32 read_u32(const BYTE *data) {
	return (UINT32)data[0] | ((UINT32)data[1] << 8) | ((UINT32)data[2] << 16) |
	       ((UINT32)data[3] << 24);
}

static void write_u32(BYTE *data, UINT32 value) {
	data[0] = (BYTE)value;
	data[1] = (BYTE)(value >> 8);
	data[2] = (BYTE)(value >> 16);
	data[3] = (BYTE)(value >> 24);
}

static void write_u16(BYTE *data, UINT16 value) {
	data[0] = (BYTE)value;
	data[1] = (BYTE)(value >> 8);
}

static void write_section_index(BYTE *data, UINT header, const char *name) {
	size_t length;

	memset(data + header, 0, 10);
	length = strlen(name);
	if (length > 10) {
		length = 10;
	}
	memcpy(data + header, name, length);
}

static UINT32 memory_hash(void) {
	UINT32 hash;
	UINT32 index;

	hash = 2166136261u;
	for (index = 0; index < STATE_MEMORY_SIZE; index++) {
		hash ^= mem[index];
		hash *= 16777619u;
	}
	return hash;
}

static void snapshot_capture(STATE_SNAPSHOT *snapshot) {
	snapshot->runtime = upd9002_core_context.s;
	upd9002_state_export(&snapshot->state_image);
	snapshot->core = pccore;
	snapshot->registers = upd9002_regs;
	snapshot->memory_hash = memory_hash();
}

static int snapshot_matches(const STATE_SNAPSHOT *snapshot) {
	Upd9002StateImage compatibility;

	upd9002_state_export(&compatibility);
	return !memcmp(&snapshot->runtime, &upd9002_core_context.s, sizeof(snapshot->runtime)) &&
	       !memcmp(&snapshot->state_image, &compatibility, sizeof(snapshot->state_image)) &&
	       !memcmp(&snapshot->core, &pccore, sizeof(snapshot->core)) &&
	       !memcmp(&snapshot->registers, &upd9002_regs, sizeof(snapshot->registers)) &&
	       (snapshot->memory_hash == memory_hash());
}

static int read_file(const char *path, BYTE **data, UINT *size) {
	FILEH file;
	BYTE *buffer;
	UINT length;

	*data = NULL;
	*size = 0;
	file = file_open_rb(path);
	if (file == FILEH_INVALID) {
		return FAILURE;
	}
	length = file_getsize(file);
	buffer = (BYTE *)_MALLOC(length, "upd9002-state-test");
	if ((buffer == NULL) || (file_read(file, buffer, length) != length)) {
		if (buffer != NULL) {
			_MFREE(buffer);
		}
		file_close(file);
		return FAILURE;
	}
	file_close(file);
	*data = buffer;
	*size = length;
	return SUCCESS;
}

static int write_file(const char *path, const BYTE *data, UINT size) {
	FILEH file;
	int result;

	file = file_create(path);
	if (file == FILEH_INVALID) {
		return FAILURE;
	}
	result = (file_write(file, data, size) == size) ? SUCCESS : FAILURE;
	file_close(file);
	return result;
}

static int find_section(const BYTE *data, UINT data_size, const char *name, UINT *header,
                        UINT *body, UINT *body_size) {
	UINT position;
	size_t name_size;

	name_size = strlen(name);
	position = STATE_FILE_HEADER_SIZE;
	while ((position + STATE_SECTION_HEADER_SIZE) <= data_size) {
		UINT current_size;
		UINT padded;

		current_size = read_u32(data + position + 12);
		padded = (current_size + 15) & ~15U;
		if ((position + STATE_SECTION_HEADER_SIZE + padded) > data_size) {
			return FAILURE;
		}
		if ((name_size <= 10) && !memcmp(data + position, name, name_size)) {
			*header = position;
			*body = position + STATE_SECTION_HEADER_SIZE;
			*body_size = current_size;
			return SUCCESS;
		}
		position += STATE_SECTION_HEADER_SIZE + padded;
	}
	return FAILURE;
}

static int check_rejection(const char *path, const char *message, const STATE_SNAPSHOT *snapshot) {
	char error[256];

	error[0] = '\0';
	if ((statsave_check(path, error, sizeof(error)) != STATFLAG_FAILURE) ||
	    (strstr(error, message) == NULL)) {
		fprintf(stderr, "upd9002-statsave-boundary: unexpected error: %s\n", error);
		return FAILURE;
	}
	if (statsave_load(path) != STATFLAG_FAILURE) {
		return FAILURE;
	}
	return snapshot_matches(snapshot) ? SUCCESS : FAILURE;
}

static int compare_section(const BYTE *left, UINT left_size, const BYTE *right, UINT right_size,
                           const char *name) {
	UINT left_header;
	UINT left_body;
	UINT left_body_size;
	UINT right_header;
	UINT right_body;
	UINT right_body_size;

	if ((find_section(left, left_size, name, &left_header, &left_body, &left_body_size) !=
	     SUCCESS) ||
	    (find_section(right, right_size, name, &right_header, &right_body, &right_body_size) !=
	     SUCCESS) ||
	    (left_body_size != right_body_size)) {
		return FAILURE;
	}
	return memcmp(left + left_body, right + right_body, left_body_size) ? FAILURE : SUCCESS;
}

static int compare_section_pair(const BYTE *left, UINT left_size, const char *left_name,
                                const BYTE *right, UINT right_size, const char *right_name) {
	UINT left_header;
	UINT left_body;
	UINT left_body_size;
	UINT right_header;
	UINT right_body;
	UINT right_body_size;

	if ((find_section(left, left_size, left_name, &left_header, &left_body, &left_body_size) !=
	     SUCCESS) ||
	    (find_section(right, right_size, right_name, &right_header, &right_body,
	                  &right_body_size) != SUCCESS) ||
	    (left_body_size != right_body_size)) {
		return FAILURE;
	}
	return memcmp(left + left_body, right + right_body, left_body_size) ? FAILURE : SUCCESS;
}

int upd9002_statsave_boundary_verify(const char *valid_path) {
	BYTE *data;
	BYTE *roundtrip;
	UINT data_size;
	UINT roundtrip_size;
	UINT header;
	UINT body;
	UINT body_size;
	UINT truncated_size;
	UINT32 current_version;
	STATE_SNAPSHOT snapshot;
	char invalid_path[MAX_PATH];
	char legacy_path[MAX_PATH];
	char legacy_only_path[MAX_PATH];
	char malformed_path[MAX_PATH];
	char opaque_path[MAX_PATH];
	char protected_path[MAX_PATH];
	char roundtrip_path[MAX_PATH];
	char truncated_path[MAX_PATH];
	int result;

	data = NULL;
	roundtrip = NULL;
	SPRINTF(invalid_path, "%s-m44-invalid", valid_path);
	SPRINTF(legacy_path, "%s-m66-legacy-current", valid_path);
	SPRINTF(legacy_only_path, "%s-m66-legacy-only", valid_path);
	SPRINTF(malformed_path, "%s-m44-size", valid_path);
	SPRINTF(opaque_path, "%s-m44-opaque", valid_path);
	SPRINTF(protected_path, "%s-m48-protected", valid_path);
	SPRINTF(roundtrip_path, "%s-m44-roundtrip", valid_path);
	SPRINTF(truncated_path, "%s-m44-truncated", valid_path);
	file_delete(invalid_path);
	file_delete(legacy_path);
	file_delete(legacy_only_path);
	file_delete(malformed_path);
	file_delete(opaque_path);
	file_delete(protected_path);
	file_delete(roundtrip_path);
	file_delete(truncated_path);

	result = FAILURE;
	if ((read_file(valid_path, &data, &data_size) != SUCCESS) ||
	    (find_section(data, data_size, UPD9002_STATE_SECTION, &header, &body, &body_size) !=
	     SUCCESS) ||
	    (body_size != sizeof(Upd9002StateImage))) {
		goto done;
	}
	snapshot_capture(&snapshot);
	current_version = read_u32(data + STATE_FILE_HEADER_SIZE - sizeof(UINT32));
	write_u32(data + STATE_FILE_HEADER_SIZE - sizeof(UINT32), current_version - 1);
	if ((write_file(invalid_path, data, data_size) != SUCCESS) ||
	    (check_rejection(invalid_path, "", &snapshot) != SUCCESS)) {
		goto done;
	}
	write_u32(data + STATE_FILE_HEADER_SIZE - sizeof(UINT32), current_version);

	data[body + offsetof(Upd9002StateImage, cpu_type)] = 0;
	if ((write_file(invalid_path, data, data_size) != SUCCESS) ||
	    (check_rejection(invalid_path, UPD9002_STATE_ERROR_CPU_TYPE, &snapshot) != SUCCESS)) {
		goto done;
	}
	data[body + offsetof(Upd9002StateImage, cpu_type)] = CPUTYPE_V30;
	data[body + offsetof(Upd9002StateImage, MSW)] = MSW_PE;
	data[body + offsetof(Upd9002StateImage, MSW) + 1] = 0;
	if ((write_file(protected_path, data, data_size) != SUCCESS) ||
	    (check_rejection(protected_path, UPD9002_STATE_ERROR_PROTECTED_MODE, &snapshot) !=
	     SUCCESS)) {
		goto done;
	}
	data[body + offsetof(Upd9002StateImage, MSW)] = 0;
	data[body + offsetof(Upd9002StateImage, MSW) + 1] = 0;

	write_u32(data + header + 12, sizeof(Upd9002StateImage) - 1);
	if ((write_file(malformed_path, data, data_size) != SUCCESS) ||
	    (check_rejection(malformed_path, UPD9002_STATE_ERROR_SIZE, &snapshot) != SUCCESS)) {
		goto done;
	}
	write_u32(data + header + 12, sizeof(Upd9002StateImage));

	truncated_size = body + sizeof(Upd9002StateImage) - 7;
	if ((write_file(truncated_path, data, truncated_size) != SUCCESS) ||
	    (check_rejection(truncated_path, "uPD9002 payload is truncated", &snapshot) != SUCCESS)) {
		goto done;
	}

	data[body + offsetof(Upd9002StateImage, padding)] = 0xa5;
	data[body + offsetof(Upd9002StateImage, padding) + 1] = 0x5a;
	if ((write_file(opaque_path, data, data_size) != SUCCESS) ||
	    (statsave_load(opaque_path) == STATFLAG_FAILURE) ||
	    (statsave_save(roundtrip_path) != STATFLAG_SUCCESS) ||
	    (read_file(roundtrip_path, &roundtrip, &roundtrip_size) != SUCCESS) ||
	    (compare_section(data, data_size, roundtrip, roundtrip_size, UPD9002_STATE_SECTION) !=
	     SUCCESS) ||
	    (compare_section(data, data_size, roundtrip, roundtrip_size, "UPD9002") != SUCCESS)) {
		goto done;
	}
	_MFREE(roundtrip);
	roundtrip = NULL;

	write_section_index(data, header, "CPU286");
	write_u16(data + header + 10, 0);
	if ((write_file(legacy_path, data, data_size) != SUCCESS) ||
	    (statsave_load(legacy_path) == STATFLAG_FAILURE) ||
	    (statsave_save(roundtrip_path) != STATFLAG_SUCCESS) ||
	    (read_file(roundtrip_path, &roundtrip, &roundtrip_size) != SUCCESS) ||
	    (compare_section_pair(data, data_size, "CPU286", roundtrip, roundtrip_size,
	                          UPD9002_STATE_SECTION) != SUCCESS)) {
		goto done;
	}
	_MFREE(roundtrip);
	roundtrip = NULL;

	if (find_section(data, data_size, "UPD9002", &header, &body, &body_size) != SUCCESS) {
		goto done;
	}
	write_section_index(data, header, "LEGPORT");
	snapshot_capture(&snapshot);
	if ((write_file(legacy_only_path, data, data_size) != SUCCESS) ||
	    (check_rejection(legacy_only_path, UPD9002_STATE_ERROR_LEGACY_MARKER, &snapshot) !=
	     SUCCESS)) {
		goto done;
	}
	result = SUCCESS;

done:
	if (statsave_load(valid_path) == STATFLAG_FAILURE) {
		result = FAILURE;
	}
	if (roundtrip != NULL) {
		_MFREE(roundtrip);
	}
	if (data != NULL) {
		_MFREE(data);
	}
	file_delete(invalid_path);
	file_delete(legacy_path);
	file_delete(legacy_only_path);
	file_delete(malformed_path);
	file_delete(opaque_path);
	file_delete(protected_path);
	file_delete(roundtrip_path);
	file_delete(truncated_path);
	if (result == SUCCESS) {
		fprintf(
		    stderr,
		    "upd9002-statsave-boundary: invalid type, protected state, size, truncation, atomicity, and opaque round trip passed\n");
	}
	return result;
}
