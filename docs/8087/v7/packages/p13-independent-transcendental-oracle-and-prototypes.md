# P13 — Independent transcendental oracle and prototypes

**Initial status:** NOT_STARTED  
**Prerequisites:** P01, P02, P03, P09

## Work

Build external MPFR/GMP or exact-integer interval tooling, freeze domains/order/constants/corpora, and prototype bounded exp/log/trig kernels without touching shipping runtime dependencies.

## Acceptance

Directed interval certificates, exact raw input conversion, difficult boundaries, insufficient-certificate rejection, reproducible corpus hashes, and viable measured error bounds pass.

Run:

```sh
python3 tools/8087/gate.py --package P13
```

## Required outputs

Oracle generator, certified corpora, constants and numerical design record.

## Boundaries

No runtime oracle linkage, host-double detours, or tolerance weakening.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
