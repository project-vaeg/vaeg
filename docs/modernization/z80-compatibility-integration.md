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
# Z80 compatibility production integration

The production subsystem unconditionally uses the suzukiplan-backed vaeg
wrapper. There is no build-time or runtime core selector:

```sh
cmake --preset linux-ci-gcc
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure
```

The production `vaeg_va` target compiles `cpu/z80_compat_cpu.cpp`,
`cpu/z80_compat_state.cpp`, and `cpu/upd780/upd780_disasm.cpp`. Standalone
conformance, state, disassembler, ZEX, and M38-derived regression targets
remain available.

The common wrapper exposes `Z80CompatCpu`: `UPD70008C` is the main CPU adapter,
and `UPD780C` is the FDC seam and C bridge in
`io/subsystem.cpp`. Third-party and STL types do not cross that seam. Every wrapper I/O callback masks the external port to eight bits, IRQ
remains a level, and the acknowledge port is read only when the core accepts a
maskable interrupt. The independently authored BSD-2-Clause decoder in
`cpu/upd780/upd780_disasm.cpp` is the only production uPD780C disassembler.

## ROM-less integration evidence

With `VAEG_ENABLE_TESTS=ON`, `vaeg --selftest` runs the implementation
through the real `Subsystem`, I/O, clock, and state-save bridge. It covers:

- ordinary execution, live PC, and return-fresh public register mirror;
- callback-stale public PC at both VA and Sorcerian SLEEP_HACK addresses;
- `Wait(true)`, clock draining, ATN wake, `Wait(false)`, and resumed execution;
- the focused EI-immediately-before-VA-sleep hypothesis;
- DI suppression and exactly one acceptance-time IM1 acknowledge read;
- retained revision-1 input, same-core round trip, HALT, external WAIT, IRQ
  level, EI inhibition, and signed clock fields;
- rejected revision propagation through the complete `statsave_load()` path;
- one production `OUT (0xf4),0x5a`, save/load between CPU calls, and proof
  that the event is not duplicated on resume; and
- bounded, side-effect-free production disassembly.

The synthetic sleep trace has the same observed contract for each core. At
the IN callback, live PC has advanced past `db fe` while public PC still names
the instruction address. At `Exec()` return both PCs agree. WAIT then drains
the next slice without advancing PC; ATN releases WAIT and the following slice
resumes at the next opcode. The production constants `0x1732` and `0x700e`
are unchanged.

The legacy core fuses EI and its following instruction inside one private
step. The new core can return immediately after EI, so the focused synthetic
case reaches the SLEEP_HACK callback on the following call with the required
stale public PC. This confirms the M38 hypothesis without changing either
magic constant. Private guest testing remains mandatory because the synthetic
case cannot prove the actual firmware path.

## State boundary and error handling

The production save boundary remains after `Z80CompatCpu::Exec()` returns. No callback
or partial instruction is serialized. The revision-1 image remains 68 bytes
and represents architectural state, HALT, external WAIT, external level IRQ,
`remainclock`, and `lastclock`; new-core bit 2 retains frame-boundary EI
inhibition. The subsystem mirrors restored external WAIT only for tracing and
wake diagnostics; the core's restored state remains authoritative.

`subsystem_savecpustatus()` and `subsystem_loadcpustatus()` now return success
or failure. `machine/statsave.c` propagates a codec rejection instead of continuing as
if an unsupported compatibility-state revision had loaded successfully. The ROM-less test
copies a valid state, changes only the embedded compatibility-state revision, and requires the
top-level load to fail before loading the valid state again.

## Optional private trace

Trace instrumentation is disabled and absent from normal builds. Enable it
only in a separate diagnostic tree:

```sh
cmake -S . -B build/z80-compat-private-trace -G Ninja \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON
cmake --build build/z80-compat-private-trace
./build/z80-compat-private-trace/sdl2/vaeg --fdctrace
```

The existing FDC trace stream then adds deterministic compatibility-CPU `Exec()` entry and
return, live/public PC, IRQ level, acceptance byte, masked IN/OUT, WAIT
assertion/release, and state-boundary records. It contains no host pointer or
timestamp. Formatting and file I/O can perturb host wall-clock scheduling, so
it is diagnostic evidence rather than a claim of uninstrumented wall-clock
performance. Emulated clock values and ordered device events remain explicit.
Private filenames, paths, hashes, screenshots, raw traces, saves, and media
stay outside Git. Tracked records use neutral stable identifiers only.

## G39 private disposition

M38's exact `0xf4` reproducer remains applicable: corrected JR timing can move
an otherwise identical FDD output to a later `Exec()` slice. M39 adds no
legacy per-opcode cycle emulation. The
[private integration manifest](z80-compatibility-private-integration.md) passed under both
production selections. Boot, read/write, representative and timing-sensitive
loaders, SLEEP_HACK, WAIT wake, and state-load behavior converged without a
guest-visible failure.

The representative trace emitted the same 21 ordered port-`0xf4` writes and
an exact 164-record common prefix of completed FDC commands. A fixed
wall-clock sample caught the new core in a later intermediate loader scene,
then sufficient execution reached the same stable menu. This remains an
accepted scheduling difference, not a lost, duplicated, reordered, or
permanently divergent device effect. The current trace has no dedicated DRQ
edge record, so DMA transfer metadata, IRQ/acknowledge events, final data, and
guest-visible completion provide the recorded evidence instead.

G39 passed on 2026-07-15. Its dual-selection procedures are retained as
historical evidence; M41 subsequently removed the selector and old source.

## M41 final cutover

The current tree contains only the wrapper-backed production path and the
independently authored disassembler. The seven approved M88/cisc-derived Z80
files are absent from current HEAD and release inputs; Git history was not
rewritten. Revision-1 remains 68 bytes, retained legacy fixtures still load,
and `LoadStatus()` rejection propagation remains active.
