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

# M99v — Native CRT settings UI

Status: PASS (settings surface and persistent configuration)

The Screen menu now contains a Native CRT (librashader) panel with enable,
preset-path editing, preset reload, parameter sliders, parameter reset, and a
clear-status action. Runtime metadata is displayed only after a successful
preset reload; each parameter update is clamped by the common M99u parameter
model and persisted through its validated state-file path.

The enable flag and preset path are stored in the existing INI configuration as
`NativeCRT` and `NativeCRTPreset`. The default path is the bundled
`assets/shaders/crt/vaeg_crt_default.slangp`. The existing SDL effects and
raw-capture settings are unchanged. GPU presenter activation and failure
fallback are connected at the presentation boundary in M99w; this milestone
does not claim native GPU output.

## Verification

```text
cmake --build /private/tmp/m99u-macos-m99u-build --target vaeg_sdl2 --parallel 4
```

Result: PASS. The macOS development executable linked successfully after the
GUI and configuration changes. The host has no usable Cocoa display, so the
interactive menu and GPU output remain deferred to the platform gates.
