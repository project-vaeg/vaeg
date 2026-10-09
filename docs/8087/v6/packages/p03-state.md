# P03 — Singleton state, raw80 codec, classifier, and backend scope

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P01, P02.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Add one optional machine-owned 8087 state, eight physical raw registers, TOP/tag addressing, CW/SW/pointers/opcode, and pending fields. Keep architectural state separate from config and scheduler state. Implement explicit raw80 load/store/trace codecs and class detection.

Wrap SoftFloat calls with save/set/clear/call/capture/restore on every path. Record source-backed PC/RC mapping and reserved-field policy. Reset architecture through a single tested function without changing the machine oscillator.

## Acceptance

Round-trip representative raw bit patterns from every encoding class, including noncanonical values, without numeric normalization by the codec. Test TOP wrap, stack indexing and alias cases, defined reset fields, and exactly one owned device/callback across repeated initialization.

Use one device with changing CW values and sentinel prior backend settings. Prove no mode/flags leak across successful and early-exit calls. Reject or classify unsupported raw inputs before any backend call.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P03
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Device state and ownership, raw classifier/codec, backend-state wrapper, state/reset/alias tests, updated private ABI description.

## Boundaries and failure handling

No device arrays, count setting, second-coprocessor framework, raw C-struct savestate, or native floating-point debug formatting. Guest transfer normalization rules remain separate from the raw codec.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
