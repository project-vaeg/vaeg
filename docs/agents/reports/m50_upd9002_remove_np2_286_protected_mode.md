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
# M50 remove approved NP2 286 protected-mode groups

Status: the three and only three groups explicitly approved at G49 have been
deleted. Local and hosted preservation validation are complete. The G50 human
gate remains external to this report; this report does not declare G50 passed
and M51 has not begun.

The final report/remote SHA is supplied in the handoff because a report cannot
contain the SHA of the commit that contains itself.

## Identity and scope

- Branch: `topic/m50-upd9002-remove-protected-mode`
- Exact starting and approved G49 SHA:
  `2a21a5264a3830f5a393ed7fbd3fbe1e900f2926`
- Accepted G49 branch:
  `topic/m49-upd9002-protected-mode-reachability`
- Pre-report implementation and QA SHA:
  `303e6cb3f128fbc0300d1a1943887cce36a2944b`
- Hosted evidence and remote report SHA:
  `8bb5b1e2c5f3cff0f19b1cfd649f0f97f4c6329a`
- Hosted evidence run:
  [build 29662304823](https://github.com/nakatamaho/vaeg/actions/runs/29662304823)
  — 7/7 jobs successful
- Final and remote SHA: supplied in the handoff after hosted validation

The worktree started clean at the exact approved G49 SHA. No reset, rebase,
squash, force push, or M42--M49 history rewrite was performed. M50 changes no
instruction, state, timing, prefetch, DMA, interrupt, exception, memory, I/O,
reset, CPU_SHUT, or REP+0F diagnostic semantics. M51 rename/move work is out of
scope and has not started.

## Commits and files

Implementation commits before this report are:

1. `0cd8358` — `M50: delete approved ARPL group`;
2. `3bde734` — `M50: delete approved legacy segment-load group`;
3. `593d013` — `M50: delete approved CTS system group`;
4. `303e6cb` — `M50: add protected-mode deletion regression checks`.

Paths changed before this report are:

```text
CMakeLists.txt
i286c/i286c.h
i286c/i286c_0f.c                         (deleted)
i286c/i286c_mn.c
tools/qa/golden/upd9002_286_deletion_manifest_m50.csv
tools/qa/golden/upd9002_dispatch_provenance_m50.csv
tools/qa/upd9002_dispatch.py
tools/qa/upd9002_protected_deletion.py
tools/qa/upd9002_rep0f_transition.py
```

This report adds:

```text
docs/agents/reports/m50_upd9002_remove_np2_286_protected_mode.md
```

No frozen-reference or binary payload path changed. No runtime or serialized
state field was removed.

## Approved, deleted, and deferred scope

The accepted M49 inventory named exactly 52 candidate rows. The deterministic
M50 deletion manifest contains the same 52 stable IDs and no others:

| Approved group | Rows | Deleted/replaced result |
|---|---:|---|
| `M50-PM-ARPL` | 4 | `_arpl` deleted; three slot-63 base targets replaced by `_reserved` |
| `M50-PM-MOV-SEG-EA` | 4 | `_mov_seg_ea` deleted; three slot-8E base targets replaced by `_reserved` |
| `M50-PM-CTS-SYSTEM` | 44 | CTS translation unit/private closure deleted; three slot-0F base targets replaced by `_reserved` |

The manifest identity is:

```text
tools/qa/golden/upd9002_286_deletion_manifest_m50.csv
SHA-256 d037e91bade95c77f120da9aba9f927279e404bf99fc0a4744af1d254a76f2dc
52 candidates
```

All M49 deferred/rejected members remain deferred or rejected. In particular,
M50 retains `v30mov_seg_ea`, `SEGSELECT`, `i286c_selector`, `MSW`, `GDTR`,
`IDTR`, `LDTR`, `LDTRC`, `TR`, `TRC`, `Cpu286StateCompat`,
`Cpu286CompatImage`, the transactional state adapter, `CPU_MSW` I/O, reset,
interrupt/exception logic, CPU_SHUT compatibility, and both M48 diagnostic
handlers. The frozen `i286x/` tier remains untouched.

## Pre-deletion revalidation

Before editing, the accepted M49 graph, source counts, macro uses, state uses,
object relocations, function sections, linker map, tests, and production
symbols were rechecked at the approved starting SHA.

The final six-table graph had no edge to any approved member. The only incoming
production addresses were the nine constructor-base entries later replaced by
placeholders:

```text
i286op/i286op_repe/i286op_repne [0x63] -> _arpl
i286op/i286op_repe/i286op_repne [0x8e] -> _mov_seg_ea
i286op/i286op_repe/i286op_repne [0x0f] -> i286c_cts
```

All nine entries were overwritten by accepted native/diagnostic patches before
publication and before the M46 immutable snapshot. There was no direct caller,
alternate constructor, lifecycle root, callback, I/O root, supported test
caller, or post-construction write. The CTS dispatcher was the only consumer
of `cts0_table` and `cts1_table`; the tables and the direct LOADALL286 case
closed the private translation-unit dependency graph.

The pre-deletion function-section ELF and map at
`/home/maho/vaeg/build/m50-pre-link` contained `_arpl`, `_mov_seg_ea`,
`i286c_cts`, both CTS tables, all non-inlined CTS handlers, and retained
`i286c_selector`/`v30mov_seg_ea`. The map SHA-256 was
`ddb9801d4ca42cb1eb9f978d08ab8ba0b1363897316ec2d2363676dbf62c3085`.
Linker presence was corroborating evidence only; the graph, direct-reference,
state, lifecycle, and diagnostic proofs established deletion safety.

No accepted candidate drifted, became shared, or required deferral.

## Per-group deletion and gates

### M50-PM-ARPL

Deleted `_arpl` only. The three 256-entry base arrays retain their indices and
now use the existing `_reserved` constructor placeholder at slot 63. The final
constructor patches still publish `v30_reserved` at all three native roots.

After this commit, compilation, final graph/support identity, the exact
one-group provenance transition, M46 constructor/immutability QA, M48 522-case
atomic diagnostic QA, ABI/state fixtures, trace, and M43 accepted baseline
checks passed.

### M50-PM-MOV-SEG-EA

Deleted only legacy `_mov_seg_ea`. The three base arrays now use `_reserved`
at slot 8E, while all final roots still receive active `v30mov_seg_ea`.
`SEGSELECT`, `i286c_selector`, and protected state ownership were not changed.

After this commit, the same focused preservation gates passed. Direct and
symbol checks distinguish deleted `_mov_seg_ea` from retained
`v30mov_seg_ea`.

### M50-PM-CTS-SYSTEM

Deleted exactly:

- `i286c_cts` and its declaration;
- `cts0_table`, `cts1_table`, and their 16 entries;
- 13 unique private system handlers: `_sldt`, `_str`, `_lldt`, `_ltr`,
  `_verr`, `_verw`, `_sgdt`, `_sidt`, `_lgdt`, `_lidt`, `_smsw`, `_lmsw`,
  and `_loadall286`;
- private header dependencies `I286_0F`, `I286OP_0F`, `I286_IDTR`,
  `I286_LDTR`, `I286_TR`, and `I286_TRC`;
- `i286c/i286c_0f.c` and its one CMake source edge.

The three base slot-0F entries now use `_reserved`. Final unprefixed 0F still
publishes `v30_ope0x0f`; REPNE/REPE 0F still publish the two M48 diagnostic
stops. The focused runtime gates passed after the deletion. Static source
absence checks were intentionally run again after the deletion commit so Git's
tracked-file inventory reflected the committed file removal; they then passed.

### Deletion size

M50 removes one 288-line production translation unit and 16 function
definitions in total: `_arpl`, `_mov_seg_ea`, `i286c_cts`, and 13 CTS system
handlers. It removes two secondary tables, 16 secondary entries, six private
header aliases/types, one declaration, and one CMake edge. Nine base entries
are replaced in place. Runtime fields removed: zero. Serialized bytes, tags,
sizes, offsets, and padding removed: zero.

Across the four pre-report commits, production paths contain 371 deleted and
15 added lines; audit tooling and deterministic golden evidence account for
the remaining diff.

## Final dispatch graph and construction provenance

The accepted post-M48 final graph regenerates byte-identically:

```text
tools/qa/golden/upd9002_final_dispatch_graph_m48.csv
SHA-256 fe9df28ad3d51cc55235afc3979ada890e86158b294762c15fa33c20d8a800a6
```

The accepted post-M48 support map also remains byte-identical at
`21dd037c3eb11e1674805ad456ef03663f17804affbd7382c8db77291ab25279`.
The M42 and M48 accepted graph/provenance/support artifacts themselves were not
rewritten.

M50 records the intentional source-construction provenance transition in a
new artifact:

```text
tools/qa/golden/upd9002_dispatch_provenance_m50.csv
SHA-256 30246dbd2bbf95b406a6bb05a182d16ea04f56dba48dca73842f5d06b74aae2c
```

Exactly nine rows change, with no final target change:

| Base table | Slots | Old base target | New base target | Final patch target |
|---|---|---|---|---|
| `i286op` | 0F / 63 / 8E | `i286c_cts` / `_arpl` / `_mov_seg_ea` | `_reserved` | `v30_ope0x0f` / `v30_reserved` / `v30mov_seg_ea` |
| `i286op_repne` | 0F / 63 / 8E | same | `_reserved` | REPNE diagnostic / `v30_reserved` / `v30mov_seg_ea` |
| `i286op_repe` | 0F / 63 / 8E | same | `_reserved` | REPE diagnostic / `v30_reserved` / `v30mov_seg_ea` |

The constructor remains the single M46 path. Runtime QA reports one successful
construction, one rejected repeat construction, roots
`256,256,256,256,8,8`, and element-wise immutability.

## Post-deletion source, object, linker, and production proof

The deterministic checker fails if any approved member remains active, any
placeholder/final patch changes, any known member is omitted, any group grows,
or any accepted M42/M48 artifact changes unexpectedly. Its negative selftests
cover a duplicate candidate, an invalid retained-replacement action, a
host-dependent manifest value, and malformed dispatch-parser inputs.

Post-deletion production object inventories contain no `i286c_0f.c.o`.
ELF and PE symbol searches contain none of the deleted functions/tables.
The function-section/linker-map build at
`/home/maho/vaeg/build/m50-post-link` likewise contains no approved member;
its map SHA-256 is
`f59b89419d412822d8b0319af03199c5a8b207a9b1cdd4e869f85740cc3c384e`.
It still contains `v30mov_seg_ea` and `i286c_selector`, proving the required
similarly named/shared paths were retained.

Both tests-disabled production caches have `VAEG_ENABLE_TESTS=OFF` and
`VAEG_Z80_INTEGRATION_TRACE=OFF`. Their Ninja graphs and ELF/PE strings contain
no M50 audit source, manifest identity, audit diagnostic, M46 test CLI, or M48
test CLI. The production M48 diagnostic API and exact guest-facing message
remain by design.

## State, reset, CPU_SHUT, and diagnostic preservation

`Cpu286StateCompat` remains 112 bytes and the UPD9002 section remains 16 bytes.
`Cpu286CompatImage`, opaque PE-clear residue, transactional preflight/commit,
MSW.PE rejection, and all field offsets remain unchanged. No runtime field was
removed or converted to opaque-only ownership.

The four-way G41/current matrix reports all booleans true. Payload identities
remain:

| Scenario | CPU286 SHA-256 | UPD9002 SHA-256 |
|---|---|---|
| reset | `6cbd3314a586ccf9c5f1d11a17b8836f76f1deaf10814de8b61ebdf96a792f89` | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| executed-3 | `45d06e424fd332027817ca66db7ed0298e87695cf0cfed5582b7fe1348faa7fe` | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| CPU_SHUT | `7636def12b5f25c39540a1367166394a8308064cce74a15860c62a487a946ff7` | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

Reset FLAGS remains f002. The historical CPU_SHUT initializer and FLAGS 0000
result remain exact. The M48 suite reports all 522 F2/F3+0F cases
state-and-memory atomic, and the diagnostic string and pre-mutation stop path
are unchanged.

## M42--M49 preservation

The original M42 immutable artifacts remain byte-identical:

| Artifact | SHA-256 |
|---|---|
| M42 final graph | `e8c8fded6e1f44ef2d74ace93ba1063d64464b217bdd4f9180bf50e86e19df3d` |
| M42 provenance | `605e9af6761e3069f6c5af0ad9647ae574ab98252f7373969586cfee892cd6ba` |
| M42 support map | `7b856f95d8c3d0356325483d188b9dde2d03a8e6cbe9b7fd0cacd004f1d09d13` |
| M42 156-case harness | `14769ca59bf589921cbcf88dc0c357937f6da36e4fe0b78f9a46dc75b9098f2a` |
| M42 trace baseline | `17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba` |
| M42 state fixtures | `c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb` |
| G41 ABI fixture | `f6995d59a293a4e81fc710400fd447031016322018a1c88c9e752c1eb1889db7` |

The M48 transition manifest remains byte-identical at
`4f3fefe8cbfb20a03364a80a0b917e475d3d545cab8eda6bee8a22c66e2147ee`.
The accepted M49 inventory remains byte-identical at
`f3843cd57b57af8f5baa4a180a7a30c88d628d0b12865d6a4a451a306794c15b`.

The pinned M43 dataset identity remains:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-
1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

The known-gap artifact remains byte-identical at
`11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1`,
with 40 selectors and 68,626 exact record hashes.

| Profile | Executed | Pass | Semantic failure | Timeout | Crash | Signature index |
|---|---:|---:|---:|---:|---:|---|
| CI | 166,821 | 156,228 | 10,593 | 0 | 0 | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` |
| full empty-prefetch | 1,443,876 | 1,359,547 | 84,329 | 0 | 0 | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |

Both independent `--expect` runs pass. Generated JSON and complete failure
sidecars compare byte-identically with their accepted baselines.

M45 still uses `v30c_step()` as the sole active primitive and has no
CPU_TYPE-controlled execution or `i286c_step`. M46 still has one constructor,
immutable six roots, and no `i286c()`, `v30c()`, `CPU_EXEC`, or `CPU_EXECV30`.
M48 diagnostic/state rejection and M49 accepted evidence remain exact.

## Toolchains and production artifacts

| Gate | Result |
|---|---|
| GCC `linux-ci-gcc` | fresh configure/build; CTest 33 passed, 1 external-data skip, 0 failed |
| Clang `linux-ci-clang` | fresh configure/build; CTest 33 passed, 1 external-data skip, 0 failed |
| GCC ASan/UBSan `linux-ci-asan` | fresh configure/build; same CTest result; focused halt-on-error 5/5 |
| GCC `linux-release` | fresh tests-disabled build; selftest and ROM-less smoke pass |
| MinGW-w64 `mingw-cross` | fresh tests-disabled build; Wine selftest and ROM-less smoke pass |
| function-section/linker map | tests-disabled build; approved symbols absent, retained symbols present |
| hosted Linux/Windows/macOS/standalone | [build 29662304823](https://github.com/nakatamaho/vaeg/actions/runs/29662304823): 7/7 successful at `8bb5b1e2c5f3cff0f19b1cfd649f0f97f4c6329a` |
| repository invariants | encoding 0, EOL 0, case 0, diff/frozen/binary checks pass; accepted unreferenced count 70 |

Production artifact identities before publication are:

| Artifact | Format | SHA-256 |
|---|---|---|
| Linux `vaeg` | ELF x86-64 PIE | `235c89a4d167c1c93ebe97046ed1794849a9482681b65c71f2978a17e046efd2` |
| MinGW `vaeg.exe` | PE32+ GUI x86-64 | `9951e733ac9574028112d7cef6da27d336ff2d3503e1c74d641d9eb5bce469c7` |

## Exact validation commands and statuses

All required commands below exited zero. Diagnostic environment attempts that
did not are listed separately under deviations.

```text
cmake --preset linux-debug --fresh
cmake --preset linux-release --fresh
cmake --preset linux-ci-gcc --fresh
cmake --preset linux-ci-clang --fresh
cmake --preset linux-asan --fresh
cmake --preset linux-ci-asan --fresh
cmake --preset mingw-cross --fresh

cmake -S . -B /home/maho/vaeg/build/m50-linux-ci-gcc \
  -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVAEG_ENABLE_TESTS=ON
cmake -S . -B /home/maho/vaeg/build/m50-linux-ci-clang \
  -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVAEG_ENABLE_TESTS=ON \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake -S . -B /home/maho/vaeg/build/m50-linux-ci-asan \
  -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_ENABLE_SANITIZERS=ON
cmake -S . -B /home/maho/vaeg/build/m50-linux-release \
  -G Ninja -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=OFF
cmake -S . -B /home/maho/vaeg/build/m50-mingw-cross \
  -G Ninja -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
cmake --build /home/maho/vaeg/build/m50-linux-ci-gcc --parallel 4
cmake --build /home/maho/vaeg/build/m50-linux-ci-clang --parallel 4
cmake --build /home/maho/vaeg/build/m50-linux-ci-asan --parallel 4
cmake --build /home/maho/vaeg/build/m50-linux-release --parallel 4
cmake --build /home/maho/vaeg/build/m50-mingw-cross --parallel 4

ctest --test-dir /home/maho/vaeg/build/m50-linux-ci-gcc --output-on-failure
ctest --test-dir /home/maho/vaeg/build/m50-linux-ci-clang --output-on-failure
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=0 \
  ctest --test-dir /home/maho/vaeg/build/m50-linux-ci-asan --output-on-failure
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir /home/maho/vaeg/build/m50-linux-ci-asan \
  --output-on-failure \
  -R 'vaeg_upd9002_(dispatch_normalization|rep0f_diagnostic_stop|protected_deletion|state_boundary|state_payload_probe)$'

python3 tools/qa/upd9002_protected_deletion.py --root .
python3 tools/qa/upd9002_protected_deletion.py --root . --selftest
python3 tools/qa/upd9002_rep0f_transition.py --root .
python3 tools/qa/upd9002_rep0f_transition.py --root . --selftest
python3 tools/qa/upd9002_native_invariant.py --root .
python3 tools/qa/upd9002_dispatch_normalization.py --root .
/home/maho/vaeg/build/m50-linux-ci-gcc/sdl2/vaeg \
  --upd9002-m46-dispatch-qa
/home/maho/vaeg/build/m50-linux-ci-gcc/sdl2/vaeg \
  --upd9002-m48-rep0f-diagnostic

python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /home/maho/vaeg/build/m50-linux-ci-gcc/sdl2/vaeg \
  --profile ci --shard-timeout 120 \
  --output /home/maho/vaeg/build/m50-ssts/v20_native_ci.json \
  --failure-directory /home/maho/vaeg/build/m50-ssts/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /home/maho/vaeg/build/m50-linux-ci-gcc/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /home/maho/vaeg/build/m50-ssts/v20_native_full.json \
  --failure-directory /home/maho/vaeg/build/m50-ssts/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
cmp /home/maho/vaeg/build/m50-ssts/v20_native_ci.json \
  tests/ssts/baseline/v20_native_ci.json
cmp /home/maho/vaeg/build/m50-ssts/v20_native_full.json \
  tests/ssts/baseline/v20_native_full.json
diff -qr /home/maho/vaeg/build/m50-ssts/v20_native_ci_failures \
  tests/ssts/baseline/v20_native_ci_failures
diff -qr /home/maho/vaeg/build/m50-ssts/v20_native_full_failures \
  tests/ssts/baseline/v20_native_full_failures

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
VAEG_M44_SCENARIO_DIR=/home/maho/vaeg/build/m50-matrix/current-dummy \
  /home/maho/vaeg/build/m50-linux-ci-gcc/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
VAEG_M44_SCENARIO_INPUT_DIR=/tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
VAEG_M44_SCENARIO_OUTPUT_DIR=/home/maho/vaeg/build/m50-matrix/g41-to-m50-dummy \
  /home/maho/vaeg/build/m50-linux-ci-gcc/sdl2/vaeg --selftest
python3 tools/qa/upd9002_state_matrix.py \
  --fixtures tests/upd9002/state_fixtures_m42.txt \
  --g41-generated /tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
  --m44-generated /home/maho/vaeg/build/m50-matrix/current-dummy \
  --g41-to-m44 /home/maho/vaeg/build/m50-matrix/g41-to-m50-dummy \
  --m44-to-g41 /tmp/vaeg-m47-matrix.i6ajc2/m47-to-g41 \
  --g41-sha dc8a72da974f0ea328613e480f1de662c28f4436 \
  --m44-sha 303e6cb

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /home/maho/vaeg/build/m50-linux-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /home/maho/vaeg/build/m50-linux-release/sdl2/vaeg --smoke
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m50-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 wineboot -u
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m50-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /home/maho/vaeg/build/m50-mingw-cross/sdl2/vaeg.exe --selftest
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m50-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /home/maho/vaeg/build/m50-mingw-cross/sdl2/vaeg.exe --smoke

cmake -S . -B /home/maho/vaeg/build/m50-post-link -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=OFF \
  -DVAEG_ENABLE_ARCHIVE_DROP=OFF \
  -DCMAKE_C_FLAGS='-ffunction-sections -fdata-sections' \
  -DCMAKE_CXX_FLAGS='-ffunction-sections -fdata-sections' \
  -DCMAKE_EXE_LINKER_FLAGS='-Wl,-Map,/home/maho/vaeg/build/m50-post-link/vaeg.map'
cmake --build /home/maho/vaeg/build/m50-post-link --parallel 4 --target vaeg
nm -a /home/maho/vaeg/build/m50-linux-release/sdl2/vaeg
nm -a /home/maho/vaeg/build/m50-post-link/sdl2/vaeg
x86_64-w64-mingw32-nm -a \
  /home/maho/vaeg/build/m50-mingw-cross/sdl2/vaeg.exe
readelf -rW \
  /home/maho/vaeg/build/m50-linux-release/CMakeFiles/vaeg_core.dir/i286c/i286c_mn.c.o
readelf -rW \
  /home/maho/vaeg/build/m50-linux-release/CMakeFiles/vaeg_core.dir/i286c/i286c_fe.c.o

git diff --exit-code 336227f093d37e3c60bc50333216d66668755cef -- \
  tools/qa/golden/upd9002_final_dispatch_graph.csv \
  tools/qa/golden/upd9002_dispatch_provenance_m42.csv \
  tools/qa/golden/upd9002_support_map_m42.csv \
  tests/upd9002/harness_manifest.csv tests/upd9002/trace_baseline.txt \
  tests/upd9002/state_fixtures_m42.txt tests/upd9002/abi_g41.txt
git diff --exit-code 2a21a5264a3830f5a393ed7fbd3fbe1e900f2926 -- \
  tools/qa/golden/upd9002_final_dispatch_graph_m48.csv \
  tools/qa/golden/upd9002_dispatch_provenance_m48.csv \
  tools/qa/golden/upd9002_support_map_m48.csv \
  tools/qa/golden/upd9002_rep0f_transition_manifest_m48.json \
  tools/qa/golden/upd9002_286_reachability_m49.csv tests/ssts
python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check
git diff --exit-code 2a21a5264a3830f5a393ed7fbd3fbe1e900f2926 -- \
  win9x i286x cpuxva/memoryva.x86 hlp romimage
git push -u origin topic/m50-upd9002-remove-protected-mode
gh run view 29662304823 --json status,conclusion,url,headSha,jobs
```

## Deviations and remaining risks

- Temporary preset builds under this worktree's small `/tmp` filesystem
  exhausted available space during sanitizer work. Only M50-created build
  directories were removed. Fresh canonical builds completed under
  `/home/maho/vaeg/build/m50-*`; no tracked or user asset changed.
- The first tests-disabled release FetchContent configure could not resolve a
  host in the restricted sandbox. The identical command passed with approved
  network access.
- Before their parent directories were created, the first two state-scenario
  commands failed to save/output fixtures. Fresh task-owned directories were
  created and both commands plus the complete four-way matrix then passed.
- The first Wine initialization attempt could not create its server directory
  under the restricted runtime mount. The same command passed with approved
  Wine runtime access, followed by successful selftest and smoke runs.
- One exploratory invocation of the historical M42-only
  `upd9002_dispatch.py --selftest` compared the current accepted M48 graph to
  the intentionally frozen M42 graph and exited 1. The applicable M48
  transition checker and M50 deletion checker regenerated and verified the
  current graph/provenance/support identities successfully; no accepted golden
  was changed.
- One exploratory SHA command named a nonexistent shorthand known-gap path
  after hashing the three preceding artifacts. The corrected committed path
  produced the accepted SHA, and its recorded counts remain 40 and 68,626.
- Immediately after deleting `i286c_0f.c` but before committing that deletion,
  Git's tracked-file query still listed the path, so two source-absence CTests
  intentionally failed. They passed after the deletion-focused commit and in
  every final GCC/Clang/ASan run.
- Real uPD9002/V52 REP+0F semantics remain unknown. M50 preserves the approved
  fail-closed diagnostic policy and does not assert architectural correctness.
- `SEGSELECT`, `i286c_selector`, protected runtime/serialized state, and other
  shared `I286_*` implementation names remain intentionally. They are active or
  compatibility-owned, not evidence of an incomplete approved deletion. Any
  mechanical rename belongs only to M51 after G50.
- The standard V3 boot, bundled VA demo, OS, media, sound, and save/load gate is
  a human check. This report neither performs nor approves it.

## G50 human-review checklist

- [ ] Confirm only `M50-PM-ARPL`, `M50-PM-MOV-SEG-EA`, and
  `M50-PM-CTS-SYSTEM` were deleted.
- [ ] Confirm nine `_reserved` base placeholders and all accepted final native
  or diagnostic patch targets.
- [ ] Confirm `v30mov_seg_ea`, `SEGSELECT`, `i286c_selector`, protected state,
  CPU_SHUT, and M48 diagnostic behavior remain.
- [ ] Review the exact nine-row M50 construction-provenance transition and
  byte-identical final graph/support map.
- [ ] Review source/object/linker/ELF/PE absence evidence and production
  isolation.
- [ ] Review M42--M49 artifact, M43 signature, state matrix, sanitizer,
  MinGW/Wine, and hosted CI results.
- [ ] From a clean checkout, build and boot V3 mode.
- [ ] Run the bundled VA demo.
- [ ] Boot an OS and verify keyboard input and FDD read.
- [ ] If practical, verify FDD write/readback, Sound Board II, save/load
  continuation, and reset/reboot.
- [ ] Confirm no test log, CLI seam, or diagnostic appears during ordinary
  supported execution.
- [ ] Explicitly approve or reject G50. Do not begin M51 before approval.
