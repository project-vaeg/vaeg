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
# M64 — Correct DIV/IDIV and requested monitor-authorized 0F support

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Approved predecessor and identifiers

Prerequisite: G62 approved exactly at
`70b8e94e96aef4cb79eed72c7813c4148c5c0dd8`.

Branch: `topic/m64-upd9002-div-idiv`

Commit prefix: `M64:`

Gate: `G64`

Report: `docs/agents/reports/m64_upd9002_div_idiv.md`

## Goal

Correct the complete executed `F6/F7 /6-/7` DIV/IDIV population and complete
the requested PC-88VA monitor-authorized, SST-covered 0F instruction support.
This is a maintainer-approved expansion of the original M64 DIV/IDIV scope.

Execute these phases in order:

1. `F6/F7 /6-/7` DIV and IDIV.
2. `0F20/0F22/0F26` ADD4S, SUB4S, and CMP4S.
3. The complete TEST1, CLR1, SET1, and NOT1 expanded opcode families.
4. `0FFF imm8` BRKEM authority and SST-coverage checkpoint.

Use separate pre-edit audits, content-addressed selectors, focused tests,
independently reviewable semantic commits, and phase checkpoints. The final
candidate profiles use one final worker and one final target-policy epoch.

## Monitor dispatch contract

The monitor records are three-byte `(mask, value, group)` table entries, not
instruction byte sequences. The requested records are:

```text
ff 20 00 and ff 20 01  ADD4S
ff 22 00 and ff 22 01  SUB4S
ff 26 00 and ff 26 01  CMP4S
f6 10 03               TEST1
f6 12 03               CLR1
f6 14 03               SET1
f6 16 03               NOT1
ff ff 04               BRKEM
```

The exact instruction encodings include the leading `0F`. Mask `f6` leaves
bits 0 and 3 free and expands to:

```text
TEST1: 0F10 0F11 0F18 0F19
CLR1:  0F12 0F13 0F1A 0F1B
SET1:  0F14 0F15 0F1C 0F1D
NOT1:  0F16 0F17 0F1E 0F1F
```

Derive byte/word and CL/immediate ownership from ROM links, metadata, and
executed SST cases; do not assign forms from numeric pattern alone. Trace the
duplicated group-00/group-01 ADD4S/SUB4S/CMP4S records independently.

`0F28 ROL4` and `0F2A ROR4` are protected G62 behavior. `0FFE imm8 BRKFEM`
is present in the same authority table but is outside M64.

## Target-policy transitions

For each exact requested monitor-authorized population, preserve an existing
`applicable` classification or activate the complete structural set from
`known_target_gap/implementation_missing`,
`known_target_gap/documented_silicon_absent`, or `upstream_nonblocking` only
as expressly authorized by the maintainer. Every newly applicable hash must
execute and pass in the same phase.

Selectors are structural and fixed before candidate outcomes. Partial or
outcome-derived activation is forbidden. Selected sets and comparison
contracts remain unchanged. No classification or gap-kind change outside the
requested forms is permitted.

## DIV/IDIV contract

Derive and implement quotient/remainder ownership, signed rounding and
remainder sign, zero divisor, exact overflow boundaries, minimum signed
dividend divided by `-1`, defined successful FLAGS, pre-event FLAGS,
failure-path register preservation, type-0 decision, and represented prefix
behavior. Exclude zero and overflow before host division and avoid host
undefined behavior. Event-frame construction remains the approved G60d
primitive.

## 0F semantic contract

Derive ADD4S, SUB4S, and CMP4S independently, including packed-digit order,
carry/borrow, comparison side effects, register updates, DF, prefixes, and
memory boundaries.

Derive TEST1, CLR1, SET1, and NOT1 independently across every expanded opcode,
operand width, CL/immediate bit source, register/memory form, index boundary,
FLAGS rule, alias, and memory boundary.

The approved SST v20 metadata defines `0FFF`, but the approved dataset manifest
contains no `0FFF.json.gz` shard. Consequently the live selected, applicable,
and executed BRKEM populations are all zero. This is an accepted
maintainer-supplied clarification, not a fixture defect.

Phase D must bind that zero coverage to the exact dataset and metadata
identities and produce a deterministic authority/coverage checkpoint. It must
not fabricate cases, alter the selected set, activate a policy population, or
make a cosmetic semantic edit. Record
`compatibility_scope = no_v20_sst_cases`,
`sst_contract_status = not_yet_present`, and
`silicon_mode_identity = underdetermined`. Executable BRKEM semantics remain
pending until an approved content-addressed corpus supplies expected and
actual compatibility cases.

## Prohibited scope

Do not alter or implement FF `/7`, F7 `/2`, BOUND range behavior, event-frame
construction, IRET, BRKFEM, RETEM, CALLN, FPO/FPU, 6C-6F, 66/67, another 0F
family, cycles, prefetch, bus timing, fixtures, or comparison contracts.
Do not reopen ROL4/ROR4 or any completed G62 family.

## Commit and evidence structure

Use a documentation-only first commit, a shared audit/policy-tooling commit,
separate semantic commits for phases with executable cases (and additional
commits for independent causes), then one final evidence-only commit. Phase D
is an evidence-only checkpoint while the approved BRKEM population is zero.
The last
worker-changing commit is `evaluated_sha`; all final profiles and evidence
must bind to it.

Generate deterministic phase checkpoints, G64 target policy when required,
architectural CI/full and fingerprint-full scoreboards, failure shards,
G62-to-G64 transitions, a fresh full ranking, result manifest, and the final
report. Generate the complete evidence family twice in the pinned environment
and prove byte identity.

## Gate G64

All four DIV/IDIV forms and every SST-covered requested 0F population are
applicable, executed, and green. BRKEM has an exact zero-coverage checkpoint
and no unsupported semantic claim. Every newly applicable hash passes; newly
failing, timeout, and crash sets are empty. ROL4/ROR4 and all protected G62
results remain exact. Dataset, contracts, selected sets, and unrelated policy
ownership are unchanged. A complete fresh ranking is published for M65.

Write `docs/agents/reports/m64_upd9002_div_idiv.md` and stop.
