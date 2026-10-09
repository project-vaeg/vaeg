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
#ifndef VAEG_SDL2_SYSMNG_H
#define VAEG_SDL2_SYSMNG_H

enum {
	SYS_UPDATECFG = 0x0001,
	SYS_UPDATEOSCFG = 0x0002,
	SYS_UPDATECLOCK = 0x0004,
	SYS_UPDATERATE = 0x0008,
	SYS_UPDATESBUF = 0x0010,
	SYS_UPDATEMIDI = 0x0020,
	SYS_UPDATESBOARD = 0x0040,
	SYS_UPDATEFDD = 0x0080,
	SYS_UPDATEHDD = 0x0100,
	SYS_UPDATEMEMORY = 0x0200,
	SYS_UPDATESERIAL1 = 0x0400
};

#ifdef __cplusplus
extern "C" {
#endif

extern UINT sys_updates;

#define sysmng_initialize() sys_updates = 0
#define sysmng_update(a) sys_updates |= (a)
#define sysmng_fddaccess(a)
#define sysmng_hddaccess(a)
void sysmng_cpureset(void);
void sysmng_modeled(BYTE num, BYTE sw);

#ifdef __cplusplus
}
#endif

#endif
