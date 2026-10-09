# P14 — F2XM1, FYL2X, and FYL2XP1 production handlers

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P12, P13.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Integrate the selected cancellation-safe exp/log kernels into the sole 8087, with documented domains, operand order, stack pops, PC/RC, special-class barrier, and exception effects. Preserve scaled exponent handling near format limits and tiny log1p/expm1 inputs.

Bind production forms and latency entries to the real decoder and independent reference tests.

## Acceptance

Run normal and extended certified corpora for all three instructions, exact identities and zero signs, domain-edge neighbors, near-one logs, tiny arguments, extreme multipliers, and applicable CW combinations. Require the frozen error gate plus exact known state/memory/condition effects.

Execute self-authored guest sequences through the CPU, not only numeric helper unit tests. Confirm deterministic output and no external oracle/runtime lib dependency in the linked VAEG target.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P14
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Three production transcendental handlers, executed semantic coverage, worst-case/corpus reports and guest integration traces.

## Boundaries and failure handling

No naive exp(x)-1 or log(1+x) cancellation, unbounded loops, silent domain extension, or partial/stub status for one of the three instructions.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
