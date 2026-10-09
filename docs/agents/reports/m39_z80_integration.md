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
# M39 Z80 production integration evidence

## Gate disposition

The public and ROM-less M39 implementation is complete. The suzukiplan-backed
wrapper is a production build option, the default remains legacy, and every
available public check is green. The maintainer-private manifest was executed
under both production selections on 2026-07-15. Boot, FDD read/write, loaders,
SLEEP_HACK, WAIT wake, and frame-boundary state cases passed. **G39 PASSED.**
M40 has not started.

## Starting state

The required files were read in the prescribed order before any change.

```text
starting branch: topic/m38-z80-differential
starting SHA:    ea0388c266e95a717d4ff2ce268d66c695e87945
new branch:      topic/m39-z80-integration
```

Initial `git status --short` was exactly:

```text
?? "PC-Engine 1.05.d88"
?? docs/modernization/PCEPAT.DOC
?? docs/tekumani/
?? his.txt
```

Those four paths remain untracked. They were not opened, hashed, copied,
modified, staged, committed, or used as test input.

## Integration and source selection

`VAEG_Z80_CORE` accepts exactly `legacy` and `suzukiplan`; an omitted value
selects `legacy`. The choice is a CMake cache/build-time value and target-local
definitions identify the selected implementation. There is no runtime toggle.

| Selection | Production `vaeg_va` Z80 sources |
|---|---|
| `legacy` | `cpucva/z80c.cpp`, `cpucva/z80diag.cpp` |
| `suzukiplan` | `cpucva/z80_compat_cpu.cpp`, `cpucva/z80_compat_state.cpp`, `cpucva/z80diag.cpp`, `cpucva/z80diag_bridge.cpp` |

The two `Z80C` implementations are never linked into the same production
target. The existing disassembler is retained for both selections; the new
selection reaches it through an independently authored fixed-width callback
bridge in a separate translation unit. M39 neither replaces nor edits it.

The `iova/subsystem.cpp` adapter preserves the vaeg memory, I/O, clock, IRQ,
and acknowledge interfaces. New-wrapper I/O masks the vaeg-facing port with
`port & 0xff`; IRQ is level-sensitive; the acknowledge port is read only from
the core's actual acceptance callback. The production default and release
workflow remain legacy.

### Retained API

The consumer-facing class remains `Z80C`. The integration retains the used
constructor/destructor and `Init`, `Exec`, `Reset`, `IRQ`, `NMI`, `Wait`,
`GetStatusSize`, `SaveStatus`, `LoadStatus`, `GetPC`, `SetPC`, `GetReg`, and
`GetDiag` signatures established by M34/M37. No suzukiplan or STL type crosses
the consumer-facing interface.

### State and public mirror

The revision-1 image remains 68 bytes with the M37 offsets and explicit codec.
It represents architectural registers, I/R/IM/IFF1/IFF2, HALT, external WAIT,
level IRQ, `remainclock`, and `lastclock`; reserved wait-byte bit 2 carries
new-to-new frame-boundary EI inhibition. HALT PC translation and the
architectural-AF/ignore-legacy-`xf` policy are unchanged.

`GetPC()` remains authoritative and live. `GetReg()->pc` remains stale during
an active callback and is refreshed when `Exec()` returns. `SaveStatus()` uses
the authoritative state. The C state bridge now returns failure and
`statsave.c` propagates unsupported revision rejection instead of silently
continuing.

## ROM-less subsystem evidence

The selected implementation is exercised through the real `Subsystem` seam,
not only through the standalone wrapper. Deterministic integration fixtures
cover:

- ordinary execution, live PC, and return-fresh public mirror;
- retained revision-1 input and new-to-new round trip;
- HALT, external WAIT, restored level IRQ, EI inhibition, signed
  `remainclock`, and nonzero `lastclock`;
- DI with an asserted IRQ, no speculative acknowledge, and exactly one IM1
  acknowledge at acceptance;
- an unsupported embedded Z80 revision rejected through `statsave_load()`;
- one real production `OUT (0xf4),0x5a`, with save/load between CPU calls and
  no loss or duplication; and
