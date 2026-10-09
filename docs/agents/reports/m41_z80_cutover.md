<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M41 Z80 final cutover evidence

## Starting state

- Approved starting SHA:
  `52f2ff6919948c28353f2f8a95fb250a3b165dcf`
- Starting branch before creating the work branch:
  `topic/m40-z80-disassembler`
- Work branch: `topic/m41-z80-cutover`
- Initial `git status --short`:

```text
?? "PC-Engine 1.05.d88"
?? docs/modernization/PCEPAT.DOC
?? docs/tekumani/
?? his.txt
```

Those four pre-existing paths were not opened, hashed, copied, renamed,
deleted, staged, or used. All private M41 assets, screenshots, logs, traces,
state files, and writable media copies remained outside Git. Tracked private
evidence uses neutral identifiers only.

## Commits

| SHA | Subject |
|---|---|
| `b708472d1b2f210e70b8ec4a2f947334d608acb2` | `M41: rename retained Z80 trace reference runner` |
| `a0a4fcf0238020c24cbc37df2a105fbb8ccf8adb` | `M41: make the replacement Z80 core unconditional` |
| `b559868ddf74bdc0c399d55b4fe1b89dd151583c` | `M41: remove approved legacy Z80 sources` |
| `0e625c49ab624f7f1da92aa7ab8343844cb3bc40` | `M41: enforce single-core CI and archive audits` |
| `5c855dd3c2823fc480f4ec12ca68902fa401beb3` | `M41: document the final Z80 production design` |
| `753ab46592aa7e19534cb9e357c1a7e7d8feaaf6` | `M41: record local and private cutover evidence` |
| `149bff9a76eaed78e7164ebe97b58f9d1bf5b8c6` | `M41: record hosted cutover validation` |
| `c0a124da042cfffa3b5b1453b7a02d184b77f258` | `M41: include Z80 license in runtime packages` |

The ending and remote SHA are reported in the final G41 handoff.

## Production cutover

`VAEG_Z80_CORE` and all choice validation are removed. There is no user-facing
alias, hidden build fallback, or runtime switch. CMake always reports:

```text
Production Z80 core: suzukiplan
```

The production `vaeg_va` Z80 sources are:

```text
cpucva/z80_compat_cpu.cpp
cpucva/z80_compat_state.cpp
cpucva/upd780_disasm.cpp
```

with the independently authored public contracts in `z80_core.h`,
`z80_bus.h`, `z80_registers.h`, `z80_legacy_state.h`, and `z80_disasm.h`, plus
the pinned vendored `external/suzukiplan-z80/z80.hpp`. `iova/subsystem.cpp`
uses only the vaeg-facing interfaces. The BSD-2-Clause M40 disassembler is the
only production Z80 disassembler.

Permanent tests retain the default and `Z80_NO_FUNCTIONAL` header/interrupt/
wrapper configurations, revision-1 fixtures, raw and wrapper ZEX, exhaustive
disassembly, and deterministic new-core regression traces. The old/new
differential evidence remains documented; targets requiring the deleted core
were replaced by repeatable `reference` and `repeat` executions of the current
wrapper.

## Approved deletions

All seven files were deleted in
`b559868ddf74bdc0c399d55b4fe1b89dd151583c`:

| Path | HEAD | Source archive | Runtime archive |
|---|---|---|---|
| `cpucva/types.h` | absent | absent | absent |
| `cpucva/z80.h` | absent | absent | absent |
| `cpucva/z80if.h` | absent | absent | absent |
| `cpucva/z80c.h` | absent | absent | absent |
| `cpucva/z80c.cpp` | absent | absent | absent |
| `cpucva/z80diag.h` | absent | absent | absent |
| `cpucva/z80diag.cpp` | absent | absent | absent |

No other source file was deleted. Historical filename references remain only
in ADRs, tasks, reports, the migration master, frozen-reference attribution,
and the archive checker's forbidden-path list. No active source include,
CMake source list, CI selector, or packaging input refers to them.

Changed tracked paths are `.github/workflows/build.yml`,
`.github/workflows/release.yml`, `AGENTS.md`,
`CHANGES.20260713.md`, `CMakeLists.txt`, `README.md`, `assets/NOTICE.md`,
`dist/readme-dist.txt`, the M41/ADR/roadmap/master and modernization documents,
`iova/subsystem.cpp`, the archive checker, retained Z80 trace sources/scripts,
the revision-1 fixture target, and the subsystem integration test. The only
rename is `tests/z80_compat/differential/legacy_trace.cpp` to
`tests/z80_compat/differential/reference_trace.cpp`.

