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


# M103e - V1/V2 compatibility against PC-8801 demonstration software

Status: **complete; G103e passed on 2026-10-04; merged to `main` at [1520c42](https://github.com/nakatamaho/vaeg/commit/1520c42994ce43f81b3d96d58af6aaf6b151d7e7)**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103e-v1v2-srdemo-sound` off
`topic/m103d-v1v2-graphics-disk` at `05d5c4e88d3392ed430bb80be325002647c00977`
(M103d, G103d pending; the maintainer asked to continue before the gate).
Commit prefix `M103e:`. Plan:
[`docs/modernization/v1v2-mode-plan.md`](../../modernization/v1v2-mode-plan.md) §7.

## Goal

The PC-8801mkIISR demonstration disk runs under V1/V2 mode with its
pictures, kanji text and FM music. The maintainer added two further
private targets during the session: a PC-8801MA2 demonstration disk
(2HD, V2 mode, started through N88-日本語BASIC) and a commercial game
disk. All three stay private and are referred to here as SR-DEMO,
MA2-DEMO and GAME-A.

## Sources

`[VA-TM]` BNN manual and the electronic technical manual; `[X88000]`
X88000 1.5.3 (public domain) as a behavioural reference for the PC-8801
side, re-implemented; `[ROM]` disassembly of the VA2 ROM images and
traces (local, not committed).

## Scope

1. **Kanji ROM ports** E8h/E9h (level 1) and ECh/EDh (level 2).
2. **Extended RAM** E2h/E3h and the PC-8801MA dictionary ROM window
   F0h/F1h, as far as the demonstration software needs them.
3. **Sound** under V1/V2: the OPN at 44h–47h and its interrupt.
4. **Compatibility fixes** found by running SR-DEMO, MA2-DEMO and GAME-A.
5. **V1 mode** boot, if V1 media is available.

## Gate G103e (human)

Standard V3 gate unchanged, plus with the VA2 ROM set: SR-DEMO shows its
title with kanji text, runs through its pictures after f・1 and plays
music (listen: the headless runs cannot check audio); MA2-DEMO boots
N88-日本語BASIC and its demonstration program runs; GAME-A, with its two
images in drives 1 and 2, runs to its title (the text grid is a known open
item).

## Summary for G103e

Machine-verifiable parts (local): full CTest without failures (108
entries, two skipped as before), the V2 BASIC acceptance test passes, the
tests-off `linux-release` preset builds, V3 PC-Engine boots and lists its
disk. New romless tests: kanji ROM ports, extended RAM, dictionary ROM
window, and D88 track tables that end inside the first track.

Policies for approval: the kanji ROM address conversion (bit 14 inverted
from byte offset 8000h, derived from the ROM's routine and the font
image); E2h/E3h following the PC-8801 port numbering against the VA
manuals' printed order; the F0h/F1h dictionary ROM window, which the VA
manuals do not document.

Moved to M103f: the text grid in GAME-A (3301 RESET and the byte-mode
start-address rule), choosing an image of a multi-image D88, and V1 mode
(no V1 media available here). Audible sound under V1/V2 is a gate item.

## Implementation progress

### SR-DEMO

- Without changes the disk boots through V2 BASIC into the demonstration
  and draws its title, but the menu text inside the cyan bars and boxes
  is missing: the program reads kanji patterns through E8h–EBh, which
  vaeg did not decode.
- `[VA-TM]` E8h/E9h (level 1) and ECh/EDh (level 2) take the low and
  high byte of a word address; IN of the even port returns the right and
  of the odd port the left half of a raster. `[ROM]` The VA2 extension
  ROM's BASIC (bank at 6000h, routine at 7290h) forms the address of a
  kanji as `((jis1 & 1Fh) | (jis2 & 60h)) << 9 | (jis2 & 1Fh) << 4`,
  and reads 16 rasters with `OUT E8h/E9h`, `OUT EAh`, `IN E9h`, `IN E8h`,
  `OUT EBh`. `[DERIVED]` The VA font image holds the same patterns with
  two bytes per raster; it differs from that order only in the sense of
  byte-offset bit 14 for kanji (offsets from 8000h). With that conversion
  the title reads "アナログRGBディスプレイなら f・1 を / デジタルRGB
  ディスプレイなら f・5 を押して下さい"; without it "押" and "下"
  came out as "臓" and "村".
- After f・1 the demonstration runs through its 512-colour pictures, the
  3D scene and the band animation. During the band scene it writes the
  OPN (2612 key-ons, frequency, level and SSG registers through 44h/45h);
  audible output was not checked (no audio capture in the headless run).

### MA2-DEMO

- The 2HD image boots to its system selection (N88-BASIC or
  N88-日本語BASIC). N88-BASIC reaches disk BASIC with an `Ok` prompt.
  N88-日本語BASIC, which the maintainer specified, first stopped with
  "カクチョウ メモリガ ヒツヨウデス" (extended memory required).
- `[VA-TM]` both manuals show E2h with page/bank fields (P1 P0 B1 B0) and
  E3h with WE (bit 4)/RE (bit 0). `[X88000]` and the MA2 loader (E3h = 03h,
  then E2h = 11h, then E2h = 00h) use E2h for RE/WE and E3h for the bank:
  the manuals' bit fields match the PC-8801 ports with the port numbers
  exchanged, so vaeg follows the PC-8801 numbering. `[NEC-GIHO]` and the
  BNN V1/V2 memory map put page p, bank b at VA main RAM
  `20000h + p * 20000h + b * 8000h`; page 3 needs the optional RAM board.
- The loader then reported "ジショROMガアリマセン" (no dictionary ROM). It
  selects bank 0 with F0h, maps it with F1h = 00h and checks for `44h 10h`
  at C000h, which is how the VA2 dictionary ROM image begins. F0h/F1h are
  not in the VA manuals and not in X88000 1.5.3. `[DERIVED]` vaeg maps a
  16 KiB bank of the dictionary ROM for reads at C000h–FFFFh while F1h
  bit 0 is clear (PC-8801MA behaviour).
- The loader then hung reading cylinder 4, head 1 (the screen still showed
  BASIC's earlier output). The FDC trace showed a 26-sector read ending
  after 17 sectors with No Data. The image has a 160-entry track table, so
  vaeg read track 0's first sector ID as a pointer (10000h) and cut that
  track short (fixed in `fdd/fdd_d88.c`, ledger entry).
- With that fix N88-日本語BASIC starts with its banner and free-memory
  line, and running the demonstration program from the disk loads and runs
  its title sequence (pictures and kanji text drawn by BASIC). The disk does not start
  the demonstration by itself in vaeg; whether it does on the real machine
  is not known.

### GAME-A

- The image holds two disks (D88 multi-image file). With only the first
  inserted, the game reads drive 2 (Not Ready) and then runs away. With
  the two images split into separate files in drives 1 and 2 (local
  copies only), it runs through its opening, the ship descriptions, a
  demonstration flight and its title.
- vaeg has no way to choose the second image of a multi-image D88; a
  frontend selection is left for a later task.
- Open: a grid of "▁♠" characters covers the screen. The game issues a
  3301 RESET (`51h = 00h`) to stop the text display; `[ROM]` the VA2 ROM
  answers with EXIT, writes start address 1000h to split screen 0 with
  8Eh/97h, and restarts EMUL. vaeg's byte-mode address rule from M103c
  (TVRAM byte = 3000h + start / 2, derived from BASIC's 678Ch ↔ 63C6h)
  maps 1000h to 3800h, where the ROM itself stores words 0080h/00E8h
  (`F000:2480`–`24A3` clears 3000h–3FFFh, then writes them with a 240-byte
  stride); vaeg shows them as characters 80h and E8h. Whether the rule is
  wrong for other start addresses or the real machine shows the same is
  not known; hiding the text layer removes the grid.
