# M44 — Serialized/runtime state boundary and transactional loading

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

Work only on **M44** in this Codex session. Do not begin the next
milestone, do not claim the human gate has passed, and stop after producing the
required report and final SHA. A failed baseline is evidence to investigate, not
permission to re-record a golden, expand a known-gap set, weaken an allowlist, or
change CPU semantics.

## Extracted task specification

Task: docs/agents/tasks/M44_upd9002_state_boundary.md

Goal:
Create the state boundary before runtime dispatch is folded. Preserve every
accepted G41 payload byte and the historical reset/shut transformation of those
bytes. Introduce the series' sole intentional externally visible behavior
change: reject a serialized CPU286 payload whose `cpu_type` is not
`CPUTYPE_V30` before the machine can resume.

Prerequisites:
- G43 explicitly accepted.
- pre-upd9002-refactor exists and points at the accepted M43 SHA.

Steps:
1. Define Cpu286StateCompat as the exact current ABI-specific CPU286 payload
   layout. Add compile-time/runtime tests for M42-recorded size, alignment, and
   relevant offsets. This type is a serialization contract, not runtime state.

2. Define Upd9002RuntimeState. Initially it may retain every field still needed
   by the current implementation; M44 creates the boundary but does not force
   field removal. Every byte range read, written, or address-taken by active
   instruction execution, reset, interrupt, DMA-coupled CPU work, or CPU_SHUT
   must be represented in this runtime state before the core is denied access to
   compatibility storage.

3. Define Cpu286CompatImage as an opaque complete CPU286 payload owned only by
   the serialization adapter. The core must have no read access to inactive
   compatibility bytes.

4. Implement import/export with the existing custom statsave callback mechanism
   or a dedicated CPU-specific callback:
   a. Read into temporary Cpu286StateCompat/Cpu286CompatImage storage.
   b. Validate payload size, cpu_type == CPUTYPE_V30, and existing loader
      invariants before committing runtime state. Document that the cpu_type
      rejection is new defensive behavior relative to the raw G41 loader and is
      the only intentional external semantic change in M42–M49.
   c. Construct a temporary Upd9002RuntimeState.
   d. Commit runtime state and compatibility image atomically.
   e. Immediate export starts from the compatibility image and overlays active
      runtime fields.
   f. Do not repurpose an unrelated STATFLAG mode merely because it has a
      callback; implement the smallest explicit CPU-state handler consistent
      with the framework.

5. Make failure all-or-nothing for the machine:
   a. Prefer preflight validation of CPU286 before applying live sections.
   b. If the framework cannot preflight, stage the load or provide complete
      rollback/reset behavior.
   c. Add a test proving invalid cpu_type and malformed CPU286 size cannot leave
      a partially loaded machine that resumes execution.

6. Synchronize the compatibility image on lifecycle operations:
   a. Fresh initialization/reset produces bytes identical to the G41 reset
      payload after active fields are overlaid.
   b. CPU_SHUT applies the same historical byte-range clear/replace semantics as
      G41 before active fields are overlaid.
   c. Immediate imported-state save preserves opaque bytes and padding exactly.
   d. Add a test with non-canonical opaque bytes proving immediate round-trip
      preservation and proving reset/shut transform them exactly as G41 would.

7. Replace the raw CPU286 STATFLAG_BIN path with the explicit adapter while
   retaining the exact CPU286 tag and payload size. The UPD9002 register section
   remains byte-identical.

8. Cross-version proof using detached worktrees and identical ABI/toolchain:
   a. G41 reset, fixed-execution, and CPU_SHUT states load current and immediately
      re-export byte-identical CPU286 and UPD9002 payloads.
   b. Current equivalents load in the G41 build.
   c. Invalid cpu_type is rejected by current before resume.
   d. Compare extracted payloads, not only whole-file hashes or vaguely defined
      stable sections.

9. Keep runtime dispatch behavior unchanged in M44. CPU_TYPE may still exist as
   a temporary runtime field, but every imported value is now validated as V30.

Gate:
- All accepted old/new payload tests pass in both directions within the recorded
  ABI; a non-V30 cpu_type fixture is deterministically rejected before resume as
  the sole approved behavior change.
- Rejection is all-or-nothing.
- Opaque-byte immediate round-trip and reset/shut transformation tests pass.
- G41 checkpoints and M42 traces/harness/final graph/shutdown fixture and M43 V20
  dataset/category/failure baselines unchanged.
- Standard human gate green.
