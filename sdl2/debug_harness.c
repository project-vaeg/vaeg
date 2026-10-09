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
#include "debug_harness.h"

#include "diagnostics/upd9002_debug.h"
#include "cpu/upd9002/upd9002_trace.h"
#include "fdd/diskdrv.h"
#include "g75_screen.h"
#include "gvramva.h"
#include "kbdpaste.h"
#include "sdlapi.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define DEBUG_PATH_SEPARATOR '\\'
#else
#define DEBUG_PATH_SEPARATOR '/'
#endif

enum {
	DEBUG_COMMAND_MAX = 512,
	DEBUG_RESOURCE_MAX = 16,
	DEBUG_COUNTER_MAX = 32,
	DEBUG_ID_MAX = 63,
	DEBUG_SCRIPT_MAX = 64 * 1024,
	DEBUG_LINE_MAX = 4096,
	DEBUG_CAPTURE_REGISTERS = 1,
	DEBUG_CAPTURE_TVRAM = 2,
	DEBUG_CAPTURE_SCREEN = 4,
	DEBUG_CAPTURE_GVRAM = 8
};

typedef enum {
	DEBUG_ACTION_WAIT_FRAME,
	DEBUG_ACTION_INPUT_LINE,
	DEBUG_ACTION_ENTER,
	DEBUG_ACTION_MOUNT_FDD,
	DEBUG_ACTION_WAIT_PC,
	DEBUG_ACTION_TRACE,
	DEBUG_ACTION_CAPTURE,
	DEBUG_ACTION_EXIT
} DEBUG_ACTION_TYPE;

typedef struct {
	char id[DEBUG_ID_MAX + 1];
	char *path;
	BOOL none;
} DEBUG_RESOURCE;

typedef struct {
	char id[DEBUG_ID_MAX + 1];
	UINT16 cs;
	UINT16 ip;
	UINT core_index;
} DEBUG_COUNTER;

typedef struct {
	DEBUG_ACTION_TYPE type;
	UINT32 value;
	UINT16 cs;
	UINT16 ip;
	UINT argument;
	char id[DEBUG_ID_MAX + 1];
	char *text;
} DEBUG_ACTION;

typedef struct {
	BOOL loaded;
	BOOL initialized;
	BOOL exit_requested;
	BOOL wait_armed;
	UINT32 frame_limit;
	UINT32 last_frame;
	UINT action_index;
	UINT action_count;
	UINT resource_count;
	UINT counter_count;
	DEBUG_ACTION actions[DEBUG_COMMAND_MAX];
	DEBUG_RESOURCE resources[DEBUG_RESOURCE_MAX];
	DEBUG_COUNTER counters[DEBUG_COUNTER_MAX];
	char output_dir[MAX_PATH];
	FILE *events;
	FILE *trace;
} DEBUG_HARNESS_STATE;

static DEBUG_HARNESS_STATE harness;

static BOOL debug_harness_trace_supported(void) {
#ifdef VAEG_Z80_COMPAT_INTEGRATION_TRACE
	return TRUE;
#else
	return FALSE;
#endif
}

static BOOL debug_id_valid(const char *value) {
	const unsigned char *p;

	if ((value == NULL) || (value[0] == '\0') || (strlen(value) > DEBUG_ID_MAX)) {
		return FALSE;
	}
	p = (const unsigned char *)value;
	if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))) {
		return FALSE;
	}
	for (p++; *p != '\0'; p++) {
		if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '_' || *p == '-')) {
			return FALSE;
		}
	}
	return TRUE;
}

static char *debug_string_duplicate(const char *value) {
	size_t length;
	char *copy;

	length = strlen(value);
	copy = (char *)malloc(length + 1);
	if (copy != NULL) {
		CopyMemory(copy, value, length + 1);
	}
	return copy;
}

