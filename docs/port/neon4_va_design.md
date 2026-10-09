<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# NEON RELAY 4 PC-88VA P2 design

Status: design complete. Decision: **GO WITH RESTRICTIONS**. This document is a
design and estimation artifact; it does not add the P3 implementation.

The implementation now also exposes the requested `NEON4_P5_PROFILE=16`
variant: 640x400 packed 4bpp G0 with sixteen 12-bit palette entries selected
from the VA's 4096-colour space. The direct RGB332 rules below remain the
contract for the original 256-colour (`PROFILE=256`) path; the 16-colour
palette conversion and distribution layout are documented in
`demos/neon4/README.md`.

The P1 contract fixes the target to the 286 scene path, direct packed RGB332,
SGP drawing, VA BIOS services, OPNA-only audio, and 320x200 first. The same
source tree will later build 640x200 by changing the physical-width parameter.

## 1. Design invariants

```text
logical scene: 640x400, unchanged from NEON4 source
physical 320x200: x >>= 1, y >>= 1 at VA primitive entry
physical 640x200: x unchanged, y >>= 1 at VA primitive entry
pixel storage: one RGB332 byte per logical physical pixel
visible page: never written while it is displayed
SGP timing: hardware_pending; VAEG output is functional evidence only
```

The scene dispatcher remains the eight live routines in
`demos/neon4/src/scene4_256.inc:10-65`. Each chapter is 384 logical frames;
`MUSIC_STEPS=512`, `MUSIC_DIVIDER=6`, and `TOTAL_FRAMES=3072` are compile-time
checked in `config4_256.inc:20-38`.

## 2. RGB332 colour design

### 2.1 No palette animation

The VA 8bpp path is direct colour. A pixel byte is interpreted as:

```text
bits 7..5: green (3 bits)
bits 4..2: red   (3 bits)
bits 1..0: blue  (2 bits)
```

This is the `rgb8to16` mapping in `vram/scrndrawva.c:21-39`; the direct-screen
composition branch at `vram/scrndrawva.c:390-420` consumes the byte directly
when `pixelmode == 2`. `vram/makegrphva.c:398-485` reads those bytes in
increasing GVRAM address order. Therefore the VA backend will not write a
palette register per frame and will not carry the PEGC palette animation into
P2/P3.

### 2.2 Source ramp conversion

The source 256-colour path selects 32-step hue ramps by adding a ramp base to
an intensity level. `GEOM4_256.INC:612-626` derives an intensity in the range
8..31 from projected face area, and `CONFIG4_256.INC:268-310` contains authored
landmark, lamp, sign, tunnel, and water values. `DATA4.INC:1123-1148` contains
the burst/finale and CAT source values. The source RGB triplets are in
`VIDEO4_DATA.INC:56-124` (`city_scene_palette_grb` and `pegc_palette_grb`).

The VA mapping rule is:

```text
source physical index -> source GRB triplet -> quantised RGB332 byte
```

The source table provides eight 32-entry hue ramps (`pegc_scene_colour_bases`
uses bases `0,32,64,96,128,160,192,224`). The source physical values are PEGC
indices, not VA palette writes. The VA
mapping resolves `pegc_palette_grb[index*3 + {0,1,2}]` and quantises each
component independently:

```text
qg = round(g8 * 7 / 255)   ; 3 bits
qr = round(r8 * 7 / 255)   ; 3 bits
qb = round(b8 * 3 / 255)   ; 2 bits
rgb332 = (qg << 5) | (qr << 2) | qb
```

Generated ramp values first clamp the level to 0..31 and select the matching
source triplet from the 32-step hue ramp. The result is a direct pixel byte;
the VA backend does not upload or mutate a DAC palette. The generated mapping
must be a pure function of `(source index, scene state)` so CPU and SGP paths
receive identical colour bytes. This is the required RGB332 direct-colour
conversion, not a palette-animation fallback.

