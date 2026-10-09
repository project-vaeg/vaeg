# P00 — Baseline, repository seams, and executable gates

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** Stage A handoff, explicit subsequent user authorization, and accepted OCR/source mapping.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Validate the user OCR receipt first, using ocr-handoff.md. Record the actual original and
OCR hashes, source IDs, page mapping, authorization message reference, and unresolved passages.
Do not proceed merely because records contain a self-written approval. Reuse the Stage A
repository identity record, but run the implementation baseline now, not during acquisition.


Inspect the actual HEAD, dirty tree, applicable AGENTS files, existing 8087 work, CPU mode dispatch, memory API, scheduler, model selection, config, reset, savestate, frontend, third-party rules, and CI. Reuse correct existing work. Identify the real V30/uPD9002 path rather than assuming a historical CPU-consolidation milestone is present.

Create the thin gate runner required by verification.md, a package/test registry, and records/repository-audit.md. Record exact file paths, build commands, available local targets, baseline outputs, and existing failures. Map internal package IDs without renumbering unrelated milestones.

## Acceptance

Run the normal local VAEG build and existing applicable tests, retaining exit codes. Prove the runner rejects an unknown package, failed command, and missing required command; future unimplemented gates must remain pending. Record an executable headless integration path or the concrete minimal test seam to add at P07.

Freeze the local required-versus-unavailable target list. The package passes only with a reproducible baseline and real runner behavior, not a printed plan. An independently reproducible preexisting failure may be recorded for delta testing; a new runner failure cannot.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P00
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Repository audit, gate runner and registry, initial evidence ledger, tested-source digest, actual file-level plans for P01-P07.

## Boundaries and failure handling

No arithmetic implementation, broad CPU refactor, invented paths, publication, or deletion of prior work. An overlapping dirty edit is a blocker only when safe reconciliation cannot be made.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