static BOOL debug_parse_uint32(const char *value, UINT32 *result) {
	char *end;
	unsigned long parsed;

	if ((value == NULL) || (value[0] == '\0') || (value[0] == '-')) {
		return FAILURE;
	}
	errno = 0;
	parsed = strtoul(value, &end, 10);
	if ((errno != 0) || (end == value) || (*end != '\0') || (parsed > 0xffffffffUL)) {
		return FAILURE;
	}
	*result = (UINT32)parsed;
	return SUCCESS;
}

static BOOL debug_parse_address(const char *value, UINT16 *cs, UINT16 *ip) {
	char copy[32];
	char *separator;
	char *end;
	unsigned long segment;
	unsigned long offset;

	if ((value == NULL) || (strlen(value) >= sizeof(copy))) {
		return FAILURE;
	}
	strcpy(copy, value);
	separator = strchr(copy, ':');
	if ((separator == NULL) || (strchr(separator + 1, ':') != NULL)) {
		return FAILURE;
	}
	*separator++ = '\0';
	errno = 0;
	segment = strtoul(copy, &end, 16);
	if ((errno != 0) || (end == copy) || (*end != '\0') || (segment > 0xffffUL)) {
		return FAILURE;
	}
	errno = 0;
	offset = strtoul(separator, &end, 16);
	if ((errno != 0) || (end == separator) || (*end != '\0') || (offset > 0xffffUL)) {
		return FAILURE;
	}
	*cs = (UINT16)segment;
	*ip = (UINT16)offset;
	return SUCCESS;
}

static char *debug_next_token(char **cursor) {
	char *start;
	char *p;

	p = *cursor;
	while ((*p == ' ') || (*p == '\t')) {
		p++;
	}
	if (*p == '\0') {
		*cursor = p;
		return NULL;
	}
	start = p;
	while ((*p != '\0') && (*p != ' ') && (*p != '\t')) {
		p++;
	}
	if (*p != '\0') {
		*p++ = '\0';
	}
	*cursor = p;
	return start;
}

static char *debug_remainder(char *cursor) {
	char *end;

	while ((*cursor == ' ') || (*cursor == '\t')) {
		cursor++;
	}
	end = cursor + strlen(cursor);
	while ((end > cursor) && ((end[-1] == ' ') || (end[-1] == '\t'))) {
		*--end = '\0';
	}
	return cursor;
}

static BOOL debug_add_action(DEBUG_ACTION_TYPE type, DEBUG_ACTION **result) {
	DEBUG_ACTION *action;

	if (harness.action_count >= DEBUG_COMMAND_MAX) {
		return FAILURE;
	}
	action = &harness.actions[harness.action_count++];
	ZeroMemory(action, sizeof(*action));
	action->type = type;
	*result = action;
	return SUCCESS;
}

static int debug_find_resource(const char *id) {
	UINT index;

	for (index = 0; index < harness.resource_count; index++) {
		if (!strcmp(harness.resources[index].id, id)) {
			return (int)index;
		}
	}
	return -1;
}

static BOOL debug_add_resource(const char *id, const char *path) {
	DEBUG_RESOURCE *resource;

	if (!debug_id_valid(id) || (path == NULL) || (path[0] == '\0') || (strlen(path) >= MAX_PATH) ||
	    (harness.resource_count >= DEBUG_RESOURCE_MAX) || (debug_find_resource(id) >= 0)) {
		return FAILURE;
	}
	resource = &harness.resources[harness.resource_count++];
	strcpy(resource->id, id);
	resource->none = !strcmp(path, "none") ? TRUE : FALSE;
	if (!resource->none) {
		resource->path = debug_string_duplicate(path);
		if (resource->path == NULL) {
			return FAILURE;
		}
	}
	return SUCCESS;
}

static BOOL debug_add_counter(const char *id, const char *address) {
	DEBUG_COUNTER *counter;

	if (!debug_id_valid(id) || (harness.counter_count >= DEBUG_COUNTER_MAX)) {
		return FAILURE;
	}
	for (counter = harness.counters; counter < harness.counters + harness.counter_count;
	     counter++) {
		if (!strcmp(counter->id, id)) {
			return FAILURE;
		}
	}
	counter = &harness.counters[harness.counter_count++];
	strcpy(counter->id, id);
	return debug_parse_address(address, &counter->cs, &counter->ip);
}

