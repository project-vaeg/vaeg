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
# M45 uPD9002 native dispatch fold

Status: implementation and required local and hosted machine validation
complete. The final evidence SHA is recorded in the handoff. This report does
not declare G45 passed and does not authorize M46.

## Identity and prerequisites

- Branch: `topic/m45-upd9002-native-dispatch-fold`
- Accepted G44 start: `b5f6ee7da7665789ce23013f2f8418fe0889773c`
- Implementation commit:
  `866d094ca5027ca273a2ca3fd1d33108d29c5f9c`
- Accepted G43 tag `pre-upd9002-refactor`:
  `91ec9a4c998928523360c37dab8d6ade8e698731`
- Accepted G41 tag `pre-upd9002-series`:
  `dc8a72da974f0ea328613e480f1de662c28f4436`

The maintainer explicitly stated `G44passed` before M45 began. The worktree
was a clean detached checkout of the accepted G44 SHA before the M45 branch was
created. Both immutable tags were verified locally after fetching `origin`.
No M46 source, rename, normalization, or deletion work was started.

## Accepted invariant and implementation

M45 establishes this invariant:

> No active execution, normal reset, interrupt, scheduler, or state-resume
> path selects an 80286 instruction mode. The legacy `cpu_type` byte is
> validated as `CPUTYPE_V30` only by the state adapter and never controls
> runtime dispatch. CPU_SHUT is the sole documented initializer-level
> exception: it preserves the historical 286-style register-init result
> required by the M42 fixture, but it does not select an 80286 opcode
> dispatcher or expose an 80286 execution mode.

The implementation makes normal initialization and `i286c_reset()` establish
`CPUTYPE_V30` and call the existing V30 register initializer unconditionally.
`pccore_exec()` now calls `v30c_step()` directly once per instruction. The
per-instruction type branch, dormant block-executor branch, local
`SINGLESTEPONLY`, active `USE_I286C` definitions, and `CPU_TYPE` macro are
removed.

`i286c_step()` has no declaration, definition, symbol, or active caller.
`i286c()` and `v30c()` remain present exactly as required for M46 ownership.
`i286c/v30patch.c`, including `v30c_step()` flag, DMA, event, exception, and
cycle order, is byte-unchanged from accepted G44.

`i286c_shut()` still clears only through `offsetof(I286STAT, cpu_type)` and
calls the 286-style register initializer. Its new comment links the exception
to ADR-0012 and states that this is not an 80286 execution mode. No shutdown
state or flag behavior was normalized.

The runtime `cpu_type` field remains temporarily because the M44 compatibility
adapter still overlays the accepted ABI tail and CPU_SHUT retains that tail.
It is written to the V30 constant during initialization/reset, range-preserved
by CPU_SHUT, and read only by state serialization/validation or tests. Removal
is deferred to the M47 state inventory unless that milestone proves it safe.

The system CPU report, printer-interface mode bit, and CPU information string
now report the invariant V30-compatible product configuration without reading
runtime state. These results are identical to every supported VA execution
before M45.

## Static reference and build map

`tools/qa/upd9002_native_invariant.py` is a fail-closed tracked/untracked source
guard and a CTest. It verified:

```text
upd9002-native-invariant: presets=13 core=i286c selectors=absent
upd9002-native-invariant: reset=v30c_initreg step=v30c_step shutdown=i286c_initreg
upd9002-native-invariant: cpu_type=i286c/cpucore.h=2,i286c/i286c.c=3,i286c/upd9002_state.c=7,i286c/upd9002_state.h=1 control=state-validation-only
upd9002-native-invariant: block-executors=i286c,v30c retained-for-m46
```

The 13 configure presets are the exact accepted Linux, MinGW, and macOS preset
set. They share `VAEG_CORE_SOURCES`, which contains `i286c/i286c.c` and
`i286c/v30patch.c` and no `i286x/` path. No active code, build, test, or tool
surface outside the guard's own banned-token table and frozen/history
documentation contains the exact selector tokens `USE_I286C`, `i286x_step`,
`v30x_step`, `i286c_step`, `CPU_TYPE`, or `SINGLESTEPONLY`.

The complete production `cpu_type` reference map is limited to:

| File | References | Disposition |
|---|---:|---|
| `i286c/cpucore.h` | 2 | serialized/runtime compatibility fields |
| `i286c/i286c.c` | 3 | two V30 constant writes and CPU_SHUT range boundary |
| `i286c/upd9002_state.c` | 7 | layout, overlay, range, and sole value validation |
| `i286c/upd9002_state.h` | 1 | deterministic validation diagnostic |

