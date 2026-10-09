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

# NEON3 PC-88VA P1 video, SGP, and VA-BIOS contract

Status: P1 research complete. This document authorizes preparation of P2 only;
it does not authorize a NEON3 guest implementation.

## 1. Scope and evidence labels

The first port target is deliberately limited to the 80286-safe source path:

```text
NEON3286
640x200 default mode
16-colour single-plane graphics
SGP drawing only
OPNA/YM2608 only
VA BIOS services, no DOS INT 21h
```

The original 386/PEGC editions remain separate future targets. The source tree
is an untracked maintainer work area containing generated COM files and an
`OPTIMIZE` duplicate; P1 does not delete, move, or publish any of those files.

| Tag | Meaning |
|---|---|
| `[VA-TM]` | PC-88VA technical-manual material available in this checkout. |
| `[VA-TEKU]` | Maintainer-local Users Club material under `docs/tekumani/`. |
| `[SRC:path:line]` | Current VAEG or guest-source implementation evidence only. |
| `[DERIVED]` | Arithmetic or layout inference from documented fields. |
| `[IMPLEMENTATION]` | VAEG behavior, not a silicon-level hardware claim. |
| `[UNKNOWN]` | No sufficient source or hardware evidence. |
| `[HARDWARE_PENDING]` | Requires real VA/VA2 measurement. |

PC-98 GRCG, PEGC, `0A0h`, `0A4h`, and `0A6h` behavior is source-platform
evidence only. It is not a PC-88VA contract.

## 2. NEON3286 source boundary

| Source | Verified role | P1 disposition |
|---|---|---|
| `NEON3286.ASM` | `cpu 286`, `org 100h`, DOS entry/exit, PC-98 video/sound/IRQ lifecycle | Retain scene ownership; replace platform shell. |
| `CITY3D286_CORE.INC` + `CITY3D286_FAITHFUL.INC` | Actual 286 include path: fixed-point projection, camera, nine scene routes, SGP-relevant line spans, rectangles, and scanline triangles | Candidate geometry/data source; no GRCG code may remain in the VA backend. The faithful path is included by `VIDEO3_286.INC` and is the path whose per-frame primitive counts must be audited. |
| `SCENE3_256.INC` | Nine-scene dispatch and 6144-frame logical timeline | Retain logical scene/timing model. |
| `VIDEO3_286.INC` | PC-98 GRCG mode setup and immediate raster primitives | Do not port; replace with a VA SGP backend. |
| `FRAME_RENDER3_286.INC` | Clear draw page, render scene, text update, page publication | Reuse ordering only after VA page/SGP contract is proven. |
| `AFS3_286.INC` | PC-98 IRQ2/INT 0Ah scheduler and page flip | Do not port; use a VA BIOS/TSP polling design first. |
| `TEXT3_286.INC` | PC-98 text VRAM and DOS console restoration | Defer overlay or replace with VA BIOS/text ownership. |
| `OPNA.INC` and `MUSIC_URBAN_D8.INC` | Shared OPNA/OPL3 score and PC-98 sound detection | Extract OPNA semantics only; exclude OPL3. |

The entry file still includes `OPL3.INC`, uses DOS `INT 21h` for messages,
keyboard, and exit, and calls the PC-98 renderer and IRQ installer
([`NEON3286.ASM`](/Users/maho/vaeg/demos/neon3_1_5/98/NEON3286.ASM:21)).
Those are confirmed port boundaries, not implementation details to copy.

## 3. P1 contract by required item

