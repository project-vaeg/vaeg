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

# M82: FDC uPD780-compatible CPU audit

Status: M82 is closed after the G82 human gate passed on 2026-08-11. No
production behavior or source path was changed by M82.

Evaluated branch: topic/m82-upd780-subsystem-cpu-audit

## Result

The FDC subsystem is driven by the UPD780C compatibility name, which is an
alias of the shared suzukiplan-backed Z80CompatCpu. The active contract is
the FDC subsystem bus and scheduler contract, not a second main-CPU emulator:

    io/subsystem.cpp
        owns UPD780C, ROM/RAM, FDC ports, I8255, IRQ/WAIT, and save/load boundary
            -> cpucva/z80_compat_cpu.cpp
               shared instruction/scheduler/state implementation
            -> cpucva/z80_compat_state.cpp
               explicit little-endian legacy state codec

The shared execution backend must remain shared with the main uPD70008
adapter. It is not correct to move z80_compat_cpu.*, z80_compat_bus.h,
z80_compat_registers.h, or z80_compat_state.* as if they were FDC-only
files.

## FDC CPU contract

io/subsystem.cpp is the hardware-facing owner of the FDC CPU instance.

- Construction creates one UPD780C, a clock, and a clock counter.
- Initialization binds the CPU to the subsystem memory and I/O interfaces,
  the subsystem clock, and the piac2 interrupt-acknowledge port.
- The CPU-visible ROM is 0000h-1fffh; RAM is 4000h-7fffh. Unmapped reads
  return ffh and writes outside RAM are ignored.
- Execution is scheduled through subsystem_exec() and UPD780C::Exec().
- Reset calls the compatibility CPU reset and restores the FDC subsystem
  state.
- IRQ is delivered through UPD780C::IRQ(0, irq). WAIT is delivered through
  UPD780C::Wait().
- The FDC I/O boundary is owned by Subsystem: FDC control/data ports are
  at f8h, fah, and fbh; I8255 ports are at fch-ffh; and the subsystem
  virtual ports expose interrupt acknowledge and related state.
- Save/load delegates to the compatibility CPU state boundary and restores
  the subsystem WAIT mirror from the saved state.

subsystemif.c only binds the C-side I8255 and VA-facing port callbacks. It
does not own a CPU and must stay under io/. subsystemmx.c selects the real
subsystem or the mock-up path. fdsubsys.c is a CPU-free mock-up
handshake/state machine and has no uPD780 wrapper, compatibility backend, or
legacy CPU codec dependency; it must not be moved to cpu/upd780/.

The SLEEP_HACK branches are subsystem-specific scheduling workarounds for
known FDC ROM PCs. They are not evidence of a separate CPU core or main CPU
emulation mode.

## Shared backend and state boundary

cpucva/z80_compat_cpu.cpp owns the suzukiplan Z80 instance and the generic
memory, I/O, clock, IRQ, WAIT, and execution callbacks. Its public aliases
are:

    using UPD70008C = Z80CompatCpu;
    using UPD780C = Z80CompatCpu;

The main adapter cpucva/upd9002_upd70008.cpp constructs UPD70008C and uses
the same backend for its compatibility mode. It also has main-CPU-specific
memory, I/O, and register hooks. Therefore the following files are shared
infrastructure and are not an M83 rename candidate:

- cpucva/z80_compat_cpu.cpp and cpucva/z80_compat_cpu.h
- cpucva/z80_compat_bus.h
- cpucva/z80_compat_registers.h
- cpucva/z80_compat_state.cpp and cpucva/z80_compat_state.h

The state codec is revision 1, 68 bytes, with explicit little-endian fields
and revision rejection. Both the FDC subsystem and main adapter use this
codec through their respective state boundaries. Moving it with the FDC
would break the main adapter state contract.

## Disassembler ownership

cpucva/upd780_disasm.cpp and cpucva/upd780_disasm.h implement the bounded
uPD780-compatible disassembler. The active semantic callers found by the
audit are:

- io/subsystem.cpp, for the FDC subsystem debugger/disassembly bridge;
- tests/upd780/disasm.cpp, for the focused disassembler test.

There is no semantic caller from cpucva/upd9002_upd70008.cpp; its CMake
membership is production target composition, not a call edge. This makes
the two upd780_disasm files the only existing cpucva/ files identified as
FDC-specific for the M83 path move. The z80_compat_* backend and codec
remain shared, as required above.

The current repository instructions also describe upd780_disasm.* as part
of the compatibility layer. M83 must preserve that API and behavior while
making the path role-accurate; if that policy is interpreted as prohibiting
the move, retain these two files in cpucva/ and add only a thin
cpu/upd780/ FDC facade. No shared backend file may be moved merely to make
the directory appear complete.

## Exact M83 move boundary

The evidence-backed rename-only candidate is:

    cpucva/upd780_disasm.cpp -> cpu/upd780/upd780_disasm.cpp
    cpucva/upd780_disasm.h   -> cpu/upd780/upd780_disasm.h

M83 must update only the corresponding subsystem/test includes and CMake
source paths in its separate reference-fixup commit. It must not move
io/subsystem.cpp, io/subsystem.h, io/subsystemif.c, io/subsystemif.h,
io/subsystemmx.c, io/fdsubsys.c, or the shared z80_compat_* files.
The UPD780C alias and suzukiplan execution behavior must remain unchanged.
If M83 adds a role-specific FDC facade, it must be a thin adapter over the
shared backend, with no duplicated instruction implementation.

## Validation

The following focused checks passed on this branch:

| Check | Result |
| --- | --- |
| vaeg_z80_compat_wrapper_default | passed |
| vaeg_z80_compat_wrapper_no_functional_test | passed |
| vaeg_upd780_disasm | passed |
| FDC subsystem integration through vaeg --selftest | passed |
| Linux production linux-debug clean-first build | passed |
| MinGW mingw-cross clean-first build | passed |

The FDC selftest exercised both subsystem sleep paths and reported all tests
passed. The test-enabled Linux build also exposed an existing stale M69 test
reference to removed iocoreva_* symbols; M82 did not alter that unrelated
history. A complete CTest sweep was therefore not used as an M82 pass claim:
the focused CPU/FDC tests and production builds above are the relevant
results for this audit.

The maintainer confirmed that the manual FDD boot/access gate passed as part
of G82 on 2026-08-11. M82 is closed; M83 remains unstarted.

## M83 handoff

M83 may now begin as a separate milestone. Its first commit should be the
rename-only move of the exact files above; its following commit should update
references and CMake. M83 must rerun the focused wrapper, disassembler,
subsystem, save/load, repository, and build checks before requesting G83.
