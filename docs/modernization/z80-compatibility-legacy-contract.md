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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# Legacy Z80 contract and M34 evidence

This report describes current HEAD at M34's starting commit
`c8f92d278adaa604ee1c02fa7361cdcc6fa25bd6`. It separates verified source facts
from future hypotheses. Line references apply to that commit.

## Provenance, build, and consumers

`cpucva/types.h:1-5`, `z80.h:5-12`, `z80c.h:5-10`, `z80diag.h:5-10`, and the
corresponding implementations identify M88/cisc origins. A provenance search
outside the frozen tier found exactly the seven files approved in ADR-0011.
`CMakeLists.txt:263-281` puts `z80c.cpp`, `z80diag.cpp`, and
`io/subsystem.cpp` in `VAEG_VA_SOURCES`; Ninja's compiler dependency output
shows that `subsystem.cpp` directly includes `z80if.h` and `z80c.h`, with
`types.h`, `z80.h`, and `z80diag.h` transitively included. The three objects
are members of the production `libvaeg_va.a` archive.

| Legacy file | Direct active dependency | Transitive production reach | Replacement |
|---|---|---|---|
| `types.h` | `z80.h`, `z80if.h`, `z80c.h`, `z80diag.h` | all three compiled Z80/subsystem objects | fixed-width definitions in M37; delete M41 |
| `z80.h` | `z80c.h` | `z80c.cpp`, test fixture, and `subsystem.cpp` through `z80c.h` | `z80_registers.h` in M37; delete M41 |
| `z80if.h` | `z80c.h`, `z80diag.h`, `z80diag.cpp`, and `subsystem.cpp` | Z80, disassembly, and subsystem objects | `z80_bus.h` in M37/M39; delete M41 |
| `z80c.h` | `z80c.cpp`, `subsystem.cpp`, and M34 test fixture | subsystem ownership, execution, state, and disassembly bridge | `z80_core.h` in M37/M39; delete M41 |
| `z80c.cpp` | CMake production and M34 test source lists | `libvaeg_va.a` and contract test | `z80_core.cpp` in M37/M39; delete M41 |
| `z80diag.h` | `z80c.h` and `z80diag.cpp` | `Z80C` ownership and subsystem bridge | `z80_disasm.h` in M40; delete M41 |
| `z80diag.cpp` | CMake production and M34 test source lists | `libvaeg_va.a` and contract test | `z80_disasm.cpp` in M40; delete M41 |

| Definition | Active definition/consumer evidence | Required replacement | Milestone |
|---|---|---|---|
| aliases in `types.h` | Defined at `cpucva/types.h:15-37`; all legacy headers use them; no independent active include | fixed-width standard types, no catch-all alias header | M37/M41 |
| `Z80Reg` | `z80.h:26-85`; held by `Z80C`; returned by bridge at `subsystem.cpp:349-351` | independently authored mirror, retaining fields consumers need | M37/M40 |
| four bus/clock interfaces | `z80if.h:29-62`; implemented only by `Clock`, `ClockCounter`, and `Subsystem` at `subsystem.cpp:34-121` | independently authored same-semantic contracts | M37/M39 |
| `Z80C` | public surface `z80c.h:81-110`; owned and initialized at `subsystem.cpp:105,128-145` | source-compatible class and used methods | M37/M39 |
| `Z80Diag` | `z80diag.h`; owned by `Z80C`; active bridge calls it at `subsystem.cpp:353-355` | temporary bridge, then independent disassembler | M40 |
| save hooks | bridge `subsystem.cpp:357-367`; consumer `machine/statsave.c:1273-1302` | same C-facing section operations | M37/M39 |
| disassembly hook | declaration `subsystem.h:26`, definition `subsystem.cpp:353-355`; no active caller found | reauthor hook before legacy deletion | M40 |

Repository-wide active searches found these used `Z80C` methods: constructor,
destructor, `Init`, `Exec`, `Reset`, `IRQ`, `Wait`, `GetStatusSize`,
`SaveStatus`, `LoadStatus`, `GetReg`, and `GetDiag`. `GetPC`, `SetPC`, and
`TestIntr` are called only inside the legacy implementation. No active caller
was found for `NMI`, `GetWaits`, `IsIntr`, `EnableDump`, `GetDumpState`, or
`GetStatistics`. `GetPC`, `SetPC`, and NMI remain in the approved architectural
contract; `TestIntr` becomes private and unused diagnostics may be removed.
The only consumers of the C register/disassembly bridges found outside the
frozen tier are their definitions; frozen Win9x debugger users do not impose
an active build contract.

