# M42 — ADR, exhaustive inventory, regression instruments, and baselines

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

Work only on **M42** in this Codex session. Do not begin the next
milestone, do not claim the human gate has passed, and stop after producing the
required report and final SHA. A failed baseline is evidence to investigate, not
permission to re-record a golden, expand a known-gap set, weaken an allowlist, or
change CPU semantics.

## Extracted task specification

Task: docs/agents/tasks/M42_upd9002_adr_inventory_harness.md

Goal:
Establish the evidence and behavior-neutral instrumentation required for the
series. Do not change emulation semantics, state format, runtime dispatch, or
public names.

Prerequisites:
- G41 explicitly accepted.
- pre-upd9002-series exists and points at the accepted G41 SHA.
- Worktree clean.

Steps:
1. Select the next unused ADR number in the actual branch and create
   docs/agents/DECISIONS/ADR-NNNN-upd9002-ownership.md. Refer to it as the
   uPD9002 ADR in all generated tasks/reports; do not assume ADR-0012 is free.
   Record:
   a. upd9002_core_* / upd9002_regs_* ownership.
   b. Final core directory cpu/upd9002/.
   c. Cpu286StateCompat / Upd9002RuntimeState / Cpu286CompatImage model.
   d. Exact state payload compatibility scope per ABI/toolchain.
   e. Runtime table construction retained; immutable final graph plus mutable
      construction provenance.
   f. Removal ownership: i286c_step() in M45; i286c()/v30c() in M46;
      protected-mode inventory/isolation in M47; approved NP2 286
      protected-mode deletion only in M48.
   g. Future upd9002_core_run(cycle_budget) pattern, not implemented here.
   h. Compatibility-mode and missing-instruction non-goals.
   i. CPU_SHUT upper-FLAGS anomaly preserved by fixture.
   j. Public API and active-file rename policy for M49; internal static handler
      identifiers and I286_* helpers may remain.
   k. G41, M42, and M43 baseline/tag policy.
   l. SingleStepTests V20 is an external semantic oracle only for the
      supported uPD9002/V52 intersection; known missing V30/V20 forms
      are target gaps, not failures or implementation requests.
   m. NP2 286 protected-mode code is retained through G47 and may be
      deleted only by the dedicated M48 task after explicit approval.
   n. Supported active presets require the C core. `USE_I286C=off` and the
      `i286x/` implementation are frozen unsupported references; if M42 finds a
      supported preset that selects them, stop and amend this ADR before M43.
   o. The M44 `cpu_type != CPUTYPE_V30` rejection is the sole intentional
      externally visible behavior change in the series and must not be
      generalized.

2. Write docs/agents/reports/m42_upd9002_inventory.md with:
   a. Every exported/public i286c_*, v30c*, CPU_EXEC*, and related core symbol,
      declaration, definition, and active caller.
   b. CPU_TYPE flow: all set, restore, validation, reset, shut, interrupt,
      execution, save, and load sites.
   c. A complete state-member use matrix. For every I286STAT byte range,
      including GDTR, MSW, IDTR, LDTR, LDTRC, TR, TRC, explicit padding,
      cpu_type, and any macro-overlaid field, record reads, direct writes,
      address-taking, range writes, ZeroMemory/memcpy effects, preprocessed
      macro uses, state restore, reset, and shut effects. Do not infer
      removability from a simple textual assignment search.
   d. The accepted state versions/formats at G41 and the actual loader rules.
      Do not infer state provenance merely from a prior milestone number.
   e. Six runtime-built root tables, every patch operation, explicit DIV/IDIV
      replacement, and the recursively reachable secondary dispatch graph.
   f. Current implemented/reachable native instruction handlers and a preliminary
      uPD9002/V52 support/gap map for M43. Missing instructions remain out of
      scope. Do not infer target support merely from V20/V30 family naming.
   g. The NP2 partial 286 protected-mode cluster: files, handlers, descriptor/
      selector/privilege helpers, system opcodes, state fields, direct references,
      and base-table/provenance edges. This is inventory only; no deletion.
   h. All relevant defines and build choices: SINGLESTEPONLY, USE_I286C,
      CPUTYPE_*, CPU_EXEC/CPU_EXECV30, SUPPORT_V30ORIGINAL, and equivalents.
      For every supported preset, record the resolved value and selected step
      function. Prove no supported preset reaches `i286x_step()` or
      `v30x_step()`; otherwise hard-stop the series for an ADR revision.
   i. G41 ABI fixtures: target, compiler, sizeof, alignment, and offsetof values
      for the raw CPU286 payload and the UPD9002 register payload.

3. Implement the fail-closed generator and commit:
   a. tools/qa/golden/upd9002_final_dispatch_graph.csv.
   b. tools/qa/golden/upd9002_dispatch_provenance_m42.csv.
   c. A permanent ctest that regenerates the final graph and requires exact
      equality with its golden.
   d. Generator selftests that fail on incomplete parsing, cardinality mismatch,
      unknown edges, duplicate/missing slots, or non-deterministic provenance.
   e. Keep the M42 provenance file as a historical diagnostic baseline. Compare
      current provenance in milestone reports; do not make permanent equality to
      the M42 provenance a gate after approved M48 protected-mode cleanup.
   The final graph, not provenance, is the immutable M42–M49 baseline.

4. Add canonical --trace-cpu N support and prove:
   a. Two identical trace-enabled runs are byte-identical.
   b. Trace enabled and disabled reach identical final CPU, memory, device,
      cycle, and framebuffer checkpoints.
   c. Event records distinguish cpu, dma, and device origins and preserve
      deterministic event order.

5. Add a direct, test-only instruction harness derived from the final graph and
   provenance. It must cover all currently reachable patched/native paths and
   relevant boundaries without adding missing instructions. Wire deterministic
   results into ctest.

6. Prepare the M43 external-oracle handoff:
   a. Record the exact internal opcode/form support map consumed by the V20
      classifier.
   b. Define a stable direct-harness API for loading initial CPU/RAM state and
      returning final state without exposing machine-global host details.
   c. Do not download, vendor, classify, or baseline the V20 corpus in M42.

7. Add and commit G41/current fixtures for:
   a. Fresh reset.
   b. A fixed instruction-count checkpoint.
   c. The real CPU_SHUT/reset-request path.
   For each, extract CPU286 and UPD9002 payloads and record active CPU/device
   checkpoints. Build G41 in a detached worktree using the same ABI/toolchain.

8. Prove all pre-existing G41/M23 goldens remain byte-identical. Record the new
   trace, instruction-harness, final-graph, provenance, and shutdown baselines.

9. Report the final M42 SHA and stop. Do not create
   pre-upd9002-refactor in M42. M43 must first add and freeze the V20
   external-oracle baseline.

Non-goals:
- No state adapter.
- No CPU_TYPE branch removal.
- No handler removal.
- No public/file rename.
- No instruction/timing fix.
- No SingleStepTests corpus download or result baseline; that is M43.
- No NP2 286 protected-mode deletion.

Gate:
- ADR and inventory approved.
- Fail-closed graph/provenance generator green.
- Existing G41 goldens unchanged.
- Trace determinism and trace-on/off equivalence green.
- Manifest-derived instruction harness green.
- Reset/executed/CPU_SHUT payload fixtures recorded and reproducible.
- Standard human gate green.
- M43 handoff support map and direct-harness API reviewed.
