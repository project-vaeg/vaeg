# M73 uPD9002 post-M49 runtime performance regression

## Status

M73 is a G73 human-review candidate. G73 has not been approved by this report.

## Starting point

| Item | Value |
|------|-------|
| Branch | `topic/m73-upd9002-post-m49-performance-regression` |
| Approved predecessor | G72 |
| Approved G72 SHA | `643d9f7289d817c67f343bf01be368b546bc1438` |
| M73 starting `origin/main` SHA | `6cf3942f1637f3ce002affafaa379940ad59e716` |
| Task authority commit | `6cf3942f1637f3ce002affafaa379940ad59e716` |
| Diagnostic-tooling commit | `34ece432b7267d8f42c35937a97ad7b97563db53` |
| Production-fix commit | `e7ac7e930c685e565bff131a42fe48f08c799990` |
| Evaluated production SHA | `e7ac7e930c685e565bff131a42fe48f08c799990` |
| Candidate SHA | supplied by the final handoff commit |

## Boundary update

The M73 task recorded the original maintainer observation as an M49-to-M50
slowdown and required the milestone to verify that boundary instead of
assuming it. Follow-up maintainer runtime checks refined the boundary:

| Checkpoint | Runtime result | Source |
|------------|----------------|--------|
| `336227f` | fast | maintainer runtime check |
| `91ec9a4c998928523360c37dab8d6ade8e698731` | fast | maintainer runtime check using GitHub Actions run `29558081970` |
| `2a21a5264a3830f5a393ed7fbd3fbe1e900f2926` | slow | maintainer correction |
| `339f5f62b3e69611f66f6689be8798f1c675b2cf` | slow | maintainer runtime check |

The confirmed actionable boundary for M73 is therefore approved G43 fast to
approved G48 slow. The M49/M50 hypothesis was not used as the root cause.

## Relevant production change

The decisive M48 production change is:

```text
bc00b370480283dbf7f7529fc6345def87a7dc75
M48: stop unresolved REP 0F before state mutation
```

That change correctly installed the fail-closed REP+0F diagnostic path, but it
also added a scheduler hot-loop poll:

```text
pccore_exec()
  -> v30c_step()
  -> upd9002_diagnostic_pending()
```

The predecessor implementation made `upd9002_diagnostic_pending()` an
out-of-line C function. Ordinary execution takes this false branch after every
uPD9002 instruction, so the call overhead is paid continuously while the
diagnostic is almost never pending.

## Root cause

The runtime regression was a host-side hot-loop overhead introduced by the
M48 diagnostic latch poll. It was not an instruction semantic regression, an
`8E` reserved-instruction regression, an `0F` guest-path regression, frame
pacing configuration, or a mapped-memory regression.

M48's fail-closed diagnostic policy remains correct. The M73 defect was only
the cost of testing that diagnostic state through an external function in the
per-instruction scheduler path.

## Correction

`cpu/upd9002/upd9002_diagnostic.c` now exposes the diagnostic latch state as
the single shared object:

```c
UPD9002_DIAGNOSTIC upd9002_diagnostic_state;
```

`cpu/upd9002/upd9002_diagnostic.h` now defines the pending check inline:

```c
#define upd9002_diagnostic_pending() \
	((BOOL)(upd9002_diagnostic_state.reason != UPD9002_DIAGNOSTIC_NONE))
```

The correction removes the external `_upd9002_diagnostic_pending` symbol while
preserving:

- REP+0F diagnostic latch state;
- diagnostic clear/raise/get APIs;
- fail-closed behavior;
- pre-mutation state and memory atomicity;
- diagnostic message behavior.

## Validation

