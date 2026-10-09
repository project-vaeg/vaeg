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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M7-0 SGP pseudo-sprite audit

## Scope

This audit precedes the M7 guest optimizations. It records the behavior that
is present in VAEG at the audit baseline; it does not change the SGP timing
model or any existing demo.

## Demo inventory and baseline

There is no stage-specific demo in the current tree. The final runnable
variants are generated from [`sgp_sprite.asm`](../demos/sgp-pseudo-sprite/sgp_sprite.asm).
The M5/M6 rows below are historical baseline records from the pre-final source
layout; they are not build targets of the current tree.

| Baseline | Source | Generated file | Rendering path |
|---|---|---|---|
| M5 (historical) | former `MILESTONE_STAGE=5` | former `SGPDEMO5.COM` | 320x200, 4bpp, G1 pages A/B, full hidden-page `CLS`, transparent SGP BITBLT |
| M6 (historical) | former `MILESTONE_STAGE=6` | former `SGPDEMO6.COM` | M5 path plus 8x8 bullets, counters, and 1-256 active records |

The former educational source ladder and baseline generator are historical
artifacts and are no longer shipped under `demos/sgp-pseudo-sprite`.
The historical baseline rebuild produced `SGPDEMO5.COM` (20,746 bytes) and
`SGPDEMO6.COM` (20,734 bytes) with the repository NASM tool. Those files are
not emitted by the final builder. A bounded VAEG run booted the disposable
PC-Engine disk, launched M5, exercised `+` and `-`, and exited without a
synchronization error. This is a smoke result, not a hardware performance
claim.

## VAEG SGP execution model

The implementation is in [`io/sgp.c`](../io/sgp.c) and
[`io/sgp.h`](../io/sgp.h). `sgp_step()` is called from the emulator's timed
execution. It adds elapsed CPU-clock units (scaled by the configured SGP
clock ratio) to `sgp.remainclock` and executes command fetch or drawing steps
while the budget is positive. This is a cooperative emulator model, not a
host thread running concurrently with the CPU.

The model clock is `PCBASECLOCK40` for VA and twice that for VA2. The
`model`, `follow-cpu`, and custom SGP multiplier modes are selected through
the existing configuration/CLI and must be treated as timing-model inputs.
No SGP timing coefficient is changed by M7.

### Command costs in the current model

The following are source-level deductions from `io/sgp.c`:

| Operation | Fixed deduction in the current model |
|---|---:|
| `SET_WORK` | `23 * 2` |
| `SET_SOURCE` | `106 * 2` |
| `SET_DESTINATION` | `106 * 2` |
| `SET_COLOR` | `10 * 2` |
| `BITBLT` / `PATBLT` setup | `338 * 2` |
| `CLS` setup | `26 * 2` |

`END` itself has no explicit clock deduction. BITBLT and PATBLT then share
the same execution engine. For forward 4bpp transfer, the current model
charges `8 * 2` per destination word for opaque transfer, `10 * 2` for
source-transparent transfer, and `14 * 2` per completed row. The inner pixel
packing loop determines the word/mask contents but has no separate per-pixel
clock deduction. Start-dot alignment, bpp, direction, ROP, and transparent
mode can therefore affect the path, but this is not a simple byte or pixel
throughput model.

`CLS` writes one word in `exec_cls()` and deducts `3 * 2` per word. Its
timing is therefore fixed setup plus word-count dependent; it is not an
instantaneous host-memory fill.

### CPU interaction and polling

The SGP busy bit is controlled through `0506h`. Starting a list sets the SGP
function to command fetch; `END` clears busy. Status reads do not have a
special SGP penalty in `io/sgp.c`. They do consume CPU execution time, and
the resulting elapsed CPU clock is what lets `sgp_step()` advance. A tight
poll therefore replaces useful CPU work even though it does not add a
separate modeled bus charge.

## Clear/fill primitives

`CLS (000Ah)` consumes an even 32-bit address and a 32-bit word count and is
a contiguous linear fill. It cannot clear an arbitrary pitch-aware rectangle
without also writing the gaps between rows.

`PATBLT (0008h)` is present in the command table and calls the same block
engine as BITBLT, with source wrapping in X/Y. VAEG's logical operation table
maps ROP 5 to source copy. With a 1x1 4bpp RAM word containing zero, mode
`0005h` (`TP=0`, source-copy ROP) is therefore the verified VAEG mechanism for
an opaque zero rectangle: the source zero is copied, not skipped. Mode `0105h`
would be wrong for clearing because TP=1 suppresses source-zero pixels.
This use is implementation evidence and is kept separate from any claim about
an authoritative hardware timing formula.

## VBLANK and display page

The demo polls TSP status port `0142h`, bit `40h`, waiting for low then high.
It writes FB1 display-start address through `022Eh`--`0230h` only after that
transition. This is the existing VAEG display scheduling path; no VBLANK
interrupt is assumed.

## Measurement consequences

The existing M6 path calls calendar BIOS `INT 8Ch/AH=02` once per frame and
performs `MUL` arithmetic for fixed sprite transfer statistics inside the
command-builder loop. Those operations are part of the measured CPU path, so
M7a will move the FPS time base to low-frequency/VBLANK-based bookkeeping and
hoist fixed transfer quantities. The existing 60/30-style step remains a
secondary VAEG timing-model regression metric, not a continuous hardware FPS
curve.

M7 logical-work counters will use the names `logical` or `modeled`; they will
not be described as physical bus bytes. A dirty clear can reduce logical
destination/source work while producing a smaller or larger VAEG cycle change,
because the current model weights command setup, words, rows, and transparent
mode separately.

## M7 implementation boundary
