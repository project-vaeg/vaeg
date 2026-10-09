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

# M99z35 — Restore the macOS native Metal menu

Status: PASS for build and focused tests; physical Apple GPU confirmation
remains deferred.

## Scope

The macOS native CRT presenter owns the CAMetalLayer drawable, so the SDL
renderer cannot draw the existing Dear ImGui menu over it. The fix keeps SDL2
as the window/event and ImGui platform layer, adds a small VAeg-owned Metal
ImGui render pass, and submits that pass after the guest/filter output.

The SDL-calculated guest viewport is also passed to the Metal bridge. The
guest image remains in its calculated viewport while the menu and overlays are
drawn in the full drawable, matching the existing SDL presentation layout.

## Changed paths

- `sdl2/np2.c`: initialize the existing GUI on macOS when native Metal owns
  presentation.
- `sdl2/librashader/metal_presenter.cpp` and `metal_bridge.h`: connect GUI
  preparation, shutdown, and viewport updates to the native presenter.
- `sdl2/librashader/backends/metal_bridge.mm`: add the Metal ImGui texture,
  buffer, pipeline, scissor, and final-drawable composition path.

## Verification

```text
cmake --build build/macos-static-qa --target vaeg_sdl2 -j4
ctest --test-dir build/macos-static-qa --output-on-failure \
  -R '^(vaeg_static_metal_loader|vaeg_static_preset|vaeg_static_cache|vaeg_librashader_.*|vaeg_romless_tests|vaeg_sdl_startup_viewport)$'
git diff --check
```

Result: the static macOS arm64 target built successfully and the focused suite
passed 16/16. The resulting executable was hashed as:

`48128224331ec5dd84993cb1edff8771fb6f80f026af98a67a31fec660329d32`

No physical Metal window was available to this agent process. The maintainer
must confirm that the menu is visible and that menu interaction remains
usable. The separately observed `FailedToCreateCommandBuffer` filter-chain
diagnostic is not claimed fixed by this menu restoration.

Fixing commits:

- [8855991b](https://github.com/nakatamaho/vaeg/commit/8855991bc0441e717a3037de2c600a2e181d61f0)
  adds the native Metal ImGui pass.
- [6c163c5f](https://github.com/nakatamaho/vaeg/commit/6c163c5ff5afa8f2df687280f2556bbb45c39a41)
  supplies the Metal vertex attribute annotations required by `stage_in`.
