# P16 — Full exceptional-case closure

**Initial status:** NOT_STARTED  
**Prerequisites:** P08-P15

## Work

Cross-audit every documented form against applicable raw classes, masks, PC/RC/IC/IEM, stack faults, pointers, condition effects, recovery paths, and bus-visible stores.

## Acceptance

Coverage registry matches actually executed tests. Corruption fixtures for missing tests/handlers, reclassification, stale results, skip, direction reversal, and generic fallback all fail closed.

Run:

```sh
python3 tools/8087/gate.py --package P16
```

## Required outputs

Closed exception matrix, repaired handlers and negative audit fixtures.

## Boundaries

No relabeling documented cases as provisional merely to pass.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
