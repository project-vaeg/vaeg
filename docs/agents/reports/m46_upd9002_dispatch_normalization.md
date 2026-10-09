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
# M46 uPD9002 dispatch normalization

Status: implementation and required local and hosted machine validation are
complete.
The final evidence SHA and final remote SHA are recorded in the handoff because
the report cannot contain the SHA of the commit that contains itself. This
report does not declare G46 passed and does not authorize M47.

## Identity, prerequisite, and commits

- Branch: `topic/m46-upd9002-dispatch-normalization`
- Accepted G45 starting SHA:
  `5d1880c9446d05e863011df41e629801c9328779`
- M45 implementation SHA:
  `866d094ca5027ca273a2ca3fd1d33108d29c5f9c`
- Final M46 implementation SHA:
  `587901f2ac52d9eaee3458df9d38c1d13d9ba52d`
- Remote SHA at implementation-validation time:
  `587901f2ac52d9eaee3458df9d38c1d13d9ba52d`
- Hosted implementation run:
  [GitHub Actions 29634445632](https://github.com/nakatamaho/vaeg/actions/runs/29634445632),
  7/7 jobs successful at the M46 implementation SHA
- Accepted G41 tag `pre-upd9002-series`:
  `dc8a72da974f0ea328613e480f1de662c28f4436`
- Accepted G43 tag `pre-upd9002-refactor`:
  `91ec9a4c998928523360c37dab8d6ade8e698731`

The maintainer explicitly approved G45 and directed work only on M46. The
branch was created from the exact accepted G45 SHA with a clean worktree. The
local and remote G45 SHA matched before implementation. No reset, rebase,
squash, or rewrite was performed, and no M47 work was started.

The implementation commits are:

1. `aff0e5f82b59e3ea6f448d7f60c4216a394080e3` — centralize V30 dispatch
   construction;
2. `b042d06d5fa9b94c0182db355108a5a4081b59a8` — enforce post-init dispatch
   immutability;
3. `d7e95901be40bb557e7e84c9ec5fae5762c580b3` — retire legacy block
   executors;
4. `5530af12d03a540b1e0924a6efcd3c87ab32891a` — remove `CPU_EXEC` macro
   remnants;
5. `587901f2ac52d9eaee3458df9d38c1d13d9ba52d` — add dispatch-normalization
   regression checks.

## Pre-change reference and write inventory

The audit at the accepted G45 SHA found the following complete block-executor
and constructor surface in the active tree:

| Item | Definition/declaration/macro | Calls, address-taking, or pointer use |
|---|---|---|
| `i286c()` | declaration `i286c/cpucore.h:261`; definition `i286c/i286c.c:289-316` | no call, macro expansion, function-pointer use, or address-taking reference |
| `v30c()` | declaration `i286c/cpucore.h:263`; definition `i286c/v30patch.c:1411-1438` | no call, macro expansion, function-pointer use, or address-taking reference |
| `CPU_EXEC` | `i286c/cpucore.h:340`, alias of `i286c` | no active use |
| `CPU_EXECV30` | `i286c/cpucore.h:341`, alias of `v30c` | no active use |
| `v30cinit()` | declaration `i286c/v30patch.h:2`; definition `i286c/v30patch.c:1389-1409` | sole production call `i286c/i286c.c:152`; no address-taking reference |

Both block executors were linkable dead symbols, but supported production and
test object relocation maps contained no caller. `pccore_exec()` already
called only `v30c_step()` after M45. No supported preset selected a block
executor, `i286x`, or a 286 instruction path.

All six roots were translation-unit-local arrays in `i286c/v30patch.c`:

| Root | Size | Runtime function-pointer reads before M46 | Constructor writes before M46 |
|---|---:|---|---|
| `v30op` | 256 | four prefix dispatches, three dead `v30c()` loop sites, and the active `v30c_step()` dispatch | base copy, then `v30patch_op` |
| `v30op_repne` | 256 | five REPNE/prefix dispatches | base copy, then `v30patch_repne` |
| `v30op_repe` | 256 | five REPE/prefix dispatches | base copy, then `v30patch_repe` |
| `v30op_repc` | 256 | five REPC/prefix dispatches | 256-entry reserved fill, then `v30patch_repc` |
| `v30ope0xf6_table` | 8 | the F6 group dispatch | base copy, then explicit V30 DIV8 and IDIV8 writes |
| `v30ope0xf7_table` | 8 | the F7 group dispatch | base copy, then explicit V30 DIV16 and IDIV16 writes |

The complete live-table write set was confined to the helper and constructor
at pre-change `i286c/v30patch.c:1379-1408`: helper assignment at line 1382;
base copies at 1393, 1395, 1397, 1399, and 1402; patch calls at 1394, 1396,
1398, and 1408; DIV/IDIV replacements at 1400, 1401, 1403, and 1404; and the
REPC fill at 1406. Array-to-pointer conversions occur only to pass those roots
to the base-copy or patch helper; no root address escapes the translation unit.

The pre-change lifecycle count was:

| Lifecycle operation | Constructor calls |
|---|---:|
| process/core initialization through `pccore_init()` → `CPU_INITIALIZE()` → `i286c_initialize()` | 1 |
| ordinary machine initialization after that process setup | 0 additional |
| `pccore_reset()` / `i286c_reset()` | 0 |
| ROM-less selftest | 1, through its single `pccore_init()` |
| state load | 0 |
| CPU_SHUT | 0 |

There was no material conflict between the repository, ADR-0012, and the M46
task. The roadmap and ADR still showed G45 at human review, but the maintainer's
explicit approval resolved that documentation lag before implementation.

## Post-change construction flow

`v30cinit()` remains the one explicit construction entry point pending its M49
public rename. Its operation order is unchanged from M42:

1. copy `i286op` to `v30op`, then apply `v30patch_op`;
2. copy `i286op_repne`, then apply `v30patch_repne`;
3. copy `i286op_repe`, then apply `v30patch_repe`;
4. copy `c_ope0xf6_table`, then replace DIV8 and IDIV8;
5. copy `c_ope0xf7_table`, then replace DIV16 and IDIV16;
6. fill `v30op_repc` with `v30_reserved_repc`, then apply `v30patch_repc`.

A translation-unit-local initialized guard permits exactly one successful
construction per process. Re-entry returns before any table write. It is not
selected by `cpu_type` or any build option. Production has one syntactic call
site, in `i286c_initialize()`. Reset, selftest after initialization, state
load, and CPU_SHUT have no construction call.

The dedicated runtime QA observes one successful construction, proves that an
ordinary reset neither reconstructs nor mutates a root, invokes the entry
point a second time, and observes one rejected re-entry with the successful
construction count still one:

```text
upd9002-dispatch-normalization: constructed=1 rejected=1 roots=256,256,256,256,8,8 immutable
```

The fail-closed static inventory reports:

```text
upd9002-dispatch-normalization-static: constructor=v30cinit production-calls=1 successful-constructions=1
upd9002-dispatch-normalization-static: roots=v30op:256,v30op_repne:256,v30op_repe:256,v30op_repc:256,v30ope0xf6_table:8,v30ope0xf7_table:8
upd9002-dispatch-normalization-static: writes=constructor-only snapshot=element-wise-equality reset=selftest=state-load=checked
```

It parses the tracked and untracked active sources, rejects alternative
constructors, checks the exact M42 construction-operation order, enumerates
every live-root write, verifies the one production call, and checks the
reset/selftest/state-load/dedicated-QA hooks.

## Six-table immutability proof

Tests-enabled builds take a test-only snapshot immediately after the final
construction write. Four arrays contain 256 `I286OP` pointers each and two
contain eight `I286OPF6` pointers each, for 1,040 function-pointer elements.
Every comparison is element-wise function-pointer `==`. No `memcmp`, pointer
serialization, address output, symbol lookup, pointer hash, or
address-dependent golden data is used.

The complete live snapshot is checked:

- in `i286c_reset()` before ordinary reset changes CPU state;
- by selftest after initialization and reset;
- by selftest again after `statsave_load()`;
- by the dedicated M46 QA before reset, after reset, and after rejected
  constructor re-entry.

The tests-disabled product has no snapshot arrays, counters, verification
functions, QA command, diagnostic string, test source, or M46 testing define.
The only writes after construction are to the test-only snapshot storage;
none targets a live dispatch table.

## Source-level graph and provenance identity

The M42 generator and selftest passed. File-scoped comparison against accepted
M42 commit `336227f093d37e3c60bc50333216d66668755cef` proved byte identity for
all three required artifacts:

| Artifact | SHA-256 | Result |
|---|---|---|
| `upd9002_final_dispatch_graph.csv` | `e8c8fded6e1f44ef2d74ace93ba1063d64464b217bdd4f9180bf50e86e19df3d` | byte-identical |
| `upd9002_dispatch_provenance_m42.csv` | `605e9af6761e3069f6c5af0ad9647ae574ab98252f7373969586cfee892cd6ba` | byte-identical |
| `upd9002_support_map_m42.csv` | `7b856f95d8c3d0356325483d188b9dde2d03a8e6cbe9b7fd0cacd004f1d09d13` | byte-identical |

No M42 baseline was regenerated or modified, and no M46 provenance transition
report is needed.

## Block-executor and macro disposition

The exact approved source-level deletion list was:

- `i286c()` declaration and definition;
- `v30c()` declaration and definition;
- `CPU_EXEC` alias;
- `CPU_EXECV30` alias.

No file was deleted. The removal was split into the executor-focused and
macro-focused commits listed above. Post-change C/header searches contain no
definition, call, macro, function-pointer reference, or address-taking
reference. GCC and production archive/executable symbol inspection contains
neither exact symbol. `v30c_step()` remains defined and is still the sole
active execution primitive. Its body and instruction semantics were not
changed.

`i286c/i286c_0f.c`, all protected-mode/system handlers, descriptor state,
selector checks, interrupt/return paths, and overwritten base handlers are
unchanged. No base slot was redirected to `_reserved`, and no dead instruction
handler was removed. Those decisions remain entirely in M47/M48.

## CPU_SHUT and M44 state-boundary preservation

`i286c_shut()` still clears exactly through `offsetof(I286STAT, cpu_type)`,
calls `i286c_initreg()`, and applies the M44 adapter's historical shutdown
image operation. The 286-style initializer was not removed with `i286c()`.
Fresh reset retains FLAGS `f002`; CPU_SHUT retains FLAGS `0000`.

A fresh detached G41 worktree at
`dc8a72da974f0ea328613e480f1de662c28f4436` was built with only the accepted
test-only raw-`I286STAT` scenario driver. The driver uses exactly three
`v30c_step()` calls. G41 generated all three states, M46 generated all three,
each consumer loaded and immediately re-saved the other's states, and the
matrix tool reported all four comparisons true. The disposable G41 patch was
then removed and its worktree returned to a clean detached state.

| Scenario | CPU286 SHA-256 | CPU type | UPD9002 SHA-256 |
|---|---|---:|---|
| reset | `6cbd3314a586ccf9c5f1d11a17b8836f76f1deaf10814de8b61ebdf96a792f89` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| executed-3 | `45d06e424fd332027817ca66db7ed0298e87695cf0cfed5582b7fe1348faa7fe` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| CPU_SHUT | `7636def12b5f25c39540a1367166394a8308064cce74a15860c62a487a946ff7` | 1 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

Every CPU286 payload remains 112 bytes and every UPD9002 payload remains 16
bytes. The G41/M46 matrix, invalid `cpu_type`, declared-size 111, seven-byte
truncation, opaque padding, transactional preflight, and atomic rejection tests
all passed. No state type, layout, section tag, version, adapter operation, or
payload was changed.

## M42-M45 preservation results

M42 preservation is exact. In addition to the three dispatch artifacts, the
156-case harness manifest, trace baseline, three state fixtures, and G41 ABI
fixture are byte-identical to the accepted M42 commit. Their SHA-256 values
remain respectively:

- harness manifest:
  `14769ca59bf589921cbcf88dc0c357937f6da36e4fe0b78f9a46dc75b9098f2a`;
- trace baseline:
  `17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba`;
- state fixtures:
  `c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb`;
- ABI fixture:
  `f6995d59a293a4e81fc710400fd447031016322018a1c88c9e752c1eb1889db7`.

The direct harness, trace equivalence, ABI test, reset/executed-3/CPU_SHUT
fixtures, and ROM-less selftest all passed.

M43 preservation used the verified 360-file external dataset with identity:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

| Profile | Executed | Pass | Semantic failure | Timeout | Crash | Signature index |
|---|---:|---:|---:|---:|---:|---|
| CI | 166,821 | 156,228 | 10,593 | 0 | 0 | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` |
| full empty-prefetch | 1,443,876 | 1,359,547 | 84,329 | 0 | 0 | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |

Both independent runs used committed `--expect`; both generated summaries and
all generated failure sidecars are byte-identical to the committed baselines.
The known-gap artifact remains byte-identical at SHA-256
`11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1`,
with exactly 40 rules and 68,626 resolved record hashes. No record
classification, canonical failure signature, failure set, or termination
class changed.

M45 preservation is reported by the fail-closed native invariant:

```text
upd9002-native-invariant: presets=13 core=i286c selectors=absent
upd9002-native-invariant: reset=v30c_initreg step=v30c_step shutdown=i286c_initreg
upd9002-native-invariant: cpu_type=i286c/cpucore.h=2,i286c/i286c.c=3,i286c/upd9002_state.c=7,i286c/upd9002_state.h=1 control=state-validation-only
upd9002-native-invariant: block-executors=absent cpu-exec-macros=absent
```

No `CPU_TYPE`-controlled execution returned, no `i286c_step()` returned, all
13 supported presets still share the C core, and normal reset remains
V30-native. CPU_SHUT remains the documented initializer-only exception.

## Build, test, and production isolation

| Configuration | Result |
|---|---|
| GCC 15.2.0 `linux-ci-gcc` | clean configure/build; 29/29 CTests passed; external CI profile executed, not skipped, in 179.08 s |
| Clang 21.1.8 `linux-ci-clang` | clean configure/build; 29/29 CTests passed; external corpus visibly skipped because GCC and both independent runs used the real corpus |
| GCC 15.2.0 `linux-ci-asan` | clean configure/build; 29/29 CTests passed with leak detection disabled; focused dispatch/state tests also passed 3/3 with `UBSAN_OPTIONS=halt_on_error=1` |
| GCC `linux-debug`, `linux-asan` | clean configure/build passed |
| GCC 15.2.0 `linux-release` | clean tests-disabled configure/build; selftest and smoke passed |
| MinGW-w64 GCC 13 `mingw-cross` | clean configure/build passed; `vaeg.exe` is PE32+ GUI x86-64 |
| Wine 10.0 | MinGW production selftest and smoke passed |
| [GitHub Actions 29634445632](https://github.com/nakatamaho/vaeg/actions/runs/29634445632) | 7/7 passed at `587901f2ac52d9eaee3458df9d38c1d13d9ba52d`: repository invariants, Ubuntu GCC, Clang and ASan, macOS FetchContent, Windows MSYS2 MinGW, and standalone conformance |

The Linux production cache contains `VAEG_ENABLE_TESTS=OFF` and
`VAEG_Z80_INTEGRATION_TRACE=OFF`. Its Ninja command graph contains neither
`dispatch_normalization.c` nor `VAEG_UPD9002_M46_TESTING`. String and symbol
inspection found no M46 QA option, diagnostic, snapshot, counter, test hook,
`i286c`, or `v30c`. It retained `v30cinit` and `v30c_step`, as required.

Hosted Linux, Windows, macOS, repository-invariant, and standalone-conformance
jobs all passed against the exact implementation SHA.

## Commands and exit statuses

Unless explicitly noted as a platform limitation, every command below exited
zero.

```text
git fetch origin
git switch -c topic/m46-upd9002-dispatch-normalization 5d1880c9446d05e863011df41e629801c9328779
git rev-parse HEAD
git status --short

python3 tools/qa/upd9002_dispatch_normalization.py --root .
python3 tools/qa/upd9002_native_invariant.py --root .
python3 tools/qa/upd9002_dispatch.py --root .
python3 tools/qa/upd9002_dispatch.py --root . --selftest
build/linux-ci-gcc/sdl2/vaeg --upd9002-m46-dispatch-qa

cmake --preset linux-debug --fresh
cmake --preset linux-release --fresh
cmake --preset linux-ci-gcc --fresh
cmake --preset linux-ci-clang --fresh
cmake --preset linux-asan --fresh
cmake --preset linux-ci-asan --fresh
cmake --preset mingw-cross --fresh
cmake --build build/linux-debug --parallel 2
cmake --build build/linux-release --parallel 2
cmake --build build/linux-ci-gcc --parallel 2
cmake --build build/linux-ci-clang --parallel 2
cmake --build build/asan --parallel 2
cmake --build build/linux-ci-asan --parallel 2
cmake --build build/mingw-cross --parallel 2

ctest --test-dir build/linux-ci-gcc --output-on-failure
ctest --test-dir build/linux-ci-clang --output-on-failure
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=0 \
  ctest --test-dir build/linux-ci-asan --output-on-failure
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build/linux-ci-asan --output-on-failure \
    -R 'vaeg_upd9002_(dispatch_normalization|state_boundary|state_payload_probe)$'

python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m42.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --profile ci --shard-timeout 120 \
  --output /tmp/vaeg-m46-ssts/v20_native_ci.json \
  --failure-directory /tmp/vaeg-m46-ssts/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m42.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /tmp/vaeg-m46-ssts/v20_native_full.json \
  --failure-directory /tmp/vaeg-m46-ssts/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
cmp /tmp/vaeg-m46-ssts/v20_native_ci.json \
  tests/ssts/baseline/v20_native_ci.json
cmp /tmp/vaeg-m46-ssts/v20_native_full.json \
  tests/ssts/baseline/v20_native_full.json
diff -qr /tmp/vaeg-m46-ssts/v20_native_ci_failures \
  tests/ssts/baseline/v20_native_ci_failures
diff -qr /tmp/vaeg-m46-ssts/v20_native_full_failures \
  tests/ssts/baseline/v20_native_full_failures

git worktree add --detach /tmp/vaeg-m46-g41-dc8a72da \
  dc8a72da974f0ea328613e480f1de662c28f4436
cmake -S /tmp/vaeg-m46-g41-dc8a72da \
  -B /tmp/vaeg-m46-g41-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_INTEGRATION_TRACE=ON -DVAEG_ENABLE_ARCHIVE_DROP=OFF \
  -DVAEG_WERROR=OFF
cmake --build /tmp/vaeg-m46-g41-build --parallel 4
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  VAEG_M44_SCENARIO_DIR=/tmp/vaeg-m46-matrix.OSib21/g41-generated \
  /tmp/vaeg-m46-g41-build/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  VAEG_M44_SCENARIO_DIR=/tmp/vaeg-m46-matrix.OSib21/m46-generated \
  build/linux-ci-gcc/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  VAEG_M44_SCENARIO_INPUT_DIR=/tmp/vaeg-m46-matrix.OSib21/g41-generated \
  VAEG_M44_SCENARIO_OUTPUT_DIR=/tmp/vaeg-m46-matrix.OSib21/g41-to-m46 \
  build/linux-ci-gcc/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  VAEG_M44_SCENARIO_INPUT_DIR=/tmp/vaeg-m46-matrix.OSib21/m46-generated \
  VAEG_M44_SCENARIO_OUTPUT_DIR=/tmp/vaeg-m46-matrix.OSib21/m46-to-g41 \
  /tmp/vaeg-m46-g41-build/sdl2/vaeg --selftest

python3 tools/qa/upd9002_state_matrix.py \
  --fixtures tests/upd9002/state_fixtures_m42.txt \
  --g41-generated /tmp/vaeg-m46-matrix.OSib21/g41-generated \
  --m44-generated /tmp/vaeg-m46-matrix.OSib21/m46-generated \
  --g41-to-m44 /tmp/vaeg-m46-matrix.OSib21/g41-to-m46 \
  --m44-to-g41 /tmp/vaeg-m46-matrix.OSib21/m46-to-g41 \
  --g41-sha dc8a72da974f0ea328613e480f1de662c28f4436 \
  --m44-sha 587901f2ac52d9eaee3458df9d38c1d13d9ba52d

git diff --exit-code 336227f093d37e3c60bc50333216d66668755cef -- \
  tools/qa/golden/upd9002_final_dispatch_graph.csv \
  tools/qa/golden/upd9002_dispatch_provenance_m42.csv \
  tools/qa/golden/upd9002_support_map_m42.csv \
  tests/upd9002/harness_manifest.csv \
  tests/upd9002/trace_baseline.txt \
  tests/upd9002/state_fixtures_m42.txt tests/upd9002/abi_g41.txt

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  build/linux-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  build/linux-release/sdl2/vaeg --smoke
WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m45-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --selftest
WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m45-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --smoke

python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check
gh run view 29634445632 --json status,conclusion,url,headSha,jobs
```

The repository checks reported zero encoding, EOL, or case violations and a
clean diff check. The unreferenced report retained the same 69 accepted
reference/utility findings; neither new M46 test file is unreferenced.

The native Windows and macOS presets cannot be configured on the Linux host;
their clean builds and tests are delegated to hosted CI. The master plan names
`golden_smoke.sh --check`, but that script and an M23 golden are not tracked in
this repository, as already recorded by M42, M44, and M45. The accepted M42
fixtures, direct harness, trace, selftest, and ordinary smoke gates are the
available replacements and all passed.

## Files changed and milestone boundary

The implementation changes:

- `CMakeLists.txt`;
- `i286c/cpucore.h`;
- `i286c/i286c.c`;
- `i286c/v30patch.c`;
- `i286c/v30patch.h`;
- `sdl2/np2.c`;
- `sdl2/selftest.c`;
- new `tests/upd9002/dispatch_normalization.c` and `.h`;
- new `tools/qa/upd9002_dispatch_normalization.py`;
- `tools/qa/upd9002_native_invariant.py`.

The evidence update changes this report, `docs/agents/ROADMAP.md`, and
`docs/agents/DECISIONS/ADR-0012-upd9002-ownership.md`.

No binary payload, ROM or disk image, accepted M42/M43 golden, state fixture,
frozen reference file, public API name, CPU file path, protected-mode handler,
or guest behavior changed. No bug-fix ledger entry is added because M46 is a
behavior-preserving structural normalization, not a guest-visible correctness
fix.

## Deviations and unresolved risks

- Local macOS and native MSYS2 execution were unavailable; hosted CI supplies
  those platform results.
- Sanitizer leak detection is disabled under the managed ptrace environment.
  The established non-halting full run retains accepted unrelated sound/vendor
  diagnostics; the focused M46 dispatch/state run passes with undefined
  behavior configured to halt.
- Existing compiler warnings in untouched protected-mode, display, sound, and
  vendored code remain outside M46.
- Runtime dispatch construction remains intentionally non-thread-safe during
  process startup, matching the accepted single-threaded lifecycle. Future
  multithreading and a cycle-budget run API remain outside this series.
- Human V3 boot, VA demo, OS, media, input, sound, and save/load testing remains
  mandatory. Machine results do not pass G46.

## G46 human-review checklist

```text
[ ] clean checkout is the reported final branch SHA
[ ] V3 mode starts
[ ] bundled VA demo runs normally
[ ] OS boots
[ ] keyboard input works
[ ] FDD reads work
[ ] FDD write and reread work, if practical
[ ] Sound Board II works
[ ] execution continues after save/load
[ ] reset reboots normally
[ ] no test-only log, QA command, scenario, worker, or trace seam appears
[ ] constructor review confirms one successful process-lifetime construction
[ ] six-table review confirms element-wise post-init immutability coverage
[ ] deletion review confirms only i286c()/v30c() and CPU_EXEC aliases were retired
```

Only the maintainer can pass G46. M47 must not begin before that explicit
approval.
