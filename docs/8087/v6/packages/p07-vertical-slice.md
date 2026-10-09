# P07 — First integrated guest computation at configurable clocks

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P02, P03, P04, P05, P06.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Implement the minimal documented path for guest initialization/detection, FLD1 and/or memory loads, FADD, a memory FSTP, and memory status/control stores. Add the actual persisted presence and integer-Hz config parser so the slice uses ordinary VAEG settings, not a test-only hidden device.

Run a self-authored native guest through the normal CPU and bus, with a minimal redistributable setup when a proprietary ROM would otherwise be needed. It must report pass/fail to the host harness.

## Acceptance

Run absent/present sentinel detection, initialization, two-value arithmetic/store/check, and normal guest exit. Repeat enabled at 5 and 10 MHz and an awkward custom frequency: semantic traces agree while isolated service-time accounting changes correctly.

Requested clock changes do not affect the running machine before reset. Repeated reset does not create a second device. Run the full applicable baseline and prove the new seam has no absent-device regression.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P07
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Working production vertical slice, ordinary config path, redistributable guest source/bytes, CPU/bus/timing traces and full-baseline report.

## Boundaries and failure handling

Detection-only or direct handler-call demos fail. Do not wait for the entire instruction set to establish that real VAEG integration works. This package is explicitly a partial implementation, not goal completion.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
