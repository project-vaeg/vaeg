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

# M98t - Couple orbit depth to 30 scales and add the G0 HUD

Status: **G98t human gate passed; M98t closed on 2026-09-01**

Branch: `topic/m98t-depth-scale-hud`

Starting and accepted M98s commit:
`cf542bff4265272f2fd563b10d159b8e65c74966`

Commit prefix: `M98t:`

Gate type: **automated VA2/VAEG evidence plus maintainer human gate**

## Goal

Keep the accepted clockwise 64-phase ellipse and derive a signed depth rank
and stored scale ID 1 through 30 from each phase.  Use the selected
descriptor's own dimensions, pitch, anchor, payload, and BMS source.  Commit
phase, depth, scale, and the page-local logical rectangle only after a READY
hidden page is published on an eligible VBLANK edge.

Add one fixed G0 information panel at `[4,4,70,20)` using a task-authored
public 5x7 font.  It displays the active nominal cadence as `FPS: <field>` and
the immutable text `ZUNDAMON: 1`.  G1 remains a homogeneous one-object layer.

## Preserved renderer and scheduler contract

Preserve the one-bank public atlas, page-A/page-B independent old rectangles,
word-rounded dirty-row CLS, two initialization full clears, zero release
steady-state full clears, one transparent BMS-to-hidden-G1 BITBLT per update,
READY-only publication, and ordinary-mapping/video restoration.

Preserve `/V1` through `/V8`, LEFT/RIGHT, SPACE, ESC, debounce, boundary
reset, pause, and missed-slot rules from M98r/M98s.  Applied divisor changes
replace exactly the 18x8 FPS field on a VBLANK boundary.  UP/DOWN remain
inactive.  No miss, pause, queued change, or failed transaction advances the
phase or scale.

## Automated evidence

Reproduce M98s before editing.  Generate and independently validate the
phase/depth/scale and HUD includes twice, build twice, and compare identities.
Compare `A/full`, `A/dirty`, `B/full`, and `B/dirty` byte-for-byte for two
revolutions.  Run all eight static divisors, opposite-page long V1/V4/V8
cases, the dynamic HUD ladder, pause/resume, consecutive missed slots, and
fail-closed negative tests.  Every publication must match an independent
indexed G0/G1/composite oracle.

Generated COM, BIN, D88, traces, captures, reports, and backup memory remain
outside Git.  No private or ROM-derived material may enter tracked files.

## Human gate

Provide one pristine generated D88.  In VA2, the maintainer checks one
camera-facing marker moving clockwise, largest at the bottom and smallest at
the top, stable descriptor anchors, all cadence fields without stale decimal
pixels, fixed `ZUNDAMON: 1`, inactive UP/DOWN, pause/resume, two revolutions,
and normal ESC restoration.  Automation cannot close G98t.

The maintainer explicitly stated `G98t passed` on 2026-09-01 after the
interactive VA2 check. M98t is closed at this gate.

## Non-goals

M98t does not add private imagery, multiple instances, UP/DOWN count controls,
depth sorting, image rotation, runtime scaling, a measured-FPS HUD, gameplay,
audio, emulator timing changes, or physical-hardware performance evidence.