Linux and MinGW archive symbol maps both contain `i286c`, `v30c`, and
`v30c_step`; `pccore.c` has the sole unresolved call to `v30c_step`. Neither
archive contains `i286c_step`. The frozen `i286x/`, `win9x/`,
`cpuxva/memoryva.x86`, and `hlp/` trees have no M45 diff.

## State and G41 checkpoint preservation

The test build generated new reset, exact-three-step, and real reset-request
CPU_SHUT state containers. Their extracted `CPU286` and `UPD9002` payloads
were compared directly with `tests/upd9002/state_fixtures_m42.txt`, the
accepted G41-derived fixture. Every byte matched:

| Scenario | CPU286 SHA-256 | cpu_type | UPD9002 SHA-256 |
|---|---|---:|---|
| reset | `6cbd3314a586ccf9c5f1d11a17b8836f76f1deaf10814de8b61ebdf96a792f89` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| executed-3 | `45d06e424fd332027817ca66db7ed0298e87695cf0cfed5582b7fe1348faa7fe` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| cpu-shut-request | `7636def12b5f25c39540a1367166394a8308064cce74a15860c62a487a946ff7` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

All CPU286 payloads remain 112 bytes and all UPD9002 payloads remain 16 bytes.
Fresh reset remains FLAGS `f002`; CPU_SHUT remains the accepted historical
FLAGS `0000` result. The fixture file SHA-256 remains
`c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb`.

The M44 adapter, malformed-state tests, and committed payload baselines are
unchanged. Direct state-boundary tests still pass invalid type, wrong size,
truncation, atomicity, and opaque-image round trips. A new fixture precheck
deliberately corrupts only the live runtime type, invokes the real normal reset,
requires V30 type, `f000:fff0`, `CS_BASE=000f0000`, and FLAGS `f002`, then
restores the prior valid state atomically.

## M42 exact preservation

Both dispatch generator modes returned zero. A file-scoped diff against
accepted M42 commit `336227f093d37e3c60bc50333216d66668755cef`
confirmed byte identity for the final graph, construction provenance, support
map, 156-case harness manifest, trace baseline, state fixtures, and G41 ABI
fixture. The trace baseline SHA-256 remains
`17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba`.

The GCC, Clang, and sanitizer CTests passed dispatch generation, generator
selftest, 156 manifest-derived instruction cases, ABI, state boundary, payload
probe, and exact trace equivalence. ROM-less selftest again reported all 156
cases and all reset/executed/CPU_SHUT fixtures passed. The accepted final
dispatch graph and `v30c_step()` implementation were not re-recorded or
changed.

## M43 V20 exact preservation

All 360 pinned corpus files reverified at dataset identity:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

Both mandatory profiles executed against that external dataset during this
M45 gate. Neither required profile was skipped.

| M43/M45 item | CI | full |
|---|---:|---:|
| selected records | 180,000 | 1,562,502 |
| applicable/executed | 166,821 | 1,443,876 |
| pass | 156,228 | 1,359,547 |
| semantic failures | 10,593 | 84,329 |
| skipped | 13,179 | 118,626 |
| known-target-gap classification | 8,179 | 68,626 |
| upstream-nonblocking classification | 5,000 | 50,000 |
| normal termination | 165,546 | 1,431,180 |
| Type-0 termination | 1,275 | 12,696 |
| timeout/signal/crash/halt | 0 | 0 |

The generated CI and full summaries were byte-identical to their committed
JSON baselines. Their SHA-256 values remain
`a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31`
and
`dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9`.
The failure-signature index digests remain
`946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597`
and
`50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0`.

The known-gap file is byte-unchanged at SHA-256
`11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1`,
with exactly 40 rules and 68,626 resolved record hashes. No gap, category,
failure set, signature, or termination class was added, removed, or changed.

## Build, test, and isolation matrix

