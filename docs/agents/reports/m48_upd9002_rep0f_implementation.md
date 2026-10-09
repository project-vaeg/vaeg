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
# M48 uPD9002 REP+0F fail-closed implementation

Status: the explicitly approved G47 fail-closed policy is implemented and the
required local validation is complete. This report does not claim the selected
behavior is architectural uPD9002/V52 semantics, does not delete the protected
mode implementation, does not begin M49, and does not declare G48 passed.

The final report/remote SHA is supplied in the handoff because this file cannot
contain the SHA of the commit that contains itself.

## Identity and approved scope

- Branch: `topic/m48-upd9002-rep0f-fail-closed`
- Exact starting and approved G47 evidence SHA:
  `c683fe502647918d8ded2b4c2243da1b787c9d0a`
- Pre-report implementation SHA:
  `e4cc903684519a08f104a6eb2e28dba4192c6a3f`
- Remote implementation SHA:
  `e4cc903684519a08f104a6eb2e28dba4192c6a3f`
- Hosted implementation run:
  [build 29645196057](https://github.com/nakatamaho/vaeg/actions/runs/29645196057)
  — 7/7 jobs successful at the pre-report implementation SHA

The worktree started clean at the exact approved evidence SHA. No reset,
rebase, squash, force push, or M42--M47 history rewrite was performed. M49 was
not started.

G47 approved the following deliberately conservative policy:

1. F2/F3-prefixed 0F latches an emulator-level diagnostic stop before the
   unresolved encoding modifies architectural state.
2. This is a fail-closed implementation policy, not an instruction-semantics
   assertion.
3. A serialized CPU286 payload with `MSW.PE` set is rejected transactionally
   before any live machine state is modified.
4. Only the explicitly affected M42 graph/provenance/support rows may receive
   post-M48 counterparts. Accepted M42 and M43 artifacts remain immutable.
5. Protected-mode source is retained for a later, separately gated
   reachability audit.

## Commits and files

The implementation commits before this report are:

1. `bc00b370480283dbf7f7529fc6345def87a7dc75` — stop unresolved REP 0F
   before state mutation;
2. `60c58c9fb2f5ba6b4526cdfa51c79b716b1b128a` — rename the REP 0F
   regression test, as a rename-only commit;
3. `71bc6d9e1fee75ab20dfa7f26ce243949e37cb0f` — cover fail-closed REP 0F
   diagnostics;
4. `9924b85ca13a87610571392968ca63bd74e85321` — reject
   protected-mode-dependent saved states;
5. `f785dbb7a71a24c1ac33bd10556bdbacdef254d4` — record the exact REP 0F
   baseline transition;
6. `e4cc903684519a08f104a6eb2e28dba4192c6a3f` — record the G47
   fail-closed decision.

Changed paths are:

```text
CMakeLists.txt
docs/agents/DECISIONS/ADR-0013-upd9002-rep0f-correctness.md
docs/modernization/bug-fixes.md
i286c/i286c.c
i286c/upd9002_diagnostic.c
i286c/upd9002_diagnostic.h
i286c/upd9002_state.c
i286c/upd9002_state.h
i286c/v30patch.c
pccore.c
sdl2/np2.c
tests/upd9002/rep0f_current_behavior.c (renamed, then rewritten)
tests/upd9002/rep0f_current_behavior.h (renamed)
tests/upd9002/rep0f_diagnostic_stop.c
tests/upd9002/rep0f_diagnostic_stop.h
tests/upd9002/state_boundary.c
tests/upd9002/statsave_boundary.c
tools/qa/golden/upd9002_dispatch_provenance_m48.csv
tools/qa/golden/upd9002_final_dispatch_graph_m48.csv
tools/qa/golden/upd9002_rep0f_transition_manifest_m48.json
tools/qa/golden/upd9002_support_map_m48.csv
tools/qa/upd9002_rep0f_transition.py
```

No frozen-reference, ROM, disk, font, icon, wave, protected-mode handler, or
CPU state-layout file was deleted. `i286c_0f.c`, `i286c_selector`, every
protected runtime field, and CPU_SHUT compatibility logic remain present.

## Fail-closed execution boundary

The sole constructor remains `v30cinit()` and the sole active instruction
primitive remains `v30c_step()`. Construction still occurs once. The two
approved slots now terminate in dedicated diagnostic handlers:

```text
v30op[0xf2] -> v30_repne -> v30op_repne[0x0f]
                            -> v30_repne_0f_diagnostic_stop
v30op[0xf3] -> v30_repe  -> v30op_repe[0x0f]
                            -> v30_repe_0f_diagnostic_stop
```

Unprefixed `v30op[0x0f] -> v30_ope0x0f` is unchanged.

At the top of `v30c_step()`, a previously latched diagnostic prevents further
execution. When the first opcode is F2, F3, or a segment prefix which can lead
to F2/F3, the complete `Upd9002RuntimeState` is copied before dispatch. The
diagnostic handler records reason, REP prefix, and original CS:IP. The step
then restores the complete runtime state, omits the DMA callback, and returns.
`pccore_exec()` checks the latch before scheduling and immediately after the
step, before Z80, SGP, or other VA device execution. The SDL frontend emits:

```text
Error: uPD9002 fail-closed diagnostic stop at CCCC:IIII: PP 0f was not
executed because its semantics are unresolved
```

It requests task exit and returns failure. Reset, initialize, and the accepted
CPU_SHUT path clear a prior latch. No instruction byte, ModR/M byte, legacy CTS
handler, DMA callback, or other device is allowed to commit state for the
stopped encoding.

This boundary intentionally stops all 512 F2/F3 plus 0F second-byte forms. It
does not assert whether a real uPD9002 ignores, interprets, reserves, or varies
the prefix by second byte.

## Diagnostic regression evidence

The target-local `--upd9002-m48-rep0f-diagnostic` test covers 522 cases:

- F2 and F3 with every possible 0F second byte: 512 cases;
- each REP prefix reached through ES, CS, SS, or DS segment prefix: 8 cases;
- F2/F3 0F 01 with `MSW.PE` already set in runtime: 2 cases.

For every case it compares the complete `Upd9002RuntimeState` byte-for-byte,
hashes all 1 MiB of test memory before and after, verifies the deterministic
reason/prefix/CS:IP record, and calls `v30c_step()` again to prove a latched
machine cannot resume. The unprefixed `0f 01 f0` control remains at IP `2002`,
MSW `0000`, with no diagnostic. The result is:

```text
upd9002-rep0f-diagnostic: cases=522 state-and-memory-atomic pass
```

The seam is compiled only when `VAEG_ENABLE_TESTS=ON`; it is absent from both
tests-disabled production binaries.

## Transactional protected-state rejection

`upd9002_state_validate()` now rejects exactly:

```text
Cpu286StateCompat.MSW & MSW_PE
```

with the deterministic compatibility error:

```text
CPU286 state requires unsupported 80286 protected-mode execution
```

`MSW` remains a two-byte serialized field at offset 70 in the unchanged
112-byte CPU286 payload. No state tag, version, field offset, padding rule,
CPU286 size, or 16-byte UPD9002 section changes.

The predicate is intentionally not “any nonzero protected residue.” `MSW.PE`
is the active selector which permits `SEGSELECT` to enter
`i286c_selector()`. GDTR, IDTR, LDTR/LDTRC, and TR/TRC bytes cannot require
protected execution while PE is clear, so those dormant bytes remain accepted
and opaque. This preserves the M44 noncanonical compatibility-image contract.

The standalone adapter test and the complete statsave preflight test both
inject PE, require the exact error, and compare CPU runtime, opaque compatibility
image, PCCORE, UPD9002, and memory before and after rejection. Both
`statsave_check()` and `statsave_load()` reject before commit. Reset remains
FLAGS `f002`; CPU_SHUT remains FLAGS `0000` and preserves its exact historical
payload transformation.

## Exact baseline transition

The deterministic, fail-closed transition verifier is
`tools/qa/upd9002_rep0f_transition.py`. It hard-checks historical artifact
hashes, parses the current constructor and dataset evidence, emits deterministic
post-M48 artifacts, and fails on an unknown or extra transition.

| Artifact | Post-M48 SHA-256 |
|---|---|
| final dispatch graph | `fe9df28ad3d51cc55235afc3979ada890e86158b294762c15fa33c20d8a800a6` |
| construction provenance | `128698af06c4e4e98183e4ec0151b7025f427c4f812f95d9012f41417461027a` |
| support map | `21dd037c3eb11e1674805ad456ef03663f17804affbd7382c8db77291ab25279` |
| complete transition manifest | `4f3fefe8cbfb20a03364a80a0b917e475d3d545cab8eda6bee8a22c66e2147ee` |

The final graph removes the two root edges to `i286c_cts`, their two derived
secondary-table edges, and the sixteen CTS0/CTS1 handler closure rows. It adds
exactly the two diagnostic root edges. Thus the graph diff is 20 removed and 2
added rows. The 18 derived removals are reachability changes only: no protected
mode implementation source is deleted.

Provenance removes the two inherited base rows and adds two explicit patch
rows. The support map replaces the same two root targets. The historical
156-case harness remains byte-identical; the two new patch rows are covered by
the dedicated 522-case test rather than being inserted into the accepted M42
harness.

The pinned M43 dataset contains zero decoded F2/F3+0F records. Therefore the
exact applicable, known-gap, semantic-failure, and failure-signature change
sets are all empty. No M43 baseline file is updated.

## M42--M46 preservation

The accepted M42 artifacts remain byte-identical to commit
`336227f093d37e3c60bc50333216d66668755cef`:

| Historical artifact | SHA-256 |
|---|---|
| M42 final dispatch graph | `e8c8fded6e1f44ef2d74ace93ba1063d64464b217bdd4f9180bf50e86e19df3d` |
| M42 construction provenance | `605e9af6761e3069f6c5af0ad9647ae574ab98252f7373969586cfee892cd6ba` |
| M42 support map | `7b856f95d8c3d0356325483d188b9dde2d03a8e6cbe9b7fd0cacd004f1d09d13` |
| 156-case harness | `14769ca59bf589921cbcf88dc0c357937f6da36e4fe0b78f9a46dc75b9098f2a` |
| trace baseline | `17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba` |
| reset/executed-3/CPU_SHUT fixtures | `c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb` |
| G41 ABI fixture | `f6995d59a293a4e81fc710400fd447031016322018a1c88c9e752c1eb1889db7` |

The direct harness, trace determinism and trace-on/off checkpoints, ABI,
fixtures, and all unaffected dispatch rows pass. The M45 invariant reports
`v30c_step` active, CPU-type execution selectors absent, and legacy block
executors/macros absent. The M46 checks report one constructor invocation,
six immutable roots of 256/256/256/256/8/8 entries, and no reset/selftest/state
load rebuild.

The M44 matrix reports all four booleans true using the approved raw-G41
payloads and roundtrip evidence, fresh M48 generation, and fresh G41-to-M48
ingest. M48 payloads match the authoritative fixtures:

| Scenario | CPU286 SHA-256 | UPD9002 SHA-256 |
|---|---|---|
| reset | `6cbd3314a586ccf9c5f1d11a17b8836f76f1deaf10814de8b61ebdf96a792f89` | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| executed-3 | `45d06e424fd332027817ca66db7ed0298e87695cf0cfed5582b7fe1348faa7fe` | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| CPU_SHUT | `7636def12b5f25c39540a1367166394a8308064cce74a15860c62a487a946ff7` | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

Only PE-set imported states intentionally leave the old acceptance set, as
authorized at G47. The three canonical matrix states have PE clear.

## M43 exact reproduction

The 360-file dataset verifies as:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-
1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

The known-gap artifact remains byte-identical at SHA-256
`11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1`,
with 40 selectors and 68,626 exact record hashes.

| Profile | Executed | Pass | Semantic failure | Timeout | Crash | Signature index |
|---|---:|---:|---:|---:|---:|---|
| CI | 166,821 | 156,228 | 10,593 | 0 | 0 | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` |
| full empty-prefetch | 1,443,876 | 1,359,547 | 84,329 | 0 | 0 | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |

Both independent `--expect` runs passed. The generated result JSON files are
byte-identical to the accepted baselines, and `diff -qr` reports no difference
in either complete failure sidecar directory. No classification, record hash,
canonical failure signature, or index digest changed.

## Toolchains and production isolation

| Gate | Result |
|---|---|
| GCC 15.2 `linux-debug`, `linux-ci-gcc` | fresh configure/build; GCC CTest 32/32 |
| Clang 21.1 `linux-ci-clang` | fresh configure/build; CTest 32/32 |
| GCC ASan/UBSan `linux-asan`, `linux-ci-asan` | fresh configure/build; CTest 32/32 with leak detection and stacktrace printing disabled; focused halt-on-error run 4/4 |
| GCC 15.2 `linux-release` | fresh tests-disabled build; selftest and ROM-less smoke passed |
| MinGW-w64 GCC 13 `mingw-cross` | fresh configure/build; PE32+ GUI x86-64 |
| Wine 10.0 | initialized dedicated prefix; MinGW production selftest and smoke passed |
| hosted Linux/Windows/macOS/standalone | [build 29645196057](https://github.com/nakatamaho/vaeg/actions/runs/29645196057): 7/7 jobs successful at `e4cc903684519a08f104a6eb2e28dba4192c6a3f` |
| repository invariants | encoding 0, EOL 0, case 0, `git diff --check` 0 |
| unreferenced report | 70 findings, exactly the accepted M47 count |

The Linux and MinGW production caches both contain
`VAEG_ENABLE_TESTS=OFF` and `VAEG_Z80_INTEGRATION_TRACE=OFF`. Their Ninja
graphs contain no uPD9002 test source or M48 test define. Their binaries contain
no M48 test main or CLI string. The production diagnostic latch, message, and
four diagnostic API symbols are intentionally present because they implement
the approved guest-safety policy.

Production artifact identities before publication are:

| Artifact | Format | SHA-256 |
|---|---|---|
| `build/linux-release/sdl2/vaeg` | ELF x86-64 PIE | `d0823dfe81adce8584b9e465671c0b2561eeafc68043c94566365075cbeb389e` |
| `build/mingw-cross/sdl2/vaeg.exe` | PE32+ GUI x86-64 | `602d536a3927f1404eb0f0a842efb0f6b682579203fd7fa2a60e7a8bedae4b5a` |

## Exact validation commands and statuses

Every command below exited zero unless an explicitly documented diagnostic
attempt says otherwise.

```text
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
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
  UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build/linux-ci-asan --output-on-failure \
  -R 'vaeg_upd9002_(rep0f_diagnostic_stop|state_boundary|state_payload_probe|dispatch_normalization)$'

python3 tools/qa/upd9002_rep0f_transition.py --root .
python3 tools/qa/upd9002_rep0f_transition.py --root . --selftest
python3 tools/qa/upd9002_native_invariant.py --root .
python3 tools/qa/upd9002_dispatch_normalization.py --root .
build/linux-ci-gcc/sdl2/vaeg --upd9002-m46-dispatch-qa
build/linux-ci-gcc/sdl2/vaeg --upd9002-m48-rep0f-diagnostic
build/linux-ci-gcc/vaeg_upd9002_state_boundary
ctest --test-dir build/linux-ci-gcc --output-on-failure \
  -R '^vaeg_upd9002_state_payload_probe$'
build/linux-ci-gcc/vaeg_upd9002_abi

python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg --profile ci \
  --shard-timeout 120 \
  --output /tmp/vaeg-m48-ssts.3JzLpH/v20_native_ci.json \
  --failure-directory /tmp/vaeg-m48-ssts.3JzLpH/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg --profile full \
  --shard-timeout 300 \
  --output /tmp/vaeg-m48-ssts.3JzLpH/v20_native_full.json \
  --failure-directory /tmp/vaeg-m48-ssts.3JzLpH/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
cmp /tmp/vaeg-m48-ssts.3JzLpH/v20_native_ci.json \
  tests/ssts/baseline/v20_native_ci.json
cmp /tmp/vaeg-m48-ssts.3JzLpH/v20_native_full.json \
  tests/ssts/baseline/v20_native_full.json
diff -qr /tmp/vaeg-m48-ssts.3JzLpH/v20_native_ci_failures \
  tests/ssts/baseline/v20_native_ci_failures
diff -qr /tmp/vaeg-m48-ssts.3JzLpH/v20_native_full_failures \
  tests/ssts/baseline/v20_native_full_failures

VAEG_M44_SCENARIO_DIR=/tmp/vaeg-m48-matrix.7IumeH/m48-generated \
  build/linux-ci-gcc/sdl2/vaeg --selftest
VAEG_M44_SCENARIO_INPUT_DIR=/tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
VAEG_M44_SCENARIO_OUTPUT_DIR=/tmp/vaeg-m48-matrix.7IumeH/g41-to-m48 \
  build/linux-ci-gcc/sdl2/vaeg --selftest
python3 tools/qa/upd9002_state_matrix.py \
  --fixtures tests/upd9002/state_fixtures_m42.txt \
  --g41-generated /tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
  --m44-generated /tmp/vaeg-m48-matrix.7IumeH/m48-generated \
  --g41-to-m44 /tmp/vaeg-m48-matrix.7IumeH/g41-to-m48 \
  --m44-to-g41 /tmp/vaeg-m47-matrix.i6ajc2/m47-to-g41 \
  --g41-sha dc8a72da974f0ea328613e480f1de662c28f4436 \
  --m44-sha e4cc903684519a08f104a6eb2e28dba4192c6a3f

git diff --exit-code 336227f093d37e3c60bc50333216d66668755cef -- \
  tools/qa/golden/upd9002_final_dispatch_graph.csv \
  tools/qa/golden/upd9002_dispatch_provenance_m42.csv \
  tools/qa/golden/upd9002_support_map_m42.csv \
  tests/upd9002/harness_manifest.csv tests/upd9002/trace_baseline.txt \
  tests/upd9002/state_fixtures_m42.txt tests/upd9002/abi_g41.txt

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  build/linux-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  build/linux-release/sdl2/vaeg --smoke
WINEDEBUG=-all \
WINEPREFIX=/home/maho/vaeg/build/m48-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --selftest
WINEDEBUG=-all \
WINEPREFIX=/home/maho/vaeg/build/m48-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --smoke

python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check
git diff --exit-code c683fe502647918d8ded2b4c2243da1b787c9d0a -- \
  win9x i286x cpuxva/memoryva.x86 hlp romimage
git push -u origin topic/m48-upd9002-rep0f-fail-closed
gh run view 29645196057 --json status,conclusion,url,headSha,jobs
```

## Deviations and unresolved risks

- The first ASan full CTest used `UBSAN_OPTIONS=print_stacktrace=1`.
  `vaeg_upd9002_trace_equivalence` then saw different ASLR addresses in the
  two pre-existing sound UBSan backtraces. The trace itself and all checkpoints
  were equal. The accepted deterministic configuration with stacktrace
  printing disabled passed 32/32; the four M48/M46/state tests separately
  passed with `halt_on_error=1` and stacktraces enabled.
- The first Wine attempt used a newly created but not explicitly initialized
  prefix and stopped at the INI roundtrip test. Running `wineboot -u` on an
  owned dedicated prefix made both selftest and smoke pass. The disposable
  failed `/tmp/vaeg-m48-wine` prefix was deleted; no tracked or user asset was
  removed.
- Two manual diagnostic invocations used the wrong CLI form and exited 2:
  `upd9002_rep0f_transition.py check/selftest` should use no subcommand and
  `--selftest`, and the standalone payload probe requires its fixture
  argument. The correct commands and their CTest forms passed.
- Whole state-container files include unrelated time-sensitive sections, so
  cross-version acceptance is based on the state parser's exact CPU286 and
  UPD9002 payload comparisons, not whole-file hashes.
- No authoritative uPD9002/V52 document or real-hardware result has resolved
  the architectural REP+0F semantics. A guest encountering the unresolved
  encoding now stops deliberately and requires operator diagnosis.
- PE-clear descriptor residue remains accepted and opaque. M49 must re-audit
  protected-mode reachability after the approved dispatch/state transition;
  M48 deletes nothing.
- The standard V3 boot, bundled VA demo, OS, media, sound, and save/load gate
  remains a human check. This report does not perform or approve it.

## G48 human-review checklist

- [x] Confirm exact G47 starting SHA and approved fail-closed scope.
- [x] Review both REP+0F root patches and absence of unrelated dispatch change.
- [x] Verify the diagnostic occurs before architectural or device mutation.
- [x] Verify all 522 focused cases and persistent stopped state.
- [x] Verify PE-set state rejection is preflighted and atomic.
- [x] Verify PE-clear opaque residue, state layout, reset, and CPU_SHUT remain.
- [x] Review exact M42 graph/provenance/support transition rows.
- [x] Verify M43 CI/full counts, signatures, hashes, and zero baseline edits.
- [x] Verify M45 native execution and M46 constructor/immutability gates.
- [x] Verify Linux/Clang/ASan/MinGW/Wine and production isolation.
- [x] Confirm hosted Linux, Windows, macOS, and standalone jobs at the pushed
  implementation SHA: build 29645196057, 7/7 successful.
- [ ] Build from a clean checkout and boot in V3 mode.
- [ ] Run the bundled VA demo.
- [ ] Boot an OS and verify keyboard input and FDD read.
- [ ] If practical, verify FDD write and re-read.
- [ ] Verify Sound Board II.
- [ ] Save/load and continue operation; verify PE-set legacy state rejects
  clearly and leaves the running machine unchanged.
- [ ] Reset and boot again.
- [ ] Confirm ordinary startup exposes no test CLI or test-only log seam.
- [ ] Maintainer decides G48 explicitly; this report does not declare it.
