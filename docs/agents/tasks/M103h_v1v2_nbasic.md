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


# M103h - N-BASIC in V1/V2 mode

Status: **complete; G103h passed on 2026-10-05; merged to `main` at [0fecd39](https://github.com/nakatamaho/vaeg/commit/0fecd39f83973117cbec9466f9329f679bb151bc)**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103h-v1v2-nbasic` off
`main` at `98ac7a89013871a067ab9d1b3f3a7c1015a2789a` (M103a–M103g merged).
Commit prefix `M103h:`.

## Goal

Run PC-8001/8801 N-BASIC from a user-supplied N80 ROM (the VA carries no
N-BASIC): entered by a start-up option and, as on a PC-8801 in V1S mode,
by `NEW ON 1` from V1/V2 N-88 BASIC (maintainer requirement). N80SR
(N80 V2) and V1 media follow later.

## Background

- `[MEAS]` (maintainer) On a real VA, N-BASIC runs once its ROM image is
  copied to RAM and run in all-RAM mode (I/O 1987-11, "88VA plus N-BASIC").
- `[ROM]` `NEW ON 1` (extension ROM bank 2, 77A7h) needs IN 31h bit 7 = 1
  (V1) and bit 6 = 0 (standard speed); otherwise error 33.
- `[VA-TM]` IN 31h reads memory switch B1FC6h: MS27 V1/V2, MS26 unused and
  always 1. `[X88000]` on the PC-8801 bit 6 is high speed.
- `[ROM]` V1/V2 is B1FC5h bit 0 (1 = V1): the VA2 ROM writes B1FC5h to
  port 1C6h at reset (F000:12E4); port 150h then reports the mode and the
  ROM recomputes MS27 (F000:14BC). Before that it sets B1FC6h bit 7 and
  checks the record's checksum B1FCDh (F000:23B7).
- Experiment (temporary code, not committed): mapping the ROM at
  0000h–7FFFh under port 31h RMODE and jumping to 0000h, N-BASIC 1.8
  initialises the screen, then waits for the FDD subsystem during its
  boot-disk probe.

## Scope

1. **Mode selection.** Emulate > Z80 mode menu: V2 H, V2 S, V1 H, V1 S.
2. **N80 ROM option** and the N-BASIC memory map.
3. **Entry**: start-up option, and `NEW ON 1` in V1S.
4. **Boot-disk probe**: find why N-BASIC waits for the FDD subsystem.

## Gate G103h (human)

Standard V3 gate unchanged, plus: the Z80 mode menu selects V1/V2 (BASIC
banner Version 1.x / 2.x after reset); N-BASIC starts with a
user-supplied ROM, both from the start-up option and by `NEW ON 1` in
V1S, and runs a short program.

## Implementation progress

### Mode selection (scope item 1)

- Maintainer request: an Emulate > Z80 mode menu with V2 H, V2 S, V1 H and
  V1 S. This replaces the M103b decision of no user-facing V1/V2 choice:
  V1/V2 is now the VA's own memory switch (B1FC5h bit 0), which its setup
  stores, not an override. H/S is a vaeg extension (the VA reports H);
  S clears IN 31h bit 6 in 88 mode, without changing CPU timing.
- While testing, saved backup memory turned out to reset its memory
  switches at every boot: vaeg's checksum did not follow the ROM's rule
  (ledger entry). With the fix the record survives three boots in a row,
  and a V1 setting boots N-88 BASIC Version 1.9.
- Romless test: V1/V2 selection with the ROM-rule checksum, and S clearing
  MS26 only in 88 mode; it fails without either change.

### Maintainer check of the menu (Windows build `e6bfac36`)

- Menu entries select and reset as intended.
- V1 S showed a black screen or the previous mode's text: in standard
  speed N-88 BASIC disables the fast TVRAM (port 32h TMODE set, 98h) and
  keeps its text in main RAM F000h–FFFFh, which vaeg did not display (the
  VA never runs that way). The 3301 rows of the TVRAM window are now read
  from 88-mode main RAM while TMODE is set in 88 mode. A V2 → V1 S reset
  with the PC-8801mkII system disk then shows N-88 BASIC 1.9 and its
  output.

### N-BASIC ROM and NEW ON 1 (scope items 2, 3)

- An optional 32 KiB `n80.rom`/`N80.ROM` beside the VA ROMs is shown at
  0000h–7FFFh under port 31h RMODE in 88 mode (ROM/RAM mode), as on a
  PC-8801. Without it RMODE keeps the VA behaviour (decision M3). The
  maintainer's MA2 N80 ROM is "NEC PC-8001 BASIC Ver 1.8"; its top 8 KiB
  equals the VA's Debug 8800 bank, and N-88 BASIC's MON still works with it.
- `[ROM]` NEW ON 1 builds `OUT 31h,A / JP (HL)` at 847Ah with RMODE set.
  In V1 S it now starts N-BASIC: the banner appears and a one-line program
  runs.
- The earlier stop during N-BASIC's start was its disk boot: it reads
  track 0 sector 1 from drive 1 and runs it; an N-88 system disk's IPL
  then waits. With drive 1 empty it starts normally. Treated as the
  expected PC-8801 behaviour, not a defect.

### Second maintainer check

- Report: on VA2 in V1 S, `NEW ON 1` from disk BASIC resets the machine.
  Not reproduced here: without `n80.rom` the screen stays black after
  `NEW ON 1` (N-88 BASIC jumps into the absent N-BASIC); with it and a
  disk in drive 1, N-BASIC starts (40-column function-key row) and then
  boots that disk, whose IPL decides what follows. About now shows
  ROM(N80) and the stored Z80 mode so the two cases can be told apart.
- Report: on the original VA, V1 H/V1 S had no effect and the cursor
  blinked at the top. V1 is fixed (port 150h, ledger entry): V1 boots
  N-88 BASIC 1.9 on the original VA. The cursor report is open; not seen
  in the headless runs.

### Third maintainer check

- Confirmed: About entries, V1 on the original VA, `NEW ON 1` with drive 1
  empty.
- Reported and fixed: the cursor at the top left on the original VA (TSP
  CURS, ledger entry); Copy screen text in V1/V2 mode.
- Implemented: N mode (decision C9, maintainer option A), described in
  [`v1v2-n-basic-mode.md`](../../modernization/v1v2-n-basic-mode.md); a
  headless run boots N-BASIC without a disk on VA and VA2, and a program
  runs.
- Open: a graphic character after `Ok` with the maintainer's N-BASIC
  Ver 1.2 ROM (not available here).

### Fourth maintainer check (Windows build `54723036`)

- Confirmed: N mode starts N-BASIC without a disk on VA and VA2; the
  original VA's V1/V2 cursor follows the input position; Copy screen text
  copies the displayed V1/V2 text.

### Fifth maintainer check

- The mark after `Ok` came from the PC-98 font setting: its FCh glyph is a
  backslash, while the VA font's FCh is blank. Reproduced headless with
  `98font.rom`; with the VA font the maintainer sees no mark. Not a defect.
