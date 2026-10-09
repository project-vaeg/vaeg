# M34 — Z80 migration contract, inventory, and feasibility gate

## Status

Complete; G34 passed by maintainer approval on 2026-07-15.

Do not vendor or integrate a replacement Z80 core in this milestone.
Do not begin M35.
Stop at gate G34 and wait for explicit maintainer approval.

M34 evidence corrected one premise below: with the current positive subsystem
clock multiplier, both normal execution and external WAIT return from
`Z80C::Exec()` only after `remainclock` becomes zero or negative. A positive
revision-1 field remains codec input coverage, but is not a proven production
return state. See `docs/modernization/z80-compatibility-legacy-contract.md` and ADR-0011.

The accepted state import/export conclusion is that architectural state,
HALT, and `execEI` are importable and exportable through the selected core's
public register state. The only state-related M35 extension required is an
inspectable and restorable level-sensitive IRQ state; no broader state API is
needed.

## Required references

Read in this order before changing the repository:

1. `AGENTS.md`
2. `docs/agents/ROADMAP.md`
3. `docs/agents/CONVENTIONS.md`
4. `docs/agents/z80_migration_master.md`
5. this task file
6. relevant existing decisions under `docs/agents/DECISIONS/`
7. relevant state-save and Z80 subsystem sources

Run:

```sh
git status --short
git rev-parse HEAD
```

Record the exact starting branch, commit SHA, and worktree status.

## Branch and commit rules

Use:

```text
topic/m34-z80-contract
```

Use one concern per commit.

Commit subjects use the form:

```text
M34: <English imperative subject>
```

All new repository text, code, comments, identifiers, and commit messages must
be in English.

Do not modify the frozen reference tier:

- `win9x/`
- `i286x/`
- `cpuxva/memoryva.x86`
- `hlp/`

Do not modify ROMs, disk images, fonts, icons, sound payloads, or other binary
assets.

## Goal

Establish a verified and maintainable contract for replacing the M88/cisc-
derived Z80 subsystem with an approved minimally extended version of the
MIT-licensed `suzukiplan/z80` core.

This milestone must determine whether the planned migration is technically
feasible before implementation begins.

No guest-visible behavior change is permitted.

## Maintainer-approved decisions

Do not reopen these decisions unless direct source evidence proves that one is
technically impossible.

### Selected core and licensing

- `suzukiplan/z80` is the selected replacement candidate.
- MIT-licensed third-party code is acceptable.
- Vendored or forked third-party code retains its MIT notice.
- Newly authored vaeg files use the repository-standard 2-clause BSD header.

### Limited C++17 exception

The general C99 preference remains.

C++17 is permitted only for CPU backends and thin compatibility adapters under
`cpucva/` when required by this approved third-party CPU core.

C++ and STL types must not cross existing consumer-visible subsystem,
debugger, or state-save interfaces.

Do not perform unrelated C-to-C++ conversion.

Update `AGENTS.md` and `docs/agents/CONVENTIONS.md` consistently with this
limited exception.

### Consumer-visible API

The implementation will be replaced, but current consumers remain source
compatible.

Requirements:

- keep the public class name `Z80C`;
- preserve every public method signature proven used by M34;
- a replacement header path may change if all consumers are migrated together
  in a later milestone;
- methods proven unused may be removed after this inventory;
- `IMemoryAccess`, `IIOAccess`, `IClock`, and `IClockCounter` may be
  independently reauthored under the same names;
- no third-party core type or STL type may appear in the public contract.

Binary compatibility with separately compiled objects is not required. vaeg is
rebuilt as a whole.

### Interrupt acknowledge

The acknowledge port is read only when a maskable interrupt is actually
accepted by the CPU.

It must not be speculatively read before an instruction.

The future core extension must support:

- a level-sensitive IRQ line;
- an acknowledge callback invoked exactly once at acceptance;
- IM0 raw first-opcode execution;
- IM1 acknowledge at acceptance;
- IM2 acknowledge at acceptance.

Do not use a pre-read / arm / cancel approximation.

### IM0 practical scope

Determine the values that the VA subsystem can actually supply and the values
observed at actual IM0 acceptance.

Required migration behavior:

- all observed values must work correctly;
- RST-family values, `0x00`, and `0x7f` receive focused tests;
- multi-byte and prefixed raw-opcode support must be evaluated against the
  selected core;
- unobserved exotic prefix cases are not automatically a vaeg release blocker
  unless they are required by the subsystem ROM or needed for a correct,
  maintainable upstream implementation.

Do not silently map a non-RST value to a no-op or fixed RST.

### I/O port width

The vaeg-facing Z80 I/O contract is eight-bit.

Every IN, OUT, and block-I/O operation presented to the subsystem must use:

```text
port & 0xff
```

### Public register freshness

Preserve the old observable distinction between:

- authoritative live core state; and
- the public `GetReg()` mirror synchronized when `Z80C::Exec()` returns.

`SaveStatus()` must use authoritative live PC and architectural flags, not a
stale diagnostic mirror.

