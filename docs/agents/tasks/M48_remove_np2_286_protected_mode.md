# M48 — Remove the explicitly approved NP2 286 protected-mode implementation

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

Work only on **M48** in this Codex session. Do not begin the next
milestone, do not claim the human gate has passed, and stop after producing the
required report and final SHA. A failed baseline is evidence to investigate, not
permission to re-record a golden, expand a known-gap set, weaken an allowlist, or
change CPU semantics.

## Extracted task specification

Task: docs/agents/tasks/M48_remove_np2_286_protected_mode.md

Goal:
Remove only the NP2 partial 80286 protected-mode implementation explicitly
approved at G47. Preserve the active uPD9002/V52 native instruction semantics,
final dispatch graph, state payload ABI, SingleStepTests V20 result baseline, and
CPU_SHUT behavior.

Prerequisites:
- G47 explicitly accepted.
- The G47 decision names the exact dependency-closed deletion groups approved for
  M48.
- No source identity, final graph, dataset identity, or evidence input has drifted
  since the accepted M47 report. Drift is a hard stop and requires re-approval.

Steps:
1. Revalidate the approved list before editing:
   a. Re-run graph, direct-reference, macro-expansion, callback, state-use,
      compiler cross-reference, function-section, and linker-map evidence.
   b. Confirm every candidate remains inside the M47 protected-mode cluster.
   c. Remove any ambiguous or newly shared candidate from the working list and
      report the deferral; never broaden the list automatically.

2. Perform any approved shared/protected source split first:
   a. Move or retain shared real-mode/V30 helpers without semantic change.
   b. Keep git history clear and avoid wholesale rewrites.
   c. Gate after each split before deleting protected code.

3. Redirect only approved dead base-table/provenance entries:
   a. Redirect slots only when every final native path overwrites or bypasses the
      protected entry.
   b. After each dependency group, regenerate the M42 final dispatch graph and
      require byte identity.
   c. Record and explain construction-provenance changes.
   d. Root function-pointer snapshots remain element-wise equal.

4. Delete only the approved protected-mode implementation:
   a. Candidate examples include 80286 system-opcode handlers, descriptor-table
      operations, selector/privilege/presence checks, protected interrupt/return
      branches, and protected-only helpers, but the accepted M47 list is the sole
      authority.
   b. Do not delete shared real-mode/V30 execution, Type-0 divide delivery,
      interrupt mechanics, memory helpers, the 0xFFF0 register model, or the
      CPU_SHUT initializer unless the exact approved evidence classifies a portion
      as protected-only.
   c. Use small dependency-group commits and gate after each group.

5. Reduce Upd9002RuntimeState only for fields proven protected-only:
   a. Remove a field only when the M47 audit and post-deletion build prove no
      active reader/writer/address-taking use remains.
   b. Preserve the exact serialized byte positions in Cpu286StateCompat and
      Cpu286CompatImage.
   c. Import/export/reset/shut overlays retain the M44 historical lifecycle
      behavior. Runtime removal never authorizes zeroing opaque bytes.
   d. Add a guard preventing instruction code from accessing serialization-only
      protected-mode residue.

6. Sweep protected-mode build/config residue only after implementation deletion:
   a. Protected-only defines and declarations.
   b. CMake sources and includes.
   c. Any remaining USE_I286C/i286x-related residue not already removed from
      the supported active selection path in M45, and only when the accepted
      evidence proves it is exclusively tied to the deleted protected cluster.
   d. Keep the frozen i286x/ reference tier and cpuxva/memoryva.x86 untouched.

7. Re-run the complete behavior-preservation matrix after every dependency group
   and at the final tip:
   a. G41/M23 checkpoints and CPU286/UPD9002 payload compatibility.
   b. M42 trace, direct instruction harness, immutable final graph, pointer
      snapshots, and CPU_SHUT fixture.
   c. M43 V20 CI and full profiles with exact dataset identity, category/hash
      sets, known gaps, pass/failure sets, failure signatures, and termination
      classes.
   d. linux-ci-asan, all supported presets, linker map, and the standard human
      gate.

8. Report:
   a. Approved vs deleted vs deferred groups.
   b. LOC/files/functions/fields removed.
   c. Evidence and gate result for each dependency group.
   d. Final construction-provenance diff and unchanged final graph proof.
   e. Remaining internal I286_* names that are shared implementation details and
      will be handled only mechanically, if at all, by M49.

Non-goals:
- No uPD9002/V52 instruction implementation or bug fix.
- No change to the M43 known-gap or expected-divergence sets.
- No timing/prefetch/MT work.
- No public/file rename; that is M49.

Gate:
- Every deletion was explicitly approved at G47 and revalidated.
- No protected-mode candidate was inferred from its name alone.
- M42 final graph and all behavior fixtures unchanged.
- M43 V20 dataset/category/gap/failure signatures exactly unchanged.
- CPU286/UPD9002 tags, sizes, offsets, and payload lifecycle unchanged.
- ASan, linker evidence, all supported presets, and human gate green.
