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
# M15 - PC-88VA Active-Tree Invariant

Status: complete

Branch: `topic/m15-pc88va-invariant`

Gate: G15 passed

Depends on: G14 passed

## Goal

Treat PC-88VA support as an unconditional property of the active
CMake/SDL2 build. Before M15, `SUPPORT_PC88VA` was always defined by
CMake but still guarded VA code throughout the active core. M15 folds
that compile-time condition to true and removes the redundant definition.

This is a behavior-preserving cleanup of the configuration that was
already shipped. It does not turn runtime model selection into a
compile-time decision.

## Resulting Invariant

After M15:

- active CMake targets always compile the VA-capable code;
- `SUPPORT_PC88VA` is not defined by `CMakeLists.txt`;
- active source and headers contain no `SUPPORT_PC88VA` conditionals;
- runtime checks using `pccore.model_va`, `PCMODEL_NOTVA`, `PCMODEL_VA1`,
  and `PCMODEL_VA2` remain;
- `VAEG_FIX` and `VAEG_EXT` remain independent and were not folded;
- PC-98, EPSON, PC-9821, VA1, and VA2 runtime behavior remains selected
  by existing state and configuration.

The active target still defines:

```text
USE_I286C
SUPPORT_BMS
SUPPORT_V30ORIGINAL
VAEG_FIX
```

## Scope

The mechanical fold touched 77 active files across:

- root core files and tables;
- `bios/`, `biosva/`, `cbus/`, `common/`, `cpucva/`, and `fdd/`;
- `generic/`, `i286c/`, `io/`, `iova/`, `sound/`, `vram/`, and
  `vramva/`;
- the SDL2 frontend and CMake definition;
- this task record, `docs/agents/ROADMAP.md`, and
  `docs/modernization/virtual-machine-architecture.md`.

The edits retained the branch selected by the active build and removed
only the preprocessor wrappers and inactive alternatives. Existing files
were not globally reindented or reformatted.

## Folding Rules Applied

The implementation used these transformations:

- `#if defined(SUPPORT_PC88VA)` and `#ifdef SUPPORT_PC88VA`: retain the
  true branch;
- `#ifndef SUPPORT_PC88VA` and `#if !defined(SUPPORT_PC88VA)`: discard
  the false-for-active-build branch, retaining `#else` where applicable;
- conditions where `SUPPORT_PC88VA` made an OR expression true: retain
  the true branch;
- `SUPPORT_PC88VA && X`: reduce to `X`;
- whole-file VA wrappers: remove the wrapper without reindenting the
  enclosed source.

Standalone `VAEG_FIX`, standalone `VAEG_EXT`, and runtime model branches
were outside this transformation.

## Main/Release Integration

The original M15 change predated the `Rel.260708` release-identification
commit. Reapplying M15 onto the current main exposed two additional
active guards:

- `sdl2/np2.c`: command-line version banner;
- `sdl2/scrnmng.c`: window title.

Both were folded to the VA branch. The current `Rel.260708` value in
`np2ver.h` was retained while its `SUPPORT_PC88VA` wrapper was removed.
The old `Rel.080608` value from the historical M15 commit was not restored.

## Intentionally Unchanged Areas

The frozen reference tier remains untouched:

- `win9x/`;
- `i286x/`;
- `cpuxva/memoryva.x86`;
- `hlp/`.

Those paths may still define or test `SUPPORT_PC88VA` for behavior
archaeology. Historical reports under `docs/agents/reports/` also retain
their original terminology. Vendored code and
`docs/agents/reports/raw/` were not modified.

The following search is the active-code invariant check:

```sh
rg -n "SUPPORT_PC88VA" . \
  --glob '!win9x/**' \
  --glob '!i286x/**' \
  --glob '!cpuxva/**' \
  --glob '!hlp/**' \
  --glob '!docs/**' \
  --glob '!external/**'
```

Expected result: no output.

## Verification

The dedicated branch was reconstructed on current main and checked with:

```sh
cmake --preset linux-release
cmake --build --preset linux-release
SDL_AUDIODRIVER=dummy ./build/linux-release/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --smoke
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff main --check
```

Results:

- Linux release configure and build passed;
- ROM-less selftest passed;
- headless smoke passed in ROM-less mode;
- encoding and EOL checks passed;
- case checker reported `0 finding(s)`;
- active-code `SUPPORT_PC88VA` search returned no matches;
- frozen-tier diff was empty;
- runtime `pccore.model_va` checks remained present.

Existing compiler warnings from legacy shared code were not changed by
this mechanical milestone.

## Human Gate

G15 passed after the maintainer verified the rebuilt application. The
manual acceptance point was unchanged behavior: V3 boot, the VA demo, OS
boot, simple commands, keyboard input, video, and sound.

## Residual Risk

M15 deliberately removes the ability to compile the active target without
VA support. That configuration was not an active product or CI target.
The frozen v141 reference keeps its own conditional structure and has no
current compile guarantee. Runtime non-VA branches remain in the active
core and were not simplified by this milestone.

## Commit Record

```text
18ac32b M15: fold PC-88VA support invariant in active tree
a77c4af M15: fold post-release VA guards and close milestone
```
