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
# M37 standalone Z80 wrapper and revision-1 codec evidence

## Entry state and scope

The maintainer passed G36 and explicitly authorized M37. The required files
were read in the prescribed order before work. The branch was created as
`topic/m37-z80-wrapper` from exact commit
`f5b25f01eb1fec16ad8755a2b2f3355e5ec93448`.

The initial commands and output were:

```text
$ git status --short
?? "PC-Engine 1.05.d88"
?? docs/modernization/PCEPAT.DOC
?? docs/tekumani/
?? his.txt
$ git rev-parse HEAD
f5b25f01eb1fec16ad8755a2b2f3355e5ec93448
$ git branch --show-current
topic/m37-z80-wrapper
```

The four paths were pre-existing. M37 did not read, modify, stage, delete, or
archive them. No private ROM or disk data was used. Work is limited to the
standalone wrapper, codec, tests, build/CI declarations, and documentation.
M38 differential work was not started.

## Replacement files and public API

The exact independently authored vaeg-side map is:

| File | Responsibility |
|---|---|
| `cpucva/z80_compat_bus.h` | fixed-width memory, I/O, clock, and clock-counter interfaces |
| `cpucva/z80_compat_registers.h` | consumer-visible `Z80Reg` mirror |
| `cpucva/z80_compat_state.h` | revision-1 constants and decoded state value |
| `cpucva/z80_compat_state.cpp` | explicit revision-1 field encoding and decoding |
| `cpucva/z80_compat_cpu.h` | consumer-visible `Z80C` declaration with opaque implementation |
| `cpucva/z80_compat_cpu.cpp` | core adapter, bus callbacks, execution, IRQ/NMI, and state transfer |
| `tests/z80_compat/wrapper.cpp` | deterministic wrapper, mirror, interrupt, and state tests |
| `tests/z80_compat/wrapper_zex_runner.cpp` | CP/M-style ZEX runner through `Z80C` |

All new vaeg-authored files use the repository BSD-2-Clause header. No
M88/cisc comment, declaration layout, helper macro collection, instruction
implementation, or disassembly table was copied. Consumer-visible headers
contain neither an STL nor a `suzukiplan/z80` type.

Retained source-compatible methods are constructor/destructor, `Init`,
`Exec`, `Reset`, `IRQ`, `NMI`, `Wait`, `GetStatusSize`, `SaveStatus`,
`LoadStatus`, `GetPC`, `SetPC`, `GetReg`, and the M37 `GetDiag` placeholder.
The placeholder returns null and exists only until the M40 disassembly bridge
is replaced. M34-authorized unused `TestIntr`, `GetWaits`, `IsIntr`, dump
controls, and statistics were not reproduced.

`GetReg()` returns a wrapper-owned mirror. It is synchronized after `Reset`,
`LoadStatus`, `SetPC`, synchronous NMI, and when `Exec()` returns. During an
I/O callback it deliberately remains at the previous return boundary.
`GetPC()` reads the authoritative third-party PC. Save also exports that live
PC and the authoritative architectural flags.

## Execution, bus, and interrupt mapping

`Exec()` obtains the current 32-bit clock, adds the modulo-32-bit elapsed
value to the signed balance, drains positive credit when external WAIT is
active, or repeatedly requests one complete third-party instruction while
credit remains positive. Fine-grained `consumeClock` callbacks call vaeg's
clock counter, so the last complete instruction may leave zero or negative
debt. The wrapper then updates the public mirror and `lastclock`. It does not
enable the per-instruction callback mode and does not suspend microstate.

Memory addresses use the third-party 16-bit callback type. All normal IN, OUT,
and block-I/O callbacks mask the vaeg-facing port with `port & 0xff`. The
configured acknowledge port remains the distinct virtual port `0x102` and is
read only by the core's acceptance callback. The wrapper sets and clears the
approved level line directly; it does not pre-read, arm, cancel, or speculate.

