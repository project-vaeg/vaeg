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
#include "librashader_gl_bridge.h"

#include <cstring>

namespace {

constexpr GLenum kSourceInternalFormat = GL_RGBA8;

bool gl_ok() { return glGetError() == GL_NO_ERROR; }

void set_texture_parameters() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

} // namespace

LibrashaderGlBridge::LibrashaderGlBridge(SDL_Renderer *renderer)
    : initialized_(false), source_texture_(0), output_texture_(0),
      source_width_(0), source_height_(0), output_width_(0), output_height_(0) {
    SDL_RendererInfo info{};
    if ((renderer == nullptr) || (SDL_GetRendererInfo(renderer, &info) != 0) ||
        (info.name == nullptr) || (std::strcmp(info.name, "opengl") != 0) ||
        (SDL_GL_GetCurrentContext() == nullptr)) {
        return;
    }

    glGetError();
    glGenTextures(1, &source_texture_);
    glGenTextures(1, &output_texture_);
    if ((source_texture_ == 0) || (output_texture_ == 0) || !gl_ok()) {
        destroy_textures();
        return;
    }

    glBindTexture(GL_TEXTURE_2D, source_texture_);
    set_texture_parameters();
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D, output_texture_);
    set_texture_parameters();
    glBindTexture(GL_TEXTURE_2D, 0);
    initialized_ = gl_ok();
    if (!initialized_) {
        destroy_textures();
    }
}

LibrashaderGlBridge::~LibrashaderGlBridge() { destroy_textures(); }

void LibrashaderGlBridge::destroy_textures() {
    if (SDL_GL_GetCurrentContext() == nullptr) {
        source_texture_ = 0;
        output_texture_ = 0;
        initialized_ = false;
        return;
    }
    if (source_texture_ != 0) {
        glDeleteTextures(1, &source_texture_);
        source_texture_ = 0;
    }
    if (output_texture_ != 0) {
        glDeleteTextures(1, &output_texture_);
        output_texture_ = 0;
    }
    initialized_ = false;
}

bool LibrashaderGlBridge::initialized() const { return initialized_; }

bool LibrashaderGlBridge::upload_source(const void *pixels, int pitch,
                                         uint32_t width, uint32_t height) {
    if (!initialized_ || (pixels == nullptr) || (pitch < 0) || (width == 0) ||
        (height == 0) || ((pitch & 1) != 0) ||
        (static_cast<uint32_t>(pitch / 2) < width)) {
        return false;
    }

    glGetError();
    glBindTexture(GL_TEXTURE_2D, source_texture_);
    if ((source_width_ != width) || (source_height_ != height)) {
        glTexImage2D(GL_TEXTURE_2D, 0, kSourceInternalFormat,
                     static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                     0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);
        source_width_ = width;
        source_height_ = height;
    } else {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 2);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(width),
                        static_cast<GLsizei>(height), GL_RGB,
                        GL_UNSIGNED_SHORT_5_6_5, pixels);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return gl_ok();
}

bool LibrashaderGlBridge::ensure_output(uint32_t width, uint32_t height) {
    if (!initialized_ || (width == 0) || (height == 0)) {
        return false;
    }
    if ((output_width_ == width) && (output_height_ == height)) {
        return true;
    }

    glGetError();
    glBindTexture(GL_TEXTURE_2D, output_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, kSourceInternalFormat,
                 static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!gl_ok()) {
        return false;
    }
    output_width_ = width;
    output_height_ = height;
    return true;
}

libra_image_gl_t LibrashaderGlBridge::source_image() const {
    return {source_texture_, kSourceInternalFormat, source_width_,
            source_height_};
}

libra_image_gl_t LibrashaderGlBridge::output_image() const {
    return {output_texture_, kSourceInternalFormat, output_width_,
            output_height_};
}
