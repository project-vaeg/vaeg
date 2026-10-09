# P02 — Pinned SoftFloat-3e and backend build validation

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P00.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Verify cached S03/T02 archive hashes against acquisition receipts before unpacking.
Freeze the exact imported release hash in the repository decision record. First retrieval
hashes are observations, not publisher signatures. Never unpack/build during Stage A.


Retrieve the official SoftFloat-3e archive or verify an approved existing copy. Record exact hash/license/version and upstream-file manifest. Build the minimal extF80 operation closure plus only justified auxiliary formats, using private target headers/build glue outside the unchanged vendor tree. Audit the 8086 specialization and portability without treating it as a device model.

Prepare external TestFloat validation against the exact built objects, not a separately installed SoftFloat. Keep all external oracle libraries out of shipping link dependencies.

## Acceptance

Compile/link the actual VAEG target and wrapper smoke target. Run applicable testsoftfloat operation suites and retain counts/versions/commands. Check upstream file hashes and exported/private symbol policy. Prove the wrapper build works with the active compiler's portable integer facilities.

Backend arithmetic results alone do not pass a later 8087 semantic gate. A missing external tool is not permission to claim it ran; repair or identify a reproducible alternate approved validation environment.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P02
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Unchanged vendored closure, dependency lock/notice, private build target, wrapper entry point, TestFloat report and link-dependency audit.

## Boundaries and failure handling

No host x87, native float fallback, copied emulator arithmetic, runtime MPFR/GMP, or edits to upstream files. No wholesale vendoring of an unrelated FPU emulator.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
