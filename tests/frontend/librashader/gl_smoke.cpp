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
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_opengl.h>

#include "librashader/librashader_gl_bridge.h"

#ifndef APIENTRY
#define APIENTRY
#endif

namespace {

int skip(const char *reason) {
    std::fprintf(stderr, "SKIP: librashader GL smoke: %s\n", reason);
    return 77;
}

bool runtime_present(const std::filesystem::path &directory) {
#if defined(_WIN32)
    constexpr const char *name = "librashader.dll";
#elif defined(__APPLE__)
    constexpr const char *name = "librashader.dylib";
#else
    constexpr const char *name = "librashader.so";
#endif
    return std::filesystem::is_regular_file(directory / name);
}

uint16_t source_pixel(unsigned int x, unsigned int y, unsigned int width,
                      unsigned int height) {
    const unsigned int red = (x * 31U) / (width - 1U);
    const unsigned int green = ((x ^ y) * 63U) / (width + height - 2U);
    const unsigned int blue = (y * 31U) / (height - 1U);
    return static_cast<uint16_t>((red << 11U) | (green << 5U) | blue);
}

struct Readback {
    uint64_t checksum;
    size_t nonzero_color_bytes;
};

bool read_output_texture(const LibrashaderGlBridge &bridge, Readback *readback) {
    using GenFramebuffers = void(APIENTRY *)(GLsizei, GLuint *);
    using BindFramebuffer = void(APIENTRY *)(GLenum, GLuint);
    using FramebufferTexture2D = void(APIENTRY *)(GLenum, GLenum, GLenum, GLuint,
                                                  GLint);
    using CheckFramebufferStatus = GLenum(APIENTRY *)(GLenum);
    using DeleteFramebuffers = void(APIENTRY *)(GLsizei, const GLuint *);
    const auto gen_framebuffers = reinterpret_cast<GenFramebuffers>(
        SDL_GL_GetProcAddress("glGenFramebuffers"));
    const auto bind_framebuffer = reinterpret_cast<BindFramebuffer>(
        SDL_GL_GetProcAddress("glBindFramebuffer"));
    const auto framebuffer_texture_2d = reinterpret_cast<FramebufferTexture2D>(
        SDL_GL_GetProcAddress("glFramebufferTexture2D"));
    const auto check_framebuffer_status = reinterpret_cast<CheckFramebufferStatus>(
        SDL_GL_GetProcAddress("glCheckFramebufferStatus"));
    const auto delete_framebuffers = reinterpret_cast<DeleteFramebuffers>(
        SDL_GL_GetProcAddress("glDeleteFramebuffers"));
    if ((gen_framebuffers == nullptr) || (bind_framebuffer == nullptr) ||
        (framebuffer_texture_2d == nullptr) ||
        (check_framebuffer_status == nullptr) ||
        (delete_framebuffers == nullptr) || (readback == nullptr)) {
        std::fprintf(stderr, "librashader GL smoke: framebuffer API unavailable\n");
        return false;
    }

    const libra_image_gl_t output = bridge.output_image();
    GLuint framebuffer = 0;
    glFinish();
    gen_framebuffers(1, &framebuffer);
    bind_framebuffer(GL_FRAMEBUFFER, framebuffer);
    framebuffer_texture_2d(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           output.handle, 0);
    const GLenum status = check_framebuffer_status(GL_FRAMEBUFFER);
    if ((framebuffer == 0) || (status != GL_FRAMEBUFFER_COMPLETE)) {
        std::fprintf(stderr,
                     "librashader GL smoke: output framebuffer incomplete: 0x%x\n",
                     static_cast<unsigned int>(status));
        bind_framebuffer(GL_FRAMEBUFFER, 0);
        if (framebuffer != 0) {
            delete_framebuffers(1, &framebuffer);
        }
        return false;
    }

    const int pitch = static_cast<int>(output.width * 4U);
    std::vector<uint8_t> pixels(static_cast<size_t>(pitch) * output.height);
    glReadPixels(0, 0, static_cast<GLsizei>(output.width),
                 static_cast<GLsizei>(output.height), GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels.data());
    const GLenum readback_error = glGetError();
    bind_framebuffer(GL_FRAMEBUFFER, 0);
    delete_framebuffers(1, &framebuffer);
    if (readback_error != GL_NO_ERROR) {
        std::fprintf(stderr,
                     "librashader GL smoke: output texture readback failed: 0x%x\n",
                     static_cast<unsigned int>(readback_error));
        return false;
    }

    readback->checksum = 0;
    readback->nonzero_color_bytes = 0;
    for (size_t index = 0; index < pixels.size(); ++index) {
        const uint8_t value = pixels[index];
        readback->checksum = (readback->checksum * 131U) + value;
        if ((index & 3U) != 3U) {
            readback->nonzero_color_bytes += (value != 0U) ? 1U : 0U;
        }
    }
    return true;
}

int test_preset(LibrashaderGlBridge *bridge, const char *path,
                unsigned int frame, Readback *readback) {
    constexpr unsigned int source_width = 96;
    constexpr unsigned int source_height = 64;
    constexpr int drawable_width = 320;
    constexpr int drawable_height = 240;
    const int source_pitch = static_cast<int>(source_width * sizeof(uint16_t));
    std::vector<uint16_t> source(source_width * source_height);
    for (unsigned int y = 0; y < source_height; ++y) {
        for (unsigned int x = 0; x < source_width; ++x) {
            source[y * source_width + x] =
                source_pixel(x, y, source_width, source_height);
        }
    }
    std::fprintf(stderr, "librashader GL smoke: testing preset: %s\n", path);
    if (!bridge->upload_source(source.data(), source_pitch, source_width,
                               source_height) ||
        !bridge->load_preset(path) ||
        !bridge->render_frame(frame, 16, drawable_width, drawable_height) ||
        !read_output_texture(*bridge, readback) ||
        !bridge->draw_output(drawable_width, drawable_height, 0, 0,
                             drawable_width, drawable_height)) {
        std::fprintf(stderr, "librashader GL smoke: preset failed: %s\n", path);
        return 1;
    }
    if (readback->nonzero_color_bytes == 0U) {
        std::fprintf(stderr, "librashader GL smoke: black output: %s\n", path);
        return 1;
    }
    std::fprintf(stderr,
                 "librashader GL smoke: passed preset=%s width=%d height=%d "
                 "nonzero_color_bytes=%zu checksum=%llu\n",
                 path, drawable_width, drawable_height,
                 readback->nonzero_color_bytes,
                 static_cast<unsigned long long>(readback->checksum));
    return 0;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s PASS_THROUGH_PRESET CRT_PRESET\n", argv[0]);
        return 2;
    }
    const char *runtime_dir = std::getenv("VAEG_LIBRASHADER_RUNTIME_DIR");
    if ((runtime_dir == nullptr) || (runtime_dir[0] == '\0')) {
        return skip("VAEG_LIBRASHADER_RUNTIME_DIR is not set");
    }
    const std::filesystem::path runtime_path(runtime_dir);
    if (!runtime_present(runtime_path)) {
        return skip("the configured librashader runtime is absent");
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return skip(SDL_GetError());
    }
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_Window *window = SDL_CreateWindow(
        "vaeg librashader GL smoke", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_HIDDEN);
    if (window == nullptr) {
        const std::string error = SDL_GetError();
        SDL_Quit();
        return skip(error.c_str());
    }
    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        const std::string error = SDL_GetError();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return skip(error.c_str());
    }
    SDL_RendererInfo renderer_info{};
    if ((SDL_GetRendererInfo(renderer, &renderer_info) != 0) ||
        (renderer_info.name == nullptr) ||
        (std::string(renderer_info.name) != "opengl")) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return skip("SDL did not create an OpenGL renderer");
    }

    LibrashaderGlBridge bridge(renderer);
    if (!bridge.initialized()) {
        std::fprintf(stderr, "librashader GL smoke: OpenGL bridge unavailable\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    Readback pass_through{};
    Readback lottes{};
    int result = test_preset(&bridge, argv[1], 0, &pass_through);
    if (result == 0) {
        result = test_preset(&bridge, argv[2], 1, &lottes);
    }
    if ((result == 0) && (pass_through.checksum == lottes.checksum)) {
        std::fprintf(stderr,
                     "librashader GL smoke: CRT preset matched pass-through "
                     "output\n");
        result = 1;
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
