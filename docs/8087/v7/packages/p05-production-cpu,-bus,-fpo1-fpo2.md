# P05 — Production CPU, bus, FPO1/FPO2

**Initial status:** NOT_STARTED  
**Prerequisites:** P01, P03, P04

## Work

Connect production native decode to the device seam, preserve ModR/M/displacement/segment/EA behavior, latch applicable first-word reads, implement absent-device semantics, and preserve FPO2 as CPU-side behavior with no attached device. Keep 8080 path separate.

## Acceptance

All 2048 slots execute decode classification; addressing/displacement/segment cases pass; MMIO counters prove no duplicate first read or phantom store; absent detection and FPO2 behavior are correct; representative 8080 overlapping bytes invoke no 8087 callback.

Run:

```sh
python3 tools/8087/gate.py --package P05
```

## Required outputs

CPU hooks, bus contract, exhaustive decode/MMIO/FPO2/8080 tests.

## Boundaries

No test-only decoder replacing production decode; no FPO2 attachment.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
