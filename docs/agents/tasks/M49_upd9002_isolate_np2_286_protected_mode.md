# M49 — Audit protected-mode reachability after the M48 fail-closed transition

Derived from `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
(v7 M47–M51 sequence amendment).

## Authoritative context

Before editing, read and obey:

1. `AGENTS.md`
2. `docs/agents/ROADMAP.md`
3. `docs/agents/CONVENTIONS.md`
4. `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
5. This task file
6. `docs/agents/DECISIONS/ADR-0012-upd9002-ownership.md`
7. The accepted M42–M48 reports

The maintainer's directly assigned M49 specification is authoritative when it
is more specific than the master plan. Stop and report a material conflict;
do not improvise.

## Session boundary

Work only on **M49**. Start at the exact accepted G48 SHA with a clean
worktree. Do not reset, rebase, squash, or rewrite M42–M48 history. Do not
delete protected-mode code, begin M50 or M51, or declare G49 passed.

## Goal

Prove which parts of the remaining NP2 80286 protected-mode implementation
became unreachable after M48:

- replaced REP-prefixed 0x0F legacy execution with an atomic diagnostic stop;
- rejected saved states with `MSW.PE` set before live-state mutation; and
- retained PE-clear protected residue under the accepted M44/M48 serialization
  policy.

Produce deterministic, dependency-closed deletion groups for possible M50
approval. M49 is audit and approval preparation only. It must not delete,
stub, redirect, disable, rename, or move production CPU behavior.

## Accepted M48 invariants

- `F2/F3 0F xx` reaches the diagnostic stop before architectural or device
  state mutation and never enters `i286c_cts`.
- The accepted 522-case suite proves runtime-state and memory atomicity.
- `MSW.PE` save states are rejected transactionally with the exact compatibility
  diagnostic before any live section is committed.
- PE-clear protected residue remains accepted and round-trips according to the
  M44/M48 policy.
- CPU_SHUT retains its 286-style initializer and FLAGS `0000` anomaly.
- The real uPD9002/V52 REP+0F semantics remain unknown.

M49 must not weaken or reinterpret these invariants.

## Required reachability audit

Audit and machine-enumerate all roots that could retain protected behavior:

1. All six final dispatch roots and every secondary/group table.
2. Base-table and constructor provenance, including entries overwritten before
   construction completes.
3. Direct and same-translation-unit calls.
4. Function pointers, table initializers, and other address-taking references.
5. Macro-expanded paths, especially `SEGSELECT`, interrupt/exception helpers,
   and segment-load helpers.
6. State import/export, normal reset, and CPU_SHUT.
7. I/O consumers such as `CPU_MSW`.
8. Test-only, fixture, audit-tool, and frozen-reference uses.

At minimum enumerate `i286c_cts`, `cts0_table`, `cts1_table`, all protected
system-instruction handlers, `i286c_selector`, `MSW`, `GDTR`, `IDTR`, `LDTR`,
`LDTRC`, `TR`, and `TRC`. Explicitly prove that `F2/F3 0F xx`, including
`F2/F3 0F 01 F0`, cannot enter the legacy cluster.

## Protected-state matrix

For each protected field record:

- every explicit and range-based runtime read, write, and address-taking use;
- serialized offset and size;
- import and export treatment;
- reset and CPU_SHUT transformation;
- I/O exposure and active-instruction dependencies; and
- whether runtime ownership could be removed while preserving the serialized
  position as opaque compatibility data.

Serialized compatibility is not runtime reachability. A runtime-removal
proposal must specify the retained compatibility representation, exact overlay
change, reset/CPU_SHUT behavior, I/O replacement, and atomic PE rejection.
M49 must not implement it.

## Classification

Assign each candidate exactly one disposition:

- `active_native_required`
- `fail_closed_diagnostic_required`
- `cpu_shut_compatibility_required`
- `serialized_compatibility_only`
- `test_evidence_only`
- `frozen_reference_only`
- `unreachable_286_candidate`
- `shared_helper`
- `unresolved`

