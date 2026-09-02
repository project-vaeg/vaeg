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
# M99l: add the CRT filter settings dialog

The Screen menu now opens `CRT Filter Settings...`. The dialog provides an
enable checkbox, the initial Off/Recommended preset selector, runtime-generated
parameter controls, range and step displays, reset-to-defaults, Apply, and
Close. Applying parameter values calls `gl_filter_chain_set_param`, so it does
not rebuild the shader chain.

When the optional runtime, OpenGL bridge, or preset is unavailable, the dialog
remains usable and reports that filtering is unavailable. No universal CRT
controls are synthesized; the controls are limited to metadata returned by
the active preset.

Validation:

```text
cmake --build /private/tmp/m99d-lib-on -j2 --target vaeg_sdl2  PASS
cmake --build /private/tmp/m99d-lib-off -j2 --target vaeg_sdl2 PASS
```
