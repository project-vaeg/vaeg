/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>
using std::clamp;
using std::min;
using std::max;
using std::sin;
using std::cos;
using std::floor;
static float mix(float a, float b, float t) { return a + (b - a) * t; }
static float smoothstep(float a, float b, float x) {
    float t = clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}
// Execute the production GLSL float functions, not a second implementation.
#include "vaeg-scanline-aa.inc"
static int failures;
static void check(bool condition, const char *code) {
    if (!condition) { ++failures; std::fprintf(stderr, "%s\n", code); }
}
static double point(double phase, double thin) {
    phase -= std::floor(phase);
    constexpr double tau = 6.283185307179586;
    return 1 + 0.5 * (std::cos(tau * std::min(0.5, phase * thin))
                      + std::cos(tau * std::min(0.5, (1 - phase) * thin)));
}
static double amplitude(const std::vector<float> &values, double frequency) {
    double re = 0, im = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        double angle = 6.283185307179586 * frequency * i;
        re += values[i] * std::cos(angle); im += values[i] * std::sin(angle);
    }
    return 2 * std::hypot(re, im) / values.size();
}
int main() {
    // Independent double-precision midpoint integration across period boundaries.
    for (float thin : {0.5f, 0.6f, 0.75f, 0.9f, 1.0f})
        for (float width : {0.002f, 0.125f, 0.25625f, 0.5f, 0.75f, 0.999f, 1.025f, 4.0f})
            for (float phase : {0.0f, 0.001f, 0.23f, 0.5f, 0.99f}) {
                double expected = 0;
                for (int i = 0; i < 10000; ++i)
                    expected += point(phase + width * ((i + 0.5) / 10000 - 0.5), thin);
                expected /= 10000;
                double fade = std::clamp((width - 0.25) / 0.25, 0.0, 1.0);
                fade = fade * fade * (3 - 2 * fade);
                expected += (0.5 / thin - expected) * fade;
                float actual = vaeg_scan_average(phase, thin, width);
                check(std::isfinite(actual) && std::abs(actual - expected) < 0.00015,
                      "M99_SCAN_AA_INTEGRAL_MISMATCH");
                check(std::abs(vaeg_scan_average(phase, thin, 0) - point(phase, thin)) < 0.000001,
                      "M99_SCAN_AA_POINT_LIMIT");
            }
    for (int height : {400, 800, 1600}) {
        std::vector<float> before(height), after(height);
        float width = 410.0f / height;
        for (int y = 0; y < height; ++y) {
            double pos = (y + 0.5) * 410.0 / height - 0.5;
            float phase = static_cast<float>(pos - std::floor(pos));
            before[y] = static_cast<float>(point(phase, 0.75));
            after[y] = vaeg_scan_average(phase, 0.75f, width);
        }
        double old_band = amplitude(before, 1.0 / 40);
        double new_band = amplitude(after, 1.0 / 40);
        std::printf("410 -> %d: 40px band %.8f -> %.8f\n", height, old_band, new_band);
        check(new_band < old_band * 0.04 + 0.000001, "M99_SCAN_AA_40PX_BAND");
        if (height == 400)
            check(*std::max_element(after.begin(), after.end()) -
                  *std::min_element(after.begin(), after.end()) < 0.000001,
                  "M99_SCAN_AA_SMALL_OUTPUT_MODULATION");
        if (height == 1600) {
            double retained = amplitude(after, 410.0 / height) / amplitude(before, 410.0 / height);
            std::printf("large-output fundamental contrast retained %.4f\n", retained);
            check(retained > 0.85 && retained < 1.01, "M99_SCAN_AA_LARGE_OUTPUT_CONTRAST");
        }
    }
    // CPU math cost only; not GPU performance or physical playback evidence.
    volatile float sink = 0;
    for (int mode = 0; mode < 2; ++mode) {
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < 1000000; ++i) {
            float phase = (i % 400) / 400.0f;
            sink = sink + (mode ? vaeg_scan_average(phase, 0.75f, 0.25625f)
                                : vaeg_scan_point(phase, 0.75f));
        }
        double ns = std::chrono::duration<double, std::nano>(
            std::chrono::steady_clock::now() - start).count() / 1000000;
        std::printf("CPU scanline %s: %.2f ns/evaluation\n", mode ? "AA" : "point", ns);
    }
    return failures ? 1 : 0;
}