The 16-colour helper names (`low_phys_to_16`, `low_use_phys_color`, and
`low_use_color`) are not reused as a hidden palette layer. P3 will replace
them with one `va_rgb332_from_source` entry point. A direct RGB332 shade may
look different from the PEGC-indexed original; that is an allowed design
difference, not a palette bug.

### 2.3 Colour update policy

```text
initial frame: choose the RGB332 clear byte and scene colours
each primitive: emit SET_COLOR only when the SGP colour word changes
frame boundary: no palette commit; only the completed page is published
fade/shutter: emit direct RGB332 black spans, not palette black
```

The source `vsync_prepare_city_palette` concept is therefore represented as a
CPU colour-state calculation. It must not write VA palette ports. Text colour
and graphics colour remain separate state domains.

## 3. Frame skip and audio-time design

### 3.1 Logical time

The visual timeline is exactly 3072 logical frames:

```text
scene 0: frames   0.. 383
scene 1: frames 384.. 767
scene 2: frames 768..1151
scene 3: frames 1152..1535
scene 4: frames 1536..1919
scene 5: frames 1920..2303
scene 6: frames 2304..2687
scene 7: frames 2688..3071
```

`select_scene` consumes the absolute counter and calculates `scene_frame`; it
does not require replaying skipped camera states because
`city_camera_catch_up` is a no-op in `scene4_256.inc:44-49`.

### 3.2 Polling scheduler

The first VA implementation uses TSP polling, not the source PC-98 IRQ2 path:

```text
wait for VB=0 -> VB=1 transition on port 0142h bit 6
count elapsed VB transitions since the previous service
advance logical_frame by elapsed transitions, wrapping to 0 after 3072
advance the OPNA logical tick accumulator by the same elapsed count
render at most one back-page state per service pass
if rendering missed a VB, skip drawing but do not roll back logical time
publish only a completed back page at a later VB edge
```

`FRAME_SKIP_MAX=10` from the source is retained as a scheduler safety cap. If
more than ten fields are missed, the scheduler records a bounded-lag event and
continues advancing logical time; it does not duplicate visual frames.

### 3.3 Music relationship

The score step is driven by logical frame time, not by the number of rendered
pages. With divider six, the accumulator emits one score event every six
logical frames. A skipped visual frame still advances the score state, so
render load cannot slow the song or create an accumulating A/V drift.

The P2 scheduler does not claim an audio interrupt or timer implementation.
OPNA lifecycle and the exact `sound_tick` service remain P8 work. The
invariant is only that audio time is derived from the same logical counter that
selects the scene.

### 3.4 Completion and termination

When logical frame reaches 3072, the published NEON4 profiles reset the
logical counter to zero and continue from scene 0.  ESC remains the explicit
normal exit path; it stops drawing and returns through the existing payload
continuation.  The final scene is therefore exactly 384 frames per pass, with
the complete 3072-frame sequence repeating until exit.

## 4. SGP command model

### 4.1 Command record sizes

The estimates use the existing word-list encoding demonstrated by
`demos/sgp-pseudo-sprite/sgp_sprite_demo.asm:940-1070` and the NEON3 LINE
emitter. One command word is two bytes.

| Command | Words | Bytes | P2 use |
|---|---:|---:|---|
| `SET_WORK` | 3 | 6 | Stable 58-byte work area; emitted at list start/replay. |
| `SET_COLOR` | 2 | 4 | Clear, face, ribbon, line, and shutter colours. |
| `CLS` | 5 | 10 | Full-page clear or one exact horizontal span. |
| `LINE` | 8 | 16 | Destination descriptor plus direction/ROP. |
| `SET_SOURCE` | 7 | 14 | Main-RAM CAT/raster source descriptor. |
| `SET_DESTINATION` | 7 | 14 | GVRAM destination descriptor. |
| `BITBLT` | 2 | 4 | One 96x40 or 48x48 transfer. |
| `END` | 1 | 2 | Terminates a submitted batch. |

These are list-encoding sizes, not hardware execution times. The 8bpp
descriptor field semantics still require the minimal P3/P6 calibration.

