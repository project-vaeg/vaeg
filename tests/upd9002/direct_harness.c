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
#include "machine/nevent.h"
#include "iocore.h"
#include "upd9002_trace.h"
#include "tests/upd9002/direct_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static UPD9002_SSTS_RESULT *ssts_io_result;
static int ssts_io_overflow;

int upd9002_ssts_io_active(void) {
	return ssts_io_result != NULL;
}

static void ssts_io_append(uint16_t port, uint8_t value, uint8_t direction) {
	UPD9002_SSTS_IO_EVENT *event;

	if (ssts_io_result->io_count >= UPD9002_SSTS_IO_CAPACITY) {
		ssts_io_overflow = 1;
		return;
	}
	event = &ssts_io_result->io[ssts_io_result->io_count++];
	event->port = port;
	event->value = value;
	event->direction = direction;
}

uint8_t upd9002_ssts_io_read(uint16_t port) {
	ssts_io_append(port, 0xff, 0);
	return 0xff;
}

void upd9002_ssts_io_write(uint16_t port, uint8_t value) {
	ssts_io_append(port, value, 1);
}

void upd9002_ssts_interrupt(uint8_t vector) {
	if (ssts_io_result != NULL) {
		ssts_io_result->interrupt_count++;
		ssts_io_result->last_interrupt_vector = vector;
	}
}

static void set_cpu(const UPD9002_HARNESS_CPU_STATE *state) {
	CPU_AX = state->ax;
	CPU_BX = state->bx;
	CPU_CX = state->cx;
	CPU_DX = state->dx;
	CPU_SI = state->si;
	CPU_DI = state->di;
	CPU_BP = state->bp;
	CPU_SP = state->sp;
	CPU_ES = state->es;
	CPU_CS = state->cs;
	CPU_SS = state->ss;
	CPU_DS = state->ds;
	CPU_IP = state->ip;
	CPU_FLAG = state->flags;
	ES_BASE = state->es_base;
	CS_BASE = state->cs_base;
	SS_BASE = state->ss_base;
	DS_BASE = state->ds_base;
	upd9002_core_context.s.ss_fix = state->ss_base;
	upd9002_core_context.s.ds_fix = state->ds_base;
	CPU_REMCLOCK = state->remain_clock;
	CPU_BASECLOCK = state->base_clock;
	CPU_CLOCK = state->clock;
	CPU_ADRSMASK = 0xfffff;
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
}

static void get_cpu(UPD9002_HARNESS_CPU_STATE *state) {
	state->ax = CPU_AX;
	state->bx = CPU_BX;
	state->cx = CPU_CX;
	state->dx = CPU_DX;
	state->si = CPU_SI;
	state->di = CPU_DI;
	state->bp = CPU_BP;
	state->sp = CPU_SP;
	state->es = CPU_ES;
	state->cs = CPU_CS;
	state->ss = CPU_SS;
	state->ds = CPU_DS;
	state->ip = CPU_IP;
	state->flags = CPU_FLAG;
	state->es_base = ES_BASE;
	state->cs_base = CS_BASE;
	state->ss_base = SS_BASE;
	state->ds_base = DS_BASE;
	state->remain_clock = CPU_REMCLOCK;
	state->base_clock = CPU_BASECLOCK;
	state->clock = CPU_CLOCK;
}

static uint32_t hash_ram(uint32_t address, uint32_t size) {
	uint32_t hash;
	uint32_t index;

	hash = 2166136261u;
	for (index = 0; index < size; index++) {
		hash ^= mem[(address + index) & 0xfffff];
		hash *= 16777619u;
	}
	return hash;
}

int upd9002_harness_run(const UPD9002_HARNESS_INPUT *input, UPD9002_HARNESS_RESULT *result) {
	Upd9002RuntimeState saved_cpu;
	_DMAC saved_dmac;
	uint8_t *saved_ram;
	uint32_t index;

	if ((input == NULL) || (result == NULL) || (input->program_size == 0) ||
	    (input->program_size > UPD9002_HARNESS_PROGRAM_CAPACITY) ||
	    (input->ram_size > UPD9002_HARNESS_RAM_CAPACITY) ||
	    (input->program_address + input->program_size > 0x10000) ||
	    (input->ram_address + input->ram_size > 0x10000) || (input->step_count == 0) ||
	    (input->step_count > 256)) {
		return FAILURE;
	}
	saved_ram = (uint8_t *)malloc(0x10000);
	if (saved_ram == NULL) {
		return FAILURE;
	}
	saved_cpu = CPU_STATSAVE;
	saved_dmac = dmac;
	CopyMemory(saved_ram, mem, 0x10000);

	ZeroMemory(&CPU_STATSAVE, sizeof(CPU_STATSAVE));
	ZeroMemory(&dmac, sizeof(dmac));
	set_cpu(&input->cpu);
	for (index = 0; index < input->ram_size; index++) {
		mem[input->ram_address + index] = input->ram[index];
	}
	CopyMemory(mem + input->program_address, input->program, input->program_size);
	for (index = 0; index < input->step_count; index++) {
		upd9002_core_step();
	}
	ZeroMemory(result, sizeof(*result));
	get_cpu(&result->cpu);
	result->ram_hash = hash_ram(input->ram_address, input->ram_size);
	result->executed_steps = input->step_count;
	result->termination = (CPU_REMCLOCK < 0) ? 1 : 0;

	CPU_STATSAVE = saved_cpu;
	dmac = saved_dmac;
	CopyMemory(mem, saved_ram, 0x10000);
	free(saved_ram);
	return SUCCESS;
}

