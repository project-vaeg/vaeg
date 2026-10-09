# P01 — Independent 8087 source inventory and core contracts

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

Read accepted I02/I01 and cross-check I03 (E8087/interface library manual) and I04
(ASM86 encoding conventions). Separate full E8087 from PE8087 and high-level library
functions. Validate any WAIT/NOP bytes as assembler conventions rather than extra FPU
opcodes. An OCR token is never authoritative over the corresponding original page.
Record source disagreements and OCR uncertainty before freezing the independent inventory.


Acquire available original Intel/NEC evidence and inspect existing docs/88va machine descriptions. Establish the complete documented form inventory independently of a production decoder. Create cited opcode fixtures, source availability records, and a compact production-description schema whose incomplete states are honest.

Resolve key CPU applicability, environment format, basic reset state, and exception framework rules needed by P03-P07. Record detailed unresolved instruction corners with bounded follow-up work; do not block all basic implementation on future silicon traces.

## Acceptance

Check the minimum family inventory against original sources and expand the decoder-slot classification to 2048 entries. Validate independent selected bytes, directions, and memory widths. Test duplicate/unclassified/reclassified fixture rejection without modifying production source.

No documented form may be silently assigned reserved/unknown to avoid implementation. P01 may contain PENDING handlers; the final checker must distinguish that from COMPLETE_TESTED. Exact source page/revision evidence must exist for frozen normative fixtures.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P01
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

Independent required-form inventory, production schema/classification source, early byte fixtures, source/conflict ledger, initial coverage checker.

## Boundaries and failure handling

Do not copy large manual tables or private pages. Do not derive every expected result from the same production table. Missing source evidence is recorded precisely; use available primary equivalents before requesting user input.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