| Item | P1 result | Evidence and restriction |
|---|---|---|
| Target mode | 640x200, 16-colour, single-plane, 4bpp is the selected first target. | `[VA-TEKU:4.TXT §4.4.1, §4.4.3]`. The existing Glass VA payload already uses the VA-BIOS `$ScnMode` sequence for this exact mode ([`glass_orbit_sgp_backend.asm`](/Users/maho/vaeg/demos/glass-orbit/src/glass_orbit_sgp_backend.asm:61)). NEON should reuse that sequence rather than invent another one. |
| GVRAM | 256 KiB; CPU mapping `A0000h-DFFFFh`; SGP mapping `200000h-23FFFFh`. | `[VA-TEKU:4.TXT §4.4.1, §4.4.6]`, `[SRC:memoryva/gvramva.c:16]`. |
| Packed pixels | One 4-bit palette code per logical pixel; 640 pixels require 320 bytes per line. | `[VA-TEKU:4.TXT §4.4.3, §4.4.6]`, `[DERIVED]`. A focused VA pixel calibration is still required before any CPU pixel fallback. |
| Page capacity | One 640x200 page is 64,000 bytes (`0xFA00`); two contiguous 200-line regions fit in a 640x400 G0 source (128,000 bytes). | `[DERIVED]`. Capacity is not proof that a DSA-only flip is valid. Coupled descriptor fields must be tested later. |
| Display source | FB0 descriptors expose FSA/DSA, FBW, FBL, OFX, and OFY fields. | `[VA-TEKU:4.TXT §4.4.5]`. The 16-colour SGP wireframe payload already configures a 640x400 G0/4bpp FB0 and exchanges two 640x400 source pages ([`sgp_wireframe.asm`](/Users/maho/vaeg/demos/sgp-wireframe/sgp_wireframe.asm:44), [`:211`](/Users/maho/vaeg/demos/sgp-wireframe/sgp_wireframe.asm:211), [`:273`](/Users/maho/vaeg/demos/sgp-wireframe/sgp_wireframe.asm:283)). The 65536-colour pseudo-sprite payload independently configures the complete FB0 surface ([`sgp_sprite_65536.asm`](/Users/maho/vaeg/demos/sgp-pseudo-sprite/65536/sgp_sprite_65536.asm:183)), and Glass P5 proves the coupled VA-BIOS `RollTo` presentation path for a 200-line window ([`glass_scene.inc`](/Users/maho/vaeg/demos/glass-orbit/src/glass_scene.inc:135)). FB0 descriptor/page switching is therefore an existing contract; NEON only needs to select the matching window height. |
| VBLANK | TSP status port `0142h`, bit 6 (`VB`). | `[VA-TEKU:2.TXT]`, [`upd72022-tsp.md`](/Users/maho/vaeg/docs/modernization/upd72022-tsp.md). Exact frame rate and safe mode profile are `[HARDWARE_PENDING]`. |
| Palette | Two 16-entry palette banks at word ports `0300h-031Eh` and `0320h-033Eh`. | `[VA-TEKU:4.TXT §4.5]`. P2 must choose one documented composition path; no PC-98 palette ports may be reused. |
| SGP command table | Even-aligned command table in main RAM, 58-byte work area, SGP GVRAM space, `SET_WORK`, `SET_COLOR`, `LINE`, `CLS`, and `END`. | `[VA-TEKU:4.TXT §4.4.6]`, [`upd92017-sgp.md`](/Users/maho/vaeg/docs/modernization/upd92017-sgp.md:185). Glass emits the 640x200/4bpp `LINE`/`CLS` lists ([`glass_orbit_sgp_backend.asm`](/Users/maho/vaeg/demos/glass-orbit/src/glass_orbit_sgp_backend.asm:307)); NEON still needs a P2 command-count and geometry mapping, not a new ABI investigation. |
| SGP submission/status | Command address is written through `0500h-0503h`; `0506h` starts execution; status bit 0 is the VAEG stopped/busy indication. | `[VA-TEKU:4.TXT §4.4.6]`, `[SRC:io/sgp.c:721-724,1522-1578]`. The sprite and Glass payloads both submit and bounded-poll this interface ([`sgp_sprite_demo.asm`](/Users/maho/vaeg/demos/sgp-pseudo-sprite/sgp_sprite_demo.asm:1185)). Hardware list limits and contention remain `[UNKNOWN]`/`[HARDWARE_PENDING]`. |
| Main RAM | SGP main-memory addresses `000000h-09FFFFh` correspond to CPU main RAM. | `[VA-TEKU:4.TXT §4.4.6]`. Exact payload, stack, command-list, and work-area placement is deferred until the loader contract is selected. |
| Payload entry/exit | The existing PC-Engine validation `.COM` route provides a working NASM flat-payload entry, private stack setup, VA-BIOS calls, and return continuation. | `[SRC:demos/glass-orbit/src/glass_orbit_sgp_backend.asm:97-148]`, `[SRC:demos/sgp-pseudo-sprite/65536/sgp_sprite_65536.asm:93-123]`. This is an existing loader contract to reuse; it is not a claim about an arbitrary bare machine ABI. |
| OPNA | VA Music BIOS exposes YM2608 initialization and low/high register operations through `INT 8Bh`; the register pairs are `44h/45h` and `46h/47h`. | `[VA-TEKU:611MUSIC.TXT §6.11]`, `[VA-TEKU:5.TXT §5.11]`, `[SRC:io/boardsb2.c:201-236]`. Glass already has the bounded OPNA path and clean shutdown ([`glass_opna.inc`](/Users/maho/vaeg/demos/glass-orbit/src/glass_opna.inc:27)); NEON should adapt its score to that proven VA route and must not use the PC-98 `0088h` candidates. |

