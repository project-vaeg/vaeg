# P10 — Comparison, constants, stack, sign, and administration

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P09.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Complete FCOM/FCOMP/FCOMPP/FICOM/FICOMP/FTST/FXAM; all constant loads; FXCH/FFREE/FINCSTP/FDECSTP; FCHS/FABS/FRNDINT/FNOP; and all administrative forms not already complete. Finish control/status word load/store and IEM/IC handling.

Use source-specific constant raw values and rounding behavior. Tag examination and condition codes must reflect 8087 old encodings and preserved/undefined-bit distinctions.

## Acceptance

Execute all documented forms, including nonzero TOP and empty/valid/special tags. Distinguish unordered comparison from stack faults, and projective from affine infinity. Test defined/preserved condition bits, sign manipulation of special values, wraparound, constants under applicable CW modes, and real WAIT/FN byte sequences.

Probe every administrative operation while relevant pending/cleared conditions exist. A legitimate no-op must be distinguishable from a generic unimplemented path in the executed coverage report.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P10
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Complete comparison/control/stack families, exact constant fixtures, raw-class examination table, condition-bit and WAIT/FN tests.

## Boundaries and failure handling

No later FUCOM or FNSTSW AX fallback. Do not apply numeric reclassification to an operation that must preserve payload or tags.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
