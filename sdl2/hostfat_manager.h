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
#ifndef VAEG_SDL2_HOSTFAT_MANAGER_H
#define VAEG_SDL2_HOSTFAT_MANAGER_H

#include "compiler.h"
#include "hostfat_snapshot.h"

enum {
	HOSTFAT_MANAGER_UNMOUNTED = 0,
	HOSTFAT_MANAGER_BUILDING,
	HOSTFAT_MANAGER_MOUNTED,
	HOSTFAT_MANAGER_ERROR
};

enum {
	HOSTFAT_MANAGER_EVENT_NONE = 0,
	HOSTFAT_MANAGER_EVENT_MOUNTED,
	HOSTFAT_MANAGER_EVENT_FAILED
};

typedef struct {
	UINT state;
	BOOL mounted;
	UINT64 completed;
	UINT64 total;
	HOSTFAT_SNAPSHOT_INFO info;
	char phase[64];
	char message[256];
} HOSTFAT_MANAGER_STATUS;

#ifdef __cplusplus
extern "C" {
#endif

BOOL hostfat_manager_initialize(void);
void hostfat_manager_shutdown(void);
BOOL hostfat_manager_mount_startup(const char *path, HOSTFAT_SNAPSHOT_INFO *info, char *error,
                                   UINT error_size);
BOOL hostfat_manager_rebuild_async(const char *path, char *error, UINT error_size);
UINT hostfat_manager_poll(void);
BOOL hostfat_manager_unmount(char *error, UINT error_size);
void hostfat_manager_get_status(HOSTFAT_MANAGER_STATUS *status);
BOOL hostfat_manager_selftest(void);

#ifdef __cplusplus
}
#endif

#endif
