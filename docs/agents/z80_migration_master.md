# vaeg Z80 core migration — Codex milestone specification

## Purpose

This document defines the complete, milestone-driven migration of the PC-88VA
subsystem Z80 implementation in `vaeg` from the M88/cisc-derived code under
`cpucva/` to an approved, minimally extended version of the MIT-licensed
`suzukiplan/z80` core.

Repository:

- `https://github.com/nakatamaho/vaeg`

Candidate core:

- `https://github.com/suzukiplan/z80`

This is a multi-milestone master specification. It is **not** permission to
implement all milestones in one session.

Codex must execute exactly one milestone per session and stop at that
milestone's gate. Never begin the next milestone until the maintainer explicitly
states that the current gate passed.

M33 has already been completed or assigned in the current roadmap, so this
plan starts at M34 and uses M34 through M41, with optional M42. Before
starting, inspect the current `docs/agents/ROADMAP.md`. If M34 or any later
identifier in this sequence has already been used, renumber the entire
sequence contiguously to the next available IDs. Update every task filename,
branch name, table entry, dependency, heading, cross-reference, commit prefix,
gate identifier, and report reference consistently without changing the
dependency order.

---

# 1. Mandatory repository workflow

Before every milestone:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md`.
3. Read `docs/agents/CONVENTIONS.md`.
4. Read the single assigned `docs/agents/tasks/M*.md`.
5. Read the Z80 migration ADR and evidence documents created by earlier
   milestones.
6. Run `git status --short`.
7. Record the exact starting commit with `git rev-parse HEAD`.
8. Confirm that the previous human or machine gate passed.
9. Work on one milestone branch only.
10. Stop after completing and reporting the current gate.

Use branch names of the form:

- `topic/m34-z80-contract`
- `topic/m35-z80-upstream-extension`
- `topic/m36-z80-vendor-conformance`
- `topic/m37-z80-wrapper`
- `topic/m38-z80-differential`
- `topic/m39-z80-integration`
- `topic/m40-z80-disassembler`
- `topic/m41-z80-cutover`
- `topic/m42-z80-performance` for the optional milestone

All source code, comments, identifiers, commit messages, test names, log
messages, and newly authored documentation inside the repository must be in
English.

Use one concern per commit. Commit subjects must use the current milestone
prefix, for example:

```text
M37: add the suzukiplan Z80 compatibility wrapper
```

Do not modify the frozen reference tier:

- `win9x/`
- `i286x/`
- `cpuxva/memoryva.x86`
- `hlp/`

Do not modify ROM images, disk images, fonts, icons, wave data, or other
protected binary payloads.

Do not rewrite Git history. Removing M88-derived sources from current HEAD and
release archives is the required distribution result. Historical commits are
out of scope.

---

# 2. Maintainer-approved decisions

The following decisions are already approved. Do not reopen them unless a
verified technical contradiction makes implementation impossible.

## 2.1 Selected core and licenses

- `suzukiplan/z80` is the selected candidate.
- Its MIT license is acceptable for vaeg.
- Vendored upstream or forked code retains its MIT license and copyright.
- Newly authored vaeg files use the repository-standard 2-clause BSD license
  header with `Copyright (c) 2026 Nakata Maho`.
- Third-party provenance must include the upstream repository, pinned commit,
  license, and any downstream fork commit.

## 2.2 Limited C++17 exception

The general C99 preference remains in force.

Add the following limited exception consistently to `AGENTS.md`,
`docs/agents/CONVENTIONS.md`, the Z80 migration ADR, and the applicable task
documents:

> C++17 is permitted for CPU backends and thin compatibility adapters under
> `cpucva/` when required by an approved third-party CPU core. C++ and C++ STL
> types must not cross existing C-facing subsystem or state-save interfaces.
> Other newly written emulator-core code remains C99 unless separately
> approved by an ADR.

Consequences:

- Do not perform unrelated C-to-C++ conversions.
- Do not use this migration to refactor unrelated core modules.
- The replacement backend and its thin adapters may use C++17.
- Public data crossing the state-save or C boundary must use fixed-width
  integer types and explicit serialization.
- `std::function` may be used internally by the third-party core initially,
  but it must not become part of vaeg's public subsystem contract.

## 2.2.1 Source-compatible vaeg API

Preserve the consumer-visible vaeg API while replacing its implementation.

Requirements:

- keep the public class name `Z80C`;
- preserve the signatures of every public method that M34 proves is currently
  used;
- a replacement header may use a new path, provided all consumers are migrated
  in the same milestone;
- M34 may authorize removal of public methods proven unused;
- `IMemoryAccess`, `IIOAccess`, `IClock`, and `IClockCounter` may be
  independently reauthored under the same names when retaining those names
  minimizes consumer changes;
- do not copy M88 comments, declaration layout, helper macros, or
  implementation structure when recreating those interfaces;
- no third-party core type, C++ STL type, or callback wrapper may cross the
  consumer-visible `Z80C`, subsystem, debugger, or state-save boundary.

The migration replaces the implementation completely while preserving the
source-level contract used by current vaeg consumers.

## 2.3 Interrupt acknowledge timing

The replacement must preserve the old externally observable acknowledge timing.

The interrupt-acknowledge byte must be read from the configured acknowledge
port only when the CPU actually accepts a maskable interrupt:

- after completion of the current instruction;
- after applying the EI inhibition rule;
- only when the IRQ line is asserted and IFF1 permits acceptance;
- exactly once per accepted interrupt;
- never when IRQ is merely asserted but cannot be accepted;
- never speculatively before executing an instruction.

Do not implement a pre-read / arm / cancel approximation in the vaeg wrapper.

Implement a minimal general-purpose extension in `suzukiplan/z80`, preferably
upstream, so the core itself determines the acceptance point and invokes an
interrupt-acknowledge callback.

## 2.4 Level-sensitive IRQ

The vaeg subsystem exposes a level-sensitive IRQ line:

- nonzero means asserted;
- zero means deasserted.

The approved upstream extension must provide a level-line API, or an equivalent
implementation whose externally observable behavior is the same.

A latched one-shot request alone is insufficient because it can preserve a
stale request after deassertion or clear a still-asserted level after one
acceptance.

Existing upstream interrupt APIs must remain backward compatible for upstream
users. Add a new level-sensitive API rather than silently changing the meaning
of an existing public API.

## 2.5 IM0 raw opcode

The same upstream extension must support the old IM0 contract:

- the byte returned by the acknowledge callback is the raw first opcode byte;
- the supplied first byte is not fetched from memory and does not advance PC;
- any following operand or prefix bytes are fetched normally from memory at
  the current PC;
- non-RST opcodes, including `0x7f`, are not silently replaced;
- prefixed and multi-byte cases must work according to the core's normal
  decoder, with all bytes after the acknowledge-supplied first opcode fetched
  from memory at the current PC;
- mandatory coverage includes CB, ED, DD, FD, DDCB, and FDCB sequences.

Do not convert all IM0 values to RST numbers.

Do not handle an unsupported value by clearing IFF and performing no
instruction.

IM1 and IM2 must also perform the acknowledge read at acceptance time. IM1
ignores the returned byte for dispatch; IM2 uses it as the vector low byte.

## 2.6 I/O port width

The old core masks all external I/O accesses to eight bits in its common I/O
helpers.

The replacement wrapper must therefore pass:

```text
port & 0xff
```

to vaeg's external I/O interface for every IN, OUT, and block-I/O form.

Do not preserve or expose the Z80's optional 16-bit composite port value at the
vaeg subsystem boundary.

## 2.7 Public register freshness

Preserve the old public-register observation semantics.

- The core may maintain a live internal PC.
- `GetPC()` may return the live internal PC.
- The `GetReg()` mirror must be synchronized at the same externally observable
  boundary as the old implementation, normally the end of `Exec()`.
- I/O callbacks during an `Exec()` slice must not unexpectedly begin seeing a
  live PC through `GetReg()`.
- `SaveStatus()` must serialize authoritative live state, not a stale
  diagnostic mirror.

Do not change the `SleepCheck_VA` or `SleepCheck_Sorcerian` constants merely
because a new implementation exposes a different PC. Preserve the old
observation boundary first. Change those constants only if trace evidence
proves that preservation is impossible or that the old condition is already
wrong.

## 2.8 State-save model and compatibility scope

Preserve the state-save model already used by vaeg.

### Machine-level save boundary

A save operation is initiated only after the current frontend/core frame
execution call has returned. In the SDL2 frontend this is the stable point
after the current `pccore_exec()` call, or its current equivalent, has
completed.

This does not mean that vaeg automatically saves every frame. It means that a
user- or system-requested save is serviced at the existing frame-call
boundary, when the emulated machine is not being advanced by the current
frame execution call.

At that boundary:

- no `Z80C::Exec()` call is active;
- no Z80 memory, I/O, clock, or interrupt-acknowledge callback is active;
- no Z80 instruction is partially executed;
- the previous Z80 execution slice has returned;
- the rest of the machine is saved through vaeg's existing section-based
  state-save coordinator.

M34 must verify and document the current call path. Do not redesign the global
state-save scheduler as part of this migration.

### Z80 save resolution

The Z80 subsystem is saved at the following resolution:

```text
completed Z80 instruction boundary
+
architectural CPU state
+
HALT / external WAIT / IRQ state
+
remaining clock balance (`remainclock`)
+
last synchronized clock (`lastclock`)
```

`Z80C::Exec()` may finish the last complete instruction even when that
instruction overshoots the available clock budget. The resulting positive,
zero, or negative clock balance is part of the saved scheduling state and must
be restored.

The save format does not represent:

- a partially executed instruction;
- an intermediate memory or I/O bus cycle;
- an opcode-prefix decoder halfway through an instruction;
- a callback stack;
- an interrupt-acknowledge operation in progress;
- arbitrary third-party-core microstate that cannot survive an `Exec()` return
  boundary.

Do not add machinery to serialize those out-of-scope states.

### Authoritative state at save time

`SaveStatus()` must construct the save image from authoritative state at the
frame-call boundary.

Required behavior:

- materialize any deferred flags into architectural AF before saving;
- save the live PC rather than a potentially stale diagnostic mirror;
- save all architectural registers represented by the legacy format;
- save IFF1, IFF2, interrupt mode, I, and R;
- save the IRQ line state;
- save HALT and external WAIT state;
- save `remainclock` and `lastclock`;
- write the existing internal status revision and size when compatible with
  the current vaeg build family.

The public `GetReg()` mirror may retain the old `Exec()`-boundary freshness
semantics used by SLEEP_HACK. That diagnostic mirror is not the authoritative
source for serialization.

### Revision-1 compatibility

The target is the existing revision-1 Z80 status representation used by vaeg.

Required:

- a revision-1 save produced by the legacy core in the same supported host
  build family must load under the new core;
- the new core must write the same revision-1 Z80 status size for that build
  family;
- a new-core save must reload under the new core;
- save/load during ordinary FDD activity must resume without corruption;
- HALT and external WAIT observable at the frame-call boundary must resume
  correctly.

Not required:

- save/load from inside `Z80C::Exec()` or a callback;
- cross-architecture or cross-compiler save portability not already provided
  by vaeg;
- new-core save data loading under an old released core;
- preservation of unreachable third-party internal states;
- a general-purpose Z80 microstate serializer.

Use explicit field mapping between the legacy byte image and the new runtime
state. Do not make the compiler-dependent legacy structure the authoritative
runtime representation.

### EI boundary check

EI inhibition is the only known between-instruction condition that requires a
focused check because it may survive after the EI instruction itself.

Add one ROM-less regression that:

1. arranges for the frame-call save point to occur at every `Z80C::Exec()`
   return shape that the wrapper can expose around EI;
2. saves at the normal frame-call boundary;
3. reloads;
4. verifies the next retired instruction, IRQ-acceptance timing,
   architectural state, external bus trace, and clock balance.

This is a boundary-compatibility test, not a requirement to serialize arbitrary
microstate.

If the new wrapper can expose an EI-inhibited state at the normal save point,
either preserve that single reachable condition within the existing
revision-1-compatible representation or arrange `Exec()` scheduling so that
the condition is not exposed, provided normal vaeg scheduling remains
unchanged.

If the normal production frame-call boundary cannot be restarted correctly
with revision 1, stop and request a separate maintainer decision. Do not
silently create revision 2.

Pending NMI state is required only if M34 finds a real vaeg path that can leave
a pending NMI at the normal frame-call save boundary.

### Required fixtures

Match vaeg's current save portability expectations.

For each actively supported host build family used in CI or maintainer
regression, retain a small authentic legacy-core fixture set generated by that
same architecture/compiler family.

At minimum cover:

- ordinary running state;
- alternate registers;
- HALT;
- external WAIT;
- asserted IRQ at an `Exec()` return boundary;
- nonzero or negative `remainclock`;
- nonzero `lastclock`;
- the focused EI boundary case.

Do not claim portability across Linux, macOS, Windows, 32-bit/64-bit, different
compilers, or different build families unless vaeg already guarantees it and
authentic fixtures prove it.

## 2.9 Removal scope

The maintainer explicitly approves deletion of the following seven
M88-derived files after their consumers have been migrated and the cutover gate
passes:

```text
cpucva/types.h
cpucva/z80.h
cpucva/z80if.h
cpucva/z80c.h
cpucva/z80c.cpp
cpucva/z80diag.h
cpucva/z80diag.cpp
```

This approval satisfies the repository rule requiring an explicit
maintainer-approved deletion list.

If the inventory discovers additional M88/cisc-derived files, report them and
obtain explicit approval before deleting them.

## 2.10 Release policy

The final release must not contain the legacy core as a fallback.

The old implementation may remain temporarily selectable only during
development and differential testing. The final cutover removes:

- the legacy core;
- the legacy disassembler;
- the seven approved files;
- the legacy build option;
- release packaging references to those sources.

---

# 3. Facts that must be reverified against the starting HEAD

These are working facts, not permission to skip verification. Record exact
file and line references in the M34 evidence.

1. `Z80C::Exec()` accumulates elapsed clock into the remaining-clock counter,
   drains it without executing while external WAIT is active, otherwise loops
   while remaining clock is positive, executes the old core, checks IRQ, and
   synchronizes the public PC mirror at the end of the slice.
2. The old common I/O helpers mask ports with `& 0xff`.
3. The old interrupt handler reads the acknowledge port only after IFF1 and
   the IRQ level permit acceptance.
4. In old IM0, the acknowledge byte is passed as a raw opcode to the normal
   instruction decoder.
5. The old EI implementation internally executes the following instruction to
   model the delay. This means internal "one step" boundaries are not
   necessarily comparable one-for-one with the replacement.
6. HALT PC representation differs between the old implementation and
   `suzukiplan/z80`.
7. The subsystem uses a public register mirror from I/O callbacks for the
   SLEEP_HACK paths.
8. The seven approved files contain M88/cisc-derived definitions or
   implementation.
9. Determine whether `NMI()`, diagnostic statistics, dump support, `TestIntr`,
   `IsIntr`, and every other public method have external consumers.
10. Determine the exact compiler ABI, offsets, padding, endian assumptions,
    and byte count of the old serialized status on every currently supported
    build.

If a working fact is wrong, correct the ADR and task documents before
implementing code that depends on it. Do not continue on a false premise.

---

# 4. Target architecture

The final active tree should have an independently authored vaeg compatibility
layer and a pinned MIT third-party backend.

Preferred new files, subject to M34 dependency findings:

```text
cpucva/z80_compat_bus.h
cpucva/z80_compat_registers.h
cpucva/z80_compat_state.h
cpucva/z80_compat_state.cpp
cpucva/z80_compat_cpu.h
cpucva/z80_compat_cpu.cpp
cpucva/upd780_disasm.h
cpucva/upd780_disasm.cpp
```

Names may be adjusted once in M34, but after the contract gate they must remain
stable unless a concrete build conflict requires a documented change.

Design rules:

- Do not copy comments, table layout, implementation structure, or unusual
  declarations from the M88-derived files.
- Standard fixed-width integer types replace M88 integer aliases.
- The runtime register model is separate from the legacy serialized byte
  format.
- State save follows vaeg's existing frame-call boundary; the Z80 image
  represents a completed instruction boundary plus `remainclock` and
  `lastclock`, not arbitrary microstate.
- The public register mirror is separate from the third-party core's internal
  register object.
- vaeg-side I/O and memory adapters force the required address and width
  masking even if the third-party core has configurable alternatives.
- The vendored third-party directory is never hand-edited.
- Any required third-party modification exists as an upstream or fork commit
  before vendoring.
- No third-party C++ or STL type crosses the vaeg subsystem/state boundary.
- The old and new cores may coexist only through explicit build selection
  during M39 and earlier tests.
- The final tree contains only the new core path.

---

# 5. Milestone map

| Milestone | Deliverable | Default core after milestone | Gate |
|---|---|---:|---|
| M34 | Inventory, contracts, ADR, conventions exception, baseline fixtures | legacy | Human review |
| M35 | Minimal upstream/fork IRQ-acknowledge and raw-IM0 extension | legacy | Human + upstream/fork commit |
| M36 | Pinned vendoring and standalone conformance CI | legacy | Machine + review |
| M37 | New vaeg interfaces, register/state model, and wrapper unit tests | legacy | Machine |
| M38 | Normalized old/new differential harness and cycle evidence | legacy | Machine + review |
| M39 | Opt-in subsystem integration and private system regression | legacy | Human |
| M40 | New disassembler and removal of active consumers of legacy headers | legacy/new selectable | Human + machine |
| M41 | New default, delete all seven M88-derived files, remove fallback | new only | Full human + release audit |
| M42 | Optional profiling-driven performance configuration | new only | Machine + human smoke |

Dependency order is strict:

```text
M34 -> M35 -> M36 -> M37 -> M38 -> M39 -> M40 -> M41 -> optional M42
```

---

# 6. M34 — inventory, contracts, and migration ADR

Task file:

```text
docs/agents/tasks/M34_z80_migration_contract.md
```

Branch:

```text
topic/m34-z80-contract
```

## Goal

Create a verified, reviewable contract before implementation. Introduce no
guest-visible behavior change.

## Required work

### 6.1 Update the roadmap

Add M34 through M41 and optional M42 to
`docs/agents/ROADMAP.md`. Do not recreate, rename, or overwrite the already
completed/assigned M33 entry.

Create the task files for the complete sequence. The task files must remain
consistent with this master specification, but each task file must be
self-contained enough for a future Codex session that has not seen this
document.

### 6.2 Add the limited C++ exception

Update:

- `AGENTS.md`
- `docs/agents/CONVENTIONS.md`

Use the exact intent from section 2.2. Do not broadly authorize C++ in the
entire core.

### 6.3 Create the migration ADR

Determine the next available ADR/decision identifier under
`docs/agents/DECISIONS/`.

The ADR must include:

- selected core and MIT acceptance;
- limited C++ exception;
- all seven approved legacy files;
- complete consumer/include/symbol inventory;
- interrupt-acknowledge timing;
- level-sensitive IRQ contract;
- raw IM0 first-opcode contract;
- eight-bit external I/O contract;
- clock and `Exec()` slicing contract;
- EI-boundary incompatibility;
- HALT representation;
- public `GetReg()` freshness contract;
- state-save compatibility scope;
- ZEX acquisition and license policy;
- disassembler strategy;
- dual-core development period;
- final no-fallback release policy;
- upstream-first, minimal-fork fallback policy.

### 6.4 Complete consumer and provenance inventory

Use grep, compiler dependency output, and CMake source lists. Record every
consumer of:

- `types.h`;
- `Z80Reg`;
- `IMemoryAccess`;
- `IIOAccess`;
- `IClock`;
- `IClockCounter`;
- `Z80C`;
- `Z80Diag`;
- all `Z80C` public methods;
- status-save hooks;
- subsystem disassembly hooks.

For each symbol, record:

- definition;
- include path;
- direct consumers;
- transitive consumers;
- whether it survives in the new contract;
- planned replacement file;
- milestone that migrates it.

Do not assume `iova/subsystem.cpp` is the only consumer until grep and link
analysis prove it.

### 6.5 Verify legacy execution behavior

Create an evidence document such as:

```text
docs/modernization/z80-compatibility-legacy-contract.md
```

Include exact source references for:

- `Exec()` scheduling;
- WAIT behavior;
- IRQ level storage;
- interrupt acceptance;
- IM0, IM1, and IM2;
- EI and DI;
- HALT and wakeup;
- NMI;
- memory and I/O masks;
- public PC synchronization;
- state-save layout and revision.

Explicitly distinguish:

- architectural Z80 behavior;
- old-core implementation behavior;
- behavior that vaeg externally observes;
- old-core behavior that is probably an implementation defect and should not
  automatically be preserved.

### 6.6 Frame-call and Z80 save-resolution baseline

Document the complete production call path from the frontend save request to
`statsave_save()` and the Z80 `SaveStatus()` callback.

Confirm:

- the current frame/core execution call has returned before saving starts;
- `Z80C::Exec()` cannot still be active;
- no Z80 callback can still be active;
- the Z80 is at a completed instruction boundary;
- clock overshoot/debt is represented by `remainclock`;
- `lastclock` is saved and restored;
- live PC and materialized architectural flags are used for the save image.

Add a ROM-less test or small test-only utility that reports for the current
host build family:

- `sizeof` of the old public register structure;
- `sizeof` of the old Z80 status structure;
- `offsetof` for every serialized field;
- size of `bool`, `uint`, and relevant aliases;
- byte order;
- compiler and architecture.

Capture deterministic legacy-core status byte fixtures at the normal
frame-call boundary for at least:

- ordinary running state;
- alternate registers populated;
- IFF1/IFF2 and interrupt mode values;
- HALT;
- external WAIT;
- asserted IRQ after `Exec()` has returned;
- nonzero or negative `remainclock`;
- nonzero `lastclock`;
- the focused EI boundary case.

Fixtures must be generated from hand-assembled ROM-less programs, not
copyrighted guest ROMs.

Do not attempt to save inside `Z80C::Exec()` or a callback. Do not replace
production serialization in M34.

### 6.7 SLEEP_HACK baseline

Document the call chain for both sleep checks and when `GetReg()->pc` is read.

Add optional trace instrumentation guarded by a test/debug-only option if
needed, but do not change release behavior.

Specify the private human test that will later prove:

- VA idle sleep still engages;
- Sorcerian sleep still engages;
- the ATN/8255 wake path still releases external WAIT.

Do not require copyrighted ROM or disk data in public CI.

### 6.8 ZEX policy

Determine the exact license and provenance of the chosen ZEXDOC/ZEXALL
artifacts.

Preferred policy:

- download only in the dedicated conformance CI job;
- pin the source URL or commit;
- verify SHA-256 before execution;
- do not include ZEX binaries in normal source or release archives;
- document how to run the test offline with a user-provided cache.

If the chosen artifact cannot be redistributed or reliably fetched under this
policy, stop and propose a legally sound alternative.

### 6.9 Disassembler feasibility

Inspect the selected pinned candidate version's dynamic debug/disassembly
facilities.

Decide and record one strategy:

1. use a side-effect-free upstream decoder API if it already exists;
2. propose a separate minimal upstream decoder API;
3. write an independently authored BSD-2-Clause vaeg disassembler from
   documented instruction encodings.

Do not copy the old M88/cisc tables. Do not copy tables from GPL code.

### 6.10 New file and API contract

Finalize the new file names and the exact public contracts.

Prefer fixed-width types. Avoid exposing STL.

The contract must cover:

- memory read/write callbacks;
- I/O read/write callbacks;
- clock source;
- remaining-clock accounting;
- IRQ line assertion/deassertion;
- NMI request;
- interrupt acknowledge callback;
- `Exec()`;
- reset;
- external WAIT;
- live PC;
- public register mirror;
- state save/load;
- disassembler access;
- intentionally retained inert diagnostics.

## Out of scope

- no vendoring;
- no new CPU implementation;
- no production wrapper;
- no subsystem integration;
- no deletion;
- no release behavior change.

## QA

Run all current repository invariant checks.

Build the unmodified emulator on the current host using the documented preset.

Run the existing CTest and headless smoke gate.

Verify that the only source-tree behavior changes are documentation and
test-only ABI/evidence tooling.

## Human gate G34

Present:

- exact starting and ending SHAs;
- ADR;
- all task files;
- consumer inventory;
- state ABI report;
- legacy fixtures;
- corrected verified facts;
- proposed upstream API;
- ZEX policy;
- disassembler decision;
- confirmation of the approved seven-file deletion list.

Stop. M35 may begin only after explicit maintainer approval.

---

# 7. M35 — minimal suzukiplan interrupt extension

Task file:

```text
docs/agents/tasks/M35_suzukiplan_irq_extension.md
```

Branch in vaeg for evidence only:

```text
topic/m35-z80-upstream-extension
```

The actual third-party change must be developed against a clean
`suzukiplan/z80` checkout or approved fork, not by editing a vendored vaeg
copy.

## Goal

Produce a minimal, general-purpose MIT-licensed extension that lets the core
model vaeg's interrupt contract without speculative wrapper behavior.

## Required upstream API behavior

The exact API names may follow upstream style, but the semantics must include:

### 7.1 Level IRQ line

Add a new level-sensitive IRQ API.

Example semantic shape:

```cpp
void setIRQLine(bool asserted);
```

Requirements:

- assertion persists until explicitly deasserted;
- acceptance clears IFF1 as normal but does not silently deassert the external
  line;
- while the line remains asserted, a later EI/RETN that reenables interrupts
  can permit another acceptance;
- deassertion before acceptance prevents the interrupt;
- existing `generateIRQ`-style APIs remain source-compatible and retain their
  existing semantics.

### 7.2 Interrupt acknowledge callback

Add a callback invoked only at actual maskable-interrupt acceptance.

Requirements:

- exactly one callback invocation per accepted maskable interrupt;
- no invocation while DI is effective;
- no invocation during the instruction inhibited by EI;
- no invocation for NMI;
- no invocation for asserted-but-unaccepted IRQ;
- no speculative invocation before the current instruction finishes;
- callback result is an unsigned eight-bit value.

Support both existing callback configurations:

- normal `std::function` build;
- `Z80_NO_FUNCTIONAL` function-pointer build.

### 7.3 IM0 raw first opcode

At IM0 acceptance:

- obtain the first opcode from the acknowledge callback;
- execute that byte through the normal instruction decoder without reading the
  first opcode from memory;
- leave PC pointing to the first memory byte after the externally supplied
  opcode;
- fetch any operands or following prefix bytes from memory;
- update R and flags consistently with the core's normal rules;
- account for interrupt-acknowledge and instruction cycles according to the
  core's documented timing model;
- do not restrict the byte to RST values.

### 7.4 IM1 and IM2

At IM1 acceptance:

- call the acknowledge callback once;
- ignore its value for dispatch;
- enter `0x0038`.

At IM2 acceptance:

- call the acknowledge callback once;
- use the returned byte with I to form the vector-table address;
- preserve normal wrapping and memory-read order.

### 7.5 HALT and EI

Test:

- IRQ accepted from HALT;
- resume PC is correct;
- acknowledge callback timing is after the EI-inhibited instruction;
- an instruction immediately following EI can change the callback result, and
  the newly changed value is observed;
- IRQ deasserted during the inhibited interval is not accepted and does not
  invoke acknowledge.

## Required third-party tests

Add focused tests in the upstream/fork repository for:

1. DI with asserted level;
2. EI delay;
3. vector value changed by the instruction after EI;
4. deassert before acceptance;
5. persistent asserted line across a later EI;
6. IM0 `RST`;
7. IM0 `NOP` (`0x00`);
8. IM0 `LD A,A` (`0x7f`);
9. IM0 multi-byte control transfer such as `CALL nn` where operand bytes come
   from memory at PC;
10. IM0 CB-prefixed instruction, with the second opcode fetched from memory;
11. IM0 ED-prefixed instruction;
12. IM0 DD-prefixed instruction;
13. IM0 FD-prefixed instruction;
14. IM0 DDCB sequence, with displacement and final opcode fetched from memory;
15. IM0 FDCB sequence, with displacement and final opcode fetched from memory;
16. IM1 acknowledge read;
17. IM2 acknowledge read and vector memory order;
18. HALT wake;
19. NMI not invoking the IRQ acknowledge callback;
20. normal and `Z80_NO_FUNCTIONAL` builds.

Retain and run the upstream's existing full test suite.

## Scope control

Do not include vaeg-specific class names, ports, save formats, or clock
interfaces in the third-party patch.

Do not add the disassembler API to the same commit unless the M34 ADR proved
that it is inseparable. Interrupt acceptance and raw IM0 form one focused
change; disassembly is a separate concern.

## Upstream-first process

Preferred:

1. implement and test against an exact upstream base SHA;
2. commit the change in an accessible fork;
3. open an upstream issue or pull request where practical;
4. record base SHA, fork SHA, issue/PR URL, and test logs.

If Codex cannot push to an approved fork:

1. generate a directly applicable `git format-patch`;
2. place a copy under an evidence path approved in M34;
3. record the exact upstream base SHA;
4. include application and test commands;
5. stop at the gate.

Do not proceed to M36 until the maintainer supplies or approves an immutable
commit SHA containing the extension.

## vaeg changes in M35

Only:

- ADR/evidence updates;
- patch or fork provenance;
- no vendored core;
- no emulator integration.

## Human gate G35

The gate requires:

- reviewable minimal patch;
- all upstream tests green;
- focused interrupt tests green;
- normal and `Z80_NO_FUNCTIONAL` builds green;
- immutable approved upstream/fork commit SHA;
- license unchanged as MIT;
- no vaeg behavior change.

Stop.

---

# 8. M36 — vendor the approved core and add standalone conformance

Task file:

```text
docs/agents/tasks/M36_z80_vendor_conformance.md
```

Branch:

```text
topic/m36-z80-vendor-conformance
```

## Goal

Vendor the exact approved third-party commit and prove it independently before
writing the vaeg wrapper.

## Required work

### 8.1 Vendor exact source

Vendor under:

```text
external/suzukiplan-z80/
```

Include only the source and license files required by the approved vendoring
policy.

Add a provenance file such as:

```text
external/suzukiplan-z80/provenance.txt
```

It must record:

- upstream repository;
- upstream base SHA;
- approved upstream or fork repository;
- exact vendored SHA;
- upstream issue/PR;
- release/tag if applicable;
- MIT license;
- summary of the interrupt extension.

Copy `LICENSE.txt` verbatim.

Never hand-edit the vendored directory after import. A later change requires
re-vendoring an immutable newer commit in its own commit.

### 8.2 Third-party notices

Update the repository's third-party license/NOTICE documentation and the Z80
migration ADR.

Clearly distinguish:

- upstream MIT code;
- downstream/fork MIT modification;
- newly authored vaeg BSD-2-Clause code;
- external ZEX test artifacts.

### 8.3 Compile smoke tests

Add build-only tests that include the header under:

- normal callback configuration;
- `Z80_NO_FUNCTIONAL`;
- debug enabled;
- release-oriented disabled-debug configuration if supported.

Compile on every platform in the existing CI matrix.

Do not yet link the core into the emulator.

### 8.4 Standalone interrupt extension tests

Mirror the most important upstream/fork interrupt tests in vaeg's CTest suite
so vendoring mistakes cannot silently remove the required API.

These tests must not depend on vaeg's old core.

### 8.5 ZEX runner

Add a headless CP/M-style runner under `tests/z80_compat/`.

Requirements:

- 64 KiB address space;
- deterministic reset;
- load at `0x0100`;
- CP/M BDOS CALL 5 handling sufficient for the chosen tests;
- bounded execution timeout;
- clear success/failure parsing;
- diagnostic dump on timeout or failure;
- no copyrighted vaeg ROM.

Run both ZEXDOC and ZEXALL where the approved artifact policy permits.

The dedicated CI job must:

1. fetch from the pinned source;
2. verify SHA-256;
3. cache only as a CI optimization;
4. fail on a hash mismatch;
5. exclude fetched files from source and release archives.

Provide an offline command accepting a user-supplied artifact directory.

### 8.6 CMake isolation

Add a standalone test target. Do not change the emulator's selected Z80 core.

Avoid global compile definitions where target-local definitions work.

## Out of scope

- no vaeg wrapper;
- no subsystem integration;
- no legacy source deletion;
- no state-save migration;
- no disassembler replacement.

## QA gate G36

Required:

- repository invariant checks;
- all current builds and tests;
- three-platform header compile smoke;
- focused interrupt tests;
- ZEXDOC/ZEXALL pass in the dedicated environment;
- release archive test proves no fetched ZEX artifact is packaged;
- emulator behavior unchanged.

Stop.

---

# 9. M37 — independently authored contracts, state codec, and wrapper

Task file:

```text
docs/agents/tasks/M37_z80_wrapper.md
```

Branch:

```text
topic/m37-z80-wrapper
```

## Goal

Implement a new vaeg-owned BSD-2-Clause compatibility layer around the vendored
core, without connecting it to `iova/subsystem.cpp`.

## Required new components

Use the M34-approved names. The following responsibilities must remain
separate even if exact files differ.

### 9.1 Bus and clock contract

Independently author the required interfaces. Preserve only the semantics that
current consumers need.

The API must cover:

- 16-bit memory address, 8-bit memory data;
- external I/O port and data;
- current host/emulated clock;
- consume/decrement remaining clock;
- get and set remaining clock.

Use standard fixed-width types.

It is acceptable to retain familiar symbol names when that minimizes consumer
changes, but do not copy M88 comments, macro machinery, typedef collections,
or declaration layout.

### 9.2 Public register representation

Define an independently authored public register structure.

Requirements:

- fixed-width register pairs;
- main and alternate register sets;
- PC, SP, IX, IY;
- I and R;
- IM;
- IFF1 and IFF2;
- no compiler-dependent padding used as serialized data;
- no dependency on `cpucva/types.h`;
- clear mapping to and from the vendored core.

Inventory findings determine whether source-level aliases or accessors are
needed for existing debugger code. Do not reproduce a complex union unless
actual consumers require it.

### 9.3 Frame-boundary legacy status codec

Implement explicit encode/decode for the old revision-1 Z80 status byte image.

The codec is invoked only at vaeg's normal frame-call save/load boundary, after
`Z80C::Exec()` has returned.

Requirements:

- constants for the exact revision-1 size and offsets derived in M34 for the
  current host build family;
- compile-time checks for the current build;
- fixture-driven decoding tests for retained same-build-family fixtures;
- no `reinterpret_cast` of an untrusted save buffer to the new runtime state;
- reject an unsupported internal revision safely;
- map all architectural register fields;
- materialize and restore architectural AF rather than legacy lazy-flag
  implementation details;
- map IFF1, IFF2, interrupt mode, I, and R;
- map IRQ level, HALT, and external WAIT;
- map `remainclock` and `lastclock`;
- translate old/new HALT PC representation explicitly if the cores differ;
- save the authoritative live PC;
- document fields that the legacy format does not contain.

The new runtime state must not use the legacy byte layout as its internal
representation.

The codec does not serialize a partially executed instruction, callback state,
or arbitrary third-party-core microstate.

### 9.4 Core wrapper

Implement the new core class with the externally required behavior.

Required operations include the verified consumer set from M34, including
equivalents of:

- initialization;
- `Exec()`;
- reset;
- level IRQ;
- NMI;
- external WAIT;
- `GetPC()` and `SetPC()`;
- public register access;
- state size/save/load;
- disassembler access placeholder if needed;
- only the diagnostic stubs that real consumers still require.

Do not retain unused legacy methods merely because they existed. If a method is
unused but must remain temporarily for source compatibility, mark it as a
documented inert compatibility stub and add a deletion plan.

### 9.5 Execution and clock semantics

Implement:

1. read `now`;
2. add `now - lastclock` to remaining clock with the same wrap semantics as
   the old contract;
3. if external WAIT is active, consume all positive remaining clock without
   executing instructions;
4. otherwise execute complete instructions while remaining clock is positive;
5. let the vendored core invoke fine-grained clock consumption;
6. synchronize the public register mirror when `Exec()` returns;
7. update `lastclock`;
8. leave the CPU at a completed instruction boundary suitable for vaeg's next
   frame-call save point.

Do not enable `Z80_CALLBACK_PER_INSTRUCTION` unless M34 explicitly changed the
contract. Fine-grained clock consumption is preferred.

Do not reproduce the old EI implementation's internal two-instruction fusion
unless a system regression proves it is required. The replacement must first
implement architecturally correct EI inhibition. Record resulting scheduling
deltas.

### 9.6 IRQ integration

Use the approved level-line and acknowledge-callback APIs from M35.

The callback reads the configured vaeg acknowledge port only at actual
acceptance.

Verify:

- eight-bit port mask;
- exact one read;
- no pre-read;
- IM0 raw opcode;
- IM1 acknowledge;
- IM2 acknowledge;
- deassertion;
- persistent line;
- EI delay;
- HALT.

### 9.7 Public register mirror

Maintain two concepts:

- authoritative live third-party core state;
- externally observable public mirror.

`GetReg()` returns the mirror.

Refresh the mirror at the M34-approved boundary. Ensure `SaveStatus()` and
`GetPC()` use authoritative state where required.

## Required unit tests

Use fake memory, I/O, clock, and clock-counter objects.

At minimum test:

1. reset state;
2. ordinary fetch and execution;
3. address wrapping;
4. all external I/O ports masked to eight bits;
5. remaining-clock accumulation;
6. instruction overshoot behavior;
7. external WAIT drains without executing;
8. resume after WAIT;
9. live `GetPC()`;
10. stale/public `GetReg()->pc` during an I/O callback;
11. mirror refresh at `Exec()` exit;
12. EI inhibition;
13. DI with asserted IRQ;
14. vector changed by the instruction following EI;
15. deassert before acceptance;
16. persistent level accepted again only after interrupts are reenabled;
17. IM0 `0x00`;
18. IM0 `0x7f`;
19. IM0 each RST family value;
20. IM0 multi-byte opcode;
21. IM1;
22. IM2;
23. HALT idle clocks;
24. HALT wake;
25. NMI;
26. R register behavior required by ZEX;
27. legacy fixture load for every retained M34 fixture;
28. new save/load round trip at the normal frame-call boundary;
29. unsupported internal revision rejection;
30. positive, zero, and negative `remainclock` with nonzero `lastclock`;
31. save/load in HALT after `Exec()` returns;
32. save/load with asserted IRQ after `Exec()` returns;
33. save/load with external WAIT after `Exec()` returns;
34. focused EI-boundary save/load without any instruction-internal save.

Run the ZEX runner through the wrapper, not only through the raw vendored core.

## Build selection

The emulator must still use the legacy core. The new wrapper exists only in
tests or a standalone library target.

## QA gate G37

Required:

- all wrapper tests on every supported platform;
- ZEX through wrapper;
- sanitizers on Linux where existing infrastructure supports them;
- no emulator behavior change;
- no source deletion;
- updated ADR mapping tables;
- exact test logs.

This gate may be machine-verifiable if all supported platform jobs pass.

Stop.

---

# 10. M38 — normalized old/new differential harness

Task file:

```text
docs/agents/tasks/M38_z80_differential.md
```

Branch:

```text
topic/m38-z80-differential
```

## Goal

Compare externally observable behavior without incorrectly assuming that old
and new internal step boundaries are one-to-one.

## Harness architecture

Avoid global class-name or symbol collisions.

Preferred approach:

- build a legacy runner executable;
- build a new runner executable;
- feed both the same deterministic corpus and event script;
- emit the same canonical trace format;
- compare traces in a separate tool.

Do not force both C++ implementations into one process if that requires unsafe
preprocessor renaming or production-source edits.

## Canonical trace

Define a versioned trace schema containing:

- checkpoint index and reason;
- authoritative architectural registers;
- public mirrored registers where relevant;
- IFF1/IFF2/IM;
- HALT state;
- IRQ line and NMI state;
- memory reads/writes with address, data, direction, and order;
- I/O reads/writes with masked port, data, direction, and order;
- interrupt-acknowledge events;
- consumed clocks;
- remaining clock at slice end;
- hash of selected memory ranges;
- explicit scheduler/`Exec()` boundary.

Use deterministic lower-case hexadecimal formatting and stable field order.

## Comparison boundaries

Compare at normalized externally observable points:

- end of an `Exec()` slice;
- interrupt acceptance completion;
- HALT entry and exit;
- state save/load;
- explicit test-program markers;
- completed I/O transactions.

Do not fail merely because old EI internally fused the following instruction
while the new core retired it separately. Compare the resulting architectural
and bus-visible behavior at the next normalized checkpoint.

## Test corpus

### 10.1 Directed instruction programs

Cover:

- all documented base opcode groups;
- CB, ED, DD, FD, DDCB, FDCB;
- branch taken/not taken;
- stack operations;
- block memory;
- block I/O;
- EI, DI, RETN, RETI;
- HALT;
- IM0/1/2;
- NMI;
- R register;
- undocumented flags used by ZEX.

### 10.2 Interrupt event scripts

Schedule assertion/deassertion by explicit trace events or clock points,
including:

- asserted while DI;
- asserted immediately before EI;
- changed acknowledge byte during the inhibited instruction;
- deasserted before acceptance;
- HALT wake;
- persistent line;
- NMI interaction;
- state save/load around pending events.

### 10.3 Deterministic random programs

Use committed seeds.

Public CI:

- bounded, fast deterministic corpus.

Nightly or maintainer-local:

- substantially longer run;
- multiple fixed seeds;
- documented command and expected runtime class;
- no unbounded fuzzing in ordinary PR CI.

Avoid self-modifying or undefined cases unless the test explicitly classifies
them.

## Divergence policy

Classify every divergence:

1. wrapper defect;
2. upstream defect;
3. old-core defect;
4. deliberate architectural correction;
5. internal scheduling difference with no external effect;
6. unresolved.

Only categories with evidence may be allowlisted.

An allowlist entry must include:

- exact minimal input;
- first divergent event;
- affected state;
- evidence source;
- why accepting it is safe;
- whether a bug report or upstream issue exists.

The new core passing ZEX is evidence, not absolute proof that every divergence
is an old-core defect.

## Cycle evidence

Generate:

```text
docs/modernization/z80-compat-cycle-deltas.md
```

Record cycle deltas by opcode/scenario.

Cycle differences are not automatically failures. Fail when they cause:

- changed external event order;
- broken FDD transfer;
- missed or extra interrupt;
- WAIT/HALT failure;
- demonstrable guest regression.

## QA gate G38

Required:

- directed corpus green or justified;
- canonical traces reproducible;
- public bounded random tests green;
- long local/nightly command completed and recorded;
- empty or evidence-backed allowlist;
- cycle report;
- no production core selection change.

Stop for review.

---

# 11. M39 — opt-in subsystem integration and state compatibility

Task file:

```text
docs/agents/tasks/M39_z80_integration.md
```

Branch:

```text
topic/m39-z80-integration
```

## Goal

Connect the new wrapper to the real PC-88VA subsystem while retaining the
legacy implementation as the default development fallback for this milestone
only.

## Required work

### 11.1 Build selection

Add a cache option or equivalent:

```text
VAEG_Z80_CORE=legacy
VAEG_Z80_CORE=suzukiplan
```

Default remains `legacy` in M39.

Use target-local source selection. Do not compile both implementations into the
production emulator target simultaneously.

Both choices must compile on Linux, macOS, and Windows-MinGW.

### 11.2 Subsystem adapter

Migrate `iova/subsystem.cpp` to the M34-approved new interface for the
`suzukiplan` selection.

Keep changes local to the Z80 subsystem seam.

Do not alter unrelated FDC, 8255, DMA, main CPU, sound, display, or pacing
behavior.

### 11.3 SLEEP_HACK

Preserve the public register mirror boundary.

Add trace evidence showing:

- the observed PC during relevant I/O callback;
- old and new values at the same normalized event;
- sleep assertion;
- wake event;
- external WAIT release.

Do not change magic constants unless the trace proves that preservation cannot
produce the old condition.

If a constant must change:

- document the old and new PC semantics;
- show the ROM listing or trace basis;
- add a focused regression;
- update the bug-fix ledger only if correcting an actual defect.

### 11.4 Frame-call state compatibility

All state tests must use vaeg's existing frame-call boundary:

```text
current frame/core execution call returns
-> no `Z80C::Exec()` or callback is active
-> state-save coordinator serializes the machine sections
```

Required:

- same-build-family legacy Z80 fixture -> new core;
- old emulator save -> new emulator load using legal maintainer-provided data
  from the same host build family;
- new save -> new load;
- ordinary running state;
- HALT;
- external WAIT;
- asserted IRQ after the Z80 execution slice has returned;
- positive, zero, and negative clock balance;
- the focused EI-boundary case;
- save/load while FDD activity is in progress across frame calls.

For the FDD test, saving may occur while the emulated transfer is logically in
progress, but only between CPU execution calls. Do not save inside
`Z80C::Exec()`, a memory or I/O callback, or a partially retired instruction.

Use the explicit revision-1 field mapping. Do not restore dependence on the M88
runtime structure.

### 11.5 System regression

Public CI:

- all ROM-less tests;
- headless smoke;
- wrapper tests;
- ZEX;
- bounded differential tests;
- build both selections.

Maintainer-local human tests for both selections:

1. clean boot in VA/V3 mode;
2. bundled VA demo;
3. OS boot and simple operations;
4. FDD read and write;
5. fixed timing-sensitive disk set;
6. normal idle entry;
7. Sorcerian sleep path;
8. ATN/8255 wake path;
9. save under legacy and load under new during FDD activity;
10. repeated reset and model change where applicable.

Do not commit private ROMs or disk images. Record hashes or maintainer-local
identifiers only where legally appropriate.

### 11.6 Regression evidence

Create or update:

```text
docs/modernization/z80-compatibility-integration.md
```

Include:

- core selection commands;
- old/new traces;
- state compatibility results;
- clock deltas;
- SLEEP_HACK evidence;
- known unresolved differences.

## Out of scope

- no default flip;
- no legacy file deletion;
- no legacy option deletion;
- no final release.

## Human gate G39

The maintainer must explicitly confirm the complete private system checklist
under `suzukiplan`.

A build-only or ZEX-only pass is insufficient.

Stop.

---

# 12. M40 — replace the disassembler and detach active consumers

Task file:

```text
docs/agents/tasks/M40_z80_disassembler.md
```

Branch:

```text
topic/m40-z80-disassembler
```

## Goal

Replace `z80diag` and migrate active subsystem/debugger consumers away from the
M88-derived headers while the legacy CPU remains available only for final
comparison.

## Implementation strategy

Follow the M34 ADR decision.

### Strategy A: upstream side-effect-free decoder

Use only if the pinned core exposes or has gained a stable API that:

- accepts a memory-reader callback and PC;
- performs no CPU execution;
- does not mutate CPU state;
- returns instruction length;
- writes bounded text or returns a bounded representation;
- supports all prefix pages.

Wrap it without exposing third-party STL across the subsystem boundary.

### Strategy B: independent vaeg implementation

If Strategy A is unavailable, implement a new BSD-2-Clause table-driven
disassembler from documented Z80 instruction encodings.

Requirements:

- do not copy old M88/cisc code;
- do not copy GPL tables;
- cover base, CB, ED, DD, FD, DDCB, and FDCB pages;
- produce bounded output;
- return the next PC or explicit length;
- handle truncated/wrapping memory reads deterministically;
- include documented and canonical commonly used undocumented mnemonics where
  required by the debugger;
- distinguish invalid/reserved encodings without buffer overrun.

## Compatibility surface

Preserve the actual consumer behavior determined in M34, including the
equivalent of:

```text
Init(memory)
Disassemble(pc, destination)
```

The exact class name may change if all consumers are migrated in the same
milestone.

Do not preserve unused diagnostic APIs.

## Tests

### 12.1 Exhaustive page corpus

Create a generated or committed byte corpus covering every opcode page and
prefix combination.

Verify:

- length;
- next PC;
- bounded output;
- no state mutation;
- wrap at `0xffff`;
- deterministic formatting.

### 12.2 Golden output

Use a reviewed golden file for debugger-facing text.

Generate it from the new implementation, then manually review it against
documented instruction encodings. Do not derive the golden file from the old
copyrighted disassembler.

### 12.3 Independent cross-check

Where practical, cross-check instruction length against the execution core's
decoder or a permissively licensed independent source. Treat mismatches as
review items, not automatic permission to copy another table.

### 12.4 UI spot check

Use the debugger/disassembly consumer in a legal ROM-less or maintainer-local
scenario.

Capture before/after samples for review, without distributing proprietary ROM
content.

## Header detachment

Migrate active consumers to the new files:

- no active subsystem/debugger include of `z80diag.h`;
- no active subsystem include of `z80.h`;
- no active subsystem include of `z80if.h`;
- no active subsystem dependence on `types.h`.

The old CPU build may still include those files internally until M41.

## QA gate G40

Required:

- exhaustive disassembler tests;
- golden tests on all platforms;
- debugger spot check;
- both CPU selections still build;
- new-core system smoke remains green;
- include graph shows the seven legacy files are now used only by the legacy
  CPU target, if at all.

Stop.

---

# 13. M41 — final cutover and removal

Task file:

```text
docs/agents/tasks/M41_z80_cutover.md
```

Branch:

```text
topic/m41-z80-cutover
```

## Goal

Make the new core the only production implementation and remove all approved
M88-derived Z80 subsystem files from HEAD and release archives.

## Required work

### 13.1 Flip and simplify

- make the suzukiplan-backed wrapper the only production Z80 core;
- remove `VAEG_Z80_CORE=legacy`;
- remove legacy source lists;
- remove legacy-only compile definitions;
- remove legacy-only CI jobs;
- keep permanent wrapper, ZEX, state-codec, and disassembler tests;
- retain useful canonical trace fixtures that do not contain M88 source.

Do not ship a one-release fallback.

### 13.2 Delete the approved files

Delete exactly these approved files:

```text
cpucva/types.h
cpucva/z80.h
cpucva/z80if.h
cpucva/z80c.h
cpucva/z80c.cpp
cpucva/z80diag.h
cpucva/z80diag.cpp
```

Use deletion commits consistent with repository commit rules.

Do not delete any additional file without explicit maintainer approval.

### 13.3 Remove stale references

Check:

- all includes;
- CMake source lists;
- IDE/package manifests still active;
- release scripts;
- documentation;
- test scripts;
- generated dependency lists.

Do not edit frozen reference-tier project files merely to remove references;
document frozen historical references if they remain.

### 13.4 Documentation and licenses

Update:

- README/build documentation;
- roadmap and task record;
- migration ADR status;
- third-party notice/license table;
- release notes;
- bug-fix ledger only for concrete corrected guest-visible defects;
- packaging documentation.

State clearly:

- vendored core repository and exact SHA;
- MIT license;
- vaeg wrapper/disassembler BSD-2-Clause;
- old M88-derived source removed from current distribution;
- history not rewritten.

### 13.5 Release archive audit

Build the real release archive using the project's distribution path.

Fail if it contains:

- any of the seven deleted paths;
- stale copies under another directory;
- ZEX artifacts;
- unrecorded third-party source;
- patch work files;
- private ROM/disk data.

Create a machine-readable archive manifest and grep report.

### 13.6 Permanent CI

Keep:

- three-platform builds;
- repository invariants;
- wrapper unit tests;
- interrupt extension tests;
- legacy state fixture loads;
- ZEX dedicated job;
- disassembler exhaustive/golden tests;
- bounded deterministic fuzz/differential-style self-consistency tests;
- headless smoke;
- release archive audit.

Remove old/new differential jobs that require compiling the deleted old core.
Preserve their final reports under documentation.

## Final automated checks

At minimum:

```text
git grep -n -i "copyright.*cisc"
git grep -n -i "m88"
git grep -n "cpucva/z80c"
git grep -n "cpucva/z80diag"
git grep -n "cpucva/z80if"
git grep -n "cpucva/types.h"
```

Hits in migration history documents may remain if they are clearly historical.
No active source, build, or release reference may remain.

Verify the seven files do not exist.

Verify the release archive does not contain them.

Run all current repository checks, full CTest, CI-equivalent builds, and
headless smoke.

## Human gate G41

The maintainer performs the standard VA gate plus Z80-specific checks:

1. clean checkout and build;
2. VA/V3 boot;
3. bundled VA demo;
4. OS boot and simple file operations;
5. FDD read/write;
6. timing-sensitive loader;
7. VA idle sleep;
8. Sorcerian path;
9. WAIT wake;
10. old save load;
11. new save/load during FDD activity;
12. reset and repeated boot;
13. release archive inspection.

The gate passes only when:

- all tests pass;
- human regression passes;
- all seven files are absent;
- no legacy fallback remains;
- release archive is clean;
- exact final SHAs are reported.

Tag only according to the repository's existing release convention after the
maintainer explicitly approves G41.

---

# 14. M42 — optional performance configuration

Task file:

```text
docs/agents/tasks/M42_z80_performance.md
```

Branch:

```text
topic/m42-z80-performance
```

Do not schedule M42 automatically.

## Entry condition

Profile a representative VA workload first.

Proceed only if the Z80 subsystem consumes enough host time to justify
optimization.

## Candidate options

Evaluate independently:

- `Z80_NO_FUNCTIONAL`;
- `Z80_DISABLE_DEBUG`;
- `Z80_DISABLE_BREAKPOINT`;
- `Z80_DISABLE_NESTCHECK`;
- `Z80_NO_EXCEPTION`;
- other approved upstream switches.

Do not enable `Z80_CALLBACK_PER_INSTRUCTION` if it weakens the required
fine-grained clock mapping.

## Benchmark

Add a deterministic benchmark that reports:

- instructions or test-program iterations per second;
- wrapper overhead;
- callback overhead;
- debug/release configuration;
- compiler and architecture;
- checksums proving identical architectural output.

Benchmark raw core and wrapper separately.

## Acceptance

An optimization is accepted only if:

- measurable;
- behavior-neutral;
- all conformance and integration tests stay green;
- no public interface regression;
- no loss of diagnostic value in debug builds.

Record accepted and rejected configurations in the ADR.

---

# 15. Cross-milestone QA matrix

| Requirement | M34 | M35 | M36 | M37 | M38 | M39 | M40 | M41 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Provenance inventory | required | update | verify | verify | — | — | update | final |
| Limited C++ exception | approve | — | enforce | enforce | enforce | enforce | enforce | enforce |
| IRQ ack at acceptance | contract | implement | test | integrate | compare | system test | — | permanent |
| Level-sensitive IRQ | contract | implement | test | integrate | compare | system test | — | permanent |
| Raw IM0 opcode | contract | implement | test | integrate | compare | system test | — | permanent |
| Eight-bit I/O port | verify | — | — | unit test | trace | system test | — | permanent |
| GetReg stale mirror | verify | — | — | unit test | trace | SLEEP_HACK | — | permanent |
| Frame-call old save -> new | boundary + fixture | — | — | unit test | — | system test | — | permanent |
| ZEX | policy | upstream | raw core | wrapper | optional | permanent | permanent | permanent |
| Disassembler | decide | — | — | placeholder | — | — | replace | permanent |
| Seven-file removal | approve | — | — | prepare | — | prepare | detach | delete |
| Release archive clean | baseline | — | test infra | — | — | — | — | required |

---

# 16. Required Codex milestone report format

At the end of every milestone, report:

## Starting point

- branch;
- starting SHA;
- previous gate evidence;
- initial `git status --short`.

## Changes

- files added;
- files changed;
- files renamed;
- files deleted;
- behavior changes;
- explicitly unchanged areas.

## Verified facts

For every relevant M34 contract item:

- confirmed;
- corrected;
- not exercised;
- unresolved.

Do not call a hypothesis a verified fact.

## Tests

List exact commands and results.

Include:

- repository checks;
- CMake configure/build;
- CTest;
- headless smoke;
- milestone-specific tests;
- platform matrix status;
- private human tests only when performed by the maintainer.

## Licensing and provenance

- third-party SHA;
- license;
- new-file headers;
- vendored-directory integrity;
- ZEX policy compliance.

## Risks and unresolved issues

State them directly. Do not hide failures behind "mostly works."

## Commits

List full commit SHAs and subjects.

## Gate

State exactly what the maintainer must verify.

Then stop.

---

# 17. Stop conditions

Stop the current milestone and report evidence if any of the following occurs:

- the acknowledge callback cannot be invoked at actual acceptance without a
  broad upstream rewrite;
- raw IM0 cannot execute non-RST opcodes correctly;
- the approved fork commit is not immutable or accessible;
- the chosen ZEX artifact lacks a lawful reproducible acquisition path;
- the production save path is found to run before the current frame/core
  execution call returns, or while `Z80C::Exec()` or one of its callbacks is
  active;
- a normal frame-call boundary, including its instruction-boundary CPU state
  and clock balance, cannot be restarted correctly using revision 1;
- the new core causes repeatable FDD corruption;
- SLEEP_HACK cannot be preserved without changing externally visible timing;
- a new M88-derived file is found and deletion approval is needed;
- a vendored file must be hand-edited to proceed;
- private ROM/disk content would need to enter the repository or public CI;
- a milestone would require modifying the frozen reference tier.

A stop condition is not permission to improvise silently. Update the ADR with
the evidence and wait at the current gate.

---

# 18. Definition of done

The migration is complete only when all of the following are true:

1. `suzukiplan/z80` is pinned by immutable commit and its MIT license is
   preserved.
2. The approved interrupt extension reads the acknowledge byte only at actual
   acceptance.
3. IRQ is level-sensitive at the vaeg subsystem boundary.
4. IM0 supports the raw first opcode rather than only RST numbers.
5. All external I/O ports are masked to eight bits.
6. The consumer-visible `Z80C` class name and all M34-proven used public
   method signatures remain source-compatible.
7. The wrapper preserves the required `Exec()`, WAIT, HALT, IRQ, NMI, public
   register, and clock behavior.
8. Same-build-family revision-1 legacy saves load under the new core at
   vaeg's existing frame-call boundary.
9. The new core writes revision-1 saves that round-trip at that boundary,
   preserving the Z80 completed-instruction state, `remainclock`, and
   `lastclock`.
10. ZEXDOC and ZEXALL pass through the wrapper under the approved test policy.
11. Directed interrupt and deterministic random tests pass.
12. The new disassembler covers all opcode pages and passes golden tests.
13. The VA subsystem passes public ROM-less and maintainer-private system
    regression.
14. The seven approved M88-derived files are absent from HEAD.
15. The seven files and ZEX artifacts are absent from release archives.
16. No legacy selectable fallback remains.
17. Third-party notices and ADRs are complete.
18. The final human gate passes.

## M41 implementation status

M41 removed the build selector and exactly the seven approved M88/cisc-derived
Z80 files. Current production unconditionally uses `cpucva/z80_compat_cpu.cpp`,
`cpucva/z80_compat_state.cpp`, and `cpucva/upd780_disasm.cpp`; the vendored MIT
source is unchanged. Historical old/new procedures and filename references in
this master and milestone evidence remain historical, not active build or
packaging inputs. G41 public, private, archive, and hosted validation passed;
details are in the M41 evidence report.
