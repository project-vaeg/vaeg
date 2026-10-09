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
# ADR-0004: Dear ImGui Vendored Release

Date: 2026-07-04

Status: Accepted

## Context

M10 requires Dear ImGui vendored at a pinned stable release under
`external/imgui`. Branches such as `master` and `docking` are not used.
ADR-0002 selects the SDL2 platform backend and SDL_Renderer2 renderer
backend.

## Decision

Vendor Dear ImGui `v1.92.8`, the latest GitHub stable release observed
for M10 on 2026-07-04.

| Field | Value |
|---|---|
| Name | Dear ImGui |
| Version | `v1.92.8` |
| Upstream URL | `https://github.com/ocornut/imgui` |
| Release URL | `https://github.com/ocornut/imgui/releases/tag/v1.92.8` |
| Tag SHA | `8936b58fe26e8c3da834b8f60b06511d537b4c63` |
| Source tarball SHA-256 | `fecb33d33930e12ff53a34064e9d3a06c8f7c3e04408f14cd36c80e3faac863b` |
| Vendored path | `external/imgui` |
| License file | `external/imgui/LICENSE.txt` |

Only the files needed by the M10 SDL2 renderer path are vendored:

- Core: `imgui.cpp`, `imgui.h`, `imgui_draw.cpp`, `imgui_internal.h`,
  `imgui_tables.cpp`, `imgui_widgets.cpp`, `imconfig.h`,
  `imstb_rectpack.h`, `imstb_textedit.h`, `imstb_truetype.h`.
- Backends: `backends/imgui_impl_sdl2.*`,
  `backends/imgui_impl_sdlrenderer2.*`.
- License: `LICENSE.txt`.

The demo source, alternate renderer backends, docking branch code,
FreeType integration, and examples are intentionally not vendored.

## Consequences

M99z6 adds the unmodified D3D11 backend from the same pinned commit
`8936b58fe26e8c3da834b8f60b06511d537b4c63`. This is an expansion of the
vendored file subset, not an ImGui version change. It renders the Windows
menu after CRT processing on the presenter's own D3D11 device and context.
The MIT license remains unchanged.

| Added file under `external/imgui/backends/` | SHA-256 |
| --- | --- |
| `imgui_impl_dx11.cpp` | `1c0c3af25b45dceb4c45de7da591244f4935718154e7149d0e90858958c6f19d` |
| `imgui_impl_dx11.h` | `fb8a314a6a2904dcc5bfeb938a169389e170f233ad367d33f1ce5c7bde586a7c` |

- Vendored files under `external/imgui` are third-party code and should
  not be hand-edited.
- Upgrading Dear ImGui requires replacing the vendored files from a new
  release tag and updating this ADR.
