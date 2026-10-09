# M59 — Produce the uPD9002/V20 semantic evidence pack

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and all reports from prerequisite gates.
5. Run `git status --short`; the tracked worktree must be clean.
6. Record the exact starting branch and SHA.
7. Resolve and verify the exact approved predecessor gate SHA. Do not infer it from the current
   worktree.
8. Work on this milestone only and stop at its gate.

All newly authored source, comments, identifiers, commit messages, test names, and repository
Documentation must be in English.

## Predecessor and identifiers

Prerequisite: G58 explicitly approved.

Branch: `topic/m59-upd9002-evidence-pack`

Commit prefix: `M59:`

Gate: `G59`

## Goal

Produce machine-readable and human-readable evidence for the first semantic corrections. Make no
production core semantic change.

## Universal evidence requirements

For every item:

1. Provide at least one readable representative dump.
2. Analyze the full relevant population, or a deterministic justified stratified subset.
3. Produce a machine-readable table keyed by case hash.
4. Label conclusions `proven`, `hypothesis`, or `underdetermined`.
5. Verify top-level classification and executed count before calling any form green.

Each evidence row contains, where applicable:

- initial architectural state;
- expected SST final state;
- actual emulator final state;
- expected and actual termination;
- expected and actual registers;
- expected and actual RAM bytes;
- mismatch kinds;
- logical and physical addresses used in derived operand or frame mappings.

An expected-only table is not a sufficient defect diagnosis.

## Item 1 — Guest-visible FLAGS materialization

Analyze CC and corroborate with CD and CE. Aggregate at least:

- initial FLAGS, SS, and SP;
- expected and actual final SP;
- expected and actual frame addresses;
- expected and actual saved IP, CS, and FLAGS;
- expected and actual final FLAGS;
- instruction bytes and termination.

Prove frame mapping from observed state without assuming an Intel frame layout. Aggregate frames
crossing a 64 KiB segment boundary or 20-bit physical boundary as separate classes.

For every pushed FLAGS bit classify the observed rule as:

- `copied`
- `forced-zero`
- `forced-one`
- `condition-dependent`
- `undetermined`

Also verify classification, selected count, and executed count for 9C PUSHF, 9F LAHF, 9D POPF,
and 9E SAHF.

- Derive PUSHF stack image independently.
- Derive LAHF AH image independently.
- Derive POPF and SAHF loadable, preserved, and forced bits.
- Test whether POPF/SAHF and interrupt delivery share a defective primitive; do not assume they do.

## Item 2 — Canary cluster

Analyze F7.2 NOT and C6/C7 MOV immediate failures. Determine whether causes include:

- effective-address calculation;
- 16-bit segment wrapping or 20-bit physical wrapping;
- displacement/immediate fetch ordering;
- ModR/M group dispatch;
- unintended FLAGS clobbering;
- another shared primitive.

Partition by register vs memory form, address mode, displacement width, wrap condition, and mismatch
class.

## Item 3 — D4/D5

Verify D4 and D5 classifications and executed counts. Stratify immediate at minimum:

`0, 1, 2, 9, 10, 11, 16, 255`.

Determine expected and actual behavior, including AAM 0 termination. Do not propose a D5 change
unless executed SST evidence demonstrates a D5 mismatch.

## Item 4 — 0F28 and 0F2A

M43 classifies all 5,000 0F28 records as `known_target_gap`; 0F28 is not a passing reference.

Determine:

- whether the intended uPD9002/V52 target supports 0F28;
- its correct `gap_kind`;
- SST-observed 0F28 and 0F2A semantics;
- whether 0F2A failures are local, shared-primitive, or conceptual.

Do not reclassify or implement 0F28 in M59.

## Item 5 — Shift forms

Before comparison, verify that rotate subforms .0-.3 are applicable and executed.

For C0/C1/D2/D3 .4-.7 stratify:

- width 8/16;
- count 0, 1, 2, width-1, width, width+1, 31, 32, 33, 255;
- sign bit 0/1;
- initial CF 0/1.

Extract expected vs actual OF, AF, CF, and count-zero behavior. Do not assume lack or presence of
`& 0x1f` masking.

## Item 6 — FF.7 and BOUND

Partition failures into:

- normal completion;
- interrupt;
- stack-frame mismatch;
- range-result mismatch;
- other register/RAM mismatch.

Determine SST-observed FF /7 behavior without assuming 286 #UD. Quantify how much of BOUND's
failure population is solely explained by the INT 5 frame.

## Deliverables and gate G59

- Evidence tables are deterministic and content-addressed.
- All analyses reproduce from the pinned corpus and approved G58 epoch.
- No production CPU semantic file changes.
- Report includes explicit recommended re-ranking of M60 onward.

Write `docs/agents/reports/m59_upd9002_semantics_evidence_pack.md` and stop.
