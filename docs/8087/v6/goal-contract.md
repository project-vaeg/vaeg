# VAEG + Intel 8087 — Implementation contract v6

**Document status:** implementation instructions; no implementation or gate is claimed passed.
**Target:** the checked-out `vaeg` repository, not a new standalone emulator.
**Required work:** Stage A (A00-A03), mandatory human OCR handoff, then separately authorized Stage B (P00-P19).


## 0. Highest-priority v6 workflow boundary

**The default activation is ACQUISITION ONLY.** This section overrides any later sentence
about continuing automatically, implementing, building, running gates, or not stopping
at documentation. Those sentences apply only to separately authorized Stage B.

1. Stage A executes [acquisition.md](acquisition.md), packages A00-A03. Download and place
   approved public originals; compute real hashes; create an OCR queue and handoff report.
2. The user performs OCR. Stage A must stop at `AWAITING_USER_OCR`, or at
   `ACQUISITION_INCOMPLETE_AWAITING_USER` when required downloads failed. Neither is an
   emulator completion claim. Finish the bounded acquisition attempt; do not retry forever.
3. Do not OCR, render-and-transcribe, extract PDF text, convert manuals into Markdown,
   unpack/build/vendor upstream archives, create the implementation gate runner, or alter
   emulator code before the user handoff. Downloaded HTML is retained as original HTML.
   Metadata reports are allowed; fabricated/manual-content Markdown substitutes are not.
4. Existing PDF text layers, pre-existing OCR, elapsed time, a marker file, successful
   downloads, or a model-generated approval do not open Stage B.
5. Stage B requires an explicit subsequent user authorization AND accepted user-provided
   OCR/source mapping under [ocr-handoff.md](ocr-handoff.md). Then execute P00-P19.
6. The mandatory no-agent-OCR rule remains in force in Stage B; report unclear passages
   with exact page IDs rather than launching OCR or guessing.

The normative acquisition companions are [acquisition.md](acquisition.md),
[ocr-handoff.md](ocr-handoff.md), [source-policy.md](source-policy.md), and
[reference-validation.md](reference-validation.md). The machine-readable download catalog
is `sources/catalog.json`. Unknown publication dates and file hashes remain unknown until
verified; public availability is not approval to redistribute scans or import source code.

Stage A's terminal handoff is successful only as that stage's bounded objective. The full
VAEG+8087 goal is still `NOT_STARTED` or `NOT_COMPLETE`. A missing optional E8087 binary,
external emulator, or silicon study does not block documented-instruction implementation.

## 1. Binding decisions and precedence

The latest user requirements fix the following, after the Stage A/human gate above. They supersede the earlier v2-v5
assumptions for this goal; they do not authorize deleting earlier files or implementation.

| Requirement | Binding value |
|---|---|
| Coprocessor type | Intel 8087; no later-x87 superset |
| Maximum installed devices | One 8087 per emulated machine |
| Absence | Optional device; disabled by default for backward compatibility |
| Default operating clock | `10000000` Hz, exactly 10 MHz |
| Clock control | Independent persisted integer-Hz setting; applies on machine reset |
| Clock meaning | Changes emulated service time, not only a label or host execution speed |
| Supported machine policy | PC-88VA2/VA3; original VA and VA-91 remain unavailable |
| CPU access policy | Native V-series execution only; keep the 8080 decoder independent |
| Instruction scope | Every documented 8087 instruction and documented encoding/operand form |
| Execution policy | `SERIAL_TIMED_V1`, specified in `clock-and-timing.md` |
| Shipping arithmetic dependency | Exact upstream Berkeley SoftFloat Release 3e |
| Host arithmetic | No host floating-point calculations in the new FPU runtime |
| Private material | Never copied into the public repository or public CI |

The clock is a user-selected **emulator operating parameter**. An 8087-1 speed grade is
not, by itself, evidence of the clock on a particular VA motherboard. Do not change the
10 MHz default after finding a generic shared-clock diagram. Record historical board
clock evidence separately.

Read root and directory-specific `AGENTS.md`, license, current roadmap, conventions,
build files, and applicable task documents. After explicit Stage B authorization, this user goal authorizes progressing through
multiple implementation packages rather than stopping after each package. It does not authorize bypassing applicable safety or repository
publication rules. If an actual governing rule prohibits that progression, identify its
exact text; do not silently edit it away. Earlier planning-only restrictions are superseded only in authorized Stage B;
they never override the mandatory Stage A handoff in section 0.