Focused tests cover DI, EI inhibition, a following instruction that changes
the acknowledge byte, deassert-before-acceptance, persistent assertion, all
eight RST bytes, raw IM0 `0x00`/`0x7f`, CALL, CB, ED, DD, FD, DDCB, FDCB,
IM1, ordered IM2 vector reads, HALT wake, and NMI isolation. Synchronous NMI
copies IFF1 to IFF2, clears IFF1, pushes live PC, charges 11 clocks, and jumps
to `0x0066` without invoking the maskable acknowledge callback.

## Revision-1 state contract

The codec operates on a 68-byte byte array and never casts untrusted input to
the new runtime object. The verified offset map is:

| Field | Offset | Mapping |
|---|---:|---|
| AF, HL, DE, BC | 0, 4, 8, 12 | architectural 16-bit pairs; two legacy padding bytes remain zero |
| IX, IY, SP | 16, 20, 24 | architectural 16-bit values |
| AF', HL', DE', BC' | 28, 32, 36, 40 | alternate pairs |
| PC | 44 | authoritative live PC; translated while halted |
| I, R-low, R7, IM | 48, 49, 50, 51 | explicit control-byte mapping |
| IFF1, IFF2 | 52, 53 | Boolean architectural state |
| IRQ | 56 | wrapper level, restored through `setIRQLine` |
| wait | 57 | HALT bit 0, external WAIT bit 1, `execEI` bit 2 |
| `xf`, revision | 58, 59 | deterministic F bits 3/5; revision 1 |
| `remainclock`, `lastclock` | 60, 64 | explicit little-endian signed 32-bit values |

Compile-time checks require eight-bit bytes, 16/32-bit fixed-width integer
sizes, the current one-byte Boolean family, and a tail ending exactly at byte
68. The 15 M34 fixtures enforce every retained offset. All have bit 2 clear
and import without pending EI inhibition. The ordinary fixture has historical
`xf=0x33` while architectural F supplies `0x20`; its immediate re-save is
intentionally normalized to `0x20`. All other bytes reproduce. This implements
the ADR policy that architectural AF is authoritative and uninitialized `xf`
is not compatibility behavior.

The third-party HALT state uses the architectural next PC. Decode therefore
adds one to the legacy HALT-opcode PC; encode subtracts one only while halted.
The focused HALT fixture and wake continuation pass. HALT, external WAIT, and
EI use independent wait bits.

The EI boundary test saves immediately after EI with an asserted level. Bit 2
is set. A fresh wrapper loads that image, executes the following `INC A`, and
accepts exactly one IRQ only afterward. Baseline and restored machines have
identical memory/I/O events, memory, architectural result, status image, and
remaining-clock debt. Separate tests load and inspect asserted IRQ, deassert
it, wake restored HALT from the restored level, drain restored external WAIT,
and preserve synthetic positive, reachable zero/negative, and nonzero-last
clock fields.

## Build and production isolation

`vaeg_z80_wrapper` and `vaeg_z80_wrapper_no_functional` are standalone static
libraries. Both use C++17 only under the approved `cpucva/` adapter and use
target-local release-oriented vendored-core definitions. The second adds the
target-local `Z80_NO_FUNCTIONAL` definition. The unit suite is compiled and
run against both libraries.

`vaeg_z80_wrapper_zex_runner` links only the no-functional wrapper. It uses
deterministic 64 KiB memory, the M36 CP/M CALL-5 services, a bounded credit
loop, wall timeout, and failure diagnostics. It uses the existing immutable,
hash-verified external ZEX cache policy; no ZEX input is tracked.

`VAEG_CORE_SOURCES` still names `cpucva/z80c.cpp` and not
`cpucva/z80_compat_cpu.cpp`. `iova/subsystem.cpp`, production state save,
SLEEP_HACK constants, the disassembler, frozen reference tier, and all seven
approved deletion-list files are unchanged. Vendored files and the approved
M35 patch have no diff from the G36 starting commit.

## Local commands and results

