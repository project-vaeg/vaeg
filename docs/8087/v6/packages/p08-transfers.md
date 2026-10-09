# P08 — All real, integer, and packed-BCD transfers

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P07.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Implement every documented FLD/FST/FSTP, FILD/FIST/FISTP, and FBLD/FBSTP operand width and register form. Use pre-instruction TOP indices for aliases. Handle exact widening, instruction-specific normalization, destination RC rounding, signed zero, invalid/indefinite outputs, and pop/store eligibility.

Add source-derived latency entries and independent exact reference vectors for every form. Preserve memory ordering/width and the first-word latch rules.

## Acceptance

Execute each encoding/form with positive cases and applicable PC/RC/mask combinations. Test integer min/max and neighbors, halfway rounding, BCD 18-digit limits/sign, malformed BCD per policy, denormal/NaN/old encodings, stack overflow/underflow, and destructive alias destinations.

Assert exact written bytes, unchanged surrounding memory, tags/TOP, flags, and suppressed effects where required. A register-form copy is checked against its own source semantics, not assumed equivalent to the raw savestate codec.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P08
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Complete transfer handlers, all-width/BCD fixtures, executed form coverage and updated latency table.

## Boundaries and failure handling

No narrowing 64-bit integers through host double, no decimal host formatting/parsing as BCD arithmetic, no silent omission of m80 or BCD forms.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
