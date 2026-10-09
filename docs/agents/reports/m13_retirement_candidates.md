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

# M13 Step 1: Legacy Retirement Candidates

Scope: proposal only. Nothing is deleted in this step.

Base tree: `4f5216e` (`Merge branch 'topic/m12-ci'`). Gates G7-G12 are
reported passed by the maintainer.

## Evidence Summary

Portable CMake is built from explicit source lists:

- `CMakeLists.txt:103-116` uses `i286c/` as the active CPU core.
- `CMakeLists.txt:240-269` uses the portable VA sources, including
  `cpucva/memoryva.c` and `cpucva/z80c.cpp`.
- `CMakeLists.txt:271-294` uses `sdl2/` and Dear ImGui for the active
  frontend.

The current CMake source lists do not compile:

- any source from `sdl/`;
- any source from `win9x/`;
- any source from `i286x/`;
- `cpuxva/memoryva.x86`;
- any file from `hlp/`;
- any Visual Studio project file.

Two residual include-directory references need attention if deletion is
approved:

- `CMakeLists.txt:67` still lists `sdl` as an include directory, but no
  `sdl/` source is compiled. Search evidence: the only active
  `fontmng.h` consumers are `font/fontmake.c` and `sdl2/fontmng.c`;
  `sdl2` precedes `sdl` in the include path, so this appears to be
  stale.
- `CMakeLists.txt:84` lists `cpuxva` as an include directory because
  active portable sources include `memoryva.h`; the deletion candidate
  here is only `cpuxva/memoryva.x86`, not `cpuxva/memoryva.h`.

CI evidence:

- GitHub Actions run
  <https://github.com/nakatamaho/vaeg/actions/runs/28843762398>
  completed successfully on `main` at
  `4f5216e7be46d626058b370440f71f0e314f6fc4`.
- Successful jobs: `repo invariants`, `ubuntu gcc`, `ubuntu clang`,
  `ubuntu asan`, `windows msys2 mingw64`, and `macos fetch-sdl2`.
- The workflow is portable-only: CMake configure/build/smoke/ctest on
  Linux, Windows MinGW, and macOS. It does not invoke Visual Studio,
  v141, NASM, `.dsp`, `.dsw`, `.sln`, `.vcproj`, or `.vcxproj`.

Legacy CI protection today: none. Under either keep option below, the
legacy build remains unprotected unless a new Visual Studio/NASM CI job
is added. Under a delete option, there is no remaining legacy build to
protect, and the existing portable CI continues to cover the active
product.

## Candidate: `sdl/`

Tracked inventory: 55 files, about 400 KiB.

Evidence:

- `sdl/` is the retired SDL1 frontend. `AGENTS.md` states that the
  active frontend is `sdl2/` and that `sdl/` is historical porting
  source material retired in M13.
- No `sdl/` source file is compiled by the active CMake target.
- CI is green with only the `sdl2/` executable built and smoked.

Deletion cost:

- Removes an old SDL1 reference implementation and its `sdlw32s`
  Visual Studio project files.
- Requires deleting the stale CMake include-directory entry
  `CMakeLists.txt:67` in the execution phase.
- Behavioral risk is low because the active product, CI, and human
  gates have moved to `sdl2/`.

Recommendation for approval: delete `sdl/`, including
`sdl/sdlw32s.dsp` and `sdl/sdlw32s.dsw`.

## Decision Point: `win9x/`

Tracked inventory: 133 files, about 1.3 MiB.

Contained legacy NASM helpers:

- `win9x/dclockd.x86`
- `win9x/x86/cputype.x86`
- `win9x/x86/makegrph.x86`
- `win9x/x86/opngeng.x86`
- `win9x/x86/parts.x86`

Legacy project files inside `win9x/`:

- `win9x/np2.dsp`
- `win9x/np2.dsw`
- `win9x/np2.sln`
- `win9x/np2.vcproj`
- `win9x/np2.vcxproj`
- `win9x/np2_v141.sln`

Evidence:

- No `win9x/` source is compiled by CMake.
- The M12 CI workflow does not build or smoke `win9x/`.
- The `win9x/np2.vcxproj` custom build still names all nine legacy NASM
  steps, including the five `win9x/` helpers plus
  `i286x/{dmap,egcmem,memory}.x86` and `cpuxva/memoryva.x86`.

### Option A: delete `win9x/`

Concrete benefits:

- Removes the frozen Win32 frontend, resource scripts, dialogs, debug
  windows, DirectDraw-era host layer, and all `win9x/` project files.
- Removes five `win9x/` NASM helper files.
- Eliminates the largest remaining legacy-only frontend tree.
- Simplifies future path, EOL, and Visual Studio metadata handling.

Concrete costs:

- Deletes the same-tree v141 reference frontend.
- Removes the easiest local route for differential A/B debugging
  against the legacy behavior.
- Requires deleting or separately deciding `hlp/` because HTML Help is
  coupled to the legacy Windows frontend.
- Current CI gives no confidence about `win9x/` either way; deletion
  relies on the already-passed G7-G12 portable gates and the
  maintainer's M13 decision, not on a legacy CI job.

### Option B: keep `win9x/` frozen

Concrete benefits:

- Preserves the v141 build as an executable behavioral oracle.
- Keeps the exact Win32 UI/resource/help integration available for
  archaeology.