### 4.2 Exact span policy

All face/ribbon/checker spans use one logical span per covered physical row.
The existing `N4_286_200_SCANLINE_THIN` policy in `config4_286.inc:21` is kept:
when two logical rows map to the same physical row, only the final emitted span
for that physical row is submitted. The VA backend then partitions the span
into exact packed-byte writes/CLS ranges; it never rounds a polygon boundary
outward.

For 8bpp, two pixels occupy one 16-bit word. A 320-dot row has 160 words and a
640-dot row has 320 words. A span that starts or ends on an odd pixel requires
one endpoint operation; the interior word range is submitted to SGP only when
both pixels are covered.

### 4.3 BITBLT policy

`n4_story_raster_panel`, `n4_raster_blit96x40`, and `n4_blit_tile48` are emitted
as SGP BITBLT operations in the VA path. Source assets live in main RAM so the
640x200 two-page GVRAM allocation is not consumed by a hidden asset tail.
Each transfer is conservatively estimated as one complete
`SET_SOURCE` + `SET_DESTINATION` + `BITBLT` group. Descriptor reuse may reduce
the final list, but no such reduction is assumed in the P2 upper bound.

The EGC/PEGC cache-build code in the source is not part of the VA command path.
It remains useful as geometry/data provenance only.

## 5. Static scene command estimates

The following are conservative **upper bounds**, not measured high-water
marks. They assume every potentially visible face reaches 200 physical rows.
Actual back-face culling and clipping should lower the counts. `CLS` includes
one full-page clear. `SET_COLOR` is counted once per colour run, not once per
scanline. `LINE` counts only the explicit cage edges; CAT/raster outlines are
BITBLT source content in the VA design.

| Scene | Fill decomposition used for estimate | CLS | LINE | BITBLT | SET_COLOR | List bytes | 16 KiB batches |
|---|---|---:|---:|---:|---:|---:|---:|
| 0 SIGNAL SEED | tetra: 4 triangles × 200 rows | 801 | 0 | 1 | 5 | 8,070 | 1 |
| 1 FACET ASSEMBLY | worst shape icosa: 20 triangles × 200; 2 cage passes × 12 edges | 4,001 | 24 | 1 | 23 | 40,526 | 3 |
| 2 MATERIAL ASSEMBLY | cube 6 quads + octa 8 triangles + pyramid 6 triangles, × 200 | 4,001 | 0 | 2 | 21 | 40,166 | 3 |
| 3 MORPH GATE | cube 6 convex quads × 200 | 1,201 | 0 | 1 | 7 | 12,078 | 1 |
| 4 RASTER TRANSFER | 3 raster bands + 4 CAT cards | 1 | 0 | 7 | 1 | 246 | 1 |
| 5 SURFACE WAVE | 20 ribbon quads × 25 physical rows + 3×16 markers | 549 | 0 | 0 | 24 | 5,594 | 1 |
| 6 GRID ARRIVAL | checker ≤368 spans + worst icosa 4,000 spans | 4,369 | 0 | 1 | 27 | 43,838 | 3 |
| 7 SOLID FINALE | worst pre-fade 4+6+20 faces × 200 rows | 6,001 | 0 | 2 | 32 | 60,210 | 4 |

The byte calculation is:

```text
bytes = 6*SET_WORK + 4*SET_COLOR + 10*CLS
      + 16*LINE + 32*BITBLT + 2*END
```

The reported totals include one `SET_WORK` and one `END`. Each additional
16-KiB batch adds an `END`; the next batch replays `SET_WORK` and the current
colour state, so the final submitted total is at most 12 bytes above the
table's simple total per extra boundary. No measured list high-water mark is
available at P2.

The scene-six checker estimate comes from authored Y differences
`12,17,23,34,42,46` in `data4_p0.inc:906-907`, physical-row rounding, eight
columns with alternating-tile skips, and a six-row limit. The scene-five
25-row ribbon bound follows the 28-pixel thickness and the documented maximum
21-pixel adjacent sample difference in `geom4_low.inc:2980-2990`.

