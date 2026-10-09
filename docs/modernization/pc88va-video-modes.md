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
# PC-88VA Video-Mode and Framebuffer Control Reconstruction

> **Status:** Reconstructed, non-authoritative specification
>
> **Version:** 0.2
>
> **Date:** 2026-07-16 (JST)
>
> **Target:** NEC PC-88VA family
>
> **Repository path:** `docs/modernization/pc88va-video-modes.md`

> [!WARNING]
> This document separates timing, graphics fetch, framebuffer layout, SGP
> drawing, and final composition. It does not establish undocumented monitor
> timings as safe PC-88VA modes. Use the documented `SYNC` vectors first;
> arbitrary synchronization values can send an out-of-range signal to a CRT.

## 1. Purpose and evidence

Changing a PC-88VA display mode is not one register operation. At least five
independent functions must agree:

1. TSP CRT synchronization and active intervals;
2. graphics vertical interpretation and line count;
3. graphics horizontal fetch width and packed-pixel format;
4. GVRAM framebuffer layout and displayed sub-screen; and
5. the SGP destination descriptor used to draw into that layout.

The final D65101-side mixer then composes TSP text/sprites and graphics screens
G0/G1 in a common output coordinate system. The reconstructed TSP and SGP
specifications remain authoritative for their individual blocks:

- [uPD72022 TSP reconstruction](upd72022-tsp.md)
- [PC-88VA uPD92017 SGP reconstruction](upd92017-sgp.md)

Evidence labels used below are:

| Label | Meaning |
|---|---|
| `[DOCUMENTED]` | Stated by PC-88VA or uPD72022 technical material |
| `[IMPLEMENTATION]` | Observable in vaeg or another emulator; not automatically hardware behavior |
| `[INFERENCE]` | Derived from documented fields and necessary address arithmetic |
| `[CONFLICT]` | Surviving sources or source and implementation disagree |
| `[UNKNOWN]` | Real hardware or better primary material is still required |

## 2. Control ownership

| Function | Control location | Responsibility |
|---|---|---|
| CRT scan and synchronization | TSP `SYNC`, command/status port `0142h`, parameter port `0146h` | Dot-clock selection, horizontal/vertical active periods, borders, blanking, HSYNC, and VSYNC |
| Graphics line and scan interpretation | `GRMODE`, ports `0100h-0101h` | 200/204/400/408 lines and graphics raster-scan interpretation |
| G0/G1 horizontal fetch and pixel format | `GRRES`, ports `0102h-0103h` | Per-screen 640/320-dot fetch and 1/4/8/16-bpp interpretation |
| GVRAM image geometry and source scrolling | Framebuffer descriptors, ports `0200h-027fh` | Frame start, pitch, vertical extent, source offsets, and display start |
| Vertical sub-screen placement | Framebuffer `DSH` and `DSP` | Visible height and destination Y position |
| Drawing geometry | SGP `SET_SOURCE`/`SET_DESTINATION` descriptors | Drawing address, rectangle size, pixel format, and pitch |
| Final source composition | ports `0106h-0111h`, `0124h-0137h` | Priority, direct/palette color, masks, transparency, backdrop, and text/sprite boundary |

The SGP does not own monitor timing. Conversely, changing TSP `SYNC` does not
automatically change GVRAM pitch or the SGP drawing descriptor.

## 3. TSP `SYNC`: physical CRT scan

Software writes command `10h` to port `0142h`, then writes 14 parameter bytes
to port `0146h`. The timing fields are:

| Byte | Bits | Field | Function |
|---:|---:|---|---|
| 0 | 7:6 | `RM` | Raster mode |
| 1 | 5 | `EC` | Select internally divided dot-clock output when set |
| 1 | 4 | `ES` | Select synchronization output when set |
| 1 | 2:0 | `RS` | Internal dot-time divider selector |
| 2 | 5:0 | `LBL` | Left blanking |
| 3 | 5:0 | `LBR` | Left border |
| 4 | 7:0 | `HAD` | Horizontal active interval |
| 5 | 5:0 | `RBR` | Right border |
| 6 | 5:0 | `RBL` | Right blanking |
| 7 | 5:0 | `HS` | Horizontal synchronization interval |
| 8 | 5:0 | `TBL` | Top blanking |
| 9 | 5:0 | `TBR` | Top border |
| 10 | 7:0 | `VAD[7:0]` | Vertical active interval, low byte |
| 11 | 6 | `VAD[8]` | Vertical active interval, high bit |
| 11 | 5:0 | `BBR` | Bottom border |
| 12 | 5:0 | `BBL` | Bottom blanking |
| 13 | 5:0 | `VS` | Vertical synchronization interval |

`RM` has the generic uPD72022 meanings:

| `RM` | Generic raster mode |
|---:|---|
| `00b` | Noninterlace; canonical 640x400/24kHz example |
| `01b` | Interlace; canonical 640x400/15kHz example |
| `10b` | Vertical magnification; canonical 640x200/24kHz example |
| `11b` | Normal; canonical 640x200/15kHz example |

### 3.1 Generic internal-clock examples

`[DOCUMENTED]` For a 20MHz generic-device source clock, the uPD72022 data
book gives:

| `RS` | Divider | Example | `HAD` | `VAD` |
|---:|---:|---:|---:|---:|
| `000b` | /4 | 256x192 | 63 | 192 |
| `001b` | /3 | 320x200 | 79 | 200 |
| `010b` | /2 | 512x192 | 127 | 192 |
| `011b` | /1.5 | 640x200 | 159 | 200 |
| `100b` | /1 | 640x400 | 159 | 400 |

The horizontal active width follows:

~~~text
active dots = (HAD + 1) * 4
~~~

Thus `HAD=63`, for example, represents 256 active dots.

These are generic uPD72022 examples, not a list of complete PC-88VA machine
modes. `[DOCUMENTED]` Known PC-88VA vectors use an external dot clock and
carry `RS=111b`; their raw timing fields, graphics fetch, sprites, and mixer
must be treated as one platform configuration.

### 3.2 Documented PC-88VA timing profiles

The PC-88VA material records the following complete `SYNC` vectors:

| Platform mode | Horizontal profile | Raw TSP raster use | 14 parameter bytes |
|---:|---:|---|---|
| 200 lines | 15.98kHz | `RM=11b`, normal raster | `C1 57 1C 00 9F 00 10 0F 25 00 C8 00 0F 08` |
| 200 lines | 15.73kHz | `RM=11b`, normal raster | `C1 47 1C 00 9F 00 12 11 24 00 C8 00 17 04` |
| 200 lines | 24.8kHz | `RM=10b`, vertically magnified | `81 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 204 lines | 15.98kHz | `RM=11b`, normal raster | `C1 57 1C 00 9F 00 10 0F 23 00 CC 00 0D 08` |
| 204 lines | 15.73kHz | `RM=11b`, normal raster | `C1 47 1C 00 9F 00 12 11 22 00 CC 00 15 04` |
| 204 lines | 24.8kHz | `RM=10b`, vertically magnified | `81 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |
| 400 lines | 15.73kHz | `RM=01b`, interlace, 200-line sprite data | `41 47 1C 00 9F 00 12 11 24 00 C8 00 17 04` |
| 400 lines | 24.8kHz | `RM=00b`, noninterlace, 200-line sprite data | `01 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 400 lines | 24.8kHz | `RM=11b`, normal raster, 400-line sprite data | `C1 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 408 lines | 15.73kHz | `RM=01b`, interlace, 200-line sprite data | `41 47 1C 00 9F 00 12 11 22 00 CC 00 15 04` |
| 408 lines | 24.8kHz | `RM=00b`, noninterlace, 200-line sprite data | `01 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |
| 408 lines | 24.8kHz | `RM=11b`, normal raster, 400-line sprite data | `C1 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |

The descriptive platform mode must not replace raw-byte decoding. In
particular, the 200/204-line 15.73kHz vectors begin with `C1h` and therefore
select `RM=11b`, not the `RM=01b` interlace mode used by the 400/408-line
15.73kHz vectors.

### 3.2.1 Invariant fields and the first two bytes

Every vector has `HAD=9Fh`, so every documented PC-88VA profile has a 640-dot
TSP active interval:

~~~text
(9Fh + 1) * 4 = 640 dots
~~~

Every first byte also has `ILM=1`, selecting the interleave memory interface.
The four observed first-byte values differ primarily in `RM`:

| Byte 0 | `RM` | `ILM` | Raw TSP interpretation |
|---:|---:|---:|---|
| `C1h` | `11b` | 1 | Normal raster progression |
| `81h` | `10b` | 1 | Vertical magnification |
| `41h` | `01b` | 1 | Interlace |
| `01h` | `00b` | 1 | Noninterlace |

The two observed second-byte values are:

| Byte 1 | `RF` | `EC` | `ES` | `RS` | Interpretation |
|---:|---:|---:|---:|---:|---|
| `57h` | 1 | 0 | 1 | `111b` | External dot clock; TSP outputs HSYN/VSYN |
| `47h` | 1 | 0 | 0 | `111b` | External dot clock; TSP accepts external HSYN/VSYN |

Thus none of these vectors uses the generic internal-clock `RS=000b` through
`100b` modes. The 15.73kHz family alone uses `ES=0`; identifying the exact
board-level synchronization master in this configuration still requires a
schematic or hardware measurement.

### 3.2.2 Three horizontal timing families

`[IMPLEMENTATION]` Current vaeg calculates PC-88VA line width without adding
one to `LBR` or `RBR`:

~~~text
H_total_dots =
    ((LBL + 1) + LBR + (HAD + 1) +
     RBR + (RBL + 1) + (HS + 1)) * 4
~~~

With the current vaeg external-dot-clock constants, the twelve vectors reduce
to three horizontal families:

| Profile | `LBL LBR HAD RBR RBL HS` | Total dots | vaeg dot clock | Derived horizontal rate |
|---:|---|---:|---:|---:|
| 15.98kHz | `1C 00 9F 00 10 0F` | 888 | 14.189837MHz | 15.9795kHz |
| 15.73kHz | `1C 00 9F 00 12 11` | 904 | 14.252364MHz | 15.7659kHz |
| 24.8kHz | `10 00 9F 00 10 0F` | 840 | 20.854022MHz | 24.8262kHz |

The generic uPD72022 formula adds one to both border fields and would instead
give 896, 912, and 848 dots. With the vaeg clock constants, those totals do not
match the named horizontal profiles as closely. This strengthens, but does not
prove, the vaeg real-machine-fit hypothesis that PC-88VA `LBR/RBR` use differs
from the generic data-book explanation. The clock constants themselves include
implementation fitting and are not all independently established crystal
frequencies.

Current vaeg chooses the clock family from the monitor selection and
`GRMODE` bit 7, not by matching a `SYNC` byte sequence. Therefore an absolute
frequency can be derived only after the TSP vector and board-level graphics
mode are paired.

### 3.2.3 Vertical totals and logical-line interpretation

`[IMPLEMENTATION]` Current vaeg uses:

~~~text
V_total_lines = TBL + TBR + VAD + BBR + BBL + VS
field_rate     = horizontal_rate / V_total_lines
~~~

This gives:

| Platform mode | Raw `VAD` | Total lines | Raw `RM` | Derived field/frame cadence |
|---:|---:|---:|---:|---:|
| 200/15.98 | 200 | 260 | `11b` | 61.46Hz |
| 204/15.98 | 204 | 260 | `11b` | 61.46Hz |
| 200/15.73 | 200 | 263 | `11b` | 59.95Hz |
| 204/15.73 | 204 | 263 | `11b` | 59.95Hz |
| 200/24.8 | 400 | 440 | `10b` | 56.42Hz |
| 204/24.8 | 408 | 448 | `10b` | 55.42Hz |
| 400/15.73 | 200 per field | 263 per field | `01b` | 59.95 fields/s, about 29.97 two-field frames/s |
| 408/15.73 | 204 per field | 263 per field | `01b` | 59.95 fields/s, about 29.97 two-field frames/s |
| 400/24.8 | 400 | 440 | `00b` or `11b` | 56.42Hz |
| 408/24.8 | 408 | 448 | `00b` or `11b` | 55.42Hz |

Several relationships are now visible:

- The 15kHz 200-to-204 change adds four active lines but removes four lines
  from top/bottom blanking, preserving totals of 260 or 263 lines.
- The 24.8kHz 200/204 modes use raw `VAD=400/408` with `RM=10b`, so the TSP
  scans 400/408 physical rasters while vertically magnifying 200/204-line data.
- The 200-line and 400-line 15.73kHz vectors have the same horizontal and
  vertical interval bytes. Changing byte 0 from `C1h` to `41h` changes raster
  progression to interlace rather than changing the CRT interval counts.
- The three 24.8kHz 200/400 variants likewise share physical interval values.
  Byte 0 selects vertical magnification, noninterlace with 200-line sprite
  data, or normal raster progression with 400-line sprite data.

The rates above are derived from the current vaeg timing model. They are not
yet direct oscilloscope measurements of every PC-88VA model.

### 3.2.4 Current confidence boundary

The vectors now determine, or strongly constrain:

- every timing-field value and the 640-dot active width;
- the three horizontal timing families;
- raw active and total line counts in the current model;
- normal, magnified, interlace, and noninterlace raster selection;
- the distinction between logical 200/204-line data and physical 400/408-line
  scanning; and
- why 200/400 or 204/408 profiles can share almost all timing bytes.

The remaining hardware questions are:

- the precise TSP/D65101 synchronization-master relationship when `ES=0`;
- exact external dot-clock frequency and model variation;
- hardware confirmation of the no-`+1` `LBR/RBR` rule;
- even/odd-field phase, half-line behavior, and sync polarity in interlace;
- exact text/sprite address progression for the two 24.8kHz 400-line
  interpretations; and
- the required `GRMODE` and monitor-switch pairing for every vector.

## 4. Port `0100h`: `GRMODE`

`GRMODE` is a little-endian 16-bit register at ports `0100h-0101h`. The
recoverable graphics line selections are:

| Bits | Value | Meaning |
|---:|---:|---|
| 1:0 | `00b` | 400 lines |
| 1:0 | `01b` | 408 lines |
| 1:0 | `10b` | 200 lines |
| 1:0 | `11b` | 204 lines |

The frozen VA diagnostic also decodes bits 7:6 as:

| Bits 7:6 | Graphics raster interpretation |
|---:|---|
| `00b` | Noninterlace 0 |
| `01b` | Noninterlace 1 |
| `10b` | Interlace 0 |
| `11b` | Interlace 1 |

Other relevant mode bits include bit 10 for single-plane graphics and bit 11
for one/two graphics screens. The complete mode word must be preserved when
only the line field changes. For example:

~~~c
/* Select the 200-line field without disturbing other GRMODE bits. */
grmode = (grmode & ~0x0003u) | 0x0002u;
~~~

`GRMODE.VW` describes how the graphics path treats its lines. Physical CRT
timing still comes from the matching TSP `SYNC` vector. Writing only port
`0100h` does not safely change 15/24kHz synchronization.

## 5. Port `0102h`: `GRRES`

`GRRES` is a little-endian 16-bit register at ports `0102h-0103h`:

| Bits | Name | Screen | Meaning |
|---:|---|---:|---|
| 1:0 | `PM0` | G0 | `00=1`, `01=4`, `10=8`, `11=16` bpp |
| 4 | `HW0` | G0 | `0=640`, `1=320` logical dots |
| 9:8 | `PM1` | G1 | `00=1`, `01=4`, `10=8`, `11=16` bpp |
| 12 | `HW1` | G1 | `0=640`, `1=320` logical dots |

`[IMPLEMENTATION]` vaeg expands each 320-dot graphics pixel to two pixels in
the common 640-dot output buffer. TSP text and sprite X coordinates therefore
remain on their 640-dot basis.

Examples that preserve unrelated bits are:

~~~c
/* G0: 320 dots, 4 bpp. */
grres = (grres & ~0x0013u) | 0x0011u;

/* G1: 320 dots, 4 bpp. */
grres = (grres & ~0x1300u) | 0x1100u;
~~~

The name "16 bpp" is retained here. The exact direct-color component layout
should be established separately before naming it RGB565 normatively.

## 6. Ports `0200h-027fh`: framebuffer descriptors

Four `20h`-byte register blocks describe GVRAM source layout and displayed
sub-screens:

| Descriptor | Port range | Current graphics use |
|---:|---:|---|
| FB0 | `0200h-021fh` | First G0 sub-screen |
| FB1 | `0220h-023fh` | G1 |
| FB2 | `0240h-025fh` | Second G0 sub-screen |
| FB3 | `0260h-027fh` | Third G0 sub-screen |

Each block has this layout:

| Offset | Field | Stored form | Reconstructed role |
|---:|---|---|---|
| `+00h..+02h` | `FSA` | 18-bit address, low two bits zero | Virtual framebuffer start |
| `+04h..+05h` | `FBW` | 11-bit value, low two bits zero | Address pitch between source lines |
| `+06h..+07h` | `FBL` | 10-bit value | Virtual framebuffer final line/vertical wrap extent |
| `+08h` | `DOT` | 5-bit value | Starting pixel lane within the source word group |
| `+0ah..+0bh` | `OFX` | aligned source X offset | Horizontal source offset/scroll |
| `+0ch..+0dh` | `OFY` | 10-bit source Y offset | Vertical source offset/scroll |
| `+0eh..+10h` | `DSA` | 18-bit address, low two bits zero | Displayed source start address |
| `+12h..+13h` | `DSH` | 9-bit value | Displayed sub-screen height |
| `+16h..+17h` | `DSP` | 9-bit value | Destination Y position on the CRT raster |
| other | - | Reserved | Write zero |

`[IMPLEMENTATION]` vaeg advances each single-plane source line by `FBW`. With
`OFY=0`, it performs vertical wrap after `FBL+1` lines. It activates a
descriptor when the current output Y equals `DSP`, and stops it at
`DSP+DSH`. These relations make the current interpretation:

~~~text
source pitch             = FBW address units
virtual height           = FBL + 1, when OFY is zero
visible destination rows = DSP through DSP + DSH - 1
~~~

FB1 is special in the current implementation: writes to its `FSA`, `FBL`,
`OFX`, and `OFY` are ignored and it is initialized without vertical wrap.

`[CONFLICT]` The vaeg structure comment calls `OFX` a 10-bit field, while the
write handler retains three high bits and therefore accepts an 11-bit aligned
value. This document does not resolve the real-hardware field width.

### 6.1 Width, start, position, and scrolling

The fields divide into three responsibilities:

~~~text
FSA / FBW / FBL
    virtual GVRAM image and vertical wrap

DOT / OFX / OFY / DSA
    source start and source scrolling within that image

DSH / DSP
    visible height and destination Y position
~~~

No equivalent of the TSP split descriptor's destination `RXP` has yet been
identified for graphics. `OFX` changes the GVRAM source position; it must not
be documented as a proven destination-X placement register.

For a tightly packed single-plane image:

~~~text
minimum byte pitch = logical_width * bits_per_pixel / 8
~~~

`FBW` is aligned to four bytes. Examples are:

| Width | 1 bpp | 4 bpp | 8 bpp | 16 bpp |
|---:|---:|---:|---:|---:|
| 320 | 40B | 160B | 320B | 640B |
| 384 | 48B | 192B | 384B | 768B |
| 640 | 80B | 320B | 640B | 1280B |

These are arithmetic minimum pitches, not proof that every width is a native
graphics-fetch mode. `GRRES` exposes only 320 and 640 logical dots.

## 7. SGP drawing pitch

The SGP destination descriptor carries its own address, block width, block
height, pixel mode, start dot, and `FBW`. When display geometry changes, the
SGP descriptor must use the same packed-pixel format and line pitch as the
GVRAM layout.

Conceptually:

~~~text
logical width
    -> packed bytes per line
    -> graphics framebuffer FBW
    -> vertical wrap and DSA stepping
    -> SGP SET_DESTINATION FBW and rectangle width
~~~

The SGP has no command that directly selects "320x200" or "640x400" monitor
resolution.

## 8. Reconstructed normal-mode combinations

The principal PC-88VA design combinations are 320x200, 640x200, 320x400, and
640x400, with 204/408 variants. Horizontal and vertical selection are in
different registers, but a combination is not called hardware-confirmed until
it has passed a real-machine test with a matching TSP timing profile.

| Logical graphics size | `GRMODE` bits 1:0 | `HW` | 4-bpp minimum `FBW` |
|---:|---:|---:|---:|
| 320x200 | `10b` | 1 | 160B |
| 640x200 | `10b` | 0 | 320B |
| 320x400 | `00b` | 1 | 160B |
| 640x400 | `00b` | 0 | 320B |

### 8.1 Candidate G0 320x200, 4-bpp setup

The following describes the field relationships, not a complete executable
initialization routine:

~~~text
TSP SYNC
    documented 24.8kHz / 200-line vertical-magnification vector

GRMODE bits 1:0
    10b, 200-line graphics interpretation

GRRES.HW0 / PM0
    HW0=1, PM0=01b, 320 dots and 4 bpp

FB0
    FBW=160 bytes
    FBL=199 candidate for a 200-line buffer in the vaeg interpretation
    DSH=200 for a full-height sub-screen
    DSP=0 for top placement

SGP destination
    4 bpp, 320-pixel rectangle, 200 lines, 160-byte pitch
