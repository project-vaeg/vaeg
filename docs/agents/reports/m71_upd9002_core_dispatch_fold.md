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
# M71 - uPD9002 core dispatch fold report

Status: M71 implementation candidate prepared for G71 human review. G71 is not
approved by this report.

## Starting point

- Branch: `topic/m71-upd9002-core-dispatch-fold`
- Starting SHA: `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`
- Approved predecessor: G70 at
  `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`

## Commit list

- `1b8ba2a8` - `M71: define core dispatch fold cleanup`
- `74423dd0` - `M71: fold dispatch tables into uPD9002 core`
- `caed63de` - `M71: update folded dispatch validation`
- `f3bb9398` - `M71: close core dispatch fold evidence`
- `d08f0917` - `M71: support legacy SST support maps after dispatch fold`

The final branch head is reported in the handoff after the report-update
commit.

## Files removed

- `cpu/upd9002/upd9002_dispatch.c`
- `cpu/upd9002/upd9002_dispatch.h`

Both files are removed from the active tree. `CMakeLists.txt` no longer lists
`cpu/upd9002/upd9002_dispatch.c` in `VAEG_CORE_SOURCES`.

## Production fold

The former dispatch translation unit is folded into the active uPD9002 core
sources:

- `upd9002_core_step()` now lives in `cpu/upd9002/upd9002_core.c`.
- The canonical root tables now live in `cpu/upd9002/upd9002_mn.c`:
  `upd9002op`, `upd9002op_repe`, `upd9002op_repne`,
  `upd9002op_repc`, and `upd9002op_repnc`.
- The canonical 0F table is `upd9002_ope0x0f_table`.
- F6/F7 division tables remain in `cpu/upd9002/upd9002_f6.c` as
  `c_ope0xf6_table` and `c_ope0xf7_table`.
- The M46 dispatch immutability test seam remains available through
  `cpu/upd9002/cpucore.h` and is implemented against the folded static tables.

## Naming transition

M71 replaces the dispatch-era `v30` handler/table names with canonical
uPD9002 names in the current production path.

Examples:

- `v30_idiv_ea8` -> `_idiv_ea8`
- `v30_div_ea8` -> `_div_ea8`
- `v30_idiv_ea16` -> `_idiv_ea16`
- `v30_div_ea16` -> `_div_ea16`
- `v30_ope0x0f` -> `_ope0x0f`
- `v30_repc` -> `_repc`
- `v30_repnc` -> `_repnc`
- `v30push_sp` -> `_push_sp`
- `v30mov_seg_ea` -> `_mov_seg_ea`

The old canonical implementations that collided with replacement handlers were
not retained beside the new names. They were replaced by the uPD9002 production
semantics that previously entered through the dispatch patch tables.

The tables are intentionally not all placed in `upd9002_core.c`: the relevant
handler macros expand to static functions in `upd9002_mn.c` and
`upd9002_f6.c`. Keeping the tables in those translation units avoids exporting
formerly static handler internals solely for the refactor.

`CPUTYPE_V30` remains unchanged because it is the approved legacy serialized
compatibility byte, not a runtime dispatch selector.

## Current generated metadata

The current dispatch generator now reads the folded static tables directly and
emits canonical table and handler names:

| Artifact | Rows including header | SHA-256 |
| --- | ---: | --- |
| `tools/qa/golden/upd9002_final_dispatch_graph.csv` | 1,514 | `3203d510afd11982460ad27704ca06d7cf8cf7a58d0abf05c437c5ef520e049c` |
| `tools/qa/golden/upd9002_dispatch_provenance_m42.csv` | 1,297 | `0a615838c943305f629a97e608f3767fb47bacbdc2a843dd805ec68eab25d920` |
| `tools/qa/golden/upd9002_support_map_m42.csv` | 1,553 | `2e0fb98a58cbfea483535fae451b4f632a806d0cde6c21bf5dd8737383950c1e` |
| `tests/upd9002/harness_manifest.csv` | 194 | `1a1afd169ee75195e9c299589e3f3abff742a2f7de19aed887734f8ca199ed55` |

Approved historical G70 artifacts were not rewritten. The M70 validator now
accepts the historical G70 dispatch CSV family as approved evidence while
checking that current folded dispatch regeneration matches the current golden
counterparts.

## Post-push G65m CI support-map compatibility

The hosted G65m CI entry point intentionally runs against the G64 target policy
support map. That generated G64 map still uses the historical dispatch table
family names:

- `v30op`
- `v30op_repe`
- `v30op_repne`
- `v30op_repc`
- `v30op_0f`
- `v30ope0xf6_table`
- `v30ope0xf7_table`

The first M71 candidate normalized the SST dispatch resolver to the current
folded `upd9002op`/`c_ope...` table family unconditionally. That made the G65m
CI classifier look up current table names in a predecessor G64 support map and
fail closed with:

```text
g65m-ci-error: 00: no M42 support-map row for ('upd9002op', 0, '-')
```

The first compatibility fix also exposed that the G64 map does not contain a
historical `v30op_repnc` table. For the G64 policy, `64 A4` and related
pre-M70 REPNC string forms must therefore resolve to the historical root
prefix row for opcode `64`, not to a nonexistent repeat-string table:

