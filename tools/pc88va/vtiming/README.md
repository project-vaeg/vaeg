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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# V480PAT: PC-88VA display-timing test pattern

`V480PAT.COM` is a PC-Engine (V3 mode) program that draws a 16-colour test
pattern in graphics screen 0 and can switch the display to non-standard line
counts by sending its own TSP `SYNC` vector. It was written for vaeg M104 to
measure, on real hardware, which display timings the PC-88VA and a monitor
accept, and how the graphics fetch behaves outside the documented 200/204/
400/408-line modes. The source, [`v480pat.asm`](v480pat.asm), is distributed
under the repository's two-clause BSD terms.

It is an independent program. It complements, but does not contain,
[VIEW480](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=404)
(mami, 1992), which shows a 640x480 picture by reprogramming the TSP for 480
lines; VIEW480 is not redistributed here. The development-disk builder
builds `VIEW480.COM` from its public source archive (see
[`pc88va-utility-media.md`](../../../docs/modernization/pc88va-utility-media.md)).

> [!WARNING]
> Non-standard `SYNC` timings can be out of range for a monitor. Fixed-
> frequency CRTs in particular should not be left driven by an out-of-range
> signal. Every switching option restores the previous timing on any key;
> if the screen is lost, press a key or switch the machine off.

## Binary

[`bin/v480pat.com`](bin/v480pat.com) is the program run on real hardware
for the second round of measurements, published as a proof of concept; it
predates the `R` option. It is
assembled from [`v480pat.asm`](v480pat.asm) at commit
[`290d976c`](https://github.com/nakatamaho/vaeg/commit/290d976c48f553b7256d82fa4e41edc61c0c5c9d)
with NASM 2.16.01 (`nasm -f bin`) and is byte-identical to the CMake build of
that source: 2,008 bytes, SHA-256
`553ffd5fa87365be91980cadd310daa87425c5475541c1faf9dbaa417ca2f59c`. Copy it
to a PC-Engine disk as `V480PAT.COM`. The same two-clause BSD terms apply.

## Building

The CMake build assembles it with NASM to `build/<preset>/guest/v480pat.com`;
the source is restricted to 8086/80186 instructions (`cpu 186`), so an
assembler choice that would emit 80386 forms fails the build. A PC-Engine 1.1
test disk is made from the user's own system disk:

```sh
tools/pc88va/build-vtiming-disk.sh \
    --source /path/to/pc-engine-1.1-system.d88 \
    --output /path/to/vtiming.d88 \
    [--view480 /path/to/VIEW480.COM]
```

The system disk is private media; keep both images outside Git.

## Usage

```text
V480PAT [lines] [S|T|U|R|D|I] [Hnn] [W] [N] [K] [Z] [Q|P]
V480PAT A|B|C|E|F|G|J
```

| Argument | Effect |
|---|---|
| `lines` | 1-480 (default 480). Graphics screen 0 is set up through the graphics BIOS (`INT 8Fh`) as single-plane 4 bits/pixel, 640 dots, with a frame buffer `lines` high, and the pattern is drawn directly into GVRAM. |
| none of S/T/U/D | The pattern is left on the normal screen (400 lines visible), for VIEW480 or a capture. |
| `S` | 24.8 kHz: VIEW480's sequence with `VAD = lines`; above 400 lines the bottom blanking and sync become 1 and 1 (VIEW480's values), below that the ROM's 7 and 8 stay. The frame grows or shrinks with `lines`. |
| `T` | 15.98 kHz, monitor switch at 15 kHz. Graphics in 200-line mode (one raster per line). `lines` 4-256, rounded down to even. Top blanking 32, sync 4, bottom blanking at least 2 and grown to keep a frame of at least 262 lines: the uPD72022 data-book minimums with sprites. |
| `U` | As `T` with top blanking 16 (the data-book minimum without sprites). |
| `R` | As `T` with top blanking 37, the ROM's 15.98 kHz value; by the measured rule graphics then show all `lines` (224: 267-line frame, 59.9 Hz; 240: 283 lines, 56.5 Hz). |
| `Hnn` | As `R` with top blanking `nn` (1-63), for values above 37 that the other options do not reach. |
| `I` | 15.73 kHz family, monitor switch at 15 kHz: as `R` but starting from the ROM's 15.73 kHz 200-line vector (`C1 47 1C 00 9F 00 12 11 24 00 C8 00 17 04`: top 36, external sync) with port `0100h` RSM = 10 (interlaced mode 0), the pairing the ROM uses for that family. `Hnn` after `I` changes the top. |
| `D` | 24.8 kHz, 320 x `lines` (1-240) line-doubled: graphics in 200-line mode with port `0100h` RSM = 01 (non-interlaced mode 1), the TSP frame at 2 x `lines` rasters; above 400 rasters bottom blanking 2 and sync 4. |
| `W` | Graphics screen 0 at 320 dots (`0102h` bit 4) with `S`, `T`, `U` or none; `D` is always 320. |
| `N` | With `S`, `T`, `U` or `D`: TSP horizontal active `HAD` 159 -> 127 (128 TCK: 256 dots at 320, 512 at 640) and 16 TCK added to each of `LBR` and `RBR`, keeping the line length. Two white two-dot marks show the edges of that window. |
| `Q` | With `S` or `D`: bottom blanking 1 and sync 1 at any line count (otherwise only above 400 lines). |
| `P` | With `S` or `D`: bottom blanking 2 and sync 4 (the data-book minimums). With `K`, `Q` and `P` shorten the 24.8 kHz frame towards 60 Hz. |
| `Z` | With `D`: RSM = 00 (non-interlaced mode 0, odd rasters blank) instead of 01. |
| `K` | With `S` or `D`: top blanking 17 instead of the ROM's 25 (480 lines with `S`: 499-line frame, about 49.8 Hz). |

