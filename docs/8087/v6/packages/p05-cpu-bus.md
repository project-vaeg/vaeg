# P05 — Native ESC/POLL/FPO2 and correct bus side effects

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P01, P03, P04.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Connect the production native CPU decoder to the device boundary. Reuse existing EA/segment logic, preserve displacement consumption and CPU-side timing, and latch the CPU's first read word for applicable FPU loads. Implement absent-device behavior and test FPO2 as no attached coprocessor.

Ensure CPU execution mode is selected before native FPO/POLL dispatch. Preserve 8080 meanings and lengths for the same byte values. Keep the original VA/VA-91 model policy and valid VA2/VA3 presence gate.

## Acceptance

Exercise all 2048 native decoder slots, displacement/default-segment/override cases, and representative address wrap boundaries through production decoding. Count MMIO reads and writes with FPU absent, enabled load, and enabled store: no duplicate first-word read or phantom store.

Verify native FPO2 byte lengths/bus behavior without 8087 mutation, and independent 8080 byte-program regressions. No-FPU detection sentinel remains unchanged and POLL cannot wait on a nonexistent BUSY source.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P05
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Production CPU hooks, bus operand-latch contract, exhaustive decoder and MMIO tests, FPO2/8080 regression fixtures.

## Boundaries and failure handling

No treating 66h/67h as 386 prefixes or one-byte NOPs without the active NEC semantics; no absent-FPU native decoding inside 8080 mode. Actual repository CPU model controls applicability.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
