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

# M99q — Integrate librashader D3D11

Status: PASS (dynamic C API integration and cross-build); Windows runtime deferred

## Implementation

The D3D11 bridge now selects the pinned librashader D3D11 declarations through
the repository loader header and calls `librashader_load_instance()` at filter
initialization. The bridge verifies that the runtime and the D3D11 filter-chain
entry point loaded, creates the preset from the selected path, and creates the
filter chain with the active D3D11 device. The chain is freed before the device
resources during teardown.

Filtered frames use the same uploaded RGBA8888 source shader-resource view as
pass-through frames and send the current render-target view, aspect-fit
viewport, and frame metadata to `d3d11_filter_chain_frame`. Filter history is
cleared on the first frame and when filtering is re-enabled. Disabling the
filter keeps the D3D11 device and reusable source resources and returns to the
existing pass-through path. Preset path, backend, and filter state are retained
for recovery.

The librashader integration remains optional and dynamically loaded. No
librashader object code is linked into VAeg, and a missing runtime, ABI
mismatch, missing symbol, preset error, or filter-chain error causes the
presenter to return to the existing fallback path.

## Verification

The native Windows bridge and presenter compiled directly with MinGW-w64, and
the complete optional-feature cross build succeeded:

```text
x86_64-w64-mingw32-g++ -std=c++17 -DSDL_VIDEO_DRIVER_WINDOWS=1 \
  -I/opt/local/x86_64-w64-mingw32/include -I/opt/local/include/SDL2 \
  -I. -I./sdl2 -Iexternal/librashader/include \
  -c sdl2/librashader/backends/d3d11_bridge.cpp -o /tmp/vaeg-m99q-d3d11_bridge.o
x86_64-w64-mingw32-g++ -std=c++17 -DSDL_VIDEO_DRIVER_WINDOWS=1 \
  -I/opt/local/x86_64-w64-mingw32/include -I/opt/local/include/SDL2 \
  -I. -I./sdl2 -Iexternal/librashader/include \
  -c sdl2/librashader/d3d11_presenter.cpp -o /tmp/vaeg-m99q-d3d11_presenter.o
cmake --build --preset mingw-cross
```

Result: PASS. The final cross-build artifact is
`build/mingw-cross/sdl2/vaeg.exe` (PE32+ x86-64). The host has no Windows
runtime or D3D11 GPU, so preset compilation, filtered pixels, toggle behavior,
device recovery, and hardware performance remain deferred to G99-3.