- legacy diagnostic decoding under both production selections.

The real SLEEP_HACK callback observations are identical for both selections:

```text
VA:        live=1734 public=1732 memory[7f67]=ff IN(fe)=00 wait-flags=02
Sorcerian: live=7010 public=700e                  IN(fe)=00 wait-flags=02
```

For both paths the test observes `Wait(true)`, drains an additional clock
slice without PC advance, applies the ATN/8255 wake path, observes
`Wait(false)`, and resumes the next instruction. Constants `0x1732` and
`0x700e` are unchanged. The focused EI-before-VA-IN fixture confirms that the
new core may return at the EI boundary and still presents the required stale
public PC at the following IN callback; the legacy core's fused private step
does not expose the same intermediate return boundary.

## M39 correction found by the build matrix

The initial `suzukiplan` MinGW build failed because `iova/subsystem.cpp` still
used the non-standard `uint` alias that the legacy header supplied
transitively. The normal cross-build was the minimal reproducer: the compiler
rejected the `Subsystem` memory/I/O overrides before linking. Commit
`386a3103b17387c87f1edd0b1e3ce101616354f9` changes only those adapter
signatures/helpers to `std::uint32_t`, matching `z80_bus.h`. Both MinGW
selections then built and passed their integration selftests under Wine. No
vendored or legacy-core source changed.

## Validation

### Repository and immutable inputs

| Exact command | Result |
|---|---|
| `python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `python3 tools/repo/find_unreferenced.py --report` | PASS; the 69-path baseline is unchanged and no M39 path is unreferenced |
| `git diff --check` | PASS |
| YAML parse of `.github/workflows/build.yml` and `.github/workflows/release.yml` | PASS |
| `git diff --quiet ea0388c266e95a717d4ff2ce268d66c695e87945 -- external/suzukiplan-z80 docs/agents/reports/m35_suzukiplan_irq_extension.patch` | PASS, no byte changed |
| frozen-tier and seven-file `git diff --name-status` from the starting SHA | PASS, empty |

Immutable hashes remained:

```text
ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1  external/suzukiplan-z80/LICENSE.txt
59d660c57262d1166cd317877691496e0a072200f4e20e388f8d88e77ba39cda  external/suzukiplan-z80/provenance.txt
abe7cd10642c6d13ad0636ef53091cd3c4bf643ecd6f564a22707393e06728c4  external/suzukiplan-z80/test/test-interrupt-extension.cpp
88c878f0087f114eb864dc6a9e8cb98473c022e052c62937ab7a23aecbdb7106  external/suzukiplan-z80/z80.hpp
d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4  docs/agents/reports/m35_suzukiplan_irq_extension.patch
```

### GCC and ZEX

The ZEX inputs were obtained outside the tree with:

```sh
python3 tests/z80_compat/fetch_zex.py --output-dir /tmp/vaeg-m39-zex
```

All five immutable artifacts passed their recorded SHA-256 checks.

```sh
cmake -S . -B build/m39-gcc-legacy -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=legacy \
  -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m39-zex
cmake --build build/m39-gcc-legacy --parallel 2
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ctest --test-dir build/m39-gcc-legacy --output-on-failure

cmake -S . -B build/m39-gcc-suzukiplan -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=suzukiplan
cmake --build build/m39-gcc-suzukiplan --parallel 2
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ctest --test-dir build/m39-gcc-suzukiplan --output-on-failure
```

Both configure/build commands passed. Legacy passed 18/18 including raw and
wrapper ZEX; suzukiplan passed 14/14. After the MinGW-derived fixed-width
correction, both production selections again passed all 14 non-ZEX tests.
The four standalone ZEX tests were then rerun together on the final code:

```sh
ctest --test-dir build/m39-gcc-legacy --output-on-failure -R zex -j 4
```

Result: 4/4 passed. Raw ZEXDOC/ZEXALL took 78.82/76.27 seconds and wrapper
ZEXDOC/ZEXALL took 148.84/149.93 seconds. The directed, state, generated
(committed seeds), and eventual-convergence M38 suites all passed with no new
allowlist or unresolved external divergence.

Both production smoke commands passed with exit 0:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/m39-gcc-legacy/sdl2/vaeg --smoke
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/m39-gcc-suzukiplan/sdl2/vaeg --smoke
```