### State-save model

Preserve vaeg's current state-save resolution.

A save is serviced only after the current frontend/core frame execution call
has returned.

At the save point:

- `Z80C::Exec()` is not active;
- no Z80 callback is active;
- the Z80 is at a completed instruction boundary;
- the machine is serialized through vaeg's existing section-based state-save
  coordinator.

The Z80 save resolution is:

```text
completed instruction boundary
+
architectural registers
+
IFF / interrupt mode / I / R
+
HALT / external WAIT / IRQ
+
remainclock
+
lastclock
```

Do not design an arbitrary Z80 microstate serializer.

Do not require save/load inside an instruction, memory callback, I/O callback,
clock callback, prefix decode, or interrupt acknowledge.

Target the existing revision-1 Z80 status size for the same supported host
build family.

Required:

- same-build-family legacy revision-1 save -> new core;
- new-core revision-1 save -> new core;
- normal FDD operation resumes after frame-boundary save/load;
- HALT and external WAIT resume correctly.

Cross-architecture, cross-compiler, and new-to-old save compatibility are not
requirements unless vaeg already provides them.

EI inhibition receives one focused boundary test. Determine whether the future
wrapper can return from `Exec()` with EI inhibition still active. If so,
determine whether it can be represented safely in revision 1 or avoided at the
observable `Exec()` boundary without changing scheduling behavior.

### Approved final deletion list

The maintainer has approved eventual deletion of:

```text
cpucva/types.h
cpucva/z80.h
cpucva/z80if.h
cpucva/z80c.h
cpucva/z80c.cpp
cpucva/z80diag.h
cpucva/z80diag.cpp
```

Do not delete them in M34.

Report any additional M88/cisc-derived file. Do not delete an additional file
without explicit approval.

## Required work

### 1. Roadmap and master-plan registration

Add M34 through M41 and optional M42 to `docs/agents/ROADMAP.md`, using the
sequence defined by `docs/agents/z80_migration_master.md`.

Do not alter or recreate completed M33 work.

The task table must identify:

- task file;
- deliverable;
- dependency order;
- human or machine gate.

Later task files may remain drafts until the preceding gate supplies verified
facts. Do not pretend that an unresolved design is final.

### 2. Create or update the migration decision

Use the next available identifier under:

```text
docs/agents/DECISIONS/
```

Record:

- selected core;
- MIT acceptance;
- limited C++17 exception;
- API source-compatibility requirement;
- all seven approved legacy files;
- interrupt-acknowledge timing;
- level-sensitive IRQ;
- practical raw-IM0 policy;
- eight-bit I/O ports;
- public register freshness;
- frame-boundary state-save model;
- revision-1 compatibility scope;
- upstream-first and minimal-fork fallback policy;
- ZEX acquisition policy;
- disassembler strategy;
- final no-fallback release policy.

### 3. Complete provenance and dependency inventory

Find every definition and consumer of:

- `cpucva/types.h`;
- `Z80Reg`;
- `IMemoryAccess`;
- `IIOAccess`;
- `IClock`;
- `IClockCounter`;
- `Z80C`;
- `Z80Diag`;
- every public `Z80C` method;
- subsystem CPU save/load hooks;
- subsystem disassembly hooks.

Use source search, include analysis, CMake source lists, and link evidence.

For every symbol or file, record:

- definition;
- copyright/provenance;
- direct consumers;
- transitive consumers;
- whether it is used;
- source-compatible replacement requirement;
- planned replacement milestone.

Do not assume `iova/subsystem.cpp` is the only consumer without evidence.

### 4. Verify legacy execution contracts

Create:

```text
docs/modernization/z80-compatibility-legacy-contract.md
```

Record exact file and line references for:

- `Z80C::Exec()` scheduling;
- remaining-clock accumulation and overshoot;
- `lastclock`;
- external WAIT;
- IRQ level storage;
- interrupt acceptance;
- acknowledge read timing;
- IM0, IM1, and IM2;
- EI and DI;
- HALT and wakeup;
- NMI;
- memory and I/O masks;
- live PC versus public mirror;
- save/load calls and status revision.

Clearly distinguish:

1. architectural Z80 behavior;
2. old-core implementation behavior;
3. behavior externally observed by vaeg;
4. old-core behavior that is probably a defect and must not be preserved
   automatically.

### 5. Verify the production state-save boundary

Trace the real production call chain from save request to:

```text
statsave_save()
-> subsystem CPU section
-> Z80C::SaveStatus()
```

Prove or disprove:

- the current frame/core execution call has returned;
- `Z80C::Exec()` cannot still be active;
- no memory, I/O, clock, or acknowledge callback is active;
- the Z80 is at a completed instruction boundary;
- `remainclock` can be positive, zero, or negative;
- `lastclock` is part of restart state;
- flags are materialized before save;
- live PC is saved.

If the production save path can run while `Z80C::Exec()` or a callback is
active, stop and report the actual call chain.

