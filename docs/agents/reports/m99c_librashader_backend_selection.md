<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M99c: initial librashader backend selection

## Decision

The first optional GPU backend is librashader OpenGL 3.3+. The adapter is
limited to the final presentation layer: VAeg continues to compose the guest
framebuffer and produce the existing raw capture, while the optional path
uploads that framebuffer to an OpenGL source texture, runs one persistent
librashader filter chain, and draws the resulting output texture to the SDL
window's default framebuffer. The existing SDL_Renderer path remains the
feature-off and failure fallback.

This is a bounded presentation adapter, not a replacement for the emulator,
SDL window/event handling, guest rendering, or deterministic capture path. The
adapter owns its OpenGL context, textures, output framebuffer, and final
present quad. It is created only for the optional feature-on presentation mode
and can be discarded to return to the existing renderer.

The pinned C API requires an output image texture: a zero OpenGL handle is
converted to no texture and cannot form the librashader output framebuffer.
Therefore the adapter uses a persistent destination texture sized to the
drawable viewport, then copies that texture to the default framebuffer. This
keeps the librashader output target explicit and avoids relying on undocumented
SDL_Renderer internals.

## Comparison

| Candidate | Existing SDL/video compatibility | Platform viability | Scope and maintenance | Decision |
| --- | --- | --- | --- | --- |
| OpenGL 3.3+ | SDL can create a GL context on the existing window; the adapter can share the established presentation boundary | Windows, Linux, and macOS have supported OpenGL 3.3-class paths, subject to the host driver | One context, source/output textures, and a final quad; librashader has an official GL C API and loader | **Selected** |
| wgpu | Would require a new surface, adapter, device, queue, swapchain, shader-format, and ImGui rendering integration alongside the existing renderer | Broad modern GPU coverage, but greater backend/package variation than the first slice needs | Large frontend and dependency expansion; no existing VAeg wgpu ownership boundary | Rejected for M99 first slice |
| Vulkan | SDL surface and device/swapchain synchronization would be new code | Good on some Windows/Linux hosts; macOS requires an additional portability layer | Highest initial lifecycle and packaging cost; not a bounded adapter for the current SDL_Renderer path | Rejected for M99 first slice |
| D3D11 / Metal | Native API integration would be platform-specific and would duplicate the presentation adapter | Strong on one platform each, but not one cross-platform first backend | Separate resource/lifecycle implementations and packaging rules | Rejected for M99 first slice |

The current SDL_Renderer does not promise an OpenGL context or a texture
handle usable by librashader. The selected design consequently does not
inspect or depend on SDL_Renderer internals. It switches presentation ownership
at the window boundary and restores the existing path whenever the optional
context, runtime, preset, or filter chain is unavailable. Feature-off builds do
not compile or link this adapter.

## Gate result

M99a proved the shader insertion point is after the raw framebuffer and before
the final window presentation. M99b established a permissive, pinned input
closure and the MPL-2.0 runtime boundary. M99c selects exactly one backend and
rejects alternatives on evidence from the current SDL frontend and the pinned
librashader API. No dependency or shader is vendored before G99-1.
