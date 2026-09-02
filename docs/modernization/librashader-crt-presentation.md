<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source and binary forms must retain the above copyright
   notice, this list of conditions and the following disclaimer.
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
-->
# Optional librashader CRT presentation

VAeg's CRT filter is an optional GPU presentation feature. It is disabled in
feature-off builds and does not alter emulation, the guest framebuffer, or
deterministic raw screenshots. The Recommended preset is the audited
`crt-lottes-fast` pass and provides scanlines, mask simulation, and curvature.

The filter is selected from `Screen -> CRT Filter Settings...`:

- `Off` uses the existing renderer.
- `Recommended` uses VAeg's bundled preset.
- `Custom...` loads a `.slangp` file from an explicitly selected local path.

Curvature can round or crop the corners of the presented image. This is a
presentation effect; it does not change VAeg's intended aspect ratio or the
source 640x200/640x400 framebuffer. User presets and their dependencies are
the user's responsibility. VAeg does not download shaders, and no GPL,
unknown-license, CRT-Geom, CRT-Royale, or Mega Bezel shader is bundled.

When filtering is enabled, VAeg loads the separately distributed
`librashader` OpenGL runtime through the official dynamic loader. A missing
runtime, incompatible ABI/API, unavailable OpenGL context, missing symbol,
preset error, shader compilation error, or filter-chain error leaves the
existing renderer active. The runtime is selected under the MPL-2.0 option;
the pinned source and license records are in
`third_party/librashader/readme.vaeg.md` and
`third_party/librashader/license-mpl-2.0.txt`.

`Capture Presented Frame` records the post-filter presentation for visual
inspection. Existing raw `--screenshot` and guest-frame captures remain
upstream of the shader and are the only captures suitable for deterministic
core/golden comparisons. Raw and presented captures use distinct names.
