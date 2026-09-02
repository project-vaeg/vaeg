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
# M99s: verify the platform build matrix

The existing feature-off Linux, Windows-MinGW, and macOS jobs remain in the
workflow. Separate feature-on jobs now configure the same platform presets
with `VAEG_ENABLE_LIBRASHADER=ON`, build the SDL2 target, run the dummy-video
smoke test before the runtime is present, build the pinned OpenGL-only MPL
runtime, run the smoke test again with that runtime on the loader search path,
and run CTest.

The runtime helper checks out librashader commit
`87e8a97b50516d997defeaa168173dcd185d4022` and builds
`librashader.so`, `librashader.dll`, or `librashader.dylib` from the pinned
Cargo lockfile with `--no-default-features --features runtime-opengl`.

Local evidence before this commit:

```text
cmake --build /private/tmp/m99d-lib-on -j2 --target vaeg_sdl2   PASS
cmake --build /private/tmp/m99d-lib-off -j2 --target vaeg_sdl2  PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  /private/tmp/m99d-lib-on/sdl2/vaeg --selftest                 PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  /private/tmp/m99d-lib-off/sdl2/vaeg --selftest                PASS
```

The local macOS host has no usable SDL display/OpenGL context, and Rust/Cargo
is not installed locally. Therefore local runtime-present and physical
resize/fullscreen checks are deferred to the platform workflow and the later
GPU gate. Dummy-video startup does not prove CRT rendering.
