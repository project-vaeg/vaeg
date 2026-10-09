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

# M99f SDL2 frontend inventory

Status: PASS

This inventory records the pre-M99 presentation boundary used by the portable
SDL2 frontend. It is the input to the backend-neutral contracts in M99h.

## Raw guest framebuffer

- `sdl2/scrnmng.c:41-69` owns the presentation state. The deterministic guest
  surface is `scrnmng.shadow`, a 640x400 RGB565 buffer with one two-byte guard
  at the left of each row.
- `sdl2/scrnmng.c:1456-1475` exposes that buffer through
  `scrnmng_surflock()`/`scrnmng_surfunlock()`; `vram/scrndrawva.c:69-110`
  renders the guest image into it. The unlock uploads the raw pixels to the
  SDL texture.
- `sdl2/scrnmng.c:928-938` updates the static RGB565 SDL texture. This is the
  stable raw-frame boundary and is independent of the host window size.

## Existing presentation path

- `sdl2/scrnmng.c:1066-1126` creates a resizable high-DPI SDL window, chooses
  an accelerated SDL renderer with software fallback, creates the 640x400
  RGB565 texture, and allocates the shadow buffer.
- `sdl2/scrnmng.c:949-990` calculates the viewport from the drawable output
  size, menu inset, scaling policy, and aspect setting. Window size and
  drawable size are intentionally queried separately for HiDPI correctness.
- `sdl2/scrnmng.c:1477-1550` clears the renderer, copies the guest texture to
  the viewport, and applies the existing scanline/CRT-lite software overlay.
  `sdl2/scrnmng.c:1552-1566` draws GUI/diagnostic overlays and calls
  `SDL_RenderPresent`.
- `sdl2/np2.c:1546-1553` defines the per-guest-frame presentation sequence:
  GUI frame, raw texture presentation, GUI render, and present. The main loop
  chooses whether to draw at `sdl2/np2.c:1625-1735`, so draw skipping and
  nowait pacing are guest-loop policies, not audio/video callback timing.

## Capture and QA boundaries

- `sdl2/scrnmng.c:126-159` reads the final SDL render target for normal
  captures, with a dummy-video reconstruction path for headless runs.
- `sdl2/scrnmng.c:1568-1590` saves the scaled/effected rendered frame, while
  `sdl2/scrnmng.c:1592-1616` saves the raw guest frame. The latter is the
  shader-independent QA artifact; the former is presentation output.
- `sdl2/np2.c:1556-1564` captures after a completed guest frame, and
  `sdl2/np2.c:1595-1601` preserves the ordering of automation, screenshot,
  and exit checks. `sdl2/README.md:121-134` documents the distinction.
- `VAEG_SCREEN_DUMP` is enabled during `scrnmng_create()` at
  `sdl2/scrnmng.c:1112-1117`; the CLI screenshot path uses the same rendered
  capture API. Existing raw TVRAM and GVRAM diagnostic capture paths remain
  outside this presentation boundary.

## Geometry, lifecycle, and threading assumptions

- SDL window resize events are handled on the main loop in
  `sdl2/taskmng.c:262-272`; the stored window dimensions are configuration
  state, while viewport calculation reads current SDL output dimensions.
- Fullscreen/windowed changes and display effects are managed by the same
  `scrnmng` owner. Destroy/recreate releases the texture, shadow buffer,
  rendered capture surface, renderer, and window in
  `sdl2/scrnmng.c:1137-1160`.
- `sdl2/np2.c:2106-2128` initializes the screen before the GUI and guest core,
  and `sdl2/np2.c:2146-2180` runs the guest/presentation loop before orderly
  shutdown. No existing render thread or GPU command queue is exposed to the
  emulator core; presentation is main-thread driven.
- The current SDL abstraction is `SDL_Renderer`/`SDL_Texture` based. No
  OpenGL, Metal, D3D11, librashader, preset parser, or shader resource is
  present in the clean baseline.

## Build ownership

`CMakeLists.txt:288-325` lists the SDL2 frontend sources, including
`sdl2/scrnmng.c`, `sdl2/np2.c`, and the existing diagnostic/capture helpers.
`CMakeLists.txt:563-573` builds and links the single `vaeg_sdl2` executable
against the core, VA layer, common layer, SDL2, and the existing vendored
ymfm/ImGui sources. M99 must keep raw framebuffer capture below any optional
shader stage and preserve the SDL/software fallback when the optional backend
is unavailable.
