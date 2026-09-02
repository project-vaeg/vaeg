<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M99a - Current presentation pipeline inventory

Evaluated baseline: `1e73f2b2c20b09b2ac97c58590014cc413401d5c`

Status: **M99a complete; no production-code changes**

## Verified pipeline

The active frontend is the CMake/SDL2/Dear ImGui target `vaeg_sdl2`. It creates
an SDL window with `SDL_WINDOW_ALLOW_HIGHDPI`, then creates an accelerated
`SDL_Renderer` with a software-renderer fallback. No OpenGL, Vulkan, Metal,
wgpu, or other librashader graphics context is currently created.

The guest framebuffer path is:

```text
VA video composition in vabitmap
  -> vram/sdrawva.c
  -> sdl2/scrnmng.c shadow framebuffer
  -> SDL_UpdateTexture(RGB565)
  -> SDL_RenderCopy to the calculated viewport
  -> existing scanline/CRT-lite overlays
  -> Dear ImGui SDL_Renderer overlay
  -> SDL_RenderPresent
```

The source texture is fixed at 640x400 pixels, RGB565, with a two-byte guard
column in the CPU shadow allocation. The shadow pitch is `(640 + 1) * 2`
bytes, and the SDL texture upload starts after the guard column. Rows are
written top-to-bottom: the VA raster compositor writes increasing y rows and
`sdrawva16` advances by `yalign` for each row. `scrnmng_setheight()` changes
the active source content height and clears the shadow; the SDL texture
allocation remains 640x400. This covers the current 640x200/640x400 content
height cases without changing the texture allocation.

The viewport is calculated from the SDL renderer output size, the SDL window
size, the menu inset, the selected scaling mode, and the aspect setting. SDL
renderer output size is used as the drawable/HiDPI size. Windowed scaling and
fullscreen scaling use the existing `sdl2/viewport.c` policy; fullscreen
transitions recreate neither the SDL renderer nor the texture.

## Capture and headless boundaries

`scrnmng_save_guest_frame()` creates an RGB565 surface directly over the
shadow framebuffer. This is the raw guest-frame capture and is upstream of
SDL presentation effects and Dear ImGui. The debug harness and screenshot
scheduler operate in the guest-frame/run-loop path; deterministic raw data
must remain at this boundary.

`scrnmng_capture_rendered_frame()` reads the SDL renderer output after the
game frame and GUI overlays have been drawn, but before `SDL_RenderPresent()`.
`VAEG_SCREEN_DUMP` and `scrnmng_save_rendered_frame()` use this separate
presented-frame path. On the dummy video driver, rendered capture instead
builds a scaled surface from the shadow framebuffer because renderer
readback is unavailable.

Headless/self-test execution still initializes the SDL frontend with the
existing dummy-video/dummy-audio environment. It must not require a
librashader runtime or a GPU. The normal run loop owns presentation on the
same thread that owns the SDL renderer and Dear ImGui context; no presentation
worker thread exists.

## Context and resource lifecycle

`scrnmng_create()` owns the SDL window, renderer, RGB565 texture, and CPU
shadow allocation. `scrnmng_destroy()` releases them in that order and is
used by normal shutdown and initialization-error cleanup. Dear ImGui is
initialized after the SDL renderer and shut down before that renderer is
destroyed. Fullscreen, resize, display-effect, and scaling changes update
the existing SDL state without changing the emulation framebuffer or raw
capture boundary.

## M99c consequence

The current frontend is not an OpenGL-context frontend. An OpenGL
librashader vertical slice would therefore require a bounded presentation
backend that owns an SDL OpenGL context and the corresponding Dear ImGui
OpenGL renderer, or a deliberate renderer-path replacement. The existing
SDL_Renderer path cannot be assumed to expose a compatible librashader output
target. This is the key architecture constraint for M99c; shader processing
must be inserted after the raw shadow capture and before the final presented
frame, while the existing SDL renderer remains the fallback.

## Evidence commands

```text
rg -n 'scrnmng_(surflock|surfunlock|present_begin|present_end|save_guest_frame|save_rendered_frame)' sdl2 vram machine
nl -ba sdl2/scrnmng.c | sed -n '1066,1161p;1426,1662p'
nl -ba vram/sdrawva.c | sed -n '8,47p'
nl -ba machine/pccore.c | sed -n '401,562p'
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/linux-debug/sdl2/vaeg --selftest
```

The source tree had no top-level `LICENSE` file at this baseline. Existing
license and lineage evidence is recorded in `LICENSES/legacy-vaeg.txt`,
`README.md`, and the relevant `external/*` license files; M99b owns the
librashader-specific records.
