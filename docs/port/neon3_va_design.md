<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# NEON3 PC-88VA P2 design

Status: P2 design prepared. This document does not claim real-PC-88VA timing
or command-list-limit conformance and does not authorize the final scene port.

## 1. Scope

The first implementation target is `NEON3286` with the following two VA video
profiles:

```text
profile 200: 640x200 display window, 16-colour 4bpp G0, two 200-line pages
profile 400: 640x400 display window, 16-colour 4bpp G0, two 400-line pages
```

Both profiles use the same logical scene coordinates (640x400), the same SGP
command vocabulary, the same VA BIOS entry/exit path, and the same OPNA-only
audio policy. Sprites are not part of the text or graphics design.

The original PC-98 GRCG, DOS, IRQ2, direct OPNA-port, and OPL code is not
carried into the VA payload.

## 2. Existing contracts reused by P2

The following are established by existing payloads rather than invented here:

| Contract | Existing evidence |
|---|---|
| VA mode entry and 640x200/4bpp window | [`glass_orbit_sgp_backend.asm`](/Users/maho/vaeg/demos/glass-orbit/src/glass_orbit_sgp_backend.asm:97) |
| 640x400/4bpp FB0 descriptor and two-page DSA exchange | [`sgp_wireframe.asm`](/Users/maho/vaeg/demos/sgp-wireframe/sgp_wireframe.asm:44), [`:211`](/Users/maho/vaeg/demos/sgp-wireframe/sgp_wireframe.asm:273) |
| Coupled 200-line FB0 presentation window | [`glass_scene.inc`](/Users/maho/vaeg/demos/glass-orbit/src/glass_scene.inc:135) |
| SGP `SET_WORK`, `SET_COLOR`, `LINE`, `CLS`, `END` list construction | [`glass_orbit_sgp_backend.asm`](/Users/maho/vaeg/demos/glass-orbit/src/glass_orbit_sgp_backend.asm:307) |
| Bounded SGP completion polling | [`sgp_sprite_demo.asm`](/Users/maho/vaeg/demos/sgp-pseudo-sprite/sgp_sprite_demo.asm:1185) |
| VA Music BIOS OPNA path | [`glass_opna.inc`](/Users/maho/vaeg/demos/glass-orbit/src/glass_opna.inc:27), [`611MUSIC.TXT`](/Users/maho/vaeg/docs/tekumani/611MUSIC.TXT) |

These sources establish the interfaces. They do not establish the maximum
NEON command-list workload or real-hardware contention.

## 3. Dual video profiles

The mode-dependent values are kept in one profile record. The geometry layer
does not contain mode-specific addresses or word counts.

| Value | 640x200 profile | 640x400 profile |
|---|---:|---:|
| logical scene size | 640x400 | 640x400 |
| display window | 640x200 | 640x400 |
| physical Y mapping | `logical_y >> 1` | `logical_y` |
| page bytes | `0xFA00` | `0x1F400` |
| page words | `0x7D00` | `0xFA00` |
| page A SGP base | `0x200000` | `0x200000` |
| page B SGP base | `0x20FA00` | `0x21F400` |
| page A DSA | `0x000000` | `0x000000` |
| page B DSA | `0x00FA00` | `0x01F400` |

The 200-line profile uses a 640x400 backing region. The 400-line profile uses
a 640x800 backing region, which occupies the available 256 KiB single-plane
GVRAM when two pages are present. The profile is selected before command-list
construction; a frame never mixes profile values.

The display-window operation and the drawing-page operation remain separate:

```text
select hidden page/profile
clear and render hidden page
wait for SGP completion
complete CPU-side state, if any
present the completed page through the profile's VA path
```

## 4. Geometry-to-SGP architecture

The actual 286 include path is `VIDEO3_286.INC`, which includes
`CITY3D286_CORE.INC` and `CITY3D286_FAITHFUL.INC`
([`VIDEO3_286.INC`](/Users/maho/vaeg/demos/neon3_1_5/98/VIDEO3_286.INC:1186)).
The source has three relevant logical primitive families:

```text
projected line -> one SGP LINE command
horizontal span -> one exact SGP CLS span
rectangle       -> a general SGP CLS rectangle/span sequence
```

