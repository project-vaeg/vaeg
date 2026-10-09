# P12 — FPREM, FSCALE, and FXTRACT

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P09, P10, P11.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Implement partial remainder with deterministic bounded reduction steps, C2 and 8087 quotient-bit rules; implement source-restricted scale and extract. Specify intermediate/repeated-execution behavior and special operands explicitly.

Use integer/significand operations and approved software arithmetic. Do not substitute IEEE remainder for the 8087 instruction.

## Acceptance

Run repeated FPREM guest loops to completion for large exponent differences with a justified progress bound, and test incomplete condition codes separately. Check small-dividend quotient-bit retention against independent original-source fixtures, quotient signs, exact divisibility, and zero/special operands.

For FSCALE test integral boundary/domain and source-specific invalid/exception behavior. For FXTRACT check two-result stack order, zero/denormal/special cases, full-stack overflow, and raw output exponents.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P12
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Three complete handlers, repeated-execution guest programs, old quotient-bit fixtures, source-specific scale/extract tests and latency entries.

## Boundaries and failure handling

No later FPREM1 semantics, arbitrary infinite reduction loops, or unchecked host integer shifts/conversions outside valid ranges.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
