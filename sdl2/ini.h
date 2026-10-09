
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

#ifndef VAEG_SDL2_INI_H
#define VAEG_SDL2_INI_H

enum {
	INITYPE_STR = 0,
	INITYPE_BOOL,
	INITYPE_BYTEARG,
	INITYPE_SINT8,
	INITYPE_SINT16,
	INITYPE_SINT32,
	INITYPE_UINT8,
	INITYPE_UINT16,
	INITYPE_UINT32,
	INITYPE_HEX8,
	INITYPE_HEX16,
	INITYPE_HEX32,
	INITYPE_USER
};

typedef struct {
	const char *item;
	UINT itemtype;
	void *value;
	UINT size;
} INITBL;

#ifdef __cplusplus
extern "C" {
#endif

void ini_read(const char *path, const char *title, const INITBL *tbl, UINT count);
int ini_write(const char *path, const char *title, const INITBL *tbl, UINT count);

void initload(void);
BOOL initcrtchanged(void);
void initsave(void);
int initsave_checked(void);
void initsetpath(const char *path);
void initsetenabled(BOOL enabled);

#ifdef __cplusplus
}
#endif

#endif
