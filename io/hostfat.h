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
#ifndef VAEG_IO_HOSTFAT_H
#define VAEG_IO_HOSTFAT_H

#include "compiler.h"

#define HOSTFAT_SECTOR_SIZE 1024U
#define HOSTFAT_SECTORS_PER_CLUSTER 16U
#define HOSTFAT_TOTAL_SECTORS 65362U
#define HOSTFAT_BACKING_SECTORS 65536U
#define HOSTFAT_IMAGE_SIZE (HOSTFAT_SECTOR_SIZE * HOSTFAT_BACKING_SECTORS)
#define HOSTFAT_VOLUME_SIZE (HOSTFAT_SECTOR_SIZE * HOSTFAT_TOTAL_SECTORS)
#define HOSTFAT_PROTOCOL_SIGNATURE "H1"
#define HOSTFAT_IDENTITY_SIZE 32U

typedef struct hostfat_prepared_image HOSTFAT_PREPARED_IMAGE;

enum {
	HOSTFAT_RESULT_OK = 0,
	HOSTFAT_RESULT_NOT_MOUNTED = 1,
	HOSTFAT_RESULT_BAD_REQUEST = 2,
	HOSTFAT_RESULT_RANGE = 3
};

#ifdef __cplusplus
extern "C" {
#endif

BOOL hostfat_mount_image(const void *image, UINT32 size);
BOOL hostfat_prepare_image(const void *image, UINT32 size, HOSTFAT_PREPARED_IMAGE **prepared,
                           UINT32 *digest);
BOOL hostfat_commit_prepared_image(HOSTFAT_PREPARED_IMAGE *prepared);
void hostfat_destroy_prepared_image(HOSTFAT_PREPARED_IMAGE *prepared);
void hostfat_unmount(void);
BOOL hostfat_is_mounted(void);
UINT32 hostfat_image_digest(void);
BOOL hostfat_snapshot_identity(void *identity, UINT size);
BOOL hostfat_read_sector(UINT32 sector, void *destination);
UINT8 hostfat_service_request(UINT32 request_far_pointer);

#ifdef __cplusplus
}
#endif

#endif
