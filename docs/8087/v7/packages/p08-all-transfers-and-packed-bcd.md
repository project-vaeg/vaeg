# P08 — All transfers and packed BCD

**Initial status:** NOT_STARTED  
**Prerequisites:** P07

## Work

Implement every documented FLD/FST/FSTP, FILD/FIST/FISTP, and FBLD/FBSTP form with exact stack/tag/store behavior and destination rounding.

## Acceptance

Every documented form has positive execution plus applicable boundary, rounding, invalid, stack-fault, and raw-class tests. Written bytes and surrounding memory are checked.

Run:

```sh
python3 tools/8087/gate.py --package P08
```

## Required outputs

Complete transfer handlers and executed form coverage.

## Boundaries

No omission of m80, m64 integer, or packed BCD forms.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