| Public method | Current active use | Replacement decision |
|---|---|---|
| `Z80C`, `~Z80C` | allocated/deleted by `Subsystem` | preserve |
| `Init` | `Subsystem::Initialize` | preserve exact used signature |
| `Exec` | `Subsystem::Exec` | preserve exact used signature |
| `Reset` | `Subsystem::Reset` | preserve exact used signature |
| `IRQ` | `Subsystem::IRQ` | preserve exact used signature and level semantics |
| `NMI` | no external caller | retain architectural service |
| `Wait` | sleep and ATN wake paths | preserve exact used signature |
| `GetStatusSize`, `SaveStatus`, `LoadStatus` | statsave C bridge | preserve exact used signatures; implement explicit codec |
| `GetPC`, `SetPC` | legacy implementation only | retain live-PC architectural service |
| `GetReg` | sleep checks, traces, C bridge | preserve exact used signature and mirror freshness |
| `GetDiag` | subsystem disassembly bridge | retain through M40, then remove with migrated bridge |
| `TestIntr` | legacy implementation only | make internal; no public replacement |
| `GetWaits`, `IsIntr` | no caller | remove after legacy cutover |
| `EnableDump`, `GetDumpState`, `GetStatistics` | no caller | remove after legacy cutover |

## Verified legacy execution behavior

These are implementation facts, not claims that every behavior is
architecturally correct.

- `Exec()` reads `now`, adds unsigned `now-lastclock` to the clock counter,
  drains positive credit during external WAIT or retires instructions while
  credit is positive, refreshes `reg.pc` only on the executing branch, and
  stores `lastclock` (`z80c.cpp:195-216`). `ClockCounter::past()` subtracts
  instruction clocks times the positive machine multiplier
  (`subsystem.cpp:44-70,153-155`). Thus normal execution and WAIT return at
  zero or a negative overshoot, never positive, under current configuration.
- WAIT is bit 1 (`z80c.cpp:163-169`); HALT is bit 0
  (`z80c.cpp:995-1005`). Legacy HALT rewinds PC to the opcode, drains the
  remaining slice if IRQ is absent, and wakes at accepted IRQ by clearing
  wait and incrementing PC (`z80c.cpp:362-366`). Architectural Z80 state is
  halted after HALT with its next-instruction PC; the wrapper therefore needs
  explicit representation translation.
- `IRQ(uint,uint d)` simply stores `d` (`z80c.h:88-91`). Acceptance requires
  both IFF1 and nonzero `intr`, clears IFF1/IFF2, and does not clear `intr`
  (`z80c.cpp:355-390`): vaeg observes a level-sensitive line.
- The configured acknowledge port is read at `z80c.cpp:368`, after acceptance
  is established. IM0 dispatches the returned byte through `SingleStep`
  (`:372-375`); IM1 ignores it and enters `0x38` (`:377-381`); IM2 uses it as
  vector low byte (`:383-386`). `subsystem.cpp:144-157` configures virtual
  port `0x102` and default byte `0x7f`; port `0xf0` can set any eight-bit byte
  (`subsystem.cpp:270-276`), and the virtual port returns it (`:257-262`). No
  current-HEAD runtime trace proves which values reach actual IM0 acceptance.
- DI clears both IFFs (`z80c.cpp:973-976`). EI fetches the following opcode;
  except for DI/EI, it executes that instruction inside the EI handler, then
  enables IFF and tests IRQ (`:978-989`). For following DI/EI it rewinds and
  returns with IFF disabled. This fuses two architectural instructions into
  one legacy scheduling step and is not automatically a replacement contract.
- NMI is synchronous: it copies IFF1 to IFF2, clears IFF1, pushes live PC,
  charges 11 clocks, and jumps to `0x66` (`z80c.cpp:343-350`). No active
  external caller or pending NMI path was found.
