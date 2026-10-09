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
# M47 — Determine uPD9002 REP-prefixed 0x0F correctness

This maintainer-approved task supersedes the original M47 protected-mode
deletion-inventory task after its pre-implementation audit disproved the
assumption that the protected-mode cluster was unreachable.

## Session boundary

Work only on M47.  Do not implement a semantic or state-policy decision, update
an accepted baseline, begin M48, or declare G47 passed.

## Goal

Determine the correct uPD9002/V52 behavior of REP-prefixed 0x0F instructions
and prepare enough evidence for the maintainer to approve one exact behavior
and state policy for M48.  M47 is evidence and decision preparation only.

## Accepted current behavior

Record, but do not certify as architecturally correct:

* unprefixed 0x0F enters the V30 0x0F handler;
* REPNE/REPE-prefixed 0x0F enters i286c_cts;
* F2/F3 0F 01 F0 can execute legacy LMSW and set MSW_PE;
* imported state can restore MSW_PE and activate i286c_selector.

No dispatch, instruction, state, trace, fixture, or accepted-baseline behavior
may change in M47.

## Evidence sources

Pin and identify:

1. NEC V20/V30 primary manuals.
2. Repository or maintainer-provided PC-88VA/uPD9002 technical documents.
3. The M43-pinned SingleStepTests V20 dataset.
4. M42 dispatch artifacts and M43 failure sidecars.
5. A PC-88VA real-hardware probe unless authoritative uPD9002-specific
   documentation resolves the behavior unambiguously.

For each document record title, edition/date, source, SHA-256, relevant page and
section, and described processor or machine.  Unsourced emulator code is not
architectural proof.

## Instruction matrix

Characterize 0F xx, F2 0F xx, and F3 0F xx for every second-byte form
represented by active V30 dispatch, legacy CTS tables, M43 records, or the
protected-mode dependency cluster.  Each row records bytes, ModR/M constraints,
initial registers/FLAGS/memory, expected IP, final registers/FLAGS, memory/I/O,
interrupt or exception behavior, prefix treatment, evidence, and confidence.

Keep all candidate outcomes open until evidence is collected:

* A: prefix ignored; normal V30 0x0F executes.
* B: uPD9002-specific prefix meaning.
* C: reserved or undefined encoding.
* D: behavior depends on second byte or another condition.

## PC-88VA probe

Add a standalone, non-production probe with exact build instructions and binary
digest.  Run at most one potentially dangerous case per boot/invocation, record
the case before execution, record completion/registers/FLAGS/IP-visible
result/memory/exception afterward, provide hang/reset recovery, avoid disk
writes, and emit machine-readable results.  Emulator and hardware results must
remain separate.

Cover at least:

* a harmless implemented V30 0x0F form, unprefixed/F2/F3;
* 0F 01 F0, F2 0F 01 F0, and F3 0F 01 F0;
* representative CTS0 and CTS1 forms;
* LOADALL286 only when a safe recoverable probe is justified.

## SingleStepTests analysis

Using the exact M43 dataset, enumerate every F2/F3+0F record, metadata form,
expected outcome, prefetch class, and current VAEG comparison.  V20 evidence is
not by itself proof of uPD9002 behavior.  Identify every M43 record hash and
failure signature affected by each candidate outcome.

## State policy analysis

Inventory accepted states with MSW_PE or nonzero GDTR, IDTR, LDTR/LDTRC, or
TR/TRC in committed fixtures, generated M42–M46 states, and any explicitly
provided user corpus.  Compare, but do not implement or select without G47:

1. reject protected-state residue;
2. canonicalize it to native state;
3. translate through a compatibility adapter;
4. preserve it only as opaque serialized data outside runtime execution.

Report data-loss, silent-change and compatibility risks, state-version need,
G41/M44 matrix effect, CPU_SHUT effect, and implementation complexity.

## Prospective M48 transition

Do not change accepted baselines.  Produce an exact prospective manifest of
M42 final-graph/provenance/support-map rows, M43 record hashes and failure
signatures, state fixtures and import outcomes, and CPU_SHUT fixtures expected
to change or remain fixed for every candidate decision.

## Decision document and report

Create or amend an ADR with current behavior, documentary/V20/hardware evidence,
selected semantics and state policy if evidence suffices, exact authorized M48
transition scope, and unresolved cases.  If evidence is insufficient, leave the
decision unresolved and forbid M48.

Create:

docs/agents/reports/m47_upd9002_rep0f_correctness.md

It must include source identities, complete instruction matrix, corpus analysis,
hardware probe design/results, state occurrences and policy comparison,
prospective transition manifest, unresolved questions, M42–M46 preservation,
commands/statuses, hosted CI, and the G47 checklist.

## G47 explicit approvals

The maintainer must separately approve:

1. REP+0F semantic rule.
2. MSW_PE/protected-residue state policy.
3. Exact M48 dispatch/state changes.
4. Exact M42/M43 baseline-transition scope.
5. Whether evidence is sufficient to begin M48.

Silence or general approval is not authorization.

## Preservation gate

M47 leaves byte-identical all active behavior; M42 graph, provenance, support
map, trace and fixtures; M43 identity/classifications/gaps/signatures; M44 state
behavior/payloads; M45 native dispatch; M46 constructor/immutability; and the
CPU_SHUT FLAGS 0000 anomaly.  Production binaries contain no M47 probe or
research seam.
