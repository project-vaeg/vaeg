# P02 — SoftFloat 3e vendoring and backend validation

**Initial status:** NOT_STARTED  
**Prerequisites:** P00

## Work

Verify `/Users/maho/work/vaeg-8087-evidence-v6/upstream/S03/SoftFloat-3e.zip`, recorded hash, and license. Vendor the minimum required official source closure unchanged. Add VAEG build glue/wrapper outside upstream files. Acquire/build TestFloat externally if needed for validation of the exact built SoftFloat objects.

## Acceptance

VAEG and backend smoke targets build; upstream files hash against S03 archive; applicable TestFloat/testsoftfloat checks pass with recorded counts; link audit shows no runtime MPFR/GMP/TestFloat/libm/host-x87 dependency.

Run:

```sh
python3 tools/8087/gate.py --package P02
```

## Required outputs

Vendored upstream closure, notices/lock data, wrapper smoke tests, backend validation report.

## Boundaries

No upstream edits, host-FP fallback, or treating SoftFloat as an 8087 device model.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
