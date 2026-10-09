# P09 — Basic arithmetic, sqrt, and precision/rounding control

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P08.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Implement all documented add/subtract/reverse/multiply/divide/reverse, integer-memory variants, pop/destination forms, and FSQRT. Reuse the exception framework and raw barrier. Apply PC only to applicable operations and RC to the actual destination behavior.

Freeze projective/affine infinity, NaN selection, denormal-operand, overflow/underflow, and invalid-case behavior with separate evidence status for truly disputed details.

## Acceptance

Use asymmetric operands so reverse/destination bugs cannot pass: test subtraction/division in each direction and TOP alias/pop cases. Cover PC 24/53/64, all RC modes, halfway cases, cancellation, signed zero, exponent extremes, masks, and special classes.

Compare exact ordinary outputs to an independent oracle; verify flags/state as independent architecture facts. Resolve FSQRT zero/+infinity/endpoints from primary sources rather than copying the v4 restriction.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P09
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Complete basic arithmetic handlers, independent raw-result/flag fixtures, PC/RC coverage, latency entries and source-backed special rules.

## Boundaries and failure handling

No relying on host x87 or a generic later-x87 helper; no tests using symmetric operands only; no rewriting expected outputs from the new handlers.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