Any key restores the timing (and RSM for `D`). The graphics mode chosen for
the pattern is left in place, so after `D` or `T` the pattern stays in
200-line mode.

`A` and `B` run a fixed list of argument sets one after another, so a
series needs no typing: each entry runs exactly as if typed after
`V480PAT` (the label shows it), any key restores the timing and goes on to
the next entry, and ESC stops. If the monitor shows nothing for an entry,
a key still moves on; the entries are numbered below to keep count.

| List | Monitor switch | Entries, in order |
|---|---|---|
| `A` | 24 kHz | 1 `400 S`, 2 `408 S`, 3 `416 S`, 4 `420 S`, 5 `424 S`, 6 `432 S`, 7 `440 S`, 8 `448 S`, 9 `456 S`, 10 `464 S`, 11 `472 S`, 12 `476 S`, 13 `480 S`, 14 `440 S W`, 15 `440 S N`, 16 `480 S W`, 17 `464 S K`, 18 `472 S K`, 19 `480 S K`, 20 `200 D`, 21 `240 D`, 22 `232 D K`, 23 `240 D K` |
| `B` | 15 kHz | 1 `200 R W`, 2 `208 R W`, 3 `216 R W`, 4 `224 R W`, 5 `232 R W`, 6 `236 R W`, 7 `240 R W`, 8 `244 R W`, 9 `248 R W`, 10 `224 R`, 11 `240 R`, 12 `240 R W N`, 13 `240 T W`, 14 `240 U W` |
| `C` | 15 kHz | 1 `200 H37 W`, 2 `200 H41 W`, 3 `200 H45 W`, 4 `200 H53 W`, 5 `224 H45 W`, 6 `240 H41 W`, 7 `240 H45 W`, 8 `200 I W`, 9 `224 I W`, 10 `240 I W`, 11 `240 I W H37`, 12 `240 I` |
| `E` | 24 kHz | 1 `200 D`, 2 `200 D Z`, 3 `232 D Z`, 4 `240 D K Z` |
| `F` | 24 kHz | 1 `400 S` (440 lines, 56.42 Hz), 2 `400 S Q` (427, 58.14), 3 `400 S K Q` (419, 59.25), 4 `400 S K P` (423, 58.69), 5 `400 S W K Q` (419, 59.25), 6 `396 S K Q` (415, 59.82), 7 `394 S K Q` (413, 60.11), 8 `392 S K Q` (411, 60.40), 9 `392 S K P` (415, 59.82), 10 `384 S K Q` (403, 61.60) |
| `G` | 24 kHz | 320-dot line-doubled: 1 `200 D` (440, 56.42), 2 `200 D K Q` (419, 59.25), 3 `200 D K P` (423, 58.69), 4 `198 D K Q` (415, 59.82), 5 `197 D K Q` (413, 60.11), 6 `196 D K Q` (411, 60.40), 7 `192 D K Q` (403, 61.60) |
| `J` | 15 kHz | 320 dots: 1 `200 R W` (262, 60.99), 2 `220 R W` (263, 60.76), 3 `222 R W` (265, 60.30), 4 `224 R W` (267, 59.85), 5 `224 H36 W` (266, 60.07; one line lost at the top by the 37-line rule), 6 `224 R` (640 dots, 267, 59.85) |

