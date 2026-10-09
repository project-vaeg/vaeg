# vaeg uPD9002 core consolidation series (M42–M51) — v7 sequence amendment

Seventh revision.  The filename is retained so accepted M42–M46 links remain
stable.  This amendment supersedes the v6 M47–M49 sequence and defines ten task
files M42–M51.

Designation: **μPD9002** in prose and **uPD9002** in ASCII identifiers and file
names.

## What v7 changes after the accepted M47 audit

1. The accepted M42 graph routes v30op_repe[0x0f] and
   v30op_repne[0x0f] to i286c_cts; the protected-mode cluster is not dormant.
2. M47 is now a behavior-neutral correctness-evidence milestone for REP+0F and
   protected-state import policy.
3. M48 may implement only the semantic rule, state policy, dispatch/state
   edits, and baseline transition explicitly approved at G47.
4. The old inventory, deletion, and rename milestones move to M49, M50, and
   M51.  M49 must inventory actual post-M48 reachability and may conclude that
   no safe M50 deletion group exists.
5. Correcting REP+0F may intentionally change explicitly approved M42 graph
   rows and M43 records.  Historical M42/M43 artifacts remain immutable; M48
   records a separately reviewed transition rather than hiding the change.

The v6 non-reachability assumption and its old M47 gate are invalid and must not
be executed.

Position: start only after the maintainer has explicitly accepted G41. Finish
before any multithreading, CPU timing re-derivation, performance optimization,
or μPD9002 compatibility-mode implementation.

## What v6 changes from v5

1. **The CPU_SHUT anomaly is now an explicit exception to the native-mode
   invariant.** CPU_SHUT preserves the existing 286-style initializer behavior
   for regression compatibility, but it never enables 80286 instruction
   dispatch. Codex must not “repair” this exception during M45.
2. **The active `USE_I286C`/`i286x` selection policy is fixed.** All supported
   active presets use the C core. `USE_I286C=off` and the `i286x/` assembly core
   are frozen, unsupported reference configurations. M42 verifies this; M45
   removes the active selector branch only after that proof. If any supported
   preset still uses `i286x`, the series stops for an ADR amendment.
3. **State-load validation is declared as the sole intentional external behavior
   change.** Rejecting a serialized `CPU286` payload whose `cpu_type` is not
   `CPUTYPE_V30` did not exist in the raw loader. It is an explicit defensive
   compatibility firewall, not a precedent for additional semantic changes.
4. **SingleStepTests metadata handling is pinned to the actual dataset
   vocabulary and 0x0F granularity.** The parser recognizes the exact observed
   status values, treats missing status explicitly, fails closed on unknown
   values, and resolves 0x0F forms by second opcode byte using the M42 target
   support map rather than the coarse top-level metadata entry.
5. **Upstream version strings are informational only.** The README version and
   `metadata.json` version are both recorded because they may disagree. Dataset
   identity is based on the pinned commit, content digests, and selection-policy
   version, never either human-readable version string.
6. **External-corpus gates can no longer evaporate through skips.** Every
   M43–M51 report records the executed dataset ID and record/category counts.
   The CI profile and full profile are mandatory at each human gate; a missing
   corpus or skipped comparison is a gate failure, although ordinary hosted CI
   may retain a clearly reported external-data skip policy.
7. **Immutable dispatch-graph identity takes precedence over cosmetic rename
   cleanup.** If an old-looking symbol is part of the committed graph identity,
   M51 records an ADR exception and defers that internal rename rather than
   changing the graph baseline.
8. **Runtime-state ownership covers writes as well as reads.** Every field read,
   written, or address-taken by active instruction execution belongs to
   `Upd9002RuntimeState`; the instruction engine may neither read nor write the
   compatibility image.

---

## Preconditions and immutable baselines

Before M42:

1. Read `AGENTS.md`, `docs/agents/ROADMAP.md`,
   `docs/agents/CONVENTIONS.md`, and all active task/report conventions.
2. Confirm G41 has been explicitly accepted by the maintainer.
3. Confirm the tracked worktree is clean and record the approved G41 SHA.
4. Create and push an immutable tag at that exact SHA, unless it already exists
   and already points there:

```sh
git tag -a pre-upd9002-series \
  -m "Baseline before uPD9002 core consolidation" <G41_SHA>
git push origin pre-upd9002-series
```

Never move or recreate an existing baseline tag. A mismatch between an existing
tag and the approved SHA is a hard stop requiring maintainer resolution.

`pre-upd9002-series` is the reference for:

- all pre-existing M23/golden checkpoint lines;
- ordinary smoke, selftest, ctest, and human-gate behavior;
- G41 save-state fixtures and extracted `CPU286`/`UPD9002` payloads;
- the current CPU_SHUT behavior.

M42 adds internal trace/dispatch/state fixtures. M43 then adds the external
SingleStepTests V20 adapter and freezes its comparison baseline. After Codex
completes M43, reports its final SHA, passes the machine gates, and stops, the
maintainer performs G43. Only after the maintainer explicitly accepts G43 is the
following immutable tag created and pushed at the accepted M43 SHA:

```sh
git tag -a pre-upd9002-refactor \
  -m "Instrumented and V20-compared baseline before uPD9002 refactoring" <G43_SHA>
git push origin pre-upd9002-refactor
```

M44 must not start until this tag exists and points at the accepted G43 SHA.
M44–M51 compare CPU traces, instruction-harness results, final dispatch graphs,
shutdown fixtures, and SingleStepTests V20 differential results with this tag.

Build an old tag for compatibility checks in a separate detached worktree. Do
not switch the active milestone worktree between old and current commits.

---

## Fixed scope and architectural decisions

1. **Behavior preservation through M47, followed only by an explicitly approved
   transition.** No
   legitimate golden, trace, instruction result, final dispatch mapping, device
   result, cycle result, or accepted save-state payload change exists in
   M42–M47. The sole intentional externally visible change before G47 is that
   M44 rejects a
   `CPU286` payload whose serialized `cpu_type` is not `CPUTYPE_V30` before the
   machine can resume. The previous raw loader did not perform that validation.
   This exception is narrowly scoped and is not precedent for any other behavior
   change. M48 may introduce only the REP+0F and protected-state transition
   explicitly approved at G47, with immutable before/after manifests.