## API and state preservation

The consumer-visible `Z80C` class retains constructor/destructor, `Init`,
`Exec`, `Reset`, `IRQ`, `NMI`, `Wait`, `GetStatusSize`, `SaveStatus`,
`LoadStatus`, `GetPC`, `SetPC`, and `GetReg`. Removed legacy-only APIs include
`GetDiag`, `TestIntr`, `GetWaits`, `IsIntr`, diagnostic dump/statistics
controls, M88 integer aliases, and legacy helper macros.

Revision-1 remains 68 bytes with verified offsets AF 0, PC 44, WAIT 57,
revision 59, `remainclock` 60, and `lastclock` 64. All 15 retained M34
fixtures load. Architectural AF is authoritative; HALT PC translation,
external WAIT, external level IRQ, EI inhibition in WAIT bit 2,
`remainclock`, and `lastclock` retain the M37 mapping. Unsupported revision
failure continues through `subsystem_loadcpustatus()` and `statsave_load()`.

## Private focused regression

Fresh M41 evidence was generated because the prior M39 evidence had been
deleted. The final unconditional build passed:

- VA2/VA3 OS boot to date entry and command prompt;
- directory and known-file reads without retry or timeout;
- a guest write and exact readback on an expendable disk copy;
- the timing-sensitive loader to its stable `Ready` destination;
- VA SLEEP_HACK at live/public PC `1734/1732`, fixed-PC WAIT draining, ATN
  release, and resumed execution;
- Sorcerian SLEEP_HACK at live/public PC `7010/700e`, fixed-PC WAIT draining,
  ATN release, and resumed execution;
- fresh same-build-family legacy revision-1 save to final-build load, followed
  by a successful FDD read;
- final-build save/load followed by normal guest/FDD operation;
- an FDD-active frame-boundary save: reload restored a mid-loader `Read`
  display and completed exactly once at `Ready` without timeout or replay;
- a live 16-instruction `subsystem_disassemble_bounded()` sequence in which
  every next PC advanced and live/public PC stayed unchanged before and after.

The diagnostic sleep capture had 21 port-`0xf4` writes. Raw private evidence
remains outside Git. No guest-visible failure, transfer corruption, stuck
WAIT, SLEEP_HACK failure, or disassembly desynchronization occurred.

## Local public validation

### Repository invariants

```sh
python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check
```

Results: 0 encoding violations, 0 EOL violations, 0 case findings, the known
69-file unreferenced report, and clean diff whitespace.

### Linux GCC

```sh
cmake --preset linux-ci-gcc -B build/m41-final-gcc \
  -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m40-zex
cmake --build build/m41-final-gcc --parallel 4
ctest --test-dir build/m41-final-gcc --output-on-failure --parallel 4
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m41-final-gcc/sdl2/vaeg --smoke
```

Result: configure and 270-step build passed; 19/19 CTest passed. The four ZEX
tests passed, including raw and wrapper ZEXDOC/ZEXALL. ROM-less smoke passed.

### Linux Clang

```sh
cmake --preset linux-ci-clang -B build/m41-final-clang \
  -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m40-zex
cmake --build build/m41-final-clang --parallel 4
ctest --test-dir build/m41-final-clang --output-on-failure --parallel 4
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m41-final-clang/sdl2/vaeg --smoke
```

Result: configure and 270-step build passed; 19/19 CTest passed, including all
four ZEX tests. ROM-less smoke passed.

### ASan/UBSan

```sh
cmake --preset linux-ci-asan -B build/m41-final-asan
cmake --build build/m41-final-asan --parallel 4
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 \
  ctest --test-dir build/m41-final-asan --output-on-failure --parallel 4
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 \
  build/m41-final-asan/sdl2/vaeg --smoke
```

Result: build and the hosted-CI-equivalent 15/15 tests passed. Smoke exited 0
with the documented shared-core UBSan backlog. An additional non-CI experiment
configured ZEX under sanitizers: all ordinary tests passed, but the four much
slower ZEX processes reached their fixed 600-second runner timeout and exposed
two signed-left-shift reports in immutable vendored `z80.hpp` lines 3000 and
3885. Normal GCC, Clang, and Wine raw/wrapper ZEX all pass. The vendor was not
edited; the combined sanitizer-plus-ZEX timeout is a known limitation rather
than a product or standard ASan-matrix failure.

### MinGW cross and Wine

```sh
cmake --preset mingw-cross -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_WINDOWS_CONSOLE=ON \
  -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m40-zex
cmake --build --preset mingw-cross --parallel 4
```

