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

# M99k — macOS Metal bridge

Status: PASS

## Scope

M99k establishes the macOS-native bridge boundary needed by the later
presenter stages. The bridge uses SDL2 for the window and event surface, then
creates and owns a CAMetalLayer and the system Metal device through a small
Objective-C++ translation unit. The existing SDL renderer remains the active
presentation path at this stage; no runtime presentation switch is made yet.

## Implementation

- `sdl2/librashader/metal_bridge.h` exposes a C ABI with opaque Metal state.
- `sdl2/librashader/backends/metal_bridge.mm` creates and destroys the SDL
  Metal view, obtains `CAMetalLayer`, selects the system Metal device, and
  updates drawable size.
- `sdl2/librashader/librashader_loader.h` selects the pinned librashader
  Metal loader declarations only for Apple Objective-C++ compilation.
- CMake enables Objective-C++ and links the system Metal and QuartzCore
  frameworks only when the optional librashader feature is enabled.

The bridge deliberately uses `int` in its C ABI instead of the project
`BOOL` type because Objective-C headers define `BOOL` themselves.

## Verification

Command:

```text
cmake -S . -B /tmp/vaeg-m99h-build.quXdyw -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=OFF
cmake --build /tmp/vaeg-m99h-build.quXdyw --target vaeg_sdl2 --parallel 4
```

Result: PASS. AppleClang configured Objective-C++ and compiled
`metal_bridge.mm`; the final `sdl2/vaeg` link completed. The linker emitted
the repository's existing duplicate-static-library and data-alignment
warnings.

Physical Metal presentation and librashader execution remain deferred to
M99l and later platform stages.