| Configuration | Result |
|---|---|
| GCC 15.2.0 `linux-ci-gcc` | 285/285 targets built; 27/27 CTests passed; external CI profile executed, not skipped, in 178.10 s |
| Clang 21.1.8 `linux-ci-clang` | 285/285 targets built; 27/27 CTests passed; external comparison visibly skipped because the required GCC run used the real corpus |
| GCC 15.2.0 `linux-ci-asan` | 285/285 targets built; 27/27 CTests passed with non-halting UBSan and leak detection disabled; 3/3 standalone M45 state targets also passed with `UBSAN_OPTIONS=halt_on_error=1` |
| GCC 15.2.0 `linux-release` | tests and trace OFF; production build, ROM-less selftest, and smoke passed |
| MinGW-w64 GCC 13 `mingw-cross` | 346/346 targets built; `vaeg.exe` is PE32+ GUI x86-64 |
| Wine 10.0 | MinGW production `vaeg.exe --selftest` and `--smoke` passed with a dedicated prefix |
| [GitHub Actions run 29631726186](https://github.com/nakatamaho/vaeg/actions/runs/29631726186) | 7/7 jobs passed at implementation SHA `866d094ca5027ca273a2ca3fd1d33108d29c5f9c`: Ubuntu GCC, Clang, ASan, repository invariants, macOS FetchContent, Windows MSYS2 MinGW, and standalone conformance |

The Linux production cache contains `VAEG_ENABLE_TESTS=OFF` and
`VAEG_Z80_INTEGRATION_TRACE=OFF`. Its Ninja command graph contains no M42/M44
test source, trace source, test-only definition, or standalone uPD9002 target.
Binary string inspection found no M44 scenario environment variable, SSTS
worker option, scenario diagnostic, or CPU trace schema. Production selftest
therefore omits all test seams while ordinary statsave, frontend, and device
selftests remain green.

LeakSanitizer cannot initialize under the managed ptrace environment. The
first sanitizer CTest attempt consequently ended processes at exit; rerunning
with the established `ASAN_OPTIONS=detect_leaks=0` policy passed 27/27. The
non-halting run retains accepted pre-existing UBSan diagnostics in sound and
vendored Z80 code; no diagnostic names an M45-modified CPU/state file.

## Repository checks and command results

Unless explicitly described below, every required command returned zero.

| Command or group | Result |
|---|---|
| G44/G43/G41 SHA and peeled-tag audit | exact |
| `python3 tools/qa/upd9002_native_invariant.py --root .` | 13 presets; selectors absent; reference map exact |
| `python3 tools/qa/upd9002_dispatch.py --root .` and `--selftest` | passed |
| accepted-M42 file-scoped `git diff --exit-code` | byte-identical |
| current state generation and fixture payload comparison | all three scenarios exact |
| GCC/Clang/ASan configure, build, and CTest | passed as recorded above |
| production selftest, smoke, command/string isolation | passed; no test seam |
| MinGW cross build, `file`, Wine selftest and smoke | passed |
| SSTS manifest verification | dataset ID exact; 360 files |
| external CI profile with committed `--expect` | passed; 166,821 executed; not skipped |
| external full profile with committed `--expect` | passed; 1,443,876 executed; not skipped |
| generated CI/full `cmp` and SHA-256 checks | byte-identical |
| `check_encoding.py --expect utf8 --exclude hlp/` | 0 violations |
| `check_eol.py --enforce` | 0 violations |
| `check_case.py` | 0 findings |
| `find_unreferenced.py --report` | 69 accepted pre-existing reference/utility findings |
| `git diff --check` | passed |

The master plan requests `golden_smoke.sh --check`, but no such tracked script
or M23 golden exists in this repository, as already recorded by M42 and M44.
The accepted M42 fixtures, graph, trace, and ordinary smoke/selftest gates are
the available replacements and all passed.

Two command corrections are retained as audit evidence. The first full-profile
run used a differently named temporary failure-sidecar directory. All record
counts, every canonical sidecar SHA, and the failure index digest were exact,
but `--expect` correctly rejected the differing relative path strings. A
second complete run used the canonical `v20_native_full_failures` basename;
`--expect` and byte comparison both passed. The first Wine attempt also
refused to create a prefix beneath an unowned `/tmp`; creating the dedicated
owned prefix first produced passing selftest and smoke runs.

## Files and milestone boundary

Implementation commit `866d094ca5027ca273a2ca3fd1d33108d29c5f9c`
changes the active reset/scheduler/diagnostic selector sites, removes the
approved `i286c_step()` entry point, updates test call sites, adds the dynamic
reset check, and adds the static invariant guard. Documentation and this report
are a separate evidence concern; the final documentation SHA is reported in
the completion handoff.

No file was deleted. No binary, ROM, disk, font, icon, cursor, wave, private
integration asset, frozen reference file, M42 golden, M43 baseline, or state
format was changed. No bug-fix-ledger entry is added because M45 is a
behavior-preserving structural fold and does not correct a guest-visible
defect.

M46 has not begun and must not begin before explicit G45 acceptance.

## G45 human gate

The remaining gate is the standard maintainer run from a clean checkout of the
final SHA:

```text
[ ] V3 mode starts
[ ] bundled VA demo runs normally
[ ] OS boots
[ ] keyboard input works
[ ] FDD reads work
[ ] FDD write and reread work, if practical
[ ] Sound Board II works
[ ] execution continues after save/load
[ ] reset reboots normally
[ ] no test-only log, worker, scenario, or trace seam appears in a normal run
```

Only the maintainer can pass G45. Until then, the roadmap remains at
`G45 human review` and M46 is unauthorized.
