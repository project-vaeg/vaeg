# M44 uPD9002 serialized/runtime state boundary

Status: implementation and required local and hosted machine validation
complete. This report does not declare G44 passed.

## Identity and starting audit

- Branch: `topic/m44-upd9002-state-boundary`
- Approved G43: `91ec9a4c998928523360c37dab8d6ade8e698731`
- Assigned continuation start: `83858f26e0896e9cb2510e247443d4c0c2728fe6`
- Evidence HEAD before this report revision:
  `59aca2daf99fefa8451a31aa4292f078dfaa39e2`
- Hosted-CI evidence SHA: `f70d1a65b1a4f25e152e41e48f0f22d5fe0b025d`
- Remote branch before the continuation commits:
  `83858f26e0896e9cb2510e247443d4c0c2728fe6`
- Final and remote SHAs are reported in the completion handoff because the
  documentation and hosted-CI evidence commits determine those identities.

`git merge-base --is-ancestor` returned zero for G43 and the M44 branch. The
local `pre-upd9002-refactor^{}` tag resolves exactly to G43. The remote peeled
tag was also verified at that SHA before implementation. No history was reset,
squashed, rebased, or rewritten.

All 18 M44 commits already present through `83858f26` were inspected before
continuing. At that point the separately typed state objects, adapter,
standalone negative tests, fixture probe, statsave integration test, section
inspector, and an initial scenario driver already existed. The remaining work
was the authoritative G41 cross-version proof, correction of the execution
scenario, complete toolchain and baseline validation, and final evidence.

Commit `83858f26` alone did not satisfy M44. Its fixed-execution scenario used
the block executor rather than the committed M42 definition of three
`v30c_step()` calls. In addition, all tests-enabled CI presets omitted the trace
option even though the existing fail-closed CMake guard required it. These were
repository implementation defects, not a conflict between the M44 task and the
master plan. Work stopped at the audit boundary; after the maintainer's
`continue` instruction, the defects were corrected without changing any
baseline. The audit was also saved outside Git as `/tmp/m44-audit.txt`.

The initial expected change set was the state types and adapter, statsave
callback/preflight, M44 tests and scenario helper, CMake test integration, and
this report. The actual file list is recorded below. The M45-M49 task documents
were already added by the first pre-existing M44 commit; no M45 implementation
was started.

## Approved G41 resolution

The approved raw-`I286STAT` G41 commit is:

```text
dc8a72da974f0ea328613e480f1de662c28f4436
```

This is unambiguous. The M42 report names the same full SHA as its accepted
start, and both the local and remote annotated `pre-upd9002-series` tags peel to
that commit. G43 and `pre-upd9002-refactor` were not substituted for tests of
the legacy raw implementation.

The authoritative scenario source is the committed M42 fixture definition and
`tests/upd9002/state_fixtures_m42.txt`, SHA-256
`c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb`.
The scenario is reset, then program bytes `b8 34 12 40 90` with the committed
register/clock setup and exactly three `v30c_step()` calls, then the real
reset-request path to CPU_SHUT.

The detached worktree `/tmp/vaeg-m44-g41-dc8a72da` was clean before and after
the comparison and remained detached at the exact G41 SHA. The G41 tree has no
scenario command, so a disposable uncommitted test-only patch compiled the M44
scenario helper with `VAEG_M44_RAW_I286STAT`; the raw G41 state type, statsave,
CPU implementation, reset, and CPU_SHUT sources were not changed. The
instrumentation diff SHA-256 was
`be3ce872fb82430f54d05f66b1cee7925bf8d8edd00a12f81f7ad85789fedfc7`.
The patch was reversed and `git diff --exit-code` returned zero.

Both sides used AppleClang 21.0.0, target
`arm64-apple-darwin25.5.0`, Mach-O arm64, LP64 (8-byte pointers),
little-endian layout, Release mode, `/opt/local`, tests and integration trace
enabled, archive drop disabled, and no extra C/C++ flags. The relevant CMake
cache values matched. This is the same ABI scope as the raw structure; M44 does
not claim cross-ABI or cross-endian portability.

