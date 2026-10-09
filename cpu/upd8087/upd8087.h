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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_UPD8087_H
#define VAEG_UPD8087_H

#include <stdbool.h>
#include <stdint.h>

enum {
	UPD8087_DEFAULT_CLOCK_HZ = 8000000U,
	UPD8087_MIN_CLOCK_HZ = 1000000U,
	UPD8087_MAX_CLOCK_HZ = 20000000U,
	UPD8087_STATE_IMAGE_VERSION = 1,
	UPD8087_STATE_IMAGE_SIZE = 132,
	UPD8087_DEFAULT_CONTROL = 0x037f,
	UPD8087_STATUS_IE = 0x0001,
	UPD8087_STATUS_DE = 0x0002,
	UPD8087_STATUS_ZE = 0x0004,
	UPD8087_STATUS_OE = 0x0008,
	UPD8087_STATUS_UE = 0x0010,
	UPD8087_STATUS_PE = 0x0020,
	UPD8087_STATUS_SF = 0x0040,
	UPD8087_STATUS_IR = 0x0080,
	UPD8087_STATUS_C0 = 0x0100,
	UPD8087_STATUS_C1 = 0x0200,
	UPD8087_STATUS_C2 = 0x0400,
	UPD8087_STATUS_TOP = 0x3800,
	UPD8087_STATUS_C3 = 0x4000,
	UPD8087_STATUS_B = 0x8000,
};

typedef struct {
	uint64_t signif;
	uint16_t sign_exp;
} UPD8087_RAW80;

enum {
	UPD8087_TAG_VALID = 0,
	UPD8087_TAG_ZERO = 1,
	UPD8087_TAG_SPECIAL = 2,
	UPD8087_TAG_EMPTY = 3,
};

typedef enum {
	UPD8087_SLOT_UNDEFINED = 0,
	UPD8087_SLOT_MEMORY_ARITH,
	UPD8087_SLOT_REGISTER_ARITH,
	UPD8087_SLOT_MEMORY_TRANSFER,
	UPD8087_SLOT_REGISTER_TRANSFER,
	UPD8087_SLOT_CONTROL,
	UPD8087_SLOT_TRANSCENDENTAL,
	UPD8087_SLOT_INTEGER,
	UPD8087_SLOT_PACKED_BCD,
} UPD8087_SLOT_CLASS;

typedef enum {
	UPD8087_EXEC_ABSENT = 0,
	UPD8087_EXECUTED = 1,
	UPD8087_EXEC_UNDEFINED = 2,
	UPD8087_EXEC_BUS_ERROR = 3,
} UPD8087_EXEC_RESULT;

typedef uint8_t (*UPD8087_READ8)(void *opaque, uint32_t address);
typedef void (*UPD8087_WRITE8)(void *opaque, uint32_t address, uint8_t value);

typedef struct {
	void *opaque;
	UPD8087_READ8 read8;
	UPD8087_WRITE8 write8;
} UPD8087_BUS;

typedef struct {
	UPD8087_BUS bus;
	uint32_t address;
	uint16_t first_word;
	bool first_word_valid;
} UPD8087_OPERAND;

typedef struct {
	bool enabled;
	uint32_t clock_hz;
} UPD8087_CONFIG;

typedef struct {
	bool enabled;
	uint32_t clock_hz;
	UPD8087_RAW80 reg[8];
	uint8_t tag[8];
	uint8_t top;
	uint16_t control;
	uint16_t status;
	uint32_t instruction_address;
	uint32_t data_address;
	uint16_t opcode;
	uint64_t service_remainder;
	uint32_t last_ndp_cycles;
	uint32_t last_cpu_ticks;
	bool busy;
	bool pending_interrupt;
	uint8_t working_flags;
} UPD8087_STATE;

/* Stable copy-out surface for a read-only debugger or diagnostics panel. */
typedef struct {
	bool enabled;
	uint32_t clock_hz;
	UPD8087_RAW80 reg[8];
	uint8_t tag[8];
	uint8_t top;
	uint16_t control;
	uint16_t status;
	uint16_t tag_word;
	uint32_t instruction_address;
	uint32_t data_address;
	uint16_t opcode;
	uint64_t service_remainder;
	uint32_t last_ndp_cycles;
	uint32_t last_cpu_ticks;
	bool busy;
	bool pending_interrupt;
} UPD8087_DEBUG_VIEW;

#ifdef __cplusplus
extern "C" {
#endif

void upd8087_config_default(UPD8087_CONFIG *config);
bool upd8087_clock_valid(uint32_t clock_hz);
void upd8087_initialize(UPD8087_STATE *state, const UPD8087_CONFIG *config);
void upd8087_reset(UPD8087_STATE *state);
void upd8087_apply_config(UPD8087_STATE *state, const UPD8087_CONFIG *config);

UPD8087_SLOT_CLASS upd8087_classify(uint8_t opcode, uint8_t modrm);
uint32_t upd8087_instruction_cycles(uint8_t opcode, uint8_t modrm);
UPD8087_EXEC_RESULT upd8087_execute(UPD8087_STATE *state, uint8_t opcode,
	uint8_t modrm, const UPD8087_OPERAND *operand);

/* Convert NDP service cycles to the host scheduler clock with a carried
 * integer residue.  The result is never computed with host floating point. */
uint32_t upd8087_service_ticks(UPD8087_STATE *state, uint32_t ndp_cycles,
	uint32_t cpu_clock_hz);

uint16_t upd8087_status(const UPD8087_STATE *state);
uint16_t upd8087_tag_word(const UPD8087_STATE *state);
uint16_t upd8087_control(const UPD8087_STATE *state);
void upd8087_debug_view(const UPD8087_STATE *state, UPD8087_DEBUG_VIEW *view);
void upd8087_set_control(UPD8087_STATE *state, uint16_t control);
void upd8087_clear_exceptions(UPD8087_STATE *state);
bool upd8087_busy(const UPD8087_STATE *state);
bool upd8087_interrupt_pending(const UPD8087_STATE *state);
bool upd8087_wait_blocked(const UPD8087_STATE *state);
void upd8087_acknowledge_interrupt(UPD8087_STATE *state);

/* Exact little-endian environment and FSAVE codecs. */
void upd8087_store_environment(const UPD8087_STATE *state, uint8_t image[14]);
void upd8087_load_environment(UPD8087_STATE *state, const uint8_t image[14]);
void upd8087_store_state(const UPD8087_STATE *state, uint8_t image[94]);
void upd8087_load_state(UPD8087_STATE *state, const uint8_t image[94]);

/* Explicit fixed-endian VAEG savestate payload; this is not a C struct dump. */
bool upd8087_store_machine_state(const UPD8087_STATE *state, uint8_t *image,
	uint32_t image_size);
bool upd8087_load_machine_state(UPD8087_STATE *state, const uint8_t *image,
	uint32_t image_size);

#ifdef __cplusplus
}
#endif

#endif
