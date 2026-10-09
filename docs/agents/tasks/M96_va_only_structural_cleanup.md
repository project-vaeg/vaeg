<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M96 - VA-only structural audit and staged cleanup

Status: **not started**

Evaluated baseline: `dfe50a1420c075040c12b96f00c315b5987a846a`

Branch: `topic/m96-va-only-structural-cleanup`

Commit prefix: `M96:`

Report: `docs/agents/reports/m96_va_only_structural_cleanup.md`

## Scope

Audit and simplify the PC-88VA-only source tree only where static
reachability, build reachability, and hardware classification agree. Preserve
demonstrated VA behavior and keep compatibility or hardware boundaries that
remain live.

The PC-88VA C-bus is a live hardware boundary and must not be deleted, renamed,
or described as PC-9801 residue. `romimage/` is read-only. `fontrom` remains
mapped inside `mem[]`. Existing uPD9002, uPD70008-compatible, and uPD780
adapters are out of scope unless a demonstrated mechanical dependency forces a
fixup.

Hardware documentation under maintainer-local `docs/tekumani/` and related
reference trees is cited only from the tracked report; source comments must
not cite those untracked paths.

## Stages and gates

Each stage is a separate concern. Stop at every human gate until the
maintainer explicitly reports that gate passed.

1. **M96a - baseline inventory and evidence freeze.** Record the evaluated
   commit, clean/dirty status, submodules, presets, production ownership,
   reset/bind order, dispatch inventory, and independent reduction/preservation
   reviews. Add the M96 report and ROADMAP entry. No source deletion.
2. **M96b - demonstrated dead residue.** In separate commits, delete only
   approved candidates whose CMake, include, symbol, runtime, and hardware
   evidence all prove deadness. Preserve C-bus support and protected payloads.
3. **M96c - truthful dispatch ownership.** Clarify the canonical VA I/O map
   and live C-bus lifecycle. Do not flatten the three lifecycle tiers without
   proof.
4. **M96d - NOP hook.** Remove only the physical-address simulated-BIOS side
   channel from uPD9002 opcode 90h while preserving NOP timing and all other
   instruction semantics.
5. **M96e - simulated BIOS audit.** Trace producers and consumers, then
   remove only helpers and initialization proven unreachable. Decide
   `biosboot.c` function by function. Do not modify `romimage/`.
6. **M96f - configuration truth.** First convert the positional `NP2CFG`
   initializer to designated fields and prove byte identity. Only then remove
   fields with no runtime, GUI, CLI, INI, or state dependency.
7. **M96g - state and memory.** Consolidate system-port ownership only after
   migration proof, version any state-format change explicitly, and calculate
   `mem[]` bounds including host font backing.
8. **M96h - dead branches and identity.** Remove only macros proven undefined
   in every supported build, clean VAEG identity residue, and update comments
   only in files already changed by M96.
9. **M96i - final audit.** Re-run reachability, protected-payload checks,
   dispatch classification, comment evidence mapping, and all gates.

## Required validation

After source-changing stages run the repository encoding, EOL, case,
clang-format, and unreferenced-source checks, the Linux Debug configure/build,
the supported MinGW cross configure/build, CTest where a test preset exists,
and the executable selftest. Run the human VA smoke checklist from a clean
checkout at each required gate. Unavailable private fixtures are `SKIP`, never
`PASS`.

## Non-goals

Do not change SGP, TSP, video, sound-board, FDC, SASI/SCSI, MPU98II, EMS/BMS,
VA ROM contents, protected binary payloads, CPU adapter semantics, DIP/memory
switch behavior, or broad `np2*` naming in this milestone. Do not mass-translate
untouched comments or claim real-hardware behavior from emulator-only tests.
