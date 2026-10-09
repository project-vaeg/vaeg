<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# PC-88VA SGP Pseudo-Sprite Demo

## Final implementation

This directory contains the final M7 implementation: multiple animated pseudo-sprites
rendered into a hidden Graphic 1 page, followed by a VBLANK-synchronized
DSA1 page exchange, with a configurable bullet stress prefix and exit-time
SGP counters. The reviewed M7 rendering variants share this source; the build
helpers emit only these final variants. On the evaluated VAEG setup, the 24x24
scene measured about 57 FPS with 26 active spheres and about 28 FPS with 27;
that remains a workload measurement, not a hardware maximum. The final runtime
prefix limit is 256 records and reserves 32 of those records for 8x8 bullets.
The 16-bit real-mode DOS `.COM` program keeps the verified 320x200, 16-color,
4-bpp, single-plane, two-screen configuration. Its final scene contains:

- a static CPU word-filled checkerboard in Graphic 0;
- two 32,000-byte Graphic 1 pages: one displayed and one rendered off-screen;
- 1-256 independently moving records, initially 16 24x24 pre-rendered shaded
  balls followed by 32 8x8 bullet records, transferred from main RAM by SGP
  BITBLT commands; and
- an eleven-glyph `FPSnnn Cnnnn` counter at the upper right, also transferred
  from main RAM by SGP BITBLT commands.

The 16 pre-rendered bitmap variants use 16 different HSV color phases; all
sprite records reuse those variants in painter order. A shared 4-bpp palette
cannot contain transparent index 0, two
checkerboard shades, dedicated shadow and highlight colors, and 16 additional
opaque hues at once. The demo therefore stores 12 saturated HSV wheel anchors
at 30-degree intervals and constructs four intermediate phases by dithering
adjacent anchors inside the RAM bitmaps. Palette indices 13-15 provide the
neutral shadow, light checkerboard, and white highlight. This gives 16
visually distinct, colorful ball bitmaps while preserving the single shared
16-color palette and color-0 transparency.

Every ball uses a pre-rendered 24x24 raster sphere with a white upper-left
highlight, a smooth body gradient, a dark lower-right edge, and a transparent
silhouette. Color index 0 surrounds the sphere. BITBLT mode `0105h` skips
those source-zero pixels, and G1 composition also treats color 0 as transparent,
so the Graphic 0 checkerboard remains visible through and around every ball.

Each 18-byte sprite record stores `x`, `y`, `vx`, `vy`, `width`,
`height`, source pitch, bitmap offset, and painter priority. The CPU updates
coordinates, bounces sprites at the display edges, and emits descriptors into
a RAM command buffer. It never clears G1 or copies, masks, or shades sprite
pixels. The record array is already in priority order: later records are drawn
later and their nonzero pixels appear above earlier records. The first two
balls move toward each other on the same path to provide a repeatable overlap
test. PC-88VA cursor Up or `+` increases the active prefix of the array up to
256 records; cursor Down or `-` decreases it to one. The first 16 records
are shaded balls, the next 32 are small bullets, and later records are extra
shaded balls for the stress run. Key sensing and retrieval
use the documented keyboard BIOS primitives, so no PC-compatible extended-key
sequence is assumed.

The final program uses these hardware interfaces, all traced in the
[M1 investigation](../../../docs/modernization/sgp-pseudo-sprite-investigation.md):

| Purpose | Interface used by the final program |
|---|---|
| Mode | Graphics BIOS `INT 8fh`, function 0, `BX=e00eh`, `CX=0404h` |
| Buffer geometry check | Graphics BIOS function 7 for screens 0 and 1 |
| Layer order | Graphics BIOS function 3, `CX=0034h` (G1 above G0) |
| CPU G0 aperture | `a000:0000`; the CPU does not write G1 after mode setup |
| GVRAM mapping | Port `0153h`: single-plane mode and system-memory bank 4 |
| CPU write mode | Port `0580h`: CPU data write for the static G0 background and before each SGP kick |
| Composition transparency | Ports `0124h`/`0126h`: G0 opaque, G1 color 0 transparent |
| SGP command address | Word ports `0500h` (low) and `0502h` (high) |
| SGP control/status | Ports `0504h` and `0506h` |
| Frame pacing and flip | TSP status port `0142h`, bit 6, then FB1 DSA word ports `022eh` (low) and `0230h` (high) |
| SGP destination | draw page A `220000h` or page B `227d00h`, pitch 160 bytes |
| Ball-count input | Keyboard BIOS `INT 82h`, functions `0ah` and `09h`; Up scan `3ah`, Down scan `3dh` |
| FPS time base | Calendar BIOS `INT 8ch`, function `02h`; display pixels are still rendered by SGP |