- Keeps the ability to perform same-tree A/B debugging without checking
  out an old tag.

Concrete costs:

- Legacy remains unprotected by CI unless a new VS2017/NASM job is
  introduced.
- Future source changes can silently rot the legacy project because no
  machine gate compiles it.
- The repository keeps CRLF project exceptions and legacy host code that
  new contributors may mistake for an active target.

## Decision Point: `i286x/` and `cpuxva/memoryva.x86`

Tracked inventory:

- `i286x/`: 21 files, about 360 KiB.
- `cpuxva/memoryva.x86`: one file; `cpuxva/` also contains
  `memoryva.h`, which remains active and is not proposed here.

Legacy NASM files in this group:

- `i286x/dmap.x86`
- `i286x/egcmem.x86`
- `i286x/memory.x86`
- `cpuxva/memoryva.x86`

Evidence:

- CMake compiles `i286c/`, not `i286x/`.
- CMake compiles `cpucva/memoryva.c`, not `cpuxva/memoryva.x86`.
- Active portable sources still include `memoryva.h`, so deletion must
  not remove `cpuxva/memoryva.h`.

Important phase-2 fact: the frozen v141 reference was decisive in
resolving the G9 defect chain. The investigation used differential FDC
traces from the same tree, confirmed the v141 behavior, and identified
the legacy V30 path's `dmap_i286` pump cadence as the missing portable
behavior. That A/B comparison would have been materially harder without
the local v141 build and the original `i286x/`/`memoryva.x86`
reference.

### Option A: delete `i286x/` and `cpuxva/memoryva.x86`

Concrete benefits:

- Removes x86/NASM CPU and VA memory implementations that are no longer
  compiled by the portable build.
- Makes `i286c/` plus `cpucva/memoryva.c` the single maintained CPU and
  VA memory path.
- Reduces the chance of future fixes landing only in dead assembly.

Concrete costs:

- Deletes the primary source-level behavior reference used during M9.
- Makes future CPU/VA regressions rely on tags, external logs, or
  historical reports instead of same-tree source comparison.
- If `win9x/` is kept, deleting these files breaks the legacy v141
  project, because its NASM custom build steps reference them.

CI protection:

- Existing CI does not compile the legacy assembly today.
- Deletion is protected only indirectly by portable CI staying green.

### Option B: keep `i286x/` and `cpuxva/memoryva.x86` as frozen references

Concrete benefits:

- Preserves the exact assembly reference used for M9 transliteration and
  debugging.
- Keeps the v141 build viable if `win9x/` is also kept.
- Provides a local comparison target for future CPU, DMA, and VA memory
  defects.

Concrete costs:

- No CI validates that the frozen reference still builds.
- Maintainers must keep treating the tree as reference-only; fixes
  belong in `i286c/` and `cpucva/memoryva.c`.
- The repository keeps NASM/x86-only source even though the active
  product is CMake/C/SDL2.

## Candidate: leftover Visual Studio project files

Tracked inventory:

- `accessories/bin2txt.dsp`
- `accessories/bin2txt.dsw`
- `accessories/lzxpack.dsp`
- `accessories/lzxpack.dsw`
- `sdl/sdlw32s.dsp`
- `sdl/sdlw32s.dsw`
- `win9x/np2.dsp`
- `win9x/np2.dsw`
- `win9x/np2.sln`
- `win9x/np2.vcproj`
- `win9x/np2.vcxproj`
- `win9x/np2_v141.sln`

Evidence:

- No Visual Studio project file is referenced by CMake or GitHub
  Actions.
- The active CI is entirely CMake-based.

Deletion rule:

- `sdl/sdlw32s.*` should go with `sdl/` if `sdl/` is approved.
- `win9x/np2.*` and `win9x/np2_v141.sln` should go with `win9x/` if
  `win9x/` is approved.
- `accessories/*.dsp` and `accessories/*.dsw` are independent leftover
  VS project metadata. The source tools in `accessories/` can remain
  unless separately approved for deletion.

Recommendation for approval: delete leftover project metadata once the
corresponding tree decision is made. If `win9x/` is kept, keep its
project files too; otherwise the frozen reference is not buildable.

## Candidate: `hlp/`

Tracked inventory: 96 files, about 616 KiB.

Evidence:

- `hlp/` is not referenced by CMake or portable CI.
- `hlp/` is the only permanent CP932 text exception in the current
  invariant checks; CI explicitly runs
  `python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/`.
- The portable SDL2/ImGui frontend does not build or ship this HTML
  Help tree.

Deletion rule:

- If `win9x/` is deleted, `hlp/` is a natural deletion candidate too:
  the Windows HTML Help integration becomes dead, and the CP932
  exception can be removed in the execution phase.
- If `win9x/` is kept as a frozen reference, keep `hlp/` unless the
  maintainer explicitly accepts losing the matching legacy help payload.

## Approval Questions for Phase 2

1. Delete `sdl/`?
2. For `win9x/`: delete it, or keep it frozen as a local v141 reference?
3. For `i286x/` plus `cpuxva/memoryva.x86`: delete them, or keep them
   frozen as behavior references?
4. If `win9x/` is deleted, also delete `hlp/`?
5. Delete leftover Visual Studio project metadata, including the
   independent `accessories/*.dsp` and `accessories/*.dsw` files?

