# P11 — 8087 environment and save/restore

**Initial status:** NOT_STARTED  
**Prerequisites:** P10

## Work

Implement FLDENV/FSTENV/FNSTENV/FSAVE/FNSAVE/FRSTOR with exact 14-byte environment and 94-byte save image, source-backed pointer/opcode/register ordering and post-save behavior.

## Acceptance

Independent byte images with nontrivial CW/SW/TW/TOP/register patterns and high address bits match exactly; surrounding memory stays untouched; guest save operations do not alter oscillator configuration.

Run:

```sh
python3 tools/8087/gate.py --package P11
```

## Required outputs

Environment/save handlers and byte-image tests.

## Boundaries

No FXSAVE/287 protected-mode/host-struct assumptions.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