### Clang

```sh
CC=/usr/bin/clang CXX=/usr/bin/clang++ \
  cmake -S . -B build/m39-clang-legacy -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=legacy
cmake --build build/m39-clang-legacy --parallel 2
ctest --test-dir build/m39-clang-legacy --output-on-failure

CC=/usr/bin/clang CXX=/usr/bin/clang++ \
  cmake -S . -B build/m39-clang-suzukiplan -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=suzukiplan
cmake --build build/m39-clang-suzukiplan --parallel 2
ctest --test-dir build/m39-clang-suzukiplan --output-on-failure
```

Both builds and both 14/14 test sets passed. Both dummy-driver smoke runs
passed. This was native Linux execution with Clang 21.1.8.

### ASan/UBSan

For each core choice, configuration used:

```sh
cmake -S . -B build/m39-asan-<core> -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=<core> \
  -DCMAKE_C_FLAGS=-fsanitize=address,undefined \
  -DCMAKE_CXX_FLAGS=-fsanitize=address,undefined \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
cmake --build build/m39-asan-<core> --parallel 2
ASAN_OPTIONS=detect_leaks=0 \
  ctest --test-dir build/m39-asan-<core> --output-on-failure
```

Both builds and both 14/14 test sets passed. Both smoke processes exited 0.
The smoke paths reproduced the same pre-existing UBSan reports in
`sound/tms3631c.c`, `sound/psggenc.c`, `sound/psggeng.c`, and
`common/parts.c`; no M39 Z80 source reported a sanitizer failure.

### MinGW and Wine

Each selection was configured and built separately:

```sh
cmake --preset mingw-cross -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=<legacy-or-suzukiplan>
cmake --build --preset mingw-cross --parallel 2
```

Both passed after the fixed-width adapter correction. This is cross-
compilation, not native Windows execution. Each resulting production binary
then passed these Wine commands:

```sh
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m39-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --selftest
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m39-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --smoke
```

Both selections reported all subsystem integration tests passed and smoke
exit 0. Wine execution is not claimed as hosted native Windows.

### Selection and archive proofs

An unqualified configure printed `Production Z80 core: legacy` and built:

```sh
cmake -S . -B build/m39-default -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=OFF
cmake --build build/m39-default --target vaeg_sdl2 --parallel 2
```

`VAEG_Z80_CORE=invalid` failed configuration with the intended exact-choice
diagnostic. `ninja -t commands vaeg_va` showed `z80c.cpp` only in the legacy
archive, and `z80_core.cpp`/`z80_legacy_state.cpp` only in the suzukiplan
archive.

```sh
git archive --format=tar.gz \
  --output=/tmp/vaeg-m39-source-386a310.tar.gz HEAD
tar -czf /tmp/vaeg-m39-runtime-386a310.tar.gz \
  build/m39-gcc-legacy/sdl2/vaeg assets/OFL.txt assets/NOTICE.md \
  CHANGES.20260713.md dist/readme-dist.txt
python3 tests/z80_compat/check_zex_archive.py \
  /tmp/vaeg-m39-source-386a310.tar.gz \
  /tmp/vaeg-m39-runtime-386a310.tar.gz
```

Result: source 993 files and runtime 5 files checked; neither archive contains
a fetched ZEX name or content hash.

### Hosted CI

