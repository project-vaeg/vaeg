# P19 — Final QA and zero omissions

**Initial status:** NOT_STARTED  
**Prerequisites:** P17, P18

## Work

Run fresh full local gates, baseline, production guest tests, zero-omission/decoder/handler/test coverage, provenance/link/private-asset audits, generated documentation freshness, and final report.

## Acceptance

Every documented form is COMPLETE_TESTED; 2048 slot counters have zero unclassified/duplicate entries; missing handler/test/fallback/stale/skip counters are zero; production route and configuration/timing gates pass; final report matches tested source digest.

Run:

```sh
python3 tools/8087/gate.py --package P19
```

## Required outputs

Final machine-readable and Markdown reports, generated coverage docs, exact remaining evidence axes and resume state.

## Boundaries

No push/merge, fabricated CI/hardware status, or calling a blocked/partial build complete.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