The normative companions are [architecture.md](architecture.md),
[clock-and-timing.md](clock-and-timing.md), [instruction-coverage.md](instruction-coverage.md),
[numerics-and-oracles.md](numerics-and-oracles.md), and [verification.md](verification.md).
[Source-register.md](source-register.md) distinguishes primary evidence, user requirements,
and emulator policies. A numerical tolerance or timing approximation here is a project
contract, never an assertion about unidentified silicon.

## 2. What must be delivered

Modify the actual CPU/memory/scheduler/device/configuration/savestate/frontend paths that
P00 finds. Deliver a normal VAEG build containing an optional usable 8087, a headless test
path that executes self-authored guest bytes through that same production CPU, regression
tests, deterministic vectors, reproducible gate commands, and user documentation.

Do not satisfy this goal with a SoftFloat demo, a second decoder used only by tests, a
library never called by VAEG, register-display scaffolding, or detection alone. The early
slice must detect, initialize, compute, store, and check a result through VAEG's CPU and
bus. The final test program must exercise every documented family and a real emulated
interrupt-handler path, not just a host callback counter.

All documented instructions remain mandatory, including packed BCD, interrupt-control,
environment/save/restore, rare operand widths, reverse/pop forms, FPREM, FSCALE, FXTRACT,
and F2XM1/FYL2X/FYL2XP1/FPTAN/FPATAN. An undocumented numerical corner may have an explicitly
recorded deterministic policy; a missing documented operation may not.

Provide the enabled checkbox and clock control in the existing settings frontend. The
configuration parser and reset-time apply path are mandatory even in a headless build.
The debug view is read-only and small: active/requested clock, raw registers, TOP/tags,
CW/SW, pointers, and pending/BUSY/INT. Do not build a general coprocessor framework or UI.

## 3. Completion and evidence axes

`SOFTWARE_VERIFIED` means the **required local software gates** passed for a specific
source digest and environment. It requires ALL of the following:

1. The real VAEG target builds; baseline regressions do not worsen; all new required tests
   run with nonzero expected counts and pass. Missing or skipped new tests are failures.
2. Independent source inventory, production decoder, reached handlers, and executed
   semantic tests agree: zero documented forms missing, partial, unsupported, or untested.
3. One optional device; default 10 MHz; valid custom clocks persist and apply on reset;
   elapsed virtual time scales correctly while arithmetic results remain invariant.
4. Native absence paths and FPO2 retain CPU byte/bus behavior; 8080 instruction execution
   is not replaced by native ESC/POLL handling.
5. Architecture, conversion, exceptional operands, PC/RC/IC/IEM, pending exceptions,
   WAIT/FN, exact guest state images, and clock-aware VAEG savestates pass the specified tests.
6. The integrated machine adapter connects the 8087's BUSY/INT semantics to the selected
   machine route with cited evidence. A guest handler is entered and returns correctly.
   A disconnected callback-only adapter is NOT sufficient for final completion.
7. All five transcendental operations satisfy the frozen deterministic numerical corpus
   gates and their separately checked architectural behavior. No runtime oracle linkage.
8. Public provenance is complete, vendored upstream files are unchanged, private artifacts
   are absent, generated documentation is fresh, and local reports identify the tested tree.
9. `tools/8087/gate.py --final` (created during implementation) passes and generates a
   final report matching the actual changed sources and tests.

Keep separate evidence fields: `cross_host`, `real_va_hardware`, `silicon_numerics`, and
`cycle_accuracy`. Values include `VERIFIED`, `NOT_RUN`, `PENDING_EVIDENCE`, and
`OUT_OF_SCOPE`. Cross-host equality may be claimed only after actual artifact comparison.
Never describe local-only testing as all-platform testing. Public CI configuration must
cover the supported matrix found at P00, but unavailable external runners need not stop
local coding. Existing mandatory CI policy still governs merge readiness.

A real VA or physical 8087 is not required to implement or test documented software
behavior. Missing physical hardware is not a blocker. However, if the **actual emulated
VA INT connection cannot be established from the available specifications/code evidence**,
finish all independent packages, retain `INTEGRATION_BLOCKED`, and give the smallest
missing evidence. Do not invent PC/AT IRQ13, NMI, or #MF, or mark this goal complete by
relabeling an unwired adapter as hardware work. No historical claim is required merely to
honor the user's 10 MHz configuration requirement.

A final `SOFTWARE_VERIFIED` report must still name the approximation `SERIAL_TIMED_V1`,
the numerical policy, and every remaining evidence limitation. It is not a claim of
cycle-exact bus concurrency or silicon-identical transcendental low bits.

## 4. Safe repository work

Start by recording HEAD, branch, dirty paths, submodule state where relevant, and baseline
commands. Inspect any existing 8087 work before adding a replacement. Reuse correct work;
repair it against this contract. Do not blindly remove previous branches or commits.

