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
#ifndef VAEG_SOUND_YMFMBRIDGE_H
#define VAEG_SOUND_YMFMBRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
	YMFMBRIDGE_FIDELITY_MINIMUM = 0,
	YMFMBRIDGE_FIDELITY_MEDIUM,
	YMFMBRIDGE_FIDELITY_MAXIMUM,
	YMFMBRIDGE_FIDELITY_COUNT,
	YMFMBRIDGE_FIDELITY_DEFAULT = YMFMBRIDGE_FIDELITY_MINIMUM
};

void ymfm_opn_initialize(UINT rate);
UINT ymfm_opn_parsefidelity(const char *name);
const char *ymfm_opn_fidelityname(UINT fidelity);
void ymfm_opn_setfidelity(UINT fidelity);
UINT ymfm_opn_getfidelity(void);
void ymfm_opn_setvol(UINT vol);
void ymfm_opn_setvr(REG8 channel, REG8 value);
void ymfm_opn_reset(void);
void ymfm_opn_setcfg(REG8 maxch, UINT flag);
void ymfm_opn_setcontrol(REG8 chbase, REG8 reg, REG8 value);
void ymfm_opn_setextch(UINT chnum, REG8 data);
void ymfm_opn_setreg(REG8 chbase, REG8 reg, REG8 value);
void ymfm_opn_keyon(UINT chnum, REG8 value);
void ymfm_opn_timerover(UINT timer);
void ymfm_opn_getpcm(SINT32 *pcm, UINT count, BOOL use_vr);

/* Serialize the complete YMFM bridge state used by OPN/OPNA synthesis. */
UINT32 ymfm_opn_state_size(void);
int ymfm_opn_state_save(void *buffer, UINT32 size);
int ymfm_opn_state_load(const void *buffer, UINT32 size);

#ifdef __cplusplus
}
#endif

#endif
