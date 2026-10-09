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


# M103c - V1/V2 interrupts, keyboard and 88-mode text display

Status: **complete; G103c passed on 2026-10-03**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103c-v1v2-text` off the
integration branch `topic/v1v2-mode` at `main`
`c1c4cd5c33b13e0097de504aaaa4ed6609130f2a` (M103a and M103b merged).
Commit prefix `M103c:`. Plan and hardware model:
[`docs/modernization/v1v2-mode-plan.md`](../../modernization/v1v2-mode-plan.md).

## Goal

Disk BASIC, booted automatically from V2 media as in M103b, shows its
first prompt on screen and echoes typed keys. This needs interrupts
delivered into compatible code, the keyboard reaching BASIC, and the
88-mode text RAM rendered through the TSP's µPD3301-compatible mode.

## Background (established before implementation)

`[VA-TM]` BNN manual §5.2.2. At reset the interrupt system is in
**8214 mode**; writing port 158H switches to 8259 mode (V3), and only a
reset returns to 8214 mode. In 8214 mode the µPD9002 ICU has a
µPD8214-compatible controller as its slave, with eight levels:

| Level | Source | V30 vector | µPD780 vector |
|---|---|---|---|
| INT0 | RS-232C receive | 40h | 00h |
| INT1 | VRTC | 41h | 02h |
| INT2 | general timer 2 (8801-compatible, 600 Hz) | 42h | 04h |
| INT3 | UINT0 (bus) | 43h | 06h |
| INT4 | sound controller | 44h | 08h |
| INT5 | UINT1 (bus) | 45h | 0Ah |
| INT6 | general timer 3 (mouse timer) | 46h | 0Ch |
| INT7 | SGP | 47h | 0Eh |

The controller selects the vector type from the CPU mode at the time of
the interrupt: compatible code takes a µPD780 interrupt (the Z80 `IM 2`
low byte), native code a V30 interrupt through the IVT. This replaces
the earlier `[DERIVED]` guess in the plan (§4) that firmware rewrites a
native frame. Ports: `E4h` OUT current status (bits 2–0 level, bit 3
XSGS: 0 = compare with the current status, 1 = priority only),
initialised with `OUT E4H,7`; `E6h` OUT mask (bit 0 general timer 2,
bit 1 VRTC, bit 2 RS-232C RXRDY; 1 = enable). The ICU initialisation
for 8214 mode masks all ICU inputs except the slave on IR7.

General timer 2 has no 8259-mode interrupt line, and vaeg does not model
it yet. vaeg's interrupt controller models only 8259 mode, and M103b's
key-wait trace (200 000 compatible instructions after 30 million)
contains no interrupt at all.

## Scope

1. **Interrupt mode latch.** 8214 mode at reset; port 158H selects 8259
   mode until reset. Saved state. Confirm by trace that V3 software
   (PC-Engine) writes 158H; if it does not, stop and re-plan, because
   V3 interrupts must not change.
2. **8214-compatible controller.** `E4h`/`E6h`, request latches, the
   acceptance rule and re-arming after acknowledge, with the rule's
   provenance recorded (VA manual for registers; Intel 8214 data sheet /
   PC-8801 behaviour, X88000 as behavioural reference, for acceptance
   and re-arm). In 8214 mode the mapped sources (VRTC, RS-232C receive,
   sound, SGP, mouse timer, bus lines) raise 8214 levels instead of
   8259 lines.
3. **General timer 2.** A 600 Hz event raising INT2 in 8214 mode.
4. **Delivery.** In compatible mode with IFF1 set: Z80 interrupt with
   the µPD780 vector through the adapter's acknowledge path (a dedicated
   virtual acknowledge port; today it is port 0, which is keyboard row
   0). In native mode with IF set: V30 vector 40h + level through the
   IVT. Tests for both, for acceptance/re-arm, masking and save/load.
5. **Keyboard.** Verify that BASIC's interrupt-driven scan of ports
   00h–0Eh reads the existing matrix and fills its key buffer; fix only
   what the trace shows is missing.
6. **88-mode text.** TVRAM at 88-mode `F000h`–`FFFFh` (physical
   `1F000h`, backed by V3 TVRAM `A6000h`) and the TSP 3301-compatible
   rendering of the text the trapped CRTC/DMAC programming describes
   (plan §2.1), enough for the BASIC screen: 80×25/20 text, 8801
   attributes, colours 8–15. ROM handler behaviour for `50h`–`53h` and
   `60h`–`68h` drives what the TSP must be told.

Out of scope: 88-mode graphics display and ALU (M103d), FDD use under
V1/V2 beyond boot, sound, V1 mode, timing accuracy.

## Gate G103c (human)

Standard V3 gate unchanged, plus with the VA2 ROM set and V2 BASIC
media: the disk BASIC banner and first prompt appear on screen, typed
characters echo, and a short BASIC line (`PRINT 1+1`) answers `2`.

## Summary for G103c

Machine-verifiable parts (local): full CTest without failures (108
entries; the external SST and, without private paths configured,
`vaeg_m103b_basic_boot` skipped); V3 smoke passes; with the VA2 ROMs and V2
BASIC media `tools/qa/m103b_basic_boot.py run` shows ` 2` after
`PRINT 1+1` on the 88-mode text screen. Human-gate items: the standard V3
gate, the BASIC screen with typed characters echoing in an interactive
session, and acceptance of the documented policies (8214 acceptance and
re-arm rule, general-timer-2 start on enable, inferred 8Eh/97h semantics,
3301 attribute carry across rows).

Deferred: 3301 semigraphics, 40-column mode, kanji ROM ports, 88-mode
graphics display (M103d), sound under V1/V2, FDD use beyond boot.

## Implementation progress

### 8214-mode interrupts (scope items 1–4)

- Traces before implementation: a V3 boot to PC-Engine writes port 158H
  (the VA2 ROM's shared initialiser `F000:2210` reprograms the ICU for
  8259 mode, ICW4 1Dh/09h); the V1/V2 BASIC boot never writes 158H and
  uses E4h/E6h. At reset the ROM programs the ICU exactly as the manual's
  8214-mode table (`F000:1317`: ICW1 11h, ICW2 00h, ICW3 80h, ICW4 03h =
  auto-EOI, OCW1 7Fh).
- `io/pic.c` now has an 8214 controller as the ICU's IR7 slave in 8214
  mode: E4h current status and XSGS, E6h masks applied at the request
  input, request latches, acceptance only above the current status (or
  any level with XSGS), and no further acceptance until E4h is written
  again. The acceptance and re-arm rule follows the Intel 8214 behaviour
  that 8801 software relies on; the VA manual documents only the
  registers. Port 158H selects 8259 mode until reset.
- Routing in 8214 mode: VRTC (IR2) → INT1, sound (IR12) → INT4, mouse
  timer (IR13) → INT6, SGP (IR8) → INT7, bus UINT0/UINT1 → INT3/INT5,
  RS-232C receive → INT0 (an explicit call; vaeg's IR4 also carries
  transmit). Master IR lines keep their old behaviour (masked by the
  ROM's OCW1); the uPD8259 slave is absent in 8214 mode.
- Delivery: compatible mode takes a Z80 interrupt through a virtual
  acknowledge port (0x10000) that returns the uPD780 vector `2*level`;
  native mode takes V30 vector 40h+level. The 8259 model now honours ICW4
  auto-EOI; only the ROM's 8214-mode setting uses it.
- General timer 2: a 600 Hz event (`NEVENT_GENTIMER2`, saved as `gtm2`)
  that runs only while E6h bit 0 enables it, so an idle timer cannot
  perturb V3 scheduling; the phase of the first tick after enabling is a
  modelling choice.
- State: new section `PIC8214`, version 0. A file without it predates
  8214 mode, and those builds always behaved as 8259 mode, so loading
  keeps 8259 mode in that case.
- Tests: `vaeg_romless_tests` gains an 8214 case (reset state, masks,
  status comparison and XSGS, priority, native vector 41h/44h with AEOI,
  re-arm, Z80 IM 2 taking vector 02h, timer 2, save/load, an older file
  loading as 8259 mode, and port 158H). Removing compatible delivery or
  auto-EOI makes it fail. The 8087 interrupt fixtures and the statsave
  test were written for an always-8259 model; they now select 8259 mode
  as V3 software does, through the same function as the port handler,
  so the M42 reset-state fixture is unchanged. Compatible code reads
  data and operands through DS, so the new fixture sets DS = CS as the
  ROM does before BRKEM2.
- Results: full CTest without failures (108 entries, two skipped); V3
  smoke passes; a V3 PC-Engine boot accepts `dir` and its clock advances.
  With V2 BASIC media, headless input `3`, Enter, `PRINT 1+1`, Enter
  produced in the 88-mode text area (local memory dump, not committed):
  the version line `NEC N-88 BASIC Version 2.4`, `Ok`, `PRINT 1+1`, ` 2`,
  `Ok` — BASIC runs interactively; only the display is missing. The VRTC
  handler scans keyboard rows 0Bh–00h with `IN A,(C)`. The M103b trace
  classifier in `vaeg_m103b_basic_boot` expects the interrupt-free
  key-wait loop and no longer matches; it is replaced by a content check
  once the text is visible (next stage).

### 88-mode text display (scope item 6)

- `[ROM]` TSP commands sent by the VA2 ROM for V1/V2 (local trace, not
  committed): SYNC, DSPDEF `00 00 21 0F 00 18` (16-raster rows; 13h for
  20 rows), ACTSCR, CURDEF, SPRON, `EMUL 8Ch 00 4E 13 18` (split screen 0,
  80 characters, 20 attribute pairs, 25 rows; the 20-row variant ends in
  13h), DSPON with the table at TVRAM 0000h. Each time BASIC reprograms the
  trapped CRTC/DMAC the ROM sends EXIT, then 8Eh/8Fh/97h sequences, then
  EMUL again. 8Eh/8Fh/97h are not in the VA manual's command list; they
  write split-screen table fields while TVRAM is unreachable from the CPU.
  `[DERIVED]` 8Eh sets a TVRAM byte address (the observed values 00h, 08h,
  0Ah and 10h are the offsets of the frame's address, pitch, mode and start
  fields), 97h writes the following parameter bytes from it until the next
  command, and 8Fh takes three bytes whose meaning is unknown (always
  `01 00 00`). BASIC's DMA start F3C8h becomes a start field of 678Ch.
- Later note (M103f): these are the µPD72022's generic display-memory
  commands, documented in [`upd72022-tsp.md`](../../modernization/upd72022-tsp.md)
  §10.6 from the NEC data book though absent from the BNN VA command list.
  8Eh is `DPLD` into DPTR0 (19-bit byte address), 8Fh is `DPLD` into DPTR1
  (a signed increment; `01 00 00` is +1), and 97h is `WDAT` with MOD = 11b,
  which adds DPTR1 after each write. The inference above matches them;
  vaeg still advances by one instead of using DPTR1. See
  [`v1v2-mode-decisions.md`](../../modernization/v1v2-mode-decisions.md) X1.
- `[VA-TM]` §8.2.1 and the TSP chapter: 88-mode F000h–FFFFh is the 4 KiB of
  V3 TVRAM at A6000h; the TSP runs in byte-access mode and EMUL expands
  uPD3301 attributes dynamically. vaeg now maps that window when no GVRAM
  plane is selected and port 32h TMODE (bit 4) is clear (BASIC writes
  A8h/A9h), records EMUL's geometry, stops emulation on EXIT and SYNC, and
  implements 8Eh/97h as above (state in former reserved TSP bytes; the
  state layout is unchanged).
- `[DERIVED]` Byte-mode addressing: TSP local byte 3000h is TVRAM byte
  6000h, and the table's start and pitch fields are twice the byte values
  (pitch F0h = 120-byte rows). The ROM programs the start two characters
  early and rxp = 1008 hides them, matching the TSP's rw/8 + 2 fetch. The
  renderer reads the 3301 row (80 characters, then 20 column/attribute
  pairs), applies pairs in column order with the state carried across
  rows, uses PC-8801 attribute bits (colour: bits 7–5 G/R/B → colour code
  8–15; decoration: secret, blink, reverse, upper line, under line), and
  draws the ANK font. Not yet modelled: semigraphics, 40-column doubling,
  CURS-positioned cursor (the cursor sprite the ROM manages is drawn by
  the existing sprite path).
- Tests: `vaeg_romless_tests` gains a TSP 3301 case (EMUL decoding, 8Eh/97h
  writes ending at EXIT, SYNC stopping emulation; a synthetic frame shows
  the hidden two-character lead, red reverse at column 1 and its carry to
  the next row; the 88-mode window with TMODE, GVRAM precedence and V3).
  Removing the window, the carry or the byte-mode addressing makes it
  fail. The M103b RAM-window/GVRAM fixture now selects TMODE 1 because it
  uses F000h–FFFFh as main RAM.
- Acceptance: `tools/qa/m103b_basic_boot.py` now types `3` and
  `PRINT 1+1` and checks the TVRAM dump written at exit: a row starting
  `PRINT 1+1`, the next row ` 2`, and an `Ok` row in the 88-mode text area
  (stable codes `M103B_NO_TVRAM_DUMP`, `M103B_NO_BASIC_PROMPT`,
  `M103B_NO_ANSWER`, `M103B_WORKER_FAILED`). It passes in about 73 s with
  the VA2 ROMs and V2 BASIC media and fails with `M103B_NO_BASIC_PROMPT` for
  media that boots natively. A local screenshot shows the banner, `Ok`,
  `PRINT 1+1`, ` 2`, the cursor and the reverse function-key row.
- Full CTest without failures (108 entries, two skipped); V3 smoke passes;
  the V3 PC-Engine screen with kanji is unchanged.

### Builds without tests

- The G103c static Windows build exposed two compile/link failures in
  configurations with `VAEG_ENABLE_TESTS=OFF`, which no local preset used:
  M103b's state test needed `io/upd9002_regs.h` outside the
  test-only include guard (on `main`; ledger entry), and this milestone's
  `selftest_select_8259_mode()` was defined inside the 8087 test guard but
  called by the unconditional statsave test (this branch only). Both are
  fixed; `linux-release` now builds and its smoke passes. Building a
  tests-off preset belongs in the local checks before a release build.

### 3301 attribute pairs: X88000 rule (G103c feedback)

- Maintainer report from the G103c build: the N-88 BASIC function-key row
  was not reversed. Its pairs are `(05,00) (11,04) (13,00) (1F,04) …`, the
  other rows `(80,00)` × 20 (local TVRAM dump, not committed). The previous
  renderer sorted pairs and applied each from its own column, so only the
  gaps 11h–12h, 1Fh–20h, … were reversed.
- The VA manual does not define the pair semantics and no µPD3301 data
  sheet is in hand. `[X88000]` X88000 1.5.3 source
  (`x88_1_5_3_src.tar.gz`, SHA-256
  `66bc0b69d5a394e2195b5e277093eebcc045fbb3930c03637aa106394ae83cf3`,
  <https://quagma.sakura.ne.jp/manuke/x88src.html>, declared Public Domain
  Software by its author), `X88ScreenDrawer.cpp`, transparent attribute
  mode: pairs are consumed in memory order, at most one per character
  position; when the first pair's column is not zero, each attribute takes
  effect from the previous pair's column and the first attribute from
  column 0; colour, secret, blink and reverse carry to the next row and
  frame, while upper/under lines start each row clear; the initial
  attribute is E0h (white). vaeg's renderer now follows this rule
  (re-implemented, no code copied). Behavioural reference only, as
  ADR-0016 requires; µPD3301 colour versus monochrome attribute mode is not
  modelled and colour mode is assumed.
- Carrying the attributes across frames exposed an initialisation order
  fault in the first attempt (colour started at 0, so all text was black on
  black); the state is now initialised on first use of each emulation.
- Tests: the TSP 3301 case adds a function-key-style row (reverse 5–16, not
  the gaps), an under line that must not reach the next row, reverse that
  must carry into a colour-only row, and a fresh emulation that starts
  white. Disabling the shift, carrying the lines, or omitting the
  initialisation each fails with its own message. A local screenshot shows
  the function keys as reversed boxes. Full CTest without failures (108
  entries, two skipped); the BASIC acceptance test and the `linux-release`
  build pass.

### MON: the Debug 8800 monitor bank (G103c feedback)

- Maintainer report: `mon` answered `Feature not available`. `[ROM]` N-88
  BASIC's MON (token CAh, handler at `02E4h`) sets port 31h RMODE (bit 2),
  checks for the signature `DB` at 6000h–6001h, jumps to 6002h if present,
  and otherwise restores RMODE and raises error 33 (`Feature not
  available`, entry `4DC1h`). vaeg left 6000h–7FFFh unchanged under RMODE,
  so the signature was absent.
- Correction: with ROM/RAM mode selected (MMODE clear) and RMODE set,
  6000h–7FFFh reads the Debug 8800 bank at varom00 offset 1E000h (CPU
  document §4.4, Appendix C), ahead of the extension ROM. `[DERIVED]`
  0000h–5FFFh stays N88-BASIC: MON keeps executing from 02ECh–02FCh after
  setting RMODE, so on a real VA that range must still hold the BASIC code.
  This contradicts the CPU document's reading of the VA `n80.rom` dump
  (lower 24 KiB showing V3 code), which was already marked as inference;
  the dump method is unknown. The 70h RAM window remains disabled under
  RMODE, as before.
- Result: `mon` reaches the `h]` prompt; `d0` shows `F3 31 A0 E1 C3 E5 3B`
  (the N88-BASIC entry), and `x` shows `A:00 F:PZ---E-- B:0000 D:EDCC
  H:0001 D':FF7B H':0911 IX:0F7C IY:0101 I:F3 PC:0000 SP:E5F9`, identical to
  the real VA2 register dump in CPU document §17.2 in every field except
  `A'F'` and `B'C'`, which the real machine shows as all ones (`FF`,
  `MZ-H-ENC`, `FFFF`) and vaeg as zero. Software in this path does not
  write those two pairs, so the difference is their reset value (M103a
  policy: cleared at hardware reset, unmeasured); the real dump is one
  third-party observation of unknown boot history. Not changed here.
- Test: the production memory test checks the bank at 6000h, the N88
  bytes below it, a word read across 5FFFh/6000h and 7FFFh/8000h, the
  extension ROM not winning, MMODE removing all ROM, and restoration.
  Removing the bank selection makes it fail. Full CTest without failures
  (108 entries, two skipped); BASIC acceptance and `linux-release` pass.

### US-layout keys in V1/V2 (G103c feedback)

- Maintainer report: with the US 101 host layout many characters could not
  be typed in BASIC; the JIS layout seemed fine.
- Demonstrated cause: the US layout produces symbols by synthetic VA key
  taps and SHIFT chords (`sdl2/kbdmap.c` `tap_action`), sending press and
  release in one host event. `keyboard_send` (`io/serial.c`) updated the
  key matrix (ports 00h–0Eh) at once for each, so a tap was down and up
  before the next VRTC. N-88 BASIC scans the matrix in its VRTC interrupt
  and never saw it. V3 software reads the keyboard controller's FIFO, which
  keeps every code, so the same taps worked there; JIS typing uses the
  host's real key timing.
- Correction: the matrix applies a key's release only after the key has
  been visible across two VRTC starts; later events queue behind it in
  order and are released at `sysp4vsyncstart`, before the VRTC interrupt.
  Events that need no delay apply at once as before; the FIFO path is
  unchanged. Two boundaries rather than one is a margin for handlers that
  run late; it limits synthetic taps to about 30 per second.
- Verification: a temporary host-event hook (not committed) typed
  `@=':+*()"_` and Enter through `kbdmap_keydown/keyup` with the US layout:
  without the change nothing reached BASIC, with it the line appeared
  exactly and BASIC answered `Syntax error`. A new `vaeg_romless_tests`
  case covers a tap, a SHIFT chord, an unshifted tap while host SHIFT is
  held (SHIFT lifted and restored), and undelayed real timing; applying
  events directly makes it fail. Full CTest without failures (108 entries,
  two skipped); BASIC acceptance, V3 smoke, a V3 PC-Engine session with the
  US layout and the `linux-release` build pass.

