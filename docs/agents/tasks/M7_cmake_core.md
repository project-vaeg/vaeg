# M7 — CMake bootstrap + portable PC-98 core canary (Linux)

Status: not-started
Branch: topic/m7-cmake-core
Gate: G7 (machine + user review; no hardware gate)

## Goal

A top-level CMake build that compiles the plain NP2 (PC-98) core as
static libraries with gcc AND clang on Linux, using zero assembly.
No executable, no SDL yet. This creates the environment in which every
later milestone is self-verifiable by the agent.

## Context

The SDL1 Makefiles (`sdl/Makefile.win`, `sdl/Makefile.zau`) are the
authoritative record of which files constitute an assembly-free NP2
build: root core files, `common/`, `generic/`, `io/`, `sound/`,
`cbus/`, `vram/`, `fdd/`, `bios/`, `lio/`, `font/`, and `i286c/` as the
CPU core. Use them as the source-list oracle; do not invent a file list.

`compiler.h` is per-frontend. Three exist: `win9x/` (Win32/x86, defines
`OPNGENX86`), `sdl/win32s/` (SDL1 Win32/x86, also `OPNGENX86`),
`sdl/slzaurus/` (SDL1 ARM — the pure-C reference). Create
`sdl2/compiler.h` for gcc/clang, starting from `sdl/slzaurus/compiler.h`:
fixed-width types from `<stdint.h>`, endianness via compiler macros,
NO `OPNGENX86`, NO x86-specific defines. This header is the single
portability switchboard; concentrate #ifdef decisions here, not in core
files.

## Constraints

- Zero modifications to core .c/.h files if at all possible. Where a
  file will not compile on gcc/clang, the fix must be minimal,
  behavior-preserving, and listed one-by-one in the PR description.
  If a fix would change behavior, STOP and report instead.
- CMake >= 3.20, explicit source lists, no GLOB.
- Targets: `vaeg_common` (common/ generic/), `vaeg_core` (root core +
  io/ sound/ cbus/ vram/ fdd/ bios/ lio/ font/ + i286c/). Link order
  and include dirs mirror the Makefile.zau layout.
- C99. Warnings on (-Wall), -Werror OFF (option VAEG_WERROR).
- Do not touch win9x/, sdl/, or any VA directory in this milestone.
- Add `cmake --preset linux-debug` / `linux-release`
  (CMakePresets.json, Ninja if available, else make).

## Steps

1. Extract the object list from sdl/Makefile.zau into
   docs/agents/reports/m7_source_list.md (file, target lib, origin).
2. Write sdl2/compiler.h (new file, copyright header per CONVENTIONS).
3. Top-level CMakeLists.txt + CMakePresets.json + the two library
   targets.
4. Build with gcc and clang; capture full logs.
5. Produce a warning inventory (count by -W category per directory) in
   docs/agents/reports/m7_warnings.md. Do NOT fix warnings beyond what
   blocks compilation.

## Machine checks (paste into PR)

    cmake --preset linux-debug && cmake --build build/linux-debug
    CC=clang CXX=clang++ cmake -B build/clang ... && cmake --build build/clang
    python3 tools/repo/check_encoding.py
    python3 tools/repo/check_case.py

## Do not do

- No executable target, no SDL2 code, no VA code, no ImGui.
- No warning-fix sweeps, no reformatting, no header reorganization.
- Do not delete or modify the Makefiles or the .dsp/.vcxproj files.

## Risks to report

- Core files that hard-assume Win32 even in the "portable" set
  (list them; do not shim silently).
- Endianness or alignment assumptions found while writing compiler.h.
- Any file where gcc and clang disagree.
