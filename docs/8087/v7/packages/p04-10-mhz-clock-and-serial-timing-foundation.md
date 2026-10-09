# P04 — 8087 clock and serial timing foundation

**Initial status:** NOT_STARTED  
**Prerequisites:** P00, P03

## Work

Implement requested/active integer-Hz clock fields, default 8000000 Hz, 1-20 MHz validation, reset-time application, exact residue conversion, latency schema, and scheduler seam for SERIAL_TIMED_V1.

## Acceptance

100-clock 5/8/10 MHz conversion checks, awkward custom frequency, long-run residue, overflow guards, reset and savestate-safe residue tests pass. A non-CPU event scheduled during service executes in order without early next CPU instruction.

Run:

```sh
python3 tools/8087/gate.py --package P04
```

## Required outputs

Clock/config core, exact converter, scheduler seam, timing tests.

## Boundaries

No decorative frequency, host sleeps, per-instruction rounded drift, second FPU queue, or fake post-commit BUSY.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
