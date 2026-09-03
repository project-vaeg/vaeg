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

#include "librashader/librashader_gl_bridge_c.h"

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

int test_preset(void *bridge, SDL_Renderer *renderer, const char *path,
                unsigned int frame) {
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
    if (!librashader_gl_bridge_upload_source(
            bridge, source.data(), source_pitch, source_width, source_height) ||
        !librashader_gl_bridge_load_preset(bridge, path) ||
        !librashader_gl_bridge_render_frame(bridge, frame, 16, drawable_width,
                                            drawable_height) ||
        !librashader_gl_bridge_draw_output(
            bridge, drawable_width, drawable_height, 0, 0, drawable_width,
            drawable_height)) {
        std::fprintf(stderr, "librashader GL smoke: preset failed: %s\n", path);
        return 1;
    }

    glFinish();
    const int pitch = drawable_width * 4;
    std::vector<uint8_t> pixels(static_cast<size_t>(pitch) * drawable_height);
    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32,
                             pixels.data(), pitch) != 0) {
        std::fprintf(stderr, "librashader GL smoke: readback failed: %s\n",
                     SDL_GetError());
        return 1;
    }
    uint64_t checksum = 0;
    size_t nonzero = 0;
    for (uint8_t value : pixels) {
        checksum = (checksum * 131U) + value;
        nonzero += (value != 0U) ? 1U : 0U;
    }
    if (nonzero == 0U) {
        std::fprintf(stderr, "librashader GL smoke: black output: %s\n", path);
        return 1;
    }
    std::printf("preset=%s width=%d height=%d nonzero=%zu checksum=%llu\n", path,
                drawable_width, drawable_height, nonzero,
                static_cast<unsigned long long>(checksum));
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

    void *bridge = librashader_gl_bridge_create(renderer);
    if (bridge == nullptr) {
        std::fprintf(stderr, "librashader GL smoke: OpenGL bridge unavailable\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    int result = test_preset(bridge, renderer, argv[1], 0);
    if (result == 0) {
        result = test_preset(bridge, renderer, argv[2], 1);
    }
    librashader_gl_bridge_destroy(bridge);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
