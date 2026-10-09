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
# M61 — Correct C6/C7 register-form MOV-immediate execution

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

Prerequisite: G60e explicitly approved and fresh ranking confirms the M59 population.

Branch: `topic/m61-upd9002-mov-immediate-register`

Commit prefix: `M61:`

Gate: `G61`

## Goal

Correct only the C6/C7 register-destination defect proven by M59. Do not treat F7 `/2` as the same
primitive.

## Evidence-bound scope

M59 established:

- every C6/C7 memory form passed;
- every C6/C7 failure was a register form whose actual destination remained initial;
- apparent register passes included value coincidences;
- no unexpected FLAGS changes;
- F7 `/2` failures were a distinct low-memory word-path defect.

Reverify those exact properties in the current target-policy epoch before editing.

## Required work

- Fix register-form dispatch/execution for C6 and C7 only.
- Add focused tests that cannot pass by initial/immediate value coincidence.
- Preserve all memory forms, FLAGS, instruction length, IP, prefixes, and termination.
- Do not modify generic EA logic, F7 `/2`, DIV/IDIV, or target classification.

## Gate G61

Every applicable non-coincidental C6/C7 register form is green; all memory forms and unrelated
opcodes retain exact approved results. Full profiles, ratchet artifacts, and deterministic evidence
are required.

Write `docs/agents/reports/m61_upd9002_mov_immediate_register.md` and stop.
