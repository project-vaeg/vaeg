/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upd8087/upd8087.h"

#include <stdio.h>
#include <string.h>

#include "platform.h"
#include "softfloat.h"

typedef struct {
	uint8_t bytes[256];
	unsigned reads;
	unsigned writes;
} TEST_MEMORY;

static const UPD8087_RAW80 TEST_ZERO = {UINT64_C(0), 0};
static const UPD8087_RAW80 TEST_ONE = {UINT64_C(0x8000000000000000), 0x3fff};
static const UPD8087_RAW80 TEST_ONE_HALF = {UINT64_C(0xc000000000000000), 0x3fff};
static const UPD8087_RAW80 TEST_HALF = {UINT64_C(0x8000000000000000), 0x3ffe};
static const UPD8087_RAW80 TEST_QUARTER = {UINT64_C(0x8000000000000000), 0x3ffd};
static const UPD8087_RAW80 TEST_TWO = {UINT64_C(0x8000000000000000), 0x4000};
static const UPD8087_RAW80 TEST_THREE = {UINT64_C(0xc000000000000000), 0x4000};
static const UPD8087_RAW80 TEST_SEVEN = {UINT64_C(0xe000000000000000), 0x4001};
static const UPD8087_RAW80 TEST_EIGHT = {UINT64_C(0x8000000000000000), 0x4002};
static const UPD8087_RAW80 TEST_NEG_SEVEN = {
	UINT64_C(0xe000000000000000), 0xc001
};
static const UPD8087_RAW80 TEST_TEN = {UINT64_C(0xa000000000000000), 0x4002};
static const UPD8087_RAW80 TEST_FIVE = {UINT64_C(0xa000000000000000), 0x4001};
static const UPD8087_RAW80 TEST_TWO_POW_70 = {
	UINT64_C(0x8000000000000000), 0x4045
};
static const UPD8087_RAW80 TEST_NEG_32768 = {
	UINT64_C(0x8000000000000000), 0xc00e
};
static const UPD8087_RAW80 TEST_NEG_32768_HALF = {
	UINT64_C(0x8000800000000000), 0xc00e
};
static const UPD8087_RAW80 TEST_POS_32768 = {
	UINT64_C(0x8000000000000000), 0x400e
};
static const UPD8087_RAW80 TEST_UNNORMAL_SMALL = {
	UINT64_C(0x4000000000000000), 0x3fff
};
static const UPD8087_RAW80 TEST_UNNORMAL_LARGE = {
	UINT64_C(0x4000000000000000), 0x4001
};
static const UPD8087_RAW80 TEST_UNNORMAL_SHORT_UNDERFLOW = {
	UINT64_C(0x4000000000000000), 0x3f80
};
static const UPD8087_RAW80 TEST_UNNORMAL_SHORT_INVALID = {
	UINT64_C(0x4000000000000000), 0x3f82
};
static const UPD8087_RAW80 TEST_PI_OVER_FOUR = {
	UINT64_C(0xc90fdaa22168c235), 0x3ffe
};
static const UPD8087_RAW80 TEST_POS_INFINITY = {
	UINT64_C(0x8000000000000000), 0x7fff
};
static const UPD8087_RAW80 TEST_NEG_INFINITY = {
	UINT64_C(0x8000000000000000), 0xffff
};
static const UPD8087_RAW80 TEST_NAN = {
	UINT64_C(0xc000000000000001), 0x7fff
};

static void fail(const char *what) {
	fprintf(stderr, "8087 test failure: %s\n", what);
}

#define CHECK(condition, text) \
	do { \
		if (!(condition)) { \
			fail(text); \
			return 1; \
		} \
	} while (0)

static uint8_t read8(void *opaque, uint32_t address) {
	TEST_MEMORY *memory = (TEST_MEMORY *)opaque;

	memory->reads++;
	return memory->bytes[address & 0xff];
}

static void write8(void *opaque, uint32_t address, uint8_t value) {
	TEST_MEMORY *memory = (TEST_MEMORY *)opaque;

	memory->writes++;
	memory->bytes[address & 0xff] = value;
}

static UPD8087_EXEC_RESULT execute_register(UPD8087_STATE *state, uint8_t opcode,
	uint8_t modrm) {
	return upd8087_execute(state, opcode, modrm, NULL);
}

static UPD8087_EXEC_RESULT execute_memory(UPD8087_STATE *state, TEST_MEMORY *memory,
	uint8_t opcode, uint8_t modrm) {
	UPD8087_OPERAND operand;

	memset(&operand, 0, sizeof(operand));
	operand.bus.opaque = memory;
	operand.bus.read8 = read8;
	operand.bus.write8 = write8;
	operand.address = 0x20;
	return upd8087_execute(state, opcode, modrm, &operand);
}

static void reset_enabled(UPD8087_STATE *state) {
	UPD8087_CONFIG config;

	config.enabled = true;
	config.clock_hz = UPD8087_DEFAULT_CLOCK_HZ;
	upd8087_initialize(state, &config);
}

static void set_stack_value(UPD8087_STATE *state, unsigned logical,
	UPD8087_RAW80 value) {
	unsigned physical = (state->top + logical) & 7;

	state->reg[physical] = value;
	state->tag[physical] = (value.signif == 0 &&
		(value.sign_exp & 0x7fff) == 0) ? UPD8087_TAG_ZERO : UPD8087_TAG_VALID;
}

static bool raw_equal(UPD8087_RAW80 lhs, UPD8087_RAW80 rhs) {
	return lhs.signif == rhs.signif && lhs.sign_exp == rhs.sign_exp;
}

static void store_test64(uint8_t *bytes, uint64_t value) {
	unsigned i;

	for (i = 0; i < 8; i++) {
		bytes[i] = (uint8_t)(value >> (i * 8));
	}
}

static uint64_t load_test64(const uint8_t *bytes) {
	uint64_t value = 0;
	unsigned i;

	for (i = 0; i < 8; i++) {
		value |= (uint64_t)bytes[i] << (i * 8);
	}
	return value;
}

static void store_test32(uint8_t *bytes, uint32_t value) {
	unsigned i;

	for (i = 0; i < 4; i++) {
		bytes[i] = (uint8_t)(value >> (i * 8));
	}
}

static int test_config_and_timing(void) {
	UPD8087_CONFIG config;
	UPD8087_STATE state;

	upd8087_config_default(&config);
	CHECK(!config.enabled, "default 8087 is disabled");
	CHECK(config.clock_hz == 8000000U, "default 8087 clock is 8 MHz");
	CHECK(upd8087_clock_valid(1000000U), "minimum clock accepted");
	CHECK(upd8087_clock_valid(20000000U), "maximum clock accepted");
	CHECK(!upd8087_clock_valid(999999U), "below-minimum clock rejected");
	CHECK(!upd8087_clock_valid(20000001U), "above-maximum clock rejected");

	config.enabled = true;
	config.clock_hz = 5000000U;
	upd8087_initialize(&state, &config);
	CHECK(upd8087_service_ticks(&state, 100, 1000000U) == 20,
	      "100 NDP clocks at 5 MHz convert to 20 CPU ticks");
	config.clock_hz = 10000000U;
	upd8087_apply_config(&state, &config);
	CHECK(upd8087_service_ticks(&state, 100, 1000000U) == 10,
	      "100 NDP clocks at 10 MHz convert to 10 CPU ticks");
	config.clock_hz = 7000000U;
	upd8087_apply_config(&state, &config);
	CHECK(upd8087_service_ticks(&state, 100, 1000000U) == 14,
	      "custom clock uses integer floor conversion");
	CHECK(upd8087_service_ticks(&state, 100, 1000000U) == 14,
	      "custom clock carries integer residue");
	CHECK(state.service_remainder == 4000000U,
	      "custom clock residue is retained exactly");
	return 0;
}

