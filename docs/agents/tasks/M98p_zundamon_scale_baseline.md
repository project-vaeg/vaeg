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

# M98p - Add the 30-scale full-page-CLS zoom baseline

Status: **G98p human gate passed on 2026-08-31; M98p closed**

Branch: `topic/m98p-zundamon-scale-baseline`

Starting commit: `543e06114a63c5f7c9f678806d11c221da96ed94`

Accepted M98o implementation: `ddc70c692ecb65066269c9894eb4b14f702fd2d9`

Accepted M98o report head: `71bcdf3467a26dc4eaeb5ca0167fe9e01a26ef20`

Commit prefix: `M98p:`

Evaluated implementation: `4e9c57975a2e3705bc7cb2c29b3b94e5b88f4bea`

Result: [`../reports/m98p_zundamon_scale_baseline.md`](../reports/m98p_zundamon_scale_baseline.md)

Gate type: **automated VA2/VAEG evidence plus maintainer human gate**

## Goal

Extend the accepted M98o hidden-page renderer so the public synthetic atlas
visits all 30 stored scale descriptors. Every update clears the complete
64,000-byte hidden G1 page, transparently BITBLTs one BMS-resident scale,
waits for SGP completion, and publishes only on a fresh low-to-high VBLANK
edge. This is the deliberately unoptimized byte-correct baseline for M98q.

## Accepted predecessor and numbering

The maintainer stated that G98o passed after observing the M98o marker and
successful ESC restoration in VA2 mode. M98l already absorbed BMS mapping,
bounded atlas streaming, and direct BMS-to-G1 transfer; M98m and M98n remain
reserved. M98p does not repeat the obsolete atlas-streaming scope.

## Fixed contract

| Item | Value |
|---|---:|
| Logical mode | 320x200 VA direct-color 8-bpp |
| G1 backing surface | 320x400, 320-byte pitch |
| G1 page size | 64,000 bytes |
| Atlas | public synthetic `ZUNDORB.BIN` |
| Scale descriptors | exactly 30, implicit IDs 1 through 30 |
| Atlas bank requirement | exactly one 128 KiB BMS bank |
| Source transparency | byte `00h` only |
| Per-update clear | one full hidden-page SGP CLS |
| Per-update draw | one transparent BMS-to-G1 BITBLT |
| Runtime scaling | none |
| Publication | after SGP completion on fresh VBLANK low-to-high edge |

All descriptor dimensions, pitch, scaled anchor, payload size, bank-relative
offset, loaded bounds, and frame CRC must be validated before graphics mode.
The fixed target anchor is `(160,100)` and every destination must fit without
clipping.

## Scale sequence

The exact 58-publication sequence is:

```text
30, 29, 28, ..., 3, 2, 1, 2, 3, ..., 28, 29
```

Endpoints are not duplicated. The sequence advances only after a completed
page is published. Release behavior repeats the sequence until ESC. Bounded
QA runs exactly one cycle and exits without keyboard input.

## Automated evidence

Build the public atlas and guest twice and compare hashes. Run two bounded
VA2 tests with identical content and opposite initial visible pages. Together
they must cover every scale on both physical G1 pages and both directions for
interior scales. The standard-library oracle independently validates the
descriptor table, sequence, anchors, full clears, transparency, source and
destination ranges, page identities, counter invariants, SGP trace, stable
final frames, and deterministic negative cases.

## Human gate

Provide one pristine generated D88. In VA2 the maintainer must confirm the
center-anchored synthetic sprite shrinks from 30 through 1, grows through 29,
and repeats without endpoint pauses, wobble, stale silhouettes, clear-only
frames, page-parity differences, flicker, or tearing. Transparent holes must
show G0 and ESC must restore the guest environment. The automated VA2
evidence passed at `4e9c57975a2e3705bc7cb2c29b3b94e5b88f4bea`. After
confirming that the single public marker performed the expected
shrink-and-grow sequence, the maintainer explicitly stated `G98p passed` on
2026-08-31. M98p is closed.

## Non-goals

M98p does not add dirty-row clearing, cadence controls, orbit movement, depth
coupling, private assets, multiple instances, sound, gameplay, performance
claims, or physical-hardware evidence. `REAL_HW_PENDING` remains separate.
