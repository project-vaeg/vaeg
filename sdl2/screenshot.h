/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
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
#ifndef VAEG_SDL2_SCREENSHOT_H
#define VAEG_SDL2_SCREENSHOT_H

#include "compiler.h"

enum {
	VAEG_SCREENSHOT_FORMAT_BMP = 1,
	VAEG_SCREENSHOT_FORMAT_PNG = 2,
	VAEG_SCREENSHOT_MAX_REQUESTS = 64
};

typedef struct {
	UINT32 frame;
	const char *path;
	UINT format;
} VAEG_SCREENSHOT_REQUEST;

typedef struct {
	VAEG_SCREENSHOT_REQUEST requests[VAEG_SCREENSHOT_MAX_REQUESTS];
	UINT count;
	UINT next;
} VAEG_SCREENSHOT_SCHEDULER;

typedef BOOL (*VAEG_SCREENSHOT_CAPTURE_FN)(UINT32 frame,
	                                        const VAEG_SCREENSHOT_REQUEST *request,
	                                        void *opaque);

void vaeg_screenshot_scheduler_init(VAEG_SCREENSHOT_SCHEDULER *scheduler,
	                                   const VAEG_SCREENSHOT_REQUEST *requests,
	                                   UINT count);
BOOL vaeg_screenshot_scheduler_wants_frame(const VAEG_SCREENSHOT_SCHEDULER *scheduler,
	                                          UINT32 frame);
BOOL vaeg_screenshot_scheduler_after_frame(VAEG_SCREENSHOT_SCHEDULER *scheduler,
	                                          UINT32 frame,
	                                          VAEG_SCREENSHOT_CAPTURE_FN capture,
	                                          void *opaque);
BOOL vaeg_screenshot_scheduler_done(const VAEG_SCREENSHOT_SCHEDULER *scheduler);

#endif