Only `unreachable_286_candidate` may enter an M50 deletion group. Reachable,
shared, serialized-layout, lifecycle, diagnostic, test-evidence, frozen, and
unresolved items must be retained or explicitly deferred.

## Machine-readable inventory

Add the deterministic golden:

```text
tools/qa/golden/upd9002_286_reachability_m49.csv
```

Its required columns are:

```text
candidate_id
symbol_or_field
kind
defining_file
disposition
final_dispatch_reachable
secondary_dispatch_reachable
directly_called
address_taken
macro_referenced
active_state_read
active_state_write
active_state_address_taken
serialized_offset
serialized_size
import_dependency
export_dependency
reset_dependency
cpu_shut_dependency
diagnostic_stop_dependency
io_dependency
test_dependency
frozen_reference_dependency
proposed_deletion_group
evidence
```

The generator must use deterministic ordering, contain no absolute path or
runtime address, fail if a known candidate or edge is omitted, fail on unknown
or partially parsed syntax, reject duplicate candidates and unresolved
generation, and reproduce the committed CSV byte-for-byte in check mode.

## M50 group policy

Produce three explicit lists:

A. Proposed M50 deletion groups, each requiring exact maintainer approval at
G49.

B. Deferred or rejected groups, with the exact blocking dependency.

C. Unresolved candidates, with the evidence still required.

An empty proposed list is valid. Do not invent groups merely because M50 is
scheduled.

For every proposed group record a stable ID, exact files/functions/tables/
macros/fields, incoming and outgoing dependencies, dispatch evidence, state
and lifecycle evidence, serialized-layout effect, retained replacement, test
and CMake changes, risk, and exact M50 acceptance proof. A group containing a
non-`unreachable_286_candidate` member is not eligible.

## Preservation requirements

M49 must preserve:

- the immutable M42 historical graph/provenance/support artifacts, trace,
  156-case harness, ABI, and state fixtures;
- the accepted post-M48 graph/provenance/support transition;
- M43 dataset identity, 40 known-gap selectors, 68,626 known-gap hashes,
  applicable pass/failure sets, every canonical failure signature, and zero
  timeout/crash;
- M44 layout, transactional adapter, accepted G41/current matrix, PE-clear
  opaque residue, and atomic rejection;
- M45 native-only execution and absence of `i286c_step()`;
- M46 one constructor, immutable six roots, and absence of block executors and
  CPU_EXEC aliases;
- M48 diagnostic text/path, 522-case atomicity, `MSW.PE` rejection, and
  transition manifest; and
- reset FLAGS `f002` and CPU_SHUT FLAGS `0000`.

No accepted baseline may be re-recorded or widened.

## Allowed changes

- this task/roadmap correction when required;
- audit documentation and ADR evidence;
- deterministic static-analysis and linker-map tooling;
- the machine-readable inventory;
- CTest wiring for audit-only checks; and
- the M49 report.

Production binaries must contain no M49-only seam or symbol.

## Validation

Run all supported clean-configure presets; GCC, Clang, and ASan/UBSan CTest;
tests-disabled production build and symbol inspection; MinGW and Wine where
available; hosted Linux, Windows, macOS, repository-invariant, and standalone
CI; every M42–M48 preservation gate; both mandatory M43 `--expect` profiles;
M44 state matrix and atomic rejection; M46 construction/immutability; the M48
522-case suite; M49 inventory regeneration/check/selftest; compiler/static/
linker reference evidence; repository encoding, EOL, case, unreferenced, and
diff checks; and final clean-worktree verification.

## Required report

Create:

```text
docs/agents/reports/m49_upd9002_protected_mode_reachability.md
```

Record branch/start/final/remote SHA, commits/files, complete post-M48 graph,
direct/address/macro evidence, runtime-versus-serialized field matrix,
lifecycle/I/O/test dependencies, inventory digest, proposed/deferred/unresolved
groups, retained serialization replacements, all M42–M48 results, production
isolation, exact command statuses, hosted CI, deviations/risks, exact G49 group
approval choices, and the human checklist.

Stop after presenting the M49 evidence and proposed groups. Do not delete any
group, begin M50/M51, or declare G49 passed.
