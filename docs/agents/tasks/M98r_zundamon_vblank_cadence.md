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

# M98r - Add selectable VBLANK cadence

Status: **G98r human gate passed; closed on 2026-09-01**

Branch: `topic/m98r-vblank-cadence`

Starting commit: `ade337c2d1f2ec0106a04361e1dd22a9995cb9b7`

Accepted M98q implementation: `6a3f229c74d1ffed9888b279e80334ac76d2e461`

Accepted M98q report and gate head: `ade337c2d1f2ec0106a04361e1dd22a9995cb9b7`

Commit prefix: `M98r:`

Gate type: **automated VA2/VAEG evidence plus maintainer human gate**

## Goal

Add only animation scheduling to the accepted M98q renderer. Accept one
optional `/V1` through `/V8` selector, default to `/V1`, and apply cadence on
fresh VBLANK low-to-high edges. LEFT requests the next faster divisor, RIGHT
the next slower divisor, SPACE requests pause/resume, and ESC exits through
the accepted restoration path.

Divisors 1 through 8 carry nominal labels 60, 30, 20, 15, 12, 10, 8.6, and
7.5 fps. The labels are not measured promises: report the measured display
VBLANK rate, requested rate, publication rate, and missed eligible slots
separately.

## Preserved renderer contract

Keep the public one-bank, 30-scale atlas; exact `30..1..29` sequence; fixed
anchor; independent page-local dirty rectangles; two initialization full
clears and no steady-state full clear; one transparent BMS-to-G1 BITBLT per
rendered update; hidden-page-only work; bounded SGP completion; fresh VBLANK
publication; and transactional page/rectangle/scale commit.

Observe VBLANK through one authoritative edge observer, including while SGP
is busy. Publish a READY page only on a divisor-qualified edge. A missed slot
retains the complete visible page and does not advance the scale. Apply
divisor and pause/resume requests at a VBLANK boundary, reset the partial
divider there, and do not count that boundary toward the new interval.

## Automated evidence

Reproduce M98q full/dirty equality before editing. Add an independent host
scheduler model, fail-closed parser and state-machine tests, bounded generated
event traces, and VA2 cases for all eight static divisors. Cover opposite
initial pages at V1/V4/V8, the V1-to-V8-to-V1 ladder, pause/resume in reachable
render states, missed slots, and all required negative paths. Compare every
publication with the accepted M98q framebuffer golden.

Generated COM, BIN, D88, traces, captures, reports, and backup memory stay
outside Git. No private or ROM-derived material may enter tracked files.

## Human gate

Provide one pristine generated D88. In VA2, the maintainer checks the default
fastest setting, one-step RIGHT/LEFT traversal and endpoint clamping, clean
cadence changes, SPACE pause/resume without a shortened first interval or
scale jump, unchanged transparency/anchor/page quality, and ESC restoration.
Automation cannot close G98r.

The first interactive candidate returned immediately after command entry. The
replacement candidate ignores a queued command Return and recognizes only the
complete ESC key result. The maintainer accepted that corrected candidate and
stated `G98r passed` on 2026-09-01.

## Non-goals

M98r does not add ellipse motion, x/y movement, depth coupling, a private
image, multiple instances, UP/DOWN controls, sound, gameplay, an SGP timing
change, or physical-hardware performance evidence. M98s and M98t remain
separate later milestones.
