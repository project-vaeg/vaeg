<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# Default CRT shader provenance

The bundled default is a two-pass preset: the VAeg BSD-2-Clause
`vaeg-screen-size.slang` followed by `vaeg-crt-aa.slang`. The latter is a
VAeg implementation of the public-domain CRTS/Lottes display equations,
with newly authored scanline-brightness antialiasing. It references the
audited `crt-lottes-fast.slang` from `libretro/slang-shaders`; that original
is retained byte-for-byte, with its attribution and Unlicense notice intact.
The VAeg additions are BSD-2-Clause, not a relicensing of the original.

The size/copy pass has no includes or external textures. Its BSD notice is
embedded in the shader. It exposes SCREEN_SIZE metadata and copies texels
without interpolation. VAeg's frontend applies integer black padding below
100 percent, or centered cropping above 100 percent, before GPU upload.
No upstream shader code was copied into the size/copy pass.
The CRT AA pass includes only the VAeg-owned `vaeg-scanline-aa.inc`, which
analytically integrates the periodic beam envelope over an output pixel and
fades unresolved modulation before Nyquist. Its row-color proportions,
four-tap horizontal kernel, mask, curvature, gamma and tone equations retain
the Lottes model; the integration does not add image samples or widen the
image reconstruction kernel. Source attribution is also in the new shader.
Both new files carry complete BSD notices. The complete default dependency
closure is one VAeg preset, two VAeg shaders and one VAeg include: no LUTs,
external textures or further includes. The original remains bundled as an
audited reference, not an active pass. These upstream identities describe
that original shader/preset, not the current VAeg default:

- Repository: https://github.com/libretro/slang-shaders
- Audited commit: `4812a82f6c9a11cc8b5a7447040a98c9fc80c00e`
- Preset source path: `crt/crt-lottes-fast.slangp`
- Shader source path: `crt/shaders/crt-lottes-fast.slang`
- Preset SHA-256: `0992238001c519503d9e8e750b37b32d30ff380e74a40287863605ae69f9fff0`
- Shader SHA-256: `576eddc662ac4f77909c0c14dbd5a16ac4164e50c67527fff634316f4441c482`
- Dependency closure: one preset and one shader; no `#include`, LUT, texture,
  or secondary shader dependency.
- License evidence: the shader contains an explicit Unlicense/public-domain
  statement. The corresponding notice is stored beside the preset.

The complete `slang-shaders` repository is not bundled. GPL-licensed or
unidentified shader families are not part of this feature.
