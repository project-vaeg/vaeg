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

#ifndef VAEG_FRAME_PADDING_H
#define VAEG_FRAME_PADDING_H
#include <cstddef>
#include <vector>
#include "librashader/frame_input.h"

namespace vaeg::librashader {
/* Display-only integer padding/cropping. Source pixels are never resampled.
 * Storage is reused until source format or canvas geometry changes. */
class FramePadding {
  public:
    bool prepare(const VAEG_FRAME_INPUT &source, float percent, VAEG_FRAME_INPUT &output);
    std::size_t storage_bytes() const { return pixels_.size(); }
  private:
    std::vector<unsigned char> pixels_;
    uint32_t source_width_ = 0, source_height_ = 0, width_ = 0, height_ = 0;
    VAEG_FRAME_PIXEL_FORMAT format_ = VAEG_FRAME_PIXEL_RGB565;
};
}
#endif
