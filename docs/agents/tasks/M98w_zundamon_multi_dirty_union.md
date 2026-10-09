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

# M98w - Multi-ZUNDAMON dirty-row interval unions

Status: **G98w passed; automated and human-gate evidence complete**

Branch: `topic/m98w-multi-dirty-union`

Accepted M98v pushed head: `33d15aa090f392d3393083e0ebab99965fc06d22`

Commit prefix: `M98w:`

## Goal

Replace the M98v steady-state complete hidden-page clear with a page-local
union of the committed old multi-instance footprint. Each old logical
rectangle is validated, rounded outward to complete 16-bit words, sorted by
row and x, and merged on overlap or adjacency. Canonical row-major CLS ranges
are emitted in bounded SGP batches. The clear barrier completes before the
unchanged M98v far-to-near transparent BITBLTs, READY state, and VBLANK
publication.

Both physical pages retain independent committed footprints. The first hidden
use skips dirty clearing because both pages are initialized once with a full
clear. Pending rectangles commit only after publication. The accepted M98v
full-clear path remains available as a QA golden mode; the release default is
dirty mode.

## Fixed scope

Build-time counts remain exactly 1, 2, 4, 8, and 16, with interactive count 4.
There is no `/N` option and UP/DOWN remain inactive. M98u phase assignment,
descriptor geometry, one-bank atlas, transparent source-zero behavior, HUD,
cadence, page ownership, and cleanup are unchanged. M98w does not add private
IDA data, runtime count controls, interval masks, or a general sprite engine.

## Verification and gate

The independent host union oracle covers word parity, overlap, containment,
adjacency, transitive chains, disjoint gaps, capacity, row/address bounds,
and one-rectangle M98q equivalence. VA2/VAEG bounded captures compare dirty
and full physical G1 and composited bytes, command order, page parity, source
records, and guards. Logical clear work and timing are reported separately;
reduced bytes are not a performance claim.

`REAL_HW_PENDING`

`G98w passed`