`C` measures top blankings above 37 at 15.98 kHz (the graphics window
for `TBL > 37` is not modelled) and the 15.73 kHz family; `E` measures
line doubling with RSM = 00; `F`, `G` and `J` measure 640- and 320-dot frames near 60 Hz
(vaeg's line rate; the rates are derived). These conditions were not reached by `A` and
`B`, which is why vaeg does not model them yet.

## Timings

Frame totals follow `TBL + TBR + VAD + BBR + BBL + VS`; rates use vaeg's
dot clocks (24.83 kHz and 15.98 kHz lines) and are derived values, not
measurements.

| Command | Lines (top / active / bottom / sync) | Frame | Vertical rate |
|---|---|---:|---:|
| normal 400-line screen | 25 / 400 / 7 / 8 | 440 | 56.4 Hz |
| `VIEW480` | 25 / 480 / 1 / 1 | 507 | 49.0 Hz |
| `V480PAT 240 S` | 25 / 240 / 7 / 8 | 280 | 88.7 Hz |
| `V480PAT 232 D` | 25 / 464 rasters / 2 / 4 | 495 | 50.2 Hz |
| `V480PAT 472 S` | 25 / 472 / 1 / 1 | 499 | 49.8 Hz |
| `V480PAT 476 S` | 25 / 476 / 1 / 1 | 503 | 49.4 Hz |
| `V480PAT 480 S K` | 17 / 480 / 1 / 1 | 499 | 49.8 Hz |
| `V480PAT 240 D K` | 17 / 480 rasters / 2 / 4 | 503 | 49.4 Hz |
| `V480PAT 240 D` | 25 / 480 rasters / 2 / 4 | 511 | 48.6 Hz |
| normal 15 kHz 200-line screen | 37 / 200 / 15 / 8 | 260 | 61.5 Hz |
| `V480PAT 240 T W` | 32 / 240 / 2 / 4 | 278 | 57.5 Hz |
| `V480PAT 240 U W` | 16 / 240 / 2 / 4 | 262 | 61.0 Hz |
| `V480PAT 224 U W` | 16 / 224 / 18 / 4 | 262 | 61.0 Hz |
| `V480PAT 224 R W` | 37 / 224 / 2 / 4 | 267 | 59.9 Hz |
| `V480PAT 232 R W` | 37 / 232 / 2 / 4 | 275 | 58.1 Hz |
| `V480PAT 240 R W` | 37 / 240 / 2 / 4 | 283 | 56.5 Hz |

## Reading the pattern

- Left half: 40-line colour bands (colours 1-14 repeating). Right half:
  sixteen vertical bars, colours 0-15.
- Top left: the command line (`V480PAT` and its arguments, upper case) in
  white on black, drawn into graphics with a built-in 5x7 font at 2x2 dots,
  so photographs identify the run even when the TSP text is disturbed.
- White full-width lines every 100 lines and on the last line; red lines at
  lines 399/400, or 199/200 at 320 dots.
- Ruler: from line 192 down to the larger of `lines` and 264, a bar at the
  left edge. GVRAM is cleared that far first, so the ruler below the last
  line shows how far the display reads graphics past it. Its length is
  `(y mod 8) + 1` steps (8 dots at 640, 4 at 320); its colour gives
  `y div 8`:

| Colour | Lines | | Colour | Lines |
|---|---|---|---|---|
| white | 192-199, 256-263 | | cyan | 224-231 |
| red | 200-207, 264-271 | | blue | 232-239 |
| yellow | 208-215 | | magenta | 240-247 |
| green | 216-223 | | light grey | 248-255 |

  The pattern repeats every 64 lines (480-line runs continue it). The last
  visible line is the last bar's colour and length; for example a green bar
  four steps long is line 219.

## TSPMODE: undocumented TSP screen modes

[`tspmode.asm`](tspmode.asm) builds `TSPMODE.COM` (same terms), also put on
the test disk by `build-vtiming-disk.sh`. The PC-88VA technical manual
states that the TSP itself has text, sprite, semigraphics, graphics and
uPD3301-emulation functions but that the VA uses only text, sprites and
the emulation; it documents attribute `MODE` values 0-5 in bits 4:0 of
word `0Ah` of each screen-table entry. The uPD72022 data sheet describes
semigraphics (video memory read as pattern codes selecting pattern data
with format and colour) and graphics (video memory shown directly as
colour) static-picture modes, without the selection encoding in the
material at hand.

```text
TSPMODE [first]
```

It writes character codes 00h-FFh as a 16 x 16 block (attribute F0h) into
the main split, sets that split's `MODE` to `first` (default 0) and steps
it up to 31 on each key; repeats of a held key are dropped, ESC stops. The
current value is shown as `MODE nn` at the left of the function-key row.
The display timing is not changed, and the TVRAM cells and `MODE` words it
touched are restored on exit. Values 0-5 are the documented attribute
modes; anything other than characters (patterns, colour fields, dots) at
6-31 would point at an undocumented static-picture mode.

## TSPFILL: known data under the undocumented modes

[`tspfill.asm`](tspfill.asm) builds `TSPFILL.COM` (same terms), also put on
the test disk.

```text
TSPFILL [mode]
TSPFILL A
```

`MODE` 8-14 (and 24-30) replace the whole main split by a non-character
pattern on the PC-88VA2. TSPFILL sets split 0's `MODE` to `mode` (default
8) and fills TVRAM from split 0's start address up to the screen table
(`7F00h`), and the same range of the attribute area (`+8000h`), with known
data; each key steps to the next fill, ESC stops, and `MODE nn Pn` is shown
at the left of the function-key row. For a byte at TVRAM offset `o`
(character and attribute areas alike unless noted):