| Command | Exit | Result |
|---------|------|--------|
| `cmake --preset macos-macports` | 0 | configure passed |
| `cmake --build build/macos-macports --target vaeg_sdl2 -j 8` | 0 | production build passed |
| `cmake -S . -B build/macos-m73-perf -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_UPD9002_PERF_DIAGNOSTIC=ON` | 0 | diagnostic build configured |
| `cmake --build build/macos-m73-perf --target vaeg_sdl2 -j 8` | 0 | diagnostic build passed |
| `/usr/bin/env VAEG_UPD9002_PERF_LOG=/tmp/vaeg-m73-selftest-perf.txt build/macos-m73-perf/sdl2/vaeg --selftest` | 0 | selftest passed; perf log recorded `steps count=0` |
| `cmake -S . -B build/macos-m73-tests -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_INTEGRATION_TRACE=ON` | 0 | test build configured |
| `cmake --build build/macos-m73-tests -j 8` | 0 | all test targets built |
| `build/macos-m73-tests/sdl2/vaeg --upd9002-m48-rep0f-diagnostic` | 0 | `cases=522 state-and-memory-atomic pass` |
| `build/macos-m73-tests/sdl2/vaeg --upd9002-m68-segmented-memory` | 0 | mapped dispatch checks passed |
| `build/macos-m73-tests/sdl2/vaeg --idp-m69-status-composition` | 0 | status composition checks passed |
| `build/macos-m73-tests/sdl2/vaeg --upd9002-m70-prefix-string` | 0 | directed checks passed |
| `build/macos-m73-tests/sdl2/vaeg --selftest` | 0 | all selftests passed; CoreAudio emitted a pre-existing host warning |
| `ctest --test-dir build/macos-m73-tests --output-on-failure -L "romless"` | 0 | 64/64 tests passed; external SST test skipped because `VAEG_SSTS_V20_ROOT` was unset |
| `cmake --preset mingw-cross` | 0 | MinGW cross configure passed |
| `cmake --build --preset mingw-cross --target vaeg_sdl2` | 0 | MinGW PE32+ GUI executable linked |
| `nm -g build/macos-m73-tests/libvaeg_core.a \| rg "upd9002_diagnostic_(pending\|state\|clear\|raise\|get)"` | 0 | `_upd9002_diagnostic_pending` absent; other diagnostic symbols present |
| `git diff --check` | 0 | whitespace check passed |

The first CTest attempt before building all test targets failed because some
test executables had not yet been built. After building all targets, the same
ROM-less CTest selection passed.

## Manual runtime result

The maintainer tested:

```text
/tmp/vaeg-m73-inline-diagnostic.exe
```

The executable was copied from:

```text
build/mingw-cross/sdl2/vaeg.exe
```

SHA-256:

```text
cd3cd52fc5b83b9831acebfd7a2b1178b4ad7c18e657f2bd0bbd3e47cc547221
```

Maintainer result:

```text
vaeg-m73-inline-diagnostic.exe was fast.
```

The Windows CI test artifact from GitHub Actions is intentionally not the
manual runtime-performance artifact. The `windows msys2 mingw64 z80` job uses
the `mingw-ci` preset, which enables `VAEG_ENABLE_TESTS`,
`VAEG_Z80_INTEGRATION_TRACE`, and the Windows console subsystem for validation.

M73 therefore adds a separate hosted release artifact job using the
`mingw-release` preset. Manual gate, runtime-performance, and pre-distribution
checks should use the `vaeg-windows-mingw64-release` artifact, while
`vaeg-windows-mingw64-z80` remains the tests-and-trace CI artifact.

## Scope audit

Changed files from the M73 starting SHA:

```text
.github/workflows/build.yml
CMakeLists.txt
cpu/upd9002/upd9002_core.c
cpu/upd9002/upd9002_diagnostic.c
cpu/upd9002/upd9002_diagnostic.h
cpu/upd9002/upd9002_mn.c
cpu/upd9002/upd9002_perf.c
cpu/upd9002/upd9002_perf.h
sdl2/np2.c
docs/agents/reports/m73_upd9002_post_m49_performance_regression.md
docs/modernization/bug-fixes.md
```

The production correction is limited to the diagnostic pending check. It does
not change instruction semantics, memory routing, state format, M68 mapped
dispatch, M69 IDP status composition, M70 prefix/string semantics, M71 dispatch
folding, M72 cleanup policy, SST fixtures, target policies, or comparison
contracts.

The optional performance diagnostic code is disabled by default behind
`VAEG_UPD9002_PERF_DIAGNOSTIC`.

The workflow update does not alter the existing Windows CI test job. It adds a
separate `mingw-release` artifact so the downloadable Windows executable used
for human runtime testing is built with tests and Z80 integration trace
disabled.

## Hosted CI

Hosted CI must target the final candidate SHA after this report is committed
and pushed. G73 remains unapproved until human review accepts that result.

## G73 checklist

- [x] Refined the slowdown boundary from the initial M49/M50 hypothesis.
- [x] Identified a bounded M48 host-side hot-loop cost.
- [x] Implemented the smallest correction within M73 scope.
- [x] Preserved the M48 diagnostic behavior.
- [x] Preserved M68, M69, and M70 focused protections.
- [x] Produced a MinGW executable for maintainer runtime validation.
- [x] Bound maintainer fast-result evidence to the executable digest.
- [ ] Hosted CI passed against the exact final candidate SHA.
- [ ] Human maintainer approved G73.
