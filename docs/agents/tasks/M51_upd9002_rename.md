# M51 — Pure renames, moves, API cleanup, and repository guards

Derived from `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
(v7 M47–M51 sequence amendment).

## Authoritative context

Before editing, read and obey:

1. `AGENTS.md`
2. `docs/agents/ROADMAP.md`
3. `docs/agents/CONVENTIONS.md`
4. `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
5. This task file

The master plan is authoritative for shared preconditions, immutable baselines,
state compatibility, dispatch identity, regression traces, SingleStepTests V20
classification, execution protocol, and G42–G51 human-gate requirements. If this
extracted task conflicts with the master plan, stop and report the conflict; do
not improvise.

## Session boundary

Work only on **M51** in this Codex session. Do not begin the next
milestone, do not claim the human gate has passed, and stop after producing the
required report and final SHA. A failed baseline is evidence to investigate, not
permission to re-record a golden, expand a known-gap set, weaken an allowlist, or
change CPU semantics.

## Extracted task specification

Task: docs/agents/tasks/M51_upd9002_rename.md

## Maintainer-resolved M51 scope

The accepted pre-implementation audit found ambiguity between the original
task wording and the required graph-preserving rename. The maintainer resolved
it before implementation as follows; this section is authoritative for M51:

* Use the exact public API spellings in the mapping below, including
  `upd9002_core_set_ext_size`, `upd9002_core_set_emm`, and
  `upd9002_dispatch_initialize`.
* Move `i286c/` to `cpu/upd9002/`, but rename only `i286c.c`, `v30patch.c`,
  and `v30patch.h` within that directory. Historical internal basenames such
  as `i286c_8x.c`, `i286c_mn.c`, and `cpucore.h` remain unchanged.
* Move `iova/upd9002.c/.h` to `iova/upd9002_regs.c/.h` and move
  `cpuxva/memoryva.h` to `cpucva/memoryva.h`.
* Preserve `cpuxva/memoryva.x86` byte-identically as a frozen reference.
* Preserve exactly these 18 internal historical REP-helper names:

  ```text
  i286c_rep_insb       i286c_rep_insw       i286c_rep_outsb
  i286c_rep_outsw      i286c_rep_movsb       i286c_rep_movsw
  i286c_rep_lodsb      i286c_rep_lodsw       i286c_rep_stosb
  i286c_rep_stosw      i286c_repe_cmpsb      i286c_repne_cmpsb
  i286c_repe_cmpsw     i286c_repne_cmpsw     i286c_repe_scasb
  i286c_repne_scasb    i286c_repe_scasw      i286c_repne_scasw
  ```

The first 17 names are part of accepted graph/provenance evidence.
`i286c_rep_outsw` remains with the same cross-translation-unit REP-helper
family to avoid asymmetric mechanical churn. These are internal exceptions,
not public APIs. This conservative scope supersedes step 1.b's provisional
instruction to rename every additional `i286c_*` basename.

Goal:
Make active names reflect the consolidated uPD9002 design. M51 contains no
semantic, timing, state-layout, dispatch, or dead-code change.

Prerequisite:
- G50 explicitly accepted.
- All semantic/state work and the dedicated M50 protected-mode
  deletion milestone are complete.

Commit discipline:
- Each move group starts with a git-mv-only commit.
- Reference/build/include fixes follow immediately in a separate commit.
- Add qualifying mechanical commit hashes to .git-blame-ignore-revs.
- Do not hide cleanup or behavior fixes in a rename fixup.

Steps:
1. Instruction-core directory and files:
   a. Move the surviving active core content to cpu/upd9002/.
   b. Rename every surviving active source/header basename whose public identity
      is i286c or v30patch according to an explicit mapping in the uPD9002 ADR.
      At minimum:
         i286c.c       -> upd9002_core.c
         v30patch.c    -> upd9002_dispatch.c
      Rename additional surviving i286c_* files mechanically; do not leave an active file/path identity suggesting an 80286 core.
      Confirm the M50 protected-mode implementation is not merely moved under the
      uPD9002 directory.
   c. Internal static handler names and I286_* helper/opcode macros may remain
      when explicitly permitted by the ADR. Do not rename them merely for
      cosmetics in this series.

2. Rename every surviving public CPU API, not only reset/step. Expected mapping
   includes, subject to the M42 public-symbol inventory:
      i286c_initialize    -> upd9002_core_initialize
      i286c_deinitialize  -> upd9002_core_deinitialize
      i286c_reset         -> upd9002_core_reset
      i286c_shut          -> upd9002_core_shut
      i286c_setextsize    -> upd9002_core_set_ext_size
      i286c_setemm        -> upd9002_core_set_emm
      i286c_interrupt     -> upd9002_core_interrupt
      v30c_step           -> upd9002_core_step
      v30cinit            -> upd9002_dispatch_initialize
   Rename any additional externally visible survivor found by M42. After M51,
   no active external declaration or definition uses an i286c_* or v30c* public
   name.

3. Built-in 0xFFF0 register model:
   a. Move iova/upd9002.c/.h to iova/upd9002_regs.c/.h.
   b. Rename public reset/bind APIs to upd9002_regs_reset/bind.
   c. Rename the generic global object upd9002 to upd9002_regs if it is visible
      outside the implementation, and rename the state type when needed to make
      ownership unambiguous.
   d. Preserve exact object layout and the literal on-disk tag "UPD9002".

4. VA memory header:
   a. Move cpuxva/memoryva.h to cpucva/memoryva.h.
   b. Remove cpuxva/ from active include paths.
   c. Leave cpuxva/memoryva.x86 untouched.

5. Final graph and state stability:
   a. The final dispatch graph excludes file paths and internal handler names
      are not cosmetically renamed, so it must remain byte-identical to M42.
      Graph identity takes priority over cosmetic naming. If the M42 inventory
      finds a surviving symbol that is both part of immutable graph identity and
      matched by an old public-name cleanup pattern, do not rename it in M51;
      record an explicit ADR exception, scope the repository guard accordingly,
      and defer that rename to a separately baselined milestone.
   b. CPU286/UPD9002 layout and cross-version payload fixtures must remain
      identical.
   c. Rename-only commits must show only path movement; fixup commits must not
      alter generated baselines.

6. Documentation and guards:
   a. Update AGENTS.md, BUILD.md, ROADMAP, active task/report references, CMake
      source lists, and include paths.
   b. Add a focused repository guard that rejects active i286c/ paths, active
      i286c_* or v30c* public declarations, obsolete CMake entries, and generic
      upd9002 register globals. Exempt frozen reference paths, historical docs,
      and explicitly retained internal static handler identifiers/golden names.
      Do not use an indiscriminate token ban that would reject the immutable
      dispatch baseline.
   c. Remove or clearly archive stale provisional M36–M41 task drafts so they
      cannot be executed accidentally.

Gate:
- Rename detection is clear; no semantic change in rename-only commits.
- No active external i286c_* or v30c* public API remains, except an explicit
  ADR exception required to preserve immutable dispatch-graph identity; any such
  exception is internal-only, documented, guard-scoped, and deferred.
- No active i286c/ source path or cpuxva include path remains.
- upd9002_core_* and upd9002_regs_* ownership is unambiguous.
- G41 checkpoints/payloads and M42 traces/harness/final graph/shutdown fixture and M43 V20
  dataset/category/gap/failure signatures remain byte-identical.
- All supported CI presets and standard human gate green.
