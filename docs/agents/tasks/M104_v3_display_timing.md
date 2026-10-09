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

# M104 - V3 display timing: non-native line counts and line doubling

Status: **complete; G104 passed on 2026-10-07; merged to `main` at [2bec316](https://github.com/nakatamaho/vaeg/commit/2bec31628adb1b9cfcfd0b9bce623a5f2fb6fc00)**

Series: V3 series, first milestone (maintainer decision 2026-10-06: the
display-timing work takes M104; SGP moves to a later milestone). Branch
`topic/m104-undocumented-tsp-display-semigraphics` (renamed from
`topic/m104-v3-display-timing` on 2026-10-06) off `main` at
`9ff34397f24af238f239540d96c526b28871debb`. Commit prefix `M104:`.

## Goal

Measure on real hardware which TSP `SYNC` timings beyond the documented
200/204/400/408-line profiles a PC-88VA displays, record the results with
`[MEAS]`, and make vaeg show them: 480-line frames (VIEW480), 15 kHz frames
of up to 256 lines (320x240), and 24.8 kHz line doubling (port `0100h`
RSM = 01) beyond 200 lines.

## Background

- `[VA-TM]` 4.9.4: 15 kHz cannot show 400/408 lines; at 24.8 kHz a 200/204-
  line graphics screen has blank odd rasters (RSM = 00) or each line on an
  even/odd raster pair (RSM = 01). Graphics resolutions are 640/320 dots and
  200/204/400/408 lines.
- uPD72022 data book (via [`upd72022-tsp.md`](../../modernization/upd72022-tsp.md)
  10.1): at most 512 active lines; `VS >= 4`, `BBR + BBL >= 2`, top
  blanking plus border at least 10h without sprites and 20h with sprites.
- VIEW480 (mami, 1992; Softlib 2-404) switches a 640 x 401-480 picture to a
  480-line 24.8 kHz frame with bottom blanking and sync of 1 line each, below
  those minimums.
- vaeg (main at the branch point): headless screenshots of these timings
  stay 640 x 422 (400 display lines); a 480-line frame and a 15 kHz
  240-line frame are not shown in full. Where the limit lies is part of
  scope 3.

## Scope

1. **Test program** (done in this milestone): `tools/pc88va/vtiming/
   v480pat.asm` (2-clause BSD, independent of VIEW480) and a disk builder.
2. **Measurements**: results in this file and in
   [`pc88va-video-modes.md`](../../modernization/pc88va-video-modes.md),
   tagged `[MEAS]`.
3. **vaeg**: output height from the TSP frame (`VAD`, RM, monitor switch);
   15 kHz frames up to 256 lines; 24.8 kHz frames up to 480 rasters;
   line doubling with RSM = 01 beyond 200 lines; graphics fetch extent below
   `VAD` as measured; `HAD` narrowing only if the measurement shows it.
4. **Tests**: romless tests feeding the measured `SYNC`/`GRMODE` settings.

## Gate G104 (human)

Standard V3 gate unchanged, plus: in vaeg, `VIEW480`, `V480PAT 240 T W`,
`V480PAT 240 U W` and `V480PAT 232 D` show what the real machine showed.

## Implementation progress

- Test program and builder committed:
  [`tools/pc88va/vtiming/README.md`](../../../tools/pc88va/vtiming/README.md).
- `[MEAS]` First hardware round (2026-10-05/06): recorded in
  [`pc88va-video-modes.md` section 12](../../modernization/pc88va-video-modes.md#12-measured-non-native-timings).
  Machine: PC-88VA2. Open: where the 240-line pictures lose their last
  line; the horizontal result of `N`.
- Line ruler added to V480PAT for the second round.
- `[MEAS]` Second round (2026-10-06):
  [`pc88va-video-modes.md` 12.1](../../modernization/pc88va-video-modes.md#121-second-round-line-ruler),
  with photographs. At 15.98 kHz graphics show `TBL + VAD - 37` lines; at
  24.8 kHz with RSM = 01 graphics stop advancing after line 200; graphics
  are not clipped by the TSP `HAD`; `240 T` (640 dots) follows the same
  rule. Open: whether 15 kHz graphics are shifted by `37 - TBL` against
  text.
- Added TSPMODE (probe of undocumented screen-table `MODE` values) and
  G160 (160 x 100 with a colour per dot in graphics). `[MEAS]` G160 runs on
  the PC-88VA2 as in vaeg (checks and moving gradient visible); hardware
  redraw rates pending.
- `[MEAS]` TSPMODE on the PC-88VA2: `MODE` 8-14 show non-character
  patterns (candidates for the semigraphics/graphics static-picture
  modes), 6-7 are attribute variants, 15-20 break the text; recorded in
  [`upd72022-tsp.md`](../../modernization/upd72022-tsp.md) section 20.4.
  `TSPFILL A` (filmed) shows the modes are deterministic and data
  dependent, not a TSP crash, but give no usable picture format; the
  investigation stops there (`upd72022-tsp.md` section 20.4).
- SYNC now stops the TSP text until DSPON, as the data sheet states and the
  PC-88VA2 photographs show ([bf55c229](https://github.com/nakatamaho/vaeg/commit/bf55c2292e40dda45b9ddb839bec84b118f0f74a)).
- `[MEAS]` G256 on the PC-88VA2: a 256 x 192 window in native 320 x 200
  graphics is shown at 15.98 and 24.8 kHz and at 16, 8 and 4 bits, matching
  vaeg; recorded in
  [`pc88va-video-modes.md`](../../modernization/pc88va-video-modes.md)
  section 12.3.
- `[MEAS]` Third round (2026-10-07, sweeps): 24.8 kHz 400-line graphics
  are shown past line 400 up to 480 (`480 S K`, 49.7 Hz); 15.98 kHz with top
  blanking 37 shows all of 200-240 lines (320 x 240 at 56.5 Hz works);
  `244/248 R W` show only lines 0-239; monitor limits 503/507 lines at
  24.8 kHz and 291/294 at 15.98 kHz; G160 runs 2.1-2.2 times faster in
  vaeg than on the PC-88VA2 at every depth; TSPMODE 21-31 recorded.
  [`pc88va-video-modes.md`](../../modernization/pc88va-video-modes.md)
  section 12.4,
  [`upd72022-tsp.md`](../../modernization/upd72022-tsp.md) section 20.4.
- vaeg implements the rules (480-row canvas, 24.8 kHz doubling stop,
  15.98 kHz start delay and top hiding, RSM = 00 blank past line 200;
  `pc88va-video-modes.md` sections 12.5 and 12.6):
  [631e3e98](https://github.com/nakatamaho/vaeg/commit/631e3e980a3883cd7b7a950a612ad206b9a79077),
  [ee21db2a](https://github.com/nakatamaho/vaeg/commit/ee21db2a2c953c8319d5fb8c68290401eaf0905d),
  [2cfa871a](https://github.com/nakatamaho/vaeg/commit/2cfa871ae32e0a6e35f56f2ba87279bdaf210e99).

### G104

- Passed on 2026-10-07 (maintainer), with the Linux build
  `5dbce92a...7c66` and the Windows build `d8dd4d09...1ccf` (SHA-256) from
  [03e88ae9](https://github.com/nakatamaho/vaeg/commit/03e88ae93d69b560d58591a45ee4e64da83df716).
- Left open: the CPU speed gap (bug-fix ledger, open defects), the
  hypothesis that the undocumented TSP modes show ROM data, the frame
  buffer wrap below the last line with a top over 37, and the 15.73 kHz
  family.