Keep all provided v6 documents under `docs/8087/v6/`; never overwrite `docs/8087/progress.md`
or other v2-v5 records. Create v6 implementation records under `records/` as work happens.
The root documentation may receive an additive link to the active v6 plan after its
existing content is reviewed. Do not apply a global search/replace across history.

Do not run `git reset --hard`, `git clean`, force pushes, broad reverts, or automatic
stashing. Preserve unrelated dirty files. Overlapping user edits require reconciliation;
stop only when no safe local reconciliation is possible. Make small local commits only
when repository policy permits; never push, open/merge a PR, modify remotes, or publish
artifacts without separate authorization. A dirty implementation tree is not a reason to
lose work: record its digest and exact diff.

Private sources may be read only from user-provided locations or already available local
documentation. Prefer existing `docs/88va/` descriptions before requesting more input.
Do not move private files, scan unrelated home directories, or copy private disassemblies,
ROM bytes, screenshots, disks, or traces into public files. Record permitted factual
conclusions with opaque source IDs; omit private absolute paths from committed reports.

## 5. Provenance and runtime scope

Use only the official SoftFloat-3e archive, with its computed hash, release, license, and
file manifest recorded. Ship the required permissive notices. Runtime source files remain
unchanged; custom build glue, generated platform header, wrappers, and symbol isolation
live outside the vendored directory. An imported header need not be edited to use C99
integer fallbacks. Audit both host endianness and supported integer operations.

Do not inspect, copy, translate, or port emulator FPU implementation code from DOSBox-X,
86Box, PCem, Bochs, QEMU, Linux wm-FPU-emu, bitblaze-fuzzball, or i486SX_soft_FPU; also exclude Open Watcom fpuemu, PCjs FPU implementation, and
Granite implementation/decoder/raw-microcode inputs. Follow source-policy.md. This is a
project provenance boundary, not a statement that merely viewing GPL code changes a
license. If the checked-out repository contains an old implementation from such a source,
record its existence and license without using its arithmetic as the implementation model.

New FPU runtime and helpers are C99. Existing C++ CPU/frontend boundaries may use thin
adapters according to repository practice. Do not downgrade the whole project or ban
pre-existing audio/rendering floating-point code. In the new FPU runtime prohibit native
`float`, `double`, `long double`, `_Float128`, `__float128`, libm, host x87, inline assembly,
and decimal floating constants used for numerical evaluation. Software `float128_t` from
SoftFloat is an integer-backed format, not native `_Float128`, and is permitted where
numerical prototypes justify it. Supply portable limb fallbacks instead of requiring
`__int128` on all supported compilers.

External MPFR/GMP and TestFloat tools are allowed for test-vector generation/build
validation only; they may never become shipping linkage, a user runtime dependency, or a
vendored runtime subtree. Self-authored generator sources and provenance-recorded public
vectors may be kept in test tooling; build the external tools outside runtime targets.
Audit versions and actual licenses rather than making unsupported legal statements.

## 6. Stage B working loop and stop policy

Use the stable internal package IDs P00-P19; do not guess the next global M-number. Map
IDs only if repository conventions require it. Before each package, record its actual
source paths, commands, dependencies, and impacted tests. Read only that package and the
needed contracts after the initial overview; avoid repeatedly rewriting all documents.

Implement, run targeted tests plus impacted baseline checks, inspect diffs, repair failures,
and update `records/progress.md`. Run the full baseline at P00, P07, P17, and P19, and
whenever changes touch CPU/bus/scheduler broadly. Do not rebuild every platform after each
comment edit. Do not weaken a frozen gate, rewrite expected results from the DUT, or reduce
test counts to make a package pass. A source-backed correction requires a separately
recorded decision and before/after regression evidence.

Only in authorized Stage B: continue automatically after an implementation package passes. A bounded numerical prototype/design gate
is work inside the goal, not a new permission request. Independent read-only reviews may
run in parallel if the installed tools support them, but give one owner write access to
CPU/scheduler and do not depend on unavailable subagent tools.

In Stage A, stop unconditionally at the human handoff. In Stage B, stop for
genuinely unavailable required dependencies/evidence, irreconcilable user
edits, a required semantic decision that cannot be defensibly resolved, an unrepairable
required test environment, or a system/budget interruption. For a local blocker, continue
independent packages first. Report attempted alternatives, actual errors, tested digest,
remaining mandatory gates, and the smallest input needed. Budget exhaustion is `PAUSED`,
not completion. On resume, revalidate changed prerequisites instead of recreating the tree.
