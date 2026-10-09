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
# M66a — Remove obsolete CPU286 save-state compatibility

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

## Scheduling

This task starts only after terminal G65m is formally approved at
`81887aae14f718d7d4d0f2a7bd3fe05d5ea80630`. In the approved M66 bundle,
M66a executes first on `topic/m66-upd9002-remove-i286-compat` and produces an
internal checkpoint for M66b. M66a is not independently approved and no G66a
pass is claimed.

Branch: `topic/m66-upd9002-remove-i286-compat`

Commit prefix: `M66a:`

Internal checkpoint: `tests/ssts/campaigns/g66b/checkpoints/m66a.json`

Terminal bundle gate: `G66b`

## Goal

Make the explicitly permitted state-format break that removes obsolete CPU286 compatibility
serialization and loading. This is not a handler rename milestone.

## Required work

- Inventory every active save/load path, tag, payload, adapter, compatibility transform, and test.
- Define the new uPD9002 state version and exact rejection behavior for obsolete payloads.
- Remove obsolete compatibility code only after focused import/export and rejection tests exist.
- Update current documentation and migration notes.
- Preserve historical reports and immutable fixtures as evidence; they may remain named CPU286.

## Scope restrictions

- Do not rename remaining active handler files/macros here unless required solely by the state API;
  broad active-core identity cleanup belongs to M66b.
- Do not silently accept an old payload under a new layout.

## Gate G66a

- New state round trips exactly at supported boundaries.
- Obsolete CPU286 payloads fail deterministically and atomically.
- Rejection leaves live machine state unchanged.
- Standard system boot/save/load regressions are green.
- Architectural SST result is unchanged from the approved predecessor.

Write `docs/agents/reports/m66a_upd9002_drop_cpu286_state_compat.md` and stop.
