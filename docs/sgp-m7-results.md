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

# M7 logical-work and validation results

## Scope and evidence status

This report accompanies the M7a--M7d guest variants. M5 and M6 remain the
regression baselines; no stage-7 baseline was assumed. The table below gives
exact logical quantities implied by the command builder for a steady-state
frame. It is not a bus-transfer measurement. In particular, transparent
BITBLT destination writes depend on the source raster and are reported as
`N/A` rather than inferred from source footprint bytes.

The current VAEG source model is documented in
[`sgp-m7-audit.md`](sgp-m7-audit.md). VAEG modeled cycles, VBLANK slack,
displayed FPS, and the first step-onset count were not captured by a dedicated
host instrumentation pass in this rebuild; those cells are therefore `N/A`.
The earlier human observation of approximately 26 records at 57 FPS and 27 at
28 FPS remains a historical, timing-model-dependent baseline, not a continuous
workload curve.

## Logical matrix

`SGP commands` counts fetched opcodes, while `command words` counts 16-bit
list words. `BITBLT source` includes sprite/glyph source footprints; M7b--M7d
also include two logical bytes for each 1x1 zero PATBLT source. `BITBLT
destination` is the logical destination footprint of sprite/glyph copies plus
PATBLT rectangle footprints; transparent pixels skipped are not claimed as
writes. `Dirty rectangles` is the candidate count recorded by the page-local
cache; a `fallback` row emits full CLS instead of PATBLT rectangles.

| Variant | Records | SGP commands | Command words | CLS destination bytes | BITBLT source logical bytes | BITBLT destination logical bytes | Transparent pixels | Dirty rectangles | Full clears | VAEG modeled SGP time | VBLANK slack | Displayed FPS | First step-onset |
|---|---:|---:|---:|---:|---:|---:|---|---:|---:|---|---|---|---|
| M7a | 16 | 85 | 443 | 32000 | 4762 | 4762 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 24 | 109 | 571 | 32000 | 5018 | 5018 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 26 | 115 | 603 | 32000 | 5082 | 5082 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 27 | 118 | 619 | 32000 | 5114 | 5114 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 32 | 133 | 699 | 32000 | 5274 | 5274 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 48 | 181 | 955 | 32000 | 5786 | 5786 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 64 | 229 | 1211 | 32000 | 10394 | 10394 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 96 | 325 | 1723 | 32000 | 19610 | 19610 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 128 | 421 | 2235 | 32000 | 28826 | 28826 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7a | 256 | 805 | 4283 | 32000 | 65690 | 65690 | N/A | 0 | 1 | N/A | N/A | N/A | N/A |
| M7b | 16 | 151 | 742 | 0 | 4828 | 14174 | N/A | 33 | 0 | N/A | N/A | N/A | N/A |
| M7b | 24 | 207 | 1014 | 0 | 5116 | 14942 | N/A | 49 | 0 | N/A | N/A | N/A | N/A |
| M7b | 26 | 221 | 1082 | 0 | 5188 | 15134 | N/A | 53 | 0 | N/A | N/A | N/A | N/A |
| M7b | 27 | 228 | 1116 | 0 | 5224 | 15230 | N/A | 55 | 0 | N/A | N/A | N/A | N/A |
| M7b | 32 | 263 | 1286 | 0 | 5404 | 15710 | N/A | 65 | 0 | N/A | N/A | N/A | N/A |
| M7b | 48 | 375 | 1830 | 0 | 5980 | 17246 | N/A | 97 | 0 | N/A | N/A | N/A | N/A |
| M7b | 64 | 487 | 2374 | 0 | 10652 | 31070 | N/A | 129 | 0 | N/A | N/A | N/A | N/A |
| M7b | 96 | 325 | 1723 | 32000 | 19610 | 19610 | N/A | 193 (fallback) | 1 | N/A | N/A | N/A | N/A |
| M7b | 128 | 421 | 2235 | 32000 | 28826 | 28826 | N/A | 257 (fallback) | 1 | N/A | N/A | N/A | N/A |
| M7b | 256 | 805 | 4283 | 32000 | 65690 | 65690 | N/A | 513 (fallback) | 1 | N/A | N/A | N/A | N/A |
| M7c | 16 | 151 | 742 | 0 | 4828 | 14174 | N/A | 33 | 0 | N/A | N/A | N/A | N/A |
| M7c | 24 | 207 | 1014 | 0 | 5116 | 14942 | N/A | 49 | 0 | N/A | N/A | N/A | N/A |
| M7c | 26 | 221 | 1082 | 0 | 5188 | 15134 | N/A | 53 | 0 | N/A | N/A | N/A | N/A |
| M7c | 27 | 228 | 1116 | 0 | 5224 | 15230 | N/A | 55 | 0 | N/A | N/A | N/A | N/A |
| M7c | 32 | 263 | 1286 | 0 | 5404 | 15710 | N/A | 65 | 0 | N/A | N/A | N/A | N/A |
| M7c | 48 | 375 | 1830 | 0 | 5980 | 17246 | N/A | 97 | 0 | N/A | N/A | N/A | N/A |
| M7c | 64 | 487 | 2374 | 0 | 10652 | 31070 | N/A | 129 | 0 | N/A | N/A | N/A | N/A |
| M7c | 96 | 325 | 1723 | 32000 | 19610 | 19610 | N/A | 193 (fallback) | 1 | N/A | N/A | N/A | N/A |
| M7c | 128 | 421 | 2235 | 32000 | 28826 | 28826 | N/A | 257 (fallback) | 1 | N/A | N/A | N/A | N/A |
| M7c | 256 | 805 | 4283 | 32000 | 65690 | 65690 | N/A | 513 (fallback) | 1 | N/A | N/A | N/A | N/A |
| M7d | 16 | 151 | 742 | 0 | 4828 | 14174 | N/A | 33 | 0 | N/A | N/A | N/A | N/A |
| M7d | 24 | 207 | 1014 | 0 | 5116 | 14942 | N/A | 49 | 0 | N/A | N/A | N/A | N/A |
| M7d | 26 | 221 | 1082 | 0 | 5188 | 15134 | N/A | 53 | 0 | N/A | N/A | N/A | N/A |
| M7d | 27 | 228 | 1116 | 0 | 5224 | 15230 | N/A | 55 | 0 | N/A | N/A | N/A | N/A |
| M7d | 32 | 263 | 1286 | 0 | 5404 | 15710 | N/A | 65 | 0 | N/A | N/A | N/A | N/A |
| M7d | 48 | 375 | 1830 | 0 | 5980 | 17246 | N/A | 97 | 0 | N/A | N/A | N/A | N/A |
| M7d | 64 | 487 | 2374 | 0 | 10652 | 31070 | N/A | 129 | 0 | N/A | N/A | N/A | N/A |
| M7d | 96 | 325 | 1723 | 32000 | 19610 | 19610 | N/A | 193 (fallback) | 1 | N/A | N/A | N/A | N/A |
| M7d | 128 | 421 | 2235 | 32000 | 28826 | 28826 | N/A | 257 (fallback) | 1 | N/A | N/A | N/A | N/A |
| M7d | 256 | 805 | 4283 | 32000 | 65690 | 65690 | N/A | 513 (fallback) | 1 | N/A | N/A | N/A | N/A |

