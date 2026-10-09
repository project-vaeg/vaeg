# P18 — Configuration, savestate and debugger

**Initial status:** NOT_STARTED  
**Prerequisites:** P11, P16

## Work

Complete enabled/clock settings, 5/8/10 presets and custom integer Hz, active/requested/reset-required UI, explicit savestate payload/versioning, and minimal read-only debugger.

## Acceptance

Config defaults/bounds/deferred apply, FINIT versus reset, old/malformed states, nonzero residue, pending line restore, repeated reset/load one-device invariant, and available frontend build/tests pass.

Run:

```sh
python3 tools/8087/gate.py --package P18
```

## Required outputs

Settings/UI, savestate integration, lifecycle tests and user documentation.

## Boundaries

No hot-plug, raw struct state, silent clock clamp, or second-device UI.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
