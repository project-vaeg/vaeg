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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "cpucore.h"
#include "tests/upd9002/direct_harness.h"
#include "tests/upd9002/ssts_worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SSTS_WORKER_MAX_RECORDS 10000
#define SSTS_WORKER_MAX_RAM 65536

static int read_exact(FILE *stream, void *value, size_t size) {
	return fread(value, 1, size, stream) == size;
}

static int write_exact(FILE *stream, const void *value, size_t size) {
	return fwrite(value, 1, size, stream) == size;
}

static int read_u8(FILE *stream, uint8_t *value) {
	return read_exact(stream, value, 1);
}

static int write_u8(FILE *stream, uint8_t value) {
	return write_exact(stream, &value, 1);
}

static int read_u16(FILE *stream, uint16_t *value) {
	uint8_t bytes[2];

	if (!read_exact(stream, bytes, sizeof(bytes))) {
		return 0;
	}
	*value = (uint16_t)(bytes[0] | (bytes[1] << 8));
	return 1;
}

static int write_u16(FILE *stream, uint16_t value) {
	uint8_t bytes[2];

	bytes[0] = (uint8_t)value;
	bytes[1] = (uint8_t)(value >> 8);
	return write_exact(stream, bytes, sizeof(bytes));
}

static int read_u32(FILE *stream, uint32_t *value) {
	uint8_t bytes[4];

	if (!read_exact(stream, bytes, sizeof(bytes))) {
		return 0;
	}
	*value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
	         ((uint32_t)bytes[3] << 24);
	return 1;
}

static int write_u32(FILE *stream, uint32_t value) {
	uint8_t bytes[4];

	bytes[0] = (uint8_t)value;
	bytes[1] = (uint8_t)(value >> 8);
	bytes[2] = (uint8_t)(value >> 16);
	bytes[3] = (uint8_t)(value >> 24);
	return write_exact(stream, bytes, sizeof(bytes));
}

static int read_cpu(FILE *stream, UPD9002_HARNESS_CPU_STATE *cpu) {
	uint16_t values[14];
	uint32_t index;

	for (index = 0; index < 14; index++) {
		if (!read_u16(stream, &values[index])) {
			return 0;
		}
	}
	cpu->ax = values[0];
	cpu->bx = values[1];
	cpu->cx = values[2];
	cpu->dx = values[3];
	cpu->sp = values[4];
	cpu->bp = values[5];
	cpu->si = values[6];
	cpu->di = values[7];
	cpu->cs = values[8];
	cpu->ss = values[9];
	cpu->ds = values[10];
	cpu->es = values[11];
	cpu->ip = values[12];
	cpu->flags = values[13];
	cpu->cs_base = (uint32_t)cpu->cs << 4;
	cpu->ss_base = (uint32_t)cpu->ss << 4;
	cpu->ds_base = (uint32_t)cpu->ds << 4;
	cpu->es_base = (uint32_t)cpu->es << 4;
	cpu->remain_clock = 0x40000000;
	cpu->base_clock = 0x40000000;
	cpu->clock = 0;
	return 1;
}

static int write_cpu(FILE *stream, const UPD9002_HARNESS_CPU_STATE *cpu) {
	const uint16_t values[14] = {cpu->ax, cpu->bx, cpu->cx, cpu->dx, cpu->sp, cpu->bp, cpu->si,
	                             cpu->di, cpu->cs, cpu->ss, cpu->ds, cpu->es, cpu->ip, cpu->flags};
	uint32_t index;

	for (index = 0; index < 14; index++) {
		if (!write_u16(stream, values[index])) {
			return 0;
		}
	}
	return 1;
}

