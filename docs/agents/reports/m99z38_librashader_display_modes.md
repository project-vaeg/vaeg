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

# M99z38 — Explicit librashader display modes

Status: PASS for source, build, and automated-test evidence.

Starting commit: `588443671ced38e16d9eb1904933736f2d94a6d3`.
Fixing commit: `6253d43325ed872ab3701fad506f81be49deb327`.
Branch: `topic/m99-native-crt-rebuild`.

## Change

`画面 > 描画方式` now exposes exactly these choices:

- `標準（SDL）`
- `CRT効果（librashader）`
- `加工なし（librashader）`

The old `Pass-through（加工なし）` item was removed. The selected native
mode is persisted by `NativeCRT` and `NativeCRTFilter` in `vaeg.cfg`; the
unfiltered mode keeps the native presenter while disabling the filter chain.
The CRT settings window is offered only when the CRT filter is active.

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
encoding, EOL, case, and diff checks PASS. Physical menu interaction was not
executed in the agent environment; the maintainer should verify the three
entries and live switching on the packaged macOS, Windows, and Linux builds.
