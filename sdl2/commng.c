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
#include "np2.h"
#include "commng.h"
#include "cmver.h"
#include "cmjasts.h"

static UINT ncread(COMMNG self, BYTE *data) {
	(void)self;
	(void)data;
	return (0);
}

static UINT ncwrite(COMMNG self, BYTE data) {
	(void)self;
	(void)data;
	return (0);
}

static BYTE ncgetstat(COMMNG self) {
	(void)self;
	return (0xf0);
}

static VAEG_INTPTR ncmsg(COMMNG self, UINT msg, VAEG_INTPTR param) {
	(void)self;
	(void)msg;
	(void)param;
	return (0);
}

static void ncrelease(COMMNG self) {
	(void)self;
}

static const _COMMNG com_nc = {COMCONNECT_OFF, ncread, ncwrite, ncgetstat, ncmsg, ncrelease};

void commng_initialize(void) {
	cmvermouth_initialize();
}

COMMNG commng_create(UINT device) {
	COMMNG ret;

	ret = NULL;
	if (device == COMCREATE_MPU98II) {
		ret = cmvermouth_create();
	} else if (device == COMCREATE_PRINTER) {
		if (np2oscfg.jastsnd) {
			ret = cmjasts_create();
		}
	}
	if (ret == NULL) {
		ret = (COMMNG)&com_nc;
	}
	return (ret);
}

void commng_destroy(COMMNG hdl) {
	if (hdl) {
		hdl->release(hdl);
	}
}