~~~

The corresponding field updates are:

~~~c
grmode = (grmode & ~0x0003u) | 0x0002u;
grres  = (grres  & ~0x0013u) | 0x0011u;
~~~

`0102h` alone is insufficient. Leaving a 640-dot 4-bpp pitch of 320 bytes
while drawing and fetching a tightly packed 320-dot image at 160 bytes causes
different blocks to disagree about the next scan line.

## 9. Non-native widths

### 9.1 Generic 256x192 is not a confirmed PC-88VA mode

The generic uPD72022 example `RS=000b`, `HAD=63`, `VAD=192` generates a
256x192 active interval with its internal-clock assumptions. The PC-88VA's
documented graphics controls do not expose a 256-dot `HW` selection, and its
known TSP vectors use external dot-clock input.

Therefore:

> The uPD72022 alone can generate 256x192, but no complete native PC-88VA
> 256x192 mode composited with GVRAM has been established.

A TSP-only 256-dot width can disagree with G0/G1 fetch, the permanent 640-dot
sprite coordinate basis, and mixer boundaries.

### 9.2 Safe initial 384x256 viewport experiment

There is no documented native 384-dot `GRRES` selection. The safe initial
experiment is therefore a content rectangle inside a documented raster:

~~~text
physical raster       640 x 400
graphics source rows  at least 640 pixels wide
content rectangle     384 x 256
content origin        (128, 0) within each source row
destination Y         DSP = 72
visible height        DSH = 256
outside content       transparent or backdrop composition
~~~

For 4 bpp, retain a 640-dot source pitch of 320 bytes and place the first
content pixel 64 bytes from the start of each row. This avoids assuming that
the graphics fetcher stops after 384 pixels. A tightly packed 192-byte pitch
for a 384-pixel row is useful as a later discriminator test, but is not yet a
documented 384-dot display mode.

`DSP` supplies vertical destination placement. No proven graphics
destination-X register has been identified, so horizontal centering should
initially be represented in the stored 640-dot row and final transparency or
masking, not attributed to `OFX`.

## 10. Safe real-hardware test order

1. Establish the known 640x400/24.8kHz profile and display a one-dot grid.
2. Change to a documented 640x200 combination and determine whether the
   logical lines are vertically enlarged or surrounded by blank output.
3. Select `HW0=1` for 320x200 and verify physical two-dot horizontal pixels.
4. Change `FBW` independently and verify the exact next-line address.
5. Test 320x400 to determine whether horizontal and vertical selections are
   independent on hardware.
6. Test the padded 384x256 content rectangle while retaining 640x400 timing.
7. Only after those controls are isolated, test a tightly packed 384-pixel
   pitch and record fetch, wrap, and composition behavior.

Every capture should retain the raw `SYNC` bytes, `GRMODE`, `GRRES`, all active
framebuffer descriptors, SGP destination descriptor, GVRAM before/after, CRT
monitor selection, and measured horizontal/vertical frequency.

## 11. Implementation evidence and unresolved points

The active vaeg implementation provides the following evidence:

- [`io/videova.c`](../../io/videova.c) binds `GRMODE`, `GRRES`, and four
  framebuffer descriptor blocks and shows each writable bit mask;
- [`io/videova.h`](../../io/videova.h) names the descriptor fields;
- [`vram/makegrphva.c`](../../vram/makegrphva.c) implements 320-dot
  horizontal duplication, `FBW` line stepping, `FBL` wrap, `DSH` height,
  `DSP` placement, and FB0/FB2/FB3 versus FB1 selection; and
