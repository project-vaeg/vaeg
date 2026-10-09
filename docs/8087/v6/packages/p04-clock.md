# P04 — Integer 10 MHz timebase and serial executor foundation

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P00, P03.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Implement active/requested clock fields, validation, fractional NDP-clock conversion, and nominal latency schema per clock-and-timing.md. Start with source-cited forms needed for the early slice. Build a serialized scheduler fixture that stages state, advances time without running the next CPU instruction, and commits once.

Reuse the machine's real timebase or document an exact rational adapter. Integrate the minimal scheduler seam now rather than postpone frequency effects to a final UI package.

## Acceptance

At default 10000000 Hz and custom values, compare exact conversion to independent arbitrary-precision integer arithmetic. Include 100-clock reference checks, 7159091 Hz, long sequences, chunk splitting, overflow boundaries, reset, and residue serialization at a safe boundary.

In a fixture schedule a non-CPU event during a long operation. It must run in order, without recursive CPU execution or early result visibility. FINIT leaves active frequency/residue unchanged; machine reset applies requested frequency.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P04
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Checked clock converter, active/requested configuration semantics, latency schema/initial entries, serialized scheduler seam and timing tests.

## Boundaries and failure handling

No decorative frequency control, per-instruction floating rounding, host sleeps, second NDP queue, or fake post-commit BUSY countdown. Do not claim overlapping execution or cycle accuracy.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