2. **Native execution only, with the preserved CPU_SHUT initializer anomaly.**
   The active machine uses the μPD9002 native V30-compatible execution path. No
   active path may select 80286 instruction execution by G45. The historical
   CPU_SHUT path remains the sole explicit exception at the initializer level:
   it invokes the existing 286-style register initializer exactly as recorded by
   the M42 fixture, but it does not select an 80286 opcode dispatcher or make an
   80286 execution mode reachable.
3. **Compatibility mode is out of scope.** The PC-88VA V1/V2 μPD9002
   Z80-compatible mode is not implemented here. Its transition mechanism,
   Z80-visible register model, interrupts, refresh behavior, undocumented
   instructions, and cycle timing remain a separate project. The V30 8080
   emulation mechanism must not be treated as a specification for that mode.
4. **Ownership is split.** The instruction engine is named
   `upd9002_core_*`. The existing 0xFFF0 built-in port/register model in
   `iova/upd9002.*` is named `upd9002_regs_*` after M51. A future aggregate
   `Upd9002Device` is outside this series.
5. **Final directory is fixed.** The surviving instruction core moves to
   `cpu/upd9002/` in M51. Do not defer this choice to Codex.
6. **Runtime table construction remains.** Do not force a merged hand-written
   `static const` table. A generated single-source table design is separate
   work.
7. **One authoritative execution primitive.** The final public execution
   primitive is `upd9002_core_step()`. The dead block executors are removed. A
   future `upd9002_core_run(cycle_budget)` may loop the step primitive, but is
   not implemented here.
8. **Serialized section names and payload layouts stay unchanged.** The
   on-disk tags `CPU286` and `UPD9002`, their section sizes, and their existing
   ABI-specific layouts are preserved. There is no state-version bump in this
   series.
9. **The current shutdown anomaly is preserved.** The present shutdown path
   uses the 286-style register initializer and does not re-establish the V30
   upper flag value in the same way as native reset. Do not fix or normalize
   this behavior here.
10. **Frozen reference material remains untouched.** Do not modify `i286x/` or
    `cpuxva/memoryva.x86`.
11. **No missing instruction implementation.** The current uPD9002/V52 native
    profile omits several instruction forms present in the broader V30/V20
    corpus. M43 records the exact forms as known target gaps. They are neither
    failures nor authorization to add instructions in this series. Tests are
    generated from the actually reachable M42 graph and the explicit M43
    applicability manifest.
12. **C implementation boundary remains.** Core implementation and test hooks
    added to the C core remain C. Source identifiers, comments, diagnostics,
    and scripts added by this series use English.
13. **Supported active builds use the C core.** Every supported active preset
    must build the `i286c`-derived C core. `USE_I286C=off`, `i286x_step()`,
    `v30x_step()`, and the `i286x/` assembly implementation are frozen reference
    configurations, not supported product presets. M42 must prove that no
    supported preset selects them. If that premise is false, stop and amend the
    ADR rather than silently dropping a supported configuration.

---

## State-compatibility model

The following are distinct concepts and must remain distinct after M44:

```text
Cpu286StateCompat
    Exact ABI-specific byte layout of the legacy serialized CPU286 payload.

Upd9002RuntimeState
    State read by the active instruction engine.

Cpu286CompatImage
    Opaque serialization shadow retaining a complete imported/canonical
    CPU286 payload, including inactive legacy fields and padding.
```

Rules:

1. The instruction engine may read, write, or take addresses only within
   `Upd9002RuntimeState`. Every field touched by an active instruction, reset,
   interrupt, DMA-coupled CPU operation, or CPU_SHUT implementation must belong
   to that runtime state. The instruction engine may neither read nor write
   `Cpu286StateCompat` or `Cpu286CompatImage`.
2. The serialization adapter exclusively owns `Cpu286StateCompat` and
   `Cpu286CompatImage` and overlays runtime-owned fields during import/export.
3. On import, first read the complete payload into temporary storage, validate
   its size and `cpu_type == CPUTYPE_V30`, construct a temporary runtime state,
   then commit both runtime state and compatibility image atomically.
4. A rejected payload must not leave a partially loaded machine that can resume
   execution. Use a preflight pass, staged load, or explicit rollback/reset
   mechanism supported by the existing state framework. Merely discovering the
   error after unrelated live sections have been applied is not acceptable.
5. On immediate export after import, start from the retained compatibility
   image and overlay only fields represented by the active runtime state.
   Opaque bytes and padding remain byte-identical.
6. A fresh machine starts from a canonical compatibility image proven against
   the G41 reset fixture.
7. Reset and shutdown must update the compatibility image according to the
   byte-range effects of the corresponding G41 operations. Opaque bytes are not
   preserved across an operation that the old implementation would have
   cleared or replaced.
8. Runtime removal of a legacy field does not authorize zeroing its serialized
   bytes. Its bytes remain in the compatibility image and follow the historical
   lifecycle semantics.
9. Compatibility is asserted only within the ABI/toolchain scope already
   supported by the raw-structure format. This series does not claim new
   cross-endian or cross-ABI portability. Record target, compiler, sizes, and
   offsets for every old/new compatibility fixture.
10. Compare extracted `CPU286` and `UPD9002` payloads explicitly. Do not call a
    whole-file comparison a payload proof when headers, timestamps, ordering,
    or unrelated sections may vary.

---

## Dispatch-identity model

M42 creates two different artifacts:

### 1. Immutable final dispatch graph

`tools/qa/golden/upd9002_final_dispatch_graph.csv`

This records the reachable final dispatch graph used by native execution:

- all six runtime-built root tables;
- every recursively reachable secondary/group table;
- table name, slot, entry kind, target handler or secondary-table identifier;
- stable source-level handler names only, with no addresses or file paths.

This artifact must be byte-identical from the accepted M42 tip through G47.
M48 may change only the exact rows approved at G47 and must preserve this M42
file as immutable historical evidence while recording a post-correction graph.

### 2. Construction provenance

