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

# M98v - Multi-ZUNDAMON full-page-clear baseline

Status: **automated evidence passed; G98v human gate pending**

Branch: `topic/m98v-multi-full-clear`

Accepted M98u implementation:
`61618f23b88730db157036d22fc2a3aa15986206`

Accepted M98u report head and starting commit:
`899678f28b301f62fa7096c7c5afb4d2cabf874b`

Commit prefix: `M98v:`

Gate type: **automated evidence plus VA2 human visual approval**

## Goal

Render complete public-fixture frames for the build-time counts 1, 2, 4, 8,
and 16. Generate the exact M98u instance records for one global phase, sort a
bounded index list by signed depth and then instance ID, clear the complete
hidden 64,000-byte G1 page, and draw every instance far to near. Publish only
after the complete transaction finishes and only on an eligible VBLANK edge.

The normal interactive build selects four instances and displays
`ZUNDAMON: 4`. There is no `/N` option, runtime count mutation, or UP/DOWN
action.

## Fixed transaction

The M98u 50-byte record ABI, 16-record capacity, direct phase assignment, and
16-byte sorted index list are authoritative. One atlas bank is selected while
the SGP is idle and remains stable until the clear and all ordered draws have
completed. Every steady frame performs exactly one full-page CLS and exactly
the build-time count of transparent BITBLTs. Separate bounded SGP lists are
permitted, but READY and DSA1 publication are forbidden before the last draw
has completed.

Global phase, page ownership, and complete-frame counters commit only after
publication. A missed slot retains the prior complete visible page and the
unskipped pending frame. Every failure uses the common bounded cleanup and
restoration path.

## Automated evidence

Build all five count variants twice. Run every count from both initial page
selections for two 64-phase revolutions, yielding 1,280 complete physical-page
and composite comparisons. Run all 40 count/divisor combinations plus the
count-four selector ladder, pause/resume, and injected missed-slot cases.
Validate full-page coverage, command order, draw order, transparent
composition, static count HUD, count-one M98t compatibility, guards, and
stable fail-closed negative diagnostics with an independent host compositor.

Natural public-fixture positions do not overlap. Preserve that geometry; test
opaque-near, transparent-over-far, and equal-depth visual ordering with a
host-only synthetic compositor fixture rather than moving the orbit.

## Non-goals

M98v does not implement dirty-row interval unions, runtime count controls,
`/N`, UP/DOWN behavior, atlas duplication, private imagery, bullets, sound,
gameplay, or a general sprite engine. M98w owns dirty unions and M98x owns
runtime count selection and HUD updates.

## Gate status

The automated result is recorded in
`../reports/m98v_zundamon_multi_full_clear.md`. The required interactive
count-four VA2 visual and ESC-restoration approval remains outstanding.

`REAL_HW_PENDING`

`G98v: human gate pending`
