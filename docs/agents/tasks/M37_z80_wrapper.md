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
# M37: Add the vaeg Z80 wrapper and revision-1 codec

Status: complete; G37 passed; stopped before M38

Branch: `topic/m37-z80-wrapper`

## Goal and boundaries

Implement the ADR-0011 vaeg-owned BSD-2-Clause bus, register mirror, explicit
legacy-state codec, and `Z80C` wrapper around the vendored core. Build them
only as standalone/test targets. Do not connect `iova/subsystem.cpp`, change
the emulator's legacy core, implement final disassembly, or delete any file.

## Required design

Use the M34-approved filenames and fixed-width public data. No STL or third-
party type crosses the consumer, C bridge, debugger, or state boundary. Keep
the M34-proven used signatures and architectural `GetPC`, `SetPC`, and NMI.
Keep `GetDiag` only as a documented placeholder if compilation requires it;
do not preserve unused diagnostics.

The explicit 68-byte revision-1 codec uses the recorded offsets, not a cast to
runtime state. It must decode every M34 fixture, reject unsupported revision,
materialize AF, map all architectural/control registers, IRQ level, HALT,
external WAIT, remaining and last clock, and translate HALT PC. Reserve WAIT
bit 2 for reachable new-core EI inhibition and prove new-to-new retention.
Production-reachable returned clock balances are zero/negative; also exercise
a synthetic positive signed field for total codec round-trip. Never serialize
instruction/callback microstate.

`Exec()` adds unsigned clock delta, drains positive credit under external WAIT,
otherwise retires complete instructions while positive, permits zero/negative
overshoot, refreshes the public mirror only on return, and saves last clock.
Use fine-grained core clock consumption. All external I/O, including block
forms, uses `port & 0xff`. Use G35's level line and acceptance callback. Save
and `GetPC` use authoritative state; callback-time `GetReg()` remains stale.

## Minimum tests

With fake memory/I/O/clock/counter, cover reset, execution, 16-bit wrapping,
all I/O forms, clock accumulation/overshoot, WAIT drain/resume, live PC,
callback-time stale mirror and return refresh, EI/DI/deassertion/persistent
IRQ, changed acknowledge after EI, IM0 `0x00`, `0x7f`, every RST and prefix/
multi-byte class, IM1/2, HALT entry/wake, NMI, R behavior, every M34 legacy
fixture, new round trip, unsupported revision, synthetic positive and reachable
zero/negative clock values, nonzero last clock, and HALT/WAIT/IRQ/EI saves only
after `Exec()` returns. Run ZEX through the wrapper and sanitizers where
supported.

## Gate G37

Run all existing QA plus wrapper tests on every supported platform and ZEX
through the wrapper. Confirm no production source selection or guest behavior
changed and no deletion occurred. G37 is machine-verifiable only after all
platform jobs pass. Push and report exact logs, SHAs, and mapping updates,
then stop.