static BOOL debug_parse_capture(DEBUG_ACTION *action, char *cursor) {
	char *token;
	UINT mask;

	mask = 0;
	while ((token = debug_next_token(&cursor)) != NULL) {
		if (!strcmp(token, "registers")) {
			mask |= DEBUG_CAPTURE_REGISTERS;
		} else if (!strcmp(token, "tvram")) {
			mask |= DEBUG_CAPTURE_TVRAM;
		} else if (!strcmp(token, "screen")) {
			mask |= DEBUG_CAPTURE_SCREEN;
		} else if (!strcmp(token, "gvram")) {
			mask |= DEBUG_CAPTURE_GVRAM;
		} else {
			return FAILURE;
		}
	}
	action->argument = mask;
	return (mask != 0) ? SUCCESS : FAILURE;
}

static BOOL debug_parse_line(char *line, UINT line_number, BOOL *version_seen,
                             BOOL *event_context) {
	char *cursor;
	char *command;
	char *first;
	char *second;
	DEBUG_ACTION *action;
	UINT32 number;
	int resource;

	cursor = line;
	command = debug_next_token(&cursor);
	if (command == NULL) {
		return SUCCESS;
	}
	if (!*version_seen) {
		first = debug_next_token(&cursor);
		if (strcmp(command, "debug-script") || (first == NULL) || strcmp(first, "1") ||
		    (debug_next_token(&cursor) != NULL)) {
			fprintf(stderr, "Error: debug script line %u must be debug-script 1\n", line_number);
			return FAILURE;
		}
		*version_seen = TRUE;
		return SUCCESS;
	}
	if (!strcmp(command, "limit-frame")) {
		first = debug_next_token(&cursor);
		if ((first == NULL) || (debug_next_token(&cursor) != NULL) ||
		    (debug_parse_uint32(first, &number) != SUCCESS) || (number == 0) ||
		    (harness.frame_limit != 0)) {
			goto invalid;
		}
		harness.frame_limit = number;
		return SUCCESS;
	}
	if (!strcmp(command, "resource")) {
		first = debug_next_token(&cursor);
		second = debug_remainder(cursor);
		if ((first == NULL) || (debug_add_resource(first, second) != SUCCESS)) {
			goto invalid;
		}
		return SUCCESS;
	}
	if (!strcmp(command, "counter")) {
		first = debug_next_token(&cursor);
		second = debug_next_token(&cursor);
		if ((first == NULL) || (second == NULL) || (debug_next_token(&cursor) != NULL) ||
		    (debug_add_counter(first, second) != SUCCESS)) {
			goto invalid;
		}
		return SUCCESS;
	}
	if (!strcmp(command, "wait-frame")) {
		first = debug_next_token(&cursor);
		if ((first == NULL) || (debug_next_token(&cursor) != NULL) ||
		    (debug_parse_uint32(first, &number) != SUCCESS) ||
		    (debug_add_action(DEBUG_ACTION_WAIT_FRAME, &action) != SUCCESS)) {
			goto invalid;
		}
		action->value = number;
		*event_context = FALSE;
		return SUCCESS;
	}
	if (!strcmp(command, "input-line")) {
		const unsigned char *character;

		first = debug_remainder(cursor);
		if (first[0] == '\0') {
			goto invalid;
		}
		for (character = (const unsigned char *)first; *character != '\0'; character++) {
			if ((*character < 0x20) || (*character > 0x7e)) {
				goto invalid;
			}
		}
		if (debug_add_action(DEBUG_ACTION_INPUT_LINE, &action) != SUCCESS) {
			goto invalid;
		}
		action->text = debug_string_duplicate(first);
		if (action->text == NULL) {
			goto invalid;
		}
		*event_context = FALSE;
		return SUCCESS;
	}
	if (!strcmp(command, "enter")) {
		if ((debug_next_token(&cursor) != NULL) ||
		    (debug_add_action(DEBUG_ACTION_ENTER, &action) != SUCCESS)) {
			goto invalid;
		}
		*event_context = FALSE;
		return SUCCESS;
	}
	if (!strcmp(command, "mount-fdd")) {
		first = debug_next_token(&cursor);
		second = debug_next_token(&cursor);
		if ((first == NULL) || (second == NULL) || (debug_next_token(&cursor) != NULL) ||
		    (strcmp(first, "1") && strcmp(first, "2")) ||
		    ((resource = debug_find_resource(second)) < 0) ||
		    (debug_add_action(DEBUG_ACTION_MOUNT_FDD, &action) != SUCCESS)) {
			goto invalid;
		}
		action->argument = (UINT)(first[0] - '1');
		action->value = (UINT32)resource;
		strcpy(action->id, second);
		*event_context = FALSE;
		return SUCCESS;
	}
	if (!strcmp(command, "wait-pc")) {
		first = debug_next_token(&cursor);
		second = debug_next_token(&cursor);
		if ((first == NULL) || (second == NULL) || (debug_next_token(&cursor) != NULL) ||
		    (debug_parse_uint32(second, &number) != SUCCESS) || (number == 0) ||
		    (debug_add_action(DEBUG_ACTION_WAIT_PC, &action) != SUCCESS) ||
		    (debug_parse_address(first, &action->cs, &action->ip) != SUCCESS)) {
			goto invalid;
		}
		action->value = number;
		*event_context = TRUE;
		return SUCCESS;
	}
	if (!strcmp(command, "trace")) {
		first = debug_next_token(&cursor);
		second = debug_next_token(&cursor);
		if (!*event_context || (first == NULL) || !debug_id_valid(first) || (second == NULL) ||
		    (debug_next_token(&cursor) != NULL) ||
		    (debug_parse_uint32(second, &number) != SUCCESS) || (number == 0) ||
		    (number > 1000000) || (debug_add_action(DEBUG_ACTION_TRACE, &action) != SUCCESS)) {
			goto invalid;
		}
		strcpy(action->id, first);
		action->value = number;
		return SUCCESS;
	}
	if (!strcmp(command, "capture")) {
		first = debug_next_token(&cursor);
		if (!*event_context || (first == NULL) || !debug_id_valid(first) ||
		    (debug_add_action(DEBUG_ACTION_CAPTURE, &action) != SUCCESS) ||
		    (debug_parse_capture(action, cursor) != SUCCESS)) {
			goto invalid;
		}
		strcpy(action->id, first);
		return SUCCESS;
	}
	if (!strcmp(command, "exit")) {
		if ((debug_next_token(&cursor) != NULL) ||
		    (debug_add_action(DEBUG_ACTION_EXIT, &action) != SUCCESS)) {
			goto invalid;
		}
		*event_context = FALSE;
		return SUCCESS;
	}
invalid:
	fprintf(stderr, "Error: invalid debug script command at line %u\n", line_number);
	return FAILURE;
}

