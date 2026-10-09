# P12 — FPREM, FSCALE and FXTRACT

**Initial status:** NOT_STARTED  
**Prerequisites:** P09-P11

## Work

Implement 8087 partial FPREM including repeated reduction/C2/legacy quotient conditions, plus source-restricted FSCALE and FXTRACT.

## Acceptance

Repeated guest loops converge with bounded progress; incomplete versus complete FPREM conditions, quotient/sign cases, scale boundaries, and two-result FXTRACT stack order/special cases pass.

Run:

```sh
python3 tools/8087/gate.py --package P12
```

## Required outputs

Three complete handlers and independent fixtures.

## Boundaries

No FPREM1 or IEEE remainder substitution.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