`city286f_fill_triangle` is retained as geometry code but its
`hline_set_same_colour` backend is replaced by the shared exact logical-span
emitter. It must not write GVRAM directly and must not use a separate CPU
rasterizer. The relevant source triangle entry and span call are
[`CITY3D286_FAITHFUL.INC`](/Users/maho/vaeg/demos/neon3_1_5/98/CITY3D286_FAITHFUL.INC:1117)
and [`CITY3D286_FAITHFUL.INC`](/Users/maho/vaeg/demos/neon3_1_5/98/CITY3D286_FAITHFUL.INC:1229).

The P2/P3 flow is therefore:

```text
NEON geometry and fixed-point projection
        |
        v
shared logical primitive stream
        |
        +-- emit LINE
        +-- emit exact CLS span
        +-- emit exact CLS rectangle rows
        +-- update command counters
        |
        v
SGP command list
```

The source's quad-as-two-triangle operations remain two geometry calls in the
first adapter. They do not imply two approximate VRAM passes: both triangles
produce exact logical spans through the same span emitter. A later P2 result
may justify a general convex-polygon adapter, but no geometry-specific repair
is permitted.

## 5. Command accounting

The primary P2 question is workload, not SGP ABI. The first VA adapter adds
instrumentation at the shared primitive/emitter boundary.

Counters are kept per logical frame and as a running maximum over all nine
scenes and 6144 logical frames:

```text
scene_index
scene_frame
video_profile (200 or 400)
LINE calls
triangle geometry calls
triangle scanline spans
fill_rect rows/spans
SET_COLOR commands
CLS commands
END commands
command-list words
command-list bytes
```

The counters are incremented when commands are actually emitted, not inferred
from source-loop counts. This includes clipping, empty-span rejection, and
colour-state changes. A command is counted once even when it is later submitted
as part of an asynchronous list.

The maximum record contains:

```text
profile, scene, logical frame, each counter, and the list byte count
```

The emulator run will show the current frame counters in the normal VA text
overlay and will retain the complete maximum record in the VAEG log. The text
overlay uses the ordinary VA text/BIOS path; no sprite is involved and no DOS
`INT 21h` service is used by the NEON payload.

The initial command-list buffer is selected from the measured maximum plus a
small fixed structural margin for `SET_WORK`, state changes, and `END`. The
margin is recorded, not hidden in a hard-coded scene-specific exception.

## 6. Source workload that must be measured

The nine logical scene lengths are 640, 640, 512, 896, 640, 640, 896, 384,
and 896 frames, totaling 6144 ([`SCENE3_256.INC`](/Users/maho/vaeg/demos/neon3_1_5/98/SCENE3_256.INC:136)).
The high-workload candidates are:

| Source path | Work to count |
|---|---|
| Road, bridge, park, rail, tunnel, terminal, and city loops | Projected `LINE` calls after clipping/rejection |
| `city286f_fill_triangle` | One span count for each accepted logical scanline |
| Rotozoom/focused tile path | Triangle spans until the full-cover transition |
| `fill_rect` whiteout/aperture paths | Actual emitted CLS rows, not only one source call |
| Palette/text updates | No SGP geometry commands; counted separately as presentation work |

The source-loop upper bound is not accepted as the final result because
projection can reject primitives and clipping changes the emitted geometry.
The VAEG run is the authoritative P2 workload measurement.

## 7. Audio and text boundaries

Audio uses the existing VA Music BIOS OPNA/YM2608 path. P2 does not port the
PC-98 `OPNA.INC` direct-port layer or the OPL branch. NEON score channel and
rhythm requirements remain a later source-adaptation item.

Text is part of the first visual milestone through the ordinary VA text/BIOS
path. It is not represented as SGP geometry and does not require sprites.

## 8. P2 acceptance and next gate

P2 is complete when the dual-profile design is documented, the shared
primitive/counter interface is specified, and the measurement run format is
fixed. It does not require a real machine or a real-hardware command-list limit.

The next milestone implements the smallest VA payload and the counters, then
runs VAEG through all 6144 frames in both profiles. The next human review must
inspect the maximum records before the full NEON scene adapter is accepted.

Disposition: **GO WITH RESTRICTIONS** for the counter-instrumented VA adapter;
real-hardware timing and command-list conformance remain explicitly deferred.
