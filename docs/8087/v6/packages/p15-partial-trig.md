# P15 — 8087 FPTAN and FPATAN production handlers

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P14.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Integrate the selected restricted-domain kernels. Implement the actual documented operand/result order and the separately frozen FPTAN pair policy, not modern tan-plus-one or unrestricted atan2 assumptions. Retain independent raw-pair and mathematical reconstruction checks.

Apply stack, exception, PC/RC, and timing policies through the same transaction framework.

## Acceptance

Run normal and extended certified corpora for both operations, targeted domain/endpoint neighbors and tiny ratios. Verify FPTAN both components, physical/logical stack order, ratio bounds, exact identity cases, and full-stack overflow. Verify FPATAN operand swapping would fail independent fixtures.

Execute actual CPU guest sequences and compare deterministic raw traces. The numerical tolerance must not excuse wrong tags, condition bits, pointers, pops, or special-operand exceptions.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P15
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Two complete handlers, pair-policy evidence, guest byte fixtures, independent numerical/architectural reports and full five-function coverage.

## Boundaries and failure handling

No unrestricted later-x87 semantics or a ratio-only pass that hides an invalid pair. Silicon-identical least bits remain a separate evidence claim.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