`tools/qa/golden/upd9002_dispatch_provenance_m42.csv`

This records base entries, patch-list operations, explicit DIV/IDIV
replacements, and the final target selected for each runtime-built root slot.
It is evidence about how M42 constructs the final graph. M48 may deliberately
change only the provenance and final-graph rows explicitly approved at G47.
Such changes are reported in a separate transition manifest and never silently
replace the M42 artifact.

Generator requirements:

1. No runtime pointer-to-symbol lookup, host addresses, `dladdr`, debugger
   databases, or stripped/unstripped assumptions.
2. Process the active build configuration and relevant preprocessor choices.
3. Fail closed on an unparsed initializer, patch operation, duplicate slot,
   missing slot, unexpected cardinality, unknown table edge, or ambiguous
   macro expansion. Partial output is a test failure, not a warning.
4. Verify the declared cardinality of every table and recursively enumerate all
   secondary tables reachable from the six roots. A hand-written list of a few
   known secondary tables is not sufficient.
5. Exclude file paths so M51 moves do not perturb the final graph.
6. The final graph is execution evidence. The provenance file is diagnostic
   evidence. Do not conflate them.
7. Function-pointer arrays may be compared at runtime only element by element
   with `==`. The prohibition on `memcmp` applies to function-pointer tables;
   byte-oriented state payload comparisons may use ordinary byte comparison.

---

## Regression model

### Existing baseline

All pre-existing G41/M23 checkpoint lines remain byte-identical through G47.
M48 may change only items explicitly named by the G47 transition decision; all
others remain byte-identical through G51.

### Canonical CPU trace

M42 adds a deterministic per-instruction trace. Each record has a fixed schema,
field order, integer width, and lowercase zero-padded hexadecimal formatting.
It contains, as applicable:

- step sequence number and instruction bytes;
- general and segment registers, cached segment bases, IP, and FLAGS;
- consumed cycles and remaining clock;
- interrupt/exception result;
- ordered memory, I/O, and DMA events with an explicit event origin
  (`cpu`, `dma`, or `device`) and sequence number.

It contains no host pointer, wall-clock time, path-dependent text, or unstable
symbolization. Trace-enabled and trace-disabled runs must reach identical final
machine checkpoints.

### Instruction harness

Prefer a direct test-only CPU harness that installs instruction bytes into
controlled emulated memory, initializes CPU/bus state, executes an exact number
of authoritative step calls, and records canonical results. Do not assume a
special ROM-less guest loader exists unless M42 verifies one.

Coverage is generated from the M42 final graph and provenance:

- every root slot whose final target differs from its base target;
- every reachable V30/uPD9002-specific handler represented by those slots;
- relevant prefix dispatch modes and secondary groups;
- normal and faulting DIV/IDIV paths;
- arithmetic boundary values for widths actually handled;
- the existing step-path O_FLAG behavior;
- the active 0x0F native path;
- interrupt/exception, I/O, memory-write, DMA-order, and cycle observations
  where the instruction can produce them.

Never add an instruction implementation merely to satisfy the harness.

### Shutdown fixture

M42 adds a dedicated fixture that reaches the real CPU_SHUT/reset-request path
and records:

- active CPU registers, bases, IP, FLAGS, clocks, and relevant device state;
- the exact extracted `CPU286` payload;
- the canonical trace/checkpoint proving the current upper-FLAGS anomaly.

Every later milestone reruns this fixture. A change is a gate failure, not an
opportunity to update the baseline.

---

## SingleStepTests V20 differential-oracle model

M43 integrates `https://github.com/SingleStepTests/v20` as an external,
hardware-generated semantic oracle for the **intersection** of the NEC V20 native
instruction set and the current uPD9002/V52 native target. It is not the target
specification, and it is not a replacement for the M42 internal trace/harness.

### Upstream and dataset identity

1. Pin an exact 40-hex upstream commit. Never consume a moving branch in a gate.
2. Record both human-readable upstream version strings separately: the
   README-declared suite version and the version stored in `metadata.json`.
   They are informational and may disagree. Also record the exact upstream
   commit, `metadata.json` digest, selection policy, and corpus digests in a
   committed dataset manifest.
3. Do not silently download the corpus during normal CI. Use an explicit cache or
   `V20_SSTS_ROOT`-style path, verify every digest before execution, and fail with
   an actionable skip/error according to the repository's established external-test
   policy.
4. Reuse any existing `tests/ssts/` adapter, `.sst` format, baseline schema, and
   runner. Do not create a parallel framework. If no such infrastructure exists,
   introduce the minimum reusable adapter in M43.
5. The committed comparison summaries are:

```text
tests/ssts/baseline/v20_native_ci.json
tests/ssts/baseline/v20_native_full.json
tests/ssts/baseline/upd9002_v20_known_gaps.json
```

6. Content addressing is mandatory. Define canonical record bytes, then compute:

```text
record_digest
opcode_testset_digest
dataset_digest
dataset_id
```

   `dataset_id` is derived only from the exact upstream commit, content digests,
   and selection-policy version. Do not include either the README version or the
   `metadata.json` version in identity or comparison keys. The baseline records
   both version strings as informational fields, plus the exact `dataset_id`,
   total record count, per-opcode counts, metadata digest, upstream commit, and
   selection-policy version. A run with a different identity is incomparable and
   must fail before result comparison.
7. The deterministic CI subset is selected by upstream test `hash`, never by
   mutable array position alone. A `trim500` policy, when used, selects a stable
   maximum of 500 hashes per opcode/form in sorted-hash order. The full profile
   records the entire applicable corpus and may run locally/nightly rather than on
   every ordinary CI job.

### Comparison scope

Each upstream record resolves to exactly one category:

```text
applicable
known_target_gap
expected_target_divergence
unsupported_fixture
upstream_nonblocking
```

- `applicable`: the opcode/form is legal and implemented in the M42 final uPD9002
  graph, the fixture can be represented faithfully, and the metadata status is in
  the blocking policy. Compare final general/segment registers, IP, changed RAM,
  termination class, and defined FLAGS after applying the upstream `flags-mask`.
