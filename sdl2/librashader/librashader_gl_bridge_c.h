/*
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef VAEG_LIBRASHADER_GL_BRIDGE_C_H
#define VAEG_LIBRASHADER_GL_BRIDGE_C_H

#include <stdint.h>

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

void *librashader_gl_bridge_create(SDL_Renderer *renderer);
void librashader_gl_bridge_destroy(void *bridge);
int librashader_gl_bridge_upload_source(void *bridge, const void *pixels,
                                        int pitch, uint32_t width,
                                        uint32_t height);
int librashader_gl_bridge_load_preset(void *bridge, const char *path);
int librashader_gl_bridge_render_frame(void *bridge, uint32_t frame_count,
                                       uint32_t frame_time_ms, uint32_t width,
                                       uint32_t height);
int librashader_gl_bridge_draw_output(void *bridge, int drawable_width,
                                      int drawable_height, int x, int y,
                                      uint32_t width, uint32_t height);
void librashader_gl_bridge_disable_filter(void *bridge);

#ifdef __cplusplus
}
#endif

#endif
