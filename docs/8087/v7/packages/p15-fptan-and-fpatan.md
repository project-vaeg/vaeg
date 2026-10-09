# P15 — FPTAN and FPATAN

**Initial status:** NOT_STARTED  
**Prerequisites:** P14

## Work

Implement original restricted-domain partial tangent/arctangent semantics, actual operand/result order, FPTAN result-pair policy, stack effects, exception behavior, and timing.

## Acceptance

Certified corpora and targeted domain tests pass; both FPTAN components and their ordering/reconstruction are checked; FPATAN operand swapping fails independent fixtures; guest traces are deterministic.

Run:

```sh
python3 tools/8087/gate.py --package P15
```

## Required outputs

Two complete production handlers and pair-policy evidence.

## Boundaries

No later tan-plus-1 or unrestricted atan2 substitution.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