static int test_debug_view(void) {
	UPD8087_STATE state;
	UPD8087_DEBUG_VIEW view;

	reset_enabled(&state);
	state.instruction_address = 0x12345;
	state.data_address = 0x54321;
	state.opcode = 0xd9e8;
	set_stack_value(&state, 0, TEST_TEN);
	state.service_remainder = 17;
	state.pending_interrupt = true;
	state.busy = true;
	upd8087_debug_view(&state, &view);
	CHECK(view.enabled && view.clock_hz == UPD8087_DEFAULT_CLOCK_HZ,
	      "debug view exposes immutable configuration snapshot");
	CHECK(view.top == state.top && view.control == state.control &&
	      view.status == state.status && view.tag_word == upd8087_tag_word(&state),
	      "debug view exposes control/status/TOP/tag state");
	CHECK(view.instruction_address == state.instruction_address &&
	      view.data_address == state.data_address && view.opcode == state.opcode &&
	      view.service_remainder == state.service_remainder && view.busy &&
	      view.pending_interrupt && raw_equal(view.reg[state.top], TEST_TEN),
	      "debug view exposes pointers/opcode/timing/stack state");
	view.reg[state.top] = TEST_ZERO;
	view.tag[state.top] = UPD8087_TAG_EMPTY;
	CHECK(raw_equal(state.reg[state.top], TEST_TEN) &&
	      state.tag[state.top] != UPD8087_TAG_EMPTY,
	      "debug view mutation cannot mutate architectural state");
	return 0;
}

static int test_environment_codecs(void) {
	UPD8087_STATE state;
	UPD8087_STATE restored;
	uint8_t environment[14];
	uint8_t image[94];
	unsigned i;

	reset_enabled(&state);
	state.control = UPD8087_DEFAULT_CONTROL;
	state.top = 3;
	state.status = (uint16_t)((0x1321 & (uint16_t)~UPD8087_STATUS_TOP) |
		                         ((uint16_t)state.top << 11));
	state.instruction_address = 0xabcde;
	state.data_address = 0x54321;
	state.opcode = 0x5a3;
	for (i = 0; i < 8; i++) {
		state.reg[i].signif = UINT64_C(0x8000000000000000) + i;
		state.reg[i].sign_exp = (uint16_t)(0x3fff + i);
		state.tag[i] = (uint8_t)(i & 3);
	}
	upd8087_store_environment(&state, environment);
	memset(&restored, 0, sizeof(restored));
	restored.enabled = true;
	restored.clock_hz = UPD8087_DEFAULT_CLOCK_HZ;
	upd8087_load_environment(&restored, environment);
	CHECK(restored.control == state.control, "environment control word round-trip");
	CHECK((restored.status & 0x7fff) == (state.status & 0x7fff),
	      "environment status word round-trip");
	CHECK(restored.top == state.top, "environment TOP round-trip");
	CHECK(restored.instruction_address == state.instruction_address,
	      "environment instruction pointer round-trip");
	CHECK(restored.data_address == state.data_address,
	      "environment data pointer round-trip");
	CHECK(restored.opcode == state.opcode, "environment opcode round-trip");
	CHECK(upd8087_tag_word(&restored) == upd8087_tag_word(&state),
	      "environment tag word round-trip");

	upd8087_store_state(&state, image);
	memset(&restored, 0, sizeof(restored));
	restored.enabled = true;
	restored.clock_hz = UPD8087_DEFAULT_CLOCK_HZ;
	upd8087_load_state(&restored, image);
	CHECK(restored.top == state.top, "FSAVE TOP round-trip");
	for (i = 0; i < 8; i++) {
		unsigned physical = (state.top + i) & 7;
		CHECK(restored.reg[physical].signif == state.reg[physical].signif,
		      "FSAVE register significand ordering");
		CHECK(restored.reg[physical].sign_exp == state.reg[physical].sign_exp,
		      "FSAVE register exponent ordering");
	}
	return 0;
}

static int test_environment_memory_operations(void) {
	UPD8087_STATE state;
	UPD8087_STATE restored;
	TEST_MEMORY memory;
	uint8_t saved_image[UPD8087_STATE_IMAGE_SIZE];
	uint16_t saved_control;
	uint16_t saved_status;
	uint64_t saved_remainder;
	uint32_t saved_clock;

	memset(&memory, 0, sizeof(memory));
	reset_enabled(&state);
	state.clock_hz = 8000000U;
	state.service_remainder = 77;
	state.control = (uint16_t)(UPD8087_DEFAULT_CONTROL &
	                           (uint16_t)~(UPD8087_STATUS_IE | 0x0080));
	state.status = (uint16_t)(UPD8087_STATUS_IE | UPD8087_STATUS_C0 |
	                          UPD8087_STATUS_IR | UPD8087_STATUS_B);
	state.busy = true;
	state.pending_interrupt = true;
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_THREE);
	saved_control = state.control;
	saved_status = (uint16_t)(state.status & (uint16_t)~UPD8087_STATUS_B);
	CHECK(execute_memory(&state, &memory, 0xd9, 0x30) == UPD8087_EXECUTED,
	      "FSTENV executes through the production memory bus");
	CHECK(memory.reads == 0 && memory.writes == 14,
	      "FSTENV writes exactly one 14-byte environment");
	CHECK(((uint16_t)memory.bytes[0x20] |
	       ((uint16_t)memory.bytes[0x21] << 8)) == saved_control &&
	      ((uint16_t)memory.bytes[0x22] |
	       ((uint16_t)memory.bytes[0x23] << 8)) == saved_status,
	      "FSTENV stores the pre-mask control and status");
	CHECK((state.control & 0x003f) == 0x003f &&
	      (state.control & 0x0080) == 0 &&
	      (state.status & UPD8087_STATUS_IE) != 0 &&
	      (state.status & (UPD8087_STATUS_IR | UPD8087_STATUS_B)) == 0 &&
	      !state.busy && !state.pending_interrupt,
	      "FSTENV masks exceptions while preserving IEM and clears request state");
	CHECK(raw_equal(state.reg[state.top], TEST_TEN) &&
	      raw_equal(state.reg[(state.top + 1) & 7], TEST_THREE),
	      "FSTENV preserves the register stack");

	memset(&memory, 0, sizeof(memory));
	reset_enabled(&state);
	state.clock_hz = 8000000U;
	state.service_remainder = 77;
	state.control = (uint16_t)(UPD8087_DEFAULT_CONTROL &
	                           (uint16_t)~UPD8087_STATUS_PE);
	state.status = (uint16_t)(UPD8087_STATUS_C3 | UPD8087_STATUS_PE);
	state.top = 3;
	state.status = (uint16_t)((state.status & (uint16_t)~UPD8087_STATUS_TOP) |
	                          ((uint16_t)state.top << 11));
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_THREE);
	saved_clock = state.clock_hz;
	saved_remainder = state.service_remainder;
	CHECK(execute_memory(&state, &memory, 0xdd, 0x30) == UPD8087_EXECUTED,
	      "FSAVE executes through the production memory bus");
	CHECK(memory.reads == 0 && memory.writes == 94,
	      "FSAVE writes exactly one 94-byte state image");
	memcpy(saved_image, memory.bytes + 0x20, sizeof(saved_image));
	CHECK(state.control == UPD8087_DEFAULT_CONTROL && state.status == 0 &&
	      state.top == 0 && state.clock_hz == saved_clock &&
	      state.service_remainder == saved_remainder,
	      "FSAVE initializes architectural state without changing oscillator state");
	CHECK(state.tag[0] == UPD8087_TAG_EMPTY &&
	      state.tag[1] == UPD8087_TAG_EMPTY &&
	      state.tag[2] == UPD8087_TAG_EMPTY &&
	      state.tag[3] == UPD8087_TAG_EMPTY &&
	      state.tag[4] == UPD8087_TAG_EMPTY &&
	      state.tag[5] == UPD8087_TAG_EMPTY &&
	      state.tag[6] == UPD8087_TAG_EMPTY &&
	      state.tag[7] == UPD8087_TAG_EMPTY,
	      "FSAVE empties all stack tags after writing the image");

	memset(&restored, 0, sizeof(restored));
	restored.enabled = true;
	restored.clock_hz = UPD8087_DEFAULT_CLOCK_HZ;
	upd8087_load_state(&restored, saved_image);
	CHECK(restored.top == 3 && raw_equal(restored.reg[restored.top], TEST_TEN) &&
	      raw_equal(restored.reg[(restored.top + 1) & 7], TEST_THREE),
	      "FSAVE image preserves logical stack order");

	memset(&memory, 0, sizeof(memory));
