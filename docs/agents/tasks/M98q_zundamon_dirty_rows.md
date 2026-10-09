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

# M98q - Add page-local dirty-row clearing

Status: **G98q human gate passed on 2026-09-01; M98q closed**

Branch: `topic/m98q-zundamon-dirty-rows`

Starting commit: `05df2d2d069f00b8b5d99d80dfc4979d4482757b`

Accepted M98p implementation: `4e9c57975a2e3705bc7cb2c29b3b94e5b88f4bea`

Accepted M98p report head: `7b0102bddf3734d7d440892b3753231033578a17`

Accepted M98p gate head: `05df2d2d069f00b8b5d99d80dfc4979d4482757b`

Commit prefix: `M98q:`

Gate type: **automated VA2/VAEG evidence plus maintainer human gate**

## Goal

Replace M98p's steady-state 64,000-byte hidden-page clear with a page-local
dirty-row clear for exactly one public synthetic pseudo-sprite. M98p remains
the byte-correct full-clear golden. Atlas bytes, the `30..1..29` scale
sequence, fixed anchor, transparent BMS-to-G1 draw, page ownership, SGP
completion rule, VBLANK publication rule, and restoration behavior remain
unchanged.

## Fixed clearing contract

Each physical G1 page owns an independent half-open logical rectangle for its
most recently published sprite. Both pages are fully cleared once during
initialization and begin with invalid old-rectangle state. On page reuse,
round the old horizontal interval outward to complete 16-bit words:

```text
clear_x0 = x0 & ~1
clear_x1 = (x1 + 1) & ~1
clear_words = (clear_x1 - clear_x0) / 2
```

Issue exactly one zero-valued SGP CLS for each old scanline, splitting only at
the checked command-list capacity. Complete every old-row clear before the
single transparent BITBLT. Commit the pending new rectangle, swap page roles,
and advance the scale only after SGP completion and a fresh VBLANK low-to-high
edge. A failed transaction publishes and commits nothing.

Release steady state must contain no full-page CLS. The algorithm is limited
to one homogeneous G1 object and must not be generalized to multiple objects.

## Automated evidence

First reproduce the accepted M98p VA2 full-clear identities. Then run two
complete 58-publication cycles for each combination `A/full`, `A/dirty`,
`B/full`, and `B/dirty`. Compare every dirty physical G1 page and composited
frame byte-for-byte with the corresponding full-clear golden. Verify both
page parities, both directions, endpoint reversals, cycle wrap, first use,
page reuse, stale-pixel removal, word-rounded guard bounds, counter
arithmetic, deterministic builds, and the required fail-closed negative
cases.

For each 116-publication dirty run require exactly two initialization full
clears, zero steady-state full clears, 114 dirty rectangles, 116 transparent
BITBLTs, 116 flips, zero mismatches/guard failures/timeouts/errors, and one
cleanup. Record dirty CLS commands, words, and bytes separately from the
116-frame full-clear baseline and make no elapsed-performance claim.

## Human gate

Provide one pristine generated D88. In VA2 the maintainer must confirm the
same centered 30-to-1-to-29 cycle as M98p, no stale silhouette or horizontal
word-rounding streak, a stable anchor, correct transparent holes, no partial
page, no parity difference, no new flicker or tearing, and successful ESC
restoration. Automation cannot close the human gate.

## Non-goals

M98q does not add cadence controls, orbit movement, depth coupling, private
assets, multiple instances, dirty-row interval unions, sound, gameplay,
elapsed-performance claims, or physical-hardware evidence.