`0034h` uses graphics BIOS layer identifiers, not the raw
`COLCOMP=00abh` register value described in the M1 report. The BIOS owns the
complete mode and composition transition.

## SGP command list and counters

The 8,566-byte maximum command buffer, 58-byte work area, 256 sprite
records, 16 pre-rendered 24x24 HSV bitmaps, one 8x8 bullet bitmap, and 11
FPS/count glyph bitmaps
reside inside the DOS-loaded COM image in main RAM. DOS chooses the load
segment, so every frame the program converts relevant `DS:offset` values to
physical addresses.

~~~text
SET_WORK
    writable 58-byte work-area physical address
SET_COLOR
    0000h
CLS
    hidden G1 draw page at 220000h or 227d00h, 00003e80h words
repeat for the active 1-256 sprite records in painter order:
    SET_SOURCE
        4 bpp, start dot 0, 24x24, pitch 12
        selected HSV shaded-ball main-RAM physical address
    SET_DESTINATION
        4 bpp, start dot (x AND 3), 24x24, pitch 160
        hidden-page base + y * 160 + ((x AND fffch) / 2)
    BITBLT
        mode 0105h: forward source copy, source color 0 skipped
repeat for the eleven FPS/count glyphs:
    SET_SOURCE
        4 bpp, start dot 0, 4x7, pitch 2
        glyph main-RAM physical address
    SET_DESTINATION
        G1 upper-right glyph position, pitch 160
    BITBLT
        mode 0105h: forward source copy, source color 0 skipped
END
~~~

At the initial 16-ball setting this is 443 command words and 85 SGP opcodes,
including 27 BITBLTs. At 48 active records (the 16 balls plus all 32 bullets)
it is 955 command words, 59 BITBLTs, 11,572 source pixels, and 5,786 source
bytes per frame. At the 256-record limit it is 4,283 command words, 267
BITBLTs, 131,380 source pixels, and 65,690 source bytes per frame. The CPU
starts the list and polls busy
bit 0 until END
clears it. The loop back edges in the COM use a short conditional exit plus
an unconditional near jump; this is required by the uPD9002 instruction model,
where `0fh` is not the 8086 near-conditional-branch prefix. The complete list
is rendered into the page opposite the current
display page. Only after SGP completion does `flip_draw_page` wait for the
non-VBLANK-to-VBLANK transition and write the two DSA1 words. It then
toggles the page variables, so the next CLS and all BITBLTs target the new
hidden page. Startup renders both pages before enabling display.

## Runtime counters

The final program records six diagnostic groups at runtime: completed frames, page flips, the
last frame's command-word/BITBLT/pixel/source-byte counts, 32-bit totals for
those transfer counts, active sprite count, and VBLANK waits that exhausted
the bounded polling window. The summary is printed after video restoration
when ESC exits (or when a bounded synchronization failure aborts the loop).
The totals wrap at 32 bits; the per-frame values do not.

## Final 16-color variants

The final 16-color build emits separate DOS 8.3 executables for the reviewed
rendering variants. They share the same final source and are not milestone
bring-up images.

| File | Variant | Rendering change |
|---|---|---|
| `SGPD_7A.COM` | M7a | Samples the calendar BIOS once per 60 completed frames and moves fixed transfer arithmetic out of the sprite loop; rendering remains full-screen CLS and synchronous. |
| `SGPD_7B.COM` | M7b | Keeps the original painter-order redraw, but clears the selected page's previous rectangles with verified SGP PATBLT zero fills. It falls back to full CLS when candidate dirty area reaches the 320x200 surface. |
| `SGPD_7C.COM` | M7c | Uses two command/work buffers. While SGP renders the current hidden page, the CPU updates state and builds the next list for the page currently displayed; the list is started only after the VBLANK flip makes that page hidden. |
| `SGPD_7D.COM` | M7d | Hoists immutable physical-address conversion and fixed sprite command fields to startup templates. Each frame patches destination start-dot and destination address; the useful Y offset table is retained and no X lookup table is added. |
| `SGPD_7S.COM` | M7s | Restores the three-band Graphic 0 scrolling background. Internal phases advance by 3, 7, and 11 byte units and are rounded to the nearest eight-byte (16-dot) checker boundary before word writes, keeping the tile edges aligned; sprite rendering remains the M7a synchronous path. |

