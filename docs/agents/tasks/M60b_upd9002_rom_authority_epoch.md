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
# M60b — Formalize ROM authority and correct the uPD9002 target-policy epoch

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

Prerequisite: G60a explicitly approved. Use the exact approved G60a SHA and fresh post-G60a
scoreboards; do not substitute G59 counts.

Branch: `topic/m60b-upd9002-rom-authority`

Commit prefix: `M60b:`

Gate: `G60b`

## Goal

Create a content-addressed ROM target-authority pack and a new target-policy epoch. Correct
`6C-6F` and exact INS/EXT gap classifications without changing production CPU semantics.

## Authority inputs

Obtain the maintainer-supplied monitor ROM and debugger evidence out of tree. Record:

- ROM SHA-256, byte size, mapping/base, and provenance;
- `0x66A8A` table start, end/terminator, adjacent boundary, and all raw three-byte records;
- deterministic `(mask,value,group)` expansion;
- group-to-mnemonic mapping and string addresses;
- independent debugger evidence for BRKFEM;
- exhaustive string-pool range and search method;
- main-table evidence proving primary `6C-6F` absence.

Do not commit a ROM binary without explicit authorization. Commit lawful extracts, hashes, scripts,
decoded tables, and manifests. If the bytes needed to verify the authority claims are unavailable,
stop before classification changes.

## Required `0F` authority result

Bind and verify:

- TEST1 `0F10/11/18/19`;
- CLR1 `0F12/13/1A/1B`;
- SET1 `0F14/15/1C/1D`;
- NOT1 `0F16/17/1E/1F`;
- ADD4S/SUB4S/CMP4S `0F20/22/26`;
- ROL4/ROR4 `0F28/2A`;
- BRKFEM `0FFE imm8`;
- BRKEM `0FFF imm8`;
- absence of `0F31/33/39/3B` from the complete table.

Record REPC/REPNC and PREPARE/DISPOSE presence independently. Do not infer FPO2, RETEM, or CALLN
absence from this table or from missing generic strings.

## Exact `6C-6F` correction

Before execution, define structural selectors for every selected record whose primary opcode is
`6C`, `6D`, `6E`, or `6F`, including plain, segment-prefixed, and repeat-prefixed forms represented
by the corpus.

- Current `applicable` records transition to `known_target_gap`.
- Every such gap receives `gap_kind=documented_silicon_absent`.
- Existing `known_target_gap` entries under these opcodes retain exact hashes and change only their
  gap kind where needed.
- Never partition by pass/fail outcome.
- Report exact per-selector and union counts and sorted-hash digests.
- After correction, no selected `6C-6F` record may remain in the blocking applicable denominator.

This is the one authorized `target_authority_correction` in the master specification.

## Other exact gap corrections

Preserve top-level `known_target_gap` and exact resolved hashes for `0F31`, `0F33`, `0F39`, and
`0F3B`, but set `gap_kind=documented_silicon_absent` if necessary.

Preserve `0F28` as `known_target_gap/implementation_missing`; record that M62b2 is mandatory. Do not
implement or make it applicable here.

No other classification or gap kind may change.

## Historical G43 reconciliation

Preserve G43/G58/G59/G60a artifacts byte-for-byte. Explicitly record:

- the OUTS fixture correction made 1,204 V20 records pass;
- the historical post-fix residuals included 6E=417 and 6F=224;
- these are V20 differential outcomes, not target progress;
- reclassified passes and failures are `retired_applicable`, not `newly_passing`.

Do not revert the fixture correction. Keep any V20 diagnostic execution separate from the target
blocking profile.

## New target-policy epoch

Create a versioned `target_policy_id` and canonical digest. Dataset and comparison-contract IDs do
not change. Generate separate architectural CI/full and fingerprint artifacts under G60b.

The transition must enumerate:

- retired applicable pass hashes/count/digest;
- retired applicable failure hashes/count/digest;
- exact classification changes;
- unaffected applicable before/after digest;
- authority-manifest digest;
- denominator, pass, and failure totals derived from exact hashes.

Compare unaffected applicable hashes against G60a and require no new failure or signature
regression. Do not guess the new total from G59 or historical 641 failures.

## FPO caveat

Record that missing `FPO1`, `FPO2`, or `ESC` strings are non-evidence because the monitor uses
individual FPU mnemonics. Do not classify 66/67 in M60b; M60c owns that audit.

## Scope restrictions

- No production semantic change and no change under `cpu/upd9002/`.
- Do not remove or modify active `6C-6F` handlers here.
- Do not alter fixtures, comparison contracts, or historical artifacts.
- Do not describe denominator retirement as a fix.

## Gate G60b

- Authority pack is deterministic, content-addressed, and independently reviewable.
- Exactly the authorized structural hashes leave `applicable`.
- Exactly the authorized gap-kind changes occur.
- `0F28` remains implementation-missing and mandatory.
- Unaffected applicable results satisfy the ratchet.
- Historical artifacts and the production CPU tree are unchanged.

Write `docs/agents/reports/m60b_upd9002_rom_authority.md`, report the candidate SHA, and stop.
