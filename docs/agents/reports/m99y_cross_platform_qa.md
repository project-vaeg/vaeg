# M99y — Cross-platform QA matrix

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
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Status: PARTIAL — automated matrix passed; physical GPU gates remain deferred.

## Linux container matrix

Colima was running with Docker on macOS Virtualization Framework, using an
arm64 Debian Bookworm container. The source was exported from the evaluated
candidate commit and mounted read-only; the one uncommitted Linux portability
fix was overlaid explicitly for the build input.

```text
cmake -S /src -B /tmp/vaeg-m99-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_ENABLE_LIBRASHADER=ON \
  -DVAEG_ENABLE_ARCHIVE_DROP=OFF
cmake --build /tmp/vaeg-m99-linux --parallel 2
ctest --test-dir /tmp/vaeg-m99-linux --output-on-failure \
  -R '^(vaeg_librashader_|vaeg_romless_tests$)'
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  /tmp/vaeg-m99-linux/sdl2/vaeg --selftest
```

Results:

- feature-on Linux build: PASS;
- focused M99/ROM-less CTest: 7/7 PASS;
- full executable selftest: PASS (exit status 0);
- no usable physical Linux GPU or display was exposed by Colima, so this is
  not G99-4 hardware evidence.

The first full-container CTest attempt was intentionally not used as a pass:
the minimal image omitted `git`, causing ten history-aware repository tests to
fail, and it also exposed the pre-existing ROM-less M74 harness requirement.
After adding `git`, the focused M99/ROM-less matrix passed. The M99 report-name
failure found in that first run was corrected by the rename-only commit
`M99y: normalize M99 report names`; the test suite now uses canonical report
basenames.

## macOS and Windows

The macOS feature-on frontend build and focused lifecycle/fallback tests passed
before M99y. The host has no usable Cocoa display in this execution context,
so Metal presentation, Retina resize, fullscreen, minimize/restore, and
resource-recreation evidence remains deferred.

No Windows host or D3D11 hardware is available in this environment. The
Windows-specific source and package checks are present in CI, but real
pass-through/CRT rendering, HiDPI, fullscreen, occlusion, device recovery,
and performance evidence remains deferred to a Windows host.

No CI result is claimed until the run for the final M99y commit completes.

## Gate status

| Gate | Status | Evidence boundary |
| --- | --- | --- |
| G99-1 | PASS from prior clean-main audit | rewritten main and ordinary refs were verified before M99 implementation |
| G99-2 | PASS for static/common and raw-capture scope | no physical GPU claim |
| G99-3 | DEFERRED | Windows/D3D11 hardware unavailable |
| G99-4 | DEFERRED | Colima software/virtual environment is smoke evidence only |
| G99-5 | DEFERRED | macOS Metal display unavailable |
| G99-6 | PASS for staged package inspection | exact runtime files were shape-checked; no release publish performed |
| G99-7 | DEFERRED | no representative real hardware benchmark was available |

M99y therefore records all safe automated work and its evidence limits; it
does not turn virtual-display or software-renderer results into hardware-gate
success.