## 4. VA replacement architecture

The NEON scene and math layer may remain logically 640x400, as in the source;
the VA backend maps the physical 640x200 raster at its primitive boundary. The
following substitutions are required:

```text
CITY3D286 projection and scene data
        |
        v
shared logical primitive stream
        |
        +-- SGP CLS for clear and exact horizontal spans
        +-- SGP LINE for wire/road/building edges
        +-- documented generic convex fill for polygon faces
        +-- SGP-only treatment for point-like primitives; no CPU drawing fallback
```

The actual `VIDEO3_286.INC` include path combines `CITY3D286_CORE.INC` and
`CITY3D286_FAITHFUL.INC`. It contains projected `LINE` calls, rectangular
fills, and several quad-as-two-triangles paths whose scanlines call
`hline_set_same_colour`. Those are candidates for the generic SGP polygon/span
treatment used by the completed Glass port; they must not receive
coordinate-specific repairs. P2 must first enumerate their worst-case
per-frame spans. If a point-like source primitive has no direct SGP opcode, P2
must find an SGP-only representation or explicitly defer that scene element;
it may not silently add a CPU drawing path. The P1 report does not change that
source.

The final-stage full-screen `fill_rect` optimization is a legitimate scene
operation, but its VA implementation must be an SGP clear/span operation rather
than a GRCG rectangle write.

## 5. Timing and presentation boundary

The source frame macro clears and renders the selected access page, updates
text, and then queues a completed page. The original `AFS3_286.INC` flips at a
PC-98 IRQ2 interrupt and advances the music clock there. This ordering cannot be
copied until VA display-source selection and TSP timing are established.

P2 may evaluate two 640x200 source halves, but must preserve these invariants:

```text
SGP idle before a displayed source is switched
CPU writes to a page only while it is not displayed
FRAME_READY is set only after SGP and any required CPU writes complete
VA BIOS/loader return is independent of DOS and PC-98 IRQ vectors
```

If real VA page exchange is not established, the first implementation must use
a hardware-valid single-buffer/polling fallback rather than a host-only hidden
buffer.

## 6. Audio boundary

`OPNA.INC` uses a common score with OPNA and OPL3 state. The P1 policy is:

```text
keep the score data only where its OPNA events are identified
use VA Music BIOS YM2608 services
remove OPL3 detection, ports, instruments, and runtime options
stop and silence OPNA before VA BIOS return
```

The Music BIOS documentation states that the score queue and timer-driven
playback have their own lifecycle, including initialize, play/stop, register
access, and `Initialize2` for rhythm use. The Glass payload already calls the
VA Music BIOS OPNA path from a payload and performs bounded waits plus clean
shutdown; therefore direct use of `INT 8Bh` through the established VA payload
route is not a new safety unknown. What remains unknown is which NEON3 score
features require which lifecycle calls, including whether rhythm/SSG data
requires `Initialize2`. No PC-98 direct-port probe is authorized by this
contract.

## 7. Explicit unresolved questions