- `known_target_gap`: a V20/V30 instruction or form absent from the current
  uPD9002/V52 profile. This is the maintainer-approved known issue described in
  this specification. It is counted and hashed separately, not executed as an
  implementation requirement, and never included in the failure count.
- `expected_target_divergence`: a supported target form whose uPD9002/V52 result
  is intentionally different from V20. This category requires explicit evidence
  and maintainer approval for an exact form/test-hash set. Do not use it as a
  generic mismatch bucket.
- `unsupported_fixture`: the instruction may be supported, but the test depends
  on a facility the harness cannot reproduce faithfully, such as an injected full
  prefetch queue, an unsupported bus mode, or an excluded halt/lock/FPU fixture.
- `upstream_nonblocking`: the pinned metadata marks the exact case as
  undocumented, undefined, FPU, or otherwise explicitly nonblocking under the
  committed policy, and the target has not claimed that behavior. An `alias`
  subentry is resolved to its exact canonical form first and is not automatically
  nonblocking. It may be reported but does not define uPD9002 correctness in
  this series. `extension` is structural metadata and is never automatically
  nonblocking.

The metadata parser is pinned to the exact vocabulary observed in the selected
upstream commit. At the primary opcode level this includes `normal`, `prefix`,
`fpu`, `extension`, `undocumented`, and an explicitly represented missing-status
case. Register/group subentries include `undefined`, `alias`, and
`undocumented`. A missing status is data requiring an explicit classifier rule;
it is not silently treated as `normal`. Any unknown status, field shape, or new
vocabulary value is a fail-closed dataset incompatibility.

The upstream metadata `arch` value (`86`, `186`, or `v30`) describes the V20
suite's family classification. It does **not** prove that uPD9002/V52 implements
that form. Applicability is the intersection of metadata, the M42 final graph,
and the explicit target support/gap manifest.

The primary `0x0F` metadata entry is only a coarse `extension`/`v30` marker and
does not describe second-byte forms. Every `0x0F xx` record must therefore be
resolved by its second opcode byte and any structural operands against the M42
support map and target gap policy. Never classify the complete 0x0F subspace as
supported, missing, or nonblocking from the top-level metadata entry alone.

### Known-gap discipline

1. M43 resolves the complete current known-gap set and commits it. Each entry
   includes exact opcode/form or structural selector, reason, evidence, upstream
   status/arch, resolved test count, sorted test-hash digest, and first baseline.
2. Allowlisting by `idx`, by a blanket `all`, or by an unbounded textual pattern is
   forbidden. Use upstream test hashes or an exact opcode/ModR-M/prefix selector,
   then commit the resolved count and hash digest.
3. M44–M47 require the known-gap file and its resolved hash set to remain
   byte-identical. M48 may alter only exact selectors/hashes approved at G47;
   the set may never grow merely to hide a regression.
4. A known gap may shrink only in a separate instruction-implementation milestone
   with its own specification and new semantic baseline. This series must not make
   that change.

### Harness and semantic limits

1. Run the tests in a direct CPU harness with 1 MiB writable memory and a
   deterministic I/O adapter, matching the upstream test assumptions where the
   target can do so.
2. The required blocking profile uses tests with an empty initial prefetch queue.
   Tests with supplied queue contents remain `unsupported_fixture` unless M43
   proves that the current core can inject and advance the V20 queue faithfully.
3. Upstream bus-cycle arrays are diagnostic only in this series. Do not gate V52
   timing or prefetch behavior against a V20 maximum-mode bus trace. M42's own
   trace remains the behavior-preservation gate for vaeg cycle/event ordering.
4. Preserve upstream V20 semantics in the adapter rather than normalizing to
   generic x86 expectations. At minimum, M43 must explicitly exercise and report:
   - REPC/REPNC string-prefix behavior on applicable instructions;
   - the REPE/REPNE CMPS operand-access-order case;
   - C0/C1 and D2/D3 counts that expose incorrect 5-bit masking;
   - REP-prefixed IDIV, where the V20 suite expects the prefix to have no effect;
   - normal and Type-0 divide-interrupt outcomes where the target profile applies.
5. Apply metadata-defined FLAGS masks before comparison. Undefined flags must not
   create false failures or be silently promoted to target requirements.
6. No individual malformed or long-running case may hang ctest. Execute opcode
   shards in child processes with deterministic timeouts and record timeout,
   signal, crash, halt, and normal completion as explicit termination classes.

### Baseline and failure signatures

M43 records the current comparison result; it does not require the current core
to pass every applicable V20 test. Existing target bugs are visible baselines, not
excuses to weaken the dataset.

For every applicable failure, record a canonical signature containing at least:

```text
test_hash
opcode_and_form
instruction_bytes
termination_class
mismatch_kinds
register_differences
masked_flags_difference
memory_differences
ordered_io_differences
```

Use fixed field order, fixed register order, ascending RAM addresses, lowercase
zero-padded hexadecimal, sorted/deduplicated `mismatch_kinds`, and no volatile
host text. Hash the canonical serialization with SHA-256.

M44–M47 require exact equality with M43 for:

- dataset identity and record counts;
- category counts and resolved hash sets;
- known target gaps and expected divergences;
- applicable pass/failure test-hash sets;
- every failure signature and termination class.

At M48 and later, every difference must be present in the exact G47-approved
transition manifest; both an extra failure and an unexplained disappearance are
gate failures.

---

## Execution protocol for every milestone

1. One milestone per Codex session. Do not begin the next milestone.
2. At session start, read the repository instructions and the current task,
   inspect `git status`, record the starting branch/SHA, and verify prerequisites
   and baseline tags.
3. Use `M<n>:` commit prefixes and the repository's required report format.
4. Run the repository's full standard build/lint/smoke/selftest/ctest gate,
   `golden_smoke.sh --check`, all applicable CPU/state/dispatch tests, and from
   M43 onward the pinned SingleStepTests V20 CI comparison. At every G43–G51
   human gate, run both the verified CI profile and the verified full profile.
   A missing corpus, external-data skip, or unexecuted required profile at a
   human gate is a gate failure, not a warning. Ordinary hosted CI may follow a
   documented external-data skip policy, but that does not satisfy a milestone
   or human gate.
