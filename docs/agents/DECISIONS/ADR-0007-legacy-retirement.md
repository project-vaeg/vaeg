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
# ADR-0007: M13 Legacy Retirement Boundary

Date: 2026-07-07

Status: Accepted

## Context

M13 reviewed the remaining legacy paths after G7-G12 passed. The
candidate inventory and evidence are recorded in
`docs/agents/reports/m13_retirement_candidates.md`.

The active product is now the CMake/C/SDL2/Dear ImGui tree. CI builds and
smokes that portable tree on Linux, Windows MinGW, and macOS. The CI
workflow does not build Visual Studio projects, invoke NASM, or compile
the frozen v141 reference.

The v141 reference nevertheless had high diagnostic value during G9. It
was decisive in the VA defect chain: differential FDC traces compared the
same tree across legacy and portable builds, and the legacy V30
`dmap_i286` pump cadence identified the missing portable behavior.

## Decision

Delete the retired SDL1 frontend:

- `sdl/`, including `sdl/sdlw32s.dsp` and `sdl/sdlw32s.dsw`.
- The stale `sdl` include-directory entry in `CMakeLists.txt`.

Delete leftover accessories Visual Studio project metadata:

- `accessories/bin2txt.dsp`
- `accessories/bin2txt.dsw`
- `accessories/lzxpack.dsp`
- `accessories/lzxpack.dsw`

Keep the local v141 behavioral reference frozen:

- `win9x/`, including its project files and NASM helpers.
- `i286x/`.
- `cpuxva/memoryva.x86`.
- `hlp/`, paired with the frozen Win32 reference and kept as the CP932
  HTML Help exception.

The active header `cpuxva/memoryva.h` remains part of the portable build
and is not a retirement candidate.

## Consequences

- Normal development targets the active CMake/C/SDL2 tree only.
- Frozen reference files are reference-only. Do not refactor or improve
  them unless a future task explicitly changes the reference tier.
- The old "legacy build must keep compiling" rule is retired. The frozen
  tier is protected by tags and source history, not by CI.
- Keeping `win9x/`, `i286x/`, and `cpuxva/memoryva.x86` preserves the
  same-tree debugging path that proved useful in G9, at the cost of
  retaining unbuilt Visual Studio/NASM reference code.
- Keeping `hlp/` preserves the CP932 encoding exception because it is
  coupled to the frozen Win32 HTML Help payload.