- Memory helpers mask addresses to 16 bits and wrap word accesses
  (`z80c.cpp:230-254`). Common I/O helpers mask every external port to eight
  bits (`:256-263`); all scalar and block I/O instructions use those helpers.
- `inst` is authoritative live PC (`z80c.h:132-134,219-222`). `reg.pc` is the
  public mirror synchronized after the execution loop (`z80c.cpp:208-213`).
  I/O callbacks therefore see the previous return boundary through `GetReg()`.
- Reset zeroes `reg` and `uf` but never initializes `xf`
  (`z80c.cpp:317-338`). `GetAF()` merges `xf` bits 3 and 5 into F
  (`:430-435`), and save serializes `xf` (`:1977-1988`). This is demonstrated
  undefined legacy state, not behavior to reproduce.
- Status revision is 1 and fields are declared at `z80c.h:113-128`.
  `SaveStatus()` materializes AF, copies registers, overrides the mirror PC
  with live PC, and writes IRQ, WAIT/HALT, `xf`, remaining clock, and last
  clock (`z80c.cpp:1972-1991`). Load checks revision and restores those fields
  (`:1993-2006`). The C load bridge discards the Boolean result
  (`subsystem.cpp:365-367`), a robustness risk outside M34 behavior changes.

Architectural behavior to preserve comprises registers/flags, documented EI
inhibition, IM behavior, HALT, NMI, address wrapping, and instruction effects.
Externally observed vaeg behavior additionally comprises acceptance-time
acknowledge, level IRQ, eight-bit ports, clock debt, external WAIT, stale
callback-time public mirror, and revision-1 frame-boundary state. Legacy EI
fusion, uninitialized `xf`, public `TestIntr`, and diagnostic internals are old
implementation details or defects and are not automatically preserved.

## Production save boundary

The only active production `statsave_save` caller is the GUI state menu at
`sdl2/gui/gui.cpp:2352-2373`. The frame driver calls `pccore_exec()` and waits
for it to return before `gui_draw()` (`sdl2/np2.c:1192-1205`); `gui_draw()`
opens the state menu (`gui.cpp:2590-2616`). During `pccore_exec()`, each VA
slice calls `subsystemmx_exec()` (`machine/pccore.c:1078-1217`), which calls the C
bridge and then `Z80C::Exec()` (`io/subsystemmx.c:48-53` and
`subsystem.cpp:341-343`). All calls are synchronous on this path.

The save path is:

```text
run_guest_frame()
  -> pccore_exec() returns
  -> gui_draw()
  -> draw_state_menu()
  -> statsave_save()
  -> STATFLAG_SUBCPU (`machine/statsave.tbl:195`)
  -> flagsave_subsystemcpu()
  -> subsystem_savecpustatus()
  -> Z80C::SaveStatus()
```

`machine/statsave.c:1327-1417` drives the section table;
`machine/statsave.c:1273-1287` allocates the exact Z80-reported size and invokes the
bridge. Consequently `Exec()` and every synchronous Z80 callback have returned
and the CPU is at the completed instruction boundary established by the loop.
No instruction-, prefix-, bus-, or callback-internal microstate is reachable
at this save point. `lastclock`, live PC, and materialized flags are confirmed
above. The documentary claim that remaining credit may be positive is
disproved for production `Exec()`; zero and negative debt are reachable.

## Revision-1 ABI and retained fixtures

`tests/z80_compat/legacy_contract.cpp` is a ROM-less, test-only probe. It uses
hand-assembled instructions, exercises save/load at returned execution
boundaries, and embeds the expected bytes so drift fails CTest. It first
executes `XOR A` to avoid depending on uninitialized `xf`.

Native GCC and Clang on little-endian x86_64 report the same layout:

| Item | Size/offsets in bytes |
|---|---|
| primitives | `bool=1`, `uint=4`, `sint32=4`, `uint8=1`, `uint16=2`, `uint32=4`, pointer `=8` |
| `Z80Reg::wordreg`, `Z80Reg`, `Status` | `4`, `56`, `68` |
| main words | AF `0`, HL `4`, DE `8`, BC `12`, IX `16`, IY `20`, SP `24` |
| main bytes | F/A `0/1`, L/H `4/5`, E/D `8/9`, C/B `12/13`, XL/XH `16/17`, YL/YH `20/21`, SPL/SPH `24/25` |
| alternate/PC | AF' `28`, HL' `32`, DE' `36`, BC' `40`, PC `44` |
| control | I `48`, R-low `49`, R7 `50`, IM `51`, IFF1 `52`, IFF2 `53` |
| status tail | IRQ `56`, WAIT `57`, `xf` `58`, revision `59`, remaining `60`, last `64` |

