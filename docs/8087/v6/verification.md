# Executable verification and reporting contract

> Applies only to Stage B, after the explicit user OCR handoff and acceptance in
> [goal-contract.md](goal-contract.md) section 0. Stage A must not execute this work.

Normative with [goal-contract.md](goal-contract.md). This pack supplies instructions, not a
preexisting emulator test runner. P00 must implement the thin gate adapter described below
using the actual repository build/test mechanisms; later packages add real tests to it.

## 1. Gate interface to create during implementation

Use Python 3 standard-library orchestration unless an existing equivalent repository gate
can provide the same interface without duplication:

```sh
python3 tools/8087/gate.py --list
python3 tools/8087/gate.py --package P00
python3 tools/8087/gate.py --through P07
python3 tools/8087/gate.py --final
```

These commands are requirements for the implementation, not commands claimed to work before
P00. If that path already exists, extend it compatibly or record a precise alternate command
in the work plan. Never overwrite an existing gate blindly. Forward argument arrays safely,
retain exit status/stdout/stderr, and do not use `shell=True` with interpolated paths.

`--package` runs that package and validates its prerequisite records. `--through` reruns the
relevant completed ladder. `--final` executes every mandatory final check; it cannot reuse
stale PASS strings. A not-yet-implemented future package appears pending, not passed. P00 is
not required to pass future package gates. The runner must reject unknown IDs, missing
commands, zero discovered tests, nonzero exits, and skipped required tests.

Store detailed logs/artifacts in the repository's ignored build/test directory, not arbitrary
private paths. Commit compact public summaries and hashes under `docs/8087/v6/records/`.
Exclude self-referential report files and timestamps from the tested-source digest; include
production sources, build options, test definitions, oracle corpus, relevant generated
inputs, and the gate's code/configuration. Record HEAD plus dirty-source digests when needed.
A commit made after a gate does not automatically invalidate identical tested source bytes.

## 2. Required test layers

| ID | Layer | Required proof | First package |
|---|---|---|---|
| V00 | Baseline | Actual VAEG builds/tests and reproducible preexisting failures | P00 |
| V01 | Inventory | Independent required forms versus production table; corruption rejection | P01 |
| V02 | Backend | Exact vendored SoftFloat testsoftfloat and wrapper smoke | P02 |
| V03 | Raw state | Raw80 preservation, stack/TOP/tags, CW/SW and backend context restoration | P03 |
| V04 | Clock | Exact rational conversion, residue, overflow, scheduler fixture | P04 |
| V05 | CPU/bus | Native decoder slots, absent/read latch, FPO2, 8080 unchanged | P05 |
| V06 | Exceptions | Per-class transaction, masks/IEM/IR/B, WAIT/FN recovery | P06 |
| V07 | Vertical slice | Real CPU guest detects/initializes/adds/stores at multiple clocks | P07 |
| V08 | Transfers | Every real/int/BCD form, boundaries, invalid store/pop effects | P08 |
| V09 | Arithmetic | Direction/pop/alias, PC/RC, overflow/underflow/specials | P09 |
| V10 | Compare/admin | Conditions, projective/affine, constants, stack and control | P10 |
| V11 | Guest images | Independent 14/94-byte images, restore, pointer/opcode rules | P11 |
| V12 | Special arithmetic | FPREM loop/old quotient bits, scale and extract boundaries | P12 |
| V13 | Oracle | Certified reference intervals, exact inputs, prototype error budget | P13 |
| V14 | Log/exp | F2XM1/FYL2X/FYL2XP1 architecture and certified-corpus tests | P14 |
| V15 | Partial trig | FPTAN pair and FPATAN operand/order tests, numerical corpus | P15 |
| V16 | Exception closure | Every applicable form/class linked to executed tests | P16 |
| V17 | Machine route | Guest interrupt handler, mode transitions, timers, actual wiring | P17 |
| V18 | Lifecycle/UI | Reset-time settings, clock-aware savestate, legacy load policy | P18 |
| V19 | Final audit | Fresh zero-omission report, full integration, CI and provenance | P19 |

Run all new required tests locally on the active host. Run platform-specific tests where
supported; report unsupported ones explicitly outside the local required set. P00 freezes
the available-host gate list and current CI matrix, so later failures cannot be hidden by
reclassifying a required local test as unsupported.

## 3. Independent guest harness

