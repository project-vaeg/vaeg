# P10 — Compare, constants, stack and control

**Initial status:** NOT_STARTED  
**Prerequisites:** P09

## Work

Complete comparisons/examination, constant loads, FXCH/FFREE/FINCSTP/FDECSTP/FABS/FCHS/FRNDINT/FNOP and remaining administrative/control/status forms.

## Acceptance

All documented forms execute with nonzero TOP, empty/valid/special tags, projective/affine behavior, defined/preserved condition bits, pending/recovery states, and correct WAIT/FN byte streams.

Run:

```sh
python3 tools/8087/gate.py --package P10
```

## Required outputs

Complete compare/control/stack handlers and constant fixtures.

## Boundaries

No FUCOM or undocumented FNSTSW AX fallback.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
