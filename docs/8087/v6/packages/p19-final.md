# P19 — Integrated QA, zero omissions, and truthful completion

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P17, P18.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Audit the Stage A receipts and user OCR acceptance alongside implementation provenance.
Record E8087, physical silicon, and other-emulator comparisons in separate NOT_RUN/VERIFIED
fields. Missing optional comparisons do not waive mandatory independent semantic tests.
The Stage A handoff and helper-script tests are never emulator test evidence.


Run the complete mandatory local gate suite and final real-CPU guest tests. Generate fresh instruction/handler/executed-test coverage, semantic/timing traces, provenance/link audit, source digest, and final report. Add/adjust supported CI jobs and aggregation without claiming remote execution that did not occur.

Review runtime sources for stubs, generic later-x87 paths, private assets, accidental oracle linkage, unchecked arithmetic, and documentation drift. Use evidence-aware checks rather than naive bans on legitimate no-op instructions.

## Acceptance

Every documented instruction/form must be COMPLETE_TESTED and displayed IMPLEMENTED_8087. Independent inventory mismatches, missing handlers/tests, stale/skipped required results, unclassified/duplicate decoder slots, baseline regressions, and production route gaps are all zero.

Run all baseline and integration commands; exercise the audit checker's corruption fixtures. Generate the final report matching the actual tested tree. SOFTWARE_VERIFIED requires all mandatory local gates; cross-host and physical/silicon axes may only claim what was actually measured.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P19
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Final machine-readable/Markdown report, complete generated opcode/coverage documentation, CI/trace artifacts, user instructions, exact remaining-evidence and resume records.

## Boundaries and failure handling

Do not push/merge, fabricate CI status, call a disconnected machine complete, or stop at a documentation milestone. A pause/blocker is not a successful final gate.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
