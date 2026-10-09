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
# M38: Add normalized legacy/new Z80 differential evidence

Status: implemented; G38 passed after eventual convergence was proven under
the maintainer-approved clock policy; stopped before M39

Branch: `topic/m38-z80-differential`

## Goal and harness

Compare externally observable legacy and replacement behavior without changing
production selection. Prefer separate legacy and new runner executables plus a
trace comparator, avoiding class/symbol collisions and production-source
renaming. Emit a versioned deterministic lower-case trace: checkpoint/reason,
authoritative and mirrored registers, IFF/IM/HALT, IRQ/NMI, ordered memory and
masked-I/O transactions, acknowledge events, clocks, end balance, selected
memory hashes, and explicit `Exec()` boundaries.

Normalize at slice return, accepted-interrupt completion, HALT entry/exit,
save/load, completed I/O, and test markers. Do not fail merely because legacy
EI fuses the following instruction; compare at the next common external
checkpoint.

## Corpus and classification

Directed programs cover base, CB/ED/DD/FD/DDCB/FDCB, branches, stack, block
memory/I/O, EI/DI/RETN/RETI, HALT, IM0/1/2, NMI, R, and ZEX-relevant flags.
Event scripts cover DI assertion, EI timing, acknowledge mutation,
deassertion, HALT wake, persistent IRQ, NMI, and frame-boundary state events.
Add bounded deterministic random programs with committed public seeds and a
documented longer local/nightly run.

Classify each divergence as wrapper defect, upstream defect, old-core defect,
deliberate architectural correction, externally inert scheduling difference,
or unresolved. An allowlist requires a minimal input, first event, affected
state, evidence, safety rationale, and issue link where applicable. ZEX alone
does not classify a divergence.

Create `docs/modernization/z80-compat-cycle-deltas.md`, recording scenario deltas and
whether they change event order, FDD behavior, interrupts, WAIT, HALT, or a
guest-visible result. G38 blocks on lost, duplicated, reordered, or
permanently divergent external effects. A slice-boundary shift caused by a
verified architectural timing correction may pass only when identical
additional clock slices prove eventual convergence. Every accepted shift
remains a mandatory M39 private integration risk.

## Gate G38

Run all repository QA, directed and bounded random corpora, reproducibility,
and the recorded long command. Require green or evidence-backed divergences,
an empty/evidence-backed allowlist, cycle report, and unchanged production
core. Push exact traces/report hashes, commands, files, and SHAs, then stop for
review.

## M38 result

The separate `vaeg_z80_legacy_trace`, `vaeg_z80_new_trace`, and
`vaeg_z80_trace_compare` executables implement the canonical
`vaeg-z80-trace-v1` schema without linking both `Z80C` classes into one
process. The normal directed, state, and four-seed generated corpora pass with
only the exact evidence-backed classifications recorded in the
[M38 report](../reports/m38_z80_differential.md). The long local run covers
16,384 generated cases with no allowlist match.

The probes found wrapper-correctable third-party behavior for prefix and HALT
R increments, RETN/RETI IFF restoration, `LD A,I`/`LD A,R` flags, and the
legacy NMI mirror. These corrections are confined to the standalone wrapper,
have focused regressions, and do not edit the vendored tree or production
selection.

The minimal `cycle-fdd-io-scheduling` suite runs `JR +2`
from `0x0000`, then `OUT (0xf4),A` at `0x0004`, with identical clock slices
`1,7`. The legacy core charges 7 clocks for the taken JR and performs the FDD
control-port write in the second `Exec()` call. The new core charges the
architectural 12 clocks and remains at `0x0004`, so it emits no write in that
call. This slice-exact difference stays documented and is intentionally not
relabelled as externally inert or removed.

The `cycle-fdd-io-eventual-convergence` test gives both runners the same
additional `4,1` slices. After 13 clocks the new core has emitted exactly one
`OUT (0xf4),0x5a`; the ordered event sequence, architectural registers,
memory, device-visible hashes, R, and final PC match legacy. There is no lost,
duplicated, or reordered I/O. The remaining differences are the classified
cycle count, clock balance, and earlier slice assignment. G38 therefore
passed under the revised policy. This scheduling difference is a mandatory
M39 private risk; M39 must stop integration on real VA/FDD transfer failure,
timeout, data corruption, IRQ/DRQ sequencing change, or broken save/load.
No legacy per-opcode timing emulation was added, and work is stopped before
M39.