memcpy(memory.bytes + 0x20, saved_image, sizeof(saved_image));
reset_enabled(&state);
	state.clock_hz = 8000000U;
	state.service_remainder = 91;
	CHECK(execute_memory(&state, &memory, 0xdd, 0x20) == UPD8087_EXECUTED,
	      "FRSTOR executes through the production memory bus");
	CHECK(memory.reads == 94 &&
	      state.top == 3 && raw_equal(state.reg[state.top], TEST_TEN) &&
      raw_equal(state.reg[(state.top + 1) & 7], TEST_THREE) &&
      state.clock_hz == 8000000U && state.service_remainder == 91,
	      "FRSTOR restores the logical stack without changing timing state");

	memset(&memory, 0, sizeof(memory));
	memcpy(memory.bytes + 0x20, saved_image, 14);
	reset_enabled(&state);
	CHECK(execute_memory(&state, &memory, 0xd9, 0x20) == UPD8087_EXECUTED,
	      "FLDENV executes through the production memory bus");
	CHECK(memory.reads == 14 && state.top == 3 &&
	      state.control == (uint16_t)(UPD8087_DEFAULT_CONTROL &
                                   (uint16_t)~UPD8087_STATUS_PE),
	      "FLDENV restores the saved environment");
	return 0;
}

static bool rejected_machine_image_preserves(UPD8087_STATE *state,
	const uint8_t image[UPD8087_STATE_IMAGE_SIZE]) {
	uint8_t before[UPD8087_STATE_IMAGE_SIZE];
	uint8_t after[UPD8087_STATE_IMAGE_SIZE];

	if (!upd8087_store_machine_state(state, before, sizeof(before)) ||
	    upd8087_load_machine_state(state, image, UPD8087_STATE_IMAGE_SIZE) ||
	    !upd8087_store_machine_state(state, after, sizeof(after))) {
		return false;
	}
	return memcmp(before, after, sizeof(before)) == 0;
}

static int test_machine_state_codec(void) {
	UPD8087_STATE state;
	UPD8087_STATE restored;
	uint8_t image[UPD8087_STATE_IMAGE_SIZE];
	uint8_t pending_image[UPD8087_STATE_IMAGE_SIZE];
	uint8_t malformed[UPD8087_STATE_IMAGE_SIZE];
	uint8_t restored_image[UPD8087_STATE_IMAGE_SIZE];

	reset_enabled(&state);
	state.service_remainder = 1234;
	state.last_ndp_cycles = 456;
	state.last_cpu_ticks = 789;
	state.instruction_address = 0xabcde;
	state.data_address = 0x54321;
	state.opcode = 0x5a3;
	state.control = UPD8087_DEFAULT_CONTROL;
	state.status = (uint16_t)((state.top << 11) | UPD8087_STATUS_C0);
	set_stack_value(&state, 0, TEST_TEN);
	CHECK(upd8087_store_machine_state(&state, image, sizeof(image)),
	      "machine state image stores");

	memset(&restored, 0, sizeof(restored));
	CHECK(upd8087_load_machine_state(&restored, image, sizeof(image)),
	      "machine state image loads");
	CHECK(upd8087_store_machine_state(&restored, restored_image,
	                                  sizeof(restored_image)),
	      "restored machine state image stores");
	CHECK(memcmp(image, restored_image, sizeof(image)) == 0,
	      "machine state image round-trips byte-for-byte");
	state.control &= (uint16_t)~UPD8087_STATUS_IE;
	state.status |= UPD8087_STATUS_IE | UPD8087_STATUS_IR |
	                UPD8087_STATUS_B;
	state.busy = true;
	state.pending_interrupt = true;
	CHECK(upd8087_store_machine_state(&state, pending_image,
	                                  sizeof(pending_image)),
	      "machine state stores simultaneous BUSY and pending interrupt");
	memset(&restored, 0, sizeof(restored));
	CHECK(upd8087_load_machine_state(&restored, pending_image,
	                                 sizeof(pending_image)) && restored.busy &&
	      restored.pending_interrupt &&
	      (restored.status & (UPD8087_STATUS_IR | UPD8087_STATUS_B)) ==
	      (UPD8087_STATUS_IR | UPD8087_STATUS_B),
	      "machine state restores simultaneous BUSY and pending interrupt");
	upd8087_acknowledge_interrupt(&state);
	CHECK(!state.pending_interrupt &&
	      (state.status & (UPD8087_STATUS_IR | UPD8087_STATUS_B)) ==
	      (UPD8087_STATUS_IR | UPD8087_STATUS_B) &&
	      upd8087_store_machine_state(&state, pending_image,
	                                  sizeof(pending_image)),
	      "machine state stores an acknowledged active exception");
	memset(&restored, 0, sizeof(restored));
	CHECK(upd8087_load_machine_state(&restored, pending_image,
	                                 sizeof(pending_image)) && !restored.pending_interrupt &&
	      restored.busy &&
	      (restored.status & (UPD8087_STATUS_IR | UPD8087_STATUS_B)) ==
	      (UPD8087_STATUS_IR | UPD8087_STATUS_B),
	      "machine state restores acknowledged active exception separately from IR");

	memcpy(malformed, image, sizeof(malformed));
	store_test64(malformed + 12, state.clock_hz);
	CHECK(rejected_machine_image_preserves(&state, malformed),
	      "invalid service residue does not mutate state");
	memcpy(malformed, image, sizeof(malformed));
	malformed[39] |= 0x80;
	CHECK(rejected_machine_image_preserves(&state, malformed),
	      "reserved control bit is rejected without mutation");
	memcpy(malformed, image, sizeof(malformed));
	malformed[6] |= 0x02;
	CHECK(rejected_machine_image_preserves(&state, malformed),
	      "busy flag/status mismatch is rejected without mutation");
	memcpy(malformed, image, sizeof(malformed));
	malformed[42] = 8;
	CHECK(rejected_machine_image_preserves(&state, malformed),
	      "invalid TOP is rejected without mutation");
	memcpy(malformed, image, sizeof(malformed));
	malformed[31] = 1;
	CHECK(rejected_machine_image_preserves(&state, malformed),
	      "noncanonical instruction address is rejected without mutation");
	return 0;
}

