# P17 — Production VA interrupt route, mode gating, and timing

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P16.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Finish the production VA2/VA3 BUSY/INT adapter using available machine evidence and actual controller code. Test native-mode access and temporary 8080 transitions, model/reset behavior, signal polarity/masking/acknowledgment, and pending condition reconciliation.

Complete source-cited nominal latency coverage. Run long operations with real scheduled timers and the actual guest wait/recovery sequence; remove test-fixture wiring from production.

## Acceptance

Install a self-authored guest handler in the headless machine. Trigger an unmasked 8087 exception through a real ESC, enter the handler via the production interrupt controller, clear/acknowledge as specified, return, and resume normally. Test CPU interrupt masking and no duplicate edges.

Run full baseline, clock scaling/residue tests, mode transitions without decoding native bytes in 8080 mode, and absent/model-disabled paths. All mandatory production routes need evidence and tests; physical hardware traces may remain NOT_RUN.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P17
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Cited production signal route, real guest handler tests, complete timing table/audit, scheduler/mode regressions and full-baseline report.

## Boundaries and failure handling

A host callback or synthetic IRQ cannot pass production acceptance. If route evidence is truly missing, mark this package INTEGRATION_BLOCKED, finish independent P18/QA work, and request only the missing source/address/decision.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
