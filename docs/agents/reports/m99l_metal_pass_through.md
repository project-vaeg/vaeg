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

# M99l — Metal pass-through

Status: PASS (build and contract evidence); physical GPU presentation deferred

## Scope

The macOS backend now has a native pass-through presenter. SDL2 continues to
own the window and event surface, while the Metal bridge owns the
`CAMetalLayer`, device, command queue, source texture, render pipeline, and
drawable submission.

## Data path

`VAEG_FRAME_INPUT` is validated and converted into a reusable RGBA8 upload
buffer. The source texture is created or recreated only when the input extent
changes. Each frame updates that texture, obtains the current layer drawable,
clears the drawable, and renders a nearest-neighbour full-screen quad through
the VAeg-owned pass-through Metal shader. The viewport is aspect-fit using the
frame's declared source aspect ratio. The layer's `drawableSize` is used for
the viewport, so Retina output is measured in drawable pixels rather than
logical window units.

The factory selects this backend only for `Automatic` or `Metal` when the
optional librashader feature is compiled on Apple platforms. Feature-off
builds retain the unavailable presenter and the existing SDL renderer path.
The frontend has not yet switched ownership of normal presentation; that
lifecycle integration is covered by later M99 milestones.

## Verification

Feature-on macOS build:

```text
cmake -S . -B /tmp/vaeg-m99l-build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=OFF
cmake --build /tmp/vaeg-m99l-build --target vaeg_sdl2 --parallel 4
```

Result: PASS. AppleClang compiled the Objective-C++ Metal bridge and the
native presenter, and linked Metal, Foundation, and QuartzCore. Existing
repository warnings remain unchanged.

Focused frontend tests:

```text
cmake --build /tmp/vaeg-m99l-build --target vaeg_librashader_frame_input_test vaeg_librashader_capture_boundary_test vaeg_librashader_presenter_state_test vaeg_librashader_pass_through_test --parallel 4
ctest --test-dir /tmp/vaeg-m99l-build --output-on-failure -R 'vaeg_librashader_(frame_input|capture_boundary|presenter_state|pass_through)$'
```

Result: PASS, 4/4 tests.

Feature-off macOS build:

```text
cmake -S . -B /tmp/vaeg-m99l-off -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_TESTS=OFF -DVAEG_WERROR=OFF
cmake --build /tmp/vaeg-m99l-off --target vaeg_sdl2 --parallel 4
```

Result: PASS. The optional Metal source and factory path are absent and the
legacy build remains linkable.

No usable physical Metal capture was performed in this environment, so
drawable pixels, resize behavior, and GPU timing remain M99n/G99-5 evidence.