## Completed state-boundary design

`Cpu286StateCompat` is the exact 112-byte, alignment-4 CPU286 serialized
contract. Compile-time and runtime checks retain every M42-recorded offset,
including compatibility padding at 94, `cpu_type` at 96, and clocks through
offset 108.

`Upd9002RuntimeState` is a distinct type used by `I286CORE.s` and the active CPU
macros. It retains all fields still touched by the preserved V30 and protected-
mode implementation. Its two-byte layout slot is always constructed as zero
and is never imported as compatibility data. The active core has no reference
to the compatibility image.

`Cpu286CompatImage` is an opaque 112-byte adapter-owned shadow. Only
`i286c/upd9002_state.c` owns the live image. Import copies a complete payload to
temporary compatibility storage, validates exact size and
`cpu_type == CPUTYPE_V30`, constructs a temporary runtime object while skipping
opaque padding, then commits runtime and image together. Export begins with the
opaque image and overlays only runtime-owned byte ranges.

The CPU286 statsave table entry keeps the exact `CPU286` tag, version, and
112-byte payload but now uses the dedicated `STATFLAG_CPU286` handler. A full
`statsave_check()` pass validates version, size, truncation, and CPU type before
`statsave_load()` stops devices or changes any live section. The UPD9002 entry
remains the original 16-byte section.

The sole intentional external behavior change in M42-M49 is deterministic
rejection of a serialized CPU286 payload whose `cpu_type` is not
`CPUTYPE_V30`. No runtime CPU_TYPE dispatch, `i286c_step()`, `i286c()`,
`v30c()`, 286 protected-mode code, instruction behavior, FLAGS, timing, DMA,
interrupt, prefetch, or I/O behavior was removed or changed.

## Cross-version compatibility matrix

The matrix tool parses complete state containers, rejects malformed headers,
duplicate sections, incorrect versions/sizes, truncation, and nonzero section
padding, then compares extracted CPU286 and UPD9002 payloads. Whole-file hashes
were not used as a substitute for payload equality.

| Producer | Consumer and immediate re-save | reset | executed-3 | CPU_SHUT |
|---|---|---:|---:|---:|
| G41 raw | M44 adapter | exact | exact | exact |
| M44 adapter | G41 raw | exact | exact | exact |
| G41 raw | authoritative M42 fixtures | exact | exact | exact |
| M44 adapter | authoritative M42 fixtures | exact | exact | exact |

| Scenario | CPU286 SHA-256 | CPU type | UPD9002 SHA-256 |
|---|---|---:|---|
| reset | `6cbd3314a586ccf9c5f1d11a17b8836f76f1deaf10814de8b61ebdf96a792f89` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| executed-3 | `45d06e424fd332027817ca66db7ed0298e87695cf0cfed5582b7fe1348faa7fe` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| cpu-shut-request | `7636def12b5f25c39540a1367166394a8308064cce74a15860c62a487a946ff7` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

All CPU286 payloads were 112 bytes and all UPD9002 payloads were 16 bytes.
`tools/qa/upd9002_state_matrix.py` returned zero with every matrix boolean
true.

## Rejection, atomicity, and lifecycle results

| Case | Expected/result | Live-state proof |
|---|---|---|
| `cpu_type != CPUTYPE_V30` | rejected with `CPU286 cpu_type is not CPUTYPE_V30` | runtime, compatibility image, PCCORE, UPD9002, and 0x130000-byte memory hash unchanged |
| declared CPU286 size 111 | rejected with `CPU286 payload size is not 112 bytes` | same whole-machine snapshot unchanged |
| CPU286 truncated by 7 bytes | rejected with `CPU286 payload is truncated` | same whole-machine snapshot unchanged |
| imported padding `a5 5a` | immediate CPU286 re-save byte-identical | padding never enters runtime state |
| reset after noncanonical import | exact G41/M42 reset payload | compatibility image canonicalized exactly |
| CPU_SHUT after noncanonical import | exact historical clear of bytes `[0,96)` | tail retained as G41; FLAGS remains `0000` |
| all valid round trips | UPD9002 payload byte-identical | 16-byte section unchanged |

