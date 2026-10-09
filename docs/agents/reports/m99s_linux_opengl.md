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

# M99s — Establish the Linux OpenGL backend

Status: PASS (Linux build and focused tests); real OpenGL runtime deferred

## Implementation

The Linux feature-on path now selects an OpenGL presenter for `Automatic` or
explicit `OpenGL` backend selection. It creates an SDL OpenGL 3.3 core context
and loads every GL entry point used by the backend through
`SDL_GL_GetProcAddress`; no direct libGL symbol dependency is added.

The bridge owns the context, shader program, VAO, source texture, upload
buffer, and default-framebuffer presentation. It uses the shared RGB565 and
ARGB8888-to-RGBA8888 conversion contract, reuses the texture and upload
storage for stable extents, and recomputes the drawable-pixel aspect-fit
viewport on every frame. The current framebuffer, viewport, texture/program/
VAO bindings, unpack alignment, clear color, and blend/cull/depth/scissor
enable states are saved and restored around the pass-through draw. Drawable
size zero is a non-fatal disabled result.

The common loader header now selects the OpenGL librashader declarations on
Linux for the subsequent filtered-rendering milestone. The M99s presenter
intentionally rejects filter enablement until M99t adds the OpenGL librashader
chain.

## Verification

The new bridge and presenter compiled with the local SDL2/OpenGL headers. A
Linux Docker build using Debian Bookworm arm64 compiled both new translation
units through the normal CMake source list:

```text
cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_WERROR=OFF -DVAEG_FETCH_LIBARCHIVE=OFF
cmake --build /build --parallel 4
ctest --test-dir /build --output-on-failure \
  -R 'vaeg_librashader_(frame_input|capture_boundary|presenter_state|pass_through)$'
```

Result: PASS. The Linux build produced `/build/sdl2/vaeg`; all four focused
librashader contract tests passed. The container did not provide a real X11/
Wayland display or a physical OpenGL GPU, so context creation, pass-through
pixels, HiDPI/fullscreen behavior, and hardware evidence remain deferred to
G99-4. Mesa software rendering will not be counted as the hardware gate.
