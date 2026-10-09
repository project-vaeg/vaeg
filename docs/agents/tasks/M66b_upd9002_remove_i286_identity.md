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
# M66b — Remove the remaining active I286/i286c implementation identity

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

Prerequisite: the exact M66a internal checkpoint on the approved M66 bundle
branch. M66a is technically validated but not independently approved. M66b is
the terminal bundle closure and produces the only G66b candidate presented for
human review. M67 starts only after approved G66b.

Branch: `topic/m66-upd9002-remove-i286-compat`

Commit prefix: `M66b:`

Gate: `G66b`

## Goal

Rename and delete the final active production 286-derived identities without changing behavior.

## Required work

For production uPD9002 build sources only:

- rename remaining source basenames;
- rename declarations, definitions, macros, and dispatch targets;
- delete now-unreachable helpers and dead compatibility tables;
- update build lists and current documentation;
- use rename-only commits followed by reference-fix commits where repository conventions require it.

## Active-scope zero gate

No active production declaration, definition, dispatch target, source basename, or macro used by the
uPD9002 build may use `I286` or `i286c` identity.

The check must exclude historical/evidence paths, at minimum:

- `docs/agents/reports/**`
- `docs/agents/tasks/archive/**`
- `tools/qa/golden/**`
- historical legal/provenance documents

Do not edit excluded evidence merely to satisfy grep.

## Gate G66b

- Active-scope identity count is zero.
- Production dispatch graph and all architectural SST hashes are unchanged.
- Full build and system regression gates are green.
- Deleted helpers have proven zero reachability.

Write `docs/agents/reports/m66b_upd9002_remove_i286_identity.md` and stop.