Do not redesign the global state-save scheduler in M34.

### 6. State ABI and fixture baseline

Add a ROM-less test or test-only utility reporting for the current build
family:

- `sizeof` of the old public register structure;
- `sizeof` of the old Z80 status structure;
- `offsetof` for every serialized field;
- relevant primitive and alias sizes;
- byte order;
- compiler;
- architecture.

Generate deterministic legacy status fixtures at the normal frame-call
boundary for:

- ordinary running state;
- alternate registers;
- IFF1/IFF2 and interrupt modes;
- HALT;
- external WAIT;
- asserted IRQ after `Exec()` returns;
- nonzero or negative `remainclock`;
- nonzero `lastclock`;
- every exposed `Exec()` return shape around EI.

Use only hand-assembled ROM-less programs.

Do not commit private ROMs or disk images.

### 7. Determine state import/export feasibility

Inspect the selected `suzukiplan/z80` revision.

Determine whether public or minimally extendable APIs can import and export
every state required at vaeg's frame-boundary save point:

- architectural registers;
- PC and SP;
- I and R;
- IFF1/IFF2;
- interrupt mode;
- HALT;
- reachable EI inhibition;
- level IRQ;
- any production-reachable pending NMI condition.

If required state is inaccessible, include the minimum state import/export API
in the proposed M35 upstream/fork extension.

Do not defer discovery of an essential state API until wrapper implementation.

### 8. Determine IM0 use and extension feasibility

Document:

- how the subsystem programs the acknowledge byte;
- reset/default values;
- values statically present in the subsystem code or ROM-facing paths;
- a debug-only method to log values only at actual IM0 acceptance.

Where private ROM testing is required, provide the instrumentation and exact
maintainer procedure without committing the ROM.

Classify:

- values proven observed;
- values statically possible but unobserved;
- theoretical values outside current subsystem evidence.

Evaluate whether the selected core can support raw first-opcode injection,
including operands fetched from current PC, without an unmaintainable rewrite.

### 9. Define the proposed M35 upstream/fork extension

Write a focused API and test proposal for:

- level-sensitive IRQ line;
- acknowledge callback at actual acceptance;
- IM0 raw first opcode;
- IM1 and IM2 acknowledge behavior;
- state import/export additions found necessary by this milestone;
- normal and `Z80_NO_FUNCTIONAL` callback configurations.

Prefer an upstream contribution.

If upstream merge is unavailable, the approved fallback is a minimal
MIT-licensed fork pinned by immutable commit.

Do not edit a future vendored copy in vaeg.

### 10. ZEX policy

Determine the exact ZEXDOC/ZEXALL source, license, and cryptographic hashes.

Preferred policy:

- dedicated CI fetch;
- fixed source/commit;
- SHA-256 verification;
- offline user cache option;
- no ZEX artifact in normal source or release archives.

Stop if a lawful reproducible acquisition path cannot be established.

### 11. Disassembler strategy

Inspect whether the selected core provides a side-effect-free reusable decoder
API.

Record one future strategy:

1. existing upstream decoder API;
2. minimal upstream decoder API addition;
3. independently authored BSD-2-Clause vaeg disassembler based on documented
   encodings.

Do not copy M88/cisc or GPL opcode tables.

### 12. M35 feasibility recommendation

End the ADR and report with an explicit recommendation:

- GO;
- GO WITH APPROVED MINIMAL FORK;
- STOP AND REVISE CORE SELECTION;
- STOP FOR A SPECIFIC MAINTAINER DECISION.

The recommendation must separately cover:

- interrupt contract;
- state import/export;
- frame-boundary revision-1 save;
- practical IM0 scope;
- disassembler;
- licensing/provenance;
- test acquisition.

## Out of scope

Do not:

- vendor `suzukiplan/z80`;
- write the production wrapper;
- integrate the new core;
- change the default core;
- replace the disassembler;
- delete the approved seven files;
- modify guest-visible behavior;
- begin M35.

## Required checks

Run the current repository invariant checkers.

Configure and build the current unmodified emulator using the documented
preset for the host.

Run the current CTest and headless/ROM-less smoke gates.

Run the new ABI and fixture tests.

Report exact commands and complete results.

## Deliverables

At minimum:

- updated `AGENTS.md`;
- updated `docs/agents/CONVENTIONS.md`;
- updated `docs/agents/ROADMAP.md`;
- migration decision/ADR;
- `docs/modernization/z80-compatibility-legacy-contract.md`;
- complete consumer and provenance inventory;
- frame-boundary save call-chain evidence;
- ABI report and legal ROM-less fixtures;
- proposed M35 upstream/fork API;
- ZEX policy;
- disassembler decision;
- M35 feasibility recommendation.

## Gate G34

Stop after presenting:

- starting and ending SHAs;
- branch;
- clean/dirty status;
- commit list;
- files changed;
- corrected verified facts;
- complete check output;
- unresolved risks;
- explicit M35 recommendation.

Do not start M35 until the maintainer explicitly states that G34 passed.