### 5.1 320x200 versus 640x200

For the same logical frame and the same one-span-per-row policy, the number of
command records and therefore the list bytes are unchanged. The payload of
those records is not unchanged:

| Quantity | 320x200 | 640x200 |
|---|---:|---:|
| bytes/pixel | 1 | 1 |
| words/pixel pair | 1 word / 2 pixels | 1 word / 2 pixels |
| words per row | 160 | 320 |
| page bytes | 64,000 | 128,000 |
| two-page bytes | 128,000 | 256,000 |
| CLS word count for the same physical span width | baseline | approximately 2× |
| BITBLT pixel/byte transfer | baseline | approximately 2× horizontally |
| command-list record bytes | unchanged | unchanged unless software splits spans |

This is why P2 does not multiply the list-byte column by two. The 640x200
variant is expected to take roughly twice the GVRAM transfer work, but VAEG
cannot supply an SGP timing claim and the real machine decides whether the
selected frame-skip rate is sufficient.

## 6. Batch and memory design

The command-list buffer is a **16,384-byte per-submission software buffer**.
This is a convenient bounded allocation, not a documented SGP maximum and not
an inheritance from GLASS or NEON3's experimental 20-KiB value. A frame whose
stream exceeds the cap is split only at complete span or object boundaries:

```text
build state + complete commands until next command would exceed 16 KiB
append END and submit
wait SGP idle
reset cursor; replay SET_WORK and colour/descriptors needed by next batch
continue until the frame stream is complete
```

The largest static estimate (scene 7, about 60 KiB) requires four submissions;
scene 1, 2, and 6 require three. A single command record is at most 32 bytes
under this design, so no command can exceed the cap. Work area and image
sources are separate from the list. The exact main-RAM placement and stack
guard are P3 deliverables.

The 16-KiB choice is a software memory-budget decision. Hardware list length,
maximum command count, and command fetch wrap are `[HARDWARE_PENDING]`. If a
real machine rejects a 16-KiB batch, P2's split protocol permits a smaller cap
without changing scene geometry. If a single 16-KiB list is accepted, the
frame may still use several submissions; no timing conclusion follows.

## 7. Frame pipeline

The intended asynchronous pipeline is:

```text
1. poll VB and advance logical time
2. choose the non-displayed page
3. build one or more SGP lists for clear, spans, lines, and BITBLTs
4. submit a list (no CPU writes to its SGP destination while busy)
5. compute the next logical geometry while SGP is executing when safe
6. poll SGP idle; perform exact endpoint CPU writes if required
7. mark FRAME_READY only after all page work and OPNA logical tick work finish
8. wait VB and switch the validated FB0/FB2 display source
```

If P1/P3 proves CPU access to the same page is safe while SGP is busy, step 5
and endpoint work may overlap. Until then, the conservative rule is disjoint
page ownership. SGP completion is never used as a VBLANK signal.

## 8. P2 restrictions and GO decision

**GO WITH RESTRICTIONS**

1. Implement 320x200 first. Use the same source and remove only the X halving
   for the 640x200 variant.
2. Keep RGB332 direct bytes; do not write a per-frame VA palette.
3. Use `CLS` for full clears and exact spans, `LINE` for wire edges, and
   `BITBLT` for the one source-image transfer family. Do not port GRCG/EGC.
4. Use the 16-KiB software batch cap and split at command boundaries. It is
   not a hardware limit.
5. Treat all SGP speed, list-limit, and same-page contention statements as
   `hardware_pending`; emulator runs validate function, not milliseconds.
6. Keep the 3072-frame logical timeline and advance OPNA time with logical
   frames, including skipped visual frames.
7. Do not implement text, OPNA, or BITBLT hardware details in this P2 task;
   they remain separate P3/P6/P8 milestones.

P3 may now implement the 320x200 VA skeleton and command-count instrumentation
using these rules. P3 must first prove the CPU clear and SGP clear images agree
before connecting the eight scenes.
