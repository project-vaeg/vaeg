<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source and binary forms must retain the above copyright notice,
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
# M99o: preserve capture and QA semantics

Raw guest captures remain sourced from the deterministic RGB565 shadow before
all SDL and librashader presentation work. The existing CLI screenshot path
continues to use the rendered capture path after presentation, and the Screen
menu now provides an explicit `Capture Presented Frame` action. Interactive
presented captures use the `vaeg-presented-` filename prefix; existing raw
captures retain their existing `vaeg-` prefix.

The graphics-analysis action remains a raw guest-frame operation. It does not
consume filtered output and therefore does not alter ROM-less golden-image
inputs.

Validation:

```text
cmake --build /private/tmp/m99d-lib-on -j2 --target vaeg_sdl2  PASS
cmake --build /private/tmp/m99d-lib-off -j2 --target vaeg_sdl2 PASS
```