static int test_special_instructions(void) {
	UPD8087_STATE state;

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_THREE);
	CHECK(execute_register(&state, 0xd9, 0xf8) == UPD8087_EXECUTED,
	      "FPREM executes");
	CHECK(raw_equal(state.reg[state.top], (UPD8087_RAW80){
		UINT64_C(0x8000000000000000), 0x3fff}),
	      "FPREM computes exact 10 mod 3");
	CHECK((state.status & (UPD8087_STATUS_C0 | UPD8087_STATUS_C2)) == 0 &&
	      (state.status & (UPD8087_STATUS_C1 | UPD8087_STATUS_C3)) ==
	      (UPD8087_STATUS_C1 | UPD8087_STATUS_C3),
	      "FPREM reports quotient low bits");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_NAN);
	set_stack_value(&state, 1, TEST_THREE);
	state.tag[state.top] = UPD8087_TAG_SPECIAL;
	CHECK(execute_register(&state, 0xd9, 0xf8) == UPD8087_EXECUTED,
	      "FPREM dispatches a NaN numerator");
	CHECK(raw_equal(state.reg[state.top], TEST_NAN) &&
	      (state.status & UPD8087_STATUS_IE) != 0,
	      "masked FPREM preserves its sole NaN numerator");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TWO_POW_70);
	set_stack_value(&state, 1, TEST_THREE);
	CHECK(execute_register(&state, 0xd9, 0xf8) == UPD8087_EXECUTED,
	      "partial FPREM executes");
	CHECK((state.status & UPD8087_STATUS_C2) != 0 &&
	      raw_equal(state.reg[state.top], (UPD8087_RAW80){
		UINT64_C(0x8000000000000000), 0x4005}),
	      "partial FPREM reduces by an exact 2^64-sized step");
	CHECK(execute_register(&state, 0xd9, 0xf8) == UPD8087_EXECUTED,
	      "second partial FPREM executes");
	CHECK((state.status & UPD8087_STATUS_C2) == 0 &&
	      raw_equal(state.reg[state.top], TEST_ONE),
	      "repeated FPREM completes exact reduction");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TWO);
	set_stack_value(&state, 1, TEST_ONE_HALF);
	CHECK(execute_register(&state, 0xd9, 0xfd) == UPD8087_EXECUTED,
	      "FSCALE executes");
	CHECK(raw_equal(state.reg[state.top],
	               (UPD8087_RAW80){UINT64_C(0x8000000000000000), 0x4001}),
	      "FSCALE truncates a noninteger factor toward zero");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ONE);
	set_stack_value(&state, 1, TEST_NEG_32768);
	CHECK(execute_register(&state, 0xd9, 0xfd) == UPD8087_EXECUTED,
	      "FSCALE accepts negative 32768 boundary");
	CHECK((state.status & UPD8087_STATUS_IE) == 0,
	      "negative 32768 FSCALE is not invalid");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ONE);
	set_stack_value(&state, 1, TEST_NEG_32768_HALF);
	CHECK(execute_register(&state, 0xd9, 0xfd) == UPD8087_EXECUTED,
	      "FSCALE dispatches a fractional lower-bound factor");
	CHECK(raw_equal(state.reg[state.top], TEST_ONE) &&
	      (state.status & UPD8087_STATUS_IE) == 0,
	      "negative fractional value below -32768 is undefined without mutation");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ONE);
	set_stack_value(&state, 1, TEST_POS_32768);
	CHECK(execute_register(&state, 0xd9, 0xfd) == UPD8087_EXECUTED,
	      "FSCALE executes for positive out-of-range factor");
	CHECK(raw_equal(state.reg[state.top], TEST_ONE) &&
	      (state.status & UPD8087_STATUS_IE) == 0,
	      "positive 32768 FSCALE is undefined without mutation or exception");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TWO);
	set_stack_value(&state, 1, TEST_HALF);
	CHECK(execute_register(&state, 0xd9, 0xfd) == UPD8087_EXECUTED,
	      "FSCALE dispatches a fractional factor");
	CHECK(raw_equal(state.reg[state.top], TEST_TWO) &&
	      (state.status & UPD8087_STATUS_IE) == 0,
	      "a nonzero factor with magnitude below one is undefined without mutation");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_QUARTER);
	CHECK(execute_register(&state, 0xd9, 0xf2) == UPD8087_EXECUTED,
	      "FPTAN executes in the strict documented domain");
	CHECK(state.tag[state.top] != UPD8087_TAG_EMPTY &&
	      raw_equal(state.reg[state.top], TEST_ONE) &&
	      !raw_equal(state.reg[(state.top + 1) & 7], TEST_ZERO),
	      "FPTAN pushes one above the tangent result");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ZERO);
	CHECK(execute_register(&state, 0xd9, 0xf2) == UPD8087_EXECUTED,
	      "FPTAN dispatches outside its documented domain");
	CHECK(raw_equal(state.reg[state.top], TEST_ZERO) &&
	      (state.status & UPD8087_STATUS_IE) == 0,
	      "FPTAN rejects zero without mutation or exception");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_PI_OVER_FOUR);
	CHECK(execute_register(&state, 0xd9, 0xf2) == UPD8087_EXECUTED,
	      "FPTAN dispatches its upper endpoint");
	CHECK(raw_equal(state.reg[state.top], TEST_PI_OVER_FOUR) &&
	      (state.status & UPD8087_STATUS_IE) == 0,
	      "FPTAN rejects pi/4 endpoint without mutation or exception");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ONE);
	set_stack_value(&state, 1, TEST_HALF);
	CHECK(execute_register(&state, 0xd9, 0xf3) == UPD8087_EXECUTED,
	      "FPATAN executes in documented range");
	CHECK(state.top == 1 && state.tag[state.top] != UPD8087_TAG_EMPTY,
	      "FPATAN pops the two-operand stack pair");
	CHECK((state.reg[state.top].sign_exp & 0x8000) == 0 &&
	      (state.reg[state.top].sign_exp & 0x7fff) < 0x3fff,
	      "FPATAN returns the positive arctangent of Y divided by X");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_HALF);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_register(&state, 0xd9, 0xf3) == UPD8087_EXECUTED,
	      "FPATAN dispatches the reversed endpoint ordering");
	CHECK(state.top == 0 && raw_equal(state.reg[state.top], TEST_HALF) &&
	      raw_equal(state.reg[(state.top + 1) & 7], TEST_ONE) &&
	      (state.status & UPD8087_STATUS_IE) == 0,
	      "FPATAN rejects Y greater than X without mutation or exception");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ONE);
	set_stack_value(&state, 1, TEST_ZERO);
	CHECK(execute_register(&state, 0xd9, 0xf3) == UPD8087_EXECUTED,
	      "FPATAN dispatches its lower endpoint");
	CHECK(state.top == 0 && raw_equal(state.reg[state.top], TEST_ONE) &&
	      (state.status & UPD8087_STATUS_IE) == 0,
	      "FPATAN rejects zero Y without mutation or exception");

	reset_enabled(&state);
	CHECK(execute_register(&state, 0xd9, 0xc0) == UPD8087_EXECUTED,
	      "FLD ST(0) dispatches a masked empty-stack source");
	CHECK(state.top == 7 && state.tag[state.top] == UPD8087_TAG_SPECIAL &&
	      (state.status & (UPD8087_STATUS_IE | UPD8087_STATUS_SF)) ==
	      (UPD8087_STATUS_IE | UPD8087_STATUS_SF),
	      "masked FLD stack underflow pushes indefinite");

	reset_enabled(&state);
	upd8087_set_control(&state,
	                   (uint16_t)(UPD8087_DEFAULT_CONTROL &
                              (uint16_t)~UPD8087_STATUS_IE));
	CHECK(execute_register(&state, 0xd9, 0xc0) == UPD8087_EXECUTED,
	      "FLD ST(0) dispatches an unmasked empty-stack source");
	CHECK(state.top == 0 && state.tag[state.top] == UPD8087_TAG_EMPTY &&
	      (state.status & (UPD8087_STATUS_IE | UPD8087_STATUS_SF)) ==
	      (UPD8087_STATUS_IE | UPD8087_STATUS_SF),
	      "unmasked FLD stack underflow leaves the stack uncommitted");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	CHECK(execute_register(&state, 0xd9, 0xf4) == UPD8087_EXECUTED,
	      "FXTRACT executes");
	CHECK(raw_equal(state.reg[state.top],
	               (UPD8087_RAW80){UINT64_C(0xa000000000000000), 0x3fff}) &&
	      raw_equal(state.reg[(state.top + 1) & 7],
	                (UPD8087_RAW80){UINT64_C(0xc000000000000000), 0x4000}),
	      "FXTRACT produces significand and exponent");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_POS_INFINITY);
	state.tag[state.top] = UPD8087_TAG_SPECIAL;
	CHECK(execute_register(&state, 0xd9, 0xf4) == UPD8087_EXECUTED,
	      "FXTRACT infinity executes its invalid-operation path");
	CHECK((state.status & UPD8087_STATUS_IE) != 0 &&
	      raw_equal(state.reg[state.top],
                (UPD8087_RAW80){UINT64_C(0xc000000000000000), 0xffff}) &&
	      state.top == 0,
	      "FXTRACT infinity returns real indefinite without a push");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_POS_INFINITY);
	state.tag[state.top] = UPD8087_TAG_SPECIAL;
	state.control &= (uint16_t)~UPD8087_STATUS_IE;
	CHECK(execute_register(&state, 0xd9, 0xf4) == UPD8087_EXECUTED,
	      "unmasked FXTRACT infinity dispatches");
	CHECK((state.status & (UPD8087_STATUS_IE | UPD8087_STATUS_IR |
	                       UPD8087_STATUS_B)) ==
	      (UPD8087_STATUS_IE | UPD8087_STATUS_IR | UPD8087_STATUS_B) &&
	      upd8087_wait_blocked(&state),
	      "unmasked FXTRACT keeps BUSY and IR until cleared");
	upd8087_clear_exceptions(&state);
	CHECK(!upd8087_wait_blocked(&state) &&
	      (state.status & (UPD8087_STATUS_IE | UPD8087_STATUS_IR |
                        UPD8087_STATUS_B)) == 0,
	      "FCLEX releases the FXTRACT wait state");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ZERO);
	CHECK(execute_register(&state, 0xd9, 0xf0) == UPD8087_EXECUTED,
	      "F2XM1 executes at zero");
	CHECK(raw_equal(state.reg[state.top], TEST_ZERO),
	      "F2XM1 zero result is exact zero");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ONE);
	set_stack_value(&state, 1, TEST_TWO);
	CHECK(execute_register(&state, 0xd9, 0xf1) == UPD8087_EXECUTED,
	      "FYL2X executes");
	CHECK(state.top == 1 && raw_equal(state.reg[state.top], TEST_ZERO),
	      "FYL2X computes 2 times log2(1) as zero");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_QUARTER);
	set_stack_value(&state, 1, TEST_TWO);
	CHECK(execute_register(&state, 0xd9, 0xf9) == UPD8087_EXECUTED,
	      "FYL2XP1 executes in its strict documented domain");
	CHECK(state.top == 1 && state.tag[state.top] != UPD8087_TAG_EMPTY,
	      "FYL2XP1 pops the X operand");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ZERO);
	set_stack_value(&state, 1, TEST_TWO);
	CHECK(execute_register(&state, 0xd9, 0xf9) == UPD8087_EXECUTED,
	      "FYL2XP1 dispatches its lower endpoint");
	CHECK(state.top == 0 && raw_equal(state.reg[state.top], TEST_ZERO) &&
	      (state.status & UPD8087_STATUS_IE) == 0,
	      "FYL2XP1 rejects zero X without mutation or exception");
	return 0;
}

