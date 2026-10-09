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

# M99z40 — Scale CRT RGB-mask intensity for small windows

Status: PASS for source, build, automated-test, and repository-check evidence.
Physical visual comparison remains maintainer verification.

Starting commit: `bdc521711b137771c5716390e8de681189c00eb8`.
Fixing commit: `6cf059bb3c57a4bb0726ae37a6ae31d5d0faac94`.
Branch: `topic/m99-native-crt-rebuild`.

## Change

The default CRT presentation now uses a scale-aware RGB-mask profile:

- x1: `MASK_INTENSITY=0.15`
- x2: `MASK_INTENSITY=0.20`
- x3 and larger: `MASK_INTENSITY=0.30`

The profile changes only RGB-mask contrast. It does not blur or resample the
guest framebuffer, alter `SCREEN_SIZE`, or change scanline processing. The
profile is applied at startup, renderer switching, CRT enablement, and window
scale changes. The CRT settings window exposes `RGB mask: Auto by window
scale`; moving the `MASK_INTENSITY` slider disables automatic adjustment, and
Reset re-enables it. The mode is persisted as `NativeCRTAutoMask` in the main
`vaeg.cfg`.

## Verification

```text
cmake --build build/macos-static-qa --target vaeg_sdl2 -j2
ctest --test-dir build/macos-static-qa -R \
  'vaeg_(romless_tests|librashader_(presenter_state|fallback|controller|shader_parameters|frame_input|frame_padding))' \
  --output-on-failure
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

Results: macOS arm64 static executable build PASS; CTest 7/7 PASS; encoding,
EOL, case, and diff checks PASS. The resulting executable is
`build/macos-static-qa/sdl2/vaeg` with SHA-256
`021606048f34c17126b77391d8c2c123c46a27a735f1886413d949ec6fd338b3`.

Physical macOS display comparison at x1, x2 and x3 was not performed in the
agent environment. The maintainer should compare the same scene at those
scales and confirm that x1/x2 have less vertical RGB-mask interference while
x3 retains the intended CRT texture. Custom presets without
`MASK_INTENSITY` are unaffected.