int upd9002_harness_run_ssts(const UPD9002_SSTS_INPUT *input, UPD9002_SSTS_RESULT *result) {
	uint32_t index;

	if ((input == NULL) || (result == NULL) || (input->ram_count > 0x100000) ||
	    (input->watch_count > 0x100000) || ((input->ram_count != 0) && (input->ram == NULL)) ||
	    ((input->watch_count != 0) &&
	     ((input->watch_addresses == NULL) || (result->watch_values == NULL)))) {
		return FAILURE;
	}
	for (index = 0; index < input->ram_count; index++) {
		if (input->ram[index].address > 0xfffff) {
			return FAILURE;
		}
	}
	for (index = 0; index < input->watch_count; index++) {
		if (input->watch_addresses[index] > 0xfffff) {
			return FAILURE;
		}
	}

	ZeroMemory(&CPU_STATSAVE, sizeof(CPU_STATSAVE));
	ZeroMemory(&dmac, sizeof(dmac));
	ZeroMemory(mem, 0x100000);
	set_cpu(&input->cpu);
	for (index = 0; index < input->ram_count; index++) {
		mem[input->ram[index].address] = input->ram[index].value;
	}
	result->termination = UPD9002_SSTS_TERMINATION_NORMAL;
	result->interrupt_count = 0;
	result->last_interrupt_vector = 0;
	result->watch_count = input->watch_count;
	result->io_count = 0;
	ssts_io_result = result;
	ssts_io_overflow = 0;
	upd9002_core_step();
	ssts_io_result = NULL;
	if (ssts_io_overflow) {
		return FAILURE;
	}
	get_cpu(&result->cpu);
	result->cpu.flags = (uint16_t)((result->cpu.flags & 0x0fff) | 0xf002);
	if (CPU_REMCLOCK < 0) {
		result->termination = UPD9002_SSTS_TERMINATION_HALT;
	}
	for (index = 0; index < input->watch_count; index++) {
		result->watch_values[index] = mem[input->watch_addresses[index] & 0xfffff];
	}
	return SUCCESS;
}

static int hex_program(const char *text, uint8_t *program, uint32_t *size) {
	uint32_t length;
	uint32_t index;
	unsigned int value;
	char byte_text[3];

	length = (uint32_t)strlen(text);
	if ((length == 0) || (length & 1) || (length / 2 > UPD9002_HARNESS_PROGRAM_CAPACITY)) {
		return FAILURE;
	}
	byte_text[2] = '\0';
	for (index = 0; index < length / 2; index++) {
		byte_text[0] = text[index * 2];
		byte_text[1] = text[index * 2 + 1];
		if (sscanf(byte_text, "%x", &value) != 1 || value > 0xff) {
			return FAILURE;
		}
		program[index] = (uint8_t)value;
	}
	*size = length / 2;
	return SUCCESS;
}

static int run_manifest_case(char **field) {
	UPD9002_HARNESS_INPUT input;
	UPD9002_HARNESS_RESULT first;
	UPD9002_HARNESS_RESULT second;
	char *end;
	unsigned long steps;

	ZeroMemory(&input, sizeof(input));
	input.cpu.ax = 0x0011;
	input.cpu.bx = strstr(field[0], "fault") ? 0 : 3;
	input.cpu.cx = 1;
	input.cpu.dx = 0x007f;
	input.cpu.sp = 0x8000;
	input.cpu.flags = 0xf002;
	input.cpu.remain_clock = 1000;
	input.program_address = 0x2000;
	input.cpu.ip = (uint16_t)input.program_address;
	input.ram_address = 0;
	input.ram_size = 0x1000;
	if (hex_program(field[4], input.program, &input.program_size) != SUCCESS) {
		return FAILURE;
	}
	end = NULL;
	steps = strtoul(field[5], &end, 10);
	if ((end == field[5]) || (*end != '\0') || (steps == 0) || (steps > 256)) {
		return FAILURE;
	}
	input.step_count = (uint32_t)steps;
	if ((upd9002_harness_run(&input, &first) != SUCCESS) ||
	    (upd9002_harness_run(&input, &second) != SUCCESS)) {
		return FAILURE;
	}
	if (memcmp(&first, &second, sizeof(first)) != 0) {
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_harness_run_manifest(const char *path) {
	FILE *stream;
	char line[512];
	char *field[7];
	char *cursor;
	char *comma;
	uint32_t count;
	uint32_t index;

	stream = fopen(path, "rb");
	if (stream == NULL) {
		return FAILURE;
	}
	count = 0;
	if (fgets(line, sizeof(line), stream) == NULL) {
		fclose(stream);
		return FAILURE;
	}
	while (fgets(line, sizeof(line), stream) != NULL) {
		line[strcspn(line, "\r\n")] = '\0';
		cursor = line;
		for (index = 0; index < 6; index++) {
			field[index] = cursor;
			comma = strchr(cursor, ',');
			if (comma == NULL) {
				fclose(stream);
				return FAILURE;
			}
			*comma = '\0';
			cursor = comma + 1;
		}
		field[6] = cursor;
		for (index = 0; index < 7; index++) {
			if (field[index][0] == '\0') {
				fclose(stream);
				return FAILURE;
			}
		}
		if (run_manifest_case(field) != SUCCESS) {
			fprintf(stderr, "upd9002-harness: failed case %s\n", field[0]);
			fclose(stream);
			return FAILURE;
		}
		count++;
	}
	fclose(stream);
	if (count != 193) {
		fprintf(stderr, "upd9002-harness: expected 193 cases, got %u\n", count);
		return FAILURE;
	}
	fprintf(stderr, "upd9002-harness: %u manifest-derived cases passed\n", count);
	return SUCCESS;
}
