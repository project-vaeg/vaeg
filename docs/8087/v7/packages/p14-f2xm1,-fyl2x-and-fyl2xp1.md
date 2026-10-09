# P14 — F2XM1, FYL2X and FYL2XP1

**Initial status:** NOT_STARTED  
**Prerequisites:** P12, P13

## Work

Integrate bounded cancellation-safe production kernels with exact documented stack/operand/domain/exception behavior.

## Acceptance

Normal and extended certified corpora, endpoint neighbors, near-one and tiny values, extreme multipliers, applicable CW modes, guest CPU execution, and runtime-link audits pass.

Run:

```sh
python3 tools/8087/gate.py --package P14
```

## Required outputs

Three complete production handlers and numerical reports.

## Boundaries

No naive host exp/log or partial implementation.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