Public [build run 29395315812](https://github.com/nakatamaho/vaeg/actions/runs/29395315812)
tested `b062b80ce7a6957b791c24a1080e9691e47d3e33`. All 12 jobs completed
successfully:

| Hosted job | Result | Duration |
|---|---|---:|
| repo invariants | PASS | 4 s |
| ubuntu gcc legacy z80 | PASS | 90 s |
| ubuntu gcc suzukiplan z80 | PASS | 82 s |
| ubuntu clang legacy z80 | PASS | 84 s |
| ubuntu clang suzukiplan z80 | PASS | 88 s |
| ubuntu asan legacy z80 | PASS | 77 s |
| ubuntu asan suzukiplan z80 | PASS | 70 s |
| windows msys2 mingw64 legacy z80 | PASS | 467 s |
| windows msys2 mingw64 suzukiplan z80 | PASS | 499 s |
| macos fetch-sdl2 legacy z80 | PASS | 217 s |
| macos fetch-sdl2 suzukiplan z80 | PASS | 169 s |
| standalone z80 conformance | PASS | 499 s |

The Windows and macOS results are native hosted execution. The standalone job
verified immutable ZEX acquisition, raw and wrapper ZEXDOC/ZEXALL, focused
interrupt/wrapper/differential tests, ROM-less smoke, and source-archive
exclusion. It remains single because it is independent of the production
selection.

## Files and commits before the final evidence commit

```text
23b70711b84deb027a1c8dbf11e6284b65d0d4fe M39: integrate opt-in production Z80 wrapper
6ab0e4afbf2c08b339c0a53fe51954cb1a07b069 M39: test both production Z80 selections in CI
5430939e42dd644748f831c04c6474aa3d0b590b M39: document production integration and private gate
c0d64ea200d5ae52d243119aeb229dc4ea5794b2 M39: keep integration fixtures warning-clean
386a3103b17387c87f1edd0b1e3ce101616354f9 M39: fix MinGW production adapter types
```

Added:

```text
cpucva/z80diag_bridge.cpp
cpucva/z80diag_bridge.h
docs/modernization/z80-compatibility-integration.md
docs/modernization/z80-compatibility-private-integration.md
tests/z80_compat/subsystem_integration.cpp
tests/z80_compat/subsystem_integration.h
```

Modified:

```text
.github/workflows/build.yml
CMakeLists.txt
docs/agents/DECISIONS/ADR-0011-z80-migration.md
docs/agents/tasks/M39_z80_integration.md
docs/modernization/BUILD.md
docs/modernization/bug-fixes.md
docs/modernization/z80-compat-cycle-deltas.md
iova/subsystem.cpp
iova/subsystem.h
sdl2/selftest.c
statsave.c
```

No file was deleted or renamed. The vendored tree, approved patch, frozen
reference tier, seven approved M88-derived files, and SLEEP_HACK constants are
unchanged.

## Private system validation

The committed
[`z80-private-integration.md`](../../modernization/z80-compatibility-private-integration.md)
records all 15 cases with neutral identifiers. Private filenames, absolute
paths, hashes, screenshots, raw logs, traces, state files, and writable media
copies remain outside Git. Originals were not mounted writable. The four
maintainer-designated untracked paths were deliberately not inspected or used.

Both production selections passed VA2/VA3 OS boot and directory operation,
three reset/boot cycles, legal demo execution, FDD read, expendable-copy write
and binary readback, representative loader, and timing-sensitive loader. The
timing-sensitive program reached the same stable menu under both cores. A
later wall-clock sample showed the new core still in an intermediate scene,
but sufficient execution converged to the same menu without a timeout, retry,
or device-visible failure.

The representative trace produced the same 21 ordered port-`0xf4` writes:
nine `00`, `10 14 04 04 04 14 1c`, then five `0c`. The common 164 completed
FDC records were byte-identical. The longer legacy wall-clock capture had 24
additional post-convergence records. FDC DMA lengths/ranges and Z80 IRQ and
acceptance-time acknowledge events were present; the current instrumentation
does not emit a dedicated DRQ-edge record, so no separate DRQ edge-count claim
is made.

Actual VA and Sorcerian sleep paths preserved the callback-stale mirror:
live/public PC was `1734/1732` with memory `ff` for VA and `7010/700e` for the
Sorcerian path. WAIT kept PC fixed while clock balance drained, actual ATN
released WAIT, and execution resumed. Both constants remain unchanged.

Revision-1 legacy-to-new and new-to-new ordinary states resumed FDD directory
activity. Legacy-to-new and new-to-new saves taken during visible loader
activity completed exactly once after load. Active external-WAIT saves under
both cores restored live/public PC `7010/7010`; a later actual ATN release and
subsequent recurring sleep/wake cycles were observed. The task-permitted idle
alternative was used for `m39-state-halt`; no private HALT assertion is
claimed, and both selections resumed a single directory operation after load.

### Private-completion validation

The documentation-only completion reran the required repository checks:

```sh
python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check
```

Encoding, EOL, case, and diff checks passed with zero findings. The
unreferenced report retained the established 69-file baseline and added no
M39 file.

Both production selections were reconfigured and built with:

```sh
cmake -S . -B build/m39-private-<core> -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=<core> -DVAEG_Z80_INTEGRATION_TRACE=OFF
cmake --build build/m39-private-<core> -j2
ctest --test-dir build/m39-private-<core> --output-on-failure
```

Configure output selected the requested core, both incremental builds were
clean, and each CTest run passed 14/14, including subsystem state, directed,
generated, state-boundary, and eventual-convergence tests.

Final ROM-less commands were:

```sh
env SDL_AUDIODRIVER=dummy \
  build/m39-private-legacy/sdl2/vaeg --selftest
env SDL_AUDIODRIVER=dummy \
  build/m39-private-suzukiplan/sdl2/vaeg --selftest
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m39-private-legacy/sdl2/vaeg --smoke
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m39-private-suzukiplan/sdl2/vaeg --smoke
```

All four final commands exited 0. An initial parallel selftest attempt made
the two processes share a generated-image pathname; the suzukiplan process
reported `existing image was overwritten`. Sequential rerun passed and CTest
had already passed. Initial direct smoke attempts while temporary private-ROM
symlinks remained in the ignored build trees exited 1 because the sandbox
could identify but not read the external targets. Removing only those
temporary symlinks restored the intended ROM-less condition; both identical
smoke commands then passed. No private target was modified.

## Risks and recommendation

### Verified facts

- Both production selections build and pass the public ROM-less subsystem,
  state, interrupt, SLEEP_HACK, WAIT, `0xf4`, differential, and conformance
  checks available locally.
- The default is legacy and the two implementations are mutually exclusive.
- Vendored and legacy-core bytes are unchanged.
- Unsupported Z80 state revisions now fail the top-level load safely.
- Public traces preserve callback-stale/return-fresh mirror behavior and
  acceptance-time acknowledge behavior.
- All 15 private manifest cases have recorded dispositions and passed their
  applicable dual-core requirements.
- No private binary, screenshot, raw trace, save, path, filename, or hash is
  tracked.

### Accepted timing difference

The M38 taken-JR reproducer still moves the otherwise identical port-`0xf4`
event to a later `Exec()` slice under corrected architectural timing. Eventual
convergence proves one ordered event and converged device-visible state for
the synthetic case. M39 adds no legacy per-opcode timing emulation.

### Known limitations

- Wine is not native Windows.
- Local Linux cannot execute native macOS or native Windows jobs.
- Sanitized smoke retains unrelated pre-existing sound/common UBSan reports.
- The private FDC trace has no dedicated DRQ-edge record. Completed-command,
  DMA transfer, IRQ, acknowledge, final-data, and guest-visible results were
  checked instead.
- The task-permitted idle alternative, rather than a positively identified
  private HALT instruction, was used for `m39-state-halt`.

### Unresolved issues

No G39-blocking private-system issue remains. Native private runs covered the
gate risks available through the current instrumentation.

### Hypotheses

The later intermediate wall-clock sample for the new timing-sensitive loader
is consistent with the M38 slice-scheduling correction. The root cause of that
wall-clock observation is not independently measured, so only eventual
device-visible convergence is treated as verified.

### G39 recommendation

**G39 PASSED.** The mandatory private boot, FDD read/write, timing-sensitive
loader, port-`0xf4`, SLEEP_HACK/WAIT wake, and state-boundary cases completed
without a blocking divergence. The default remains legacy. Stop here; M40 is
deliberately not started.
