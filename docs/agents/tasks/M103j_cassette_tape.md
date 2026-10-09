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


# M103j - Cassette tape (vaeg extension)

Status: **complete; G103j passed on 2026-10-05; merged to `main` at [b1888e0](https://github.com/nakatamaho/vaeg/commit/b1888e04bc6a886b056dc2c9b467a5e66b77d402)**

Series: V1/V2 mode (ADR-0016), closing milestone. Branch
`topic/m103j-cassette` off `main` at
`98f7eb066c33a24f322658789883d49cb7fcfed8` (M103a–M103i merged). Commit
prefix `M103j:`.

## Goal

Let V1/V2 BASIC load and save programs and data on cassette tape images
(`LOAD "CAS:..."`, `BLOAD`, `SAVE "CAS:..."`), as on a PC-8801
(maintainer request). The VA has no cassette interface, so this is a
vaeg extension, like the N mode.

## Background

- `[VA-TM]` Ports 20h/21h are the µPD8251 for RS-232C only; port 30h
  output defines only bit 0 (80CM) and bit 1 (MCM). The PC-8801 shares
  the 8251 with the cassette and selects it, and switches the motor,
  with port 30h bits 2–5. A real VA cannot load from tape.
- `[ROM]` The VA's N-88 BASIC ROM still contains the PC-8801 cassette
  code: writing polls TXRDY (21h bit 0) and outputs to 20h (7FD0h);
  reading takes bytes from a buffer filled by the 8251 receive interrupt
  handler (3167h: `IN 21h` RXRDY, `IN 20h`); `MOTOR` sets port 30h bit 3
  through the shadow at E6C0h (7F20h–7F4Bh), and port 30h bits 4–5
  select the 8251 channel.
- `[X88000]` (behaviour reference only, ADR-0016): channel 00 = CMT
  600 baud, 01 = CMT 1200 baud, 1x = RS-232C; bytes arrive while the
  motor is on and receive is enabled; T88 tape images carry tagged data,
  blank, space and mark blocks.

## Scope

1. **Tape images**: load raw byte images (`.cmt`) and T88 (`.t88`);
   save to a raw image.
2. **8251 cassette channel** in 88 mode: port 30h bits 3–5 (motor,
   channel), receive at the selected baud rate with RXRDY and its 8214
   interrupt, transmit to the save image. RS-232C and V3 unchanged.
3. **Frontend**: a cassette menu (set, rewind, eject, save image) and a
   command-line option for headless tests.
4. **Tests**: a romless round trip of the 8251 path, and a BASIC `SAVE`
   then `LOAD` through a tape image.

## Gate G103j (human)

Standard V3 gate unchanged, plus: a BASIC program saved to tape loads
back; the maintainer's tape images load in V1/V2 BASIC.

## Implementation progress

- Implemented (vaeg extension, decision C10): in 88 mode port 30h bits
  5-4 select the cassette channel of the 8251 and bit 3 the motor;
  received bytes raise RXRDY and the 8214 level 0 request; `[ROM]` BASIC
  waits for port 40h bit 2 (carrier, 7EDFh) before reading, which vaeg
  reports while the motor runs and the tape has data. Transmitted bytes
  are recorded to a raw image. Tape images: raw byte streams and T88 data
  blocks (blank, space and mark blocks are not timed). `CMT_Fast`
  (default on) delivers bytes 16 times faster than the baud rate. Tape
  contents and the recording are not part of state files.
- Frontend: Emulate menu bar > Tape (load or record, rewind, eject, stop
  recording, fast load, position); `--tape` and `--tape-save` for
  headless runs.
- Verified headless: a BASIC program saved with `SAVE "CAS:..."` is
  byte-identical across runs and loads back with `LOAD "CAS:..."` and
  runs; three maintainer tape images (two raw, one T88) load in the
  original VA's ROM BASIC (no disk) and their machine-code parts start.
  Disk BASIC loads the BASIC part of a tape, but programs that need the
  memory disk BASIC occupies do not run there; use the ROM BASIC.
- Romless test: T88 data extraction, carrier, two bytes through port
  20h, recording of a transmitted byte, RS-232C selection leaving the
  tape alone, rewind. It fails if port 20h does not read the tape.

| Change | Commit |
|---|---|
| Cassette channel | [ada42eae](https://github.com/nakatamaho/vaeg/commit/ada42eae5572c1fad5895a8e881605626b07da8e) |
| `--tape`, `--tape-save` | [98249d13](https://github.com/nakatamaho/vaeg/commit/98249d13caebff91372395e8fa239fe20754ab77) |
| Tape menu | [f5e0dd46](https://github.com/nakatamaho/vaeg/commit/f5e0dd4697cc36f162fb3301636885a8381a9711) |
| Test | [f1e5be7d](https://github.com/nakatamaho/vaeg/commit/f1e5be7d8bb25053076a51040fe8c06fabfe176f) |

### Tape sound (maintainer request)

- Bytes read from or written to the tape sound as PC-8801 FSK (start bit,
  eight data bits LSB first, stop bit; 0 = 1200 Hz, 1 = 2400 Hz) with the
  2400 Hz carrier between bytes while the motor runs; motor off silences
  it. The sound plays at the real baud rate; with fast load, bytes that
  arrive while one is sounding are skipped. Separate volume: Sound > Tape
  volume (`CMT_vol`, 0-128, default 48). A real-time capture of a load
  shows the carrier, then mixed 1200/2400 Hz during the machine-code
  block. Maintainer check: sounds, but a little high; measured cause: with
  fast load the gaps between sampled bytes were carrier (2400 Hz 74% of
  the time); they now play data (1200 Hz 56%, the data's own ratio about
  59%). Commits [f6f49f57](https://github.com/nakatamaho/vaeg/commit/f6f49f57d67a7c3813252b6f530a9015fbe26a86),
  test [04a60f8c](https://github.com/nakatamaho/vaeg/commit/04a60f8c3fbc1544fe524fd137e4a78a88471727).

### New FDD image sizes (maintainer request, outside the cassette scope)

- FDD > New FDD image also creates unformatted D88 images: 2DD 720 KB
  (80 x 2 x 9 x 512), 2D 320 KB (40 x 2 x 16 x 256, N88-BASIC layout) and
  2D 360 KB (40 x 2 x 9 x 512), every sector present and filled with E5h,
  no file system ("no need to format"). D88 only; the raw loader knows
  none of these sizes. Commits [bc18620f](https://github.com/nakatamaho/vaeg/commit/bc18620f71d9cb24574f6fa3b958eef94ff361af),
  test [a4def8aa](https://github.com/nakatamaho/vaeg/commit/a4def8aac8bea55d5f6b8ef3bea44eead90f218a).

### G103j

- Passed on 2026-10-05 (maintainer): tape sound now close to the
  maintainer's memory; decision C10 approved.