| Exact command | Result |
|---|---|
| `g++ -std=c++17 -Wall -fsigned-char -DZ80_NO_FUNCTIONAL -DZ80_DISABLE_DEBUG -DZ80_DISABLE_BREAKPOINT -DZ80_DISABLE_NESTCHECK -DZ80_NO_EXCEPTION -DZ80_UNSUPPORT_16BIT_PORT -I. -Iexternal/suzukiplan-z80 cpucva/z80_compat_cpu.cpp cpucva/z80_compat_state.cpp tests/z80_compat/wrapper.cpp -o /tmp/vaeg-z80-wrapper-test && /tmp/vaeg-z80-wrapper-test` | PASS; all 10 grouped wrapper tests |
| same command without `-DZ80_NO_FUNCTIONAL`, output `/tmp/vaeg-z80-wrapper-default` | PASS; all 10 grouped wrapper tests |
| `cmake --preset linux-ci-gcc -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m36-z80.7UocP2/zex-cache` | PASS; all five M36 hashes reverified at configure |
| `cmake --build --preset linux-ci-gcc` | PASS |
| `ctest --test-dir build/linux-ci-gcc --output-on-failure` | PASS, 14/14; raw ZEXDOC 72.32 s, raw ZEXALL 70.10 s, wrapper ZEXDOC 110.82 s, wrapper ZEXALL 110.90 s, ROM-less selftest 1.23 s |
| `./build/linux-ci-gcc/vaeg_z80_wrapper_zex_runner --max-seconds 900 /tmp/vaeg-m36-z80.7UocP2/zex-cache/zexdoc.cim` | PASS; every case `OK`, clean completion, 46,740,000,000 credited/consumed clocks at batch return |
| same command with `zexall.cim` | PASS; every case `OK`, clean completion, 46,740,000,000 clocks at batch return |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-gcc/sdl2/vaeg --smoke` | PASS; exit 0, ROM-less software renderer |
| `cmake --preset linux-ci-clang && cmake --build --preset linux-ci-clang && ctest --test-dir build/linux-ci-clang --output-on-failure` | PASS, 10/10 |
| `cmake --preset linux-ci-asan && cmake --build --preset linux-ci-asan && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan --output-on-failure` | PASS, 10/10 under ASan/UBSan |
| `cmake --preset mingw-cross -DVAEG_ENABLE_TESTS=ON && cmake --build --preset mingw-cross` | PASS; emulator and wrapper PE32+ targets linked |
| `WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m37-wine WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 wine64 build/mingw-cross/vaeg_z80_wrapper_default.exe` | PASS; all grouped tests |
| same environment with `vaeg_z80_wrapper_no_functional_test.exe` | PASS; all grouped tests |
| same Wine environment plus dummy SDL variables and `build/mingw-cross/sdl2/vaeg.exe --smoke` | PASS; exit 0, ROM-less software renderer |
| `python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `python3 tools/repo/find_unreferenced.py` | PASS, 69 existing paths; no M37 path |
| `git diff --check` | PASS |
| `python3 -c 'import pathlib, yaml; [yaml.safe_load(pathlib.Path(p).read_text(encoding="utf-8")) for p in (".github/workflows/build.yml", ".github/workflows/release.yml")]; print("workflow YAML: PASS")'` | PASS |
| `git archive --format=tar.gz --output=/tmp/vaeg-m37-source-d13119a.tar.gz HEAD && python3 tests/z80_compat/check_zex_archive.py /tmp/vaeg-m37-source-d13119a.tar.gz` | PASS; 977 files, no ZEX artifact |
| staged five-file Linux runtime layout, `tar -C /tmp/vaeg-m37-release -czf /tmp/vaeg-m37-linux-x86_64.tar.gz vaeg-m37-linux-x86_64`, then archive checker | PASS; 5 files, no ZEX artifact |

The integrity command
`sha256sum external/suzukiplan-z80/LICENSE.txt external/suzukiplan-z80/z80.hpp external/suzukiplan-z80/test/test-interrupt-extension.cpp docs/agents/reports/m35_suzukiplan_irq_extension.patch`
returned:

```text
ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1  external/suzukiplan-z80/LICENSE.txt
88c878f0087f114eb864dc6a9e8cb98473c022e052c62937ab7a23aecbdb7106  external/suzukiplan-z80/z80.hpp
abe7cd10642c6d13ad0636ef53091cd3c4bf643ecd6f564a22707393e06728c4  external/suzukiplan-z80/test/test-interrupt-extension.cpp
d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4  docs/agents/reports/m35_suzukiplan_irq_extension.patch
```

