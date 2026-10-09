# P11 — Exact 14-byte environment and 94-byte save/restore

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P10.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Complete FLDENV/FSTENV/FNSTENV/FSAVE/FNSAVE/FRSTOR using exact 8087 formats and memory accessors. Freeze pointer/opcode capture rules, exemptions, register-image order, reserved fields, post-FSAVE initialization, and environment-store mask effects.

Keep guest save images distinct from the emulator's clock-aware savestate. Resolve all format uncertainties using original sources before passing.

## Acceptance

Compare independent byte-by-byte golden images for nontrivial CW/SW/TW, nonzero TOP, distinct raw patterns in all eight registers, high physical address nibbles, and opcode bits. Test 14/94-byte boundaries and untouched surrounding memory.

Round-trip old raw classes according to guest instruction semantics, verify post-save behavior and FN versus WAIT forms under pending exceptions, and ensure guest saves do not overwrite the oscillator or requested configuration.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P11
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Complete guest environment handlers, cited layout and order, independent golden images, pointer/mask/reset tests.

## Boundaries and failure handling

No host struct serialization, segment-selector or protected-mode image, modern FXSAVE order assumption, or convenient 'clear everything' after restore.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
