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
#ifndef VAEG_UPD9002_PERF_H
#define VAEG_UPD9002_PERF_H

#include <stdint.h>

enum {
	UPD9002_PERF_RESERVED_PLAIN = 0,
	UPD9002_PERF_RESERVED_0F,
	UPD9002_PERF_RESERVED_REPNC,
	UPD9002_PERF_RESERVED_REPC,
	UPD9002_PERF_RESERVED_REP0F_DIAGNOSTIC,
	UPD9002_PERF_RESERVED_COUNT
};

#ifdef VAEG_UPD9002_PERF_DIAGNOSTIC

void upd9002_perf_start_from_env(void);
void upd9002_perf_stop(void);
void upd9002_perf_record_step(uint32_t cs_base, uint16_t ip, uint8_t opcode, uint8_t next_byte);
void upd9002_perf_record_0f(uint8_t opcode);
void upd9002_perf_record_reserved(uint32_t kind);
void upd9002_perf_record_exception(uint8_t vect);
void upd9002_perf_record_interrupt(uint8_t vect);

#else

#define upd9002_perf_start_from_env() ((void)0)
#define upd9002_perf_stop() ((void)0)
#define upd9002_perf_record_step(cs_base, ip, opcode, next_byte)                                   \
	((void)(cs_base), (void)(ip), (void)(opcode), (void)(next_byte))
#define upd9002_perf_record_0f(opcode) ((void)(opcode))
#define upd9002_perf_record_reserved(kind) ((void)(kind))
#define upd9002_perf_record_exception(vect) ((void)(vect))
#define upd9002_perf_record_interrupt(vect) ((void)(vect))

#endif

#endif
