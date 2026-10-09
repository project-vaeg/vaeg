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
# ADR-0013: Determine uPD9002 REP+0F semantics before consolidation

## Status

Accepted as a fail-closed implementation policy at G47. The approved M47
evidence SHA is `c683fe502647918d8ded2b4c2243da1b787c9d0a`. This decision
authorizes M48 safety behavior; it does not determine the architectural
uPD9002/V52 semantics of REP-prefixed 0F.

This ADR amends ADR-0012. Its ownership and state-boundary decisions remain in
force, while its original M47--M49 schedule and protected-cluster
non-reachability premise are superseded by M47--M51.

## Context

The accepted M47 pre-implementation audit contradicted the original deletion
plan. The active M42 graph contains both of these edges:

```text
v30op_repe[0x0f]  -> i286c_cts
v30op_repne[0x0f] -> i286c_cts
```

`i286c_cts` can call `cts0_table`, `cts1_table`, and `LOADALL286`. In the
accepted emulator, F2/F3 0F 01 F0 reaches the legacy LMSW handler and AX=0001
sets MSW_PE. A valid imported CPU286 image can also restore MSW_PE and
descriptor residue, after which SEGSELECT callers can enter `i286c_selector`.
This is active, guest-visible legacy behavior. Activity does not establish that
the behavior is architecturally correct for uPD9002/V52.

## Evidence

The pinned identities, editions, digests, and page/section citations are in
`docs/agents/research/m47_upd9002_rep0f_documents.json`.

* The October 1986 V20/V30 User's Manual defines the family prefix and
  interrupt rules but does not define uPD9002 REP+0F.
* NEC U11301EJ5V0UMJ1 defines REP-family targets and 0F Group 3 separately. It
  neither describes uPD9002 nor resolves the combined encoding.
* The V30MX manual does not supply a uPD9002/PC-88VA rule.
* The revised January 1988 PC-88VA Technical Manual is identified by NCID
  BA48885862, but its contents were not available to inspect and hash.
* The complete M43-pinned V20 corpus has zero decoded F2/F3+0F records. The 105
  adjacent F2/F3,0F byte pairs occur only inside operands, immediates, or
  displacements. V20 therefore supplies no applicable expected result here and
  would not alone prove uPD9002 behavior even if it did.
* The standalone eight-case PC-88VA probe is deterministic and recoverable by
  design, but no real-hardware result has yet been supplied.

The complete 768-row evidence matrix is
`tools/qa/golden/upd9002_rep0f_instruction_matrix_m47.csv`. Candidate outcomes
remain open:

1. A — ignore F2/F3 and execute the normal V30 0F instruction.
2. B — give F2/F3+0F a uPD9002-specific meaning.
3. C — treat the combination as reserved or undefined.
4. D — select behavior by second byte or another condition.

## State-policy alternatives

No state policy is selected. The G47 decision must address every protected
field, not only MSW_PE.

| Policy | Data-loss risk | Silent-change risk | Compatibility and versioning | G41/M44 matrix | CPU_SHUT | Complexity |
|---|---|---|---|---|---|---|
| Reject protected residue | No payload is silently changed, but affected saves become unusable | Low after an explicit error; high operational disruption | Breaks accepted import and requires an explicit compatibility rule, normally a new state version or capability marker | Three accepted matrix scenarios remain valid because their protected fields are zero; noncanonical valid-import coverage changes | Must retain its exact accepted transform and FLAGS 0000 | Medium |
| Canonicalize to native state | High: nonzero MSW/descriptors are discarded | High unless migration is prominently reported | Breaks byte-exact G41/M44 round trips; requires versioned migration semantics | Canonical fixtures can remain, but residue round trips change | Must be exempt and remain exact | Medium |
| Translate through an adapter | Depends on whether every protected concept has a native representation | Medium; semantic approximation can be invisible | May preserve older inputs but requires a versioned, specified mapping and new fixtures | Requires a new cross-version transition matrix | Adapter must not touch the CPU_SHUT anomaly | High |
| Preserve as opaque only, exclude from runtime | Low for serialized bytes; runtime meaning is intentionally dropped | Medium unless resume semantics are explicitly disclosed | Can retain payload bytes, but runtime/export overlay ownership and likely version policy must change | Existing zero-residue matrix can remain exact; residue-specific tests are required | Opaque ownership must not change the accepted transform | Medium-high |

The committed reset, executed-3, and CPU_SHUT fixtures contain no protected
residue. The M44 valid noncanonical import test deliberately supplies nonzero
GDTR and TRC bytes and is accepted today. No maintainer-provided user save
corpus was available, so real-world occurrence frequency is unresolved.

## Prospective transition boundary

`tools/qa/golden/upd9002_rep0f_transition_manifest_m47.json` names all 512
F2/F3+0F matrix cases. Outcome A would replace exactly two final-root rows, two
construction-provenance rows, and two support-map rows. Outcomes B--D cannot
name replacement targets until their semantics are known; the manifest lists
all unresolved rows rather than omitting them. All outcomes affect zero M43
record hashes and zero M43 failure signatures because the pinned corpus has no
decoded REP+0F records.

The three accepted state fixtures and CPU_SHUT FLAGS 0000 must remain exact.
Any newly rejected, canonicalized, translated, or opaque-only state is governed
only by the separately approved G47 state policy. M42 and M43 historical
artifacts are never overwritten to hide a transition.

## Decision

The documentary, corpus, and pending-hardware evidence remains insufficient to
select architectural outcome A, B, C, or D. G47 therefore selected a separate
fail-closed emulator policy:

* every F2/F3-prefixed 0F form raises one emulator-level diagnostic stop;
* the complete pre-instruction runtime state is restored, the following 0F
  second byte is not executed, DMA and VA coprocessor scheduling do not run,
  and the latch prevents a later scheduler call from resuming the instruction;
* this stop is deliberately not an emulated interrupt, exception, reserved
  instruction, or claim about real uPD9002/V52 behavior;
* imported CPU286 payloads with `MSW.PE` set are rejected during preflight,
  before any live section is committed;
* descriptor-table and selector-cache residue remains accepted and opaque when
  `MSW.PE` is clear, because that residue alone cannot select the legacy
  protected-mode execution path;
* CPU286 layout, version, opaque-byte ownership, reset, and CPU_SHUT remain
  unchanged.

The exact transition is frozen by
`tools/qa/golden/upd9002_rep0f_transition_manifest_m48.json`. The two final-root
targets, two provenance rows, and two support-map rows change. The final graph
also drops the 18-row derived CTS0/CTS1 reachability closure; M48 does not delete
the corresponding implementation. The M43 transition contains no record hash,
classification, or failure-signature change because the pinned dataset has no
decoded F2/F3+0F record.

## G47 resolution

The maintainer explicitly approved:

1. the fail-closed diagnostic rule for all 512 F2/F3+0F forms;
2. transactional rejection of state that requires legacy protected execution,
   implemented by rejecting `MSW.PE` while retaining dormant opaque residue;
3. replacement of exactly the two active REP+0F root targets;
4. retention of M42/M43 history and a separate exact M48 transition baseline;
5. sufficient evidence to implement this safety policy without claiming an
   architectural answer.

The approval does not authorize protected-mode source deletion.

## Consequences

M49 inventories actual post-M48 reachability. M50 may delete only groups
explicitly approved at G49, and M51 performs renames. Authoritative
uPD9002/V52 documentation or PC-88VA hardware results may later replace this
emulator policy through a separately approved correctness milestone and an
explicit baseline transition.