| Fill | Data |
|---|---|
| `P0` | 00h |
| `P1` | FFh |
| `P2` | characters FFh, attributes 00h |
| `P3` | characters 00h, attributes FFh |
| `P4` | `o and FFh` (byte ramp) |
| `P5` | `(o shr 4) and FFh` (changes every 16 bytes) |
| `P6` | `(o shr 8) and FFh` (changes every 256 bytes) |
| `P7` | nibbles 0, 1, ..., F repeated, high nibble first |

If the pattern follows the fills, the TSP reads this TVRAM in these modes:
uniform screens for `P0`-`P3` tell which area it reads, the stripe and band
widths of `P4`-`P7` give the bytes per dot, per raster and per row. The
filled range and the `MODE` word are saved in a 64 KiB block (INT 21h
function 48h) and restored on exit; the timing is not changed. vaeg draws
these modes as text, so its screen is not a reference.

`A` runs unattended, for filming: `MODE` 8, 12, 14, 10 and 13 in turn,
each with `P0`-`P7` held for about 3 seconds (170 frames after each fill,
which itself takes a moment), then it restores TVRAM and exits by itself.
Any key stops it early. The label may be unreadable in some modes; the
order is fixed, so the step can be counted from the start of `P0`.

## G160: 160 x 100 with a colour per dot