static BOOL debug_parse_buffer(char *buffer, UINT size) {
	UINT position;
	UINT start;
	UINT line_number;
	BOOL version_seen;
	BOOL event_context;

	position = 0;
	line_number = 0;
	version_seen = FALSE;
	event_context = FALSE;
	while (position < size) {
		UINT content;

		line_number++;
		start = position;
		while ((position < size) && (buffer[position] != '\r') && (buffer[position] != '\n')) {
			position++;
		}
		if ((position - start) >= DEBUG_LINE_MAX) {
			return FAILURE;
		}
		if (position < size) {
			buffer[position++] = '\0';
			if ((position < size) && (buffer[position - 1] == '\r') && (buffer[position] == '\n')) {
				position++;
			}
		}
		content = start;
		while ((buffer[content] == ' ') || (buffer[content] == '\t')) {
			content++;
		}
		if ((buffer[content] == '\0') || (buffer[content] == '#')) {
			continue;
		}
		if (debug_parse_line(buffer + start, line_number, &version_seen, &event_context) !=
		    SUCCESS) {
			return FAILURE;
		}
	}
	return (version_seen && (harness.frame_limit != 0) && (harness.action_count != 0)) ? SUCCESS
	                                                                                   : FAILURE;
}

static BOOL debug_make_path(char *path, UINT path_size, const char *id, const char *suffix) {
	int length;

	length =
	    snprintf(path, path_size, "%s%c%s%s", harness.output_dir, DEBUG_PATH_SEPARATOR, id, suffix);
	return ((length > 0) && ((UINT)length < path_size)) ? SUCCESS : FAILURE;
}

