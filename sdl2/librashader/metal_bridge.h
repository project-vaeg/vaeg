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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_SDL2_LIBRASHADER_METAL_BRIDGE_H
#define VAEG_SDL2_LIBRASHADER_METAL_BRIDGE_H

#include <stdint.h>

#include "librashader/frame_input.h"

typedef struct {
	void *state;
	VAEG_OUTPUT_CAPTURE *capture;
} VAEG_METAL_BRIDGE;

typedef enum {
	VAEG_METAL_BRIDGE_OK = 0,
	VAEG_METAL_BRIDGE_INVALID_ARGUMENT,
	VAEG_METAL_BRIDGE_INVALID_FRAME,
	VAEG_METAL_BRIDGE_NO_DRAWABLE,
	VAEG_METAL_BRIDGE_FILTER_FAILURE,
	VAEG_METAL_BRIDGE_RESOURCE_FAILURE
} VAEG_METAL_BRIDGE_RESULT;

#ifdef __cplusplus
extern "C" {
#endif

int vaeg_metal_bridge_initialize(void *host_window, const char *preset_path, int enable_filter,
                                 VAEG_METAL_BRIDGE *bridge);
void vaeg_metal_bridge_set_drawable_size(const VAEG_METAL_BRIDGE *bridge, uint32_t width,
                                         uint32_t height);
void vaeg_metal_bridge_set_output_viewport(VAEG_METAL_BRIDGE *bridge, int x, int y, int width,
                                           int height);
int vaeg_metal_bridge_gui_prepare(VAEG_METAL_BRIDGE *bridge);
void vaeg_metal_bridge_gui_shutdown(VAEG_METAL_BRIDGE *bridge);
VAEG_METAL_BRIDGE_RESULT vaeg_metal_bridge_set_filter_enabled(VAEG_METAL_BRIDGE *bridge,
                                                               int enabled);
VAEG_METAL_BRIDGE_RESULT vaeg_metal_bridge_set_filter_parameter(VAEG_METAL_BRIDGE *bridge,
                                                                 const char *name, float value);
VAEG_METAL_BRIDGE_RESULT vaeg_metal_bridge_present(VAEG_METAL_BRIDGE *bridge,
                                                    const VAEG_FRAME_INPUT *frame);
void vaeg_metal_bridge_shutdown(VAEG_METAL_BRIDGE *bridge);

#ifdef __cplusplus
}
#endif

#endif
