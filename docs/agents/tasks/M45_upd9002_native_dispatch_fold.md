# M45 — Native-mode invariant and per-instruction dispatch fold

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

Work only on **M45** in this Codex session. Do not begin the next
milestone, do not claim the human gate has passed, and stop after producing the
required report and final SHA. A failed baseline is evidence to investigate, not
permission to re-record a golden, expand a known-gap set, weaken an allowlist, or
change CPU semantics.

## Extracted task specification

Task: docs/agents/tasks/M45_upd9002_native_dispatch_fold.md

Goal:
Make native V30-compatible execution an unconditional runtime invariant, then
remove the per-instruction 286/V30 selection and i286c_step().

Prerequisites:
- G44 explicitly accepted.
- pre-upd9002-refactor points at accepted G43.

Steps:
1. Reset and initialization:
   a. Remove CPU_TYPE-based initializer selection.
   b. Invoke the existing native V30/uPD9002 initializer unconditionally.
   c. Verify every normal reset, reset-request continuation, and
      interrupt-initialization path cannot select 80286 instruction execution.
      The real CPU_SHUT path is the explicit ADR exception described in step 2:
      it preserves the historical 286-style initializer output but does not
      select an 80286 dispatcher.

2. Shutdown — sole initializer-level exception to the native invariant:
   a. Preserve the current i286c_shut() -> 286-style init behavior exactly.
   b. Link the implementation comment to the uPD9002 ADR and state explicitly
      that this is a regression-preservation exception, not an available 80286
      execution mode.
   c. Require the M42 CPU_SHUT trace, active-state checkpoint, and CPU286 payload
      to remain byte-identical.
   d. Do not “fix” the upper-FLAGS anomaly or replace the initializer in M45.

3. Execution and active-core selection:
   a. First prove from the M42 preset matrix that every supported active preset
      selects the C core. Declare `USE_I286C=off` and the i286x/v30x execution
      path unsupported frozen reference configurations in the ADR and build
      documentation. If any supported preset selects that path, stop rather than
      removing it.
   b. Remove the active `USE_I286C`/i286x-vs-i286c selector branch from
      pccore.c and any active build/header surface, while leaving `i286x/`
      untouched as frozen reference material.
   c. Remove the per-instruction CPU_TYPE branch and call v30c_step() directly.
   d. Remove i286c_step(), its declaration, and step-selector-only macros and
      call sites. This named removal is pre-approved by this specification after
      evidence confirms no remaining caller.
   e. Keep i286c() and v30c() block executors until M46; M45 must not delete
      them.
   f. Preserve v30c_step() O_FLAG, V30_DMAP, cycle, exception, and event order
      exactly.

4. State/control separation:
   a. The serialized cpu_type byte remains in Cpu286StateCompat and is exported
      as CPUTYPE_V30.
   b. Only the M44 importer validates it.
   c. No execution, normal reset, interrupt, scheduler, or state-resume path
      branches on it after this milestone. CPU_SHUT remains only the explicit
      fixed-output initializer anomaly and does not branch on cpu_type or enable
      80286 dispatch.
   d. Remove the runtime cpu_type field if and only if no compatibility adapter
      or temporary code still requires it; otherwise mark it for M47 and prove
      it is not read as control.

5. Acceptance invariant:
   "No active execution, normal reset, interrupt, scheduler, or state-resume
   path selects an 80286 instruction mode. The legacy cpu_type byte is validated
   as CPUTYPE_V30 only by the state adapter and never controls runtime dispatch.
   CPU_SHUT is the sole documented initializer-level exception: it preserves the
   historical 286-style register-init result required by the M42 fixture, but it
   does not select an 80286 opcode dispatcher or expose an 80286 execution
   mode."

Gate:
- Acceptance invariant, including the explicit CPU_SHUT exception, demonstrated
  by static reference map and tests.
- Every supported preset uses the C core; no active supported build surface can
  select i286x_step()/v30x_step() or `USE_I286C=off`.
- i286c_step has no declaration, definition, or active caller.
- G41 checkpoints and state payloads unchanged.
- M42 traces, instruction harness, final graph, and shutdown fixture
  unchanged.
- M43 V20 dataset identity, category/hash sets, known gaps, and every failure
  signature unchanged.
- Standard human gate green.
