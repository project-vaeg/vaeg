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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_SDL2_LIBRASHADER_GL_BRIDGE_H
#define VAEG_SDL2_LIBRASHADER_GL_BRIDGE_H

#include <stdint.h>

#include "librashader/frame_input.h"

typedef struct {
	void *state;
} VAEG_GL_BRIDGE;

typedef enum {
	VAEG_GL_BRIDGE_OK = 0,
	VAEG_GL_BRIDGE_INVALID_ARGUMENT,
	VAEG_GL_BRIDGE_INVALID_FRAME,
	VAEG_GL_BRIDGE_NO_OUTPUT,
	VAEG_GL_BRIDGE_FILTER_FAILURE,
	VAEG_GL_BRIDGE_RESOURCE_FAILURE
} VAEG_GL_BRIDGE_RESULT;

#ifdef __cplusplus
extern "C" {
#endif

int vaeg_gl_bridge_initialize(void *host_window, const char *preset_path, int enable_filter,
                              VAEG_GL_BRIDGE *bridge);
VAEG_GL_BRIDGE_RESULT vaeg_gl_bridge_set_drawable_size(VAEG_GL_BRIDGE *bridge, uint32_t width,
	                                                    uint32_t height);
VAEG_GL_BRIDGE_RESULT vaeg_gl_bridge_set_filter_enabled(VAEG_GL_BRIDGE *bridge, int enabled);
VAEG_GL_BRIDGE_RESULT vaeg_gl_bridge_set_filter_parameter(VAEG_GL_BRIDGE *bridge,
                                                          const char *name, float value);
VAEG_GL_BRIDGE_RESULT vaeg_gl_bridge_present(VAEG_GL_BRIDGE *bridge,
	                                           const VAEG_FRAME_INPUT *frame);
void vaeg_gl_bridge_shutdown(VAEG_GL_BRIDGE *bridge);

#ifdef __cplusplus
}
#endif

#endif
