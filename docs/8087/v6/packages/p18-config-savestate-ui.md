# P18 — Settings, complete savestate integration, and read-only debugger

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P11, P16.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Complete the normal frontend checkbox and clock editor with presets/custom validated integer Hz, default disabled/10 MHz, valid-model gating, active/requested display, and reset-required indication. Keep headless and frontend validation identical.

Integrate explicit FPU/clock/residue/pending payloads into the existing savestate versioning policy. Add a minimal read-only debug display without host floating-point conversion or frontend globals in the core.

## Acceptance

Test missing/valid/invalid config values, lower/upper bounds, deferred apply, FINIT versus machine reset, old states, malformed payload atomic rejection, nonzero clock residue round trips, and pending-line restore without extra callbacks. Repeated reset/load must retain at most one device.

Build the available frontend and test its settings model; record any visual/manual check not run instead of claiming it passed. Restore active snapshot frequency without changing the saved user configuration. Verify debugger inspection cannot change emulated flags or SoftFloat settings.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P18
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Working settings controls, state codec/version policy, lifecycle and malformed-state tests, read-only debug view, user usage instructions.

## Boundaries and failure handling

No hot-plug halfway through operations, silent frequency clamping on snapshot load, raw struct serialization, extra instance UI, or permanent routing selector.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
