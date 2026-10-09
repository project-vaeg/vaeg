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
# M60c — Audit the main opcode dispatch and FPO2/66-67 target status

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

Prerequisite: G60b explicitly approved.

Branch: `topic/m60c-upd9002-fpo2-audit`

Commit prefix: `M60c:`

Gate: `G60c`

## Goal

Determine, without implementing FPU semantics, how primary opcodes 66/67 are classified in SST and
how the monitor ROM main dispatch maps them. Remove unsupported absence claims; do not guess FPO2.

## Current-classification audit

For every selected record with primary opcode 66 or 67, report:

- upstream metadata mnemonic/status/architecture and whether it labels FPO2;
- top-level classification;
- selected, executed, pass, and failure counts;
- support-map/dispatch classification;
- exact resolved hashes and digests;
- `gap_kind` if and only if top-level classification is `known_target_gap`.

Absence from the failure distribution is not evidence of passing. Distinguish explicitly among
`applicable`, `known_target_gap`, `unsupported_fixture`, and `upstream_nonblocking`.

## ROM main-dispatch audit

Using the same content-addressed ROM authority:

- locate and bound the main opcode group table near `0x66900`;
- record raw records including the observed forms such as `fe f6 00`, `ff ff 01`, `ff 8f 02`,
  `fc 80 05`, `fc d0 07`, and `e7 26 08` only after verifying their exact offsets;
- determine the record format instead of assuming it from the `0F` table;
- verify that `e7 26` represents segment overrides before using it as a decoder check;
- trace 66 and 67 through group tables/handlers;
- record the D8-DF FPU record region near `0x66B3B` and the individual 8087 mnemonics it references.

The hypothesis that 66/67 are FPO2 is not a conclusion until the dispatch path proves it.

## Explicit non-evidence rule

The absence of generic strings `ESC`, `FPO1`, or `FPO2` is expected when the monitor uses individual
names such as FADD/FMUL. It cannot support `documented_silicon_absent`.

## Permitted evidence-taxonomy result

M60c makes no production semantic or top-level classification change.

For records already classified `known_target_gap`, it may set the exact gap kind according to
positive evidence:

- `implementation_missing` if target support is proven;
- `documented_silicon_absent` only if target absence is positively proven;
- `target_support_unverified` if support remains unresolved.

A change to `target_support_unverified` requires exact matching `hardware_pending.json` coverage.
If records are `upstream_nonblocking` because they are FPU metadata, preserve that top-level class
and record the target question in the orthogonal authority-pending registry.

Do not promote any hash to `applicable` in M60c. If target support is proven, propose a separately
gated implementation/profile-policy task for M65 review.

## Validation

- Reproduce G60b target-policy and all blocking profiles exactly.
- Add deterministic table decoders and fail-closed tests for wrong boundaries, overlapping masks,
  unresolved group links, unsupported generic-string inference, and taxonomy/registry mismatch.
- Generate the audit twice with byte-identical results in the recorded environment.
- Prove no change under `cpu/upd9002/` and no selected/applicable-set change.

## Gate G60c

- Every 66/67 selected hash has an explicit classification/execution account.
- Main-dispatch evidence is content-addressed and the support conclusion is labelled proven or
  underdetermined.
- No false absence claim rests on string-pool nomenclature.
- Any gap-kind correction is exact and governance-complete.
- No production semantics or top-level classification changes.

Write `docs/agents/reports/m60c_upd9002_fpo2_main_dispatch_audit.md` and stop.