These are the unchanged G36 hashes. `git diff --exit-code` from the G36 commit
over those paths returned exit 0. A name-status diff over the frozen tier,
seven approved deletion-list files, and `iova/subsystem.cpp` produced no
output.

An initial diagnostic used the nonexistent name
`tools/repo/check_unreferenced.py` and exited 2; the repository-standard
`tools/repo/find_unreferenced.py` above passed. Initial Wine execution omitted
the MinGW runtime `WINEPATH` and exited 53 for missing `libgcc_s_seh-1.dll` and
`libstdc++-6.dll`; the exact supported environment above passed both wrapper
executables and the emulator smoke. Neither diagnostic failure was presented
as a successful platform result.

## Platform classification

- Linux x86-64 under WSL2: native GCC, Clang, and ASan/UBSan compile, link,
  and execution passed.
- Windows x86-64: MinGW-w64 cross-compilation passed; both wrapper
  configurations and the emulator smoke executed under Wine. This is not a
  native MSYS2 result.
- Native Windows/MSYS2 and native macOS were not available locally. They were
  built and executed by the configured hosted jobs below.

## Hosted CI

GitHub Actions build run
[`29386487600`](https://github.com/nakatamaho/vaeg/actions/runs/29386487600)
tested implementation commit
`d13119a479901072236d9d6d9ec6d173995ecc46`. Every configured job completed
successfully:

| Job | Result |
|---|---|
| repo invariants | PASS; encoding, EOL, and case checks |
| ubuntu gcc | PASS; configure, build, ROM-less smoke, and complete non-ZEX CTest |
| ubuntu clang | PASS; configure, build, ROM-less smoke, and complete non-ZEX CTest |
| ubuntu asan | PASS; sanitizer build, smoke, and complete non-ZEX CTest |
| macos fetch-sdl2 | PASS; native Apple configure/build, ROM-less smoke, and complete non-ZEX CTest |
| windows msys2 mingw64 | PASS; native configure/build, ROM-less smoke, complete non-ZEX CTest, and runtime import audit |
| standalone z80 conformance | PASS; verified ZEX acquisition, both wrapper configurations, raw and wrapper ZEXDOC/ZEXALL, and source archive exclusion |

This is native hosted evidence for Windows and macOS, distinct from the local
MinGW cross-compilation and Wine execution. No platform result is inferred
from a syntax-only or layout-only probe.

## Verified facts, limitations, and hypotheses

Verified facts:

- The public core state supplies all M34-classified register/HALT/`execEI`
  import/export operations, and the approved level setter restores IRQ.
- The explicit 68-byte codec loads every retained fixture and safely rejects
  revision 2 without mutating runtime state.
- Both callback configurations pass all focused wrapper tests; ZEXDOC and
  ZEXALL pass through the wrapper.
- The public mirror, SLEEP_HACK callback freshness, clock debt, eight-bit I/O,
  acceptance-time acknowledge, and level IRQ contracts are exercised.
- No vendored byte, M35 patch byte, production-core source selection, frozen
  reference file, private input, or approved deletion-list file changed.

Known limitations:

- Local Windows evidence is cross-compilation plus Wine, not native MSYS2.
- Native macOS requires the configured hosted job.
- `GetDiag` is intentionally an inert M37 placeholder until M40 replaces the
  production disassembly bridge.
- Wrapper ZEX completion is detected after a ten-million-clock batch returns,
  so the reported total is a deterministic upper batch boundary rather than
  the exact HALT-entry cycle.
- The wrapper exposes the vendored core's observed R behavior; instruction-
  by-instruction comparison with the legacy core belongs to M38.

Hypotheses:

- None is used to pass the local M37 checks. Private-ROM behavior and legacy-
  versus-new differential equivalence remain unclaimed.

## Gate

**G37 PASSED.** Local validation and every configured hosted job are green.
The branch is stopped at G37. M38 has not been started and is not authorized.
