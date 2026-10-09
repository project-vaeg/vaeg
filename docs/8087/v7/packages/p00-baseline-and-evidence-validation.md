# P00 — Baseline and evidence validation

**Initial status:** NOT_STARTED  
**Prerequisites:** None

## Work

Inspect current HEAD, dirty tree, AGENTS files, CPU/memory/scheduler/config/savestate/frontend seams, CI, and existing 8087 work. Validate every expected PDF/OCR pair under `/Users/maho/work/vaeg-8087-evidence-v6` and the S03 SoftFloat evidence files. Create the actual gate runner/registry and repository/evidence audit records.

## Acceptance

Normal local VAEG build/tests are reproducible; preexisting failures are recorded. OCR/PDF title/order/revision mappings are verified, N01 applicability is recorded, and critical table spot-checks do not reveal unresolved OCR corruption. Gate runner rejects unknown IDs, zero tests, and failed commands.

Run:

```sh
python3 tools/8087/gate.py --package P00
```

## Required outputs

Repository audit, evidence ledger, baseline report, gate runner/registry.

## Boundaries

No arithmetic implementation yet; no OCR rerun; no modification of external evidence.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