static int test_infinity_control(void) {
	UPD8087_STATE state;

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_POS_INFINITY);
	set_stack_value(&state, 1, TEST_NEG_INFINITY);
	state.tag[state.top] = UPD8087_TAG_SPECIAL;
	state.tag[(state.top + 1) & 7] = UPD8087_TAG_SPECIAL;
	CHECK(execute_register(&state, 0xd8, 0xd1) == UPD8087_EXECUTED,
	      "projective infinity comparison executes");
	CHECK((state.status & (UPD8087_STATUS_C0 | UPD8087_STATUS_C3)) ==
	      UPD8087_STATUS_C3,
	      "projective infinities compare equal");
	CHECK((state.status & UPD8087_STATUS_IE) == 0,
	      "projective infinity equality is not invalid");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_POS_INFINITY);
	set_stack_value(&state, 1, TEST_ONE);
	state.tag[state.top] = UPD8087_TAG_SPECIAL;
	CHECK(execute_register(&state, 0xd8, 0xd1) == UPD8087_EXECUTED,
	      "projective infinity versus finite comparison executes");
	CHECK((state.status & (UPD8087_STATUS_C0 | UPD8087_STATUS_C2 |
	                       UPD8087_STATUS_C3)) ==
	      (UPD8087_STATUS_C0 | UPD8087_STATUS_C2 | UPD8087_STATUS_C3) &&
	      (state.status & UPD8087_STATUS_IE) != 0,
	      "projective infinity versus finite is unordered and invalid");

	reset_enabled(&state);
	state.control |= 0x1000;
	set_stack_value(&state, 0, TEST_POS_INFINITY);
	set_stack_value(&state, 1, TEST_NEG_INFINITY);
	state.tag[state.top] = UPD8087_TAG_SPECIAL;
	state.tag[(state.top + 1) & 7] = UPD8087_TAG_SPECIAL;
	CHECK(execute_register(&state, 0xd8, 0xd1) == UPD8087_EXECUTED,
	      "affine infinity comparison executes");
	CHECK((state.status & (UPD8087_STATUS_C0 | UPD8087_STATUS_C3)) == 0,
	      "affine positive infinity orders above negative infinity");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_POS_INFINITY);
	set_stack_value(&state, 1, TEST_NEG_INFINITY);
	state.tag[state.top] = UPD8087_TAG_SPECIAL;
	state.tag[(state.top + 1) & 7] = UPD8087_TAG_SPECIAL;
	CHECK(execute_register(&state, 0xd8, 0xc1) == UPD8087_EXECUTED,
	      "projective infinity addition executes");
	CHECK((state.status & UPD8087_STATUS_IE) != 0 &&
	      raw_equal(state.reg[state.top],
	                (UPD8087_RAW80){UINT64_C(0xc000000000000000), 0xffff}),
	      "projective infinity addition returns real indefinite");

	reset_enabled(&state);
	state.control |= 0x1000;
	set_stack_value(&state, 0, TEST_POS_INFINITY);
	set_stack_value(&state, 1, TEST_NEG_INFINITY);
	state.tag[state.top] = UPD8087_TAG_SPECIAL;
	state.tag[(state.top + 1) & 7] = UPD8087_TAG_SPECIAL;
	CHECK(execute_register(&state, 0xd8, 0xc1) == UPD8087_EXECUTED,
	      "affine infinity addition executes");
	CHECK((state.status & UPD8087_STATUS_IE) != 0,
	      "affine opposite infinity addition is invalid");
	return 0;
}

static int test_bcd_and_integer_transfers(void) {
	UPD8087_STATE state;
	TEST_MEMORY memory;

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 0x23;
	memory.bytes[0x21] = 0x01;
	reset_enabled(&state);
	CHECK(execute_memory(&state, &memory, 0xdf, 0x20) == UPD8087_EXECUTED,
	      "FBLD executes");
	CHECK(execute_memory(&state, &memory, 0xdf, 0x30) == UPD8087_EXECUTED,
	      "FBSTP executes");
	CHECK(memory.bytes[0x20] == 0x23 && memory.bytes[0x21] == 0x01 &&
	      state.tag[state.top] == UPD8087_TAG_EMPTY,
	      "packed BCD round-trip preserves 123 and pops");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 0xfa;
	reset_enabled(&state);
	CHECK(execute_memory(&state, &memory, 0xdf, 0x20) == UPD8087_EXECUTED,
	      "invalid FBLD dispatches");
	CHECK((state.status & UPD8087_STATUS_IE) == 0 &&
	      state.tag[state.top] != UPD8087_TAG_EMPTY,
	      "invalid packed BCD remains an unchecked undefined load");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 0x85;
	memory.bytes[0x21] = 0xff;
	reset_enabled(&state);
	CHECK(execute_memory(&state, &memory, 0xdf, 0x00) == UPD8087_EXECUTED,
	      "FILD m16 executes");
	CHECK(execute_memory(&state, &memory, 0xdf, 0x18) == UPD8087_EXECUTED,
	      "FISTP m16 executes");
	CHECK(memory.bytes[0x20] == 0x85 && memory.bytes[0x21] == 0xff,
	      "signed 16-bit integer transfer round-trips");

	memset(&memory, 0, sizeof(memory));
	store_test64(memory.bytes + 0x20, UINT64_C(0x8000000000000000));
	reset_enabled(&state);
	CHECK(execute_memory(&state, &memory, 0xdf, 0x28) == UPD8087_EXECUTED,
	      "FILD m64int accepts INT64_MIN");
	CHECK(execute_memory(&state, &memory, 0xdf, 0x38) == UPD8087_EXECUTED,
	      "FISTP m64int stores INT64_MIN");
	CHECK(load_test64(memory.bytes + 0x20) == UINT64_C(0x8000000000000000),
	      "signed 64-bit integer minimum round-trips");
	return 0;
}