### Alternate-register reset value (maintainer decision)

- With MON working, vaeg's `x` matched the real VA2 dump in CPU document
  §17.2 except `A'F'` and `B'C'`, which no code on the boot-to-MON path
  writes: real `FF`/`MZ-H-ENC`/`FFFF`, vaeg zero (M103a reset policy).
  The maintainer chose to set the alternate set to `FFFFh` at hardware
  reset. vaeg's `x` output now matches the dump in every field (local
  run). The adapter self-test checks the new value and fails with the old
  one. Power-on versus warm reset and the `DE'`/`HL'` values remain
  unmeasured; the maintainer offered a real-machine check (cold power-on
  and warm reset, then V2 BASIC → `mon` → `x`).

### G103c (passed 2026-10-03)

- First Windows build `97375537`: standard V3 gate OK; BASIC prompt, typed
  input, `PRINT 1+1` and FOR/NEXT programs with LIST/RUN OK; policies
  approved. Reported: function keys not reversed, `mon` not available,
  US 101 symbols lost (all fixed above).
- Second Windows build `411e6c8802c298d01a6b344208238ac0c20eee9a`
  (`vaeg.exe` SHA-256
  `3dbccf3b0d5970f44188c22e3622b44ad7f1314f593bfd160903ae44d167abf3`): the
  three fixes confirmed by the maintainer.
- The alternate-register reset value (`13a3431a`) was decided afterwards
  and verified locally (`mon` → `x` equals the real VA2 dump); the
  maintainer passed G103c with it. Candidate:
  `b12cd06f73dd698b600a6764b41798749c247893`.
- The tests-off build fix `5c319d28` was cherry-picked to `main` as
  `10d490c421d13b24d8bd8f46cbeb54075f1e3ca8` before the M103c merge.
