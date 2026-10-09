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
# M58 — Add the hash-level SST ratchet, dual profiles, and gap taxonomy

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and all reports from prerequisite gates.
5. Run `git status --short`; the tracked worktree must be clean.
6. Record the exact starting branch and SHA.
7. Resolve and verify the exact approved predecessor gate SHA. Do not infer it
   from the current worktree.
8. Work on this milestone only and stop at its gate.

All newly authored source, comments, identifiers, commit messages, test names,
and repository documentation must be in English.

## Predecessor and identifiers

Prerequisite: G57 explicitly approved at
`72322d5c9b8e40e4a988312aebe163a8190e2aa5`.

Branch: `topic/m58-upd9002-ssts-ratchet`

Commit prefix: `M58:`

Gate: `G58`

## Goal

Add the immutable hash-level comparison epoch used by every later semantic
milestone. Add no core semantic change.

## Required milestone-name tooling audit

Before creating or relying on the first lettered milestone, audit scripts and
documentation tooling for assumptions equivalent to `M[0-9]+` or `G[0-9]+`,
including:

- ROADMAP and task/report discovery;
- branch-name and commit-prefix checks;
- documentation validators;
- gate parsers;
- artifact path generation;
- CI workflow filters.

Canonical lettered forms are `M60a`, `m60a`, `G60a`,
`topic/m60a-*`, and `M60a:`. If tooling cannot support them
unambiguously, prepare and obtain approval for a contiguous integer
renumbering before any lettered semantic task starts. Do not mix schemes.

## Immutable M43 evidence

Do not modify or re-schema existing M43 summaries or failure sidecars. Add
verified references or digests under `tests/ssts/epochs/g43/`.

## Profiles and artifacts

Create separate artifacts for:

```text
tests/ssts/scoreboard/g58_architectural_ci.json
tests/ssts/scoreboard/g58_architectural_ci_failures/*.json.gz
tests/ssts/scoreboard/g58_architectural_full.json
tests/ssts/scoreboard/g58_architectural_full_failures/*.json.gz
tests/ssts/scoreboard/g58_fingerprint_full.json
tests/ssts/scoreboard/g58_fingerprint_full_failures/*.json.gz
tests/ssts/transitions/
```

Do not combine architectural CI, architectural full, and fingerprint results
in one ambiguous summary.

Each summary must record:

- `epoch_gate` = `G58`;
- `evaluated_sha`, referring to the M58 implementation commit and stored in a
  later evidence commit;
- approved G57 predecessor gate and SHA;
- `profile`, `scope`, and `blocking`;
- comparison-contract ID and SHA-256;
- dataset ID;
- selected hash-set digest;
- applicable hash-set digest;
- separate immutable M43 CI/full summary and failure-index digests.

Never put the containing commit's own SHA in the summary.

## Scoreboard schema

Emit per structural form at least:

```json
{
  "opcode": "f7",
  "subform": "7",
  "classification": "applicable",
  "selected": 5000,
  "executed": 5000,
  "pass": 1214,
  "fail": 3786,
  "termination_classes": {},
  "mismatch_classes": {}
}
```

The exact schema must be versioned, canonicalized, deterministically ordered,
and covered by self-tests.

## Hash-level ratchet

Implement comparison against an explicitly supplied approved predecessor SHA.
A current-worktree self-comparison is invalid.

Enforce:

- no newly failing hash;
- no per-form pass-count decrease;
- zero timeout/crash;
- identical dataset and comparison-contract identities;
- deterministic changed-failure shards;
- exact classification-transition governance.

The transition artifact identifies profile and scope and includes
selected/applicable before/after hash-set digests.

## Gap taxonomy

Annotate every existing `known_target_gap` entry with exactly one:

- `documented_silicon_absent`
- `implementation_missing`
- `target_support_unverified`

Do not change existing M43 top-level classifications or resolved hash sets in
M58.

Every `target_support_unverified` entry must have exact matching coverage in
`hardware_pending.json`, with identical selector, resolved hashes, count, and
sorted-hash digest.

Add schemas and negative tests proving:

- unknown taxonomy values fail closed;
- missing matching hardware-pending coverage fails;
- open-ended selectors fail;
- overlapping ownership fails;
- outcome-based partitioning cannot be represented as an approved structural
  split.

## Divergence registries

Add empty or evidence-preserving, schema-valid:

- `approved_target_divergences.json`
- `hardware_pending.json`

`hardware_pending.json` is orthogonal and cannot alter top-level
classification or denominator.

## CI/full enforcement

- Hosted CI runs the deterministic architectural CI profile when the verified
  corpus is available.
- Every gate requires both verified CI and full profiles without skip.
- The 84,329 and 10,593 immutable M43/G51 populations must already have been
  reproduced at the approved predecessor and preserved through G57 before the
  new epoch is recorded.

## Gate G58

- No core semantic source changed.
- M43 artifacts are byte-identical.
- New architectural CI/full and fingerprint artifacts are reproducible.
- Ratchet negative tests cover mutable-golden self-approval, identity
  mismatch, new failure, signature change, invalid classification transition,
  and missing hardware-pending coverage.
- G58 report records implementation commit, evidence commit, and approved
  predecessor SHA.

Write `docs/agents/reports/m58_upd9002_ssts_ratchet.md` and stop.