static int test_denormal_memory_loads(void) {
	UPD8087_STATE state;
	TEST_MEMORY memory;

	memset(&memory, 0, sizeof(memory));
	store_test32(memory.bytes + 0x20, UINT32_C(1));
	reset_enabled(&state);
	CHECK(execute_memory(&state, &memory, 0xd9, 0x00) == UPD8087_EXECUTED,
	      "FLD short-real denormal executes");
	CHECK((state.status & UPD8087_STATUS_DE) != 0 &&
	      state.tag[state.top] == UPD8087_TAG_VALID &&
	      raw_equal(state.reg[state.top], (UPD8087_RAW80){
		UINT64_C(0x0000010000000000), 0x3f81}),
	      "FLD short-real denormal preserves the equivalent unnormal");

	reset_enabled(&state);
	upd8087_set_control(&state,
	                   (uint16_t)(state.control & (uint16_t)~UPD8087_STATUS_DE));
	CHECK(execute_memory(&state, &memory, 0xd9, 0x00) == UPD8087_EXECUTED,
	      "unmasked FLD short-real denormal executes its trap path");
	CHECK(state.top == 0 && state.tag[state.top] == UPD8087_TAG_EMPTY &&
	      (state.status & (UPD8087_STATUS_DE | UPD8087_STATUS_IR)) ==
	      (UPD8087_STATUS_DE | UPD8087_STATUS_IR),
	      "unmasked FLD denormal leaves the stack uncommitted");

	memset(&memory, 0, sizeof(memory));
	store_test64(memory.bytes + 0x20, UINT64_C(1));
	reset_enabled(&state);
	CHECK(execute_memory(&state, &memory, 0xdd, 0x00) == UPD8087_EXECUTED,
	      "FLD long-real denormal executes");
	CHECK((state.status & UPD8087_STATUS_DE) != 0 &&
	      state.tag[state.top] == UPD8087_TAG_VALID &&
	      raw_equal(state.reg[state.top], (UPD8087_RAW80){
		UINT64_C(0x0000000000000800), 0x3c01}),
	      "FLD long-real denormal preserves the equivalent unnormal");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_UNNORMAL_LARGE);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_register(&state, 0xd8, 0xc1) == UPD8087_EXECUTED,
	      "unnormal-dominant FADD executes");
	CHECK(state.tag[state.top] == UPD8087_TAG_VALID &&
	      (state.reg[state.top].signif & UINT64_C(0x8000000000000000)) == 0,
	      "unnormal-dominant addition keeps an unnormal result");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_UNNORMAL_SMALL);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_register(&state, 0xd8, 0xc1) == UPD8087_EXECUTED,
	      "normal-dominant FADD with an unnormal executes");
	CHECK(state.tag[state.top] == UPD8087_TAG_VALID &&
	      (state.reg[state.top].signif & UINT64_C(0x8000000000000000)) != 0,
	      "normal-dominant addition returns a normal result");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_UNNORMAL_LARGE);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_register(&state, 0xd8, 0xc9) == UPD8087_EXECUTED,
	      "unnormal FMUL executes");
	CHECK((state.reg[state.top].signif & UINT64_C(0x8000000000000000)) == 0,
	      "FMUL preserves an unnormal result class");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_UNNORMAL_LARGE);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_register(&state, 0xd8, 0xf1) == UPD8087_EXECUTED,
	      "unnormal dividend FDIV executes");
	CHECK((state.reg[state.top].signif & UINT64_C(0x8000000000000000)) == 0,
	      "unnormal dividend FDIV preserves an unnormal result class");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_ONE);
	set_stack_value(&state, 1, TEST_UNNORMAL_SMALL);
	CHECK(execute_register(&state, 0xd8, 0xf1) == UPD8087_EXECUTED,
	      "unnormal divisor FDIV executes its invalid path");
	CHECK((state.status & UPD8087_STATUS_IE) != 0 &&
	      state.tag[state.top] == UPD8087_TAG_SPECIAL,
	      "unnormal divisor FDIV returns real indefinite");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_UNNORMAL_SMALL);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_register(&state, 0xd8, 0xd1) == UPD8087_EXECUTED,
	      "unnormal FCOM executes");
	CHECK((state.status & UPD8087_STATUS_DE) != 0 &&
	      (state.status & (UPD8087_STATUS_C0 | UPD8087_STATUS_C2 |
                       UPD8087_STATUS_C3)) == UPD8087_STATUS_C0,
	      "unnormal FCOM raises denormal and compares normalized values");

	memset(&memory, 0, sizeof(memory));
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_UNNORMAL_SHORT_UNDERFLOW);
	CHECK(execute_memory(&state, &memory, 0xd9, 0x10) == UPD8087_EXECUTED,
	      "short-real store of an in-range unnormal executes");
	CHECK((state.status & UPD8087_STATUS_UE) != 0 && memory.writes == 4,
	      "short-real unnormal store reports masked underflow and writes");

	memset(&memory, 0, sizeof(memory));
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_UNNORMAL_SHORT_INVALID);
	CHECK(execute_memory(&state, &memory, 0xd9, 0x10) == UPD8087_EXECUTED,
	      "short-real store of an above-boundary unnormal executes");
	CHECK((state.status & UPD8087_STATUS_IE) != 0 && memory.writes == 4,
	      "short-real above-boundary unnormal stores an invalid indefinite");

	memset(&memory, 0, sizeof(memory));
	store_test32(memory.bytes + 0x20, UINT32_C(1));
	reset_enabled(&state);
	state.status |= UPD8087_STATUS_C3;
	upd8087_set_control(&state,
	                   (uint16_t)(state.control & (uint16_t)~UPD8087_STATUS_DE));
	set_stack_value(&state, 0, TEST_ONE);
	CHECK(execute_memory(&state, &memory, 0xd8, 0x10) == UPD8087_EXECUTED,
	      "unmasked denormal compare executes its trap path");
	CHECK((state.status & (UPD8087_STATUS_DE | UPD8087_STATUS_IR)) ==
	      (UPD8087_STATUS_DE | UPD8087_STATUS_IR) &&
	      (state.status & UPD8087_STATUS_C3) != 0,
	      "unmasked denormal compare leaves condition codes uncommitted");
	return 0;
}

static int test_integer_memory_direction(void) {
	UPD8087_STATE state;
	TEST_MEMORY memory;

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 3;
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	CHECK(execute_memory(&state, &memory, 0xda, 0x20) == UPD8087_EXECUTED,
	      "DA FISUB m32int executes");
	CHECK(raw_equal(state.reg[state.top], TEST_SEVEN),
	      "DA FISUB computes ST minus the integer operand");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 3;
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	CHECK(execute_memory(&state, &memory, 0xda, 0x28) == UPD8087_EXECUTED,
	      "DA FISUBR m32int executes");
	CHECK(raw_equal(state.reg[state.top], TEST_NEG_SEVEN),
	      "DA FISUBR computes the integer operand minus ST");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 3;
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_memory(&state, &memory, 0xda, 0x30) == UPD8087_EXECUTED,
	      "DA FIDIV m32int executes");
	CHECK(execute_register(&state, 0xd8, 0xd1) == UPD8087_EXECUTED &&
	      (state.status & (UPD8087_STATUS_C0 | UPD8087_STATUS_C3)) == 0,
	      "DA FIDIV computes ST divided by the integer operand");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 3;
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_memory(&state, &memory, 0xda, 0x38) == UPD8087_EXECUTED,
	      "DA FIDIVR m32int executes");
	CHECK(execute_register(&state, 0xd8, 0xd1) == UPD8087_EXECUTED &&
	      (state.status & UPD8087_STATUS_C0) != 0,
	      "DA FIDIVR computes the integer operand divided by ST");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 3;
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	CHECK(execute_memory(&state, &memory, 0xde, 0x20) == UPD8087_EXECUTED,
	      "DE FISUBR m16int executes");
	CHECK(raw_equal(state.reg[state.top], TEST_NEG_SEVEN),
	      "DE FISUBR computes the integer operand minus ST");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 3;
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	CHECK(execute_memory(&state, &memory, 0xde, 0x28) == UPD8087_EXECUTED,
	      "DE FISUB m16int executes");
	CHECK(raw_equal(state.reg[state.top], TEST_SEVEN),
	      "DE FISUB computes ST minus the integer operand");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 2;
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_memory(&state, &memory, 0xde, 0x30) == UPD8087_EXECUTED,
	      "DE FIDIVR m16int executes");
	CHECK(execute_register(&state, 0xd8, 0xd1) == UPD8087_EXECUTED &&
	      (state.status & UPD8087_STATUS_C0) != 0,
	      "DE FIDIVR computes the integer operand divided by ST");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 2;
	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_ONE);
	CHECK(execute_memory(&state, &memory, 0xde, 0x38) == UPD8087_EXECUTED,
	      "DE FIDIV m16int executes");
	CHECK(execute_register(&state, 0xd8, 0xd1) == UPD8087_EXECUTED &&
	      (state.status & UPD8087_STATUS_C0) == 0 &&
	      (state.status & UPD8087_STATUS_C3) == 0,
	      "DE FIDIV computes ST divided by the integer operand");
	return 0;
}

