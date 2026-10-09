# P06 — Early exception transaction and WAIT/FN recovery framework

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P01, P03, P04, P05.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Implement the shared effect proposal/commit framework for masks, sticky flags, IR/B/INT, stack invalid cases, and pointer/condition effects. Define per-exception result-delivery policy rather than one universal rollback rule. Add minimum FNINIT/FNCLEX/FNDISI/FNENI/status/control behavior required for initialization/recovery.

Connect POLL to the real CPU wait/interrupt-eligibility seam using a test-fixture route until production VA evidence is integrated at P17. Mark that route TEST_FIXTURE_ONLY.

## Acceptance

Use source-authored fixtures for masked and unmasked invalid/divide-by-zero plus framework cases for other classes. Test no premature destructive pop/store, correct sticky state, IEM versus individual masks, and clearing/deassertion behavior. Verify WAIT-prefixed byte streams differ from FN sequences and recovery can execute as specified.

A pending condition must not be auto-cleared to stop a wait. The emulator must remain responsive to scheduling/pause/reset. A host callback count is not yet the production route acceptance.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P06
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Exception-effect framework, wait/pending state machine, essential administrative handlers, recovery fixtures and explicit route evidence status.

## Boundaries and failure handling

Do not postpone all exception support until after bulk arithmetic. Do not emulate every error as later-x87 SF/#MF or suppress every unmasked result identically.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
