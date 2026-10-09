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
# M67 — Consolidate divergences and hardware/authority questions

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


## Predecessor and identifiers

Prerequisite: G66b explicitly approved and the target-correct architectural full profile has zero
applicable failures.

Approved predecessor gate: `G66b`

Approved predecessor SHA: `97f760e8da573888edf089c2875c623895a3c2c9`

Branch: `topic/m67-upd9002-divergence-consolidation`

Commit prefix: `M67:`

Gate: `G67`

## Goal

Publish the final minimal, content-addressed target inventory, approved divergences, and unresolved
hardware questions without production semantic change.

M67 records the current divergence-domain state in the canonical registry:

`tests/ssts/divergence/g67/registry.json`

Compatibility views such as approved-target-divergence, hardware-pending,
evidence-backlog, zero-coverage, fingerprint-diagnostic, and
state-compatibility-exception views are generated from that registry. They are
not separate hand-edited current sources of truth.

M67 must make an identity transition from approved G66b for CPU execution,
target policy, classifications, gap kinds, selected/applicable sets,
comparison contracts, SST corpus, fixtures, and architectural/fingerprint
result identities. Any contradiction requiring a policy or classification
change is a blocker for a later maintainer-approved corrective milestone, not
part of M67.

## Required review

- Verify every `expected_target_divergence` against exact primary target/hardware evidence.
- Verify every remaining `target_support_unverified` hash has exact hardware-pending coverage.
- Verify all `6C-6F` and `0F31/33/39/3B` records preserve the approved
  scope-limited target-absence authority and are never reported as passes.
- Verify `0F28` is applicable and passing.
- Reconcile active/reserved behavior for `6C-6F` with the final handler reachability decision.
- Consolidate BRKFEM/BRKEM, RETEM/CALLN, MD/Z80 mode, and FPO2 questions.
- State explicitly that generic FPO string absence is non-evidence.
- Preserve the historical G43 1,204 OUTS gain as V20 evidence only.

## Source precedence and conflicts

Current source precedence is:

1. exact approved target authority;
2. approved content-addressed target evidence;
3. approved monitor authority within its documented scope;
4. approved corpus compatibility evidence;
5. hardware-pending question;
6. hypothesis.

Monitor evidence must not be used outside its documented scope. Existing
approved historical evidence remains immutable. Every discovered current
divergence-domain source record must appear once in
`tests/ssts/divergence/g67/source_migration.json`. Current-source conflicts are
recorded in `tests/ssts/divergence/g67/conflicts.json`; any conflict that
affects current semantics or gate behavior blocks G67 presentation.

## Protected current domains

- The M65j backlog remains 19 target-support-unverified selector groups,
  5,908 hashes, digest
  `240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5`,
  non-applicable, unimplemented, not officially executed, not claimed passing,
  and nonblocking under its recorded amendment.
- `6C-6F` remains documented target absence outside the blocking denominator;
  production cleanup status and reserved-byte questions are separate records.
- `66` and `67` remain upstream-nonblocking with monitor-disassembler target
  absence only within monitor scope; complete silicon support remains
  underdetermined and must not be recorded as documented silicon absence.
- BRKEM (`0F FF imm8`) and BRKFEM (`0F FE imm8`) remain zero-coverage or
  evidence-backlog records with no implementation or passing claim.
- Fingerprint-only residue remains diagnostic and non-architectural-blocking.
- The exact G66b state migration bridge
  `CPU286 v0 + UPD9002 v0 -> UPD9CPU v1 + UPD9002 v0` remains a
  state-compatibility exception only, with broader CPU286 compatibility
  prohibited.

## Gate G67

- Zero target-correct applicable failures.
- No implementation-missing or unclassified record.
- Every target-absent and divergence entry is exact/evidence-backed.
- Unresolved FPO2/BRK/mode questions are minimal and explicit.
- No active I286/i286c identity remains and historical artifacts are unchanged.
- Report does not claim complete silicon validation.
- Generated compatibility views match the canonical registry.
- No CPU semantic, target-policy, classification, gap-kind, selected-set,
  applicable-set, fixture, corpus, or comparison-contract change occurs.

Write `docs/agents/reports/m67_upd9002_divergence_consolidation.md` and stop.