static void debug_log(const char *event, UINT32 frames, const char *id, UINT32 value) {
	if (harness.events == NULL) {
		return;
	}
	fprintf(harness.events, "%s\t%u\t%s\t%u\n", event, frames, (id != NULL) ? id : "-", value);
	fflush(harness.events);
}

static BOOL debug_write_registers(const char *id, const UPD9002_DEBUG_SNAPSHOT *snapshot) {
	char path[MAX_PATH];
	FILE *fp;

	if (debug_make_path(path, sizeof(path), id, ".registers.tsv") != SUCCESS) {
		return FAILURE;
	}
	fp = fopen(path, "wb");
	if (fp == NULL) {
		return FAILURE;
	}
	fprintf(fp,
	        "schema\tvaeg-registers-v1\nsequence\t%u\nordinal\t%u\n"
	        "clock\t%u\nax\t%04x\nbx\t%04x\ncx\t%04x\ndx\t%04x\n"
	        "si\t%04x\ndi\t%04x\nbp\t%04x\nsp\t%04x\nes\t%04x\n"
	        "cs\t%04x\nss\t%04x\nds\t%04x\nip\t%04x\nflags\t%04x\n"
	        "es_base\t%08x\ncs_base\t%08x\nss_base\t%08x\n"
	        "ds_base\t%08x\n",
	        snapshot->sequence, snapshot->ordinal, snapshot->clock, snapshot->ax, snapshot->bx,
	        snapshot->cx, snapshot->dx, snapshot->si, snapshot->di, snapshot->bp, snapshot->sp,
	        snapshot->es, snapshot->cs, snapshot->ss, snapshot->ds, snapshot->ip, snapshot->flags,
	        snapshot->es_base, snapshot->cs_base, snapshot->ss_base, snapshot->ds_base);
	return (fclose(fp) == 0) ? SUCCESS : FAILURE;
}

static BOOL debug_start_trace(const DEBUG_ACTION *action) {
	char path[MAX_PATH];

	if (!debug_harness_trace_supported() || upd9002_trace_active() || (harness.trace != NULL) ||
	    (debug_make_path(path, sizeof(path), action->id, ".trace.log") != SUCCESS)) {
		return FAILURE;
	}
	harness.trace = fopen(path, "wb");
	if (harness.trace == NULL) {
		return FAILURE;
	}
	upd9002_trace_start(harness.trace, action->value);
	return SUCCESS;
}

static void debug_close_completed_trace(void) {
	if ((harness.trace != NULL) && !upd9002_trace_active()) {
		upd9002_trace_stop();
		if (fclose(harness.trace) != 0) {
			fprintf(stderr, "Error: debug trace close failed\n");
		}
		harness.trace = NULL;
	}
}

