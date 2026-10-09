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
#include "upd9002_diagnostic.h"

UPD9002_DIAGNOSTIC upd9002_diagnostic_state;

void upd9002_diagnostic_clear(void) {
	ZeroMemory(&upd9002_diagnostic_state, sizeof(upd9002_diagnostic_state));
}

void upd9002_diagnostic_raise_rep0f(UINT8 prefix, UINT16 cs, UINT16 ip) {
	if (upd9002_diagnostic_state.reason != UPD9002_DIAGNOSTIC_NONE) {
		return;
	}
	upd9002_diagnostic_state.reason = UPD9002_DIAGNOSTIC_REP0F;
	upd9002_diagnostic_state.prefix = prefix;
	upd9002_diagnostic_state.cs = cs;
	upd9002_diagnostic_state.ip = ip;
}

int upd9002_diagnostic_get(UPD9002_DIAGNOSTIC *diagnostic) {
	if ((diagnostic == NULL) || !upd9002_diagnostic_pending()) {
		return FAILURE;
	}
	*diagnostic = upd9002_diagnostic_state;
	return SUCCESS;
}
