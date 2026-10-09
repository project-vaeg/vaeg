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

#include "librashader/frame_padding.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

using namespace vaeg::librashader;
static int failures;
static void check(bool ok, const char *label) {
    if (!ok) { ++failures; std::fprintf(stderr, "FRAME_PADDING_FAILED: %s\n", label); }
}
int main() {
    FramePadding padding;
    for (auto format : {VAEG_FRAME_PIXEL_RGB565, VAEG_FRAME_PIXEL_ARGB8888}) {
        const unsigned bytes = format == VAEG_FRAME_PIXEL_RGB565 ? 2 : 4;
        const unsigned pitch = 640 * bytes + 16;
        std::vector<unsigned char> source(pitch * 400);
        for (size_t i = 0; i < source.size(); ++i) source[i] = static_cast<unsigned char>(i * 37);
        const auto original = source;
        for (auto origin : {VAEG_FRAME_ROWS_TOP_DOWN, VAEG_FRAME_ROWS_BOTTOM_UP}) {
            VAEG_FRAME_INPUT frame = {source.data(), 640, 400, pitch, format, origin,
                                      4, 3, 60, 1, 42, 16666667};
            VAEG_FRAME_INPUT output{};
            for (float percent : {100.0f, 96.5f, 80.0f, 120.0f, 96.5f, 100.0f}) {
                check(padding.prepare(frame, percent, output), "prepare");
                check(output.width * 400 == output.height * 640, "source aspect");
                check(output.source_aspect_width == 4 && output.source_aspect_height == 3 &&
                      output.frame_number == 42 && output.frame_time_delta_ns == 16666667 &&
                      output.row_origin == origin && output.pixel_format == format, "metadata");
                if (percent == 100) {
                    check(output.pixels == source.data(), "100 percent borrowed frame");
                    continue;
                }
                if (percent == 96.5f) check(output.width == 672 && output.height == 420, "96.5 canvas");
                if (percent == 80) check(output.width == 800 && output.height == 500, "80 canvas");
                const unsigned cw = std::min(frame.width, output.width);
                const unsigned ch = std::min(frame.height, output.height);
                const unsigned dx = (output.width - cw) / 2, dy = (output.height - ch) / 2;
                const unsigned sx = (frame.width - cw) / 2, sy = (frame.height - ch) / 2;
                const auto *data = static_cast<const unsigned char *>(output.pixels);
                for (unsigned row = 0; row < ch; ++row)
                    check(std::memcmp(data + (row + dy) * output.pitch_bytes + dx * bytes,
                                      source.data() + (row + sy) * pitch + sx * bytes,
                                      cw * bytes) == 0, "every copied pixel exact");
                if (percent < 100) {
                    check(cw == 640 && ch == 400, "all 256000 source pixels retained");
                    for (unsigned y = 0; y < output.height; ++y)
                        for (unsigned x = 0; x < output.width; ++x)
                            if (x < dx || x >= dx + cw || y < dy || y >= dy + ch)
                                for (unsigned c = 0; c < bytes; ++c)
                                    check(data[y * output.pitch_bytes + x * bytes + c] ==
                                          (c == 3 ? 255 : 0), "opaque black border");
                }
                const void *storage = output.pixels;
                check(padding.prepare(frame, percent, output) && output.pixels == storage,
                      "steady-state storage reused");
                check(source == original, "raw source untouched");
            }
            check(!padding.prepare(frame, std::numeric_limits<float>::quiet_NaN(), output), "reject NaN");
            frame.width = 65536; frame.height = 65536; frame.pitch_bytes = 65536 * bytes;
            check(!padding.prepare(frame, 80, output), "bounded storage");
        }
    }
    // Reproducible CPU-copy cost only, not GPU/frame presentation performance.
    std::vector<unsigned char> source(640 * 400 * 4, 255);
    VAEG_FRAME_INPUT frame = {source.data(), 640, 400, 2560, VAEG_FRAME_PIXEL_ARGB8888,
                              VAEG_FRAME_ROWS_TOP_DOWN, 4, 3, 60, 1, 1, 16666667}, output{};
    padding.prepare(frame, 96.5f, output);
    const void *storage = output.pixels;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 10000; ++i) {
        source[0] = static_cast<unsigned char>(i);
        if (!padding.prepare(frame, 96.5f, output)) return 1;
    }
    const double microseconds = std::chrono::duration<double, std::micro>(
        std::chrono::steady_clock::now() - start).count() / 10000;
    check(output.pixels == storage, "benchmark reused buffer");
    std::printf("padding 640x400 -> 672x420: %.3f us/frame CPU; storage=%zu bytes\n",
                microseconds, padding.storage_bytes());
    return failures ? 1 : 0;
}
