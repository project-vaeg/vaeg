<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
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

# M99z39 — Metal display screenshot readback

Status: PASS for source, build, and automated-test evidence. Physical macOS
PNG capture remains maintainer verification.

Starting commit: `7f4f0615b868f0ec42a4bf50d2e775fedca66c6b`.
Fixing commit: `382a4d2ca2c2691406d334fb959153503378ffbc`.
Branch: `topic/m99-native-crt-rebuild`.

## Change

The native Metal presenter now accepts the existing one-shot display-capture
request used by the screenshot menu. After librashader output and native GUI
rendering, it copies the drawable into a shared Metal texture, waits only for
the requested capture, converts BGRA to top-down RGBA, and completes the
existing PNG path. Normal frames do not perform readback. The OpenGL native
path remains unsupported and continues to report failure rather than silently
saving a different image.

## Verification

```text
cmake --build build/macos-static-qa --target vaeg_sdl2 -j2
ctest --test-dir build/macos-static-qa -R \
  'vaeg_librashader_(presenter_state|fallback|controller|shader_parameters|frame_input|frame_padding)' \
  --output-on-failure
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

Results: macOS arm64 static executable build PASS; focused CTest 6/6 PASS;
encoding, EOL, case, and diff checks PASS. The resulting executable is
`build/macos-static-qa/sdl2/vaeg` with SHA-256
`13601a2cf1d5ff96e3f7c38670be94f13a9a43e1abb0d99a419ac3d88bf3059f`.

The agent environment did not perform a physical GUI screenshot operation, so
maintainer validation should launch the executable with native Metal enabled,
save a normal screenshot in both pass-through and CRT modes, and confirm that
the saved PNG contains the displayed post-shader output and enabled overlays.
