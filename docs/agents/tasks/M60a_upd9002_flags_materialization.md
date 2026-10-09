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
# M60a — Correct FLAGS canonicalization and guest-visible materialization

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

Prerequisite: G59 explicitly approved and lettered milestone tooling approved.

Branch: `topic/m60a-upd9002-flags-materialization`

Commit prefix: `M60a:`

Gate: `G60a`

## Goal

Correct the shared FLAGS get/set/canonicalization primitives and the
9C/9D/9E/9F instruction family using M59 evidence. Do not change interrupt
delivery or IRET in this milestone.

## Required work

Implement only evidence-supported rules for:

- canonical internal FLAGS representation;
- defined-bit updates and fixed/preserved bits;
- PUSHF materialized stack word;
- POPF loadable, preserved, and forced bits;
- LAHF materialized AH image;
- SAHF loadable, preserved, and forced bits.

PUSHF, LAHF, interrupt-frame, and final-FLAGS conventions are independent
unless M59 proved a shared rule. Record any adopted undefined-bit image as a
V20-compatibility convention and add exact hardware-pending coverage where
uPD9002 commonality is unconfirmed.

## Scope restrictions

- Do not change CC/CD/CE interrupt delivery.
- Do not change CF IRET.
- Do not modify D5 or unrelated arithmetic families.
- Do not rename handlers until semantic evidence is green; any rename is a
  separate rename-only commit after the semantic transition is recorded.

## Required semantic gate

- Run the deterministic architectural CI profile against the exact approved
  predecessor.
- Run the verified complete architectural full profile against the exact
  approved predecessor.
- Run the diagnostic fingerprint profile required by this task.
- Verify dataset and comparison-contract identities before comparing results.
- `newly_failing` must be empty.
- Timeout and crash counts must remain zero.
- Enumerate every changed failure signature in deterministic
  content-addressed shards.
- Run the repository's full standard build, lint, smoke, selftest, ctest,
  sanitizer, MinGW, and hosted-CI matrix required by current repository
  conventions.
- Regenerate the opcode/form scoreboard and full failure distribution.
- Record the semantic commit as `evaluated_sha` in an evidence-only follow-up
  commit.
- Write the milestone report, report evidence-commit SHA, and stop for the
  human gate.

## Gate G60a

In addition to the common gate:

- 9C/9D/9E/9F applicable failure populations are zero or each residual has an
  evidence-backed disposition that remains blocking or is explicitly
  approved.
- No previously green unrelated opcode regresses.
- Full distribution is re-ranked before M60b starts.

Write `docs/agents/reports/m60a_upd9002_flags_materialization.md` and stop.
