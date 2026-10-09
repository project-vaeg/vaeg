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
#ifndef VAEG_SDL2_HOSTFAT_SNAPSHOT_H
#define VAEG_SDL2_HOSTFAT_SNAPSHOT_H

#include "compiler.h"

typedef struct {
	UINT files;
	UINT directories;
	UINT64 source_bytes;
	UINT32 digest;
} HOSTFAT_SNAPSHOT_INFO;

typedef struct hostfat_snapshot_candidate HOSTFAT_SNAPSHOT_CANDIDATE;
typedef void (*HOSTFAT_SNAPSHOT_PROGRESS)(void *context, const char *phase, UINT64 completed,
                                          UINT64 total);

#ifdef __cplusplus
extern "C" {
#endif

BOOL hostfat_snapshot_mount_directory(const char *path, HOSTFAT_SNAPSHOT_INFO *info, char *error,
                                      UINT error_size);
BOOL hostfat_snapshot_build_directory(const char *path, HOSTFAT_SNAPSHOT_CANDIDATE **candidate,
                                      HOSTFAT_SNAPSHOT_PROGRESS progress, void *progress_context,
                                      char *error, UINT error_size);
BOOL hostfat_snapshot_candidate_mount(HOSTFAT_SNAPSHOT_CANDIDATE *candidate,
                                      HOSTFAT_SNAPSHOT_INFO *info, char *error, UINT error_size);
void hostfat_snapshot_candidate_destroy(HOSTFAT_SNAPSHOT_CANDIDATE *candidate);
void hostfat_snapshot_unmount(void);
BOOL hostfat_snapshot_selftest(void);

#ifdef __cplusplus
}
#endif

#endif