5. If any pre-existing golden, M42 CPU artifact, or M43 V20 dataset/category/
   failure signature differs, stop and identify the cause. Do not re-record it
   in this series.
6. Ambiguous reachability means keep and defer.
7. M47 produces REP+0F correctness and state-policy evidence and stops. M48 may
   implement only all five explicit G47 approvals. M49 then inventories the
   remaining protected cluster; M50 may delete only exact groups approved at
   G49. M51 is rename-only cleanup.
8. By accepting this master specification, the maintainer pre-approves removal
   of only these three named dead entry points at their assigned milestones,
   after their gates establish non-use: `i286c_step()`, `i286c()`, and `v30c()`.
   No other handler or file deletion is pre-approved.
9. Renames use a `git mv`-only commit followed immediately by a reference/build
   fixup commit. Add mass mechanical commit hashes to
   `.git-blame-ignore-revs` when required by repository convention.
10. At milestone completion, report exact commands/results, final SHA, tracked
    worktree status, remaining risks, and stop for the human gate. From M43
    onward, every report must include the actually executed `dataset_id`, CI and
    full record counts, category counts, applicable pass/failure counts, and an
    explicit statement that neither required profile was skipped at the human
    gate.

---

## M42 — ADR, exhaustive inventory, regression instruments, and baselines

```text
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
   f. Ownership: i286c_step() in M45; i286c()/v30c() in M46; REP+0F
      correctness evidence in M47; approved semantics/state transition in M48;
      protected-mode inventory in M49; approved deletion only in M50.
   g. Future upd9002_core_run(cycle_budget) pattern, not implemented here.
   h. Compatibility-mode and missing-instruction non-goals.
   i. CPU_SHUT upper-FLAGS anomaly preserved by fixture.
   j. Public API and active-file rename policy for M51; internal static handler
      identifiers and I286_* helpers may remain.
   k. G41, M42, and M43 baseline/tag policy.
   l. SingleStepTests V20 is an external semantic oracle only for the
      supported uPD9002/V52 intersection; known missing V30/V20 forms
      are target gaps, not failures or implementation requests.
   m. NP2 286 protected-mode code is retained through G49 and may be
      deleted only by the dedicated M50 task after explicit G49 approval.
   n. Supported active presets require the C core. `USE_I286C=off` and the
      `i286x/` implementation are frozen unsupported references; if M42 finds a
      supported preset that selects them, stop and amend this ADR before M43.
   o. The M44 `cpu_type != CPUTYPE_V30` rejection is the sole intentional
      externally visible change through G47. M48 may apply only the exact
      REP+0F/state transition approved at G47.

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
      the M42 provenance a gate after the approved M48 correctness transition.
   Both M42 artifacts remain immutable historical baselines; M48 records any
   approved post-correction graph separately.

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
```

---

## M43 — SingleStepTests V20 external semantic baseline

