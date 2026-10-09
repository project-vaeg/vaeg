# M46 — Dispatch normalization and block-executor removal

Derived from `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
(master SHA-256: `07c53e542adbe838de3d79999aa3d7acebf7a15f85f4d17aa0f5a5e50a293938`).

## Authoritative context

Before editing, read and obey:

1. `AGENTS.md`
2. `docs/agents/ROADMAP.md`
3. `docs/agents/CONVENTIONS.md`
4. `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
5. This task file

The master plan is authoritative for shared preconditions, immutable baselines,
state compatibility, dispatch identity, regression traces, SingleStepTests V20
classification, execution protocol, and G42–G49 human-gate requirements. If this
extracted task conflicts with the master plan, stop and report the conflict; do
not improvise.

## Session boundary

Work only on **M46** in this Codex session. Do not begin the next
milestone, do not claim the human gate has passed, and stop after producing the
required report and final SHA. A failed baseline is evidence to investigate, not
permission to re-record a golden, expand a known-gap set, weaken an allowlist, or
change CPU semantics.

## Extracted task specification

Task: docs/agents/tasks/M46_upd9002_dispatch_normalization.md

Goal:
Retain one production construction path, prove final dispatch identity and
post-construction immutability, and remove the dead block execution semantics.

Prerequisites:
- G45 explicitly accepted.
- pre-upd9002-refactor points at accepted G43.

Steps:
1. Construction lifecycle:
   a. Keep v30cinit() as the sole production constructor until M49 renames it.
   b. Invoke it once per production core/process initialization lifecycle.
   c. Test-only scratch construction must not mutate live tables.
   d. Reject or assert on an attempted second mutation of initialized live
      tables.

2. Cross-build identity:
   a. Regenerate upd9002_final_dispatch_graph.csv and require byte identity with
      M42.
   b. Regenerate current construction provenance and report differences; the
      final graph remains the gate.
   c. Do not use a second invocation of the same constructor as equivalence
      evidence.

3. Post-construction immutability:
   a. Snapshot the six runtime-built root function-pointer arrays immediately
      after construction.
   b. At selftest/debug checkpoints compare each entry with ==.
   c. Do not memcmp or hash function-pointer object representations.
   d. Secondary static tables are covered by the source-level final graph and
      ordinary read-only/linker evidence.

4. Remove i286c() and v30c() in this milestone only:
   a. Confirm no active caller under every supported preset.
   b. Delete definitions, declarations, and block-executor-only macros.
   c. These two named removals are pre-approved by this specification after the
      evidence check.
   d. Do not add upd9002_core_run() in this series.
   e. Keep v30c_step() behavior unchanged.

5. Produce the M46 report:
   a. Final-graph diff result.
   b. Pointer-snapshot results for all presets.
   c. Current construction provenance.
   d. Candidate base slots that are overwritten on every final native path.
      These are M47/M48 inputs only, not sufficient deletion evidence.

Gate:
- Final graph byte-identical to M42.
- Root pointer snapshots green.
- No i286c()/v30c() block-executor reference remains in the active tree.
- All trace, harness, state, shutdown, ordinary, and human gates unchanged.
- M43 V20 dataset identity, category/hash sets, known gaps, and failure signatures
  unchanged.
