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
# M62 — Correct the consolidated uPD9002 semantic bundle

## Identity

Prerequisite: G61 approved at
`829f314bb0d363ec5b6e9aa738e948b1a3adb365`.

Branch: `topic/m62-upd9002-semantics-bundle`

Commit prefix: `M62:`

Gate: `G62`

Report: `docs/agents/reports/m62_upd9002_semantics_bundle.md`

M62 is a maintainer-approved one-time exception to the usual
one-family-per-gate rule. It consolidates and supersedes the unexecuted
prospective M62a, M62b1, M62b2, M62c, and M63 tasks. Each phase remains
independently reviewable through separate semantic commits and
content-addressed evidence. M64 and later identifiers are unchanged.

## Mandatory preparation

Read `AGENTS.md`, ROADMAP, CONVENTIONS, the semantics migration
specification, this task, all reports from G59 through G61, and every
referenced authority, policy, evidence, scoreboard, transition, ranking, and
schema. Run milestone discovery. Create a clean dedicated worktree at the
exact approved G61 SHA without modifying another worktree.

Record the worktree and initial repository state; Git, compiler, CMake,
Python, gzip, and zlib versions; corpus identity; comparison-contract
identities; target-policy identity; and G60b ROM-authority identity.

Before production editing, reproduce all G61 profile counts, pass/failure
sets, signature indexes, selected/applicable sets, classifications, taxonomy,
registries, policy, and protected artifact identities. Any drift is a hard
stop.

All newly authored code, identifiers, comments, tests, schemas, commit
messages, and repository documentation must be English.

## Fixed phase order

Execute every phase in this order:

1. Phase A — D4 AAM.
2. Phase B — 0F2A ROR4.
3. Phase C — 0F28 ROL4 activation and implementation.
4. Phase D — 27/2F/37/3F BCD and ASCII adjust.
5. Phase E — C0/C1/D2/D3 shift subforms `/4` through `/7`.

For each phase, complete the pre-edit audit, fix structural selectors before
observing candidate outcomes, add focused tests, create a separate semantic
commit, replay the complete owned population, emit a deterministic checkpoint,
and prove earlier phases remain green. Do not squash semantic commits. If any
phase cannot be completed safely, M62 is incomplete.

Each machine-readable case row must keep expected and actual state side by
side and include its hash, structure, complete bytes, prefixes,
classification, selection/execution state, initial and final state, FLAGS,
termination, represented RAM, partition, mismatches, notes, and a conclusion
label of `proven`, `hypothesis`, or `underdetermined`. Structural selectors
must not depend on outcomes.

## Phase A — D4 AAM

Audit all 5,000 applicable D4 cases, including immediate values 0, 1, 2, 9,
10, 11, 16, and 255. The G61 cross-check is 197 pass and 4,803 fail with
failure digest
`e0ffd2df098de38bc99cc0fc455b351a266baff2d74bddeb3e2f1fc0e857b731`.
Identify all immediate-zero cases; preserve their SST-observed normal
termination.

Derive quotient, remainder, AH/AL placement, immediate consumption, IP,
defined FLAGS, termination, and unrelated-register preservation from the
complete population. Do not assume base ten or Intel divide-error behavior.
Require D4 5,000/5,000, no new failures, and exact ownership of improvements.

D5 remains outside scope and must remain architectural 5,000/5,000.

## Phase B — 0F2A ROR4

Audit all 5,000 applicable and executed 0F2A records. The G61 cross-check is
308 pass and 4,692 fail with failure digest
`4bbe0bf9537bbae74bb0c7d9c2e94bfa82a6ac0f3283945e6841de36c48bf3a3`.
Partition register/memory forms, ModR/M, source and destination bytes, AL,
segments, displacement, prefixes, both address-wrap domains, FLAGS, and
termination.

Derive ROR4 independently. Do not use unimplemented 0F28 as an oracle and do
not modify another 0F family. Require 0F2A 5,000/5,000 while 0F28 remains
unchanged at this checkpoint.

## Phase C — 0F28 ROL4 and target-policy transition

Verify the live pre-transition state is exactly
`known_target_gap/implementation_missing`, selected-full count 5,000,
official executed count zero, selector digest
`d4978211d0588687f1e04486b42209460c585a89126367df76742a749463ae01`,
and resolved-hash digest
`1d01e7d8ec9cd05fa804acc5c9cb7e30cc451f8eea710847826b15b0622ef247`.
G60b ROM authority proves `0F28 = ROL4` and remains protected.

Implement the complete independently derived register/memory, ModR/M, AL,
destination, addressing, prefix, FLAGS, length, and termination contract.
Transition the complete structural set in the same semantic-and-policy commit:

```text
known_target_gap / implementation_missing -> applicable
```

All 5,000 hashes must transition, execute, and pass. No other classification
or gap kind may change. Full selected remains 1,562,502 and full applicable
becomes exactly 1,443,594. Derive CI applicable arithmetic from the live
selector. Create a content-addressed G62 target-policy epoch.

## Phase D — BCD and ASCII adjust

Audit complete applicable populations for `27 DAA`, `2F DAS`, `37 AAA`, and
`3F AAS`; do not infer passing from ranking omission. The G61 cross-check for
3F is 284 pass and 4,716 fail. Partition initial AL/AH/AF/CF, both nibbles,
adjustment branch, expected AL/AH/AF/CF, defined ZF/SF/PF and other observed
FLAGS, and termination.

