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


# M105 - 15 kHz scanline fill and the tape leader sound

Status: **complete; G105 passed on 2026-10-07; merged to `main` at [9b4f8ef](https://github.com/nakatamaho/vaeg/commit/9b4f8ef8b08084fec69a31315af20e28f4c77d9b)**

Series: V3 series (scanline fill) with a V1/V2 cassette follow-up
(maintainer decision 2026-10-07). Branch
`topic/m105-scanline-fill-tape-leader` off `main` at
`2508ccf47e6a10d9c2b67d1fad62b5a3c7522491`. Commit prefix `M105:`. The CPU
speed gap found in M104 moves to M106.

## Goal

1. **15 kHz scanline fill.** At 15 kHz (non-interlaced) vaeg shows each
   line on the even raster of a pair and leaves the odd raster black, as a
   CRT shows its scanline gaps. An LCD that accepts 15 kHz doubles each
   line instead (maintainer observation, M104 photographs). Add a monitor
   setting: gaps (CRT, default, the current picture) or filled (LCD), saved
   in the configuration. Interlaced modes and 24.8 kHz are unchanged.
2. **Tape leader sound** (vaeg cassette extension, C10). When the motor
   starts for reading a raw or T88 image, play 1200 Hz for one second and
   then 2400 Hz for one second before the data, and hold the data back for
   those two seconds so that BASIC prints `Found` after the sound, as the
   maintainer remembers a PC-8801 ("low, then high, then Found"). With
   fast load (`CMT_Fast`) there is no leader sound and no delay. Saving is
   unchanged. `[POLICY]` (maintainer memory; no document at hand).
3. **Emulation speed** (maintainer request, 2026-10-07): a slider in the
   Emulate menu from 10 % to 400 % of real time with No Wait (full speed)
   at its right end; No Wait leaves the Screen menu.
4. **PC-8801-style V1/V2 display** (maintainer request, 2026-10-07,
   decision C12): an option to show V1/V2 at 24 kHz as a PC-8801 does,
   8x8 text without gaps and 640 x 200 graphics with scanline gaps; the
   default stays the VA's own picture.
5. **CPU speed** (maintainer request, 2026-10-07): a V1/V2 game ran too
   fast, and the speed slider garbles the BEEP below 100 %. A second slider
   slows only the CPU (10-100 %), in V3 and in the compatible mode; frames,
   timers and sound keep real time. The speed slider stays as it is.

## Gate G105 (human)

Standard V3 gate unchanged, plus: at 15 kHz the new setting fills the
scanline gaps and the default keeps them; loading a tape without fast load
plays low then high before `Found`; with fast load it loads at once. The
speed slider slows and speeds the guest and its right end runs at full
speed. With "V1/V2 表示: PC-8801 風" V1/V2 text uses the 8x8 font and
200-line graphics show scanline gaps at 24 kHz; the default is unchanged. The CPU
speed slider slows a program without changing its sound.

## Implementation progress

- 15 kHz scanline fill: Emulate > モニタ, "15 kHz 走査線: 隙間あり (CRT)"
  (default) or "埋める (液晶)", saved as `Monitor15kHzFill`. Romless test
  composes a 15.98 kHz frame and checks the odd raster (gap, or a copy of
  the even raster); it fails with the fill disabled. Headless G256 at
  15 kHz shows the gaps by default and none when filled.
  [be71480c](https://github.com/nakatamaho/vaeg/commit/be71480cb438b9c368d8364c5d58f92bd416eb28).
- Tape leader (decision C11): romless test checks that the data are held
  back, the zero crossings of the first and the second second (1200 and
  2400 Hz), the first byte after the leader, and no leader with fast load;
  it fails with the leader or the tone change disabled. Headless V2 BASIC
  `LOAD "CAS:"` without fast load prints `Found` 110 frames (1.95 s at
  56.4 Hz) later than without the leader.
  [ef85911c](https://github.com/nakatamaho/vaeg/commit/ef85911cabc4e92a6bb39eb35403b90abc02af41).
- V480PAT `F` (640 dots), `G` (320 dots doubled, 24.8 kHz) and `J` (320
  dots, 15 kHz) sweep frames near 60 Hz, with `Q` (bottom 1, sync 1) and `P`
  (bottom 2, sync 4). `[MEAS]` On the PC-88VA2 every entry was displayed
  and the monitor read-outs match the computed rates with `VS` as written
  (`pc88va-video-modes.md` section 12.8): 59.25 Hz for 640 x 400 and 320 x
  200 doubled with top 17, bottom 1, sync 1; 60.1 Hz for 640 x 394 and
  320 x 197 doubled; 59.9 Hz for 224 lines at 15 kHz.
- `VS` below 4 is now counted as written in the frame length (ledger):
  [62690c08](https://github.com/nakatamaho/vaeg/commit/62690c0883d8aba19e1da70b40b0678f60374ef6).
- Emulation speed: `timing_setspeed` scales the frames due per host
  millisecond (romless test: 10, 50, 100, 400 % and clamping; fails with the
  speed ignored); the Emulate menu slider sets it and No Wait at its right
  end. A release build ran the VA2 ROM screen at 28.20, 56.41, 112.81 fps
  at 50, 100, 200 % and 572 fps with No Wait.
  [087c0b07](https://github.com/nakatamaho/vaeg/commit/087c0b0715a01b87a84d76b98eecb5718a8f9892),
  [cc7c95ac](https://github.com/nakatamaho/vaeg/commit/cc7c95ac5a23d6564107e88917250689faa496c2).
- `[MEAS]` (maintainer, real VA, 2026-10-07) In V1/V2 mode at 24 kHz the VA
  shows 200-line graphics without scanline gaps (each line on both rasters)
  and the text in a clean 8 x 16 font, unlike a PC-8801's 24 kHz monitor,
  where 640 x 200 graphics show gaps and text does not. vaeg matches the VA:
  a V2 N-88 BASIC `LINE ...,BF` box is drawn without gaps and the text uses
  the 16-raster font. No change needed.
- PC-8801-style V1/V2 display (C12): romless test checks the 8x8 glyph rows
  on raster pairs, the odd rasters of 200-line graphics, the unchanged VA
  standard, and no effect at 15 kHz or in V3 mode; it fails with either the
  text or the graphics change removed. Headless V2 N-88 BASIC: the text uses
  the 8x8 font doubled and a `LINE ...,BF` box alternates lit and blank
  rasters; with the default the box has no gaps.
  [33ecf84d](https://github.com/nakatamaho/vaeg/commit/33ecf84dda2bf26a0e5c79b8039edd1b487c1d5a).

- CPU speed: `pccore_set_cpu_speed` scales the V30 cycle cost (with the
  Configure multiplier, which never applied to the compatible core) and the
  uPD70008-compatible counter. Tests: romless clockscale cases (10, 50, 100 %,
  clamping, multiplier 4) and a compatible-mode run of ten NOPs (twice the
  clocks at 50 %); each fails with its scaling removed. Headless G160 8-bit
  over the same 32 guest seconds: 106 redraws at 100 %, 47 at 45 % (the
  PC-88VA2 managed about 1.6 per second, 51 in 32 s).
  [a09077a1](https://github.com/nakatamaho/vaeg/commit/a09077a19058be396793a3ef333bd4e67f8cd27f),
  [266a28c5](https://github.com/nakatamaho/vaeg/commit/266a28c50397ed11b890dd3df0062cc18e51bbb9).
- Open (to measure on the real VA, later milestone): the V1/V2 H and S
  speeds. Decision C6 keeps S at the VA's full CPU speed until then.

- Open (later milestone): the port 40h sound is wrong, possibly a timing
  mismatch (maintainer report, 2026-10-07): `CMD SING` is not clean and V1/V2
  game sound is clearly off, also at 100 % speed; the plain `BEEP` seems
  fine. Recorded in the bug-fix ledger's open defects, not investigated.

### G105

- Passed on 2026-10-07 (maintainer), with the Linux build `c21fe71a...799d`
  and the Windows build `d2e80734...92b6` (SHA-256) from
  [b8206006](https://github.com/nakatamaho/vaeg/commit/b82060065aa0c53ceb76e58cc26bced3ce22a5a0);
  decisions C11 and C12 approved.
- Left open: the port 40h sound defect (bug-fix ledger, open defects;
  predates M105), the V1/V2 H and S speed measurement (decision C6), and the
  CPU speed gap found in M104.