```text
Task: docs/agents/tasks/M43_upd9002_singlestep_v20_baseline.md

Goal:
Establish a reproducible, content-addressed comparison between the current
uPD9002/V52 native core and the hardware-generated SingleStepTests V20 native
suite. Add test infrastructure and baselines only; do not change CPU semantics,
dispatch, timing, state layout, or public names.

Prerequisites:
- G42 explicitly accepted.
- The M42 final graph, direct harness, trace, and support-map artifacts are green.
- Worktree clean.

Steps:
1. Inventory and reuse existing SSTS infrastructure:
   a. Search tests/, tools/qa/, CMake/ctest, prior reports, and baselines for an
      existing SingleStepTests adapter, .sst record format, dataset manifest,
      timeout runner, or failure-signature schema.
   b. Extend the existing design rather than creating a second framework.
   c. Record any migration needed to reach the contracts in this task; do not
      silently invalidate a previously accepted baseline.

2. Pin the upstream dataset:
   a. Source: https://github.com/SingleStepTests/v20, v1_native.
   b. Record the exact 40-hex commit, MIT attribution, README-declared version,
      `metadata.json`-declared version, metadata digest, and corpus/selection
      digests. The two version strings are informational and may differ.
   c. Never use a moving branch in a gate and never download unverified data as
      part of an ordinary test run.
   d. Provide an explicit acquisition/verification command and cache-root
      setting. A missing corpus may produce a clear external-data skip only in
      ordinary hosted CI where the task explicitly permits it. It is a hard gate
      failure at G43–G51 and in every milestone report's required local/human
      comparison. Digest mismatch is always a hard failure.

3. Define and test the content-addressed dataset contract:
   a. Canonical serialized record bytes and record_digest.
   b. Per-opcode/form opcode_testset_digest over sorted record digests.
   c. dataset_digest over metadata, all opcode-testset digests, and the selection
      policy.
   d. dataset_id derived from upstream commit identity, content digests, and
      policy version only. Exclude README and metadata version strings.
   e. Baseline comparison first requires exact dataset_id, dataset_digest,
      metadata digest, total, and per-opcode counts. Record both version strings
      as informational fields but never compare identity through them.

4. Create deterministic profiles:
   a. CI profile: empty-prefetch-queue records only; statuses allowed by the
      blocking policy; stable maximum 500 records per opcode/form selected by
      sorted upstream test hash, not idx. Commit
      tests/ssts/baseline/v20_native_ci.json.
   b. Full profile: every applicable empty-queue record. Commit the result
      summary to tests/ssts/baseline/v20_native_full.json; run locally/nightly or
      at the human gate if ordinary CI cannot carry the corpus cost.
   c. Prefetched records are reported separately and remain unsupported_fixture
      unless queue injection is proven faithful. Do not blend them into the
      required semantic profile.

5. Build the target applicability classifier from three independent inputs:
   a. SingleStepTests metadata status/arch/reg/flags-mask information.
   b. The M42 final dispatch graph and opcode/form support map.
   c. The maintainer-approved uPD9002/V52 target policy.
   Resolve every selected record to exactly one category: applicable,
   known_target_gap, expected_target_divergence, unsupported_fixture, or
   upstream_nonblocking. Fail closed on an unclassified record.
   d. Pin the parser to the exact metadata vocabulary observed at the selected
      commit: primary statuses `normal`, `prefix`, `fpu`, `extension`,
      `undocumented`, plus an explicit missing-status state; register/group
      statuses `undefined`, `alias`, and `undocumented`. Unknown values or
      shapes are hard failures. `extension` is structural, not automatically
      nonblocking, and `alias` requires exact canonical-form handling.
   e. Resolve every `0x0F xx` record by second opcode byte and structural form
      against the M42 map. The coarse primary `0x0F` metadata entry must never
      classify the complete extension space.

6. Commit tests/ssts/baseline/upd9002_v20_known_gaps.json:
   a. Record every V20/V30 opcode/form absent from the current uPD9002/V52 target
      that appears in the selected corpus.
   b. Each entry contains an exact structural selector, reason, evidence,
      upstream status/arch, resolved count, sorted test-hash digest, and first
      baseline.
   c. Treat the target's reduced instruction set as a known issue. Do not count
      these records as failures and do not implement the missing instructions.
   d. Forbid idx-based, blanket-all, or open-ended textual allowlists.
   e. Any expected_target_divergence is separate, exact, evidence-backed, and
      explicitly approved; do not disguise a core bug as a target gap.

7. Implement the direct comparison adapter:
   a. Initialize registers, segments, IP, FLAGS, 1 MiB RAM, and deterministic I/O
      from each record; execute the authoritative step primitive to the expected
      termination boundary.
   b. Compare final changed registers/RAM and defined FLAGS after metadata masks.
   c. Record Type-0 divide-interrupt behavior where applicable.
   d. Keep V20 maximum-mode cycle arrays and prefetch traces diagnostic only;
      do not make them V52 timing gates.
   e. Run opcode shards in child processes with deterministic timeout and report
      normal, type0, timeout, signal, crash, and halt termination classes.

8. Preserve upstream-specific edge cases in tests and report output:
   a. REPC/REPNC behavior for applicable string/I/O forms.
   b. REPE/REPNE CMPS operand access order.
   c. C0/C1 and D2/D3 counts that detect improper 5-bit masking.
   d. REP-prefixed IDIV with no V20 semantic effect.
   e. Normal and faulting DIV/IDIV paths and defined/undefined flag masking.

9. Define canonical failure signatures:
   a. Include test_hash, opcode/form, instruction bytes, termination class,
      mismatch kinds, register differences in fixed order, masked FLAGS,
      ascending-address memory differences, and ordered I/O differences.
   b. Use lowercase fixed-width hexadecimal, sorted/deduplicated mismatch kinds,
      canonical JSON or an equally specified serialization, and SHA-256.
   c. Store both readable content and signature digest. Never key approval only by
      idx or raw log text.

10. Record the current baseline without repairing the CPU:
    a. Exact dataset/category counts and resolved hash sets.
    b. Applicable pass and failure test-hash sets.
    c. Complete failure signatures and termination classes.
    d. Known gaps and any separately approved target divergences.
    e. The current core is not required to have zero failures. M44–M47 require
       exact equality; M48 and later allow only the explicit G47 transition.

11. Add ctest/CI integration:
    a. Required CI comparison when the verified CI dataset is available under the
       repository's external-data policy. A hosted-CI external-data skip must be
       explicit and visible, but it never satisfies G43 or any later human gate.
    b. A fast adapter selftest using tiny committed synthetic fixtures, always
       runnable without the external corpus.
    c. A full-profile command documented and executed for G43.
    d. A negative test proving dataset/digest mismatch fails before comparison.
    e. A negative test proving a newly unclassified record cannot be silently
       skipped.
    f. A report helper that prints the executed dataset_id, profile, total record
       count, per-category counts, and skip status in canonical form. Every
       M43–M51 milestone report embeds this output.
    g. Both CI and full profiles are mandatory at G43 and every later human gate;
       missing corpus or skip is a gate failure.

12. Prove M42 traces, direct-harness results, final graph, state fixtures, and
    all G41/M23 goldens are unchanged. Report final M43 SHA and stop. Do not
    create pre-upd9002-refactor inside the task. After G43 is explicitly
    accepted, the maintainer creates and pushes that tag at the accepted SHA.

Non-goals:
- No uPD9002/V52 instruction implementation.
- No timing or prefetch-queue emulation change.
- No conversion of V20-only behavior into target requirements.
- No state adapter, CPU_TYPE fold, handler deletion, or rename.

Gate:
- Exact upstream identity and content digests recorded; README and metadata
  version strings recorded separately as informational values.
- CI and full profiles reproducible from test hashes.
- Every selected record classified exactly once.
- Known-gap manifest explicitly approved and no gaps counted as failures.
- Failure signatures and timeout handling deterministic.
- Upstream edge-case fixtures exercised and reported.
- G41/M42 behavior artifacts unchanged.
- CI and full V20 comparisons completed without skip for G43, with executed
  dataset_id and record/category counts in the report; standard human gate green.
```

---

## M44 — Serialized/runtime state boundary and transactional loading

```text
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
      the only intentional external semantic change through M47.
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
```

---

## M45 — Native-mode invariant and per-instruction dispatch fold

```text
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
```

---

## M46 — Dispatch normalization and block-executor removal

```text
Task: docs/agents/tasks/M46_upd9002_dispatch_normalization.md

Goal:
Retain one production construction path, prove final dispatch identity and
post-construction immutability, and remove the dead block execution semantics.

Prerequisites:
- G45 explicitly accepted.
- pre-upd9002-refactor points at accepted G43.

Steps:
1. Construction lifecycle:
   a. Keep v30cinit() as the sole production constructor until M51 renames it.
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
      These are future M49/M50 inputs only, not sufficient deletion evidence.

Gate:
- Final graph byte-identical to M42.
- Root pointer snapshots green.
- No i286c()/v30c() block-executor reference remains in the active tree.
- All trace, harness, state, shutdown, ordinary, and human gates unchanged.
- M43 V20 dataset identity, category/hash sets, known gaps, and failure signatures
  unchanged.
```

---

## M47 — Determine uPD9002 REP-prefixed 0x0F correctness