Result: cross-configuration and production/test link passed. Under Wine 8.0,
the four header configurations, both interrupt configurations, both wrapper
configurations, 15-fixture revision-1 target, 3,844-case disassembler,
directed/state/generated/convergence regression suites, raw and wrapper
ZEXDOC/ZEXALL, `vaeg.exe --selftest`, and `vaeg.exe --smoke` all passed. This
is Wine execution, not native Windows.

### Source and release archives

```sh
git archive --format=tar.gz --output=build/vaeg-m41-source.tar.gz HEAD
python3 tests/z80_compat/check_zex_archive.py build/vaeg-m41-source.tar.gz
tar -C build/m41-release -czf build/vaeg-m41-linux-x86_64.tar.gz \
  vaeg-m41-local-linux-x86_64
python3 tests/z80_compat/check_zex_archive.py \
  build/vaeg-m41-linux-x86_64.tar.gz
tar -tzf build/vaeg-m41-linux-x86_64.tar.gz
```

Result: the ending source archive checked 992 files and the
release-workflow-equivalent runtime archive checked six files; both contained
no deleted path, copied
legacy hash, ZEX input, private media, patch work tree, or unrecorded external
source. The runtime archive contains the byte-identical upstream MIT text as
`licenses/suzukiplan-z80.txt`. A negative exact-path/copy fixture and a neutral
`.d88` fixture were both rejected by the checker.

### Required source audit

The exact required commands were run:

```sh
git grep -n -i 'copyright.*cisc'
git grep -n -i 'm88'
git grep -n 'cpucva/z80c'
git grep -n 'cpucva/z80diag'
git grep -n 'cpucva/z80if'
git grep -n 'cpucva/types.h'
git grep -n 'VAEG_Z80_CORE'
```

All hits are historical ADR/task/report/master text, explicit current removal
notes, frozen-reference attribution, or forbidden archive-check inputs. There
is no active source include, CMake source, CI selection, or package input hit.

## Licensing and provenance

The G40-to-M41 diff of `external/suzukiplan-z80/` and the approved M35 patch
is empty. Verified hashes are:

```text
LICENSE.txt  ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1
z80.hpp      88c878f0087f114eb864dc6a9e8cb98473c022e052c62937ab7a23aecbdb7106
M35 patch    d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4
```

The approved base remains
`e3926769a790fab0af1c34a5540e317f8d4f0ddc`, tested resulting commit
`b4a0a5a238fecc280781e6fe5719faf0eafcd667`, and resulting tree
`8a606eb39332a6e79b69bb62d9dedca042b923dc`. The vendored code retains MIT;
the wrapper, codec, and disassembler retain BSD-2-Clause. Documentation says
the removed files were removed from current HEAD, not relicensed. Git history
was not rewritten. Linux, Windows, and macOS release staging copies the
unchanged upstream MIT text to `licenses/suzukiplan-z80.txt`; its runtime-copy
SHA-256 remains
`ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1`.

## Platform status and risks

Verified facts:

- local GCC, Clang, ASan/UBSan CI-equivalent, MinGW cross-build, Wine, public
  ROM-less, ZEX, state, disassembly, archive, and focused private tests pass;
- no core selector or legacy source fallback remains;
- state size and fixture compatibility are unchanged;
- vendored bytes and the approved M35 patch are unchanged.

Known limitations:

- the optional sanitizer-plus-ZEX combination is too slow for the runner's
  600-second wall timeout and emits the two immutable-vendor signed-shift
  reports described above;
- the active SDL frontend has no interactive Z80 debugger, so the private
  check uses the live production C seam under GDB;
- the diagnostic FDD stream has no dedicated DRQ-edge record; final data,
  command completion, IRQ/acknowledge, and guest-visible behavior provide the
  recorded evidence.

Hosted Actions run
[`29454125225`](https://github.com/nakatamaho/vaeg/actions/runs/29454125225)
passed all seven jobs on
`753ab46592aa7e19534cb9e357c1a7e7d8feaaf6`: repository invariants, Ubuntu
GCC, Ubuntu Clang, Ubuntu ASan, standalone Z80 conformance, native Windows
MSYS2/MinGW64, and native macOS arm64. Local MinGW/Wine remains classified as
non-native evidence. There is no unresolved architectural, bus, interrupt,
state, WAIT, SLEEP_HACK, FDD, or disassembly finding. Optional M42 performance
work was not started.

## Gate

**G41 PASSED.** The final implementation, local/hosted public validation,
private focused regression, state compatibility, licensing, source audit, and
release audit all pass. M42 has not started.