static BOOL debug_capture(const DEBUG_ACTION *action, const UPD9002_DEBUG_SNAPSHOT *snapshot) {
	char tvram[MAX_PATH];
	char screen[MAX_PATH];
	char gvram[MAX_PATH];
	const char *tvram_path;
	const char *screen_path;
	FILE *fp;

	if ((action->argument & DEBUG_CAPTURE_REGISTERS) &&
	    (debug_write_registers(action->id, snapshot) != SUCCESS)) {
		return FAILURE;
	}
	tvram_path = NULL;
	screen_path = NULL;
	gvram[0] = '\0';
	if (action->argument & DEBUG_CAPTURE_TVRAM) {
		if (debug_make_path(tvram, sizeof(tvram), action->id, ".tvram.bin") != SUCCESS) {
			return FAILURE;
		}
		tvram_path = tvram;
	}
	if (action->argument & DEBUG_CAPTURE_SCREEN) {
		if (debug_make_path(screen, sizeof(screen), action->id, ".screen.bmp") != SUCCESS) {
			return FAILURE;
		}
		screen_path = screen;
	}
	if (action->argument & DEBUG_CAPTURE_GVRAM) {
		if (debug_make_path(gvram, sizeof(gvram), action->id, ".gvram.bin") != SUCCESS) {
			return FAILURE;
		}
	}
	if ((tvram_path != NULL) || (screen_path != NULL)) {
		if (g75_screen_capture_to(tvram_path, screen_path, action->id, FALSE) != SUCCESS) {
			return FAILURE;
		}
	}
	if (gvram[0] != '\0') {
		fp = fopen(gvram, "wb");
		if (fp == NULL) {
			return FAILURE;
		}
		if (fwrite(grphmem, 1, sizeof(grphmem), fp) != sizeof(grphmem)) {
			(void)fclose(fp);
			return FAILURE;
		}
		if (fclose(fp) != 0) {
			return FAILURE;
		}
	}
	return SUCCESS;
}

void debug_harness_clear(void) {
	UINT index;

	if (harness.trace != NULL) {
		upd9002_trace_stop();
		(void)fclose(harness.trace);
	}
	if (harness.events != NULL) {
		for (index = 0; index < harness.counter_count; index++) {
			fprintf(harness.events, "counter\t%u\t%s\t%u\n", harness.last_frame,
			        harness.counters[index].id,
			        upd9002_debug_counter_value(harness.counters[index].core_index));
		}
		(void)fclose(harness.events);
	}
	for (index = 0; index < harness.action_count; index++) {
		free(harness.actions[index].text);
	}
	for (index = 0; index < harness.resource_count; index++) {
		free(harness.resources[index].path);
	}
	ZeroMemory(&harness, sizeof(harness));
	upd9002_debug_reset();
}

BOOL debug_harness_load(const char *script_path, const char *output_dir) {
	FILE *fp;
	long length;
	char *buffer;
	char events_path[MAX_PATH];
	BOOL result;

	debug_harness_clear();
	if ((script_path == NULL) || (output_dir == NULL) || (script_path[0] == '\0') ||
	    (output_dir[0] == '\0') || (strlen(output_dir) >= sizeof(harness.output_dir))) {
		return FAILURE;
	}
	fp = fopen(script_path, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Error: debug script could not be opened\n");
		return FAILURE;
	}
	if ((fseek(fp, 0, SEEK_END) != 0) || ((length = ftell(fp)) <= 0) ||
	    (length > DEBUG_SCRIPT_MAX) || (fseek(fp, 0, SEEK_SET) != 0)) {
		fclose(fp);
		return FAILURE;
	}
	buffer = (char *)malloc((size_t)length + 1);
	if ((buffer == NULL) || (fread(buffer, 1, (size_t)length, fp) != (size_t)length)) {
		free(buffer);
		fclose(fp);
		return FAILURE;
	}
	fclose(fp);
	buffer[length] = '\0';
	result = debug_parse_buffer(buffer, (UINT)length + 1);
	free(buffer);
	if ((result == SUCCESS) && !debug_harness_trace_supported()) {
		UINT index;

		for (index = 0; index < harness.action_count; index++) {
			if (harness.actions[index].type == DEBUG_ACTION_TRACE) {
				fprintf(stderr, "Error: debug trace action requires a trace-enabled build\n");
				result = FAILURE;
				break;
			}
		}
	}
	if (result != SUCCESS) {
		debug_harness_clear();
		return FAILURE;
	}
	strcpy(harness.output_dir, output_dir);
	if (debug_make_path(events_path, sizeof(events_path), "events", ".tsv") != SUCCESS) {
		debug_harness_clear();
		return FAILURE;
	}
	harness.events = fopen(events_path, "wb");
	if (harness.events == NULL) {
		fprintf(stderr, "Error: debug output directory is not writable\n");
		debug_harness_clear();
		return FAILURE;
	}
	fprintf(harness.events, "event\tframe\tid\tvalue\n");
	harness.loaded = TRUE;
	fprintf(stderr, "debug-harness loaded actions=%u counters=%u resources=%u\n",
	        harness.action_count, harness.counter_count, harness.resource_count);
	return SUCCESS;
}

