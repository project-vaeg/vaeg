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

# M98s - Add a constant-size 64-phase ellipse

Status: **G98s human gate passed; M98s closed on 2026-09-01**

Branch: `topic/m98s-64-phase-ellipse`

Starting and accepted M98r commit:
`4c5a7724e31cc0a52c8bfe8e827198c1c30a8c37`

Commit prefix: `M98s:`

Gate type: **automated VA2/VAEG evidence plus maintainer human gate**

## Goal

Move the one public marker clockwise around a deterministic 64-phase
screen-space ellipse while using stored scale ID 15 for every render.  The
anchor-space center is `(160,100)`.  Phase 0 is right, phase 16 bottom, phase
32 left, and phase 48 top.  A phase advances only after a complete hidden G1
page is published on an eligible VBLANK edge.

Generate the signed integer table on the host from one canonical fixed-point
unit-circle table.  The guest performs no runtime trigonometry or scaling.
Validate all 64 anchored rectangles before graphics mode and before each SGP
submission.

## Preserved renderer and cadence contract

Keep the complete public 30-descriptor, one-bank atlas, but bind only scale 15
in the release loop.  Preserve independent page-A/page-B old rectangles,
word-rounded dirty-row CLS, two initialization full clears, zero steady-state
full clears, one transparent BMS-to-G1 BITBLT, READY-only hidden-page
publication, and ordinary-mapping/video restoration.

Preserve `/V1` through `/V8`, LEFT/RIGHT cadence selection, SPACE
pause/resume, ESC cleanup, missed-slot retention, debounce, and boundary-reset
semantics from M98r.  UP/DOWN remain inactive.  Pause, misses, divisor changes,
and failed transactions never advance the pending phase.

## Automated evidence

Reproduce M98r before editing.  Regenerate and validate the orbit table twice,
build twice, and compare hashes.  Compare dirty and full-clear output for
`A/full`, `A/dirty`, `B/full`, and `B/dirty`, each over 128 publications.  Run
all eight 64-publication static divisors, opposite-page 128-publication cases
at V1/V4/V8, the selector ladder, pause/resume, missed-slot, bounds, page-state,
and fail-closed negative tests.  Every publication must match an independent
indexed framebuffer oracle.

Generated COM, BIN, D88, traces, captures, reports, and backup memory stay
outside Git.  No private or ROM-derived material may enter tracked files.

## Human gate

Provide one pristine generated D88.  In VA2, the maintainer checks one
constant-size clockwise ellipse for at least two revolutions; no trail,
clipping, flicker, tear, or partial page; unchanged transparency; inherited
cadence and pause controls; inactive UP/DOWN; and normal ESC restoration.
Automation cannot close G98s.

The maintainer explicitly stated `G98s passed` on 2026-09-01 after the
interactive VA2 check. M98s is closed at this gate.

## Non-goals

M98s does not add depth, phase-to-scale mapping, private imagery, image
rotation, multiple instances, UP/DOWN count controls, gameplay, audio,
emulator timing changes, or physical-hardware performance evidence.  M98t
owns depth and 30-level scale coupling.