Implement only the observed result, branch, FLAGS, register, and termination
rules. D4/D5 and packed-BCD rotates are not semantic oracles. If the four
forms have independent causes, use additional separate M62 semantic commits.
The exact union `B` of G61 architectural failures for 27/2F/37/3F must pass,
and every earlier pass must remain unchanged.

## Phase E — shifts

Audit C0/C1/D2/D3 subforms `/4` through `/7`. The expected current failure
cross-check is C0=3,084, C1=6,885, D2=2,731, D3=6,439, total 19,139, digest
`85c431ba4d46a285aa6c352192ba1b583ac3aadd739b6154e79c8d96f0b06bce`.

Partition opcode, subform, width, register/memory destination, destination,
count source, raw and observed count, count 0/1/2/width-1/width/width+1/31/32/
33/255, sign, initial CF, result, CF/OF/AF/ZF/SF/PF, termination, and RAM.
Audit `/6` explicitly. Raw-count, masked-count, both-model, and neither-model
observations coexist; an unconditional count rule is forbidden.

Treat count, destination, individual FLAGS domains, count-zero behavior, and
memory behavior as potentially independent causes and split commits where
needed. Require zero architectural failures for all authorized shift
subforms.

The exact 40,000 C0/C1/D2/D3 rotate `/0` through `/3` hashes remain
architecturally green with unchanged results and signatures. Their approved
digest is
`638a5692ff2b2b98dc37ac0ee7d23458e1ca5185054b3937985144599a3b3b83`.

## Scope and protected behavior

Do not change FF `/7`, F7 `/2`, DIV/IDIV, BOUND range behavior,
PUSHF/POPF/SAHF/LAHF, interrupt entry, IRET, C6/C7, 6C-6F, 66/67, FPO/FPU,
BRKFEM/BRKEM, D5, RETEM, cycles, prefetch, bus timing, fixtures, comparison
contracts, selected sets, registries, or target policy outside exact 0F28
activation.

Protect at least:

```text
9C  4,999/1       9D/9E/9F  5,000/0
CC/CD/CE/CF       5,000/0
C6/C7             5,000/0
62 BOUND          3,756/1,244
D5                5,000/0
```

Also protect the empty G60d frame residual, the 214-hash M60a divide
dependency, the 2,208-hash G61 C6/C7 improvement set, all historical
authority and erratum evidence, and all protected artifacts.

## Ratchet and arithmetic

Compare only against G61 SHA
`829f314bb0d363ec5b6e9aa738e948b1a3adb365`. Dataset, contracts, selected
sets, and all unrelated classification/taxonomy/registry data remain exact.
The applicable-set increase is exactly the 5,000 0F28 hashes.

Define:

```text
A = G61 D4 architectural failure set
R = G61 0F2A architectural failure set
B = G61 27/2F/37/3F architectural failure union
S = G61 C0/C1/D2/D3 /4-/7 architectural failure union
L = exact newly applicable 0F28 set
```

Previously applicable newly passing hashes must equal `A union R union B
union S`; newly applicable passing hashes must equal `L`. `L` is denominator
activation, not failure reduction. Require no newly failing hash, zero timeout
and crash, no unrelated form decrease, and complete enumeration of changed
signatures.

The final full arithmetic must reconcile:

```text
fail = 53,964 - 4,803 - 4,692 - |B| - 19,139
     = 25,330 - |B|

pass = 1,384,630 + 4,803 + 4,692 + |B| + 19,139 + 5,000
     = 1,418,264 + |B|

applicable = 1,443,594
```

## Artifacts, tests, and execution

Create versioned deterministic phase checkpoints, complete per-family case
tables, representative evidence, schemas, a G62 evidence manifest, target
policy, architectural CI/full and fingerprint-full scoreboards and shards,
G61-to-G62 transitions, result manifest, and a fresh complete/full top-30 and
family-aggregated ranking.

The final verifier must reject missing/reordered/squashed phases, identity
drift, incorrect 0F28 ownership, incomplete case or count coverage, outcome
selectors, unsupported formulas, cross-family analogies, protected-family
regressions, scope expansion, comparison-mask or fixture changes,
nondeterministic serialization/compression, ranking mismatch, and an evidence
commit containing implementation changes. Tests must prove rejection.

Generate the entire evidence family twice in the pinned environment and
require byte identity. Artifacts record the last worker-changing semantic
commit as `evaluated_sha`, never the containing evidence commit.

Against the exact final evaluated commit and G62 policy, run without skip:

- standard native build and tests;
- all focused phase tests and M62 positive/negative selftests;
- architectural CI and full profiles;
- fingerprint full;
- M58 through M61 validators;
- milestone discovery;
- encoding, EOL, path-case, documentation, and diff checks;
- repository-required hosted CI.

Record commands, exit codes, elapsed times, worker and all governing
identities, phase commits and results, profile counts and digests, transitions,
rankings, artifact-tree digest, regeneration proof, and hosted CI.

## Commit and gate discipline

Use a documentation-only consolidation commit, a shared audit/tooling commit,
at least one separate semantic commit for every phase, additional commits for
independent Phase D/E causes, optional justified validator-only commits, and
one final evidence-only commit. The last worker-changing commit is the final
semantic/evaluated SHA. Push that chain before final evidence generation and
never amend it afterward.

Update `docs/modernization/bug-fixes.md` for every demonstrated corrected
guest-visible defect, with exact fixing commit links.

Write `docs/agents/reports/m62_upd9002_semantics_bundle.md`, push
`topic/m62-upd9002-semantics-bundle`, and stop at unapproved candidate G62.
Do not merge, declare G62 passed, tag it, or begin M64.
