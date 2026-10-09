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
#ifndef VAEG_UPD9002_STATE_H
#define VAEG_UPD9002_STATE_H

#include "cpucore.h"

#include <stddef.h>

#define UPD9002_STATE_PAYLOAD_SIZE 112
#define UPD9002_STATE_SECTION "UPD9CPU"
#define UPD9002_STATE_VERSION 1
#define UPD9002_STATE_ERROR_SIZE "uPD9002 payload size is not 112 bytes"
#define UPD9002_STATE_ERROR_CPU_TYPE \
	"uPD9002 cpu_type is not CPUTYPE_V30"
#define UPD9002_STATE_ERROR_PROTECTED_MODE \
	"uPD9002 state requires unsupported protected-mode execution"
#define UPD9002_STATE_ERROR_LEGACY_SECTION \
	"obsolete processor state section is unsupported"
#define UPD9002_STATE_ERROR_LEGACY_MARKER \
	"obsolete processor state section lacks uPD9002 format marker"

#ifdef __cplusplus
extern "C" {
#endif

void upd9002_state_initialize(void);
void upd9002_state_reset(void);
void upd9002_state_shut(void);

int upd9002_state_validate(const void *payload, size_t size,
										char *error, size_t error_size);
int upd9002_state_import(const void *payload, size_t size,
									char *error, size_t error_size);
void upd9002_state_export(Upd9002StateImage *state);

#ifdef __cplusplus
}
#endif

#endif
