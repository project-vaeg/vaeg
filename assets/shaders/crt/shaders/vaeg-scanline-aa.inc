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

// Shared GLSL/C++ math: only the periodic brightness, not image colors.
float vaeg_beam_integral(float distance, float thin) {
    float x = clamp(distance, 0.0f, 0.5f / thin);
    float omega = 6.28318530718f * thin;
    return 0.5f * (x + sin(omega * x) / omega);
}

float vaeg_scan_point(float phase, float thin) {
    float a = min(0.5f, phase * thin);
    float b = min(0.5f, (1.0f - phase) * thin);
    return 1.0f + 0.5f * (cos(6.28318530718f * a) + cos(6.28318530718f * b));
}

float vaeg_scan_primitive(float position, float thin) {
    float periods = floor(position);
    float phase = position - periods;
    float half_mean = vaeg_beam_integral(1.0f, thin);
    return periods * (2.0f * half_mean) + vaeg_beam_integral(phase, thin)
           + half_mean - vaeg_beam_integral(1.0f - phase, thin);
}

float vaeg_scan_average(float phase, float thin, float footprint) {
    float mean = 2.0f * vaeg_beam_integral(1.0f, thin);
    // Fade before Nyquist: tone/gamma curves generate additional harmonics.
    // At two or fewer output pixels per source row, omit the periodic gaps.
    if (footprint >= 0.5f) return mean;
    if (footprint <= 0.001f) return vaeg_scan_point(phase, thin);
    float half_width = 0.5f * footprint;
    float average = (vaeg_scan_primitive(phase + half_width, thin)
                     - vaeg_scan_primitive(phase - half_width, thin)) / footprint;
    // Four or more output pixels per row retain the box-filtered scanlines.
    float fade = smoothstep(0.25f, 0.5f, footprint);
    return mix(max(average, 0.0f), mean, fade);
}
