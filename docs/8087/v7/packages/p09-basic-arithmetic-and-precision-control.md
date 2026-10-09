# P09 — Basic arithmetic and precision control

**Initial status:** NOT_STARTED  
**Prerequisites:** P08

## Work

Implement all documented add/subtract/reverse/multiply/divide/reverse forms and FSQRT with PC/RC and source-backed special-value/exception semantics.

## Acceptance

Asymmetric operands catch direction bugs; PC 24/53/64 and all RC modes, signed zero, cancellation, extreme exponents, masks, aliases, and pop forms pass against independent references.

Run:

```sh
python3 tools/8087/gate.py --package P09
```

## Required outputs

Arithmetic handlers, exact fixtures, PC/RC coverage and timing entries.

## Boundaries

No host x87 or generic later-x87 helper.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
