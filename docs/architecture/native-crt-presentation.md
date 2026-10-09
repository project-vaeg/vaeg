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
# Native CRT presentation architecture

This document describes the optional librashader presentation path in the
portable SDL2 frontend. It is downstream of emulation and does not alter the
PC-88VA framebuffer, guest timing, raw captures, or headless execution.

## Data flow and ownership

```text
guest video writes
        |
        v
SDL2 shadow framebuffer (RGB565, deterministic source)
        |
        +--> guest-frame/raw capture and golden comparison
        |
        +--> VAEG_FRAME_INPUT --> one selected native presenter
                                      |
                    +-----------------+------------------+
                    |                 |                  |
                 Metal             D3D11              OpenGL
              (macOS)           (Windows)             (Linux)
                    |                 |                  |
                    +---- pass-through or librashader filter
                                      |
                                      v
                                 native window
```

`VAEG_FRAME_INPUT` is the common C-facing contract. It carries the borrowed
pixel pointer, dimensions, pitch, RGB565 or ARGB8888 format, row origin,
source aspect, nominal frame rate, frame number, and frame delta. It contains
no SDL renderer, OpenGL, Metal, D3D11, or librashader object. Each backend
owns its own window binding, context/device, textures, upload storage,
filter-chain object, and teardown code.

When Native CRT is selected, the native presenter owns the window. The SDL
renderer and SDL texture are not created for the normal path. If native
initialization or later presentation cannot continue, the presenter is
destroyed and the existing SDL renderer is created from the same shadow
framebuffer. A filter error first attempts to disable the filter while keeping
the native pass-through path; a device or resource error falls back to SDL.

The backend selection is fixed at compile time and automatic at runtime:

| Host | Presenter | Native API |
| --- | --- | --- |
| macOS | `MetalPresenter` | Metal and `CAMetalLayer` |
| Windows | `D3D11Presenter` | D3D11, DXGI, and the application `HWND` |
| Linux | `GLPresenter` | OpenGL 3.3 core context |

No backend type crosses the `NativePresenter` interface. The emulation core
does not include this interface or the librashader headers.

## Lifecycle

Initialization creates the host-native presentation objects, then loads the
configured `.slangp` through librashader's audited dynamic C loader when the
filter is enabled. Preset metadata is enumerated once during initialization;
the preset is not parsed on every frame. Backend resources are allocated once
and recreated only when a source or drawable size changes, a context/device
is lost, or recovery is requested.

For each frame, the frontend updates the drawable size, submits the borrowed
shadow framebuffer through the common contract, and presents. The caller and
the backend bridge run on the frontend presentation thread; there is no
background GPU worker or cross-backend resource sharing. The GUI changes
parameters on that same frontend side and saves the small parameter state
file; selecting Native CRT itself takes effect after restart because native
window ownership is established during screen creation.

The state machine is:

```text
unavailable -> initializing -> pass-through <-> filtered
       ^             |                 |
       +-------------+-----------------+
             shutdown or recovery
```

A zero-sized drawable is treated as temporarily having no output. Minimize,
nil drawable, or an unavailable presentation surface does not feed invalid
dimensions into the filter. Recovery reconstructs the backend using the
remembered window, drawable size, preset, and filter state.

## Capture boundary

Deterministic capture remains upstream:

```text
emulation framebuffer -> guest-frame/raw capture -> golden comparison
```

`--screenshot` and TVRAM diagnostics are therefore suitable for reproducible
QA. The post-scale `VAEG_SCREEN_DUMP`/rendered capture path is not available
while Native CRT owns the output; the frontend emits one warning and directs
the user to guest-frame capture. The native path has no byte-exact filtered
capture contract, because output pixels can vary by GPU, driver, and shader
implementation.

## Optional runtime and failure policy

VAEG links no librashader object code. The official `librashader_ld.h` loader
loads the platform runtime at startup/use time and checks its ABI. The
expected names are `librashader.dylib`, `librashader.dll`, and
`librashader.so` on macOS, Windows, and Linux respectively. A missing runtime,
ABI mismatch, missing preset, shader failure, unavailable GPU, or lost device
does not make CRT presentation mandatory: VAEG retains the established SDL
renderer.

The default preset is relative to the process working directory:
`assets/shaders/crt/vaeg_crt_default.slangp`. Release packages keep that
relative path and place an optional matching runtime beside the executable.
The exact package and license rules are in
[`ADR-0014`](../agents/DECISIONS/ADR-0014-librashader-crt.md) and the
[CRT user guide](../modernization/native-crt-user-guide.md).

## GUI composition and remaining boundary

On Windows, M99z6 attaches the official same-version ImGui D3D11 renderer to
the native presenter's device. SDL2 still handles GUI input; ImGui draw data
is composed after filtering and before DXGI Present. The common presenter
contract only exposes GUI lifecycle calls and a pixel viewport. SDL and native
Windows output use the same viewport calculator and logical menu inset.
Filter toggle calls retain the D3D11 device and filter chain. M99z7's explicit
`CRT shader > SDL / librashader` renderer selector instead changes ownership
between GUI frames, as requested by the maintainer; it is not a filter toggle.
Preset reload is queued
until the current GUI frame finishes, then tears down GUI GPU objects before
replacing presentation resources. Device recovery recreates GUI resources on
the next frame; a total native failure detaches the native GUI before creating
the SDL renderer and its GUI. About textures use ImGui's managed texture data.

macOS/Metal and Linux/OpenGL still defer the GUI during native presentation.
Those platform GUI integrations and their physical GPU gates remain pending;
the Windows correction does not establish cross-platform UI completion.