The standalone state test and the full statsave integration test both cover
these cases. The integration test calls both `statsave_check()` and
`statsave_load()` for each malformed file and verifies that no partially loaded
machine can resume.

Fresh reset retains FLAGS `f002`. CPU_SHUT intentionally does not normalize
FLAGS: clearing the legacy prefix leaves the accepted historical `0000`
anomaly. The matrix's CPU_SHUT payload hash proves the complete byte-range
transformation against raw G41 rather than checking FLAGS alone.

## M42 and M43 exact preservation

The M42 graph generator and selftest returned zero. A file-scoped diff against
accepted M42 commit `336227f093d37e3c60bc50333216d66668755cef`
confirmed byte identity for the final graph, construction provenance, support
map, 156-case harness manifest, trace baseline, state fixtures, and G41 ABI
fixture. The trace baseline SHA-256 remains
`17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba`.
The current ROM-less selftest passed all 156 manifest-derived cases and all
three reset/executed/CPU_SHUT fixtures. Trace determinism, every origin, golden
content, and trace-on/off final checkpoints passed.

The exact pinned dataset was acquired at upstream commit
`9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21`; all 360 compressed files passed
manifest verification. Dataset identity remains:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

| M43 item | CI | full |
|---|---:|---:|
| selected records | 180,000 | 1,562,502 |
| applicable/executed | 166,821 | 1,443,876 |
| pass | 156,228 | 1,359,547 |
| semantic failures | 10,593 | 84,329 |
| skipped | 13,179 | 118,626 |
| normal termination | 165,546 | 1,431,180 |
| Type-0 termination | 1,275 | 12,696 |
| timeout/signal/crash/halt | 0 | 0 |

Both independently generated summaries passed `--expect` and were
byte-identical to the committed JSON. CI SHA-256 remains
`a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31`;
full remains
`dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9`.
All failure signatures therefore remain exact, including index digests
`946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597`
and
`50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0`.
The known-gap file remains byte-identical at SHA-256
`11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1`,
with exactly 40 structural selectors and 68,626 resolved test hashes.

## Build and production-isolation matrix