BOOL debug_harness_initialize(void) {
	UINT index;

	if (!harness.loaded) {
		return SUCCESS;
	}
	upd9002_debug_reset();
	for (index = 0; index < harness.counter_count; index++) {
		if (upd9002_debug_counter_add(harness.counters[index].cs, harness.counters[index].ip,
		                              &harness.counters[index].core_index) != SUCCESS) {
			fprintf(stderr, "Error: debug counter initialization failed\n");
			harness.exit_requested = TRUE;
			return FAILURE;
		}
	}
	harness.initialized = TRUE;
	debug_log("initialized", 0, NULL, harness.frame_limit);
	return SUCCESS;
}

BOOL debug_harness_active(void) {
	return harness.loaded;
}

static BOOL debug_execute_ready(UINT32 frames) {
	DEBUG_ACTION *action;
	DEBUG_RESOURCE *resource;
	char *text;
	size_t length;

	if ((harness.trace != NULL) && upd9002_trace_active()) {
		return SUCCESS;
	}
	while (harness.action_index < harness.action_count) {
		action = &harness.actions[harness.action_index];
		switch (action->type) {
		case DEBUG_ACTION_WAIT_FRAME:
			if (frames < action->value) {
				return SUCCESS;
			}
			debug_log("frame", frames, NULL, action->value);
			harness.action_index++;
			break;
		case DEBUG_ACTION_INPUT_LINE:
		case DEBUG_ACTION_ENTER:
			if (kbdpaste_active()) {
				return SUCCESS;
			}
			if (action->type == DEBUG_ACTION_ENTER) {
				text = debug_string_duplicate("\r");
			} else {
				length = strlen(action->text);
				text = (char *)malloc(length + 2);
				if (text != NULL) {
					CopyMemory(text, action->text, length);
					text[length] = '\r';
					text[length + 1] = '\0';
				}
			}
			if ((text == NULL) || !kbdpaste_start_text(text)) {
				free(text);
				return FAILURE;
			}
			free(text);
			debug_log("input", frames, NULL, harness.action_index + 1);
			harness.action_index++;
			return SUCCESS;
		case DEBUG_ACTION_MOUNT_FDD:
			resource = &harness.resources[action->value];
			diskdrv_setfdd((REG8)action->argument, resource->none ? NULL : resource->path, 0);
			debug_log("mount-fdd", frames, action->id, action->argument + 1);
			harness.action_index++;
			break;
		case DEBUG_ACTION_WAIT_PC:
			if (!harness.wait_armed) {
				if (upd9002_debug_wait_arm(action->cs, action->ip, action->value) != SUCCESS) {
					return FAILURE;
				}
				harness.wait_armed = TRUE;
				debug_log("wait-pc", frames, NULL, action->value);
			}
			return SUCCESS;
		case DEBUG_ACTION_TRACE:
		case DEBUG_ACTION_CAPTURE:
			fprintf(stderr, "Error: debug event action has no pending PC event\n");
			return FAILURE;
		case DEBUG_ACTION_EXIT:
			harness.exit_requested = TRUE;
			harness.action_index++;
			debug_log("exit", frames, NULL, 0);
			return SUCCESS;
		}
	}
	return SUCCESS;
}

BOOL debug_harness_after_frame(UINT32 frames) {
	BOOL result;

	if (!harness.loaded || !harness.initialized) {
		return SUCCESS;
	}
	harness.last_frame = frames;
	debug_close_completed_trace();
	result = debug_execute_ready(frames);
	if ((result == SUCCESS) && !harness.exit_requested && (frames >= harness.frame_limit)) {
		harness.exit_requested = TRUE;
		debug_log("frame-limit", frames, NULL, harness.frame_limit);
	}
	return result;
}

