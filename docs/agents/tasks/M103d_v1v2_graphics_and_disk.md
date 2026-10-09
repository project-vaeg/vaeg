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


# M103d - V1/V2 graphics and disk BASIC

Status: **complete; G103d passed on 2026-10-04; merged to `main` at [98a2333](https://github.com/nakatamaho/vaeg/commit/98a233335fc66c9e03849b51fc42adf0e3d7170b)**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103d-v1v2-graphics-disk`
off the integration branch `topic/v1v2-mode` at `main`
`c16ef9cf44f2bc0bbd9cf0f7b28a5ac30899fd71` (M103a–M103c merged). Commit
prefix `M103d:`. Plan and hardware model:
[`docs/modernization/v1v2-mode-plan.md`](../../modernization/v1v2-mode-plan.md).

## Goal

N88-DISK BASIC V2, booted automatically from V2 media as in M103b/M103c,
is usable with its graphics statements and disk commands: `LINE`,
`CIRCLE`, `PSET`, `PAINT`, `POINT`, `COLOR`, `SCREEN`, `CLS` draw and
display as on the real machine, and `FILES`, `SAVE`, `LOAD` and `KILL`
work on a writable image.

## Background (established before implementation)

Sources: `[VA-TM]` BNN manual (port map, ch. 4 display, ch. 8 V1/V2 mode)
and the electronic technical manual (`tekumani`, same register text);
`[X88000]` X88000 1.5.3 (public domain) as a behavioural reference for
PC-8801 semantics, re-implemented without copying code; `[ROM]` traces
and disassembly of the VA2 ROM (local, not committed).

- `[VA-TM]` Port 32h bit 6 GVAM selects independent (0) or extended (1)
  GVRAM access and is forced to 0 in V3 memory mode. Ports 34h and 35h
  are documented as OUT-only and V1/V2-only: 34h holds a two-bit ALU
  operation per plane (00 reset, 01 set, 10 invert, 11 no operation;
  plane n uses bits n and n+4); 35h bit 7 GAM maps GVRAM (1) or main RAM
  (0), bits 5–4 GDM select the write data (ALU output, read data, plane 1
  → plane 0, plane 0 → plane 1) and bits 2–0 the read comparison data.
- `[VA-TM]` §8.3: V1/V2 uses multiplane mode, graphics screen 0 only,
  640 dots, 200 lines with a 15 kHz CRT and 400 lines with 24.8 kHz; each
  plane's 16 KiB is A4000h–A7FFFh of V3 plane n, mapped at 1C000h–1FFFFh.
- `[VA-TM]` Port 31h (OUT, V1/V2 only): bit 4 PM00 (1 or 3 bits/pixel),
  bit 3 GDEN0 (graphics display enable), bit 2 RMODE, bit 1 MMODE, bit 0
  VW1 (400 or 200 lines). PM00, GDEN0 and VW1 are the names of GrRes
  (102h) bit 0 and GrMode (100h) bits 15 and 1.
- `[VA-TM]` 10Ch PLTM2 = 0 selects the palette mode from 32h PMODE and
  31h PM00 (0/0 → mode 1, 0/1 → mode 2, 1/0 → mode 0, 1/1 → mode 2);
  §8.4.2 lists the same four V1/V2 cases. 110h bit 6 88MD selects the
  graphics display circuit's V1/V2 mode and bits 3–0 GnSW the planes of
  multiplane 1 bit/pixel mode.
- `[VA-TM]` ch. 4 §2.1: in multiplane 1 bit/pixel mode the OR of the
  switched-on planes is merged into the text screen's character-generator
  data, treated as text and decorated by the text attributes
  (pixel = P0 | P1 | P2 | P3 | text). `[X88000]` shows the same PC-8801
  monochrome rule (graphics dots take the text cell's colour and reverse).
- `[VA-TM]` §8.4: palettes (54h–5Bh) and the background colour (52h) are
  emulated by the ROM through the I/O trap; the 8801 background colour
  becomes the VA backdrop colour.

## Scope

1. **Extended GVRAM access.** GVAM (32h bit 6, read back in 88 mode),
   34h/35h, ALU writes, latch and comparison reads, GAM mapping of
   C000h–FFFFh, and its interaction with the independent planes and the
   88-mode TVRAM window. Saved state.
2. **Graphics display.** Port 31h controlling the VA display bits it
   names, the palette mode chosen by PLTM2 = 0, colour (3 bits/pixel)
   display at 200 lines, monochrome (1 bit/pixel) display merged with
   text, and 400-line monochrome where the machine provides it.
3. **Disk BASIC.** `FILES`, `SAVE`, `LOAD`, `KILL` under V1/V2 on a
   scratch writable image; fix only what fails.
4. **Tests.** Romless unit tests for the ALU and the port 31h/PLTM2
   behaviour; the private-media acceptance test extended to a graphics
   statement and a disk round trip.

Out of scope: sound under V1/V2, V1 mode, PC-8801mkIISR DEMO (M103e);
3301 semigraphics, 40-column mode, kanji ROM ports, ERAM (unless BASIC
requires them); timing accuracy (GVRAM wait states).

## Gate G103d (human)

Standard V3 gate unchanged, plus with the VA2 ROM set and V2 BASIC
media: `LINE`, `CIRCLE`, `PAINT` and `COLOR` draw in colour, `SCREEN 1`
(640x200) and `SCREEN 2` (640x400) show monochrome graphics, and on a
writable copy of the media a program survives `SAVE`, `NEW`, `LOAD` and
`RUN`.

## Implementation progress

### Extended access (scope item 1)

- Before the change, `LINE` in V2 BASIC wrote into the text screen and
  `CIRCLE` restarted BASIC. `[ROM]` BASIC's plotting code sets GVAM
  (`IN A,(32h)` / `SET 6,A` / `OUT (32h),A`), selects ALU operations with
  34h, and writes each byte with `OUT (C),B` to 35h = 80h, the store, and
  35h = 00h, so outside those writes C000h–FFFFh stays main RAM (BASIC's
  stack is at E5xxh).
- `[ROM]` The VA2 ROM's text trap handler (`F000:1844`, `F000:1CE5`) saves
  32h, 34h, 35h and 5Ch with `IN`, sets port 152h bit 14 for its V3-mode
  work, and restores them with `OUT` (`F000:18A6`). vaeg returned FFh for
  34h/35h, so the restore set GAM; the next `LINE` then turned BASIC's
  stack into GVRAM. `[DERIVED]` 34h and 35h read back the last written
  value although the manuals list them as output ports.
- After the change, the GVRAM dump after `LINE`, `CIRCLE` and
  `LINE …,BF` shows the figures in the expected colours, and `POINT`
  returns the painted colours (comparison read).

### Display (scope item 2)

- `[ROM]` The ROM sets the VA display for V1/V2 once (100h = 3062h with
  GDEN0 clear, 102h = 0001h, 106h = 0A89h, 10Ch = 0000h, 110h = 7F47h,
  frame buffer 0 at 10000h with 140h-byte pitch, 200 lines); BASIC then
  only writes 31h (19h in colour mode). Treating 31h's PM00, GDEN0 and
  VW1 as aliases of the VA bits displays colour graphics.
- `[ROM]` V3 BASIC and PC-Engine write 10Ch = 0180h (PLTM2 = 1); only the
  V1/V2 setup uses PLTM2 = 0.
- `COLOR f,b` in colour mode: the ROM writes the backdrop colour, and
  graphics colour 0 stays opaque, so the background stays black as in
  `[X88000]`'s colour mode.
- The palette mode follows 10Ch PLTM2: with it clear, 32h PMODE and 31h
  PM00 select the mode as tabulated in the manual. Colour mode is then
  palette mode 2 (text from palette set 1, graphics from set 0).
- `SCREEN 1` selects 1 bit/pixel (31h PM00 = 0) with 110h G0SW–G2SW on.
  vaeg drew nothing in multiplane 1 bit/pixel mode; the graphics raster now
  holds the OR of the switched-on planes, the text renderer records each
  cell's colour after the reverse attribute (before secret and blink), and
  the compositor shows set dots in that colour instead of a separate
  graphics screen. `LINE` and `CIRCLE` in `SCREEN 1` appear in the text
  colour. Blank text frames are no longer skipped while this merge is
  active. Not covered: 320-dot horizontal mode.
- `SCREEN 2` (vaeg's fixed DIP switch selects a 24.8 kHz monitor): BASIC
  writes 31h = 08h (1 bit/pixel, 400 lines) and the ROM only switches on
  planes 0 and 1 (110h = 7F43h); frame buffer 0 stays at 200 lines.
  `[DERIVED]` With 88MD set the display circuit must show plane 0 as the
  upper and plane 1 as the lower 200 lines (the PC-8801 640x400 format, as
  in `[X88000]`); vaeg showed both planes ORed in the upper half and
  nothing below. `LINE (0,0)-(639,399)` and a 150-dot circle now span the
  full screen.

### Disk BASIC (scope item 3)

- Without changes, `FILES` lists the media and, on a writable copy,
  `SAVE`, `NEW`, `LOAD`, `LIST` and `RUN` round-trip a program. The V2
  media itself carries the D88 write-protect flag, so `SAVE` on it
  reports `File write protected`, as it should. `KILL` removes a saved
  file (`LOAD` then reports `File not found`).

### Tests (scope item 4)

- `vaeg_romless_tests`: `test_v1v2_graphics` covers independent and ALU
  writes, the comparison read, the three latch-copy modes, 34h/35h read
  back, GAM switching between GVRAM, RAM and the TVRAM window, port 31h
  (88 mode only), the PLTM2-clear palette modes, the merge condition, the
  1 bit/pixel plane OR and the 88MD 400-line plane layout;
  the V1/V2 state-section test covers `MEM88ALU`. Removing the 34h
  read-back, the 31h alias, the PLTM2 rule or the comparison makes it fail.
- `vaeg_m103b_basic_boot` (private ROMs and media): after `PRINT 1+1` it
  draws `CIRCLE (320,100),60,2:PAINT (320,100),4,2` and counts red and
  green pixels in the rendered BMP (measured about 600 and 11 000), then
  `SAVE`, `NEW`, `LOAD` and `RUN` of a one-line program on the temporary
  copy with its write-protect flag cleared. Stable codes
  `M103D_NO_DISK_ROUNDTRIP`, `M103D_NO_SCREENSHOT`, `M103D_NO_GRAPHICS`
  with one-mutation selftest cases. A run takes about 160 s.
