# P06 — Exception transaction and WAIT/FN framework

**Initial status:** NOT_STARTED  
**Prerequisites:** P01, P03, P04, P05

## Work

Implement staged effect proposal/commit, masks/sticky flags/IEM/IR/B/INT, stack invalid cases, pointer/condition effects, and minimum FNINIT/FNCLEX/FNDISI/FNENI/control/status recovery. Connect POLL to a marked test route until P17.

## Acceptance

Masked/unmasked representative exceptions, pending state, no premature destructive pop/store, WAIT-prefixed versus FN byte streams, and recovery semantics pass. Pending state is never silently cleared to escape a wait.

Run:

```sh
python3 tools/8087/gate.py --package P06
```

## Required outputs

Exception framework, pending/wait state machine, recovery fixtures.

## Boundaries

No universal later-x87 SF/#MF model and no blanket unmasked rollback rule.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
