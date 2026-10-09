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

# M99w — Fallback and headless behavior

Status: PASS (machine-verifiable fallback boundary)

The SDL2 frontend now selects the optional native presenter before creating an
SDL renderer. A successful native selection leaves the native backend as the
owner of the window's GPU output. If the runtime, preset, device, or initial
resource setup is unavailable, startup continues by creating the existing SDL
renderer and GUI path. The dummy SDL video driver is explicitly treated as
headless and bypasses native initialization.

Presentation failures are handled on the presentation thread. A failed native
frame first receives one backend recovery attempt. A librashader filter-frame
failure disables only that filter and retries the frame through the same
backend's pass-through path. Device/resource failures destroy the native
presenter, create the SDL renderer, and present the current raw frame through
the existing path. If the fallback succeeds, the GUI is initialized on the
following frame. No native or SDL backend type crosses the C frame-input
contract.

Raw guest pixels remain in `scrnmng.shadow` and are passed to the native
presenter as a borrowed RGB565 `VAEG_FRAME_INPUT`; no post-shader pixels are
fed back into raw capture. Native output capture is reported unavailable
instead of being mislabeled as a filtered screenshot. Headless runs retain the
existing SDL dummy-renderer behavior and raw/screenshot test boundary.

## Verification

```text
cmake --build /private/tmp/m99u-macos-m99u-build --target vaeg_sdl2 --parallel 4
```

Result: PASS with the optional Metal path enabled. The host has no usable
Cocoa display, so physical Metal failure/recovery behavior remains deferred to
the macOS hardware gate.

```text
cmake -S . -B /private/tmp/m99w-macos-tests \
  -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_ARCHIVE_DROP=OFF
cmake --build /private/tmp/m99w-macos-tests \
  --target vaeg_librashader_fallback_test \
  vaeg_librashader_frame_input_test vaeg_librashader_capture_boundary_test \
  vaeg_librashader_presenter_state_test vaeg_librashader_pass_through_test \
  vaeg_librashader_shader_parameters_test --parallel 4
ctest --test-dir /private/tmp/m99w-macos-tests --output-on-failure \
  -R 'vaeg_librashader_(frame_input|capture_boundary|presenter_state|pass_through|shader_parameters|fallback)$'
```

Result: 6/6 focused tests passed. The feature-off build also linked the full
SDL2 executable, proving that the optional runtime is not required merely to
build or start the existing path. A dummy-video ROM-less smoke was exercised;
the existing local ROM-less smoke returned nonzero before a complete guest
run because its expected VA2 ROM root was absent, so it is not claimed as a
passing smoke result here. The output selected the SDL software renderer,
confirming the headless bypass.

`clang-format` was not available on the host; no unrelated files were
formatted.
