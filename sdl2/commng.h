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
#ifndef VAEG_SDL2_COMMNG_H
#define VAEG_SDL2_COMMNG_H

enum {
	COMCREATE_SERIAL = 0,
	COMCREATE_PRINTER,
	COMCREATE_MPU98II
};

enum {
	COMCONNECT_OFF = 0,
	COMCONNECT_SERIAL,
	COMCONNECT_MIDI,
	COMCONNECT_PARALLEL
};

enum {
	COMMSG_MIDIRESET = 0,
	COMMSG_SETFLAG,
	COMMSG_GETFLAG,
	COMMSG_SETRSFLAG,
	COMMSG_USER
};

struct _commng;
typedef struct _commng _COMMNG;
typedef struct _commng *COMMNG;

struct _commng {
	UINT connect;
	UINT (*read)(COMMNG self, BYTE *data);
	UINT (*write)(COMMNG self, BYTE data);
	BYTE (*getstat)(COMMNG self);
	VAEG_INTPTR (*msg)(COMMNG self, UINT msg, VAEG_INTPTR param);
	void (*release)(COMMNG self);
};

typedef struct {
	UINT32 size;
	UINT32 sig;
	UINT32 ver;
	UINT32 param;
} _COMFLAG, *COMFLAG;

#ifdef __cplusplus
extern "C" {
#endif

COMMNG commng_create(UINT device);
void commng_destroy(COMMNG hdl);
void commng_initialize(void);

#ifdef __cplusplus
}
#endif

#endif