The dirty/full-clear crossover in the current implementation is a simple
logical-area guard: candidate rectangle pixel area reaches the 64,000-pixel
surface at 84 active records. This threshold is an algorithmic guard, not a
claim that VAEG or PC-88VA uses the same cost boundary.

## What was actually built and run

All four DOS 8.3 outputs were assembled from the same source with NASM
2.16.03:

~~~sh
NASM=/private/tmp/nasm-install/bin/nasm   demos/sgp-pseudo-sprite/build_m7_coms.sh /tmp/sgpd-m7
~~~

The resulting exact sizes in this rebuild were:

| File | Bytes |
|---|---:|
| `SGPD_7A.COM` | 20,910 |
| `SGPD_7B.COM` | 36,952 |
| `SGPD_7C.COM` | 55,000 |
| `SGPD_7D.COM` | 64,846 |

`SGPD_7C.COM` and `SGPD_7D.COM` were each installed into a fresh disposable
PC-Engine 1.1 boot disk and launched with the existing VAEG headless input
script. The script reached the `+` and `-` injections and completed without a
reported synchronization failure. The bounded headless run did not emit the
DOS exit summary, so no runtime FPS or counter value is claimed here.

## Milestone status and open measurements

- M7a separates calendar/FPS sampling and fixed transfer accounting from the
  sprite builder while retaining full CLS and synchronous rendering.
- M7b uses per-page old/current rectangle caches, verified zero PATBLT clears,
  and full-CLS fallback; it still redraws all sprites in painter order.
- M7c uses two command/work buffers and overlaps CPU state/list construction
  with the currently executing SGP list without changing page ownership.
- M7d precomputes immutable addresses and templates, patches only per-frame
  destination fields, and uses a Y offset table without an X table.
- M7e dirty redraw and M7f triple buffering are not implemented in this
  branch. Their CPU bookkeeping, pacing, and threshold questions remain open.

Consequently, the key optimization questions that require a future host
instrumentation run are explicitly unanswered here: measured SGP-cycle change,
CPU work hidden by M7c, command-template threshold movement, dirty-redraw
break-even, and triple-buffer presentation interval distribution. This keeps
algorithmic quantities separate from the current VAEG timing model and from
real-hardware performance claims.
