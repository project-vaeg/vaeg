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


# M103i - V1 mode

Status: **complete; G103i passed on 2026-10-05; merged to `main` at [a727a45](https://github.com/nakatamaho/vaeg/commit/a727a458a112d90c721d78f8db9d4580953c18b1)**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103i-v1-mode` off `main` at
`093385e922f3bcc07b15ce5340310c61ba1d0841` (M103a–M103h merged). Commit
prefix `M103i:`.

## Goal

Run PC-8801 V1-mode software (N88-BASIC V1 and V1 media) on the emulated
VA and VA2 and compare it with the real machine (maintainer request:
"see what the real machine does"). Differences found are fixed or
recorded as decisions; vaeg follows the real VA, not a PC-8801.

## Background

- V1/V2 is the VA's memory switch B1FC5h bit 0, selected in Emulate >
  Z80 mode (M103h, decision C4); port 150h follows it at reset on both
  models (C8). N88-BASIC Version 1.x starts in V1 on both models
  (maintainer check, G103h).
- `[POLICY]` H/S is a vaeg extension (C6): the VA always reports high
  speed, and its compatible-mode CPU speed does not change with S.
  Whether V1 S software timed for a 4 MHz PC-8801 runs too fast on a real
  VA is one of the questions for the real machine.
- Before M103i no V1 media had been run (decisions register, open items).
- N80SR and the PC-8001mkII (N80) mode are not pursued (maintainer
  decision, 2026-10-05): on a real VA neither display is reachable even
  with a custom I/O-trap handler (study in
  [`v1v2-n-basic-mode.md`](../../modernization/v1v2-n-basic-mode.md#8-n80sr-and-n80-modes-not-pursued)).

## Scope

1. **Media.** V1 media from the maintainer, referred to by neutral test
   identifiers (V1-A, V1-B, …) in tracked files.
2. **Comparison.** Each title in V1 S and V1 H on VA and VA2 against the
   maintainer's real-machine observations (photos, timing, sound).
3. **Fixes** for differences with a demonstrated cause, with ledger
   entries; other differences become decisions or open observations.

## Gate G103i (human)

Standard V3 gate unchanged, plus: the V1 media chosen by the maintainer
behave in vaeg as on the real VA, or each difference is fixed or
recorded.

## Implementation progress

- Task started; V1 media supplied by the maintainer (V1-A: a PC-8801mkII
  system disk; V1-B: a game collection; V1-C, V1-D: single games).

### V1-A demonstration: `Disk I/O error` at its second BLOAD (real VA too)

- Symptom: in V1, loading V1-A's demonstration program and running it
  stops with `Disk I/O error` at its second `BLOAD`; an 8801 emulator
  runs it. `[MEAS]` The real VA shows the same error at the same line
  (maintainer, 2026-10-05), so vaeg matches the hardware. Not a defect.
- `[ROM]` Cause: the program's first `BLOAD ...,R` installs a fast loader
  that writes 34h bytes of its own code to the FDD subsystem at
  7F60h–7F93h with subsystem command 0Ch (B,C = address, D,E = count;
  0Ch at 053Bh in both ROMs), code that calls the sub-ROM's 06AFh
  (identical in both ROMs). On a PC-8801 that area is free scratch. The
  VA subsystem ROM differs from the PC-8801MA2 `DISK.ROM` in 865 bytes;
  its additions use 7F67h/7F68h: 7F67h is set to FFh by VA-only command
  25h (1607h) and cleared by 26h (1615h), and while it is nonzero the
  disk commands fail (1600h returns carry; for example command 0Eh,
  0D62h → 1465h → error) and the motor-on step is skipped (1741h,
  1764h). The uploaded code puts 40h at 7F67h, so the next disk access
  fails.
- Experiment (not a real-machine configuration): with the PC-8801MA2
  `DISK.ROM` as the subsystem ROM, the program passes that line, shows
  its wait message, and later fails with `Disk I/O error` in its
  unpacking routine. Not investigated further; a real VA never gets
  there.

### V1-C (single game)

- Headless, V1 H, no configuration file: boots from the disk on VA and
  VA2, shows its title, starts with SPACE and plays round 1 (score
  counts, enemies move).
- Maintainer check (vaeg, 2026-10-05): runs; "too high" (open
  observation, meaning to be confirmed: speed or sound pitch). Accepted
  for now.
- `[MEAS]` Real VA2 in V1 S: boots and runs, as in vaeg (maintainer,
  2026-10-05; an earlier "does not boot" report was withdrawn). The
  maintainer's VA2 has no working sound output, so sound cannot be
  compared on it.

### Monitor switch (maintainer request)

- `[VA-TM]` DIP switch SW1 selects the monitor (port 40h bit 1: 0 =
  24.8 kHz, 1 = 15.7 kHz). vaeg fixed it at 24 kHz; Emulate > モニタ now
  selects 24 kHz or 15 kHz (`Monitor15kHz`) and resets. The ROMs program
  the 15 kHz timing themselves, and vaeg's existing 15.98 kHz path draws
  200 lines with blank odd rasters. Headless: V1-C on VA and VA2 and the
  VA2 V3 start screen show that layout.
- Maintainer check (Windows build `d82c8680`, 2026-10-05): OK.

### Digital RGB monitor (maintainer request)

- `[VA1]` schematic p. 263: the DIGITAL connector carries R, G, B,
  XHSDI, XVSDI and NCLK, no intensity line. p. 261: each digital colour
  line is taken from the analog output stage's drive node through 1 kΩ
  (R30 for red), so there is no conversion logic and the monitor's input
  threshold decides each component. `[MEAS]` (maintainer) On a digital
  monitor the VA demo looked almost white, which fits a low threshold.
- Emulate > モニタ > デジタル RGB (`MonitorDigital`, display only, no
  reset) shows each component full on from a quarter of full scale
  (decision G9). 88 digital-palette software (V1-C) is unchanged by it,
  as expected.
- Maintainer check (Windows build `62e2f8e4`, 2026-10-05): "about like
  this"; the quarter-scale threshold is kept.

### V1-D (single game)

- Headless, V1 H, no configuration file, VA and VA2: title, high-score
  disk question, key help and character pages loop as an attract mode;
  RET on the key page starts the game (scene 1, scores, enemies), which
  ends in GAME OVER and a replay prompt without input.

### Port 40h sound (maintainer report)

- Report: with V1-E (a user disk with its own DOS), after loading the
  PC-8801mkII `CMD` extension and its demonstration, neither `BEEP` nor
  `CMD SING` sounded; the real VA plays them.
- `[VA-TM]` 5.10.3: the buzzer is on when 40h bit 5 or 1CDh bit 3 (XBEEP,
  active low) is on; 5.10.4: 40h bit 7 (FBEEP) drives the speaker
  directly, enabled by 190h bit 4 (FBEN, on at reset). vaeg modelled
  neither 40h bit. Headless, the demonstration toggles bit 7 about
  106,000 times. Fixed (ledger); the port sound level is not saved in
  state files.
- Maintainer check: silent at first because the master volume was below
  21, which rounded the buzzer volume to 0 (fixed, ledger). With that,
  `CMD SING` plays but "sounds completely different" from the real VA
  (open). Measured: the demonstration toggles 40h bit 7 every about 2,070
  CPU clocks in its first tone, a 1.93 kHz square wave at 7.9872 MHz;
  vaeg outputs it as a 0/1 level, so held levels between notes add DC
  steps (most of the captured energy is below 10 Hz).

### G103i

- Passed on 2026-10-05 (maintainer). Fine pitch and tempo comparison of
  the buzzer, port sound and V1-C is deferred.