- the archived
  [`win9x/debuguty/viewvideova.cpp`](https://github.com/nakatamaho/vaeg/blob/b72e641733ddea6f0e8faef2507093f7c3aee5a4/win9x/debuguty/viewvideova.cpp)
  records the original diagnostic interpretation of `GRMODE` and `GRRES`.

The archived file is evidence only and must not be modified.

The principal unresolved points are:

1. exact hardware field width and units for `OFX`;
2. exact count-versus-last-index convention for every framebuffer dimension;
3. whether FB1's non-writable fields reflect hardware or only legacy design;
4. behavior when `FBW` is smaller than the selected 320/640 graphics fetch;
5. whether a graphics destination-X placement control exists elsewhere;
6. native status of the apparent 320x400 combination;
7. exact direct-color layout of `PM=11b`; and
8. model differences among VA, VA2, and VA3.

## 12. Measured non-native timings

`[MEAS]` Real-hardware runs of
[`V480PAT`](../../tools/pc88va/vtiming/README.md) (M104; binary SHA-256
`3a1e5a03477c47dd2f8ac71337fcbb5bce941b6d66ddba49756fe653f82f7550`, before
the line ruler) and VIEW480 under PC-Engine 1.1 on the maintainer's
PC-88VA2, 2026-10-05/06. Monitor:
I-O DATA LCD-MF225XBR (LCD; 15, 24 and 31 kHz inputs). Frame totals and
rates are derived from the `SYNC` bytes with vaeg's dot clocks, not
measured.

| Run | `SYNC` lines (top/active/bottom/sync) | Frame | Rate | Result |
|---|---|---:|---:|---|
| `V480PAT` (no switching) | ROM 400-line | 440 | 56.4 Hz | pattern drawn as designed: the GVRAM bank (`0153h` = 4) and CPU-data write mode (`0580h` = 10h) work on hardware |
| `VIEW480` | 25/480/1/1 | 507 | 49.0 Hz | out of range on this monitor (one earlier look showed a vertically compressed picture with the 400-line edge visible; the repeat reported out of range) |
| `V480PAT 200 D` | 25/400/7/8 | 440 | 56.4 Hz | 320x200 shown with every line on two rasters, no gaps: RSM = 01 doubles graphics lines |
| `V480PAT 208 D` to `232 D` | 25/416-464/2/4 | 447-495 | 55.5-50.2 Hz | all displayed; with `232 D` the last line (231) was not seen |
| `V480PAT 240 D` | 25/480/2/4 | 511 | 48.6 Hz | out of range |
| ROM 15 kHz 200-line screen | 37/200/15/8 | 260 | 61.5 Hz | displayed (monitor switch at 15 kHz) |
| `V480PAT 240 T W` | 32/240/2/4 | 278 | 57.5 Hz | 320x240 displayed; the last line (239) not seen |
| `V480PAT 240 U W` | 16/240/2/4 | 262 | 61.0 Hz | displayed; the last line (239) not seen |
| `V480PAT 224 U W` | 16/224/18/4 | 262 | 61.0 Hz | displayed; the white line seen with a little yellow below it was line 200 and lines 201-202 (second round), not line 223 |
| `V480PAT 240 T W N` | as `240 T W`, `HAD` 127, `LBR`/`RBR` 16 | 278 | (57.5 Hz) | displayed; the lower band visible; horizontal result not yet recorded |

Conclusions supported by these runs:

- The TSP accepts a 15.98 kHz `VAD` of 224 and 240 lines (beyond the
  documented 200/204) with 262- and 278-line frames, and 320-dot graphics
  in 200-line mode then gives a 320x224 or 320x240 picture.
- At 24.8 kHz, the RSM = 01 line doubling continues past 204 lines, to at
  least 232 lines (464 rasters).
- This monitor's lower vertical limit at 24.8 kHz lies between 49.0 and
  50.2 Hz; a 480-raster frame cannot reach 50 Hz with the ROM's top
  blanking.
- Where the 240-line pictures lose their bottom lines is measured in the
  second round below.

### 12.1 Second round: line ruler

`[MEAS]` 2026-10-06, PC-88VA2, same monitor, V480PAT with the line ruler
and command label (binary built from
[`290d976c`](https://github.com/nakatamaho/vaeg/commit/290d976c48f553b7256d82fa4e41edc61c0c5c9d)).
"Last line" is read from the ruler (about plus or minus 1 line). Photographs
(maintainer-authorised, metadata removed) are in
[`m104-photos/`](m104-photos/).

| Run | Clock | `SYNC` lines (top/active/bottom/sync) | Last graphics line seen | Photo |
|---|---|---|---:|---|
| `V480PAT 232 D` | 24.8 kHz | 25/464 rasters/2/4 | lines 0-200, then line 200 repeated to the end of the active area | [232 D](m104-photos/va2-232-d.jpg) |
| `V480PAT 232 D N` | 24.8 kHz | as above, `HAD` 127 | as above | [232 D N](m104-photos/va2-232-d-n.jpg) |
| `V480PAT 240 T W` | 15.98 kHz | 32/240/2/4 | 234 | [240 T W](m104-photos/va2-240-t-w.jpg) |
| `V480PAT 240 U W` | 15.98 kHz | 16/240/2/4 | 218 | [240 U W](m104-photos/va2-240-u-w.jpg) |
| `V480PAT 224 U W` | 15.98 kHz | 16/224/18/4 | 202 | [224 U W](m104-photos/va2-224-u-w.jpg) |
| `V480PAT 224 T W N` | 15.98 kHz | 32/224/2/4, `HAD` 127 | 218 | [224 T W N](m104-photos/va2-224-t-w-n.jpg) |
| `V480PAT 240 T` | 15.98 kHz | 32/240/2/4, 640 dots | 234 | [240 T](m104-photos/va2-240-t.jpg), [repeat](m104-photos/va2-240-t-repeat.jpg) |
| `V480PAT 300 S` | 24.8 kHz | 25/300/7/8 (340, 73.0 Hz) | monitor: unsupported signal | [out of range](m104-photos/va2-out-of-range.jpg) |
| `V480PAT 256 T W` | 15.98 kHz | 32/256/2/4 (294, 54.4 Hz) | monitor: unsupported signal (same screen) | - |

Findings:

- **15 kHz vertical extent.** In all four 15 kHz runs the number of
  graphics lines shown equals `TBL + VAD - 37`: 235, 219, 203 and 219,
  against last lines 234, 218, 202 and 218 read off the ruler; `240 T` at
  640 dots also ends at 234. 37 is the top blanking of the
  ROM's own 15.98 kHz vector (`25h`). Line 0 is visible at the top in every
  run. `[DERIVED]` A consistent model: the graphics display circuit
  (D65101 side) starts graphics line 0 a fixed 37 lines after vertical sync,
  independently of the TSP's `TBL`, and output ends with the TSP active
  area. With the ROM's vector the two coincide. The equivalent count rule
  holds whether the start is fixed (and the picture shifts by `37 - TBL`
  lines against TSP text) or the count alone is short; the photos do not
  separate them, because nothing TSP-drawn is in the measured area.
- **24.8 kHz line doubling.** With port `0100h` 200-line mode and RSM = 01,
  graphics lines 0-200 are shown doubled and line 200 is then repeated on
  every remaining raster: the graphics line address stops advancing after
  line 200 at 24.8 kHz. At 15.98 kHz, also in 200-line mode, it advances to
  at least line 234.
- **Horizontal (`N`).** With `HAD` reduced to 127 TCK and 16 TCK added to
  each border, the whole 320-dot graphics line is still shown: both
  window-edge marks (dots 32 and 286) appear inside the picture with
  pattern outside them, at 15.98 and 24.8 kHz. Graphics are not clipped to
  the TSP horizontal active period. The monitor rescaled the picture width,
  so the photos do not show the line frequency.
- The monitor reported an unsupported signal for a 340-line 24.8 kHz frame
  (73 Hz) and a 294-line 15.98 kHz frame (54.4 Hz). These are monitor
  limits, not VA results.

### 12.2 Feasible resolutions

Combining the native modes with the 12.1 findings (15.98 kHz: graphics show
`TBL + VAD - 37` lines; 24.8 kHz RSM = 01 doubling stops after line 200;
widths only 640 or 320, not clipped by `HAD`) and the monitor limits seen
in 12-12.4. Status: **native** = documented mode; **measured** = shown
on the PC-88VA2 in 12-12.4; **derived** = follows from the rules, not yet
run; **not possible** = excluded by a measured rule or a missing control.
Rates use vaeg's line rates (15.98 kHz: 15.980 kHz, 24.8 kHz: 24.826 kHz).

To show `N` full graphics lines at 15.98 kHz the frame needs at least
`N + 37 + BBL + VS` lines (`N + 43` with the data-book minimums
`BBL = 2`, `VS = 4`). `TBL = 37` (the ROM's value) also keeps graphics and
TSP text aligned, which avoids the open shift question.

| Resolution (W x graphics lines) | Clock | `SYNC` lines (top/active/bottom/sync) | Frame | Rate | Status |
|---|---|---|---:|---:|---|
| 640 x 400 | 24.8 kHz | 25/400/7/8 | 440 | 56.4 Hz | native |
| 640/320 x 200, line-doubled (RSM = 01) or blank odd rasters (RSM = 00) | 24.8 kHz | 25/400/7/8 | 440 | 56.4 Hz | native; doubling measured (`200 D`) |
| 640/320 x 200 | 15.98 kHz | 37/200/15/8 | 260 | 61.5 Hz | native |
| 640/320 x 219 | 15.98 kHz | 16/240/2/4 | 262 | 61.0 Hz | measured (`240 U W`; lines 219-239 of the buffer not shown) |
| 640/320 x 224 | 15.98 kHz | 37/224/2/4 | 267 | 59.9 Hz | measured (`224 R W`, `224 R`; 12.4) |
| 640/320 x 232 | 15.98 kHz | 37/232/2/4 | 275 | 58.1 Hz | measured (`232 R W`) |
| 640/320 x 235 | 15.98 kHz | 32/240/2/4 | 278 | 57.5 Hz | measured (`240 T W`, `240 T`) |
| 640/320 x 236 | 15.98 kHz | 37/236/2/4 | 279 | 57.3 Hz | measured (`236 R W`) |
| 640/320 x 240 | 15.98 kHz | 37/240/2/4 | 283 | 56.5 Hz | measured (`240 R W`, `240 R`, `240 R W N`; all 240 lines) |
| 640/320 x 241-248 | 15.98 kHz | 37/244-248/2/4 | 287-291 | 55.7-54.9 Hz (monitor: 55.7, 55.0) | frame accepted; this monitor showed it as 240 lines (1740 x 240), so lines 240-247 were not visible |
| 640/320 x 201-240, line-doubled | 24.8 kHz | 25/402-480/2/4 | 443-511 | 56.0-48.6 Hz | not possible: graphics stop advancing after line 200 (again with top 17: `232 D K`, `240 D K`) |
| 640 x 401-476, 400-line graphics mode | 24.8 kHz | 25/401-476/1/1 | 428-503 | 58.0-49.4 Hz | measured (`432 S`-`476 S`): graphics lines past 400 are shown; at 408-424 lines this monitor treated the input as 640 x 400 and showed only lines 0-399 |
| 640 x 480, 400-line graphics mode | 24.8 kHz | 17/480/1/1 | 499 | 49.7 Hz (monitor: 49.7) | measured (`480 S K`): all 480 lines |
| 640 x 480 (VIEW480) | 24.8 kHz | 25/480/1/1 | 507 | 49.0 Hz | out of range on this monitor (`480 S`, `480 S W`) |
| 256 (or any width other than 640/320) | any | `HAD` narrowed | - | - | not possible: graphics width is 640 or 320 and is not clipped by `HAD` |
| 256 x 192 window in 320 x 200 | 15.98 kHz / 24.8 kHz | ROM vectors | 260 / 440 | 61.5 / 56.4 Hz | measured (`G256`, 12.3) |

The 320 x 224 at about 60 Hz row is the closest to common console timing
that the rules allow with this monitor; 320 x 240 at 56.5 Hz (283-line
frame) is measured. This monitor accepted 24.8 kHz frames up to 503 lines
(49.4 Hz) and 15.98 kHz frames up to 291 lines (54.9 Hz), and rejected 507
and 294 lines.

### 12.3 256 x 192 window (G256)

`[MEAS]` 2026-10-06, PC-88VA2, same monitor,
[`G256`](../../tools/pc88va/vtiming/README.md#g256-a-256-x-192-window)
(built from
[`a1363f4d`](https://github.com/nakatamaho/vaeg/commit/a1363f4d59df878dfe5f0c1f150f5bb69f2246c4)).
The program keeps the ROM's 200-line vectors and draws a 256 x 192 window
at GVRAM x 32-287, y 4-195 in single-plane 320 x 200 graphics, black
outside; at 24.8 kHz port `0100h` RSM = 01 doubles each line. The line
frequency of each photograph is identified from the PC-Engine text-row
pitch (15.98 kHz rows are twice as tall, as in vaeg).

| Run | Clock | Result | Photo |
|---|---|---|---|
| `G256` | 15.98 kHz | window shown, one-dot checks resolved, 16 depth dots | [15k 16](m104-photos/va2-g256-15k-16.jpg) |
| `G256 8` | 15.98 kHz | as above, 8-bit gradient bands, 8 dots | [15k 8](m104-photos/va2-g256-15k-8.jpg) |
| `G256 4` | 15.98 kHz | as above, 16-colour blocks, 4 dots | [15k 4](m104-photos/va2-g256-15k-4.jpg) |
| `G256` | 24.8 kHz | as at 15.98 kHz, line-doubled | [24k 16](m104-photos/va2-g256-24k-16.jpg) |
| `G256 8` | 24.8 kHz | as above | [24k 8](m104-photos/va2-g256-24k-8.jpg) |
| `G256 4` | 24.8 kHz | as above | [24k 4](m104-photos/va2-g256-24k-4.jpg) |

Findings:

- The top, left and right white borders, the grey 32-dot grid, the
  one-dot red/white checks and the depth dots on line 1 are visible in all
  six runs, and the layout and colours match vaeg's screenshots of the same
  program at each depth.
- The window's bottom rows (lines about 192-195) lie under PC-Engine's
  function-key row, which is TSP text drawn over graphics, so the bottom
  border is hidden by text at both frequencies, as in vaeg. It is not a
  timing loss.
- `[DERIVED]` A 256 x 192 picture needs no `SYNC` change on the VA: it is
  a letterboxed window in the native 320 x 200 mode, with 320-mode dots, at
  61.5 Hz (15.98 kHz) or 56.4 Hz (24.8 kHz).

### 12.4 Third round: sweeps, monitor read-out and G160 rates

`[MEAS]` 2026-10-07, PC-88VA2, same monitor, the sweep lists of
[`V480PAT`](../../tools/pc88va/vtiming/README.md#usage) (`A` at 24 kHz, `B`
at 15 kHz), `G256 A`, `G160 A` and `TSPMODE`, all built from
[`4119af16`](https://github.com/nakatamaho/vaeg/commit/4119af1672fd2f9593e5ca89f669b2b375ad8886).
The monitor's input information (resolution, line and field rate) was
photographed where it appeared; it is the monitor's measurement, not the
VA's. "Last line" is read from the ruler.

| Run | Frame (top/active/bottom/sync) | Monitor read-out | Last graphics line seen | Photo |
|---|---|---|---:|---|
| `400 S` | 25/400/7/8 (440) | - | 399 | - |
| `408 S`-`424 S` | 25/N/1/1 (435-451) | `416 S`: 640x400, 24.8 kHz, 56.0 Hz | 399 (the monitor's 640 x 400 window) | [416 S](m104-photos/va2-416-s-osd.jpg) |
| `432 S`-`472 S` | 25/N/1/1 (459-499) | `440 S`: 640x448, 24.9 kHz, 53.3 Hz | N - 1 | [440 S](m104-photos/va2-440-s-osd.jpg) |
| `476 S` | 25/476/1/1 (503, 49.4 Hz) | - | 475 | [476 S](m104-photos/va2-476-s.jpg) |
| `480 S`, `480 S W` | 25/480/1/1 (507, 49.0 Hz) | unsupported signal | - | [480 S](m104-photos/va2-480-s-out-of-range.jpg) |
| `440 S W`, `440 S N` | as `440 S` | `440 S N`: 640x448, 24.8 kHz, 53.1 Hz | 439 | - |
| `464 S K`-`480 S K` | 17/N/1/1 (483-499) | `480 S K`: 640x480, 24.8 kHz, 49.7 Hz | N - 1 | [480 S K](m104-photos/va2-480-s-k-osd.jpg) |
| `200 D` | 25/400/7/8 (440) | 640x400, 24.8 kHz, 56.4 Hz | 199 | - |
| `240 D` | 25/480/2/4 (511, 48.6 Hz) | unsupported signal | - | - |
| `232 D K`, `240 D K` | 17/2N/2/4 (487, 503) | `240 D K`: 640x480, 24.8 kHz, 49.3 Hz | 200, then line 200 repeated | [232 D K](m104-photos/va2-232-d-k.jpg), [240 D K](m104-photos/va2-240-d-k-osd.jpg) |
| `200 R W`-`240 R W` (8 runs) | 37/N/2/4 (243-283) | - | N - 1 in every run | [240 R W](m104-photos/va2-240-r-w.jpg) |
| `244 R W`, `248 R W` | 37/N/2/4 (287, 291) | 1740x240, 16.0 kHz, 55.7 Hz and 55.0 Hz | 239 (the monitor's 240-line window) | [248 R W](m104-photos/va2-248-r-w.jpg), read-out [244](m104-photos/va2-244-r-w-osd.jpg), [248](m104-photos/va2-248-r-w-osd.jpg) |
| `224 R`, `240 R` (640 dots) | 37/N/2/4 | - | 223, 239 | [240 R](m104-photos/va2-240-r.jpg) |
| `240 R W N` | 37/240/2/4, `HAD` 127 | - | 239, both window marks inside the picture | [240 R W N](m104-photos/va2-240-r-w-n.jpg) |
| `240 T W`, `240 U W` | 32 and 16 top | - | about 234 and 218, as in 12.1 | - |

Findings:

- **24.8 kHz, 400-line graphics mode past line 400.** With `S` the
  graphics frame buffer is shown to its last line: lines 400-479 appear
  below the red 400-line edge, up to 480 lines (`480 S K`). The monitor
  read-outs agree with the frame totals computed from the `SYNC` vectors
  (443 lines: 56.0 Hz; 467: 53.2 Hz; 499: 49.75 Hz; 503: 49.4 Hz).
- **15.98 kHz with the ROM's top blanking (37).** Every run from 200 to 240
  lines shows all lines, at 320 and 640 dots, which confirms the rule
  `TBL + VAD - 37` at `TBL = 37`. Above 240 (`244 R W`, `248 R W`) the
  frame is accepted but nothing past line 239 is visible; the monitor
  reported these inputs as 240-line pictures (1740 x 240), so the cut is
  its picture window, as with `408 S`-`424 S` at 24.8 kHz. Whether the VA
  outputs lines 240-247 needs a monitor that shows the whole frame.
- **24.8 kHz line doubling** stops after line 200 also with top blanking
  17 (`232 D K`, `240 D K`), as in 12.1.
- **Monitor limits** (this monitor only): 24.8 kHz frames of 503 lines
  (49.4 Hz) accepted and of 507 and 511 lines rejected; at 15.98 kHz 291
  lines (54.9 Hz) accepted and 294 rejected (12.1).
- **G160 rates** (redraws over the calendar-clock interval; the values are
  binary, the clock had not been set):

| Depth | 24.8 kHz | 15.98 kHz |
|---|---|---|
| 16 bits | 39 in 27 s (1.4/s) [photo](m104-photos/va2-g160-24k-16.jpg) | 42 in 29 s (1.4/s) [photo](m104-photos/va2-g160-15k-16-r2.jpg) |
| 8 bits | 40 in 25 s (1.6/s) [photo](m104-photos/va2-g160-24k-8.jpg) | 41 in 26 s (1.6/s) [photo](m104-photos/va2-g160-15k-8-r2.jpg) |
| 4 bits | 246 in 127 s (1.9/s) [photo](m104-photos/va2-g160-24k-4.jpg) | 68 in 35 s (1.9/s) [photo](m104-photos/va2-g160-15k-4-r2.jpg) |

  The 15.98 kHz column is a repeat run (2026-10-07, later the same
  morning); the first 15.98 kHz run (104 redraws in 68-83 s, 40 in 26 s, 81)
  had the end times partly hidden ([16](m104-photos/va2-g160-15k-16.jpg),
  [8](m104-photos/va2-g160-15k-8.jpg), [4](m104-photos/va2-g160-15k-4.jpg)).

  vaeg ran the same program at 3.0, 3.4 and 4.25 redraws per second (16, 8
  and 4 bits), about 2.1-2.2 times the PC-88VA2 at every depth. The rate
  does not depend on the line frequency on the hardware. `G256 A` matched
  12.3 at both frequencies ([15 kHz](m104-photos/va2-g256-15k-16-r2.jpg)).

### 12.5 vaeg model

vaeg implements the measured rules (M104,
[631e3e98](https://github.com/nakatamaho/vaeg/commit/631e3e980a3883cd7b7a950a612ad206b9a79077),
[ee21db2a](https://github.com/nakatamaho/vaeg/commit/ee21db2a2c953c8319d5fb8c68290401eaf0905d)):

- The SDL canvas has as many rows as the frame's rasters, 400 to 480; the
  frame buffer is shown to its last line (24.8 kHz `S` runs, `480 S K`).
- 24.8 kHz, 200-line graphics with RSM = 01: graphics lines 0-200 are
  read, then line 200 repeats (`D` runs).
- 15.98 kHz: graphics line 0 is shown 37 lines after vertical sync. When
  `TBL + TBR` is under 37 it is delayed by the difference, so
  `TBL + VAD - 37` lines are shown; when it is over 37 that many lines at
  the top are hidden (12.6). The fixed start, chosen as `[POLICY]` before
  12.6 because 12.1 could not separate it from a short count, is supported
  by 12.6: a longer top hides the top of the picture.
- 24.8 kHz, 200-line graphics with RSM = 00: lines 0-200 are shown, the
  rest is blank (12.6) ([2cfa871a](https://github.com/nakatamaho/vaeg/commit/2cfa871ae32e0a6e35f56f2ba87279bdaf210e99)).
- The TSP text stops at SYNC until DSPON, which is why the V480PAT
  photographs show no PC-Engine text ([bf55c229](https://github.com/nakatamaho/vaeg/commit/bf55c2292e40dda45b9ddb839bec84b118f0f74a)).
- Not modelled: the frame buffer's first lines that the PC-88VA2 shows
  below the last line when the top is over 37 and `VAD` is 224 or more
  (12.6; this may mean `DSH` does not end the picture), the 15.73 kHz family
  (12.6: inconsistent), and anything past line 239 at 15.98 kHz (12.4:
  hidden by this monitor). The monitor's own limits are not emulated; vaeg
  shows every frame.

### 12.6 Fourth round: long top blanking, 15.73 kHz family, RSM = 00

`[MEAS]` 2026-10-07, PC-88VA2, same monitor, `V480PAT C` (15 kHz) and
`E` (24 kHz) built from
[`c5ef4e9d`](https://github.com/nakatamaho/vaeg/commit/c5ef4e9d718faa9c7d2647c3b72c18f08efbca4b).

| Run | Frame (top/active/bottom/sync) | Result | Photo |
|---|---|---|---|
| `200 D Z` | 25/400/7/8 | each line on the even raster, odd rasters blank; monitor 640x400, 56.4 Hz | [200 D Z](m104-photos/va2-200-d-z-osd.jpg) |
| `232 D Z`, `240 D K Z` | 25/464/2/4, 17/480/2/4 | lines 0-199 as above, line 200 (red) once, then black to the end; monitor 640x480, 50.1 and 49.3 Hz | [232 D Z](m104-photos/va2-232-d-z-osd.jpg) |
| `200 H37 W`-`200 H53 W` | 37-53/200/.../4 (262) | the label at the top loses about `TBL - 37` lines (H53: all but its last row); nothing below line 199 | [H45](m104-photos/va2-200-h45-w.jpg), [H53](m104-photos/va2-200-h53-w.jpg) |
| `224 H45 W`, `240 H41 W`, `240 H45 W` | 45/224, 41/240, 45/240 | top cut as above; below the last line one dark line and then the frame buffer's first lines (the blue band and the top of the label), about `TBL - 37` lines in all | [224 H45](m104-photos/va2-224-h45-w.jpg), [240 H41](m104-photos/va2-240-h41-w.jpg), [240 H45](m104-photos/va2-240-h45-w.jpg) |
| `200 I W` | 15.73 kHz vector, RSM = 10 | all 200 lines | [200 I W](m104-photos/va2-200-i-w.jpg) |
| `224 I W` | as above, 224 | the monitor kept re-adjusting; the picture ghosted | [224 I W](m104-photos/va2-224-i-w.jpg) |
| `240 I W`, `240 I` | top 36, 240 | to about line 217 | [240 I W](m104-photos/va2-240-i-w.jpg), [240 I](m104-photos/va2-240-i.jpg) |
| `240 I W H37` | top 37, 240 | to about line 199 | [240 I W H37](m104-photos/va2-240-i-w-h37.jpg) |

Findings:

- **RSM = 00 past line 200** (24.8 kHz): the line address stops after
  line 200 as with RSM = 01, but the rasters after it are blank instead of
  repeating line 200. `[DERIVED]` Consistent with the doubling logic
  treating every raster past the stop as an odd raster: repeated in mode 1,
  blank in mode 0.
- **Top blanking over 37** (15.98 kHz): the top of the picture is hidden by
  about `TBL - 37` lines, so graphics line 0 is tied to vertical sync, not
  to the TSP's active area. With `VAD` 224 or 240 the same number of lines
  appear below the last line, wrapped to the frame buffer's start
  (V480PAT sets `FBL = DSH = VAD`), so the display does not stop at `DSH`
  there; with `VAD` 200 nothing appears below line 199. Not separated
  further.
- **15.73 kHz family** (external sync, interlaced RSM): results are not
  consistent with one rule (217 vs 199 lines with top 36 vs 37) and the
  monitor re-adjusted; not modelled.

### 12.7 15 kHz scanline fill (M105)

`[POLICY]` At 15 kHz (non-interlaced) vaeg draws each line on the even
raster of a pair. The odd raster is a gap by default, as a CRT shows its
scanlines; the monitor setting "15 kHz 走査線: 埋める (液晶)"
(`Monitor15kHzFill`) repeats the line there instead, as the maintainer's
LCD showed the M104 photographs. Interlaced modes and 24.8 kHz are
unchanged. Implemented in
[be71480c](https://github.com/nakatamaho/vaeg/commit/be71480cb438b9c368d8364c5d58f92bd416eb28).

### 12.8 Frames near 60 Hz (M105)

`[MEAS]` 2026-10-07, PC-88VA2, same monitor, V480PAT `F`, `G` and `J`
built from
[`336c449f`](https://github.com/nakatamaho/vaeg/commit/336c449fa6e982bd28258161b3aa586b10fbfabc).
Every entry was displayed. The monitor's read-out is compared with the
rate from the `SYNC` vector (vaeg line rates 24.826 and 15.980 kHz,
`VS` counted as written). `K` = top 17, `Q` = bottom 1 / sync 1,
`P` = bottom 2 / sync 4.

| Run | Frame lines | Computed | Monitor | Photo |
|---|---:|---:|---:|---|
| `400 S` | 440 | 56.42 Hz | 56.6 Hz | - |
| `400 S Q` | 427 | 58.14 | 58.1 | - |
| `400 S K Q`, `400 S W K Q` | 419 | 59.25 | 59.2 | [400 S K Q](m104-photos/va2-400-s-k-q-osd.jpg) |
| `400 S K P` | 423 | 58.69 | 58.6 | - |
| `396 S K Q`, `392 S K P` | 415 | 59.82 | 60.0, 59.8 | - |
| `394 S K Q` | 413 | 60.11 | 60.0 | [394 S K Q](m104-photos/va2-394-s-k-q-osd.jpg) |
| `392 S K Q` | 411 | 60.40 | 60.3 | - |
| `384 S K Q` | 403 | 61.60 | 61.5 | - |
| `200 D` | 440 | 56.42 | 56.4 | - |
| `200 D K Q` | 419 | 59.25 | 59.2 | [200 D K Q](m104-photos/va2-200-d-k-q-osd.jpg) |
| `200 D K P` | 423 | 58.69 | 58.6 | - |
| `198 D K Q` | 415 | 59.82 | 59.8 | - |
| `197 D K Q` | 413 | 60.11 | 60.0 | [197 D K Q](m104-photos/va2-197-d-k-q-osd.jpg) |
| `196 D K Q` | 411 | 60.40 | 60.3 | - |
| `192 D K Q` | 403 | 61.60 | 61.5 | - |
| `200 R W` | 262 | 60.99 | 61.1 | - |
| `220 R W` | 263 | 60.76 | 60.8 | - |
| `222 R W` | 265 | 60.30 | 60.4 | - |
| `224 R W`, `224 R` | 267 | 59.85 | 59.9 | [224 R W](m104-photos/va2-224-r-w-osd.jpg) |
| `224 H36 W` | 266 | 60.07 | 60.2 | [224 H36 W](m104-photos/va2-224-h36-w-osd.jpg) |

Findings:

- The read-outs agree with the computed rates within the monitor's
  0.1-0.2 Hz rounding, with `VS = 1` counted as one line (a 4-line minimum
  would give 58.83 Hz for `400 S K Q` against the 59.2 Hz read). This
  confirms the M105 short-sync fix.
- 60 Hz is within reach in every family: 640 x 394 or 320 x 197
  line-doubled at 24.8 kHz (60.1 Hz), 640 x 400 / 320 x 200 doubled at
  59.25 Hz with top 17, bottom 1, sync 1, and 320/640 x 224 at 15.98 kHz
  (59.9 Hz). The 24.8 kHz runs were read by the monitor as 640 x 400 and
  the 15.98 kHz runs as 1740 x 240.

## 13. Change log

### Version 0.3 - 2026-10-06

- Added the M104 measurements of non-native line counts and line doubling
  (section 12), the second round with the line ruler (12.1), and the
  feasible-resolution table (12.2), the G256 256 x 192 window (12.3), and
  the third round with the sweeps, monitor read-out and G160 rates (12.4),
  the vaeg model of the measured rules (12.5), and the fourth round with
  long top blanking, the 15.73 kHz family and RSM = 00 (12.6), the 15 kHz
  scanline fill setting (12.7) and the frames near 60 Hz (12.8).

### Version 0.2 - 2026-07-16

- Decoded all twelve PC-88VA `SYNC` vectors into raw `RM`, clock/sync, active,
  blanking, border, and synchronization fields.
- Reduced the vectors to three horizontal timing families and derived their
  current vaeg horizontal and field rates.
- Explained the 200/204 versus 400/408 logical/physical raster relationships.
- Corrected the 200/204-line 15.73kHz description: its raw TSP mode is
  `RM=11b` normal raster, not `RM=01b` interlace.
- Separated deductions supported by raw bytes from vaeg timing-model evidence
  and remaining real-hardware questions.

### Version 0.1 - 2026-07-16

- Separated TSP scan timing, `GRMODE`, `GRRES`, framebuffer descriptors, SGP
  pitch, and final composition.
- Recorded the generic uPD72022 `RS=000b` through `100b` examples without
  promoting them to PC-88VA system modes.
- Reconstructed ports `0100h`, `0102h`, and `0200h-027fh` from documentation
  and vaeg behavior.
- Added a candidate 320x200/4-bpp field relationship and safe hardware-test
  order.
- Defined 256x192 as generic-TSP capability only and changed the first
  384x256 experiment to padded 640-dot rows rather than an assumed native
  384-dot fetch.
