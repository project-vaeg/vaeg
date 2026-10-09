<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# ADR-0002: Dear ImGui Rendering Backend

Date: 2026-07-04

Status: Accepted

Decision maker: maintainer pre-decision for M10.

## Context

M10 adds a Dear ImGui GUI to the SDL2 frontend. The current SDL2 screen
manager creates an `SDL_Renderer` and an `SDL_Texture`, locks that
texture for guest pixels, then presents through `SDL_RenderCopy` and
`SDL_RenderPresent` (`sdl2/scrnmng.c:68`, `sdl2/scrnmng.c:75`,
`sdl2/scrnmng.c:140`, `sdl2/scrnmng.c:160`). The GUI renderer must fit
that pipeline without adding an unrelated graphics stack.

## Decision

Use Dear ImGui's SDL2 platform backend plus SDL_Renderer2 renderer
backend:

- `imgui_impl_sdl2`
- `imgui_impl_sdlrenderer2`

Require SDL >= 2.0.18 for the M10 ImGui frontend.

The renderer shares the existing `scrnmng` SDL renderer/texture path. No
OpenGL backend and no OpenGL loader dependency are introduced in M10.
`imgui_freetype` is explicitly deferred as a later opt-in.

## Consequences

M99z6 extends this decision for Windows native CRT presentation: the SDL2
platform backend continues to own input, while the same-version official
ImGui D3D11 backend draws the UI on the native presenter's device after the
guest filter and before Present. SDLRenderer2 remains the feature-off and
SDL fallback UI backend. Common presenter interfaces expose lifecycle and
pixel viewport operations, never native GPU objects. Logical font size remains
16 pixels, and drawable scaling and the menu inset match the SDL path.

- M10 implementation must expose or route the existing SDL window and
  renderer to the GUI layer without replacing the guest texture pipeline.
- MinGW and macOS builds avoid GL loader selection and driver-specific GL
  context setup in this milestone.
- HiDPI behavior is constrained by SDL_Renderer and Dear ImGui's
  SDL_Renderer2 backend. Any later switch to OpenGL or another renderer
  requires a new ADR.
- Font rasterization uses Dear ImGui's default stb path in M10. Enabling
  `imgui_freetype` later requires an explicit opt-in change.