```text
g65m-ci-error: A4: no M42 support-map row for ('v30op_repnc', 164, '-')
```

M71 now resolves the support-map table family from the actual support map
passed to `tools/qa/upd9002_ssts.py`. Current folded maps resolve through the
`upd9002op` family and use `upd9002op_repnc` when present. G64 predecessor maps
resolve through the historical `v30op` family and fall back to the opcode-`64`
prefix row when no REPNC table exists. This keeps historical target-policy
replay compatible without restoring production V30 dispatch names.

## Validation

Commands run on macOS in this worktree:

| Command | Exit | Result |
| --- | ---: | --- |
| `python3 tools/qa/upd9002_ssts.py selftest` | 0 | SST resolver selftest passed for current folded and historical G64 table families |
| Focused G64 support-map dispatch probe for forms `00` and `64 A4` | 0 | Resolved `00` through `v30op` and `64 A4` through the G64 opcode-`64` prefix row |
| `ctest --test-dir build/linux-ci-clang -R 'vaeg_upd9002_ssts_selftest\|vaeg_upd9002_ssts_ci_baseline\|vaeg_upd9002_ssts_full_baseline\|vaeg_upd9002_m70_prefix_string_selftest\|vaeg_upd9002_m70_prefix_string_static' --output-on-failure` | 0 | 5 focused tests passed |
| `python3 tools/qa/upd9002_dispatch.py --root . --write --selftest` | 0 | Regenerated and verified folded graph, provenance, support map and harness manifest |
| `python3 tools/qa/upd9002_dispatch_normalization.py --root .` | 0 | Folded canonical roots verified; retired dispatch source/header/constructor absent |
| `python3 tools/qa/upd9002_native_invariant.py --root .` | 0 | Native lifecycle invariant passed with `upd9002_core_step` and folded reset initializer |
| `python3 tools/qa/upd9002_rename.py --root .` | 0 | Retired public APIs and retired active dispatch paths absent |
| `python3 tools/qa/upd9002_rep0f_transition.py --root . --selftest` | 0 | Folded REP+0F diagnostic-stop graph verified |
| `python3 tools/qa/upd9002_protected_deletion.py --root . --selftest` | 0 | Protected M50 deletion evidence and current folded graph verified |
| `python3 tools/qa/upd9002_m62_bundle.py verify-static --root .` | 0 | M62 static authority remained valid after folded names |
| `python3 tools/qa/upd9002_m70_prefix_string.py selftest --root .` | 0 | M70 population validator selftest passed |
| `python3 tools/qa/upd9002_m70_prefix_string.py verify --root .` | 0 | M70 approved artifacts verified with folded dispatch equivalence |
| `cmake --build build/linux-ci-clang -j 4` | 0 | Test-enabled Clang build passed |
| `ctest --test-dir build/linux-ci-clang --output-on-failure` | 0 | 73 passed, 1 skipped (`vaeg_upd9002_ssts_ci_external`), 0 failed |
| `ctest --test-dir build/linux-ci-clang --output-on-failure` after G65m compatibility fix | 0 | 73 passed, 1 skipped (`vaeg_upd9002_ssts_ci_external`), 0 failed |
| `cmake --build build/linux-debug --target vaeg -j 4` | 0 | Normal debug build passed |
| `git diff --check` | 0 | Whitespace check passed |

Build artifact digests:

- `build/linux-ci-clang/sdl2/vaeg`:
  `98cda664ea2e97efe2e04c84e041b309f289ee77e0a0ed5db2f09da7ad737618`
- `build/linux-debug/sdl2/vaeg`:
  `3759ebabe03850a8ba35e3a980baa71616de26bc8e5d77712e8a00859eddbfa8`

CTest protected results included:

- M65a FF /7: pass
- M65b BOUND: pass
- M65c F7 /2: pass
- M65d FF /6: pass
- M65e tail10: pass
- M68 segmented mapped-memory protection: pass
- M69 IDP 0142H status composition protection: pass
- M70 prefix/string protection: pass

## Scope audit

No intentional changes were made to:

- target policy semantics;
- SST corpus records;
- state format or state compatibility schema;
- memory mapping behavior;
- TSP/IDP behavior;
- Z80 behavior;
- frontend behavior.

The remaining `V30` spellings in active uPD9002 production files are limited
to `CPUTYPE_V30` state-compatibility checks and messages. They are not current
dispatch table names or handler names.

## Remaining risks

- The change is large because folding a translation unit necessarily moves many
  static handlers and dispatch tables into one compilation boundary.
- Hosted CI has not yet been run for the final M71 branch candidate in this
  report snapshot.
- MinGW was not rerun in this snapshot.

## G71 human-review checklist

- Build from a clean checkout of the final M71 branch.
- Boot in V3 mode.
- Run the bundled VA demo.
- Boot PC-Engine/MS-DOS and verify `DIR A:`, `CHKDSK A:`, multi-screen text
  output, `CLS`, save/load, and Sound Board II.
- Confirm that `cpu/upd9002/upd9002_dispatch.c` and
  `cpu/upd9002/upd9002_dispatch.h` are absent.
- Confirm that G71 is approved only by the maintainer.