| ID | Question | Status |
|---|---|---|
| N1-U1 | NEON-specific use of the complete VA V3 enter/leave sequence. | The reusable 640x200/4bpp `$ScnMode`, buffer, palette, composition, mapping, and return sequence is established by the Glass VA payload. The remaining work is to reuse it and verify NEON's own state changes; it is not a new ABI unknown. |
| N1-U2 | NEON-specific loader integration and exact return continuation. | The existing PC-Engine validation `.COM` route and Glass/sprite payloads provide the entry, private stack, VA-BIOS call, and continuation pattern. The remaining question is only which existing wrapper the NEON payload will use; an arbitrary bare-machine ABI is not being assumed. |
| N1-U3 | NEON-specific `LINE`/`CLS` list sizing, span count, clipping cases, and command ordering. | The command encoding and submission path are established by Glass and sprite sources. P2 will instrument all nine scene routes in VAEG, count projected `LINE` calls, `fill_rect` spans, and every scanline emitted by `city286f_fill_triangle`/`hline_set_same_colour`, then print the per-frame and worst-frame totals. This is a measurement task, not a new ABI blocker. |
| N1-U4 | Maximum command-list size, CPU/SGP same-page contention, and completion latency on VA/VA2. | For the first port, emulator-side command counts and completion are sufficient. Real-machine list limits, contention, and latency are deferred to the later hardware gate. |
| N1-U5 | Whether NEON should present a 200-line window over the already-proven 640x400/4bpp FB0 pages, or use the full 400-line window. | The 640x400/16-colour FB0 descriptor and two-page exchange are already solved by `sgp-wireframe/16`; the 200-line `RollTo` window is solved by Glass P5. This is now a NEON presentation choice, not an unknown descriptor ABI. |
| N1-U6 | Independent raw-pixel calibration for all four packed-pixel positions in the chosen VA mode. | `[HARDWARE_PENDING]`; VAEG-only testing is not hardware proof. |
| N1-U7 | NEON-specific ownership and score requirements when using VA Music BIOS `INT 8Bh`. | Tekumani Music BIOS documentation and the Glass payload establish the safe VA-BIOS OPNA route. P2/P6 must determine NEON's required OPNA channels, tick/lifecycle calls, and whether any score feature needs additional BIOS services. |
| N1-U8 | Which OPNA channels, SSG, rhythm, and timer features the NEON3 score actually requires. | Source audit pending in P2/P6. |
| N1-U9 | Text overlay scope. | Resolved for the first milestone: provide the normal VA text overlay through the VA text/BIOS path; sprites are not required. |

The following are therefore not P1 blockers: the basic VA mode entry/exit
sequence, the existing `LINE`/`CLS` command-list ABI, the known payload
loader/return pattern, the 640x400/4bpp FB0 descriptor and two-page exchange
used by `sgp-wireframe/16`, the 200-line `RollTo` window used by Glass, and the
VA Music BIOS OPNA register path. They remain subject to NEON-specific
adaptation and the real-hardware gate where noted.

## 8. P2 emulator measurement plan

The first P2 build will add counters around the shared logical primitive
stream, without changing the scene geometry:

```text
LINE calls
triangle calls
triangle scanline spans
fill_rect/CLS spans
SET_COLOR commands
CLS commands
END commands
command-list words and bytes
```

Counters will be accumulated per logical frame and as a running maximum across
all nine scenes. The result will be emitted through the normal VA text overlay
and retained in the VAEG log. The measurement must include the worst frame of
the 6144-frame timeline, not only one representative screenshot. The emulator
measurement is sufficient to choose the initial command-list buffer size; real
hardware command limits and contention remain a later validation item.

## 9. P1 disposition

**GO WITH RESTRICTIONS: P2 may be prepared.** The 286 fixed-point scene code
is a viable source basis, but no PC-98 video, DOS, IRQ, GRCG, PEGC, or OPL code
may be copied into the VA payload. P3 implementation remains blocked until the
maintainer approves the P2 design and the NEON-specific scene decomposition,
command-list budget, and real-hardware timing limits are explicitly bounded.

No source, binary, disk image, or existing demo file was modified by this P1
investigation other than this report.