BOOL debug_harness_handle_pc_event(UINT32 frames) {
	UPD9002_DEBUG_SNAPSHOT snapshot;
	DEBUG_ACTION *action;

	harness.last_frame = frames;
	if (!harness.loaded || !harness.wait_armed ||
	    (upd9002_debug_event_snapshot(&snapshot) != SUCCESS) ||
	    (harness.action_index >= harness.action_count) ||
	    (harness.actions[harness.action_index].type != DEBUG_ACTION_WAIT_PC)) {
		return FAILURE;
	}
	action = &harness.actions[harness.action_index++];
	harness.wait_armed = FALSE;
	debug_log("pc", frames, NULL, snapshot.ordinal);
	while (harness.action_index < harness.action_count) {
		action = &harness.actions[harness.action_index];
		if (action->type == DEBUG_ACTION_TRACE) {
			if (debug_start_trace(action) != SUCCESS) {
				return FAILURE;
			}
			debug_log("trace", frames, action->id, action->value);
		} else if (action->type == DEBUG_ACTION_CAPTURE) {
			if (debug_capture(action, &snapshot) != SUCCESS) {
				return FAILURE;
			}
			debug_log("capture", frames, action->id, action->argument);
		} else {
			break;
		}
		harness.action_index++;
	}
	upd9002_debug_event_resume();
	return debug_execute_ready(frames);
}

BOOL debug_harness_exit_requested(void) {
	return harness.exit_requested;
}

BOOL debug_harness_selftest(void) {
	static const char valid[] = "debug-script 1\n"
	                            "limit-frame 1000\n"
	                            "resource boot local-only.d88\n"
	                            "counter dispatch e000:002a\n"
	                            "wait-frame 120\n"
	                            "input-line basic\n"
	                            "mount-fdd 1 boot\n"
	                            "wait-pc e000:0180 2\n"
	                            "trace service 16\n"
	                            "capture service registers tvram screen gvram\n"
	                            "exit\n";
	static const char invalid[] = "debug-script 1\n"
	                              "capture bad registers\n";
	static const char missing_limit[] = "debug-script 1\n"
	                                    "exit\n";
	char extended[8192];
	char *buffer;
	size_t extended_size;
	UINT index;
	BOOL passed;

	debug_harness_clear();
	buffer = debug_string_duplicate(valid);
	if (buffer == NULL) {
		return FAILURE;
	}
	passed = (debug_parse_buffer(buffer, sizeof(valid)) == SUCCESS) &&
	         (harness.resource_count == 1) && (harness.counter_count == 1) &&
	         (harness.action_count == 7) && (harness.actions[3].type == DEBUG_ACTION_WAIT_PC) &&
	         (harness.actions[4].type == DEBUG_ACTION_TRACE) && (harness.actions[5].argument == 15);
	free(buffer);
	debug_harness_clear();
	buffer = debug_string_duplicate(invalid);
	if (buffer == NULL) {
		return FAILURE;
	}
	passed = passed && (debug_parse_buffer(buffer, sizeof(invalid)) == FAILURE);
	free(buffer);
	debug_harness_clear();
	buffer = debug_string_duplicate(missing_limit);
	if (buffer == NULL) {
		return FAILURE;
	}
	passed = passed && (debug_parse_buffer(buffer, sizeof(missing_limit)) == FAILURE);
	free(buffer);
	debug_harness_clear();
	extended_size =
	    (size_t)snprintf(extended, sizeof(extended), "debug-script 1\nlimit-frame 1000\n");
	for (index = 0; index < 300; index++) {
		extended_size += (size_t)snprintf(extended + extended_size,
		                                  sizeof(extended) - extended_size, "wait-frame 1\n");
	}
	extended_size +=
	    (size_t)snprintf(extended + extended_size, sizeof(extended) - extended_size, "exit\n");
	passed = passed && (extended_size < sizeof(extended)) &&
	         (debug_parse_buffer(extended, (UINT)extended_size + 1) == SUCCESS) &&
	         (harness.action_count == 301);
	debug_harness_clear();
	return passed ? SUCCESS : FAILURE;
}