Reuse the normal CPU and memory bus in a headless fixture. Use self-authored, redistributable
native guest byte programs, or source-built test binaries with recorded assembler version
and inspected bytes. A private BIOS, BASIC, ROM, or DOS installation must not be required
for public CI. A fixture may provide a minimal self-authored initialization environment,
but may not replace CPU execution with direct calls into arithmetic handlers.

The early slice must include real absent/present detection through sentinel memory,
initialization, two loads, arithmetic, a store, memory/status checks, and normal CPU exit.
Execute the same slice with absent FPU, enabled default clock, and another active clock.
The final harness also installs and runs a guest interrupt handler through the production
VA adapter and emulated interrupt controller, clears the condition, returns, and continues.
Test failures must reach a nonzero host test result, not only a guest screen message.

## 4. Traces, determinism, and undefined bits

Serialize exact raw values, defined machine fields, operation bytes, and event sequence with
explicit endianness. Never hash C struct padding or host locale-formatted floats. Separate
semantic traces from timing traces; different configured clocks legitimately change timing.

Source-undefined bits follow a frozen deterministic policy. A hardware comparison may mask
source-undefined fields and report that mask, but cross-host software determinism must still
compare the software's chosen values. Do not mask a defined field to remove a mismatch.

Produce trace artifacts with semantic schema version, policy IDs, source/corpus digest,
active clock, host/compiler metadata outside the semantic payload, and SHA-256. Aggregate
actual supported-architecture artifacts in CI or compare them to an independently validated
baseline. A matrix of unrelated green jobs is not cross-host equality. A missing second
host leaves `cross_host=NOT_RUN`, not an invented successful comparison.

## 5. Exception and state stress

Use full PC/RC/IC and exception-mask coverage where applicable, not only reset CW. Test
TOP wrap, all logical stack indices, full/empty stack, destructive destinations, old raw
encodings, BCD sign/range, pointer exemptions, and memory bus widths. Exercise a pending
unmasked exception through recovery instructions and interrupt masking.

State tests include toggling requested settings without applying them; reset applying exactly
once; FINIT leaving active clock and requested settings alone; savestate with nonzero residue;
pending interrupt restore without duplicate edges; absent-device legacy states; incompatible
payload rejection without partial mutation; and repeated enable/reset/load without a second
device or callback. Run sanitizers and compiler warnings where already supported; neither a
missing sanitizer nor a debugger screenshot substitutes for semantic tests.

## 6. Provenance and shipping checks

Hash vendored files against the approved archive. Inspect actual link commands/dependencies
for forbidden runtime MPFR/GMP/TestFloat/libm/host-x87 use. Restrict a no-host-FP audit to the
new FPU runtime so existing renderer/audio code is not broken. Static scanning is advisory
unless it is syntax-aware enough to distinguish SoftFloat type names from native floats.
Check licenses and generated constants/vectors. Do not auto-approve a binary simply because
its filename looks like a test; record origin and redistribution status.

## 7. Final report schema

Create a machine-readable report and a short Markdown explanation, with these fields:

```text
completion_class
source_digest, head, dirty_source_digest
local_host, compiler, build_options
packages_completed, required_packages_pending
required_tests_expected, run, passed, failed, skipped
instruction_coverage_counters
clock_default_hz, clock_range_hz, device_limit, execution_policy
numerical_policy, oracle_versions, corpus_hashes, worst_case_errors
production_interrupt_route, route_evidence_ids, guest_handler_test
cross_host, real_va_hardware, silicon_numerics, cycle_accuracy
baseline_regressions, preexisting_failures
provenance_audit, private_asset_audit
remaining_evidence, exact_resume_command
```

A preparation pack checksum is not a tested emulator source digest. An oracle generator
passing is not production-handler validation. A hard blocker remains visible even when all
independent packages pass. Final prose must agree with machine-readable counters.

## v6 provenance and human gate

Before P00, verify the actual acquisition receipts and accepted user OCR as specified in
[ocr-handoff.md](ocr-handoff.md). Before P01 freezes critical tables, compare their page
images to supplied OCR and record unresolved notation; do not run OCR as a repair.
Acquisition checks and helper unit tests must never be counted as emulator semantic tests.
Report optional E8087, other-emulator comparison and silicon evidence separately, with
NOT_RUN/UNAVAILABLE where true. They do not waive mandatory local instruction or VA wiring
gates, and missing optional references do not create new mandatory dependencies.