M7b, M7c, and M7d still redraw every active sprite in the original painter
order. Dirty-region intersection redraw (M7e) and a triple-buffer experiment
(M7f) are intentionally not enabled in this branch. All variants use the
verified 320x200, 16-color, single-plane 4bpp mode and the existing Graphic 0
background / Graphic 1 page exchange.

The parallel 8-bpp demos live under `256/`. `SGP256S.COM` keeps the static
checkerboard, while `SGP256T.COM` applies the same three-band 3/7/11-phase
scrolling background as `SGPD_7S.COM`; both retain the 320x200 logical window
and two-page Graphic 1 sprite surface.

Build all M7 variants with:

~~~sh
NASM=nasm demos/sgp-pseudo-sprite/build.sh /tmp/sgpd-final
~~~

The logical-work matrix in [`docs/sgp-m7-results.md`](../../../docs/sgp-m7-results.md)
separates exact command/transfer quantities from VAEG timing-model-dependent
observations. VAEG FPS, VBLANK thresholds, and modeled cycles are not claims
about PC-88VA hardware performance.

## Distribution D88

`build-d88.sh` creates a raw non-bootable 2HD data disk and a compressed
`.d88.xz` companion. The image contains no PC-Engine system files. Programs
are grouped by color depth, matching the wireframe demo layout:

~~~text
16/SGPD_7A.COM  ... 16/SGPD_7D.COM
16/SGPD_7S.COM
256/SGP256S.COM
256/SGP256T.COM
65536/SGP655S.COM
~~~

The source D88 passed to the generator is used only as a local 2HD/FAT12
geometry template. The output is first reset to an empty data disk, then the
COM payloads are installed and compressed with `xz -9e`.

## Build

Configure a normal VAEG preset, then build the dedicated guest target. For
example on a MacPorts development machine:

~~~sh
cmake --preset macos-macports
cmake --build --preset macos-macports --target sgpsprite_com
~~~

The output is:

~~~text
build/macos-macports/guest/sgpdemo.com
~~~

Build the color-grouped distributed files with the repository helpers:

~~~sh
NASM=nasm demos/sgp-pseudo-sprite/16/build.sh /tmp/sgpdemo-16
NASM=nasm demos/sgp-pseudo-sprite/256/build.sh /tmp/sgpdemo-256/SGP256S.COM
NASM=nasm demos/sgp-pseudo-sprite/256/build-scroll.sh /tmp/sgpdemo-256/SGP256T.COM
NASM=nasm demos/sgp-pseudo-sprite/65536/build.sh /tmp/sgpdemo-65536/SGP655S.COM
~~~

The 16-color builder writes `SGPD_7A.COM` through `SGPD_7D.COM`, plus the
scrolling `SGPD_7S.COM`.
The complete data disk pair is generated
with:

~~~sh
NASM=nasm demos/sgp-pseudo-sprite/build-d88.sh \
  /path/to/pcengine110-bootonly.d88 /tmp/sgp-pseudo-sprite.d88
~~~

This writes `/tmp/sgp-pseudo-sprite.d88`; the compressed companion is written
to `demos/disks/sgp-pseudo-sprite.d88.xz`. Generated COM files and the raw D88
remain outside the source tree. The `.d88.xz` companion may be checked in only
when it contains the freely distributable payloads listed above; it represents
a non-bootable PC-Engine-free data image.
The canonical checked-in archive is
`demos/disks/sgp-pseudo-sprite.d88.xz`; the older
`sgpdemo.d88.xz` name is deprecated.

## Local bootable validation disk

For emulator or hardware boot-path validation, create a separate bootable
image from a local PC-Engine 2HD template:

~~~sh
NASM=/opt/local/bin/nasm demos/sgp-pseudo-sprite/build-bootable-d88.sh \
  /path/to/pcengine110-bootonly.d88 /private/tmp/sgp-demo-bootable.d88
~~~

This preserves the template's PC-Engine system files and installs the same
color-grouped payloads as the data-only archive. The resulting
`sgp-demo-bootable.d88` is a local validation artifact and must not be
committed or compressed for distribution.

The final source can also be assembled independently:

~~~sh
nasm -f bin -dM7_VARIANT=1 \
  demos/sgp-pseudo-sprite/sgp_sprite.asm \
  -o /tmp/SGPD_7A.COM
~~~

No generated `.COM` file belongs in the source tree.

## Disposable disk installation

The generated image is intentionally a data disk and is not bootable by
itself. Its root contains only the three color directories listed above; it
does not contain `ENGINEIO.SYS`, `PCENGINE.SYS`, `ADVGBIOS.SYS`, or
`PCENGINE.COM`. Create it from a local copy of the repository's PC-Engine-
layout source image:

~~~sh
work=$(mktemp -d /tmp/sgpdemo.XXXXXX)
NASM=nasm demos/sgp-pseudo-sprite/build-d88.sh \
  docs/disks/pcengine110-bootonly.d88 "$work/sgp-pseudo-sprite.d88"
python3 tools/pc88va/pcengine_disk.py list \
  --image "$work/sgp-pseudo-sprite.d88"
xz -dc demos/disks/sgp-pseudo-sprite.d88.xz | \
  cmp - "$work/sgp-pseudo-sprite.d88"
~~~

The final `list` must show exactly the 16-color, 256-color, and 65536-color
directories above, with no PC-Engine system files. To run them, mount a
bootable PC-Engine system disk in FDD1 and this data disk in FDD2:

~~~sh
build/macos-macports/sdl2/vaeg \
  --fdd1 docs/disks/pcengine110-bootonly.d88 \
  --fdd2 /path/to/sgp-pseudo-sprite.d88
~~~

At the DOS prompt, run a selected final variant on drive B:

~~~text
B:\16\SGPD_7A
B:\16\SGPD_7B
B:\16\SGPD_7C
B:\16\SGPD_7D
B:\16\SGPD_7S
B:\256\SGP256S
B:\256\SGP256T
B:\65536\SGP655S
~~~

For a developer-only disposable bootable test image, `vanilla` may still be
used with a system D88, but that image is not the distribution artifact.

## Expected final result

The screen must show the Graphic 0 checkerboard with at least 16 pre-rendered
shaded spheres spanning the HSV wheel and moving independently above it.
`FPSnnn Cnnnn` must be visible at the upper right after the first complete
measurement interval. Every sphere must retain the bright upper-left
highlight, dark lower-right body, and cast shadow. The checkerboard must
remain visible through each source-zero corner and everywhere Graphic 1 was
cleared to color 0.

The two balls on the central horizontal path must periodically overlap. During
an overlap, nonzero pixels from the later record must appear above those from
the earlier record while transparent pixels still reveal the lower ball or
checkerboard. Every ball must bounce at the screen edges.

Cursor Up or `+` must add records one at a time to a maximum of 256. Cursor Down
or `-` must remove balls one at a time to a minimum of one. Newly enabled balls
continue from their stored initial or last-active state.

The final program additionally exercises the bullet prefix: records 17 through 48 are
small 8x8 transparent bullets, so the first count increase after the initial
16 balls changes both the BITBLT dimensions and the transfer counters.

The hidden page is fully rendered before its VBLANK DSA1 exchange, so the
final program does not expose a visible clear. Pressing ESC must return to the
DOS prompt without a hang.
The program restores the saved BIOS mode and resets the standard palette;
arbitrary application palette contents cannot be preserved because VAEG does
not expose palette readback.

## 65536-color double-buffered track

`demos/sgp-pseudo-sprite/65536/` contains `SGP655S.COM`, a separate direct-color
track for the same SGP pseudo-sprite idea. It uses two 320x200 G0 pages from a
320x400, 16-bpp source surface and exchanges the contiguous pages with
the FB0 DSA registers. Each frame clears the hidden page and emits up to 128
moving 24x24 transparent 16-bpp BITBLT spheres. The 16 source bitmaps cycle
through HSV hue values; each bitmap remains monochromatic while using
supersampled Phong-style shading. The deterministic records use sixteen
velocity directions. SPACE toggles a SGP-drawn white 16-pixel square grid.
UP/DOWN (or `+`/`-`) changes the active count from one to 128.
It does not use
G1 or a CPU pixel loop. The wireframe track remains the reference for the
larger contiguous 16-bpp geometry workload.

Build it with:

```sh
NASM=/opt/local/bin/nasm demos/sgp-pseudo-sprite/65536/build.sh /tmp/SGP655S.COM
```
