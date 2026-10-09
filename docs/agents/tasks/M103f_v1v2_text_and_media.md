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


# M103f - V1/V2 text addressing and multi-image media

Status: **complete; G103f passed on 2026-10-04; merged to `main` at [8b446b1](https://github.com/nakatamaho/vaeg/commit/8b446b131413d6d71fa9a50ce69bd75f3523f942)**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103f-v1v2-text-media` off
`topic/m103e-v1v2-srdemo-sound` at `e4f16e577a6f09ab614aea0ea737ca976d950366`
(M103e, G103e pending; the maintainer asked to continue to M103f before the
M103d and M103e gates). Commit prefix `M103f:`.

## Goal

Take over the items M103e left open: the stray text that GAME-A shows
after it stops the 3301 display, and loading the second disk of a
multi-image D88 without splitting the file. V1 mode remains in the series
plan but needs V1 media, which is not available here.

## Scope

1. **TSP byte-mode addresses.** The rule that turns a split-screen start
   address into a TVRAM byte address in 3301 emulation.
2. **Multi-image D88.** Selecting an image inside a D88 file that holds
   several disks, from the command line (and the GUI if it fits).
3. **Tests** for both.

Out of scope: V1 mode (no media), sound beyond the M103e gate check.

## Gate G103f (human)

Standard V3 gate unchanged, plus: GAME-A, loaded from its original
two-image file, shows its screens without the text grid; SR-DEMO,
MA2-DEMO and V2 BASIC text still display as at G103e.

## Implementation progress

### TSP byte-mode addresses (scope item 1)

- `[VA-TM]` BNN §8.2.1: in V1/V2 mode the TSP runs in byte-access mode,
  and its local word addresses 3000h–37FFh and B000h–B7FFh (V3) become
  byte addresses 3000h–3FFFh and B000h–BFFFh (the halves from 3800h and
  B800h are unusable). The 4 KiB window is TVRAM A6000h–A6FFFh.
- M103c used TVRAM byte = 3000h + local, where local is half the table's
  start field. That matches the 3000h range (BASIC: 678Ch → local 33C6h →
  63C6h) but maps B000h to E000h instead of 16000h. `[DERIVED]` Keeping the
  low 12 bits and doubling the upper four, `((L & F000h) << 1) |
  (L & 0FFFh)`, matches both documented ranges.
- `[ROM]` GAME-A stops the text with a 3301 RESET; the VA2 ROM then sets
  the start field to 1000h (local 0800h). The old rule read TVRAM 3800h,
  where the ROM keeps words 0080h/00E8h, and showed them as characters;
  the new rule reads TVRAM 0800h, which is clear.
- Result: in GAME-A the grid is gone from the first 18 rows. The last two
  rows of the 20-row screen cross local 1000h and read TVRAM 2070h onward,
  where the V3 BIOS text-screen setup (`F000:2470`–`24AF`, run at every
  boot) leaves words 0080h; they show as a dashed line. Local addresses
  below 3000h are outside the documented byte-mode ranges, so what the real
  TSP shows there is not known. Open.

### Multi-image D88 (scope item 2)

- The loader walks the images' `fd_size` fields to the chosen one and adds
  its base to every track read and write; formatting is refused unless the
  file holds a single image (growing a track would move later images).
- `--fdd1-image N` / `--fdd2-image N` (from 1) choose the disk. GAME-A runs
  from its original two-image file with `--fdd2-image 2`, with the same
  screens as the split copies. Not stored in the configuration or in state
  files (a state file reloads the drive's current choice); no GUI control.

### Tests (scope item 3)

- `vaeg_romless_tests`: the byte-mode address rule at 33C6h, 3FFFh, B000h
  and 0800h; a two-image file (second image selected with its base, a
  missing third image rejected) and the new options.
- Full CTest without failures (108 entries, two skipped as before); the V2
  BASIC acceptance test passes; the tests-off `linux-release` preset
  builds; each code commit builds alone; with the release build SR-DEMO
  shows its title, MA2-DEMO loads its demonstration, and V3 PC-Engine
  lists its disk.

### Gate feedback (2026-10-04)

- Maintainer check with the Windows build `4bdbb33c`: standard V3 gate,
  V2 BASIC colour graphics with correct `POINT`, `SCREEN 1`/`2`, SR-DEMO
  pictures and sound, MA2-DEMO (including sound board II) and GAME-A all
  run. Policies approved: the byte-mode address rule, refusing to format
  multi-image files, and no V1/V2 feature flag (ADR-0016).
- Reported: stray graphics in V2 BASIC (after `PAINT`) and the SR demo
  after running a V3 game and resetting. Cause: plane 3 drawn although
  110h G3MSK was clear (fixed, ledger entry).
- Reported: a dashed line under GAME-A's title (open item X4). The
  maintainer chose to keep the text blank while the 3301 display is
  stopped; rows starting outside the usable byte-mode ranges are now not
  displayed, and the line is gone.
- Moved to M103g: uPD3301 semigraphics (missing in the SR demo) and sound
  timing (in the SR demo's winter scene the music starts late and briefly).

## Summary for G103f

Policies for approval: the byte-mode address rule (derived from the two
ranges BNN 8.2.1 tabulates); refusing to format inside multi-image files.
Open: the two dashed rows in GAME-A after a 3301 RESET; V1 mode (no media).
