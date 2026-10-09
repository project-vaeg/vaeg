#version 450
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

// VAeg implementation of the public-domain CRTS/Lottes display model.
// Reference: crt-lottes-fast.slang by Timothy Lottes, RetroArch adaptation
// by hunterk. That audited original and its Unlicense notice remain unchanged.
// Keep its color/kernel/mask/warp parameter semantics; replace only periodic
// scanline brightness with a pixel-footprint average. No additional image taps.
#pragma parameter MASK "Mask Type" 1.0 0.0 3.0 1.0
#pragma parameter MASK_INTENSITY "Mask Intensity" 0.5 0.0 1.0 0.05
#pragma parameter SCANLINE_THINNESS "Scanline Intensity" 0.5 0.0 1.0 0.1
#pragma parameter SCAN_BLUR "Sharpness" 2.5 1.0 3.0 0.1
#pragma parameter CURVATURE "Curvature" 0.02 0.0 0.25 0.01
#pragma parameter TRINITRON_CURVE "Trinitron-style Curve" 0.0 0.0 1.0 1.0
#pragma parameter CORNER "Corner Round" 3.0 0.0 11.0 1.0
#pragma parameter CRT_GAMMA "CRT Gamma" 2.4 0.0 51.0 0.1
layout(push_constant) uniform Push {
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float MASK;
    float MASK_INTENSITY;
    float SCANLINE_THINNESS;
    float SCAN_BLUR;
    float CURVATURE;
    float TRINITRON_CURVE;
    float CORNER;
    float CRT_GAMMA;
} params;
layout(std140, set = 0, binding = 0) uniform UBO { mat4 MVP; } global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
void main() {
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
#include "vaeg-scanline-aa.inc"

vec3 vaeg_decode(vec3 color) {
    return mix(pow((abs(color) + 0.055) / 1.055, vec3(params.CRT_GAMMA)),
               color / 12.92, lessThanEqual(color, vec3(0.04045)));
}
vec3 vaeg_encode(vec3 color) {
    return mix(1.055 * pow(max(color, vec3(0.0)), vec3(0.41666)) - 0.055,
               color * 12.92, lessThan(color, vec3(0.0031308)));
}
vec3 vaeg_mask(vec2 pixel, float dark) {
    if (params.MASK == 0.0) return vec3(1.0);
    vec3 mask = params.MASK == 1.0 ? vec3(1.0) : vec3(dark);
    float stripe = params.MASK == 3.0
        ? fract((pixel.x + pixel.y * 2.9999) / 6.0) : fract(pixel.x / 3.0);
    // D3DCompile rejects a dynamically indexed vector l-value after HLSL
    // translation (X3500). Constant component writes are equivalent and portable.
    float selected = params.MASK == 1.0 ? dark : 1.0;
    if (stripe < 1.0 / 3.0) mask.r = selected;
    else if (stripe < 2.0 / 3.0) mask.g = selected;
    else mask.b = selected;
    return mask;
}

void main() {
    // OutputSize can describe the whole render target, not the inset viewport.
    // Anchor the mask to raster pixel centers (SV_Position on D3D11) so that
    // letterboxing/resize cannot compress its stripes into a beating pattern.
    vec2 pixel = gl_FragCoord.xy;
    vec2 screen = vTexCoord * 2.0 - 1.0;
    vec2 warp = vec2(params.CURVATURE * (1.0 - params.TRINITRON_CURVE),
                     params.CURVATURE * 0.75);
    screen *= 1.0 + screen.yx * screen.yx * warp;
    vec2 position = (screen + 1.0) * 0.5 * params.SourceSize.xy;
    // Evaluate derivatives before any conditional: includes both curvature axes.
    float footprint = fwidth(position.y);
    vec2 base = vec2(floor(position.x - 1.5) + 0.5,
                     floor(position.y - 0.5) + 0.5);
    float phase = position.y - base.y;
    float thin = 0.5 + 0.5 * params.SCANLINE_THINNESS;
    float upper = 0.5 + 0.5 * cos(6.28318530718 * min(0.5, phase * thin));
    float lower = 0.5 + 0.5 * cos(6.28318530718 * min(0.5, (1.0 - phase) * thin));
    float brightness = vaeg_scan_average(phase, thin, footprint);
    float sum = upper + lower;
    // Retain the original relative row-color weights, not a wider image blur.
    // At an exact zero of both beams, the symmetric limiting blend is 1/2.
    vec2 rows = sum > 0.000001 ? vec2(upper, lower) / sum : vec2(0.5);
    vec3 color = vec3(0.0);
    float total = 0.0;
    for (int x = 0; x < 4; ++x) {
        float distance = position.x - (base.x + float(x));
        float weight = exp2(-params.SCAN_BLUR * distance * distance);
        vec2 uv = (base + vec2(float(x), 0.0)) * params.SourceSize.zw;
        vec3 a = vaeg_decode(texture(Source, uv, -16.0).rgb);
        vec3 b = vaeg_decode(texture(Source, uv + vec2(0.0, params.SourceSize.w), -16.0).rgb);
        color += (a * rows.x + b * rows.y) * weight;
        total += weight;
    }
    float edge = 1.0 - (1.0 - clamp(screen.x * screen.x, 0.0, 1.0))
                          * (1.0 - clamp(screen.y * screen.y, 0.0, 1.0));
    float vignette = clamp(params.SourceSize.y
        * (1.0 - edge * (0.998 + 0.001 * params.CORNER)), 0.0, 1.0);
    color *= (brightness * vignette / total);
    float dark = 1.0 - params.MASK_INTENSITY;
    color *= vaeg_mask(pixel, dark);
    // Same Lottes mid-grey tone mapping; contrast=1, saturation=0.
    float exposure_mask = params.MASK == 0.0 ? 1.0
        : (params.MASK == 1.0 ? 0.5 + 0.5 * dark : dark);
    float mid = 0.18 / ((1.5 - thin) * (0.5 * exposure_mask + 0.5));
    float slope = (mid - 0.18) / (0.82 * mid);
    float intercept = (0.18 - 0.18 * mid) / (0.82 * mid);
    float peak = max(max(color.r, max(color.g, color.b)), 1.0 / 16777216.0);
    color /= peak * slope + intercept;
    FragColor = vec4(vaeg_encode(color), 1.0);
}