static int test_register_arithmetic_directions(void) {
	UPD8087_STATE state;

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TWO);
	set_stack_value(&state, 1, TEST_TEN);
	CHECK(execute_register(&state, 0xdc, 0xf9) == UPD8087_EXECUTED,
	      "DC F9 FDIV ST1,ST0 executes");
	CHECK(raw_equal(state.reg[(state.top + 1) & 7], TEST_FIVE),
	      "DC F9 computes ST1 as ST1 divided by ST0");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_TWO);
	CHECK(execute_register(&state, 0xdc, 0xf1) == UPD8087_EXECUTED,
	      "DC F1 FDIVR ST1,ST0 executes");
	CHECK(raw_equal(state.reg[(state.top + 1) & 7], TEST_FIVE),
	      "DC F1 computes ST1 as ST0 divided by ST1");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TWO);
	set_stack_value(&state, 1, TEST_TEN);
	CHECK(execute_register(&state, 0xde, 0xf9) == UPD8087_EXECUTED,
	      "DE F9 FDIVP ST1,ST0 executes");
	CHECK(raw_equal(state.reg[state.top], TEST_FIVE),
	      "DE F9 computes ST1 as ST1 divided by ST0 before popping");

	reset_enabled(&state);
	set_stack_value(&state, 0, TEST_TEN);
	set_stack_value(&state, 1, TEST_TWO);
	CHECK(execute_register(&state, 0xde, 0xe1) == UPD8087_EXECUTED,
	      "DE E1 FSUBRP ST1,ST0 executes");
	CHECK(raw_equal(state.reg[state.top], TEST_EIGHT),
	      "DE E1 computes ST1 as ST0 minus ST1 before popping");
	return 0;
}

static int test_exception_wait_and_bus_seam(void) {
	UPD8087_STATE state;
	UPD8087_STATE compare_state;
	TEST_MEMORY memory;
	UPD8087_OPERAND operand;

	reset_enabled(&state);
	CHECK(execute_register(&state, 0xd8, 0xc0) == UPD8087_EXECUTED,
	      "empty-stack instruction dispatches as an executed instruction");
	CHECK((state.status & (UPD8087_STATUS_IE | UPD8087_STATUS_SF)) ==
	      (UPD8087_STATUS_IE | UPD8087_STATUS_SF) &&
	      !upd8087_interrupt_pending(&state) && !upd8087_wait_blocked(&state),
	      "masked stack fault is sticky but does not block WAIT");

	reset_enabled(&compare_state);
	CHECK(execute_register(&compare_state, 0xd8, 0xd0) == UPD8087_EXECUTED,
	      "empty-stack FCOM dispatches its masked fault path");
	CHECK((compare_state.status & (UPD8087_STATUS_C0 | UPD8087_STATUS_C2 |
	                       UPD8087_STATUS_C3 | UPD8087_STATUS_C1)) ==
	      (UPD8087_STATUS_C0 | UPD8087_STATUS_C2 |
	       UPD8087_STATUS_C3 | UPD8087_STATUS_C1),
	      "masked compare keeps C1 for a stack underflow");

	upd8087_set_control(&state,
	                   (uint16_t)(UPD8087_DEFAULT_CONTROL &
                              (uint16_t)~UPD8087_STATUS_IE));
	CHECK(upd8087_interrupt_pending(&state) &&
	      (state.status & UPD8087_STATUS_IR) != 0 &&
	      upd8087_wait_blocked(&state),
	      "unmasking a sticky exception asserts the NDP request");
	CHECK(execute_register(&state, 0xdb, 0xe1) == UPD8087_EXECUTED &&
	      !upd8087_interrupt_pending(&state) && upd8087_wait_blocked(&state),
	      "FDISI withdraws the external request but retains BUSY/IR");
	CHECK(execute_register(&state, 0xdb, 0xe0) == UPD8087_EXECUTED &&
	      upd8087_interrupt_pending(&state),
	      "FENI reasserts a pending unmasked exception");
	upd8087_acknowledge_interrupt(&state);
	CHECK(!upd8087_interrupt_pending(&state) && upd8087_wait_blocked(&state) &&
	      (upd8087_status(&state) &
	       (UPD8087_STATUS_IR | UPD8087_STATUS_B)) ==
	      (UPD8087_STATUS_IR | UPD8087_STATUS_B),
	      "interrupt acknowledgement leaves the 8087 exception active");
	CHECK(execute_register(&state, 0xdb, 0xe2) == UPD8087_EXECUTED,
	      "FCLEX executes");
	CHECK((state.status & (UPD8087_STATUS_IE | UPD8087_STATUS_SF |
	                       UPD8087_STATUS_IR)) == 0,
	      "FCLEX clears exception summary and stack-fault state");
	CHECK(execute_register(&state, 0xdb, 0xe1) == UPD8087_EXECUTED &&
	      (state.control & 0x0080) != 0,
	      "FDISI sets the original interrupt-enable mask");
	CHECK(execute_register(&state, 0xdb, 0xe0) == UPD8087_EXECUTED &&
	      (state.control & 0x0080) == 0,
	      "FENI clears the original interrupt-enable mask");

	memset(&memory, 0, sizeof(memory));
	reset_enabled(&state);
	memset(&operand, 0, sizeof(operand));
	operand.first_word = 0x0b7f;
	operand.first_word_valid = true;
	CHECK(upd8087_execute(&state, 0xd9, 0x28, &operand) == UPD8087_EXECUTED,
	      "two-byte memory form can consume the CPU first-word latch");
	CHECK(state.control == 0x0b7f,
	      "FLDCW consumes the little-endian first-word latch");
	memset(&operand, 0, sizeof(operand));
	operand.first_word_valid = true;
	CHECK(upd8087_execute(&state, 0xd9, 0x00, &operand) ==
	      UPD8087_EXEC_BUS_ERROR,
	      "multi-byte memory form requires the device bus after first word");
	return 0;
}

