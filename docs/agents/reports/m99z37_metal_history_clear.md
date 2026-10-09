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

# M99z37 — Avoid the empty Metal history pass

Status: PASS for source/build/test evidence; physical Apple GPU confirmation
remains pending.

## Demonstrated cause

The maintainer reproduced the same `MetalFilterError(FailedToCreateCommandBuffer)`
after M99z36 had moved the filter output to an intermediate texture. The
pinned librashader Metal source showed why: when `FrameOptions.clear_history`
is true, its `frame()` implementation creates a render pass even when the
preset has no `OriginalHistory` resources. That descriptor has no color
attachment, so Metal returns no encoder and librashader reports the generic
`FailedToCreateCommandBuffer` error.

The bundled `vaeg_crt_default.slangp` has no `OriginalHistory` or feedback
inputs. Therefore the Metal bridge must not request a first-frame history clear
for this preset.

## Correction

The Metal bridge now sets `frame_options.clear_history` to false for the
bundled history-free CRT preset. The intermediate filter output introduced by
M99z36 remains in place: librashader renders into it, VAEG copies the guest
viewport to the drawable, and native ImGui is rendered afterward.

This is a Metal-side workaround for the pinned runtime behavior. No vendored
librashader source, preset, shader, SDL version, or GUI layout was changed.

## Verification

```text
cmake --build build/macos-static-qa --target vaeg -j2
ctest --test-dir build/macos-static-qa --output-on-failure \
  -R '^(vaeg_static_metal_loader|vaeg_static_preset|vaeg_static_cache|vaeg_librashader_.*|vaeg_romless_tests|vaeg_sdl_startup_viewport)$'
git diff --check
```

Results:

- macOS arm64 static target: PASS.
- Focused CTest suite: 16/16 PASS.
- `git diff --check`: PASS.
- The agent environment has no Metal device/display, so a physical filtered
  frame was not executed. The maintainer should replace the binary and rerun
  `VAEG_NATIVE_CRT=1 ./vaeg --no-cfg`; success means no repeated frame error,
  visible CRT output, and the native menu remaining visible.

Fixing commit: [583eb8a9](https://github.com/nakatamaho/vaeg/commit/583eb8a97a32dc4444c054ec124d71722b7fe96a).
