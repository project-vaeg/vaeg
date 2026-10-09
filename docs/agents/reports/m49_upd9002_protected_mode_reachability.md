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
# M49 uPD9002 protected-mode reachability after M48

Status: the post-M48 audit, deterministic inventory, local validation, and
hosted validation are complete. This report proposes three exact
dependency-closed groups for maintainer consideration at G49. It does not
delete a group, alter CPU or state behavior, begin M50/M51, or declare G49
passed.

The final report/remote SHA is supplied in the handoff because this file cannot
contain the SHA of the commit that contains itself.

## Identity and scope

- Branch: `topic/m49-upd9002-protected-mode-reachability`
- Exact starting and approved G48 SHA:
  `339f5f62b3e69611f66f6689be8798f1c675b2cf`
- Pre-report audit SHA:
  `7fd80f7299402e2f5c324b89bf3edfc2b56e012c`
- Hosted evidence and remote report SHA:
  `a4cde3140bf00d9beb6c471c14833a2d10084725`
- Hosted evidence run:
  [build 29659803344](https://github.com/nakatamaho/vaeg/actions/runs/29659803344)
  — 7/7 jobs successful
- Accepted G48 branch: `topic/m48-upd9002-rep0f-fail-closed`

The worktree started clean at the exact approved G48 SHA. No reset, rebase,
squash, force push, or M42--M48 history rewrite was performed. The checked-in
M49 task was first corrected from the obsolete rename wording to the approved
M49 reachability / M50 deletion / M51 rename sequence. ROADMAP and the master
plan already described that sequence and did not require another edit.

M49 changes only the task document, deterministic audit tooling, a generated
inventory, tests-only CMake wiring, and this report. No production instruction,
dispatch, state, timing, I/O, interrupt, exception, reset, or CPU_SHUT source is
changed.

## Commits and files

Commits before this report are:

1. `2e53ccb6c2be98557aa50db64445c82c3e8ee933` —
   `M49: correct post-M48 reachability audit task`;
2. `7fd80f7299402e2f5c324b89bf3edfc2b56e012c` —
   `M49: add deterministic protected-mode reachability inventory`.

Paths changed before this report are:

```text
CMakeLists.txt
docs/agents/tasks/M49_upd9002_isolate_np2_286_protected_mode.md
tools/qa/golden/upd9002_286_reachability_m49.csv
tools/qa/upd9002_protected_reachability.py
```

This report adds:

```text
docs/agents/reports/m49_upd9002_protected_mode_reachability.md
```

No frozen-reference or binary payload path changed. No protected handler,
runtime field, macro, declaration, source file, or CMake source edge was
deleted.

## Accepted M48 boundaries

The audit begins from, and re-proves, these two accepted boundaries.

The REP+0F execution paths are:

```text
v30c_step
  -> v30op[0xf2] -> v30_repne -> v30op_repne[0x0f]
                                  -> v30_repne_0f_diagnostic_stop
  -> v30op[0xf3] -> v30_repe  -> v30op_repe[0x0f]
                                  -> v30_repe_0f_diagnostic_stop
```

Both handlers latch the diagnostic. `v30c_step()` restores the complete
`Upd9002RuntimeState`, omits the DMA callback, and returns. `pccore_exec()`
checks the latch before scheduling and again immediately after the step, before
other VA devices execute. Therefore F2/F3 0F xx, including F2/F3 0F 01 F0,
cannot reach `i286c_cts`, `cts0_table`, `cts1_table`, or their handlers.

The state path is:

```text
statsave_load/check
  -> CPU286 preflight
  -> upd9002_state_validate
  -> reject when Cpu286StateCompat.MSW & MSW_PE
  -> no Cpu286CompatImage or Upd9002RuntimeState commit
```

The exact compatibility error remains:

```text
CPU286 state requires unsupported 80286 protected-mode execution
```

PE-clear descriptor residue is still accepted and round-trips through the
M44 opaque-image adapter. This audit does not reinterpret the fail-closed rule
as architectural uPD9002/V52 semantics.

## Post-M48 reachability graph

### Final dispatch roots and protected CTS cluster

All six constructed roots were regenerated from source and compared with the
accepted post-M48 graph. None has an edge to `i286c_cts`, `cts0_table`,
`cts1_table`, or a protected system handler.

| Root | Entries | Protected CTS edge after construction |
|---|---:|---|
| `v30op` | 256 | none; 0F is `v30_ope0x0f` |
| `v30op_repne` | 256 | none; 0F is the F2 diagnostic stop |
| `v30op_repe` | 256 | none; 0F is the F3 diagnostic stop |
| `v30op_repc` | 256 | none |
| `v30ope0xf6_table` | 8 | none |
| `v30ope0xf7_table` | 8 | none |

Before the M46 constructor patches are applied, the three source base tables
still contain `i286c_cts` at slot 0F. Those addresses are copied only into
scratch-under-construction roots and are necessarily overwritten before the
single successful construction completes or its immutability snapshot is
taken. There is no block executor, alternate constructor, direct production
caller, or post-construction table write that can expose a base slot.

`i286c_cts` is the only consumer of `cts0_table` and `cts1_table`. Those tables
are the only address-taking owners of their 16 entries. `_loadall286` has only
the dispatcher call. Consequently the complete CTS/system-instruction closure
has no final, secondary, direct, lifecycle, I/O, or state-execution root after
M48.

### Remaining conditional selector path

Protected-state import rejection removes the accepted external path that sets
PE, but source-level native handlers still expand `SEGSELECT`. The audit does
not infer source unreachability solely from the current absence of a PE writer.
The remaining edges are:

```text
v30op/v30op_repe/v30op_repne
  [07] -> _pop_es
  [17] -> _pop_ss
  [1f] -> _pop_ds
  [9a] -> _call_far
  [c4] -> _les_r16_ea
  [c5] -> _lds_r16_ea
  [ca] -> _ret_far_data16
  [cb] -> _ret_far
  [ea] -> _jmp_far

c_ope0xff_table
  [03] -> _call_far_ea16
  [05] -> _jmp_far_ea16

each caller -> SEGSELECT(selector)
  -> MSW.PE clear: selector << 4
  -> MSW.PE set: i286c_selector(selector)
                   -> GDTR or LDTRC
```

All 11 final-reachable callers, `SEGSELECT`, `i286c_selector`, `MSW`, `GDTR`,
and `LDTRC` are therefore retained. The two relocations from unreachable
`_lldt` and `_ltr` into `i286c_selector` disappear only if the CTS group is
later approved and deleted; they are not used as evidence for deleting the
selector.

Interrupt and exception paths remain the active real-mode implementations:
`i286c_intnum`, `i286c_interrupt`, and `v30_iret`. They do not read IDTR or a
protected descriptor cache. They are retained as active native behavior.

### Overwritten individual handlers

Two same-translation-unit candidates are independently closed:

- `_arpl` is address-taken only by slots 63 in `i286op`, `i286op_repe`, and
  `i286op_repne`. Each is patched to `v30_reserved` before publication.
- `_mov_seg_ea` is address-taken only by slots 8E in those same base tables.
  Each is patched to `v30mov_seg_ea`. The latter is the active native handler;
  it does not use `SEGSELECT`.

Neither candidate has a direct call or final-graph edge. Their active
translation unit `i286c_mn.c` and all helpers used elsewhere must remain.

### Static and linker evidence

The source checker verifies exact protected symbol counts, all three relevant
base slots, the complete CTS arrays and handler set, all 12 source SEGSELECT
callers (11 active plus `_mov_seg_ea`), all state offsets, and the M48
diagnostic patterns. Unknown handler/table syntax or a changed count fails the
generator.

The tests-disabled release binary still contains `_arpl`, `_mov_seg_ea`,
`cts0_table`, `cts1_table`, `i286c_cts`, and `i286c_selector`, as expected
before M50. ELF relocations show three base-table address references to
`i286c_cts`, ten selector calls from `i286c_mn.c` (including the unreachable
`_mov_seg_ea`), two from active FF-group handlers, and two from the unreachable
CTS handlers. Linker presence is recorded only as corroboration; final-graph,
direct-reference, state, and lifecycle closure are the reachability proof.

## Runtime versus serialized state

`Cpu286StateCompat` remains exactly 112 bytes. `Upd9002RuntimeState` retains the
same corresponding fields, and `Cpu286CompatImage` retains all opaque bytes.

| Field | Offset | Size | Runtime dependencies after M48 | Import/export and lifecycle | M50 disposition |
|---|---:|---:|---|---|---|
| MSW | 70 | 2 | read by `SEGSELECT` and `CPU_MSW`; PE tested by preflight | imported only when PE clear; overlaid on export; zeroed by reset/CPU_SHUT | shared; retain |
| GDTR | 64 | 6 | read/address-taken by `i286c_selector` | imported, overlaid, zeroed | shared; retain |
| IDTR | 72 | 6 | no post-M48 instruction reader | imported, overlaid, zeroed | serialized compatibility; defer runtime removal |
| LDTR | 78 | 2 | no post-M48 instruction reader | imported, overlaid, zeroed | serialized compatibility; defer runtime removal |
| LDTRC | 80 | 6 | read/address-taken by `i286c_selector` | imported, overlaid, zeroed | shared; retain |
| TR | 86 | 2 | no post-M48 instruction reader | imported, overlaid, zeroed | serialized compatibility; defer runtime removal |
| TRC | 88 | 6 | no post-M48 instruction reader | imported, overlaid, zeroed | serialized compatibility; defer runtime removal |

`state_construct_runtime()` presently imports bytes 0--93 and 96--111 into a
temporary runtime object, then commits the runtime and opaque image together.
`upd9002_state_export()` starts with the opaque image and overlays the same
runtime-owned ranges. Normal reset zeros the full runtime and image. CPU_SHUT
zeros runtime and compatibility bytes below `cpu_type`, preserving the
historical FLAGS 0000 transformation. `cpuio_of0()` reads `CPU_MSW & 1`, records
R/P mode, disables A20, and requests reset.

IDTR, LDTR, TR, and TRC are not execution-reachable, but removing their runtime
ownership would change the adapter contract. A future group would have to keep
their exact serialized positions solely in `Cpu286CompatImage`, stop importing
them into runtime, stop overlaying them on export, define the reset/CPU_SHUT
opaque-byte transformation, and re-prove G41/current compatibility. M49 does
not specify or propose that wider adapter change, so these fields are deferred.

## Deterministic inventory

The generated inventory is:

```text
tools/qa/golden/upd9002_286_reachability_m49.csv
SHA-256 f3843cd57b57af8f5baa4a180a7a30c88d628d0b12865d6a4a451a306794c15b
130 candidates
```

Counts by the required exclusive disposition are:

| Disposition | Count |
|---|---:|
| `active_native_required` | 34 |
| `fail_closed_diagnostic_required` | 11 |
| `cpu_shut_compatibility_required` | 2 |
| `serialized_compatibility_only` | 11 |
| `test_evidence_only` | 5 |
| `frozen_reference_only` | 1 |
| `unreachable_286_candidate` | 52 |
| `shared_helper` | 14 |
| `unresolved` | 0 |

The generator deterministically sorts stable candidate IDs, emits LF UTF-8
without host paths or pointer values, and compares byte-for-byte in check mode.
It fails on an omitted known candidate, duplicate ID, unknown disposition,
unresolved row, non-candidate group member, unknown group, incomplete group
set, changed M48 graph/dispatch edge, changed protected symbol/table syntax,
changed SEGSELECT caller set, changed state layout/adapter/lifecycle pattern, or
changed M48 diagnostic/atomicity pattern. Its negative selftests cover malformed
arrays, duplicate candidates, and a shared helper incorrectly placed in a
deletion group.

The two inventory CTests are target-local and wired only under
`VAEG_ENABLE_TESTS`. They add no production source, define, CLI, symbol, or
diagnostic seam.

## A. Proposed M50 deletion groups

The following identifiers are proposals only. G49 must explicitly approve each
identifier; unapproved or modified groups must not be deleted in M50.

### M50-PM-ARPL

Exact members (4):

```text
_arpl
i286op[0x63] -> _arpl
i286op_repe[0x63] -> _arpl
i286op_repne[0x63] -> _arpl
```

- Incoming dependencies: the three constructor-only base initializers.
- Outgoing dependencies: opcode fetch, work-clock, and real-mode exception
  macros inside `_arpl`; no callee is exclusive to this group.
- Reachability: all three slots are patched to `v30_reserved`; no direct call,
  final edge, secondary edge, lifecycle, state, I/O, or diagnostic dependency.
- State/serialization/CPU_SHUT impact: none.
- Retained replacement required in M50: replace each source base entry with the
  existing `_reserved` constructor placeholder while retaining the accepted
  `v30_reserved` final patch. This keeps all source arrays at 256 elements and
  does not redirect any final slot.
- Tests/tooling required: update the M49 checker to require the placeholder and
  absence of `_arpl`; regenerate a successor inventory; re-prove the M48 final
  graph, provenance, support map, M43 results, constructor immutability, and
  production symbols.
- CMake change: none; the shared `i286c_mn.c` remains.
- Risk: low, with the principal risk being an array-index shift or accidental
  final-patch removal.
- Exact M50 acceptance proof: source/direct/address inventory contains no
  `_arpl`; the three final rows remain `v30_reserved`; six-table snapshot,
  trace, timings, state fixtures, and V20 signatures remain exact.

### M50-PM-MOV-SEG-EA

Exact members (4):

```text
_mov_seg_ea
i286op[0x8e] -> _mov_seg_ea
i286op_repe[0x8e] -> _mov_seg_ea
i286op_repne[0x8e] -> _mov_seg_ea
```

- Incoming dependencies: the three constructor-only base initializers.
- Outgoing dependencies: `_mov_seg_ea` calls shared EA/memory helpers and
  `SEGSELECT`; none is part of this group.
- Reachability: every slot is patched to active `v30mov_seg_ea`; no direct
  caller or final/secondary edge reaches `_mov_seg_ea`.
- State/serialization/CPU_SHUT impact: the removed function can conditionally
  read MSW/GDTR/LDTRC, but no accepted root reaches that read. All fields,
  `SEGSELECT`, `i286c_selector`, and active `v30mov_seg_ea` remain.
- Retained replacement required in M50: use `_reserved` as the three source
  base placeholders while retaining the `v30mov_seg_ea` final patches.
- Tests/tooling required: update the checker to require the placeholders and
  function absence; regenerate a successor inventory; prove all three final
  8E rows still name `v30mov_seg_ea`; rerun direct harness, trace, timing,
  constructor, M43, and state gates.
- CMake change: none; the shared `i286c_mn.c` remains.
- Risk: medium because an active native implementation with a similar name is
  in another translation unit and must not be confused with this candidate.
- Exact M50 acceptance proof: no `_mov_seg_ea` definition/reference remains;
  active `v30mov_seg_ea` and three final rows are unchanged; selector and state
  ownership remain; all preservation gates are exact.

### M50-PM-CTS-SYSTEM

Exact members (44 inventory rows):

```text
i286op[0x0f] -> i286c_cts
i286op_repe[0x0f] -> i286c_cts
i286op_repne[0x0f] -> i286c_cts
i286c_cts declaration and definition
cts0_table and its 8 entries:
  _sldt, _str, _lldt, _ltr, _verr, _verw, _verr, _verw
cts1_table and its 8 entries:
  _sgdt, _sidt, _lgdt, _lidt, _smsw, _smsw, _lmsw, _lmsw
13 unique handlers:
  _sldt, _str, _lldt, _ltr, _verr, _verw,
  _sgdt, _sidt, _lgdt, _lidt, _smsw, _lmsw, _loadall286
I286_0F
I286OP_0F
I286_IDTR
I286_LDTR
I286_TR
I286_TRC
i286c/i286c_0f.c
VAEG_CORE_SOURCES entry for i286c/i286c_0f.c
```

- Incoming dependencies: three constructor-only base initializer addresses;
  the declaration and CMake edge; tests/static tools that record absence from
  the final graph. There is no post-M48 production caller.
- Outgoing dependencies: protected handlers reference shared exception,
  memory, effective-address, selector, GDTR/LDTRC/MSW, and runtime-state
  facilities. None of those shared facilities is included. `_lldt` and `_ltr`
  call retained `i286c_selector`.
- Reachability: M48 replaces normal 0F with `v30_ope0x0f`, F2 0F with the REPNE
  diagnostic, and F3 0F with the REPE diagnostic. The complete accepted final
  graph contains no CTS/table/handler target. The 522-case suite proves all
  F2/F3 second bytes and F2/F3 0F 01 F0 stop before mutation.
- State/lifecycle: legacy handlers would read/write protected fields, but the
  dispatcher is unreachable. Runtime fields, state adapter, PE rejection,
  reset, CPU_SHUT, `CPU_MSW`, selector, interrupt, and exception logic remain.
- Serialized-layout impact: none. The six removed header aliases are not
  storage; every `Cpu286StateCompat` and `Upd9002RuntimeState` member and the
  `I286DTR` type remain.
- Retained replacement required in M50: replace each base slot 0F with existing
  `_reserved` solely as a constructor placeholder. Keep all three accepted
  final patches, both diagnostic handlers, and the native unprefixed V30 0F
  handler. Remove `i286c_0f.c` from CMake only after all base references and
  declaration/header-only dependencies are removed.
- Tests/tooling required: replace the M49 source-presence assertions with
  source-absence and placeholder checks; retain the M42/M48 immutable artifacts
  and 522-case runtime evidence; add a production symbol-absence gate; rerun
  direct harness, trace, state, M43, M46 immutability, and M48 atomicity.
- CMake change: remove exactly `i286c/i286c_0f.c` from `VAEG_CORE_SOURCES`.
- Risk: medium-high because this is a whole translation-unit deletion with
  shared outgoing references, although the dependency closure is exact.
- Exact M50 acceptance proof: source, preprocessor, object relocation, linker,
  test, and production binary inventories contain none of the group symbols;
  no final graph/provenance/support row changes; all 522 diagnostics, PE
  transactional rejection, CPU_SHUT payload, M42 harness/trace/state, M43
  signatures, and M46 immutable roots remain exact.

The three groups are independent: the two same-TU functions may be approved
without the CTS file, and the CTS file may be approved while either same-TU
function remains. M50 must keep them as separate deletion-focused commits.

## B. Deferred or rejected groups

- **Selector/descriptor execution cluster:** deferred because 11
  final-reachable native handlers expand `SEGSELECT`, which has a direct source
  edge to `i286c_selector`; the selector reads/address-takes GDTR or LDTRC.
- **MSW runtime ownership:** deferred because `SEGSELECT`, state preflight, and
  `cpuio_of0` read it. The diagnostic policy also depends on the PE mask.
- **GDTR and LDTRC runtime ownership:** deferred because `i286c_selector`
  reads/address-takes them.
- **IDTR, LDTR, TR, and TRC runtime ownership:** deferred despite no active
  instruction reader because the M44 adapter imports and overlays them and
  reset/CPU_SHUT transform them. No separately approved opaque-only adapter
  replacement exists.
- **`I286DTR` and serialized fields:** rejected for deletion. Their exact
  offsets/sizes are part of the accepted 112-byte CPU286 contract.
- **state import/export, reset, CPU_SHUT, `CPU_MSW`, interrupts and
  exceptions:** rejected as active lifecycle, compatibility, I/O, or native
  behavior.
- **M48 diagnostic functions and two REP+0F final entries:** rejected because
  they implement the approved fail-closed production policy.
- **test and static-analysis evidence:** retained until successor M50 checks
  explicitly replace M49 source-presence assertions with deletion proofs.
- **frozen `i286x/` references:** excluded from active work by repository rule;
  they neither justify deletion nor may be edited.

## C. Unresolved candidates

There are zero unresolved inventory rows. The architectural behavior of real
uPD9002/V52 REP+0F remains externally unresolved, but it is not a source
classification gap: the accepted diagnostic implementation is explicitly
`fail_closed_diagnostic_required`.

## Preservation results

### M42 and accepted M48 dispatch artifacts

The original M42 graph, construction provenance, support map, 156-case direct
harness, trace baseline, ABI fixture, and reset/executed-3/CPU_SHUT fixtures
remain byte-identical to the accepted M42 implementation commit. The accepted
post-M48 artifacts regenerate byte-identically:

| Artifact | SHA-256 |
|---|---|
| M42 final graph | `e8c8fded6e1f44ef2d74ace93ba1063d64464b217bdd4f9180bf50e86e19df3d` |
| M42 provenance | `605e9af6761e3069f6c5af0ad9647ae574ab98252f7373969586cfee892cd6ba` |
| M42 support map | `7b856f95d8c3d0356325483d188b9dde2d03a8e6cbe9b7fd0cacd004f1d09d13` |
| M42 156-case harness | `14769ca59bf589921cbcf88dc0c357937f6da36e4fe0b78f9a46dc75b9098f2a` |
| M42 trace baseline | `17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba` |
| M42 state fixtures | `c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb` |
| G41 ABI fixture | `f6995d59a293a4e81fc710400fd447031016322018a1c88c9e752c1eb1889db7` |
| M48 final graph | `fe9df28ad3d51cc55235afc3979ada890e86158b294762c15fa33c20d8a800a6` |
| M48 provenance | `128698af06c4e4e98183e4ec0151b7025f427c4f812f95d9012f41417461027a` |
| M48 support map | `21dd037c3eb11e1674805ad456ef03663f17804affbd7382c8db77291ab25279` |
| M48 transition manifest | `4f3fefe8cbfb20a03364a80a0b917e475d3d545cab8eda6bee8a22c66e2147ee` |

M49 modifies none of these accepted files.

### M43 exact reproduction

The pinned dataset identity remains:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-
1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

The known-gap artifact remains byte-identical, with 40 selectors and 68,626
exact hashes, at SHA-256
`11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1`.

| Profile | Executed | Pass | Semantic failure | Timeout | Crash | Signature index |
|---|---:|---:|---:|---:|---:|---|
| CI | 166,821 | 156,228 | 10,593 | 0 | 0 | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` |
| full empty-prefetch | 1,443,876 | 1,359,547 | 84,329 | 0 | 0 | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |

Both independent `--expect` runs pass; generated JSON and complete failure
sidecars are byte-identical to their accepted baselines.

### M44--M48 behavior

- M44: the 112-byte CPU286 layout, 16-byte UPD9002 section, transactional
  adapter, G41/current matrix, PE-clear opaque residue, malformed-state
  rejection, reset FLAGS f002, and CPU_SHUT FLAGS 0000 remain exact.
- M45: `v30c_step()` remains the sole active primitive; there is no CPU_TYPE
  execution selector or `i286c_step`.
- M46: exactly one constructor builds the six roots once; the element-wise
  immutable snapshots pass; `i286c()`, `v30c()`, `CPU_EXEC`, and `CPU_EXECV30`
  remain absent.
- M48: transition checks and all 522 REP+0F state/memory atomicity cases pass;
  PE-set state rejection is preflighted and atomic; the exact diagnostic
  message and PE-clear state acceptance remain.

The state matrix reports all four booleans true. Current payload identities
remain reset CPU286
`6cbd3314a586ccf9c5f1d11a17b8836f76f1deaf10814de8b61ebdf96a792f89`,
executed-3 CPU286
`45d06e424fd332027817ca66db7ed0298e87695cf0cfed5582b7fe1348faa7fe`,
CPU_SHUT CPU286
`7636def12b5f25c39540a1367166394a8308064cce74a15860c62a487a946ff7`,
and UPD9002
`374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb`
for all three scenarios. Sizes remain 112 and 16 bytes.

## Toolchains and production isolation

| Gate | Result |
|---|---|
| GCC `linux-ci-gcc` | fresh configure/build; CTest 34/34 |
| Clang `linux-ci-clang` | fresh configure/build; CTest 34/34 |
| GCC ASan/UBSan `linux-ci-asan` | fresh configure/build; CTest 34/34; focused halt-on-error 5/5 |
| GCC `linux-release` | fresh tests-disabled build; selftest and ROM-less smoke pass |
| MinGW-w64 `mingw-cross` | fresh tests-disabled build; Wine selftest and ROM-less smoke pass |
| hosted Linux/Windows/macOS/standalone | [build 29659803344](https://github.com/nakatamaho/vaeg/actions/runs/29659803344): 7/7 successful at `a4cde3140bf00d9beb6c471c14833a2d10084725` |
| repository invariants | encoding 0, EOL 0, case 0, diff/frozen/binary checks pass; unreferenced report remains the accepted 70 |

Both production caches have `VAEG_ENABLE_TESTS=OFF` and integration tracing
off. Their Ninja graphs contain no M49 audit source. ELF and PE binaries contain
no M49 test CLI, CSV filename, inventory diagnostic, M46 test seam, or M48 test
CLI. The M48 production diagnostic API and message intentionally remain.

Production artifact identities before publication are:

| Artifact | Format | SHA-256 |
|---|---|---|
| Linux `vaeg` | ELF x86-64 PIE | `d0823dfe81adce8584b9e465671c0b2561eeafc68043c94566365075cbeb389e` |
| MinGW `vaeg.exe` | PE32+ GUI x86-64 | `1492daa9f1350915a58236f65b8729d8d3a83e803f909f5b6745fef07991483c` |

## Exact validation commands and statuses

Every listed command exited zero. The deviations section records the two
diagnostic environment attempts that did not.

```text
cmake --preset linux-debug --fresh
cmake --preset linux-release --fresh
cmake --preset linux-ci-gcc --fresh
cmake --preset linux-ci-clang --fresh
cmake --preset linux-asan --fresh
cmake --preset linux-ci-asan --fresh
cmake --preset mingw-cross --fresh

cmake -S . -B /home/maho/vaeg/build/m49-linux-ci-gcc \
  -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVAEG_ENABLE_TESTS=ON
cmake -S . -B /home/maho/vaeg/build/m49-linux-ci-clang \
  -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVAEG_ENABLE_TESTS=ON \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake -S . -B /home/maho/vaeg/build/m49-linux-ci-asan \
  -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_ENABLE_SANITIZERS=ON
cmake -S . -B /home/maho/vaeg/build/m49-linux-release \
  -G Ninja -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=OFF
cmake -S . -B /home/maho/vaeg/build/m49-mingw-cross \
  -G Ninja -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
cmake --build /home/maho/vaeg/build/m49-linux-ci-gcc --parallel 2
cmake --build /home/maho/vaeg/build/m49-linux-ci-clang --parallel 2
cmake --build /home/maho/vaeg/build/m49-linux-ci-asan --parallel 2
cmake --build /home/maho/vaeg/build/m49-linux-release --parallel 2
cmake --build /home/maho/vaeg/build/m49-mingw-cross --parallel 2

ctest --test-dir /home/maho/vaeg/build/m49-linux-ci-gcc --output-on-failure
ctest --test-dir /home/maho/vaeg/build/m49-linux-ci-clang --output-on-failure
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=0 \
  ctest --test-dir /home/maho/vaeg/build/m49-linux-ci-asan --output-on-failure
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir /home/maho/vaeg/build/m49-linux-ci-asan \
  --output-on-failure \
  -R 'vaeg_upd9002_(dispatch_normalization|rep0f_diagnostic_stop|protected_reachability|state_boundary|state_payload_probe)$'

python3 tools/qa/upd9002_protected_reachability.py --root .
python3 tools/qa/upd9002_protected_reachability.py --root . --selftest
python3 tools/qa/upd9002_rep0f_transition.py --root .
python3 tools/qa/upd9002_rep0f_transition.py --root . --selftest
python3 tools/qa/upd9002_native_invariant.py --root .
python3 tools/qa/upd9002_dispatch_normalization.py --root .
/home/maho/vaeg/build/m49-linux-ci-gcc/sdl2/vaeg \
  --upd9002-m46-dispatch-qa
/home/maho/vaeg/build/m49-linux-ci-gcc/sdl2/vaeg \
  --upd9002-m48-rep0f-diagnostic

python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /home/maho/vaeg/build/m49-linux-ci-gcc/sdl2/vaeg \
  --profile ci --shard-timeout 120 \
  --output /home/maho/vaeg/build/m49-ssts/v20_native_ci.json \
  --failure-directory /home/maho/vaeg/build/m49-ssts/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /home/maho/vaeg/build/m49-linux-ci-gcc/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /home/maho/vaeg/build/m49-ssts/v20_native_full.json \
  --failure-directory /home/maho/vaeg/build/m49-ssts/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
cmp /home/maho/vaeg/build/m49-ssts/v20_native_ci.json \
  tests/ssts/baseline/v20_native_ci.json
cmp /home/maho/vaeg/build/m49-ssts/v20_native_full.json \
  tests/ssts/baseline/v20_native_full.json
diff -qr /home/maho/vaeg/build/m49-ssts/v20_native_ci_failures \
  tests/ssts/baseline/v20_native_ci_failures
diff -qr /home/maho/vaeg/build/m49-ssts/v20_native_full_failures \
  tests/ssts/baseline/v20_native_full_failures

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
VAEG_M44_SCENARIO_DIR=/home/maho/vaeg/build/m49-matrix/current-dummy \
  /home/maho/vaeg/build/m49-linux-ci-gcc/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
VAEG_M44_SCENARIO_INPUT_DIR=/tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
VAEG_M44_SCENARIO_OUTPUT_DIR=/home/maho/vaeg/build/m49-matrix/g41-to-m49-dummy \
  /home/maho/vaeg/build/m49-linux-ci-gcc/sdl2/vaeg --selftest
python3 tools/qa/upd9002_state_matrix.py \
  --fixtures tests/upd9002/state_fixtures_m42.txt \
  --g41-generated /tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
  --m44-generated /home/maho/vaeg/build/m49-matrix/current-dummy \
  --g41-to-m44 /home/maho/vaeg/build/m49-matrix/g41-to-m49-dummy \
  --m44-to-g41 /tmp/vaeg-m47-matrix.i6ajc2/m47-to-g41 \
  --g41-sha dc8a72da974f0ea328613e480f1de662c28f4436 \
  --m44-sha 7fd80f7

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /home/maho/vaeg/build/m49-linux-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /home/maho/vaeg/build/m49-linux-release/sdl2/vaeg --smoke
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m49-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 wineboot -u
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m49-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /home/maho/vaeg/build/m49-mingw-cross/sdl2/vaeg.exe --selftest
WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m49-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /home/maho/vaeg/build/m49-mingw-cross/sdl2/vaeg.exe --smoke

nm -a /home/maho/vaeg/build/m49-linux-release/sdl2/vaeg
readelf -rW /home/maho/vaeg/build/m49-linux-release/CMakeFiles/vaeg_core.dir/i286c/i286c_mn.c.o
readelf -rW /home/maho/vaeg/build/m49-linux-release/CMakeFiles/vaeg_core.dir/i286c/i286c_fe.c.o
readelf -rW /home/maho/vaeg/build/m49-linux-release/CMakeFiles/vaeg_core.dir/i286c/i286c_0f.c.o

git diff --exit-code 336227f093d37e3c60bc50333216d66668755cef -- \
  tools/qa/golden/upd9002_final_dispatch_graph.csv \
  tools/qa/golden/upd9002_dispatch_provenance_m42.csv \
  tools/qa/golden/upd9002_support_map_m42.csv \
  tests/upd9002/harness_manifest.csv tests/upd9002/trace_baseline.txt \
  tests/upd9002/state_fixtures_m42.txt tests/upd9002/abi_g41.txt
python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check
git diff --exit-code 339f5f62b3e69611f66f6689be8798f1c675b2cf -- \
  win9x i286x cpuxva/memoryva.x86 hlp romimage
git push -u origin topic/m49-upd9002-protected-mode-reachability
gh run view 29659803344 --json status,conclusion,url,headSha,jobs
```

## Deviations and risks

- Initial preset builds under this worktree's `/tmp` build directory exhausted
  the small `/tmp` filesystem during ASan compilation. Only M49-generated build
  output was removed, and fresh builds were completed under the task-specific
  `/home/maho/vaeg/build/m49-*` directories. No tracked or user asset changed.
- The first release FetchContent attempt could not resolve the network host in
  the restricted sandbox. The same configure was retried with approved network
  access and passed.
- The first state-scenario invocation used the host audio driver, emitted a
  PulseAudio wake error, and did not complete; it was interrupted with exit 130.
  Fresh output directories plus SDL dummy audio/video produced and ingested all
  three scenarios successfully, and the four-way matrix passed.
- One comparison chain named a nonexistent shorthand known-gap path after all
  four required `cmp`/`diff` operations had already passed, so that exploratory
  chain exited 1. The corrected committed path produced the accepted SHA and
  the four comparisons were repeated successfully. `jq` is not installed and
  was not required; committed `rule_count` and `resolved_record_count` supply
  the 40/68,626 values.
- A broad exploratory MinGW strings pattern included the literal build label
  `m49` and therefore matched embedded compiler build paths. The correctly
  scoped audit for M49 test symbols, inventory names, and test CLI strings
  passed; the failed exploratory command did not identify a product seam.
- Linker symbols prove presence, not reachability. The proposed groups rely on
  final-graph, constructor-order, direct-reference, state, lifecycle, test, and
  fail-closed runtime evidence together.
- Real uPD9002/V52 REP+0F semantics remain unknown. M50 must preserve the M48
  diagnostic boundary exactly and may delete only explicitly approved groups.
- Runtime protected fields not included in the three groups retain adapter
  ownership. A later opaque-only representation needs its own state-policy
  approval and compatibility proof.
- The standard V3 boot, bundled VA demo, OS, media, sound, and save/load gate
  remains a human check. This report does not perform or approve it.

## Exact G49 approval choices

The maintainer must make each choice explicitly:

- [ ] Approve or reject `M50-PM-ARPL` exactly as listed, including the retained
  `_reserved` base placeholders and unchanged final patches.
- [ ] Approve or reject `M50-PM-MOV-SEG-EA` exactly as listed, including
  retention of `v30mov_seg_ea`, selector/state ownership, and final patches.
- [ ] Approve or reject `M50-PM-CTS-SYSTEM` exactly as listed, including only
  `i286c_0f.c`, its private closure, six header-only dependencies, declaration,
  three base references, and one CMake edge.
- [ ] Confirm selector/SEGSELECT, all protected runtime/serialized fields,
  state adapter, CPU_MSW I/O, interrupts/exceptions, reset, CPU_SHUT, and M48
  diagnostics remain deferred or retained.
- [ ] Confirm M50 may replace constructor-only base addresses with `_reserved`
  placeholders but may not change any accepted final dispatch row.
- [ ] Confirm M50 may begin only with the exact approved group IDs; M51 remains
  blocked until G50.

## G49 human-review checklist

- [x] Verify exact approved G48 starting SHA and clean starting tree.
- [x] Review all six post-construction roots and complete secondary closure.
- [x] Review direct, address-taking, macro, object-relocation, state, reset,
  CPU_SHUT, diagnostic, I/O, test, and frozen-reference evidence.
- [x] Verify all 522 F2/F3+0F cases stop before state or memory mutation.
- [x] Verify PE-set import rejection is transactional and PE-clear residue
  remains accepted.
- [x] Review all 130 inventory rows, digest, fail-closed checks, and zero
  unresolved rows.
- [x] Review each proposed M50 group member and retained replacement.
- [x] Review every deferred/rejected group and blocking edge.
- [x] Verify M42--M48 local preservation and production isolation.
- [x] Confirm hosted Linux, Windows, macOS, and standalone jobs at pushed
  evidence SHA `a4cde3140bf00d9beb6c471c14833a2d10084725`: build
  29659803344, 7/7 successful.
- [ ] Build from a clean checkout and boot in V3 mode.
- [ ] Run the bundled VA demo.
- [ ] Boot an OS and verify keyboard input and FDD read.
- [ ] If practical, verify FDD write and re-read.
- [ ] Verify Sound Board II.
- [ ] Save/load and continue; verify an MSW.PE state rejects clearly without
  changing the running machine.
- [ ] Reset and boot again.
- [ ] Confirm ordinary startup exposes no M49 test or audit seam.
- [ ] Make the six explicit G49 approval choices above. This report does not
  declare G49 passed.
