# P16 — Complete exceptional-operand and recovery coverage

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P08, P09, P10, P11, P12, P14, P15.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

For disputed behavior, consult only approved behavioral evidence from the silicon ledger;
never consume raw microcode/decoder code or evidence-only article snippets as implementation
input. A microcode claim alone does not certify the VAEG result; link independent test cases.


Cross-audit every documented form against applicable raw classes, masks, PC/RC/IC/IEM, stack faults, pointer and condition-code effects. Close gaps in handlers and tests, not just the coverage metadata. Exercise competing exceptions and class-specific result delivery.

Reconcile administrative recovery, environment restore, and pending INT transitions through the complete instruction set.

## Acceptance

The form-to-applicable-case registry must agree with actual executed tests. Run negative coverage-checker fixtures: missing special test, fake test name, skipped test, stale source digest, and a generic fallback. All must fail.

Test memory stores and destructive stack forms under exceptional paths with bus counters, preserving irreversible bus effects where required. Confirm the single device remains usable after recovery, not merely that a flag was set.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P16
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Closed exception/class matrix, repaired handlers, audit fixtures, full-family executed test evidence and remaining evidence list.

## Boundaries and failure handling

Do not turn source-defined failures into provisional behavior to meet a count. Do not rewrite correct earlier handlers into a universal exception rule for convenience.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