Task: docs/agents/tasks/M47_upd9002_rep0f_correctness.md

M47 is evidence and decision preparation only.  It records the accepted current
REPNE/REPE-to-i286c_cts behavior without certifying it as correct and without
changing dispatch, state, instruction behavior, or an accepted baseline.

M47 pins primary NEC and PC-88VA/uPD9002 documents, analyzes every M43
F2/F3+0F record and affected hash under each candidate outcome, builds a
complete instruction matrix, inventories protected-state residue, designs a
safe machine-readable PC-88VA probe, and prepares an exact prospective M48
transition manifest.  Unsourced emulator implementations are not architectural
proof, and V20 evidence alone is not uPD9002 proof.

G47 must explicitly approve the REP+0F semantic rule, protected-state policy,
exact dispatch/state edits, exact M42/M43 transition scope, and evidence
sufficiency.  An unresolved decision forbids M48.

---

## M48 — Implement the explicitly approved REP+0F and state decision

Task: docs/agents/tasks/M48_upd9002_rep0f_implementation.md

M48 starts only after all five G47 approvals.  It implements exactly the
approved semantic/state rule and exact prospective transition manifest.  M42
and M43 historical artifacts remain immutable; M48 records a separately
reviewed post-correction baseline and accounts for every changed graph row,
record hash, classification, and failure signature.

M48 does not perform protected-mode deletion or rename work.  CPU_SHUT FLAGS
0000, unaffected state bytes, v30c_step ownership, and the single immutable
constructor remain required.

---

## M49 — Inventory the remaining NP2 286 protected-mode cluster

Task: docs/agents/tasks/M49_upd9002_isolate_np2_286_protected_mode.md

M49 evaluates actual reachability after the approved M48 correction.  It must
not assume the cluster is unreachable.  Every file, symbol, dispatch edge,
shared helper, lifecycle edge, runtime field, and serialized position receives
deterministic evidence.  Only dependency-closed members proven unreachable
after M48 may be proposed for M50; an empty proposal list is valid.

---

## M50 — Remove only G49-approved protected-mode groups

Task: docs/agents/tasks/M50_remove_np2_286_protected_mode.md

M50 may delete only exact dependency-closed identifiers explicitly approved at
G49 and revalidated at its starting SHA.  It preserves the G48 semantic/state
decision, every unapproved or shared dependency, serialization compatibility
required by that decision, and CPU_SHUT.  No correctness or rename work belongs
in M50.

---

## M51 — Pure renames, moves, API cleanup, and repository guards

Task: docs/agents/tasks/M51_upd9002_rename.md

M51 performs only mechanical moves, public API renames, and final guards after
G50.  It contains no semantic, state-policy, dispatch, or dead-code decision.

---

## Superseded v6 M47 — historical text; do not execute

```text
Task: docs/agents/tasks/M47_upd9002_isolate_np2_286_protected_mode.md

Goal:
Separate the active uPD9002/V52 native core from NP2's partial 80286
protected-mode implementation and produce the exact deletion evidence for M48.
This milestone must not delete protected-mode handlers, files, descriptor state,
or supporting code.

Prerequisites:
- G46 explicitly accepted.
- pre-upd9002-refactor points at accepted G43.
- M42 final graph, M43 V20 baseline, M44 state boundary, M45 native invariant,
  and M46 single-step construction path are green.

Protected-mode cluster definition:
The cluster includes every candidate whose purpose is 80286 protected-mode or
system-mode semantics, including but not assumed limited to: 0x0F system groups,
MSW/GDTR/IDTR/LDTR/TR operations, descriptor-cache loading, selector validation,
privilege/type/presence checks, protected interrupt/return paths, and helper/state
code used only by those paths. Names alone are not evidence; classify by behavior
and references.

Steps:
1. Create docs/agents/reports/m47_np2_286_protected_mode_inventory.md with one
   row per file, handler, helper, table edge, macro, and runtime field:
   a. Exact definition and all direct/macro-expanded/address-taking references.
   b. Which NP2 286 protected-mode behavior it implements.
   c. Root/secondary/base-table reachability and whether native final dispatch can
      reach it.
   d. Runtime state reads/writes, range writes, reset/shut effects, and serialized
      byte ranges.
   e. Compiler cross-reference, function-section, gc-sections, and linker-map
      evidence under every supported preset.
   f. Proposed M48 disposition: delete, retain-shared, split-before-delete, or
      ambiguous/defer.

2. Establish an explicit internal boundary without deleting semantics:
   a. Identify the active native-core files/functions and the dormant protected
      cluster in source/CMake documentation or focused internal headers.
   b. Add a repository/ctest guard proving no active uPD9002 final dispatch edge,
      runtime selector, callback registration, or direct call enters the protected
      cluster.
   c. The guard must allow the cluster to remain compiled in M47; it proves
      non-reachability, not absence.
   d. If a translation unit irreducibly mixes shared native helpers and protected
      handlers, record the exact split required by M48. Do not perform a broad
      engine rewrite in M47.

3. Keep every protected-mode source and handler behavior unchanged:
   a. Do not redirect base slots to _reserved in M47.
   b. Do not delete i286c_0f.c or equivalent system-instruction code.
   c. Do not delete descriptor/selector/privilege helpers.
   d. Do not remove GDTR, IDTR, MSW, LDTR, LDTRC, TR, TRC, or related runtime
      fields merely because native execution does not read them.
   e. Do not alter the serialized CPU286 compatibility image.

4. Ordinary native cleanup is permitted only outside the protected cluster:
   a. A dead selector macro, declaration, or wrapper may be proposed only if it is
      not required to compile or audit the retained protected cluster.
   b. Use the normal evidence and explicit approval rule for any such deletion.
   c. Ambiguous code remains. Do not use M47 to perform the M48 deletion early.

5. Produce the exact M48 candidate list:
   a. Every candidate has an immutable path/symbol identity and evidence row.
   b. Group candidates by dependency-closed deletion unit.
   c. State which base-table redirections, source splits, runtime-field removals,
      CMake changes, and compatibility-image overlays M48 would require.
   d. State what must remain shared, especially real-mode/V30 interrupt mechanics,
      memory helpers, exception delivery, and CPU_SHUT behavior.
   e. Include expected construction-provenance changes and prove the M42 final
      graph is expected to remain identical.

6. Run all gates with the cluster still present:
   a. G41/M23 checkpoints and state payload fixtures.
   b. M42 trace, direct harness, final graph, root-pointer snapshot, and shutdown
      fixture.
   c. M43 V20 CI and full comparison: exact dataset/category/hash/failure-signature
      equality and unchanged known-gap manifest.
   d. ASan plus all standard presets and the human gate.

7. Stop after reporting the final M47 SHA and exact candidate list. G47 acceptance
   must explicitly state which dependency-closed groups, if any, are approved for
   M48. Silence or a general approval of the series is not deletion approval.

Non-goals:
- No NP2 286 protected-mode handler/file/state-field deletion.
- No base-slot redirection that makes such deletion possible.
- No uPD9002 semantic, timing, instruction-set, state-layout, or rename change.

Gate:
- Active-native-to-protected-cluster reachability guard green.
- Complete evidence table and dependency-closed M48 candidate list reviewed.
- Protected cluster still present and behaviorally untouched.
- M42 and M43 baselines exactly unchanged.
- ASan and standard human gate green.
```

