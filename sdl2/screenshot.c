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
#include "screenshot.h"

void vaeg_screenshot_scheduler_init(VAEG_SCREENSHOT_SCHEDULER *scheduler,
	                                   const VAEG_SCREENSHOT_REQUEST *requests,
	                                   UINT count) {
	UINT i;

	if (scheduler == NULL) {
		return;
	}
	ZeroMemory(scheduler, sizeof(*scheduler));
	if (requests == NULL) {
		return;
	}
	if (count > VAEG_SCREENSHOT_MAX_REQUESTS) {
		count = VAEG_SCREENSHOT_MAX_REQUESTS;
	}
	for (i = 0; i < count; i++) {
		UINT insert;

		insert = i;
		while ((insert > 0) && (scheduler->requests[insert - 1].frame > requests[i].frame)) {
			scheduler->requests[insert] = scheduler->requests[insert - 1];
			insert--;
		}
		scheduler->requests[insert] = requests[i];
	}
	scheduler->count = count;
}

BOOL vaeg_screenshot_scheduler_wants_frame(const VAEG_SCREENSHOT_SCHEDULER *scheduler,
	                                          UINT32 frame) {
	if ((scheduler == NULL) || (scheduler->next >= scheduler->count)) {
		return FALSE;
	}
	return (scheduler->requests[scheduler->next].frame == frame) ? TRUE : FALSE;
}

BOOL vaeg_screenshot_scheduler_after_frame(VAEG_SCREENSHOT_SCHEDULER *scheduler,
	                                          UINT32 frame,
	                                          VAEG_SCREENSHOT_CAPTURE_FN capture,
	                                          void *opaque) {
	if ((scheduler == NULL) || (capture == NULL)) {
		return (scheduler != NULL) ? SUCCESS : FAILURE;
	}
	if ((scheduler->next < scheduler->count) &&
	    (frame > scheduler->requests[scheduler->next].frame)) {
		fprintf(stderr,
		        "Error: screenshot frame %u was missed before completed guest frame %u\n",
		        scheduler->requests[scheduler->next].frame, frame);
		return FAILURE;
	}
	while ((scheduler->next < scheduler->count) &&
	       (scheduler->requests[scheduler->next].frame == frame)) {
		if (capture(frame, &scheduler->requests[scheduler->next], opaque) != SUCCESS) {
			return FAILURE;
		}
		scheduler->next++;
	}
	return SUCCESS;
}

BOOL vaeg_screenshot_scheduler_done(const VAEG_SCREENSHOT_SCHEDULER *scheduler) {
	return ((scheduler != NULL) && (scheduler->count != 0) &&
	        (scheduler->next >= scheduler->count)) ? TRUE : FALSE;
}
