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
#include <cmath>
#include <cstring>
#include <numeric>

namespace vaeg::librashader {
bool FramePadding::prepare(const VAEG_FRAME_INPUT &source, float percent,
                           VAEG_FRAME_INPUT &output) {
    if (vaeg_frame_input_validate(&source) != VAEG_FRAME_INPUT_OK || !std::isfinite(percent))
        return false;
    output = source;
    percent = std::clamp(percent, 80.0f, 120.0f);
    const uint32_t divisor = std::gcd(source.width, source.height);
    // Even increments preserve both aspect and integral, symmetric margins.
    // Round toward a larger canvas so underscan never supplies less margin.
    const int64_t steps = static_cast<int64_t>(
        std::ceil((divisor * 100.0 / percent - divisor) / 2.0 - 1e-10));
    const int64_t units = static_cast<int64_t>(divisor) + 2 * steps;
    if (units <= 0) return false;
    const uint64_t width = (source.width / divisor) * static_cast<uint64_t>(units);
    const uint64_t height = (source.height / divisor) * static_cast<uint64_t>(units);
    if (width == source.width && height == source.height) return true;
    const uint32_t bytes = source.pixel_format == VAEG_FRAME_PIXEL_RGB565 ? 2 : 4;
    constexpr uint64_t maximum_bytes = 128ULL * 1024 * 1024;
    if (width > maximum_bytes / bytes || height > maximum_bytes / (width * bytes))
        return false;
    const uint32_t pitch = static_cast<uint32_t>(width * bytes);
    const size_t count = static_cast<size_t>(height * pitch);
    if (width_ != width || height_ != height || source_width_ != source.width ||
        source_height_ != source.height || format_ != source.pixel_format) {
        // Swap drops the old capacity, keeping memory bounded after mode changes.
        std::vector<unsigned char> replacement(count, 0);
        if (bytes == 4)
            for (size_t i = 3; i < count; i += 4) replacement[i] = 255;
        pixels_.swap(replacement);
        width_ = static_cast<uint32_t>(width);
        height_ = static_cast<uint32_t>(height);
        source_width_ = source.width;
        source_height_ = source.height;
        format_ = source.pixel_format;
    }
    const uint32_t copy_width = std::min(source.width, width_);
    const uint32_t copy_height = std::min(source.height, height_);
    const uint32_t source_x = (source.width - copy_width) / 2;
    const uint32_t source_y = (source.height - copy_height) / 2;
    const uint32_t dest_x = (width_ - copy_width) / 2;
    const uint32_t dest_y = (height_ - copy_height) / 2;
    const auto *input = static_cast<const unsigned char *>(source.pixels);
    // Preserve row origin as well as format; symmetric margins work bottom-up too.
    for (uint32_t row = 0; row < copy_height; ++row)
        std::memcpy(pixels_.data() + static_cast<size_t>(dest_y + row) * pitch + dest_x * bytes,
                    input + static_cast<size_t>(source_y + row) * source.pitch_bytes + source_x * bytes,
                    static_cast<size_t>(copy_width) * bytes);
    output.pixels = pixels_.data();
    output.width = width_;
    output.height = height_;
    output.pitch_bytes = pitch;
    return true;
}
}