The GCC report SHA-256 is
`6acc21e65a13da252e426657fa067ae732001183710bb1cd2bd9cc1c569fa5f9`;
the Clang report differs only in compiler text and hashes to
`bd6cd8822072abc8912ac48cfa2d0772be816f878bfbed8346792129b5578501`.
The MinGW x86_64 test executable and Clang layout-only probes for
x86_64-w64-windows-gnu, arm64-apple-macos11, and aarch64-linux satisfy the
same compile-time sizes. They were not executed locally; supported-platform
CI execution remains required.

The 15 retained byte fixtures cover ordinary/main and alternate registers,
IM0/1/2 with IFF enabled, IFF2-only after NMI, HALT, external WAIT, IRQ
asserted only after `Exec()` returns, negative remaining debt, nonzero last
clock, EI/NOP, EI/NOP with accepted IRQ, EI/DI, and EI/EI. The test performs
immediate byte-identical load/save for all and continuation comparisons for
all EI shapes, including memory/I/O traces and exactly one acknowledge at
acceptance. Exact 68-byte hex images live in the test source as executable
fixtures rather than duplicated prose.

Positive `remainclock` is absent from every reachable fixture. M37 should
nevertheless decode/encode a synthetic positive revision-1 value unchanged to
make the explicit codec total over the signed field; that is defensive format
coverage, not a claim of production reachability.

## SLEEP_HACK and private G39 procedure

The port-FE callback runs `SleepCheck_VA()` and `SleepCheck_Sorcerian()`
(`subsystem.cpp:205-255`). They compare stale mirror PC with `0x1732` and
`0x700e`, then set external WAIT. ATN bit 7 clears WAIT through the 8255 bridge
(`:313-325`). Do not alter the constants to compensate for a live-PC wrapper.

At G39, in a private-ROM build, enable a bounded debug trace containing slice-
entry live/mirror PC, port-FE mirror PC, WAIT transitions, ATN, and accepted
IM0 acknowledge bytes only. Verify VA subsystem idle reaches WAIT, Sorcerian
idle reaches WAIT, ATN/8255 releases WAIT, and normal FDD operation resumes.
Keep logs and ROM/disk data outside the repository.

## Candidate feasibility facts

The selected `z80.hpp` public `Register` exposes architectural registers,
I/R, IFF/IM, HALT, `consumeClockCounter`, and `execEI`. The execute loop can
return with `execEI` set after EI, so the revision-1 mapping decision in
ADR-0011 is required. Current interrupt code clears the upstream IRQ latch,
has no acknowledge callback, and handles IM0 through `RST` rather than raw
decode. Its normal opcode tables already provide the injection point needed
for a small extension. Dynamic debug decoding is not a public side-effect-free
API.

## Contradictions and hypotheses

Verified contradictions:

1. `M34_z80_migration_contract.md:370` and
   `z80_migration_master.md:292-294` say a post-`Exec()` balance can be
   positive, zero, or negative. Current `Exec()` and WAIT loops plus the
   positive multiplier prove the returned value is non-positive. Preserve
   zero/negative production state; do not redesign scheduling.
2. `z80_migration_master.md:25-27,572-574` says M33 is already represented in
   the roadmap. Current `ROADMAP.md` ends at M32, although M33 commits are in
   history. M34 does not alter or recreate M33.

Unverified hypotheses and future evidence needs:

- Private subsystem software probably accepts the reset `0x7f` in IM0, but
  only acceptance-time G39 tracing can prove it.
- The reserved WAIT bit 2 is sufficient for every reachable new-core EI
  boundary; M37 fixture/continuation tests must prove it on all build families.
- Normal decoder injection makes all raw prefixes maintainable; M35 upstream
  tests, not inspection alone, must prove cycles, R updates, and operand order.
- Preserving mirror refresh at `Exec()` return should preserve both sleep
  hacks; private G39 testing remains decisive.
