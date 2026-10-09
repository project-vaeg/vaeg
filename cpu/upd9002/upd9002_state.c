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
#include "upd9002_state.h"

#include <stddef.h>
#include <string.h>

#define STATE_ALIGNOF(type) offsetof(struct { char byte; type value; }, value)
#define STATE_ASSERT(name, condition) \
	typedef char state_assert_##name[(condition) ? 1 : -1]

STATE_ASSERT(compat_size,
	sizeof(Upd9002StateImage) == UPD9002_STATE_PAYLOAD_SIZE);
STATE_ASSERT(compat_align, STATE_ALIGNOF(Upd9002StateImage) == 4);
STATE_ASSERT(compat_r, offsetof(Upd9002StateImage, r) == 0);
STATE_ASSERT(compat_es_base, offsetof(Upd9002StateImage, es_base) == 28);
STATE_ASSERT(compat_cs_base, offsetof(Upd9002StateImage, cs_base) == 32);
STATE_ASSERT(compat_ss_base, offsetof(Upd9002StateImage, ss_base) == 36);
STATE_ASSERT(compat_ds_base, offsetof(Upd9002StateImage, ds_base) == 40);
STATE_ASSERT(compat_ss_fix, offsetof(Upd9002StateImage, ss_fix) == 44);
STATE_ASSERT(compat_ds_fix, offsetof(Upd9002StateImage, ds_fix) == 48);
STATE_ASSERT(compat_adrsmask, offsetof(Upd9002StateImage, adrsmask) == 52);
STATE_ASSERT(compat_prefix, offsetof(Upd9002StateImage, prefix) == 56);
STATE_ASSERT(compat_trap, offsetof(Upd9002StateImage, trap) == 58);
STATE_ASSERT(compat_resetreq, offsetof(Upd9002StateImage, resetreq) == 59);
STATE_ASSERT(compat_ovflag, offsetof(Upd9002StateImage, ovflag) == 60);
STATE_ASSERT(compat_gdtr, offsetof(Upd9002StateImage, GDTR) == 64);
STATE_ASSERT(compat_msw, offsetof(Upd9002StateImage, MSW) == 70);
STATE_ASSERT(compat_idtr, offsetof(Upd9002StateImage, IDTR) == 72);
STATE_ASSERT(compat_ldtr, offsetof(Upd9002StateImage, LDTR) == 78);
STATE_ASSERT(compat_ldtrc, offsetof(Upd9002StateImage, LDTRC) == 80);
STATE_ASSERT(compat_tr, offsetof(Upd9002StateImage, TR) == 86);
STATE_ASSERT(compat_trc, offsetof(Upd9002StateImage, TRC) == 88);
STATE_ASSERT(compat_padding, offsetof(Upd9002StateImage, padding) == 94);
STATE_ASSERT(compat_cpu_type, offsetof(Upd9002StateImage, cpu_type) == 96);
STATE_ASSERT(compat_itfbank, offsetof(Upd9002StateImage, itfbank) == 97);
STATE_ASSERT(compat_ram_d0, offsetof(Upd9002StateImage, ram_d0) == 98);
STATE_ASSERT(compat_remainclock,
	offsetof(Upd9002StateImage, remainclock) == 100);
STATE_ASSERT(compat_baseclock,
	offsetof(Upd9002StateImage, baseclock) == 104);
STATE_ASSERT(compat_clock, offsetof(Upd9002StateImage, clock) == 108);
STATE_ASSERT(runtime_size,
	sizeof(Upd9002RuntimeState) == sizeof(Upd9002StateImage));
STATE_ASSERT(runtime_align, STATE_ALIGNOF(Upd9002RuntimeState) == 4);
STATE_ASSERT(runtime_padding,
	offsetof(Upd9002RuntimeState, padding) ==
	offsetof(Upd9002StateImage, padding));
STATE_ASSERT(runtime_cpu_type,
	offsetof(Upd9002RuntimeState, cpu_type) ==
	offsetof(Upd9002StateImage, cpu_type));
STATE_ASSERT(image_size,
	sizeof(Upd9002StateOpaqueImage) == sizeof(Upd9002StateImage));

static Upd9002StateOpaqueImage upd9002_state_image;

static void state_set_error(char *error, size_t error_size,
									const char *message) {

	size_t length;

	if ((error == NULL) || (error_size == 0)) {
		return;
	}
	length = strlen(message);
	if (length >= error_size) {
		length = error_size - 1;
	}
	memcpy(error, message, length);
	error[length] = '\0';
}

static void state_overlay_runtime(Upd9002StateImage *state,
									const Upd9002RuntimeState *runtime) {

	const size_t padding = offsetof(Upd9002StateImage, padding);
	const size_t tail = offsetof(Upd9002StateImage, cpu_type);

	memcpy(state, runtime, padding);
	memcpy((UINT8 *)state + tail, (const UINT8 *)runtime + tail,
		sizeof(*state) - tail);
}

static void state_construct_runtime(Upd9002RuntimeState *runtime,
									const Upd9002StateImage *state) {

	const size_t padding = offsetof(Upd9002StateImage, padding);
	const size_t tail = offsetof(Upd9002StateImage, cpu_type);

	memset(runtime, 0, sizeof(*runtime));
	memcpy(runtime, state, padding);
	memcpy((UINT8 *)runtime + tail, (const UINT8 *)state + tail,
		sizeof(*runtime) - tail);
}

void upd9002_state_initialize(void) {

	memset(&upd9002_state_image, 0, sizeof(upd9002_state_image));
}

void upd9002_state_reset(void) {

	memset(&upd9002_state_image, 0, sizeof(upd9002_state_image));
}

void upd9002_state_shut(void) {

	memset(&upd9002_state_image, 0,
		offsetof(Upd9002StateImage, cpu_type));
}

int upd9002_state_validate(const void *payload, size_t size,
										char *error, size_t error_size) {

	Upd9002StateImage state;

	if ((payload == NULL) || (size != sizeof(state))) {
		state_set_error(error, error_size, UPD9002_STATE_ERROR_SIZE);
		return FAILURE;
	}
	memcpy(&state, payload, sizeof(state));
	if (state.cpu_type != CPUTYPE_V30) {
		state_set_error(error, error_size, UPD9002_STATE_ERROR_CPU_TYPE);
		return FAILURE;
	}
	if (state.MSW & MSW_PE) {
		state_set_error(error, error_size,
			UPD9002_STATE_ERROR_PROTECTED_MODE);
		return FAILURE;
	}
	if ((error != NULL) && (error_size != 0)) {
		error[0] = '\0';
	}
	return SUCCESS;
}

int upd9002_state_import(const void *payload, size_t size,
									char *error, size_t error_size) {

	Upd9002StateOpaqueImage next_image;
	Upd9002StateImage state;
	Upd9002RuntimeState next_runtime;

	if (upd9002_state_validate(payload, size, error, error_size) != SUCCESS) {
		return FAILURE;
	}
	memcpy(&state, payload, sizeof(state));
	memcpy(next_image.bytes, payload, sizeof(next_image.bytes));
	state_construct_runtime(&next_runtime, &state);

	/* No CPU callback can run while statsave commits these two objects. */
	upd9002_state_image = next_image;
	upd9002_core_context.s = next_runtime;
	return SUCCESS;
}

void upd9002_state_export(Upd9002StateImage *state) {

	if (state == NULL) {
		return;
	}
	memcpy(state, upd9002_state_image.bytes, sizeof(*state));
	state_overlay_runtime(state, &upd9002_core_context.s);
}
