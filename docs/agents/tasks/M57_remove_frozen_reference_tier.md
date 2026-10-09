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
# M57 — Remove the frozen Win9x/i286x reference tier

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and all reports from prerequisite gates.
5. Run `git status --short`; the tracked worktree must be clean.
6. Record the exact starting branch and SHA.
7. Resolve and verify the exact approved predecessor gate SHA. Do not infer it from the current
   worktree.
8. Work on this milestone only and stop at its gate.

All newly authored source, comments, identifiers, commit messages, test names, and repository
Documentation must be in English.

## Status and predecessor

The repository has consumed M52-M56 for unrelated work. The maintainer treats G56 as passed
because its intended implementation was not feasible. Resolve the exact approved G56 SHA from the
repository's approved G56 report/ROADMAP before any edit. If no single exact SHA is recorded, stop.

Branch: `topic/m57-remove-frozen-tier`

Commit prefix: `M57:`

Gate: `G57`

## Goal

Remove exactly the frozen reference tier while preserving all required legal and historical
provenance. No production CPU semantics or active uPD9002 source may change.

## Scope

Remove exactly:

- `win9x/`
- `i286x/`
- `hlp/`
- `cpuxva/memoryva.x86`

Explicitly out of scope:

- M50 protected-mode deletion;
- any active source under `cpu/upd9002/`;
- any semantic, timing, prefetch, interrupt, state-layout, or dispatch change.

## Precondition before the first commit

1. Check whether annotated tag `archive/frozen-win9x-i286x-g56` exists.
2. If absent, create and push it at exactly the resolved approved G56 SHA.
3. If present, verify that its peeled commit is exactly the resolved approved G56 SHA.
4. A tag pointing elsewhere is a hard stop.

Record the local and remote tag object and peeled commit IDs.

## Commit plan

### Commit 1 — Preserve legal evidence

- Preserve `win9x/readme.txt` as legal and lineage evidence.
- If it satisfies repository encoding policy, add byte-identical
  `LICENSES/legacy-vaeg.txt` and prove SHA-256 equality.
- If it does not satisfy policy, add:
  - a byte-identical archival copy;
  - a separate UTF-8 transcription;
  - `docs/legal/legacy-source-provenance.md` documenting both SHA-256 values and their relationship.
- Update no production source.

### Commit 2 — Delete the frozen tier

```sh
git rm -r win9x i286x hlp
git rm cpuxva/memoryva.x86
```

Do not delete similarly named active files elsewhere.

### Commit 3 — Repair live references

Update only current documentation, build configuration, repository checks, and packaging references
that would otherwise point to deleted paths. Do not rewrite historical reports or immutable evidence.

## Required proof of no behavior change

Before the first deletion commit, run both M43 architectural profiles without skip at the approved
G56 predecessor and prove exact equality with the immutable M43/G51 evidence at G51 SHA
`78c712f0bab53a6960cfc102eae7ee54b3fc29ef`. If the pre-deletion G56 result differs, stop and
produce a read-only drift audit; do not rebaseline. After deletion, prove
exact equality with that verified G56 predecessor for:

- dataset identity;
- category counts and resolved hash sets;
- applicable pass and failure hash sets;
- every failure signature;
- every termination class;
- CI and full summaries;
- failure-sidecar files and their digests;
- signature-index digests.

## Gate G57

- All required native and cross-platform builds and tests are green.
- Both verified CI and full SST profiles ran without skip.
- Every M43 artifact listed above is byte-identical in behavior and digest.
- The report lists removed paths, archive-tag object and peeled SHA, legal-evidence locations, and all
  relevant SHA-256 values.
- `cpu/upd9002/` has no behavioral diff from the approved G56 predecessor.

Write `docs/agents/reports/m57_remove_frozen_reference_tier.md`, report the final SHA, and stop.