static int run_record(FILE *input_stream, FILE *output_stream) {
	UPD9002_SSTS_INPUT input;
	UPD9002_SSTS_RESULT result;
	UPD9002_SSTS_RAM_ENTRY *ram;
	uint32_t *watch;
	uint8_t *watch_values;
	uint8_t record_digest[32];
	uint32_t ram_count;
	uint32_t watch_count;
	uint32_t index;
	int success;

	ZeroMemory(&input, sizeof(input));
	ZeroMemory(&result, sizeof(result));
	ram = NULL;
	watch = NULL;
	watch_values = NULL;
	success = 0;
	if (!read_exact(input_stream, record_digest, sizeof(record_digest)) ||
	    !read_cpu(input_stream, &input.cpu) || !read_u32(input_stream, &ram_count) ||
	    (ram_count > SSTS_WORKER_MAX_RAM)) {
		goto cleanup;
	}
	if (ram_count != 0) {
		ram = (UPD9002_SSTS_RAM_ENTRY *)malloc(sizeof(*ram) * ram_count);
		if (ram == NULL) {
			goto cleanup;
		}
	}
	for (index = 0; index < ram_count; index++) {
		if (!read_u32(input_stream, &ram[index].address) ||
		    !read_u8(input_stream, &ram[index].value)) {
			goto cleanup;
		}
	}
	if (!read_u32(input_stream, &watch_count) || (watch_count > SSTS_WORKER_MAX_RAM)) {
		goto cleanup;
	}
	if (watch_count != 0) {
		watch = (uint32_t *)malloc(sizeof(*watch) * watch_count);
		watch_values = (uint8_t *)malloc(watch_count);
		if ((watch == NULL) || (watch_values == NULL)) {
			goto cleanup;
		}
	}
	for (index = 0; index < watch_count; index++) {
		if (!read_u32(input_stream, &watch[index])) {
			goto cleanup;
		}
	}
	input.ram = ram;
	input.ram_count = ram_count;
	input.watch_addresses = watch;
	input.watch_count = watch_count;
	result.watch_values = watch_values;
	if (upd9002_harness_run_ssts(&input, &result) != SUCCESS) {
		goto cleanup;
	}
	if (!write_exact(output_stream, record_digest, sizeof(record_digest)) ||
	    !write_u8(output_stream, result.termination) ||
	    !write_u32(output_stream, result.interrupt_count) ||
	    !write_u8(output_stream, result.last_interrupt_vector) ||
	    !write_cpu(output_stream, &result.cpu) || !write_u32(output_stream, result.watch_count) ||
	    !write_exact(output_stream, result.watch_values, result.watch_count) ||
	    !write_u32(output_stream, result.io_count)) {
		goto cleanup;
	}
	for (index = 0; index < result.io_count; index++) {
		if (!write_u16(output_stream, result.io[index].port) ||
		    !write_u8(output_stream, result.io[index].value) ||
		    !write_u8(output_stream, result.io[index].direction)) {
			goto cleanup;
		}
	}
	success = 1;

cleanup:
	free(watch_values);
	free(watch);
	free(ram);
	return success;
}

int upd9002_ssts_worker_main(int argc, char **argv) {
	static const uint8_t request_magic[8] = {'V', '2', '0', 'R', 'E', 'Q', '2', 0};
	static const uint8_t response_magic[8] = {'V', '2', '0', 'R', 'S', 'P', '2', 0};
	uint8_t actual_magic[8];
	FILE *input_stream;
	FILE *output_stream;
	uint32_t record_count;
	uint32_t index;
	int success;

	if (argc != 3) {
		fprintf(stderr, "usage: --upd9002-ssts-worker request.bin response.bin\n");
		return EXIT_FAILURE;
	}
	input_stream = fopen(argv[1], "rb");
	if (input_stream == NULL) {
		fprintf(stderr, "ssts-worker: cannot open request\n");
		return EXIT_FAILURE;
	}
	output_stream = fopen(argv[2], "wb");
	if (output_stream == NULL) {
		fprintf(stderr, "ssts-worker: cannot open response\n");
		fclose(input_stream);
		return EXIT_FAILURE;
	}
	success = read_exact(input_stream, actual_magic, sizeof(actual_magic)) &&
	          (memcmp(actual_magic, request_magic, sizeof(request_magic)) == 0) &&
	          read_u32(input_stream, &record_count) && (record_count <= SSTS_WORKER_MAX_RECORDS) &&
	          write_exact(output_stream, response_magic, sizeof(response_magic)) &&
	          write_u32(output_stream, record_count);
	upd9002_core_initialize();
	for (index = 0; success && (index < record_count); index++) {
		success = run_record(input_stream, output_stream);
	}
	upd9002_core_deinitialize();
	if (fgetc(input_stream) != EOF) {
		success = 0;
	}
	if ((fclose(output_stream) != 0) || (fclose(input_stream) != 0)) {
		success = 0;
	}
	if (!success) {
		fprintf(stderr, "ssts-worker: malformed request or execution failure\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
