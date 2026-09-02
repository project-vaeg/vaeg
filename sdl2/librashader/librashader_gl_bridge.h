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
#ifndef VAEG_LIBRASHADER_GL_BRIDGE_H
#define VAEG_LIBRASHADER_GL_BRIDGE_H

#include <stdint.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include "librashader_loader.h"

class LibrashaderGlBridge {
public:
    explicit LibrashaderGlBridge(SDL_Renderer *renderer);
    ~LibrashaderGlBridge();

    bool initialized() const;
    bool upload_source(const void *pixels, int pitch, uint32_t width,
                       uint32_t height);
    bool ensure_output(uint32_t width, uint32_t height);
    bool load_preset(const char *path);
    bool render_frame(uint32_t frame_count, uint32_t frame_time_ms,
                      uint32_t width, uint32_t height);
    bool draw_output(int drawable_width, int drawable_height, int x, int y,
                     uint32_t width, uint32_t height);
    void disable_filter();

    libra_image_gl_t source_image() const;
    libra_image_gl_t output_image() const;

private:
    void destroy_textures();

    bool initialized_;
    GLuint source_texture_;
    GLuint output_texture_;
    uint32_t source_width_;
    uint32_t source_height_;
    uint32_t output_width_;
    uint32_t output_height_;
    SDL_Renderer *renderer_;
    LibrashaderLoader loader_;
    libra_gl_filter_chain_t chain_;
    bool chain_ready_;
    struct GlApi *api_;
};

#include "librashader_gl_bridge_c.h"

#endif
