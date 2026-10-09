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

# M99z36 — Metal filter output surface

Status: partial correction; the intermediate output surface remains required by
the librashader contract, but it was not sufficient to resolve the reported
Metal frame error. The follow-up is recorded in M99z37.

## Symptom and cause

The macOS static build initialized the native Metal CRT chain and restored the
native ImGui menu, but every filtered frame reported:

```text
librashader Metal frame rendering failed
MetalFilterError(FailedToCreateCommandBuffer)
```

The VAEG bridge passed the `CAMetalLayer` drawable directly as librashader's
filter output. The pinned librashader revision documents that a filter chain
terminates at a caller-provided output surface and that the caller copies that
surface to the backbuffer. The drawable was therefore being used in a role
that is not the intended output-surface boundary. This was a real API-contract
violation, but the maintainer reproduced the same frame error after the
intermediate surface was added; it was not the sole cause of that error.

## Correction

`metal_bridge.mm` now maintains a reusable intermediate Metal texture matching
the current drawable's size and pixel format. Filter frames are rendered into
that texture. The bridge then clears the drawable, copies only the calculated
guest viewport with a Metal blit encoder, and renders the native ImGui pass on
top. This preserves the black letterbox border, the guest viewport, menu
composition, and the existing pass-through fallback. The intermediate is
recreated when the drawable size or pixel format changes and released with the
other Metal resources.

No vendored librashader source, preset, shader, SDL version, or GUI layout was
changed.

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
- A physical Metal frame could not be executed in this agent environment:
  SDL reported `The video driver did not add any displays`. The maintainer
  subsequently reran the binary and observed the same filter failure, leading
  to the M99z37 correction. Filtered-frame success is not claimed here.

Fixing commit: [b76d635d](https://github.com/nakatamaho/vaeg/commit/b76d635d1246899507afb02c6bc595e323fe1dab).
