# P07 — Production vertical slice

**Initial status:** NOT_STARTED  
**Prerequisites:** P02-P06

## Work

Implement enough production instructions/config to run a self-authored guest through detection, initialization, loads/constants, FADD, store, status checks, and normal exit using the real CPU/bus.

## Acceptance

Absent and present cases pass; same semantic result at 5, 10, and custom MHz while isolated NDP timing scales; requested clock changes wait for reset; repeated resets keep one device maximum; full baseline is rerun.

Run:

```sh
python3 tools/8087/gate.py --package P07
```

## Required outputs

Integrated guest slice, redistributable guest bytes/source, traces and baseline report.

## Boundaries

No direct-handler demo or detection-only completion.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
