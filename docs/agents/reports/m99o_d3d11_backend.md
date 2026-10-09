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

# M99o — Windows D3D11 backend

Status: PASS (MinGW cross-compilation evidence); Windows runtime deferred

## Implementation

The Windows backend obtains the native HWND through SDL2's
`SDL_GetWindowWMInfo` API. It owns a hardware D3D11 device and immediate
context, a DXGI flip-discard swap chain, the back-buffer render-target view,
the source texture/SRV, pass-through vertex and pixel shaders, and the point
sampler. The D3D11 device requests feature level 11.1 and retries 11.0 for
systems that reject the multi-level request.

The bridge is backend-local and exposes only an opaque C handle plus
`VAEG_FRAME_INPUT` and scalar lifecycle values. It does not expose D3D11 or
DXGI types through the common presenter contract. The factory selects it only
for `Automatic` or `D3D11` on Windows feature-on builds. No WARP fallback is
used by the product path.

The D3D11 shader source is compiled once during initialization with the system
`d3dcompiler` API. It is not compiled per frame. Source and output resources
are created lazily/recreated only when their dimensions change.

## Verification

The MinGW-w64 compiler supplied by the local toolchain compiled both new
Windows translation units against the Windows SDK headers:

```text
x86_64-w64-mingw32-g++ -std=c++17 -DSDL_VIDEO_DRIVER_WINDOWS=1 -I/opt/local/x86_64-w64-mingw32/include -I/opt/local/include/SDL2 -I. -I./sdl2 -Iexternal/librashader/include -c sdl2/librashader/backends/d3d11_bridge.cpp -o /tmp/vaeg-m99o-d3d11_bridge.o
x86_64-w64-mingw32-g++ -std=c++17 -DSDL_VIDEO_DRIVER_WINDOWS=1 -I/opt/local/x86_64-w64-mingw32/include -I/opt/local/include/SDL2 -I. -I./sdl2 -Iexternal/librashader/include -c sdl2/librashader/d3d11_presenter.cpp -o /tmp/vaeg-m99o-d3d11_presenter.o
```

Result: PASS. The repository's Windows CMake path links the system `d3d11`,
`dxgi`, and `d3dcompiler` libraries when the optional feature is enabled.
The current host has no Windows runtime or D3D11 GPU, so full Windows CMake,
device, swap-chain, and hardware evidence remains deferred to G99-3.