static int test_instruction_slice(void) {
	UPD8087_STATE state;
	TEST_MEMORY memory;

	memset(&memory, 0, sizeof(memory));
	reset_enabled(&state);
	CHECK(execute_register(&state, 0xd9, 0xe8) == UPD8087_EXECUTED,
	      "FLD1 executes");
	CHECK(execute_register(&state, 0xd9, 0xe8) == UPD8087_EXECUTED,
	      "second FLD1 executes");
	CHECK(execute_register(&state, 0xd8, 0xc1) == UPD8087_EXECUTED,
	      "register FADD executes");
	CHECK(state.reg[state.top].signif == UINT64_C(0x8000000000000000) &&
	      state.reg[state.top].sign_exp == 0x4000,
	      "register FADD produces exact two");

	memset(&memory, 0, sizeof(memory));
	memory.bytes[0x20] = 0;
	memory.bytes[0x21] = 0;
	memory.bytes[0x22] = 0x80;
	memory.bytes[0x23] = 0x3f;
	CHECK(execute_memory(&state, &memory, 0xd9, 0x00) == UPD8087_EXECUTED,
	      "memory FLD executes");
	CHECK(memory.reads == 4, "memory FLD reads one operand exactly");
	CHECK(execute_memory(&state, &memory, 0xd9, 0x10) == UPD8087_EXECUTED,
	      "memory FST executes");
	CHECK(memory.writes == 4, "memory FST writes one operand exactly");
	CHECK(memory.bytes[0x22] == 0x80 && memory.bytes[0x23] == 0x3f,
	      "memory FST writes IEEE single value");

	CHECK(execute_register(&state, 0xd9, 0xd0) == UPD8087_EXECUTED,
	      "FNOP executes");
	CHECK(execute_register(&state, 0xdb, 0xe1) == UPD8087_EXECUTED,
	      "FDISI executes");
	CHECK(execute_register(&state, 0xdb, 0xe0) == UPD8087_EXECUTED,
	      "FENI executes");
	CHECK(upd8087_classify(0x66, 0xc0) == UPD8087_SLOT_UNDEFINED,
	      "FPO2 is not an 8087 slot");
	return 0;
}

static int test_decoder_inventory(void) {
	unsigned opcode;
	unsigned modrm;
	unsigned defined = 0;
	unsigned executed = 0;
	UPD8087_STATE state;
	TEST_MEMORY memory;

	/* This is an independent transcription of the normative inventory.  It
	 * must not call the production classifier or derive its sets from it. */
	#define MEMORY_FORM(op, r) \
		(((op) == 0xd8) || ((op) == 0xda) || ((op) == 0xdc) || \
		 ((op) == 0xde) || \
		 (((op) == 0xd9) && ((r) != 1)) || \
		 (((op) == 0xdb) && ((r) == 0 || (r) == 2 || (r) == 3 || \
		                    (r) == 5 || (r) == 7)) || \
		 (((op) == 0xdd) && ((r) == 0 || (r) == 2 || (r) == 3 || \
		                    (r) == 4 || (r) == 6 || (r) == 7)) || \
		 (((op) == 0xdf) && ((r) != 1)))
	#define REGISTER_FORM(op, r, m) \
		(((op) == 0xd8) || \
		 (((op) == 0xd9) && \
		  (((r) == 0) || ((r) == 1) || ((r) == 2 && (m) == 0) || \
		   ((r) == 4 && ((m) == 0 || (m) == 1 || (m) == 4 || (m) == 5)) || \
		   ((r) == 5 && (m) <= 6) || \
		   ((r) == 6 && ((m) <= 4 || (m) >= 6)) || \
		   ((r) == 7 && ((m) == 0 || (m) == 1 || (m) == 2 || \
		                (m) == 4 || (m) == 5)))) || \
		 (((op) == 0xdb) && (r) == 4 && (m) <= 3) || \
		 (((op) == 0xdc) && ((r) == 0 || (r) == 1 || (r) == 4 || \
		                    (r) == 5 || (r) == 6 || (r) == 7)) || \
		 (((op) == 0xdd) && ((r) == 0 || (r) == 2 || (r) == 3)) || \
		 (((op) == 0xde) && ((r) == 0 || (r) == 1 || (r) == 4 || \
		                    (r) == 5 || (r) == 6 || (r) == 7 || \
		                    ((r) == 3 && (m) == 1))))

	for (opcode = 0xd8; opcode <= 0xdf; opcode++) {
		for (modrm = 0; modrm < 256; modrm++) {
			unsigned reg = (modrm >> 3) & 7;
			unsigned rm = modrm & 7;
			bool expected = ((modrm & 0xc0) != 0xc0) ?
				MEMORY_FORM(opcode, reg) : REGISTER_FORM(opcode, reg, rm);
			bool actual = upd8087_classify((uint8_t)opcode, (uint8_t)modrm) !=
				UPD8087_SLOT_UNDEFINED;

			CHECK(expected == actual, "production decoder differs from independent inventory");
			if (actual) {
				defined++;
				CHECK(upd8087_instruction_cycles((uint8_t)opcode,
					(uint8_t)modrm) != 0,
					"every documented FPO1 slot has a nominal timing entry");
				reset_enabled(&state);
				memset(&memory, 0, sizeof(memory));
				CHECK(((modrm & 0xc0) != 0xc0) ?
					execute_memory(&state, &memory, (uint8_t)opcode,
						(uint8_t)modrm) == UPD8087_EXECUTED :
					execute_register(&state, (uint8_t)opcode,
						(uint8_t)modrm) == UPD8087_EXECUTED,
					"every documented FPO1 slot dispatches");
				executed++;
			}
		}
	}
	#undef MEMORY_FORM
	#undef REGISTER_FORM
	CHECK(defined == 1597, "documented FPO1 inventory count is 1597");
	CHECK(executed == defined, "all documented FPO1 slots were exercised");
	CHECK(upd8087_classify(0xd9, 0xf5) == UPD8087_SLOT_UNDEFINED,
	      "reserved D9 F5 remains undefined");
	CHECK(upd8087_classify(0xdf, 0xc0) == UPD8087_SLOT_UNDEFINED,
	      "DF register encoding remains undefined");
	CHECK(upd8087_classify(0x66, 0xc0) == UPD8087_SLOT_UNDEFINED,
	      "FPO2 is not an 8087 slot");
	CHECK(upd8087_instruction_cycles(0xd9, 0xd0) != 0,
	      "FNOP has a timing entry");
	CHECK(upd8087_instruction_cycles(0xd9, 0xf0) != 0,
	      "transcendental instruction has a timing entry");
	return 0;
}

static int test_softfloat_context(void) {
	UPD8087_STATE state;
	uint_fast8_t rounding_mode;
	uint_fast8_t detect_tininess;
	uint_fast8_t exception_flags;
	uint_fast8_t rounding_precision;

	reset_enabled(&state);
	CHECK(execute_register(&state, 0xd9, 0xe8) == UPD8087_EXECUTED,
	      "context test FLD1 executes");
	CHECK(execute_register(&state, 0xd9, 0xe8) == UPD8087_EXECUTED,
	      "context test second FLD1 executes");
	rounding_mode = softfloat_round_max;
	detect_tininess = softfloat_tininess_beforeRounding;
	exception_flags = softfloat_flag_inexact;
	rounding_precision = 53;
	softfloat_roundingMode = rounding_mode;
	softfloat_detectTininess = detect_tininess;
	softfloat_exceptionFlags = exception_flags;
	extF80_roundingPrecision = rounding_precision;
	CHECK(execute_register(&state, 0xd8, 0xc1) == UPD8087_EXECUTED,
	      "context test FADD executes");
	CHECK(softfloat_roundingMode == rounding_mode &&
	      softfloat_detectTininess == detect_tininess &&
	      softfloat_exceptionFlags == exception_flags &&
	      extF80_roundingPrecision == rounding_precision,
	      "SoftFloat mutable context is restored after execution");
	return 0;
}

int main(void) {
	if (test_config_and_timing() || test_debug_view() || test_environment_codecs() ||
	    test_environment_memory_operations() ||
	    test_machine_state_codec() || test_instruction_slice() ||
	    test_special_instructions() || test_bcd_and_integer_transfers() ||
	    test_denormal_memory_loads() ||
	    test_integer_memory_direction() ||
	    test_register_arithmetic_directions() ||
	    test_infinity_control() ||
	    test_exception_wait_and_bus_seam() || test_decoder_inventory() ||
	    test_softfloat_context()) {
		return 1;
	}
	fprintf(stdout, "8087 core tests: PASS\n");
	return 0;
}