| Configuration | Result |
|---|---|
| AppleClang 21 Release, tests+trace, arm64 | build passed; 26/26 CTests passed with real external CI profile; selftest and smoke passed |
| AppleClang 21 tests OFF, trace OFF | production build, selftest, and smoke passed; no M44 scenario/SSTS worker strings or test-source compile commands |
| MacPorts GCC 15.2 | all three M44 C state targets built; 3/3 state CTests passed |
| AppleClang ASan+UBSan Debug | build passed; 26/26 CTests passed, external corpus visibly skipped because the separate required runs were already executed |
| MinGW-w64 GCC 15.2 x86_64 | complete Windows console build passed; all three M44 state executables and `vaeg.exe` are PE32+ x86-64 |
| Wine | unavailable on this host; local Wine execution not claimed |
| [GitHub Actions run 29616059501](https://github.com/nakatamaho/vaeg/actions/runs/29616059501) | 7/7 jobs passed: Ubuntu GCC, Clang, ASan, repository invariants, macOS FetchContent, Windows MSYS2 MinGW, and standalone conformance |

The production cache contained `VAEG_ENABLE_TESTS=OFF` and
`VAEG_Z80_INTEGRATION_TRACE=OFF`. `ninja -t commands` contained none of
`state_scenario.c`, `statsave_boundary.c`, `ssts_worker.c`, or the standalone
state targets. Binary string inspection found no M44 scenario environment
variables or SSTS worker option.

ASan reported no M44 memory error. UBSan retained two pre-existing sound-side
shift diagnostics in `sound/tms3631c.c:51` and `sound/psggenc.c:119`. Enabling
UBSan stack traces made the trace-equivalence test's stderr comparison differ
only in process IDs and ASLR addresses; rerunning with
`UBSAN_OPTIONS=print_stacktrace=0` passed, as did the complete sanitizer CTest
set. No CPU/state source was changed for those unrelated diagnostics.

## Commands and exit statuses

Unless stated otherwise, every command returned 0.

| Command or command group | Status |
|---|---:|
| `git merge-base --is-ancestor 91ec9a4... HEAD` | 0 |
| local and remote peeled G41/G43 tag verification | 0 |
| detached G41 configure/build and clean-worktree checks | 0 |
| same-option AppleClang G41 and M44 configure/build | 0 |
| G41 generation, M44 generation, G41-to-M44, M44-to-G41 | 0 each |
| `python3.13 tools/qa/upd9002_state_matrix.py ...` | 0 |
| direct ABI/state-boundary/payload-probe executables | 0 each |
| `python3 tools/qa/upd9002_dispatch.py --root .` and `--selftest` | 0 each |
| accepted-M42 file-scoped `git diff --exit-code` | 0 |
| `upd9002_ssts.py acquire` and 360-file `verify` | 0 each |
| `upd9002_ssts.py selftest`, worker selftest, CI/full report | 0 each |
| CI profile with committed `--expect` | 0; 166,821 executed |
| full profile with committed `--expect` | 0; 1,443,876 executed |
| `cmp` generated CI/full JSON against committed summaries | 0 each |
| AppleClang full build and `ctest --output-on-failure` | 0; 26/26 |
| AppleClang ROM-less `--selftest` and `--smoke` | 0 each |
| tests-disabled production build/selftest/smoke/isolation audit | 0 each |
| GCC 15 M44 state target build and targeted CTest | 0; 3/3 |
| ASan+UBSan build and complete CTest | 0; 26/26, external skip |
| MinGW cross configure and build with `CCACHE_DISABLE=1` | 0 |
| hosted build run 29616059501 at `f70d1a65...` | 0; 7/7 jobs successful |
| `check_encoding --expect utf8 --exclude hlp/` | 0; 0 violations |
| `check_eol --enforce` | 0; 0 violations |
| `check_case.py` | 0; 0 findings |
| `find_unreferenced.py --report` | 0; 69 pre-existing reference-tier/utility findings |
| `git diff --check` | 0 |

Two non-product command attempts are retained in the audit trail. System
Python 3.9 failed the M43 tool selftest because that interpreter lacks
`Path.write_text(newline=...)`; all required SSTS work then ran successfully
with installed Python 3.13. The first MinGW build attempt could not write to
the sandbox-external default ccache directory; the same build passed with
`CCACHE_DISABLE=1`.

The full native MacPorts GCC application build was attempted for both G41 and
M44 with identical options. Both failed identically in C++ standard-library
headers because MacPorts libstdc++ 15 and the macOS SDK disagree about
`at_quick_exit`/`quick_exit`. The M44 C adapter and all standalone C tests did
build and pass under that compiler. This is a host toolchain limitation, not a
G41/M44 result difference.

## Commits and files

The M44 history through the evidence HEAD is, in order:

1. `9a4d7befd947c51ed3c05ce3bf1014fa15c44274` — add approved milestone tasks
2. `beb9843e6bd89855acaccef97b744c26d2ec00a0` — define state types
3. `2895c11354c73b1758b6c06fad4b5c5ec8e68570` — transactional adapter
4. `8e709db3431a3a7c64f7040c4cf719e5102de559` — state/rejection tests
5. `44aa172d7bfe1ba2fca65c7ba9eac2ad791fd82b` — payload probe
6. `3847a30a6751ac0f93c931f7657421cc13587078` — initial report
7. `792c3d5726af983aaadd48646158e8091a231a18` — trace diagnosis
8. `f72d3c60e602a94bb9e5a3a77537941116322609` — fail-closed trace configuration
9. `3b7c17247517582f3772cdd78d32fc9a458b9ccb` — validation status
10. `db0517bac0fdd163d85f4c28cec3023760390d88` — production trace isolation
11. `4d5645c2de66d01c793e0864692fc48cae610259` — trace side-effect audit
12. `994535467c81513d7cf3a3e915aec8380a0f9984` — trace/state run
13. `7572f1cc16231433d2ab742524f67164af752ebd` — compiler/platform status
14. `9a09137f5e75613a5dbefb1cd713e71ff7b14c6a` — initial G41 limits
15. `93ec1bfbb275f6445d57092d97080214dd66c5b4` — black-box constraints
16. `b1abcf47a73cdd08ac3009708df35e82d2b0aa4e` — state input audit
17. `73954666397dc27ee8d48f7cae7cabe88036d54d` — section inspector
18. `83858f26e0896e9cb2510e247443d4c0c2728fe6` — initial scenario driver
19. `f70ed36f545f710dd438735a9785a0c4eed62572` — approved G41 resolution
20. `d2f8f2e2ea23e487f33ec144d95b969507fa6e18` — trace-enabled CI presets
21. `485fc0efc74c1d15427b5cd93a085121e93ea00b` — authoritative scenarios
22. `59aca2daf99fefa8451a31aa4292f078dfaa39e2` — bidirectional matrix verifier
23. `f70d1a65b1a4f25e152e41e48f0f22d5fe0b025d` — final local evidence and ledger

The documentation/hosted-CI evidence commits after this list are included in
the final handoff. Across G43 through the evidence HEAD, 27 paths changed; this
completion documentation also updates the permanent bug-fix ledger, for 28
paths in the final branch:

- build integration: `CMakeLists.txt`, `CMakePresets.json`
- CPU/state implementation: `i286c/cpucore.h`, `i286c/i286c.c`,
  `i286c/upd9002_state.c`, `i286c/upd9002_state.h`,
  `i286c/upd9002_trace.h`, `statsave.c`, `statsave.tbl`
- test integration: `sdl2/selftest.c`, `tests/upd9002/abi.c`,
  `tests/upd9002/fixtures.c`
- new tests/helpers: `tests/upd9002/state_boundary.c`,
  `state_payload_probe.c`, `state_scenario.c/.h`,
  `statsave_boundary.c/.h`
- QA: `tools/qa/statsave_sections.py`,
  `tools/qa/upd9002_state_matrix.py`
- documentation: this report, `docs/modernization/bug-fixes.md`, and the
  already approved M44-M49 task files

## Deviations and remaining risks

- `golden_smoke.sh` does not exist in this repository, as already recorded by
  M42, so that named master-plan command could not be run. All committed M42
  goldens and the ROM-less smoke path were run instead.
- The state format remains ABI-specific. Only the recorded arm64 little-endian
  LP64 ABI was used for the required G41 matrix; the exact MinGW LLP64 layout
  compiled, but no new cross-ABI compatibility claim is made.
- Wine is not installed locally. MinGW cross-compilation succeeded, and the
  hosted Windows MSYS2 job built, smoked, tested, and audited runtime imports.
- The two pre-existing sound UBSan diagnostics remain outside M44 scope.
- The maintainer's clean-checkout boot/demo/OS human gate has not been run by
  this automated session. G44 remains a human decision.

## G44 human-review checklist

- [x] Confirm branch ancestry and immutable G41/G43 tags.
- [x] Review the three-type ownership boundary and dedicated statsave handler.
- [x] Verify invalid type, malformed size, and truncation reject atomically.
- [x] Verify opaque-byte immediate round trip.
- [x] Verify reset and CPU_SHUT against raw G41, including FLAGS `0000`.
- [x] Verify bidirectional G41/M44 CPU286 and UPD9002 payload equality.
- [x] Verify exact M42 graph, harness, trace, ABI, reset, and CPU_SHUT fixtures.
- [x] Verify exact M43 CI/full profiles, all signatures, and zero timeout/crash.
- [x] Review tests-disabled production isolation and available toolchains.
- [x] Record hosted CI for pushed evidence SHA `f70d1a65` (run 29616059501,
  7/7 jobs successful).
- [ ] Build from a clean checkout and perform the standard V3/demo/OS human
  gate.
- [ ] Maintainer decides G44; this report does not make that declaration.

Stop after M44. No M45 source work is included.
