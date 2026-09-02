<!--
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
-->
# M99f: VAeg-owned pass-through preset

The first preset is a minimal VAeg-owned BSD-2-Clause pair:

- `assets/shaders/crt/vaeg_crt_passthrough.slangp`;
- `assets/shaders/crt/shaders/vaeg_crt_passthrough.slang`; and
- `assets/shaders/crt/licenses/VAeg_PASSTHROUGH_LICENSE.txt`.

It uses one nearest-filtered viewport pass, samples only the `Source` image,
and writes RGB with an opaque alpha. The vertex stage follows the same
coordinate convention as the audited candidate: librashader supplies the MVP,
and the source texture coordinates are passed without a vertical flip. The
preset is loaded and compiled during filter-chain creation; it is not parsed or
compiled in the frame loop.

This pass is the fallback-owned presentation test and is independent of the
audited third-party CRT shader. No third-party shader or dependency is copied
into this directory.
