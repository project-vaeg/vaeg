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
# M51 pure uPD9002 rename and move

Status: the approved mechanical moves, public API renames, register-model
rename, VA header move, documentation, and fail-closed ownership guard are
complete. Local M42--M50 preservation is complete. The G51 human gate remains
external to this report; this report does not declare G51 passed.

The final report/remote SHA is supplied in the handoff because a report cannot
contain the SHA of the commit that contains itself.

## Identity and scope

- Branch: `topic/m51-upd9002-rename`
- Exact starting and approved G50 SHA:
  `8b818fca173b2e86b4a8af20e75bc069f7697cdf`
- Accepted G50 branch: `topic/m50-upd9002-remove-protected-mode`
- Pre-report implementation/documentation SHA:
  `bb562fc1092b67e3609b5049533ca069b5e2c1d0`
- Hosted implementation run:
  [build 29665120771](https://github.com/nakatamaho/vaeg/actions/runs/29665120771)
  at `bb562fc1092b67e3609b5049533ca069b5e2c1d0`; all seven Linux, Windows,
  macOS, standalone Z80, and repository-invariant jobs succeeded
- Final and remote SHA: supplied in the handoff after the report and hosted
  validation commits

The worktree started clean at the exact approved G50 SHA. No reset, rebase,
squash, force push, or M42--M50 history rewrite was performed. M51 changes no
instruction, dispatch, state, timing, prefetch, DMA, interrupt, exception,
memory, I/O, reset, CPU_SHUT, or REP+0F diagnostic behavior. No additional
protected-mode code was deleted.

## Commits

Implementation commits before this report are:

1. `f1210cac575541acb660fd3fd288d1ea859063ea` —
   `M51: move uPD9002 core sources`;
2. `dbf9766fbffec8671c2a5806c14eb5372d7cd7c6` —
   `M51: fix core paths and build references`;
3. `9d6ecfefc73ea9e660b7045e652e296ccd87f618` —
   `M51: rename public uPD9002 core APIs`;
4. `ad53391352a61b7a8efe420f4497a27c8519427b` —
   `M51: move uPD9002 register model files`;
5. `09fbee326352abaccedcf65f332c3ad6190ad06f` —
   `M51: fix register model names and references`;
6. `6077367ff15df6e824d7a153f2d358c6a741de6b` —
   `M51: move VA memory header`;
7. `1825075a92ad58e29b65f820ff05dbe480cc6aff` —
   `M51: fix VA memory header include path`;
8. `5246d3fa3c0d461a02e8be586b78f7abff9989f8` —
   `M51: add active uPD9002 ownership guard`;
9. `bb562fc1092b67e3609b5049533ca069b5e2c1d0` —
   `M51: document final uPD9002 ownership`.

The three move-only commits are byte-pure: Git reports all core paths at 100%
similarity in `f1210ca`, both register-model paths at 100% in `ad53391`, and
the VA memory header at 100% in `6077367`; each has zero content insertion or
deletion. Only those three hashes were added to `.git-blame-ignore-revs`.

## Exact move map

| Before M51 | After M51 | Content policy |
|---|---|---|
| `i286c/` | `cpu/upd9002/` | directory ownership move |
| `i286c/i286c.c` | `cpu/upd9002/upd9002_core.c` | approved public owner basename |
| `i286c/v30patch.c` | `cpu/upd9002/upd9002_dispatch.c` | approved dispatch basename |
| `i286c/v30patch.h` | `cpu/upd9002/upd9002_dispatch.h` | approved dispatch basename |
| `iova/upd9002.c` | `iova/upd9002_regs.c` | separate built-in register model |
| `iova/upd9002.h` | `iova/upd9002_regs.h` | separate built-in register model |
| `cpuxva/memoryva.h` | `cpucva/memoryva.h` | active C VA memory owner |

The other core basenames moved without cosmetic renaming:

```text
cpucore.h       dmap.c          dmap.h          egcmem.c
egcmem.h        i286c.h         i286c.mcr       i286c_8x.c
i286c_ea.c      i286c_f6.c      i286c_fe.c      i286c_mn.c
i286c_rp.c      i286c_sf.c      i286c_sf.mcr    memory.c
memory.h        upd9002_diagnostic.c/.h          upd9002_state.c/.h
upd9002_trace.c/.h
```

`cpuxva/memoryva.x86` was not moved or modified.

## Exact public API rename map

No compatibility wrapper was added. The definitions, declarations, active
callers, tests, and audit tools use:

| Retired public name | Canonical M51 name |
|---|---|
| `i286c_initialize` | `upd9002_core_initialize` |
| `i286c_deinitialize` | `upd9002_core_deinitialize` |
| `i286c_reset` | `upd9002_core_reset` |
| `i286c_shut` | `upd9002_core_shut` |
| `i286c_setextsize` | `upd9002_core_set_ext_size` |
| `i286c_setemm` | `upd9002_core_set_emm` |
| `i286c_interrupt` | `upd9002_core_interrupt` |
| `v30c_step` | `upd9002_core_step` |
| `v30cinit` | `upd9002_dispatch_initialize` |

The separate register model changed as follows:

| Retired register-model name | Canonical M51 name |
|---|---|
| `upd9002_reset` | `upd9002_regs_reset` |
| `upd9002_bind` | `upd9002_regs_bind` |
| global `upd9002` | global `upd9002_regs` |
| `_UPD9002` / `UPD9002` | `UPD9002_REGS` |

ELF and PE production symbol inventories contain all nine canonical core APIs,
both canonical register functions, and `upd9002_regs`; they contain none of
the retired public names.

## Exact internal historical-name exceptions

The active-tree guard permits exactly these 18 internal REP-helper names:

```text
i286c_rep_insb       i286c_rep_insw       i286c_rep_outsb
i286c_rep_outsw      i286c_rep_movsb       i286c_rep_movsw
i286c_rep_lodsb      i286c_rep_lodsw       i286c_rep_stosb
i286c_rep_stosw      i286c_repe_cmpsb      i286c_repne_cmpsb
i286c_repe_cmpsw     i286c_repne_cmpsw     i286c_repe_scasb
i286c_repne_scasb    i286c_repe_scasw      i286c_repne_scasw
```

The first 17 are recorded in accepted dispatch graph/provenance evidence.
`i286c_rep_outsw` remains with the same cross-translation-unit REP-helper
family. The guard records the exact permitted file set for every name and has
no wildcard exception.

The following separate compatibility/internal classes also remain by explicit
policy: literal state tags `CPU286` and `UPD9002`, `Cpu286StateCompat`,
`Upd9002RuntimeState`, `Cpu286CompatImage`, `I286_*` macros,
`i286c_initreg`, `i286c_selector`, `i286c_intnum`, historical internal
basenames, immutable evidence text, and frozen-reference names. None is a
retired public lifecycle API.

## CMake, includes, guard, and files changed

`VAEG_INCLUDE_DIRS` now contains `cpu/upd9002` and `cpucva`, not `i286c` or
`cpuxva`. `VAEG_CORE_SOURCES`, test sources, and trace instrumentation use the
new core paths. All active includes use `upd9002_dispatch.h`,
`upd9002_regs.h`, and `cpucva/memoryva.h` through the active include roots.

`tools/qa/upd9002_rename.py` deterministically fails on:

- an active tracked or untracked path below `i286c/`;
- any retired core/register/header path;
- any of the nine retired public core APIs in active source;
- obsolete CMake source or include entries;
- old register-model APIs, type, or generic global uses;
- a missing canonical declaration or definition;
- any addition, removal, or file-set drift in the exact 18-name exception;
- a missing or duplicated literal `UPD9002` state entry.

It is wired as `vaeg_upd9002_rename` only when tests are enabled. The exact
changed path set before this report is:

```text
.git-blame-ignore-revs
AGENTS.md
BUILD.md
CMakeLists.txt
cpu/upd9002/ (26 moved core files)
cpucva/memoryva.h
docs/agents/DECISIONS/ADR-0012-upd9002-ownership.md
docs/agents/ROADMAP.md
docs/agents/tasks/M51_upd9002_rename.md
docs/agents/tasks/README.md
docs/modernization/virtual-machine-architecture.md
io/iocore.c
io/pit.c
iova/upd9002_regs.c
iova/upd9002_regs.h
pccore.c
statsave.c
statsave.tbl
tests/upd9002/abi.c
tests/upd9002/direct_harness.c
tests/upd9002/dispatch_normalization.c
tests/upd9002/fixtures.c
tests/upd9002/rep0f_diagnostic_stop.c
tests/upd9002/ssts_worker.c
tests/upd9002/state_payload_probe.c
tests/upd9002/state_scenario.c
tests/upd9002/statsave_boundary.c
tools/qa/upd9002_dispatch.py
tools/qa/upd9002_dispatch_normalization.py
tools/qa/upd9002_native_invariant.py
tools/qa/upd9002_protected_deletion.py
tools/qa/upd9002_rename.py
tools/qa/upd9002_rep0f_transition.py
```

The task archive notice makes passed M36--M41 task documents unambiguously
historical without deleting or rewriting them.

## Dispatch and source-level preservation

The accepted final M48 graph regenerates byte-identically:

```text
tools/qa/golden/upd9002_final_dispatch_graph_m48.csv
SHA-256 fe9df28ad3d51cc55235afc3979ada890e86158b294762c15fa33c20d8a800a6
```

The accepted M50 construction provenance remains byte-identical:

```text
tools/qa/golden/upd9002_dispatch_provenance_m50.csv
SHA-256 30246dbd2bbf95b406a6bb05a182d16ea04f56dba48dca73842f5d06b74aae2c
```

The M48 support map remains
`21dd037c3eb11e1674805ad456ef03663f17804affbd7382c8db77291ab25279`.
No golden was renamed or re-recorded. The generator accepts the new source
path and canonical public names while continuing to emit the exact historical
handler graph. The M46 runtime gate reports one successful construction, one
rejected repeat construction, roots `256,256,256,256,8,8`, and element-wise
immutability.

The original M42 immutable identities also remain:

| Artifact | SHA-256 |
|---|---|
| M42 final graph | `e8c8fded6e1f44ef2d74ace93ba1063d64464b217bdd4f9180bf50e86e19df3d` |
| M42 provenance | `605e9af6761e3069f6c5af0ad9647ae574ab98252f7373969586cfee892cd6ba` |
| M42 support map | `7b856f95d8c3d0356325483d188b9dde2d03a8e6cbe9b7fd0cacd004f1d09d13` |
| M42 156-case harness | `14769ca59bf589921cbcf88dc0c357937f6da36e4fe0b78f9a46dc75b9098f2a` |
| M42 trace | `17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba` |
| M42 state fixture | `c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb` |
| G41 ABI fixture | `f6995d59a293a4e81fc710400fd447031016322018a1c88c9e752c1eb1889db7` |

## State and serialized payload preservation

The four-way G41/current state matrix reports every comparison true. Payloads
remain:

| Scenario | CPU286 bytes / SHA-256 | UPD9002 bytes / SHA-256 |
|---|---|---|
| reset | 112 / `6cbd3314a586ccf9c5f1d11a17b8836f76f1deaf10814de8b61ebdf96a792f89` | 16 / `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| executed-3 | 112 / `45d06e424fd332027817ca66db7ed0298e87695cf0cfed5582b7fe1348faa7fe` | 16 / `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |
| CPU_SHUT | 112 / `7636def12b5f25c39540a1367166394a8308064cce74a15860c62a487a946ff7` | 16 / `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

The literal tags remain `CPU286` and `UPD9002`. Transactional preflight,
atomic invalid-state and MSW.PE rejection, opaque PE-clear residue, reset FLAGS
f002, and CPU_SHUT FLAGS 0000 remain unchanged. The M48 gate reports all 522
REP+0F cases state-and-memory atomic with the same diagnostic stop.

## M43 exact preservation

The verified pinned dataset identity remains:

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

Both independent `--expect` runs passed. Generated JSON and complete failure
directories compare byte-identically with the accepted baselines. Their JSON
SHA-256 values are respectively
`a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31`
and `dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9`.

## M42--M50 preservation summary

- M42 graph, provenance, support, trace, 156-case harness, ABI, reset,
  executed-3, and CPU_SHUT fixtures are exact.
- M43 dataset, classifications, 40/68,626 known gaps, all CI/full failure
  signatures, and zero timeout/crash counts are exact.
- M44 state layout, transactional adapter, G41/current matrix, opaque residue,
  and atomic rejection are exact.
- M45 still has unconditional native V30 execution, no CPU_TYPE runtime
  dispatch, and no `i286c_step`.
- M46 still has one constructor, immutable six roots, and no `i286c()`,
  `v30c()`, `CPU_EXEC`, or `CPU_EXECV30`.
- M48 retains the exact REP+0F diagnostic, 522-case pre-mutation atomicity,
  and protected-state rejection.
- M49 inventory and M50 52-row deletion manifest remain exact. Only the three
  approved groups are absent; final graph and the accepted nine-row M50
  provenance transition are unchanged.

## Toolchains, production isolation, and frozen references

| Gate | Result |
|---|---|
| GCC tests-enabled | fresh configure/build; CTest 34 passed, one external-data skip, 0 failed |
| Clang tests-enabled | fresh configure/build; same 35-test result |
| GCC ASan/UBSan | fresh instrumented build; same 35-test result; focused halt-on-error 6/6 |
| GCC release | fresh tests-disabled build; selftest and ROM-less smoke pass |
| MinGW-w64 cross | fresh tests-disabled build; Wine selftest and ROM-less smoke pass |
| hosted Linux/Windows/macOS/standalone | build 29665120771 at the pre-report SHA; all seven jobs succeeded |
| repository invariants | encoding 0, EOL 0, case 0, diff/frozen/binary checks pass; accepted unreferenced count 70 |

Both production caches have `VAEG_ENABLE_TESTS=OFF` and
`VAEG_Z80_INTEGRATION_TRACE=OFF`. ELF and PE contain no M46/M48 test CLI,
M51 checker text, CTest name, or dispatch-test symbol. The production M48
diagnostic API and guest-facing message remain by design.

`win9x/`, `i286x/`, `hlp/`, `romimage/`, and `cpuxva/memoryva.x86` compare
unchanged with G50. The frozen memory reference SHA-256 remains
`e4e56460fdadc16d8a25fdfd11241c0253e30c84404f1221c187939dc666225e`.

## Exact validation commands and statuses

The applicable commands below exited zero:

```text
cmake -S . -B /home/maho/vaeg/build/m51-linux-ci-gcc -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_INTEGRATION_TRACE=ON
cmake -S . -B /home/maho/vaeg/build/m51-linux-ci-clang -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_INTEGRATION_TRACE=ON \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --fresh -S . -B /home/maho/vaeg/build/m51-linux-ci-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_INTEGRATION_TRACE=ON \
  -DCMAKE_C_FLAGS=-fsanitize=address,undefined \
  -DCMAKE_CXX_FLAGS=-fsanitize=address,undefined \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
cmake -S . -B /home/maho/vaeg/build/m51-linux-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=OFF
cmake --fresh -S . -B /home/maho/vaeg/build/m51-mingw-cross -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=OFF \
  -DVAEG_WINDOWS_CONSOLE=OFF -DVAEG_FETCH_SDL2=ON \
  -DVAEG_STATIC_SDL2=ON -DVAEG_FETCH_LIBARCHIVE=ON \
  -DCMAKE_TOOLCHAIN_FILE=/tmp/vaeg-m51-upd9002/cmake/mingw-w64-x86_64.cmake
cmake --build /home/maho/vaeg/build/m51-linux-ci-gcc --parallel 4
cmake --build /home/maho/vaeg/build/m51-linux-ci-clang --parallel 4
cmake --build /home/maho/vaeg/build/m51-linux-ci-asan --parallel 4
cmake --build /home/maho/vaeg/build/m51-linux-release --parallel 4
cmake --build /home/maho/vaeg/build/m51-mingw-cross --parallel 4

ctest --test-dir /home/maho/vaeg/build/m51-linux-ci-gcc --output-on-failure
ctest --test-dir /home/maho/vaeg/build/m51-linux-ci-clang --output-on-failure
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=0 \
  ctest --test-dir /home/maho/vaeg/build/m51-linux-ci-asan \
  --output-on-failure
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir /home/maho/vaeg/build/m51-linux-ci-asan \
  --output-on-failure \
  -R 'vaeg_upd9002_(dispatch_normalization|rep0f_diagnostic_stop|protected_deletion|rename|state_boundary|state_payload_probe)$'

python3 tools/qa/upd9002_rename.py --root .
python3 tools/qa/upd9002_protected_deletion.py --root .
python3 tools/qa/upd9002_protected_deletion.py --root . --selftest
python3 tools/qa/upd9002_rep0f_transition.py --root .
python3 tools/qa/upd9002_rep0f_transition.py --root . --selftest
python3 tools/qa/upd9002_native_invariant.py --root .
python3 tools/qa/upd9002_dispatch_normalization.py --root .
/home/maho/vaeg/build/m51-linux-ci-gcc/sdl2/vaeg \
  --upd9002-m46-dispatch-qa
/home/maho/vaeg/build/m51-linux-ci-gcc/sdl2/vaeg \
  --upd9002-m48-rep0f-diagnostic

python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /home/maho/vaeg/build/m51-linux-ci-gcc/sdl2/vaeg \
  --profile ci --shard-timeout 120 \
  --output /home/maho/vaeg/build/m51-ssts/v20_native_ci.json \
  --failure-directory /home/maho/vaeg/build/m51-ssts/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /home/maho/vaeg/build/m51-linux-ci-gcc/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /home/maho/vaeg/build/m51-ssts/v20_native_full.json \
  --failure-directory /home/maho/vaeg/build/m51-ssts/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
cmp /home/maho/vaeg/build/m51-ssts/v20_native_ci.json \
  tests/ssts/baseline/v20_native_ci.json
cmp /home/maho/vaeg/build/m51-ssts/v20_native_full.json \
  tests/ssts/baseline/v20_native_full.json
diff -qr /home/maho/vaeg/build/m51-ssts/v20_native_ci_failures \
  tests/ssts/baseline/v20_native_ci_failures
diff -qr /home/maho/vaeg/build/m51-ssts/v20_native_full_failures \
  tests/ssts/baseline/v20_native_full_failures

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
VAEG_M44_SCENARIO_DIR=/home/maho/vaeg/build/m51-matrix/current-dummy \
  /home/maho/vaeg/build/m51-linux-ci-gcc/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
VAEG_M44_SCENARIO_INPUT_DIR=/tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
VAEG_M44_SCENARIO_OUTPUT_DIR=/home/maho/vaeg/build/m51-matrix/g41-to-m51-dummy \
  /home/maho/vaeg/build/m51-linux-ci-gcc/sdl2/vaeg --selftest
python3 tools/qa/upd9002_state_matrix.py \
  --fixtures tests/upd9002/state_fixtures_m42.txt \
  --g41-generated /tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
  --m44-generated /home/maho/vaeg/build/m51-matrix/current-dummy \
  --g41-to-m44 /home/maho/vaeg/build/m51-matrix/g41-to-m51-dummy \
  --m44-to-g41 /tmp/vaeg-m47-matrix.i6ajc2/m47-to-g41 \
  --g41-sha dc8a72da974f0ea328613e480f1de662c28f4436 \
  --m44-sha bb562fc

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /home/maho/vaeg/build/m51-linux-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /home/maho/vaeg/build/m51-linux-release/sdl2/vaeg --smoke
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m51-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 wineboot -u
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m51-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 wine64 \
  /home/maho/vaeg/build/m51-mingw-cross/sdl2/vaeg.exe --selftest
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m51-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 wine64 \
  /home/maho/vaeg/build/m51-mingw-cross/sdl2/vaeg.exe --smoke

nm -g --defined-only /home/maho/vaeg/build/m51-linux-release/sdl2/vaeg
x86_64-w64-mingw32-nm -g --defined-only \
  /home/maho/vaeg/build/m51-mingw-cross/sdl2/vaeg.exe
strings /home/maho/vaeg/build/m51-linux-release/sdl2/vaeg
strings /home/maho/vaeg/build/m51-mingw-cross/sdl2/vaeg.exe

git diff --exit-code 336227f093d37e3c60bc50333216d66668755cef -- \
  tools/qa/golden/upd9002_final_dispatch_graph.csv \
  tools/qa/golden/upd9002_dispatch_provenance_m42.csv \
  tools/qa/golden/upd9002_support_map_m42.csv \
  tests/upd9002/harness_manifest.csv tests/upd9002/trace_baseline.txt \
  tests/upd9002/state_fixtures_m42.txt tests/upd9002/abi_g41.txt
git diff --exit-code 8b818fca173b2e86b4a8af20e75bc069f7697cdf -- \
  tools/qa/golden tests/ssts
git diff --exit-code 8b818fca173b2e86b4a8af20e75bc069f7697cdf -- \
  win9x i286x cpuxva/memoryva.x86 hlp romimage
python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check
git push -u origin topic/m51-upd9002-rename
gh run view 29665120771 --json status,conclusion,url,headSha,jobs
```

## Deviations and remaining risks

- `/tmp` was already 99% full before final validation. Only the task-generated
  `/tmp/vaeg-m51-upd9002/build` directory was removed; it is reproducible and
  contained no source or user asset. Fresh final builds were placed below
  `/home/maho/vaeg/build/m51-*`.
- An initial MinGW command copied an obsolete report's nonexistent
  `cmake/toolchains/mingw-w64-x86_64.cmake` path and exited 1. The current
  preset's `cmake/mingw-w64-x86_64.cmake` path was then used in a fresh
  configure, build, Wine selftest, and smoke, all successful.
- An exploratory ASan configure used the obsolete no-op
  `VAEG_ENABLE_SANITIZERS` cache name; CMake warned but exited zero. The build
  directory was immediately recreated with explicit address/undefined
  sanitizer compile and linker flags before any accepted sanitizer result.
- A debug preset cache had tests disabled, so an exploratory filtered CTest
  exited zero with "No tests were found". All three applicable tests-enabled
  fresh builds subsequently ran 35 tests with zero failure.
- An exploratory JSON query expected a nonexistent `summary` key and exited 1.
  A corrected read of the committed schema verified dataset, counts,
  termination classes, failure counts, and signature indices.
- Native Windows and macOS builds are provided by hosted CI rather than this
  Linux host. The MinGW cross/Wine result supplies an independent local Windows
  artifact check.
- Real uPD9002/V52 REP+0F semantics remain unknown. M51 preserves the accepted
  fail-closed M48 policy and does not claim architectural correctness.
- Historical internal basenames, graph-bound REP handlers, selector/state
  compatibility identifiers, and frozen references intentionally remain. They
  are not public ownership names and are guarded by exact policy.
- The standard V3 boot, bundled VA demo, OS, keyboard, media, sound, save/load,
  and reset gate is a human check. This report neither performs nor approves
  it.

## G51 human-review checklist

- [ ] Confirm every pure move is followed immediately by its reference-fix
  commit and that only the three pure move SHAs are blame-ignored.
- [ ] Confirm the exact directory/file move map and canonical nine-core plus
  two-register API map.
- [ ] Confirm no retired public wrapper, active `i286c/` path, active
  `cpuxva` include path, or generic `upd9002` register global remains.
- [ ] Review the exact 18 internal REP-helper exceptions and verify there is no
  wildcard exemption.
- [ ] Confirm `CPU286` and `UPD9002` tags, 112/16-byte payloads,
  transactional rejection, opaque residue, and CPU_SHUT FLAGS 0000.
- [ ] Confirm the final graph, M50 provenance, M42 fixtures, M43 CI/full
  signatures, M46 immutable constructor, and M48 522-case diagnostic results.
- [ ] Confirm frozen references and production-isolation checks.
- [ ] Review hosted Linux, Windows, macOS, and standalone Z80 CI.
- [ ] From a clean checkout, build and boot in V3 mode.
- [ ] Run the bundled VA demo.
- [ ] Boot an OS and verify keyboard input and FDD read.
- [ ] If practical, verify FDD write/readback, Sound Board II, save/load
  continuation, and reset/reboot.
- [ ] Confirm no test log, CLI seam, or diagnostic appears during ordinary
  supported execution.
- [ ] Explicitly approve or reject G51.