[`g160.asm`](g160.asm) builds `G160.COM` (same terms), also put on the test
disk.

```text
G160 [16|8|4|A]
```

Graphics screen 0 becomes single-plane 320 x 200 at 16, 8 or 4 bits per
pixel (default 16) in 320-dot and 200-line mode, with port `0100h` RSM = 01
so that 24.8 kHz shows every line on two rasters (measured in M104). The
hardware enlarges 2 x 2 and the CPU writes each picture dot as 2 x 2
pixels, so a dot is 4 x 4 screen dots. The SGP is not used: BitBlt copies
blocks and has no enlargement. The picture (a moving gradient with a
32 x 32 block of one-dot checks at the top left) is computed and redrawn
in full until a key; then `G160 nn-bit N redraws hh:mm:ss-hh:mm:ss`
(the start and end calendar-clock values, each byte as two hexadecimal
digits) appears in the function-key row until a second key. The rate includes computing every
dot's colour, so it is a lower bound for plain copying.

PC-Engine's `INT 21h` provides only file, memory and process functions
(technical manual chapter 7): there is no console output or time call. The
time comes from the calendar clock BIOS (`INT 8Ch` function 02h, whole
seconds), so run for ten seconds or more; the result is written to TVRAM.
Under vaeg (2026-10-06, emulated time, `--nowait`) 16, 8 and 4 bits gave
98 redraws in 34 s, 112 in 33 s and 138 in 33 s.

`[MEAS]` On the PC-88VA2 (2026-10-06, 24.8 kHz monitor setting) `G160`
shows the one-dot checks at the top left and a gradient moving slowly
upwards, as in vaeg: single-plane graphics in 320-dot and 200-line mode
with RSM = 01 show a 160 x 100 picture with a colour per dot. Rates on the
hardware: `G160 8` gave 91 redraws in 59 s (1.5 per second; vaeg 3.4), so
vaeg runs this loop about 2.2 times faster than the PC-88VA2. A 16-bit run
gave 189 redraws but an impossible computed time (3494 s), so the program
now shows the raw clock values instead of a computed time. The calendar
BIOS returns binary, not BCD, values, in vaeg and on the PC-88VA2. With
the raw values the PC-88VA2 gave 1.4, 1.6 and 1.9 redraws per second at 16,
8 and 4 bits (24.8 kHz; about the same at 15.98 kHz), against 3.0, 3.4 and
4.25 in vaeg (`pc88va-video-modes.md` section 12.4).

`A` runs 16, 8 and 4 bits one after another; each needs the usual two
keys (stop drawing, then leave the result line).

## G256: a 256 x 192 window

[`g256.asm`](g256.asm) builds `G256.COM` (same terms), also put on the test
disk.

```text
G256 [16|8|4|A]
```

The VA has no 256-dot graphics mode, and the TSP horizontal active period
does not clip graphics, so 256 x 192 is shown as a window in single-plane
320 x 200 graphics (320-dot and 200-line mode): GVRAM x 32-287, y 4-195,
black outside, pixels 1:1 with GVRAM at 16, 8 or 4 bits (default 16). 192
lines fit the native 200-line mode, so no `SYNC` change is made and the
same program runs with the monitor switch at 15 or 24 kHz: at 15.98 kHz
each line is one raster (about 61.5 Hz), at 24.8 kHz port `0100h` RSM = 01
shows it on two rasters (56.4 Hz). The window has a white one-dot border,
a grey grid every 32 dots, one-dot red/white checks in its top-left
32 x 32 cell and a gradient; line 1 carries one white dot per bit of depth
(16, 8 or 4 dots at the top left). Any key restores the screen. `A` shows
16, 8 and 4 bits one after another, a key each.

## Results

See [M104](../../../docs/agents/tasks/M104_v3_display_timing.md) and
[`pc88va-video-modes.md`](../../../docs/modernization/pc88va-video-modes.md#12-measured-non-native-timings)
for the measured results.