---

## Superseded v6 M48 — historical text; do not execute

```text
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
```

---

## Superseded v6 M49 — historical text; do not execute

```text
Task: docs/agents/tasks/M49_upd9002_rename.md

Goal:
Make active names reflect the consolidated uPD9002 design. M49 contains no
semantic, timing, state-layout, dispatch, or dead-code change.

Prerequisite:
- G48 explicitly accepted.
- All semantic/state work and the dedicated M48 protected-mode
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
      Confirm the M48 protected-mode implementation is not merely moved under the
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
   Rename any additional externally visible survivor found by M42. After M49,
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
      matched by an old public-name cleanup pattern, do not rename it in M49;
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
```

---

## Revised series completion criteria checked at G51

1. `upd9002_core_step()` is the sole public execution primitive. No block
   executor remains.
2. REP+0F execution and protected-state import follow the exact G47-approved
   uPD9002 rule.  Any retained legacy behavior is explicit and evidence-backed,
   not inferred from an inherited 80286 implementation.
3. Runtime and compatibility-image ownership follows the G47-approved state
   policy.  Instruction code cannot access serialization-only opaque bytes.
4. Every change from the G41/M44 state matrix is limited to the exact G47
   approval and recorded in the G48 transition manifest; all unaffected
   CPU286/UPD9002 bytes still round-trip exactly.
5. Reset and CPU_SHUT transformations remain exact, including FLAGS 0000.
   Import behavior for protected residue matches the selected G47 policy.
6. Historical M42 graph/provenance artifacts remain immutable.  The current
   graph equals the separately approved G48 post-correction baseline, and every
   transition row is documented.
7. The six live runtime-built root tables remain element-wise equal to their
   post-construction snapshots. No function-pointer `memcmp`, raw pointer hash,
   or runtime symbol resolution is used.
8. SingleStepTests V20 is pinned by exact upstream commit and content-addressed
   dataset identity derived from commit/digests/policy, not human-readable
   version strings. The README and metadata versions are recorded separately.
   Every G43–G51 human gate executed both CI and full profiles without skip and
   reported dataset ID and record/category counts. The M43 category counts,
   resolved hash sets, applicable pass/failure sets, failure signatures, and
   termination classes differ only by the exact G47-approved transition.
9. The uPD9002/V52 known-gap manifest explicitly lists the V20/V30 instruction
   forms missing from the current target. Those records are counted separately,
   are not failures, did not grow during the series, and did not trigger new
   instruction implementation.
10. V20 metadata flag masks are applied; the parser uses the pinned observed
    vocabulary and fails closed on unknown values; `0x0F` records are resolved
    by second opcode byte against the target map. Empty-queue semantic
    comparison is the required profile; prefetched/cycle traces are not
    misrepresented as V52 timing requirements.
11. M47 established REP+0F correctness evidence without changing behavior.
    M48 implemented only the G47-approved transition. M49 inventoried the
    remaining cluster, and M50 deleted only dependency-closed groups explicitly
    approved at G49. Ambiguous/shared code remains documented.
12. The CPU_SHUT active-state, trace, and serialized payload fixtures remain
    byte-identical; the known upper-FLAGS anomaly was not silently fixed.
13. No missing uPD9002/V52 instruction, timing change, performance optimization,
    MT API, or compatibility mode was added.
14. Active ownership is unambiguous: instruction core under `cpu/upd9002/` with
    `upd9002_core_*`; built-in 0xFFF0 registers under
    `iova/upd9002_regs.*` with `upd9002_regs_*`.
15. All surviving public CPU APIs and active file basenames have uPD9002 names,
    except a narrowly documented ADR exception if immutable dispatch-graph
    identity makes an internal rename unsafe in M51. Retained `I286_*` and static
    handler identifiers are explicitly internal and documented, not evidence
    that protected-mode code was retained accidentally.
16. `CPU286` and `UPD9002` tags, sizes, offsets, versions, and payload
    transition rules equal the exact G47 approval; all unrelated layout bytes
    are unchanged.
17. Every supported active preset uses the C core and has no selectable
    `USE_I286C=off`/i286x execution path. `i286x/` and
    `cpuxva/memoryva.x86` remain untouched frozen references; `cpuxva/` is not
    an active include path.
18. `pre-upd9002-series` points at accepted G41 and
    `pre-upd9002-refactor` points at accepted G43; neither tag was moved.
19. All pre-existing G41/M23, M42, and M43 artifacts remain immutable historical
    evidence. Current results differ only where the G47/G48 transition manifest
    explicitly authorizes them.
20. The accepted uPD9002 ADR records ownership, directory, public API mapping,
    state-shadow lifecycle semantics, dispatch final/provenance split,
    SingleStepTests V20 applicability and target-gap policy, M47 correctness,
    M48 transition, M49 inventory, and M50 deletion policy, future run-budget pattern,
    compatibility-mode non-goals, tag policy, and shutdown anomaly.
