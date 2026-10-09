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

# M99r — Harden D3D11 lifecycle

Status: PASS (lifecycle handling and cross-build); Windows runtime deferred

## Lifecycle behavior

The D3D11 presenter now enters `Filtered` when initialized with a valid
filter request and accepts both filtered and pass-through states for present,
resize, and toggle operations. It reads the current HWND client area before
each frame, recreates swap-chain output resources after a size change, and
treats zero-sized/minimized windows as temporarily disabled.

`DXGI_STATUS_OCCLUDED` is a non-fatal no-output result. Device-removed,
device-reset, and driver-internal-error results from resize, upload, or
presentation are reported separately as device loss; the presenter moves to
`Unavailable` and exposes `recover()` to tear down and recreate the device,
swap chain, output resources, source texture, and optional librashader chain.
Recovery retains the bounded preset path and filter choice. Filter history is
restarted after recovery or re-enabling the filter. Teardown is idempotent and
frees the filter chain before the D3D11 device.

## Verification

```text
x86_64-w64-mingw32-g++ -std=c++17 -DSDL_VIDEO_DRIVER_WINDOWS=1 \
  -I/opt/local/x86_64-w64-mingw32/include -I/opt/local/include/SDL2 \
  -I. -I./sdl2 -Iexternal/librashader/include \
  -c sdl2/librashader/backends/d3d11_bridge.cpp -o /tmp/vaeg-m99r-d3d11_bridge.o
x86_64-w64-mingw32-g++ -std=c++17 -DSDL_VIDEO_DRIVER_WINDOWS=1 \
  -I/opt/local/x86_64-w64-mingw32/include -I/opt/local/include/SDL2 \
  -I. -I./sdl2 -Iexternal/librashader/include \
  -c sdl2/librashader/d3d11_presenter.cpp -o /tmp/vaeg-m99r-d3d11_presenter.o
cmake --build --preset mingw-cross
```

Result: PASS. Both backend translation units and the complete optional-feature
PE32+ build succeeded. No Windows runtime or D3D11 GPU is available on the
current host, so actual device-loss injection, occlusion, HiDPI/fullscreen
pixels, and hardware recovery remain deferred to G99-3.
