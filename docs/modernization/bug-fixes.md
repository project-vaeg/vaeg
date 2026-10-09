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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# VAEG Fork Bug-Fix Ledger

This is the permanent index of demonstrated correctness fixes in the
maintained VAEG fork. It complements release-oriented `CHANGES*.md` files and
milestone-oriented `docs/agents/tasks/M*.md` records. It is not a feature
list.

The initial historical entries below cover the major active-tree fixes whose
cause and correction can be recovered from the milestone records and commit
history. Smaller build-only and presentation changes remain in their task
documents and git history. New correctness fixes must be added here when they
land.

## Maintenance Rules

### M105 — TSP frames with a sync shorter than 4 lines ran slow

- **Symptom/scope:** frames that set the vertical sync field `VS` below 4
  lines (VIEW480, V480PAT `S` above 400 lines and the `Q` option) ran at a
  lower field rate in vaeg than on a PC-88VA2: `480 S K` (499 lines) ran
  as 502 lines, 49.45 Hz, where the PC-88VA2's monitor read 49.7 Hz. V3
  mode, any program that writes such a SYNC.
- **Demonstrated cause:** `tsp_updateclock` raised `VS` to 4 (the
  uPD72022 data-book minimum) when it computed the frame length; the
  hardware read-outs (`pc88va-video-modes.md` section 12.4: 49.7 Hz for
  499 lines, 53.3 Hz for 467) match the written value.
- **Correction:** only an empty `VS` field is raised (to 1).
- **Verification:** romless test (a 1-line sync gives a frame 499/502 of a
  4-line one; fails with the old minimum).
- **Task/evidence/commit:** [M105 task](../agents/tasks/M105_scanline_fill_tape_leader.md),
  [measurements](pc88va-video-modes.md#124-third-round-sweeps-monitor-read-out-and-g160-rates).
  Fix: [62690c08](https://github.com/nakatamaho/vaeg/commit/62690c0883d8aba19e1da70b40b0678f60374ef6).

### M104 — TSP text stayed visible after a SYNC without DSPON

- **Symptom/scope:** after a program reprogrammed the TSP timing with SYNC
  and did not send DSPON (V480PAT's `S`, `T`, `U`, `R`, `D` runs; VIEW480
  uses the same sequence), vaeg kept drawing PC-Engine's text over the
  pattern, while the PC-88VA2 showed graphics only (M104 photographs, for
  example `va2-240-r-w.jpg`). V3 mode, any program that issues SYNC.
- **Demonstrated cause:** `[uPD72022]` "The SYNC command terminates display
  controller operation"; DSPON starts it again. vaeg's `exec_sync` left
  `tsp.dspon` set.
- **Correction:** SYNC clears `tsp.dspon`; sprites stay as they are (the
  data sheet stops the sprite controller only with DSPOFF).
- **Verification:** romless test (SYNC stops the text, DSPON restarts it,
  sprites untouched; fails without the fix); headless boots of the VA and
  VA2 ROM screens, PC-Engine at 24.8 and 15.98 kHz, V2 N88-BASIC and N mode
  still show their text; G256 keeps the text and V480PAT runs hide it, as
  on the PC-88VA2.
- **Task/evidence/commit:** [M104 task](../agents/tasks/M104_v3_display_timing.md),
  [measurements](pc88va-video-modes.md#12-measured-non-native-timings).
  Fix: [bf55c229](https://github.com/nakatamaho/vaeg/commit/bf55c2292e40dda45b9ddb839bec84b118f0f74a).

### M104 — frames taller than 400 rasters and non-native graphics windows

- **Symptom/scope:** V3 programs that reprogram the TSP frame (VIEW480,
  480-line and 15 kHz 240-line experiments) showed at most 400 rasters, so
  graphics lines past 400 were lost; 24.8 kHz line-doubled 200-line
  graphics kept advancing past line 200; at 15.98 kHz a top blanking under
  37 showed all graphics lines. The PC-88VA2 shows the frame buffer to
  line 479, repeats line 200 when doubling, and loses `37 - TBL` lines.
- **Demonstrated cause:** the SDL canvas, texture and shadow were fixed at
  640 x 400 (`SCRNMNG_CANVAS_HEIGHT`), and the graphics renderer had no
  model of the display circuit's own line window; both against the
  M104 hardware measurements.
- **Correction:** the canvas grows to 400-480 rows with the frame
  ([631e3e98](https://github.com/nakatamaho/vaeg/commit/631e3e980a3883cd7b7a950a612ad206b9a79077)); the
  measured windows are applied per frame through
  `videova_graphics_window` ([ee21db2a](https://github.com/nakatamaho/vaeg/commit/ee21db2a2c953c8319d5fb8c68290401eaf0905d);
  follow-up for a top over 37 and RSM = 00:
  [2cfa871a](https://github.com/nakatamaho/vaeg/commit/2cfa871ae32e0a6e35f56f2ba87279bdaf210e99)).
- **Verification:** romless test (window values, delayed start and line
  repetition; fails with either rule disabled); headless V480PAT sweeps
  `A` and `B` reproduce the last graphics lines photographed on the
  PC-88VA2.
- **Task/evidence/commit:** [measurements](pc88va-video-modes.md#12-measured-non-native-timings),
  [M104 task](../agents/tasks/M104_v3_display_timing.md).

### M104 — N mode from the menu did not boot a disk in drive 1

- **Symptom/scope:** with Emulate > Z80 mode > N, N-BASIC started with
  `Ok` even with a disk in drive 1, while `NEW ON 1` from V1 S booted the
  same disk (maintainer report). Both models, N mode only.
- **Demonstrated cause:** `[ROM]` N-BASIC reads port 40h bit 3 at start-up
  (00CBh `IN A,(40h) / AND 8 / XOR 8`) and skips its disk boot when the
  bit is 1. vaeg's N mode reported the bit as 1 (SW7 OFF) for the whole
  session so that the VA2 ROM enters V1/V2 without a disk; `NEW ON 1` in
  V1 S sees 0.
- **Correction:** the bit is reported only from reset until the first
  compatible-mode entry, i.e. during the VA ROM's boot search.
- **Verification:** romless test (the bit is clear after the compat
  entry; fails without the fix); headless N mode on VA and VA2 with a
  disk whose boot sector writes text: before the fix N-BASIC `Ok`, after
  it the boot sector's text; without a disk N-BASIC still starts.
- **Task/evidence/commit:** [N-BASIC mode](v1v2-n-basic-mode.md#6-booting-directly-in-n-mode-implemented-decision-c9).
  Fix: [39e9df39](https://github.com/nakatamaho/vaeg/commit/39e9df3919902cd378851f710506896067a18bce).

### M103i — BEEP and port sound silent at low master volume

- **Symptom/scope:** with the master volume below about 21 (of 128) the
  buzzer and the port sound were silent while FM sound still played
  (maintainer report; found while checking the port 40h sound fix). All
  modes, SDL frontend.
- **Demonstrated cause:** the slider set the buzzer volume as
  `round(volume × 3 / 128)`, a 0–3 value; every volume up to 21 gave 0.
- **Correction:** the buzzer volume has 1/128 steps (`BEEP_level`), set
  in proportion to the master volume; full scale equals the old maximum.
- **Verification:** romless test (a low master volume keeps the buzzer
  audible; fails with the old rounding); a real-time audio capture of
  `BEEP` and `CMD SING` shows output at default volume.
- **Task/evidence/commit:** [M103i task](../agents/tasks/M103i_v1_mode.md#port-40h-sound-maintainer-report).
  Fix: [1f0d0c16](https://github.com/nakatamaho/vaeg/commit/1f0d0c16f1ad7bb766f270b7c3ac58f73b848de3).

### M103i — V1/V2 BEEP and port sound (CMD SING) were silent

- **Symptom/scope:** in V1/V2 mode, sound made through port 40h was
  silent: the buzzer switched by bit 5 (N-88 BASIC `BEEP`) and the port
  sound driven by bit 7 (the PC-8801mkII `CMD SING` extension). The real
  VA plays both (maintainer report). Both models.
- **Demonstrated cause:** `sysp_o040` stored the latch but did not model
  bits 5 and 7; the buzzer followed only 1CDh bit 3 (XBEEP). The VA
  technical manual 5.10.3 has the buzzer on when either bit is on, and
  5.10.4 has 40h bit 7 drive the speaker directly, enabled by 190h bit 4
  (FBEN). A headless run of the maintainer's `CMD SING` demonstration
  toggles bit 7 about 106,000 times.
- **Correction:** the buzzer takes either bit; FBEEP feeds its own level
  stream through the one-shot beep generator, gated by FBEN.
- **Verification:** romless test of both bits and the FBEN mask (fails
  without the change); maintainer listening check pending.
- **Task/evidence/commit:** [M103i task](../agents/tasks/M103i_v1_mode.md#port-40h-sound-maintainer-report).
  Fix: [fab15174](https://github.com/nakatamaho/vaeg/commit/fab15174b719f38e0c54c8076c7323b40eacf4f5).

### M103h — The V1/V2 cursor stayed at the top left on the original VA

- **Symptom/scope:** on the original PC-88VA model the cursor was shown in
  the top left corner in every V1/V2 mode (maintainer report). Original VA
  only; any software positioning the cursor with TSP CURS.
- **Demonstrated cause:** the TSP model ignored ACTSCR (16h) and CURS (1Eh).
  The original VA's ROM moves the V1/V2 cursor with CURS (traced: row 4,
  cells 2, 4, … while typing); the VA2 ROM writes the cursor sprite
  descriptor itself, so it was unaffected.
- **Correction:** ACTSCR records the split screen; CURS writes the dot
  position of its row and column into the cursor sprite descriptor.
- **Verification:** for the same cursor positions the original VA now gets
  exactly the descriptor values the VA2 ROM writes (Y 40, X 0 and 32), and
  the cursor follows typed text; V3 BASIC and PC-Engine cursors unchanged;
  the romless test fails without the change.
- **Task/evidence/commit:** [M103h task](../agents/tasks/M103h_v1v2_nbasic.md#third-maintainer-check).
  Fix: [4613d9ec](https://github.com/nakatamaho/vaeg/commit/4613d9ec2775df0248d1ecd67689a3d51cff9337).

### M103h — The original VA ignored the V1/V2 selection

- **Symptom/scope:** on the original PC-88VA model, selecting V1 (Emulate >
  Z80 mode) still booted N-88 BASIC Version 2.x (maintainer report:
  V1 H/V1 S had no effect on VA, unlike VA2). Original VA model only.
- **Demonstrated cause:** port 150h (system mode, active low) was never
  initialised. The VA2 ROM sets it through port 1C6h from memory switch
  B1FC5h bit 0 (F000:12E4); the original VA's ROM reads 150h without
  writing 1C6h (F000:0A77), so it saw an unset value and took V2.
- **Correction:** reset starts port 150h from B1FC5h bit 0; the VA2 ROM
  rewrites the same value. How the original VA hardware chooses the mode
  is not documented (decision C8).
- **Verification:** with V1 selected the original VA now boots N-88 BASIC
  Version 1.9 (before: 2.4) and V2 still boots 2.4; the romless test reads
  port 150h after reset and fails without the change.
- **Task/evidence/commit:** [M103h task](../agents/tasks/M103h_v1v2_nbasic.md#second-maintainer-check).
  Fix: [827a3a67](https://github.com/nakatamaho/vaeg/commit/827a3a6799a0868ad80a6a5fc3b844c750d0e9df).

### M103h — Saved backup memory lost its memory switches at every boot

- **Symptom/scope:** with a saved backup-memory file, the VA2 ROM restored
  the default memory-switch record (B1FC0h–B1FC7h) at every boot, so
  settings stored there did not survive a restart; the RAM-size byte
  B1FC4h reverted from 64h to 63h. All modes, whenever backup memory is
  saved.
- **Demonstrated cause:** at reset the ROM sets bit 7 of B1FC6h, sums the
  record and compares the sum with B1FCDh (F000:23B7); it recomputes bit 7
  from the V1/V2 selection afterwards, leaving 79h after a V2 boot.
  `bkupmemva_sync_mainram` summed the saved 79h, so the checksum differed
  by 80h and the ROM's restore path (F000:23F9) ran (traced byte writes).
- **Correction:** the checksum is computed with B1FC6h bit 7 set.
- **Verification:** three boots in a row with one backup file keep the
  record unchanged (before: restored on the second); the romless test fails
  with the old rule.
- **Task/evidence/commit:** [M103h task](../agents/tasks/M103h_v1v2_nbasic.md#mode-selection-scope-item-1).
  Fix: [d7257d77](https://github.com/nakatamaho/vaeg/commit/d7257d77122fa42f857c1ab8eec321bf3bae73cb).

### M103g — Interrupts were not delivered through short EI windows in V1/V2 mode

- **Symptom/scope:** in the PC-8801mkIISR demonstration's winter scene the
  music did not start with the scene and played only briefly at its end
  (maintainer report). Any V1/V2 software that keeps interrupts disabled
  most of the time and enables them only briefly; native V3 code is not
  affected.
- **Demonstrated cause:** the scene scrolls GVRAM with `DI / LDDR / EI`
  twenty times per frame. The machine arbitrates interrupts only between
  CPU time slices (`pic_irq`) and only while interrupts are enabled; the
  native `STI` ends the slice one instruction later when a request waits,
  compatible-mode `EI` did not. A counter showed the sound timer raising
  about 75 requests per second while none was delivered for about 14 s,
  with the 8214 holding the request and the CPU sampled in the `LDDR`.
- **Correction:** when compatible-mode code sets IFF1 and a request is
  pending, the slice ends one instruction later, as for `STI`.
- **Verification:** in the same scene every request is now accepted (the
  counters match); the romless 8214 test fails without the change. Audible
  confirmation is part of G103g.
- **Task/evidence/commit:** [M103g task](../agents/tasks/M103g_v1v2_semigraphics_and_sound.md#sound-timing-scope-item-3).
  Fix: [01381a9f](https://github.com/nakatamaho/vaeg/commit/01381a9f2b9102fd6533ae8fd0203652f4be2711).

### M103f — V3 leftovers in GVRAM plane 3 showed through V1/V2 graphics

- **Symptom/scope:** after running a V3 game and resetting into V1/V2
  mode, V2 BASIC graphics (after `PAINT`) and the PC-8801mkIISR
  demonstration showed stray dots (maintainer report, G103d/G103e check).
  Multiplane 4 bit/pixel display, V1/V2 mode and any V3 software that
  clears 110h bit 7.
- **Demonstrated cause:** the multiplane 4 bit/pixel renderer always
  included plane 3, ignoring 110h bit 7 (G3MSK, plane 3 screen switch).
  The VA2 ROM sets 110h to 7F47h for V1/V2 (G3MSK clear, as BNN 8.3.2
  requires), and 88-mode software cannot reach plane 3, so earlier V3
  contents stayed visible. The self-test shows plane 3 data reaching the
  pixel with G3MSK clear.
- **Correction:** plane 3 is masked out of 4 bit/pixel pixels while G3MSK
  is clear.
- **Verification:** the romless self-test sets plane 3 data and checks the
  pixel with G3MSK clear and set; it fails without the mask. The reported
  V3 game was not run here; confirmation with it is part of the gate.
- **Task/evidence/commit:** [M103f task](../agents/tasks/M103f_v1v2_text_and_media.md#gate-feedback).
  Fix: [0a5cb5c9](https://github.com/nakatamaho/vaeg/commit/0a5cb5c98889b390b60b8f6df5bbaadfc8b1b041).

### M103e — D88 tracks were cut short by track 0 data read as pointers

- **Symptom/scope:** a PC-8801MA2 2HD disk hung while loading
  N88-日本語BASIC: a 26-sector read of cylinder 4, head 1 ended after 17
  sectors with "No Data", and the loader retried forever. Any D88 image
  with a 160-entry track table, where one of entries 160–163 decodes to an
  offset inside the image, is affected for the track containing that
  offset, in every mode.
- **Demonstrated cause:** the loader reads 164 track pointers, but such
  images start track 0 at 2A0h. Entry 160 is track 0's first sector ID
  (C=0 H=0 R=1 N=0), read as 10000h; `nexttrackptr` took it as the end of
  the track starting at EE40h, so `d88trk_read` loaded only 4544 bytes and
  sector 18 onward was not found (FDC trace and image table dump).
- **Correction:** the first track is the lowest entry 0–159 at or above
  2A0h; entries at or beyond it are ignored in memory. The header bytes
  are not modified.
- **Verification:** the disk now loads N88-日本語BASIC and runs its
  demonstration; the romless self-test builds a 160-entry
  image and fails without the fix, and a 164-entry image keeps entry 160.
- **Task/evidence/commit:** [M103e task](../agents/tasks/M103e_v1v2_demo_compatibility.md#ma2-demo).
  Fix: [4e1c7a0d](https://github.com/nakatamaho/vaeg/commit/4e1c7a0d372b283e80f71dbb1fc8f02eca4fc882).

### M103d — Graphics statements corrupted the text screen and restarted V2 BASIC

- **Symptom/scope:** in V1/V2 N-88 BASIC, `LINE` drew its dots into the
  text screen as stray characters, and a following graphics statement such
  as `CIRCLE` restarted BASIC at its `How many files` prompt. V1/V2 mode
  only; V3 software never uses ports 34h/35h.
- **Demonstrated cause:** BASIC's plotting code selects extended GVRAM
  access (port 32h bit 6), which vaeg did not model, so its writes reached
  main RAM and the 88-mode TVRAM window. In addition the VA2 ROM's text trap
  handler saves ports 34h/35h with `IN` and restores them with `OUT`; vaeg
  returned FFh, so the restore set 35h GAM and the next `LINE` mapped
  C000h–FFFFh, including BASIC's stack at E5xxh, to GVRAM (trace and ROM
  disassembly in the task).
- **Correction:** extended access with the per-plane ALU, latch and
  comparison reads, GAM mapping, and 34h/35h reading back the last value
  written.
- **Verification:** the drawn figures appear in GVRAM in the expected
  colours and `POINT` returns them; the romless self-test fails when the
  read-back or the comparison read is removed; the V2 BASIC acceptance test
  draws and paints a circle and round-trips a program through the disk.
- **Task/evidence/commit:** [M103d task](../agents/tasks/M103d_v1v2_graphics_and_disk.md#extended-access-scope-item-1).
  Fix: [e3ef809c](https://github.com/nakatamaho/vaeg/commit/e3ef809c8a3fbb788f2a051d1da7580a7490af73).

### M103c — Alternate registers started at zero instead of all ones

- **Symptom/scope:** after a hardware reset, Z80 `AF'` and `BC'` (and
  `DE'`/`HL'`) read `0000h` in compatible mode; a real PC-88VA2's Debug 8800
  `x` command in V1/V2 mode shows `A'=FF`, `F'=MZ-H-ENC`, `B'=FFFF` on a path
  where no code writes them. Visible to any compatible-mode code that reads
  the alternate set before writing it.
- **Demonstrated cause:** M103a chose zero for the hardware-reset value as an
  unmeasured policy; the §17.2 real-machine dump disagrees, and vaeg running
  the same ROM path reproduces every other field of that dump.
- **Correction:** hardware reset sets all four pairs to `FFFFh`. Power-on
  versus warm-reset behaviour and the `DE'`/`HL'` values are not measured.
- **Verification:** vaeg's V1/V2 `mon` → `x` output now equals the real
  dump in every field; the adapter self-test fails with the old value; full
  local CTest without failures (108 entries, two skipped).
- **Task/evidence/commit:** [M103c task](../agents/tasks/M103c_v1v2_interrupts_and_text.md#alternate-register-reset-value-maintainer-decision),
  [CPU document §17.2](upd9002-upd70008-mode.md).
  Fix: [13a3431a](https://github.com/nakatamaho/vaeg/commit/13a3431a0502663258a0251841b8430f1ee561c7).

### M103c — US-layout symbols were lost in matrix-scanning software

- **Symptom/scope:** with the US 101 host layout, symbols that the layout
  produces by synthetic taps or SHIFT chords (and Enter) did not reach V1/V2
  N-88 BASIC; JIS typing and V3 software were unaffected. Any guest that
  reads the key matrix (ports 00h–0Eh) once per frame is affected.
- **Demonstrated cause:** `keyboard_send` updated the matrix immediately for
  both the press and the release of a tap sent in one host event, so the key
  was never down when the VRTC handler scanned the matrix.
- **Correction:** a release becomes visible only after its key has been down
  across two VRTC starts; following events keep their order behind it and
  are applied at the start of VRTC. The keyboard controller FIFO is unchanged.
- **Verification:** a local host-event run with the US layout typed
  `@=':+*()"_` into BASIC with the fix and nothing without it; a
  `vaeg_romless_tests` case fails when events are applied directly; full
  local CTest without failures (108 entries, two skipped).
- **Task/evidence/commit:** [M103c task](../agents/tasks/M103c_v1v2_interrupts_and_text.md#us-layout-keys-in-v1v2-g103c-feedback).
  Fix: [2d5e471b](https://github.com/nakatamaho/vaeg/commit/2d5e471be95ccd0eaca804d85a71fd2bb17f4e72).

### M103c — Builds without tests failed after the M103b merge

- **Symptom/scope:** every configuration with `VAEG_ENABLE_TESTS=OFF`
  (`linux-release`, `mingw-release`, the static Windows container build)
  failed to compile `sdl2/selftest.c` with `upd9002_iotrap` undeclared, on
  `main` from the M103b merge until this fix. Test-enabled presets, which
  all local and CTest runs used, were unaffected.
- **Demonstrated cause:** the unconditional V1/V2 state-section test added in
  M103b uses `upd9002_iotrap`, but its header was included only inside the
  `VAEG_UPD780_INTEGRATION_TESTING` block.
- **Correction:** include `io/upd9002_regs.h` unconditionally.
- **Verification:** `linux-release` builds and its romless smoke passes; the
  static Windows container build succeeds; full CTest on `linux-ci-gcc`
  without failures (108 entries, two skipped).
- **Task/evidence/commit:** [M103c task](../agents/tasks/M103c_v1v2_interrupts_and_text.md#builds-without-tests).
  Introduced by [667dddd4](https://github.com/nakatamaho/vaeg/commit/667dddd4c8a75aae33144ed26a8ae74ec798ecc5);
  fix: [5c319d28](https://github.com/nakatamaho/vaeg/commit/5c319d28c8da441b0bd82359803b9476eb62c8a0).

### M103b — FDD interface fast transfer read garbage from port FDh

- **Symptom/scope:** a PC-8801-style disk loader using the 8255 fast
  protocol (main ports A and B both input) read header words `FF00h`
  and then waited forever. Affects any guest code that reads main port
  FDh or writes main port FCh; V3 transfers were unaffected.
- **Demonstrated cause:** of the four cross-wired 8255 data paths only
  main B→sub A and sub B→main A were connected; the main side also had no
  `IN FDh`/`OUT FCh` handlers, so sub port A output never reached the main.
- **Correction:** connect sub A→main B and main A→sub B through the
  existing callbacks and attach the missing main handlers. Saved `_I8255`
  layouts are unchanged.
- **Verification:** a production-seam test exchanging distinct bytes on
  all four paths fails without the fix and passes with it (`vaeg
  --selftest`); full local CTest has no failures (106 entries, one external
  SST skipped). Integration BASIC then receives a valid header and
  completes the transfer; BASIC boot completion is not yet established.
- **Task/evidence/commit:** [M103b FDD fast-transfer data path](../agents/tasks/M103b_v1v2_mode_plumbing.md#fdd-fast-transfer-data-path).
  Fix: [331e6759](https://github.com/nakatamaho/vaeg/commit/331e6759a20de2a04b98554d98238a8f8c605001).

### M103b — Memory-mode control readback ignored guest writes

- **Symptom/scope:** port `153h` bit 6 always read as V3 even after the
  firmware cleared it before BRKEM2, including word I/O at port `152h`.
- **Demonstrated cause:** `memctrlva_o153` discarded bit 6 and
  `memctrlva_i153` unconditionally ORed `40h` into the result.
- **Correction:** latch the inverted mode request independently of CPU
  execution mode, return it on reads, and clear the request on reset.
  A separate optional `MEM88MODE` state section avoids changing the old
  `MEMORYVA` layout. This fixes readback only: the 88-mode address decoder
  and successful BASIC execution remain pending.
- **Verification:** production byte/word I/O and reset regression passes;
  full local CTest has no failures (106 entries, one external SST skipped).
  Full state-file round trips remain unverified.
- **Task/evidence/commit:** [M103b implementation progress](../agents/tasks/M103b_v1v2_mode_plumbing.md#implementation-progress),
  [automatic handoff traces](v1v2-mode-plan.md#52-bounded-automatic-selection-traces).
  Fix: [cd7d16e4](https://github.com/nakatamaho/vaeg/commit/cd7d16e4c92ee8ff4c0ef8017cb4db93100f3b95).

### M103a — CALLN round trip set the compatible N flag

- **Symptom/scope:** after a native handler entered by `CALLN` (or by an
  interrupt taken in compatible mode) returned with `IRET`, compatible F
  bit 1 (N) read as one and bits 3/5 as zero, whatever F held before. On a
  real PC-88VA2, `CALLN 91h` returns F unchanged (00h stays 00h, C5h stays
  C5h); vaeg returned 02h and C7h.
- **Demonstrated cause:** the native `IRET` normalises every restored frame
  with `(flag & 0FD7h) | F002h`, including frames that resume compatible
  mode, whose low byte is the compatible F.
- **Correction:** when the `IRET` resumes compatible mode, bits 1, 3 and 5
  of the frame are kept; native returns are unchanged. Bits 3/5 are policy,
  not measured.
- **Verification:** the M103a ALTPRB real-machine run; the adapter self-test
  (`XOR A`, `CALLN`, `IRET`, F=44h) fails with the old masking and passes;
  full local CTest without failures (106 entries, one external SST skipped).
- **Task/evidence/commit:** [M103a ALTPRB results](../agents/tasks/M103a_alternate_register_storage.md#altprb-results-and-the-f-bit-1-correction),
  [ALTPRB outputs](../agents/reports/m103a_altregs_qa/README.md).
  Fix: [5ed797b2](https://github.com/nakatamaho/vaeg/commit/5ed797b28bb413af27e02ba458d4d8444e6ed982).

### M103a — States saved in compatible mode could not be resumed

- **Symptom/scope:** loading a state saved while compatible code (for
  example CP/MVA at `A>`) waited in a native handler entered by `CALLN`
  or by an interrupt hung the guest and then crashed; the PC-Engine BIOS
  reported a disk error. Present since M76 (reproduced at the M102 merge).
  States saved in native mode were unaffected.
- **Demonstrated cause:** UPD9CPU export/import skips the two padding bytes
  that hold the execution mode and the return-pending flag, and the return
  SP was a core static that was never saved. After loading, the flag was
  clear, the UPD9Z80 blob was skipped, and the handler's IRET returned into
  the Z80 code as native code.
- **Correction:** a new optional `UPD9MODE` section (mode, flag, return SP)
  between UPD9CPU and UPD9Z80. Older files without it whose UPD9Z80 payload
  shows active compatible code are refused with an explicit message, since
  the missing data cannot be reconstructed; native-mode older files load.
- **Verification:** headless CP/MVA save/load at `A>` lists the directory
  after loading in the same and in a fresh process; an end-to-end
  `vaeg_romless_tests` case and the adapter self-test; full local CTest
  without failures (106 entries, one external SST skipped).
- **Task/evidence/commit:** [M103a compatible-mode state save/load](../agents/tasks/M103a_alternate_register_storage.md#compatible-mode-state-saveload).
  Fix: [777d385a](https://github.com/nakatamaho/vaeg/commit/777d385a698e4a15acb59ca1c9ef1bb869b855d6).

### M99z40 — Small CRT windows overemphasize RGB-mask interference

- **Symptom/scope:** x1 and x2 CRT presentation could show conspicuous
  vertical RGB-mask interference and moire, even though the source pixels and
  scanline treatment were otherwise sharp.
- **Demonstrated cause:** the mask is a three-column raster pattern. At small
  output scales its contrast is not sufficiently resolved by the available
  output pixels, so the mask pattern beats with the displayed content.
- **Correction:** apply an automatic mask-intensity profile of 0.15 at x1,
  0.20 at x2, and 0.30 at x3 or larger. The profile changes mask contrast
  only; source padding, texel copies, scanline averaging and raw capture are
  unchanged. Manual mask control remains available in CRT settings.
- **Verification:** macOS arm64 build, focused librashader tests, ROM-less
  selftest, encoding/EOL/case checks and diff validation pass. Physical visual
  comparison at x1/x2/x3 remains a maintainer check.
- **Task/evidence/commit:** [M99z40 report](../agents/reports/m99z40_crt_mask_profile.md).
  Fix: [6cf059bb](https://github.com/nakatamaho/vaeg/commit/6cf059bb3c57a4bb0726ae37a6ae31d5d0faac94).

### M99z28 — Inset CRT viewports produce vertical mask bands

- **Symptom/scope:** the default CRT shows broad vertical bands when the guest
  viewport occupies only part of the native output target.
- **Demonstrated cause:** `vTexCoord * OutputSize` incorrectly treats the
  target dimensions as the viewport dimensions. This compresses the nominal
  pixel mask. A 640px viewport in a 1000px target reproduces a 24px beat.
- **Correction:** derive mask coordinates from `gl_FragCoord.xy` (D3D11
  `SV_Position`). Image sampling, curve, padding, scanline AA and raw capture
  are unchanged; the correction does not blur the guest image.
- **Verification:** actual inset-viewport EGL raster tests for all four masks,
  controlled old-coordinate negative test, exact unmasked-image A/B,
  GLSL/HLSL compilation, metadata and macOS ON/OFF tests. Corrected physical
  D3D11 display remains a maintainer check; software rendering is not that gate.
- **Task/evidence/commit:** [M99z28 report](../agents/reports/m99z10_renderer_settings.md#m99z28--raster-anchored-rgb-mask).
  Fix: [dadbd38f](https://github.com/nakatamaho/vaeg/commit/dadbd38fafcac017685239513a0cfe34b0d6e4ff).

### M99z27 — CRT AA mask compilation fails on D3D11

- **Symptom/scope:** the M99z26 default preset fails filter-chain creation on
  Windows; native pass-through remains active.
- **Demonstrated cause:** the maintainer's D3DCompile diagnostic is X3500,
  `array reference cannot be used as an l-value; not natively addressable`.
  The AA mask used a dynamically indexed vector write, `mask[channel]`.
  GLSL validation alone did not detect this D3D11 compilation restriction.
- **Correction:** replace that write with constant RGB component assignments;
  leave scanline AA, color equations, texture taps and raw capture unchanged.
- **Verification:** mask/source regression, GLSL and HLSL round-trip compilation,
  all-mask software-raster parity, runtime metadata, macOS ON/OFF tests and
  MinGW compilation. A real-runtime WARP probe is provided; local Wine 8
  cannot load the pinned DLL because `bcryptprimitives.dll` is absent.
  Corrected physical D3D11 playback is still a maintainer check, not a PASS.
- **Task/evidence/commit:** [M99z27 report](../agents/reports/m99z10_renderer_settings.md#m99z27--d3d11-mask-compilation-and-pass-through).
  Fix: [c3efc946](https://github.com/nakatamaho/vaeg/commit/c3efc946799a33e00f6ac739b240711bf8b626f5).

### M99z26 — CRT scanlines alias into broad horizontal bands

- **Symptom/scope:** at small/non-integer display sizes the default CRT
  produced broad horizontal bands, even with curvature disabled.
- **Demonstrated cause:** point-sampled periodic beam brightness aliases when
  the source-row/output-pixel ratio cannot resolve the scanlines. A neutral
  410-row synthetic input rendered to 400 pixels reproduces a 40px beat.
- **Correction:** the VAeg CRT AA pass integrates the brightness envelope
  over the local pixel footprint and fades unresolved contrast at 2-4 pixels
  per source row. Original color weights/kernel, padding and raw QA remain.
  The audited external shader is retained unchanged.
- **Verification:** production float-math integral/oracle tests, EGL/llvmpipe
  raster comparisons, actual runtime metadata, shader compilation, macOS
  ON/OFF and MinGW builds. No physical Windows GPU success claimed.
- **Task/evidence:** [M99z26 report](../agents/reports/m99z10_renderer_settings.md#m99z26--scanline-brightness-antialiasing).
  Fix: [058e238c](https://github.com/nakatamaho/vaeg/commit/058e238cb113ed8da9ae57895e09eba13944ad65).

### M99z24 — Pre-CRT size adjustment blurred source dots

- **Symptom/scope:** SCREEN_SIZE below 100 softened the image before CRT;
  the maintainer confirmed that 100 restored sharpness.
- **Cause:** the owned size pass linearly sampled a scaled image into a
  source-sized target, blending original neighboring pixels before CRT.
- **Correction:** integer black padding with 1:1 row copies, then a texel-copy
  shader and the unchanged CRT pass. Preserve aspect and symmetric margins;
  sizing is quantized. Raw QA and unprocessed capture remain unchanged.
- **Verification:** byte-exact source/border tests in both formats/origins,
  runtime metadata + fake-presenter retry integration, shader compilation,
  macOS ON/OFF and MinGW builds. Physical Windows visual confirmation pending.
- **Task/evidence:** [M99z24 report](../agents/reports/m99z10_renderer_settings.md#m99z24--pixel-preserving-padding-before-crt).
  Fix: [6bcda1e0](https://github.com/nakatamaho/vaeg/commit/6bcda1e02de3aae94b1605a01cd1c93e69f1b70d).

### M99z22 — CRT settings bypassed main configuration controls

- **Symptom/scope:** CRT parameter changes used a hard-coded standalone file,
  separately from the `--cfg` destination and `--no-cfg` persistence policy.
- **Cause:** both the GUI and presenter unconditionally supplied the separate
  parameter-state filename to the file writer, outside `ini.c`'s controls.
- **Correction:** bind a session buffer stored as `NativeCRTParameters` in the
  main configuration and use the existing save policy. Old state is ignored,
  with no import or deletion as requested by the maintainer.
- **Verification:** bounded transactional codec tests, main-ini round-trip,
  disabled persistence tests and local feature-on/off plus MinGW builds.
- **Task/evidence:** [M99z22 report](../agents/reports/m99z10_renderer_settings.md#m99z22--crt-parameters-in-the-main-configuration).
  Fix: [d8105e3c](https://github.com/nakatamaho/vaeg/commit/d8105e3c8da3d10f958756b9e1b35a4b2a404eb1).

### M99z20 — Default CRT preset failed parameter enumeration

- **Symptom/scope:** the M99z18 bundled two-pass CRT preset stopped working
  with "shader parameter enumeration failed". The shared librashader 0.12.0
  preprocessor is affected; reproduced locally using its official macOS runtime.
- **Demonstrated cause:** the VAeg size shader began with its BSD comment,
  rather than `#version 450`. The actual C API returned
  `PreprocessError(MissingVersionHeader)`; standalone glslang had accepted it.
- **Correction:** put the required version directive first without changing
  the license notice or shader math. Refresh distribution hashes and expose
  runtime error details in the GUI instead of hiding them behind a generic error.
- **Verification:** actual C API enumerates all nine parameters after the fix;
  adding just a leading comment to a passing temporary fixture reproduces the
  exact MissingVersionHeader error. Focused local builds/tests pass; physical
  Windows GPU revalidation remains pending.
- **Task/evidence:** [M99z20 report](../agents/reports/m99z10_renderer_settings.md#m99z20--librashader-version-header-regression).
  Fix: [0730d507](https://github.com/nakatamaho/vaeg/commit/0730d507e44a4e5fc4eb4d827763b5b93cbdabcf).

For every new entry, record:

- observed symptom and affected model/platform;
### Screen-text Copy included horizontal guard cells and mispaired kanji

- **Status:** fixed in the hotfix commit.
- **Symptom:** copying a VA directory screen could append the next row's first
  two characters at the right edge. Japanese text also became intermittently
  displaced when half-width and kanji cells were mixed.
- **Root cause:** the Copy path used the frame width value directly even though
  the VA renderer adds two horizontal guard cells, and it treated the HCCODE
  right-half marker independently instead of consuming a matching left/right
  pair. Shift-JIS trail selection must use JIS row parity, not the screen
  column parity.
- **Correction:** Copy now limits rows to the rendered 80-/40-column viewport,
  collapses matching HCCODE left/right pairs into one kanji, and retains the
  JIS-row-parity Shift-JIS conversion.
- **Verification:** MinGW cross-build completed successfully after the change;
  the copy width and pair handling mirror `vram/maketextva.c` and the
  existing TVRAM diagnostic decoder's HCCODE pair contract.
- **Evidence:** [Copy implementation](../../sdl2/gui/gui.cpp#L295).
- **Commit:** [6bbb8429](https://github.com/nakatamaho/vaeg/commit/6bbb842990b0503601df5aa933557f28960da30b).

- demonstrated root cause, clearly separated from rejected hypotheses;
- correction and compatibility boundary;
- automated and human verification actually performed;
- milestone/task document and fixing commit;
- status: `fixed`, `accepted parity correction`, `open`, or `reverted`.

Do not mark a defect fixed solely because a plausible code difference was
found. If the human reproduction still fails, retain the finding as a
separate parity correction or move it to Open Defects.

## Fixed Defects

### Zilog BIT n,(IX+d)/(IY+d) took X/Y from a stale WZ

- **Status:** fixed on `topic/m102-upd9002-undocumented-opcodes`.
- **Symptom/scope:** under the Zilog flag profile, `BIT n,(IX+d)` and
  `BIT n,(IY+d)` set F bits 5 and 3 from whatever WZ (MEMPTR) held before the
  instruction instead of from the effective address, as a Zilog Z80 does.
  Affected: the FDC `UPD780C`, which uses the Zilog profile. The uPD9002
  profile is not affected because R1 keeps bits 5/3 at 0.
- **Demonstrated root cause:** the vendored core's `OP_IX4`/`OP_IY4` did not
  update WZ before dispatching the IX/IY-prefixed CB instruction; the BIT
  handlers read WZ for X/Y.
- **Correction:** the vendored core was rebased onto upstream master
  `ab97d3f` (1.11.0), which sets WZ = IX/IY + d in `OP_IX4`/`OP_IY4`
  ([ADR-0011](../agents/DECISIONS/ADR-0011-z80-migration.md#m102-rebase-on-upstream-1110)).
- **Verification:** `vaeg_z80_compat_flag_profile` checks F and WZ for
  `BIT 0/7,(IX/IY+d)` under both profiles (8 failures with the previous
  core); the host `CBPRB` Zilog output now has X/Y from the effective
  address in the BIT rows, and the uPD9002 host outputs are byte-identical
  to the previous reference. ZEXDOC/ZEXALL still pass 67/67.
- **Task/evidence/commit:** [M102 task](../agents/tasks/M102_upd9002_undocumented_opcodes.md);
  fix [bc890bf8](https://github.com/nakatamaho/vaeg/commit/bc890bf810917824a336d3bdbbcf44d9e3408cb2),
  test [17171b18](https://github.com/nakatamaho/vaeg/commit/17171b18a72d186c5bf7b05b6b90fa8de1c5340f).

### Undefined ED vector page read as zeros after loading a state

- **Status:** fixed on `topic/m103a-alt-registers`.
- **Symptom/scope:** when a state saved in the uPD70008-compatible mode was
  loaded into a fresh vaeg process (no mode entry since start), the
  undefined ED 00–3F/74/75/77 returned 0000h instead of the guest's IVT
  words until the next BRKEM, so guest code resumed from such a state saw
  different values than before the save.
- **Demonstrated root cause:** `StateLoad()` initialised the compat core
  with `SetFlagProfile` and `Init` but, unlike `Enter()`, never installed
  the `SetNativeVectorRead` callback added in M102; the core's default
  reader returns 0x00.
- **Correction:** `Enter()` and `StateLoad()` share `InitializeCore()`,
  which installs the reader.
- **Verification:** `upd9002_upd70008_alt_regs_selftest` (test
  `vaeg_upd9002_brkem_upd70008`) loads a compat blob before any entry and
  executes `ED 20` with the IVT word 2800h at bytes 30h/31h; it fails with
  0000h without the fix.
- **Task/evidence/commit:** [M103a task](../agents/tasks/M103a_alternate_register_storage.md);
  fix [b441a558](https://github.com/nakatamaho/vaeg/commit/b441a558456fd39c555a2761e986c1012de4863d).

### uPD9002 undefined ED opcodes followed the Zilog Z80

- **Status:** fixed on `topic/m102-upd9002-undocumented-opcodes`.
- **Symptom/scope:** in the uPD70008-compatible main-CPU mode, the
  undefined ED opcodes executed as two-byte NOPs or Zilog NEG/RETN
  duplicates. On a real µPD9002 they are a large, visible instruction set:
  loads from the native-side interrupt vector table, RRD/RLD variants,
  word block transfers and three-byte no-operations. Guest software
  running in V1/V2/V3 Z80 emulation observed wrong register, flag and
  memory results for all of them.
- **Demonstrated root cause:** the vendored core had no measured µPD9002
  behavior for these encodings; eight real-machine probe runs
  ([QA outputs](../agents/reports/m102_zexund_qa/README.md), runs 2–8)
  established rules R14–R19, including the reduction of the ED 00–3F value
  to a C-indexed read of the x86 IVT page (EDPRB3 isolation, INPRB
  refutation of the I/O hypothesis, EDPRB4 full sweep).
- **Correction:** R14–R19 in the `Upd9002` profile of the vendored core
  ([patch](../agents/reports/m102_upd9002_undefined_ed_semantics.patch)),
  with the native page read wired through
  `Z80CompatCpu::SetNativeVectorRead` to `upd9002_memoryread` so V3-mode
  guests read the live IVT. Unmeasurable mode exits (ED 54/55/AC/EE–FC),
  the ED 5C hang and the ED 5D fault keep the Zilog fallback.
- **Verification:** the uPD9002 host references reproduce all 911
  real-machine probe records byte for byte (EDPRB2 513, EDPRB2S 27,
  EDPRB3 28, EDPRB4 256, run-2 EDPRB 67) with the run-8 IVT fixture in the
  CP/M runner; the wrapper contract test asserts the new semantics; the
  upstream suite and ctest (112/112) pass; Zilog profile outputs are
  unchanged.
- **Task/evidence/commit:** [M102 task](../agents/tasks/M102_upd9002_undocumented_opcodes.md);
  fix [d90a6cd9](https://github.com/nakatamaho/vaeg/commit/d90a6cd997086269f297e56e09e20aafee7c408f),
  wiring [2125b159](https://github.com/nakatamaho/vaeg/commit/2125b159c2b0a0235601083e0046513246cca453),
  runner [97f04efe](https://github.com/nakatamaho/vaeg/commit/97f04efe53ed070834aed42f14142fb47712591f).

### uPD9002 DDCB/FDCB register forms followed the Zilog Z80

- **Status:** fixed on `topic/m102-upd9002-undocumented-opcodes`.
- **Symptom/scope:** in the uPD70008-compatible main-CPU mode, the
  undocumented DD/FD CB d xx forms with a register operand (low three bits
  not 6) behaved as on a Zilog Z80: BIT tested the memory byte, and RES/SET
  modified memory and copied the result to the register. A real PC-88VA2
  tests and modifies the register only.
- **Demonstrated root cause:** the `Upd9002` flag profile had no rule for
  these forms, so the core used its Zilog implementation. The M102 probe
  `CBPRB` recorded all 512 opcodes with two input sets on the real machine.
- **Correction:** R12 (BIT n,(i+d),r tests r) and R13 (RES/SET n,(i+d),r
  modify r only; memory is not written) in the `Upd9002` profile
  ([patch](../agents/reports/m102_suzukiplan_ddcb_register_forms.patch),
  upstream [#64](https://github.com/suzukiplan/z80/pull/64)). The rotate
  forms keep the Zilog behavior, which the probe confirmed.
- **Verification:** `vaeg_z80_compat_flag_profile` re-executes all 1,024
  real CBPRB records (560 fail without R12/R13); the host `CBPRB` output is
  byte-identical to the real one, and the `rot`, `bit` and `res,set` ZEX CRCs
  of `ZEXUND` equal the values read from the real screen
  ([QA outputs](../agents/reports/m102_zexund_qa/README.md)).
- **Task/evidence/commit:** [M102 task](../agents/tasks/M102_upd9002_undocumented_opcodes.md);
  fix [732bdf7f](https://github.com/nakatamaho/vaeg/commit/732bdf7f1c5d951341952ee61aa6bc58262f550b),
  test [378c69da](https://github.com/nakatamaho/vaeg/commit/378c69da89232b0faec29973251dc0775ffd70bf).

### Undefined Z80 opcodes crashed the emulator

- **Status:** fixed on `topic/m102-upd9002-undocumented-opcodes`.
- **Symptom/scope:** executing an undefined ED opcode (for example ED 00 or
  ED A4), a NEG/RETN/IM duplicate (for example ED 4C), or a DD/FD prefix
  before an opcode without an index form (for example DD 00) crashed vaeg
  with a segmentation fault. Both Z80-compatible CPUs were affected: the
  uPD70008-compatible main-CPU mode and the FDC `UPD780C`.
- **Demonstrated root cause:** the vendored suzukiplan core leaves 196 ED,
  90 DD and 90 FD opcode-table entries empty. vaeg builds the core with
  `Z80_NO_EXCEPTION`, which compiles out the unknown-opcode check, so the
  empty entry was called as a null function pointer.
- **Correction:** with `Z80_NO_EXCEPTION`, opcodes without a handler now run
  as on a Zilog Z80: undefined ED opcodes are two-byte NOPs, the NEG/RETN/IM
  duplicates act as the base instruction, and a DD/FD prefix before a
  non-index opcode is ignored
  ([patch](../agents/reports/m102_suzukiplan_undefined_opcodes.patch),
  upstream [#63](https://github.com/suzukiplan/z80/pull/63)). Real µPD9002
  behavior for these opcodes is measured separately in M102.
- **Verification:** the wrapper test executes all 256 ED, DD and FD
  opcodes under both flag profiles and checks NOP, NEG-duplicate and
  ignored-prefix semantics; with the previous core the same test crashes
  with SIGSEGV. The upstream test `test-undefined-opcodes` passes 38/38, and
  ZEXDOC/ZEXALL still pass 67/67.
- **Task/evidence/commit:** [M102 task](../agents/tasks/M102_upd9002_undocumented_opcodes.md);
  fix [3876ac80](https://github.com/nakatamaho/vaeg/commit/3876ac8037ff1164b7cd3e20ab02522511a12622),
  test [de48a275](https://github.com/nakatamaho/vaeg/commit/de48a27572fcb6dc3c718b70baa5e35562d5a302).

### uPD9002 Z80-emulation-mode flags followed the Zilog Z80

- **Status:** fixed on `topic/m101-upd9002-flag-profile`, including
  DAA/CPL/SCF/CCF; every rule is directly observed on a real PC-88VA2.
- **Symptom/scope:** in V1/V2 mode the main CPU produced Zilog Z80 flags. A
  real PC-88VA2 differs: stock ZEXDOC reports 12 and ZEXALL 32 failing
  groups, all of which the emulator passed. Only the uPD70008-compatible
  main-CPU mode is changed. The FDC `UPD780C` keeps the Zilog profile;
  whether the real µPD780C matches it is unverified (MAME 0.250 reports NEC
  NMOS SCF/CCF X/Y as `F & A & 28h`, unlike Zilog's `(F|A) & 28h`).
- **Demonstrated root cause:** the main-CPU adapter used the Zilog behaviour
  of the shared core. The real machine follows rules R1-R11 of
  [the ZEX results](uPD9002-zex-results.md): F bits 5/3 never held; AND and
  BIT clear H; accumulator rotates and block loads keep H and N; ADD rr keeps
  H; ADC/SBC HL take H from bit 3; CPL/SCF/CCF change only A or C; DAA uses
  an H-dependent high threshold and sets P/V to the signed overflow.
- **Correction:** the vendored core gained a per-instance flag profile
  ([patch](../agents/reports/m101_suzukiplan_flag_profile.patch), with
  R8-R11 in [a second patch](../agents/reports/m101_suzukiplan_daa_cpl_scf_ccf.patch));
  `Z80CompatCpu` exposes it, applies R1 to every F value the adapter writes
  (register import, state load, LD A,I/LD A,R), and the uPD70008 adapter
  selects `kUpd9002`. The FDC keeps `kZilog`.
- **Verification:** under `kUpd9002`, stock ZEXDOC and ZEXALL through the
  wrapper match the real machine in all 67 groups each, found CRCs included.
  The host reference reproduces all nine M101 real-machine output files byte
  for byte ([QA outputs](../agents/reports/m101_zexall_qa/README.md)), and
  `vaeg_z80_compat_flag_profile` checks every record of the real DAA/CPL/
  SCF/CCF dumps. Under `kZilog` both suites still pass 67/67.
- **Task/evidence/commit:** [M101 task](../agents/tasks/M101_upd9002_flag_profile.md);
  core [5c2a6c10](https://github.com/nakatamaho/vaeg/commit/5c2a6c10bfb86a7bc55a01de5ab195c052bc1bb6),
  adapter [b8b462e7](https://github.com/nakatamaho/vaeg/commit/b8b462e73743924199bece033dae3c519b8bc3b6),
  ZEX expectations [0133ff2d](https://github.com/nakatamaho/vaeg/commit/0133ff2de28f1dddc5811d078e3d3b3becf58cdb),
  R8-R11 [7a05e388](https://github.com/nakatamaho/vaeg/commit/7a05e3888739dccfbb912f9605dbd458f002135b),
  real-machine tests [485a4044](https://github.com/nakatamaho/vaeg/commit/485a4044350447eba65755071853b29fb5603bda).

### Z80 ADC/SBC HL,rr flags were wrong when the carry was set

- **Status:** fixed on `topic/m101-upd9002-flag-profile`.
- **Symptom/scope:** with C = 1, `ADC HL,rr` and `SBC HL,rr` produced a wrong
  H flag whenever `rr & 0FFFh = 0FFFh`, a wrong P/V flag whenever
  `rr & 7FFFh = 7FFFh`, and also lost C when `rr = FFFFh`. HL itself was
  correct. Both Z80-compatible CPUs were affected: the FDC `UPD780C` and the
  uPD70008-compatible main-CPU mode.
- **Demonstrated root cause:** the vendored suzukiplan core passed `c + nn`
  as one 16-bit operand to `setFlagByAdc16`/`setFlagBySbc16`. Folding the
  carry into the operand inverts the carry chain at each bit where `nn` has a
  run of ones, and truncates `FFFFh + 1` to `0000h`.
- **Correction:** pass the carry separately and compute the flags from
  `before + nn + c` (or `before - nn - c`), through the recorded downstream
  patch [m101_suzukiplan_adc_sbc_carry.patch](../agents/reports/m101_suzukiplan_adc_sbc_carry.patch)
  applied by the M36 provenance procedure.
- **Verification:** `vaeg_z80_compat_flag_profile` compares 1,600 ADC/SBC
  cases per profile with an independent arithmetic model. The pre-fix header
  fails 200 of them (all with C = 1, F only); the fixed header passes all.
  Stock ZEXDOC and ZEXALL still pass 67/67 on the raw core and through the
  wrapper.
- **Task/evidence/commit:** [M101 task](../agents/tasks/M101_upd9002_flag_profile.md);
  fix [5c2a6c10](https://github.com/nakatamaho/vaeg/commit/5c2a6c10bfb86a7bc55a01de5ab195c052bc1bb6),
  test [6410c90e](https://github.com/nakatamaho/vaeg/commit/6410c90e0b9b3172f4c493c4a5e27202e533cbc5).

### 8087 DC/DE register arithmetic passed operation direction incorrectly

- **Status:** fixed and integrated on `main`.
- **Symptom/scope:** VA2 N88 BASIC `/87` converted `A=1.5` and `B=2.25` to
  values scaled by the packed-BCD integer magnitude, so `PRINT A+B` produced
  `3.75E+17` instead of `3.75`. The affected scope was 8087 `DC`/`DE`
  register-form arithmetic, including the ROM's `DC F9` conversion loop.
- **Demonstrated root cause:** `execute_register_instruction()` passed the
  `binary_stack_operation()` `operation` and `reverse` parameters in the
  opposite order for the `DC` and `DE` register forms. `DC F9` therefore
  selected the add operation instead of the documented `FDIV ST(1),ST(0)`
  operation.
- **Correction:** pass the operation selector and operand-reversal flag in
  the declared order, with the documented `DC` and `DE` direction tables.
- **Verification:** focused `DC F9`/`DC F1` and `DE F9`/`DE E1` tests pass;
  `vaeg_8087_tests`, production selftest, full CTest (`98/98`, one existing
  external SST skip), and the VA2 `/87` FAC/BASIC workload pass. Evidence:
  [v7 progress record](../8087/v7/records/progress.md) and
  [VA2 BASIC issue record](../8087/v7/records/issue-basic87-va2-int-route.md).
- **Task/evidence/commit:** v7 8087 local QA; integrated in
  [6af71282](https://github.com/nakatamaho/vaeg/commit/6af7128276c0e03f496fc63ab10847f3ef94f366).

### Windows native CRT skipped the Dear ImGui menu

- **Status:** fixed in the MinGW native-presenter startup path.
- **Symptom/scope:** Windows sessions with `NativeCRT=1` showed no main menu,
  including when started with `--windowed`; the 8087 boot-profile menu was
  therefore unreachable even though it was present in the executable.
- **Demonstrated root cause:** `np2.c` only called `gui_initialize()` when
  `scrnmng_native_active()` was false on Windows. The native D3D11 presenter
  owns presentation, but `gui_initialize()` already has a native-presenter
  backend; skipping it left `g_gui.initialized` false and `gui_draw()` returned
  immediately.
- **Correction:** initialize Dear ImGui for both SDL and native presenter
  paths, retaining native-to-SDL fallback if native GUI preparation fails.
  `--windowed` remains a display-mode override and no longer depends on
  disabling NativeCRT to expose menus.
- **Verification:** Linux 8087 core tests and production selftest passed;
  MinGW cross build produced a PE32+ executable containing the 8087 profile
  menu strings. Physical Windows runtime remains unverified because Wine is
  unavailable in the local environment.
- **Task/evidence/commit:** local MinGW menu regression; fix in
  [17b0bc68](https://github.com/nakatamaho/vaeg/commit/17b0bc68539abccd8e11d4c39d7297e45333c564).

### 8087 default clock did not match the VA2 8 MHz field measurement

- **Status:** fixed and integrated on `main`.
- **Symptom/scope:** the emulator selected 10,000,000 Hz when a new optional
  8087 was configured without an explicit frequency, while the VA2 field
  report measured 8 MHz at the 8087 socket clock pin.
- **Demonstrated root cause:** `UPD8087_DEFAULT_CLOCK_HZ` and its default
  configuration test were hard-coded to 10,000,000 Hz; the reset and service
  paths correctly consumed that constant, so the wrong default also affected
  emulated NDP service time.
- **Correction:** set the default to 8,000,000 Hz, retain 5/8/10 MHz and
  custom-frequency selection, and document the hardware evidence and the
  unresolved physical BUSY/INT boundary in
  [8087 implementation notes](8087.md).
- **Verification:** `vaeg_8087_tests`, P04 gate, production selftest,
  Linux rebuild, and MinGW rebuild passed. Existing 10 MHz BASIC evidence is
  retained as explicit custom-frequency evidence.
- **Task/evidence/commit:** VA2 clock-policy correction; fix in
  [a4ec8dbc](https://github.com/nakatamaho/vaeg/commit/a4ec8dbcef807de6c557a6af7a1d3ac77e8d4c45).

### Native Metal CRT wrote the filter chain directly to the drawable

- **Status:** intermediate-surface correction landed in M99z36; it was
  necessary but not sufficient for the reported frame error. The actual
  first-frame failure is corrected in M99z37.
- **Symptom/scope:** after native Metal and the ImGui menu were restored, every
  filtered frame reported `MetalFilterError(FailedToCreateCommandBuffer)` and
  fell back to pass-through. Audio and the SDL fallback path were unaffected.
- **Demonstrated contract issue:** `sdl2/librashader/backends/metal_bridge.mm`
  passed the `CAMetalLayer` drawable directly as librashader's output surface.
  The pinned librashader contract requires a caller-provided filter surface
  followed by a caller-owned copy to the backbuffer. This was a real issue, but
  the maintainer reproduced the frame error after this correction.
- **Correction:** render the filter chain into a reusable drawable-sized,
  drawable-format intermediate texture, clear the drawable, blit the filtered
  guest viewport into it, and then retain the native Metal ImGui pass.
  Recreate the intermediate when the drawable changes size or format.
- **Verification:** macOS static build passed; the focused librashader,
  ROM-less, and viewport suite passed 16/16. Physical Metal confirmation is
  deferred; the follow-up diagnosis is in [M99z37 report](../agents/reports/m99z37_metal_history_clear.md).
- **Task/evidence/commit:** [M99z36 report](../agents/reports/m99z36_metal_filter_output.md).
  Fix: [b76d635d](https://github.com/nakatamaho/vaeg/commit/b76d635d1246899507afb02c6bc595e323fe1dab).

### Native Metal CRT requested an empty first-frame history pass

- **Status:** corrected in M99z37; physical Apple GPU confirmation remains a
  maintainer check.
- **Symptom/scope:** the default history-free CRT preset initialized and the
  menu rendered, but every filtered frame returned
  `MetalFilterError(FailedToCreateCommandBuffer)` and fell back to pass-through.
- **Demonstrated cause:** the pinned Metal runtime creates a render encoder
  whenever `clear_history` is true, even when the filter chain has no history
  attachments. The empty descriptor is rejected by Metal and is reported using
  the generic command-buffer error.
- **Correction:** the Metal bridge no longer requests a first-frame history
  clear for the bundled preset, which has no `OriginalHistory` resources. The
  M99z36 intermediate output surface and ImGui composition remain unchanged.
- **Verification:** macOS static build and the focused 16-test suite passed;
  no Metal device/display was available to this agent process.
- **Task/evidence/commit:** [M99z37 report](../agents/reports/m99z37_metal_history_clear.md).
  Fix: [583eb8a9](https://github.com/nakatamaho/vaeg/commit/583eb8a97a32dc4444c054ec124d71722b7fe96a).

### Native Metal CRT presentation omitted the ImGui menu

- **Status:** corrected in M99z35; physical Apple GPU visual confirmation
  remains a maintainer check.
- **Symptom/scope:** macOS native CRT presentation showed the guest output but
  the SDL/ImGui menu disappeared because the native presenter owned the
  drawable and no longer used the SDL renderer backend.
- **Demonstrated cause:** the macOS startup path deferred GUI initialization
  while the Metal presenter had no GUI render pass. Its drawable also ignored
  the SDL-calculated output viewport.
- **Correction:** initialize the existing SDL2 ImGui platform layer on macOS
  even when Metal owns presentation, upload its textures to Metal, and render
  the ImGui draw lists after the guest/filter pass on the same drawable. Pass
  the calculated guest viewport to the Metal bridge while leaving the SDL
  fallback path unchanged.
- **Verification:** macOS static `vaeg_sdl2` build and the focused 16-test
  librashader/ROM-less/viewport suite passed. A physical menu screenshot and
  the observed Metal filter-chain command-buffer failure remain separate
  maintainer checks.
- **Task/evidence/commit:** [M99z35 report](../agents/reports/m99z35_metal_imgui_menu.md).
  Main fix: [8855991b](https://github.com/nakatamaho/vaeg/commit/8855991bc0441e717a3037de2c600a2e181d61f0).
  Metal shader correction: [6c163c5f](https://github.com/nakatamaho/vaeg/commit/6c163c5ff5afa8f2df687280f2556bbb45c39a41).

### Startup splash omitted with native CRT presentation

- **Status:** corrected in M99z17; native GPU visual retest pending.
- **Cause:** `splash_show` returned immediately without an SDL renderer, even
  when a valid native presenter owned the window.
- **Correction:** submit the bitmap through native pass-through presentation
  and restore the guest viewport/filter afterward; share SDL/native sizing.
- **Verification:** [M99z17 report](../agents/reports/m99z10_renderer_settings.md#m99z17--two-thirds-splash-and-native-presentation).
  MinGW/macOS builds and focused tests pass; no physical GPU result claimed.
- **Commit:** [74e32878](https://github.com/nakatamaho/vaeg/commit/74e32878f70cb8809ab6306dd676927ad6225a8a).

### Normal screenshots omitted display effects and information overlays

- **Status/scope:** corrected in M99z11 for SDL and Windows native D3D11;
  physical Windows screenshot comparison pending.
- **Cause:** the GUI normal-save handler called `scrnmng_save_guest_frame`,
  which encodes the raw guest shadow before effects, scaling and overlays.
- **Correction:** queue a one-shot composed-output readback before presentation;
  retain enabled information overlays while excluding menus/dialogs. Preserve
  separate deterministic and raw-analysis capture paths.
- **Verification:** [M99z11 report](../agents/reports/m99z11_display_screenshots.md),
  including actual SDL software-renderer effect/overlay readback tests and
  controller target lifetime tests. Native Metal/OpenGL readback is unsupported.
- **Commit:** [5699d96e](https://github.com/nakatamaho/vaeg/commit/5699d96e471ac72b07203e26182ec62a5a210a06).
- **M99z12 follow-up:** the unprocessed GUI screenshot previously included
  both analysis sections unconditionally. It now follows the independent
  Video/Framebuffer visibility flags. The linked report records four-state
  surface tests, including byte-identical output when both flags are off.
  Fix: [e25b14c9](https://github.com/nakatamaho/vaeg/commit/e25b14c907d18e8bbdb73c1f364a3edc40a28bf5).

### Information overlays omitted by native CRT presentation

- **Status:** corrected in M99z9; Windows visual retest pending.
- **Symptom/scope:** Video info and Framebuffer info appeared in SDL but
  disappeared with native CRT, as reported by the maintainer after M99z8.
- **Demonstrated cause:** both overlay calls were below the native early
  return in `scrnmng_present_end()` and used SDL-renderer-only primitives.
- **Correction:** reuse the overlay data, layout and glyphs through the
  native ImGui background draw list, after guest filtering and behind menus.
  Convert drawable coordinates to logical GUI coordinates for high DPI.
- **Verification:** [M99z9 report](../agents/reports/m99z9_crt_info_overlays.md),
  including actual ImGui geometry/color checks at 1x and 2x scale and existing
  raw-capture boundary tests. No guest/core changes.
- **Commit:** [d8979d1d](https://github.com/nakatamaho/vaeg/commit/d8979d1d9f3ebcca6d035eeb0d5090d0379c66ed).

### Windows native CRT excluded the GUI and hid activation state

- **Status:** corrected in M99z6; physical CRT retest pending.
- **Symptom/scope:** the MinGW package displayed the guest with no apparent
  CRT effect; enabling native presentation also made the menu unavailable.
- **Demonstrated cause:** DLL presence did not enable the default-off setting,
  startup explicitly skipped GUI initialization for native presentation, and
  the native present call did not compose GUI draw data. The maintainer's DLL
  load result was not captured and is not inferred from appearance alone.
- **Correction:** compose the pinned ImGui D3D11 backend after the shader,
  share the SDL viewport/menu sizing, enable and toggle from the live menu,
  expose runtime state, resolve bundled assets beside the executable, and
  provide an explicit CRT launcher/log. Preserve native pass-through on filter
  errors and release GUI resources before native device/SDL transitions.
  Request Windows per-monitor DPI awareness and scale fonts and style from a
  baseline, with a persistent UI-size override. The previous fixed 16-pixel
  font did not account for effective Windows DPI; physical sizing is pending.
- **Verification:** MinGW build, macOS feature-on/off builds and 10/10 focused
  tests each; Debug assertion-enabled suite 8/8. Controller/presenter doubles
  cover repeated toggles and recovery, not physical GPU output.
- **Task/evidence:** [M99z6 report](../agents/reports/m99z6_windows_crt_gui.md).
- **Commit:** [271cb211](https://github.com/nakatamaho/vaeg/commit/271cb2117cc369694437668a16a2d9500409acdd).

### SDL resource creation success was treated as failure

- **Status:** fixed in M99z; real GPU presentation remains separately gated.
- **Symptom:** ROM-less smoke runs exited with status 1 after reporting the
  SDL renderer backend, and the same inverted condition could misreport SDL
  renderer recreation after a native-presentation fallback.
- **Affected scope:** SDL2 screen startup and native CRT fallback on all
  platforms using the portable frontend.
- **Demonstrated root cause:** `scrnmng_create_sdl_resources()` follows the
  repository convention `SUCCESS == 0`, but both callers used logical
  negation, so a successful resource creation entered the failure path.
- **Correction:** both callers now compare the helper result explicitly with
  `SUCCESS` and fail only when resource creation returns `FAILURE`.
- **Verification:** macOS feature-on build, seven focused librashader/ROM-less
  CTests, and dummy-driver ROM-less `--smoke` all pass after the correction.
- **Evidence:** [M99 final report](../agents/reports/m99-final-report.md).
- **Milestone/task:** M99 librashader CRT pipeline;
  [M99 specification](../agents/tasks/M99_librashader_crt_pipeline.md).
- **Commit:** [1bd5330f](https://github.com/nakatamaho/vaeg/commit/1bd5330f9b0458f19d8043bdefc28d2fc87f48d8).

### Drawable-size success was treated as failure

- **Status:** fixed in M99z1; maintainer confirmed Windows guest display during
  the M99z6 follow-up, but clarified that menu sizing remains too small.
- **Symptom:** the Windows MinGW frontend showed its menu and continued to
  produce emulated audio and floppy-drive sounds, but the guest display stayed
  black. Startup and scale-change diagnostics reported a zero-sized guest
  viewport even though the window and drawable had valid dimensions.
- **Affected scope:** SDL2 viewport calculation, high-DPI menu placement,
  window-to-guest pointer mapping, and native-presenter resize/presentation on
  every portable frontend platform.
- **Demonstrated root cause:** `scrnmng_get_drawable_size()` follows the
  repository convention `SUCCESS == 0`, but four callers used logical
  negation. A successful drawable query was therefore treated as failure;
  most visibly, `scrnmng_calculate_viewport()` returned before assigning a
  guest rectangle and left the diagnostic at `0x0`.
- **Correction:** all four callers now compare explicitly with `SUCCESS`.
  Startup also rejects an invalid viewport, and a dummy-driver ROM-less CTest
  exercises that guard.
- **Verification:** the macOS dummy-driver smoke diagnostic changed from an
  invalid guest rectangle to `guest=0,22 640x400`; 90 executed CTests passed
  and the external SSTS case was separately skipped. MinGW
  SDL-only and librashader-enabled console builds both linked successfully.
  The maintainer subsequently confirmed Windows guest display. Native D3D11
  shader output remains subject to the M99z6 retest.
- **Evidence:** [M99 final report](../agents/reports/m99-final-report.md).
- **Milestone/task:** M99z1 follow-up to the M99 librashader CRT pipeline.
- **Commit:** [fa2a3d78](https://github.com/nakatamaho/vaeg/commit/fa2a3d78749d84fc08d90ee749bc32f10b2f08fa).

### macOS FetchContent ran before Objective-C languages were initialized

- **Status:** fixed in M99z2.
- **Symptom:** the `macos-ci` FetchContent preset failed during CMake
  generation with missing `CMAKE_OBJCXX_COMPILE_OBJECT`, archive-create, and
  archive-finish rules, before any VAEG source was compiled.
- **Affected scope:** Apple builds that enable librashader while fetching SDL2
  and the other pinned dependencies; the MacPorts discovery build was not
  affected.
- **Demonstrated root cause:** the top-level project enabled Objective-C++ only
  after creating the FetchContent dependency graph. CMake therefore generated
  Apple subprojects before the required language rules existed. Enabling only
  Objective-C++ early also made SDL Objective-C sources inherit incompatible
  C++ language options.
- **Correction:** Apple librashader builds initialize both Objective-C and
  Objective-C++ before declaring or populating FetchContent dependencies. The
  later Objective-C++-only initialization was removed.
- **Verification:** a clean local `macos-ci` configure and complete build
  passed; 103 executed CTests passed with six skips. The hosted macOS job in
  [Actions run 33877195164](https://github.com/nakatamaho/vaeg/actions/runs/33877195164)
  passed configure, build, smoke, unit tests, package validation, and artifact
  upload.
- **Evidence:** [M99 final report](../agents/reports/m99-final-report.md).
- **Milestone/task:** M99z2 follow-up to the M99 librashader CRT pipeline.
- **Commit:** [2aeaf519](https://github.com/nakatamaho/vaeg/commit/2aeaf519e60695ce2f9f9bd4c32e3a115421f51a).

### Private IDA64 candidate exited during draw-order validation

- **Status:** fixed in M98aa; real VA/VA2 hardware confirmation remains
  pending.
- **Symptom:** the private 64-instance IDA candidate returned to the DOS
  prompt before entering graphics mode, while the same renderer's public
  candidate remained byte-identical and functional.
- **Affected scope:** private M98aa startup validation and SGP destination
  command construction; the public ZUNDAMON profile is unchanged.
- **Demonstrated root cause:** the private four-word draw-order seen mask was
  cleared with `ES` still selecting the BMS aperture. The first validation
  phase passed only because the image's initial bytes were zero; the next
  phase observed stale bits and rejected a valid permutation. The private
  capacity expansion also reused the footprint-index shift for the SGP
  destination selector, even though that wire field remains four bits.
- **Correction:** bind `ES` to the guest data segment for the bounded mask
  clear, restore the prior segment, and use an explicit fixed SGP destination
  shift independent of private footprint capacity.
- **Verification:** public rebuilds remain byte-identical to the accepted
  M98z guest. Private deterministic rebuilds remain 60,848 bytes; VAEG startup
  reaches a complete-frame flip checkpoint for private counts 1, 4, 16, and
  64, with the private HUD and instances rendered. Focused M98aa, M98z, and
  M98y tests pass.
- **Evidence:** [M98aa IDA64 report](../agents/reports/m98a1_ida_64_animation.md).
- **Milestone/task:** M98aa IDA 64-instance animation launch correction.
- **Commit:** [ee65ac70](https://github.com/nakatamaho/vaeg/commit/ee65ac701dec5d9d759b3efed245b76cf4686052).

### Private IDA64 RIGHT cadence request returned to DOS

- **Status:** fixed in M98aa; real VA/VA2 hardware confirmation remains
  pending.
- **Symptom:** pressing RIGHT to lower the FPS on the private IDA64
  candidate could leave the graphics program and return to the DOS prompt,
  most visibly at the 15 FPS ladder entry.
- **Affected scope:** private keyboard polling during SGP/VBLANK waits and
  publication-synchronous FPS HUD updates. The public ZUNDAMON binary is
  unchanged.
- **Demonstrated root cause:** the keyboard BIOS polling call was allowed to
  clobber caller registers used for SGP ports and bounded loop counters.
  Separately, a VBLANK edge falling during the private FPS field write was
  treated as a fatal HUD failure instead of a retryable boundary miss.
- **Correction:** the private poll path now preserves all caller registers,
  segment registers, and flags, and restores the guest data segment after the
  BIOS calls. Private cadence changes keep the previous active divisor until
  the FPS field write succeeds; a narrow VBLANK miss rolls back the request
  and retries at the next edge.
- **Verification:** the accepted public guest rebuild remains byte-identical;
  private deterministic rebuilds remain 60,848 bytes. Private VAEG startup
  reaches complete-frame checkpoints for counts 1, 4, 16, and 64, and the
  bounded divisor-change scenario completes without a runtime failure.
- **Evidence:** [M98aa IDA64 report](../agents/reports/m98a1_ida_64_animation.md).
- **Milestone/task:** M98aa private cadence-key failure correction.
- **Commit:** [36593140](https://github.com/nakatamaho/vaeg/commit/36593140e2fca01fdc62ae1aa18c195f08d593e1).

### NEON3 text overlay repainted the whole TVRAM surface every frame

- **Status:** fixed in M97; real PC-88VA/VA2 hardware confirmation remains
  pending for this increment.
- **Symptom:** the live NEON3 text blinked during rendering, stale console
  text remained until the first repaint, and the release payload stopped at
  the end of its 6144-frame timeline instead of continuing until `ESC`.
- **Root cause:** `neon_counter_update_overlay` toggled text composition,
  cleared all 25 TVRAM rows, and rewrote static labels on every frame after
  the page flip.  The frame-limit path then fell through to the diagnostic
  status screen after one timeline pass.
- **Correction:** clear TVRAM once before the first frame through the VA Text
  BIOS path, write static labels once, and update only fixed-width dynamic
  fields after the VBLANK-boundary presentation.  Rewrite the scene title only
  when the scene changes.  Text BIOS `INT 83h/AH=25h, AL=00h` suppresses cursor
  blink while running and `AL=03h` restores it on exit.  The default release
  build wraps the 6144-frame timeline to frame zero until `ESC`; diagnostic
  builds can disable wrapping with `NEON_BUILD_LOOP=0`.
- **Verification:** NEON3 200/400 NASM payloads build at 57328 bytes; VAEG
  startup/frame-loop capture reports `TEXT_VISIBLE=PASS`,
  `BOTTOM_GUIDE=PASS`, and `FORBIDDEN_VISIBLE=NONE`; `vaeg --selftest` passes;
  CTest passes 84/84 with one expected skip; repository encoding, case, and
  diff checks pass.  No real VA/VA2 hardware gate was run for this change.
- **Evidence:** [NEON3 text-console report](../port/neon3_text_console.md),
  [NEON3 P3 status](../port/neon3_p3_status.md), and
  [NEON3 source](../../demos/neon3/src/neon_counter.asm).
- **Milestone/task:** M97 NEON3 text repaint cleanup and continuous timeline.
- **Commit:** [b62e0e07](https://github.com/nakatamaho/vaeg/commit/b62e0e07d4f37f3dcbd8fb559667cc21536b7a2)

### NEON4 inherited the DOS soft-key guide instead of owning the VA text plane

- **Status:** fixed in M97; real PC-88VA hardware confirmation remains
  pending.
- **Symptom:** NEON4's graphics could be accompanied by the stale bottom
  console/function-key row (`DIR A:`, `DIR B:`, `COPY`, `TIME`), while the
  intended NEON status text was not rendered by the stage-8 port.
- **Root cause:** unlike GLASS, NEON4 must keep TEXT enabled, but its stage-8
  startup had no VA text-console ownership sequence and `text_update` was only
  a stub.  The inherited soft-key/system-line state therefore remained
  visible.
- **Correction:** stage 8 now calls VA Text BIOS `INT 83h/AH=2Fh, AL=00h` and
  Screen Editor BIOS `INT 94h/AH=01h, AL=FFh` before graphics.  A shared
  NEON3-style overlay selects text-only composition, clears all TVRAM rows
  while the display is hidden, writes fixed labels once, and restores
  text-above-G0 composition.  Dynamic fields are grouped immediately after
  each VBLANK edge, and the scene title row is rewritten only on a scene
  change.  Text BIOS `AH=25h, AL=00h` hides the cursor blink; exit restores
  `AL=0Ah` guide settings and the normal cursor.  TEXT stays enabled and no
  DOS or direct-TVRAM clearing path is used.
- **Verification:** 16-colour and 65536-colour payloads assemble; VAEG
  captures show the live overlay and blank rows below it, with no inherited
  guide text in the decoded TVRAM.  Graphics remains visible in both
  profiles.  CTest and hardware acceptance are reported separately below.
- **Evidence:** [NEON4 P5 report](../port/neon4_p5.md) and [NEON4 stage-8
  source](../../demos/neon4/src/neon4_p3.asm).
- **Milestone/task:** M97 NEON4 text-console cleanup.
- **Commit:** [d013466f](https://github.com/nakatamaho/vaeg/commit/d013466f35eb9af8666a0906a3a9e26435485eb5).

### NEON4 4bpp spans exposed a deferred four-pixel completion

- **Status:** fixed in M97.
- **Symptom:** the 640x400 16-colour profile could show a four-pixel hole in
  a face span before a later write filled it.  The completed framebuffer did
  not necessarily retain the hole, but the intermediate paint sequence was
  visibly wrong.
- **Root cause:** the packed 4bpp span writer split one logical span between
  immediate CPU RMW operations for partial endpoint words and an asynchronous
  SGP CLS operation for complete interior words.  That is two visible write
  stages for one span; it is not a geometry or nibble-mapping rule.
- **Correction:** the 4bpp path now walks every touched word through one
  geometry-independent exact CPU RMW routine.  The first and last words use
  their logical endpoint masks, while wholly covered words select all four
  nibbles through the same routine.  No endpoint-correction pass or SGP CLS
  overlap remains for face spans.  SGP remains in use for clear and LINE
  operations.
- **Verification:** NASM builds passed for the 16-colour and 65536-colour
  profiles; consecutive VAEG captures of the 16-colour profile retained the
  intended face geometry without a later span-completion stage; `git diff
  --check`, CTest (84/84 with one expected skip), and the repository case
  check passed.  Real-hardware timing remains outside this VAEG result.
- **Evidence:** [NEON4 P5 source](../../demos/neon4/src/neon4_p3.asm) and the
  local VAEG exact-span capture used for this correction.
- **Milestone/task:** M97 NEON4 4bpp transient span correction.
- **Commit:** pending publication.

### GLASS P4 face fill used geometry-specific repair and empty-span overdraw

- **Status:** fixed in M97; real-hardware keyboard-return parity remains a
  separate unresolved item.
- **Symptom:** the GLASS P4 cube needed a temporary one-pixel outline bridge
  after face filling, and a vertex row could gain a face-colour pixel outside
  the polygon when an empty span was swapped into an ordered range.
- **Root cause:** triangle spans used inconsistent integer-X ownership at
  `y+1/2`; the generic span emitter swapped `x_left > x_right` instead of
  treating it as an empty span. The earlier repair stage masked this symptom
  with cube-specific writes. This was not an SDL blank-raster presentation
  issue.
- **Correction:** use one general `ceil(left)`/`floor(right)` half-row rule,
  discard empty spans, preserve exact logical endpoints with masked 4bpp word
  RMW, and keep only the intended outline redraw. The CPU verification path
  now uses the same exact logical-span contract. A source-level no-repair
  guard prevents reintroducing named geometry-specific repair stages.
- **Verification:** independent x-mod-4 calibration, face underfill/overfill,
  internal-hole, edge-registration, vertex, shared-edge, slope, and rectangle
  matrices passed. CPU/SGP complete GVRAM and composed-screen comparison
  passed; CTest 84/84 passed with one external test skipped by policy; VAEG
  selftest passed. Real observations remain PC-88VA graphics/ESC PASS with
  keyboard-after-return unresolved, and PC-88VA2 graphics/ESC/keyboard PASS.
- **Evidence:** [M97 P4 visual report](../agents/reports/m97_p4_visual_holes.md),
  [shared SGP implementation](../../demos/glass-orbit/src/glass_orbit_sgp_backend.asm),
  and [no-repair guard](../../demos/glass-orbit/tools/check-p4-no-repair.py).
- **Milestone/task:** M97 P4 fill algorithm cleanup.
- **Commit:** [76d7de7](https://github.com/nakatamaho/vaeg/commit/76d7de7cfffd51b49302cd46cd47afb82ad200f4).

### GLASS final scene rebuilt the visible page and enqueued cube edges twice

- **Status:** fixed in M97a; real-hardware timing and keyboard parity remain
  separate pending items.
- **Symptom:** the complete P5 scene could visibly show face construction and
  a temporary four-pixel endpoint stage, with substantial flicker while the
  active page was cleared and rebuilt. The final VAEG framebuffer was correct.
- **Root cause:** P5 used the common backend at stage 3, so the first SGP list
  already contained the edge LINE commands before endpoint RMW and the scene
  submitted a second edge list. It also rendered every frame into the single
  displayed FB0 source half; the VBLANK poll happened only after construction.
- **Correction:** P5 now uses stage 2 (grid/faces only), applies the shared
  exact endpoint partition once, submits one outline list, and renders into
  the hidden 200-line FB0 source half. After SGP idle and CPU stars complete,
  it waits for TSP VBLANK and selects the complete source through Graphics
  BIOS `$RollTo`. CPU endpoint/stars use the matching A0000h aperture segment
  for each source half. Explicit `FRAME_READY`, geometry-complete,
  star-complete, and SGP-idle state bytes guard the page selection; partial
  endpoint words remain exact masked RMW writes and are never SGP-full-filled.
- **Verification:** P4 raw GVRAM and screen remained byte-identical to the
  pre-change capture; P5 VA and VA2 captures show complete presented frames;
  three temporal checkpoints showed no visible construction frame. The P5
  source/partition guard, P4 alignment/slope matrices, CTest, and repository
  checks passed.
- **Evidence:** [final temporal report](../port/glass_orbit.md), [temporal
  checker](../../demos/glass-orbit/tools/verify-temporal.py), and
  [final scene](../../demos/glass-orbit/src/glass_scene.inc).
- **Milestone/task:** M97 P5 temporal rendering cleanup.
- **Commit:** [49242f3](https://github.com/nakatamaho/vaeg/commit/49242f3d3d739eb1602042cb6819314f328ef7ac).

### SGP wireframe demo corrupted its projected solid geometry

- **Status:** fixed in the revised M97 visual-gate candidate; G97 remains
  pending.
- **Symptom:** the program identified its four objects as a tetrahedron,
  cuboid, dodecahedron, and icosahedron, but the animated line sets did not
  consistently resemble those solids. It also used 320x200 output instead of
  the subsequently required 640x400 mode.
- **Root cause:** the projection routine saved one multiplication result in
  `DX`, then executed another one-operand 16-bit `IMUL`, which overwrote
  `DX:AX` before the two products were combined. The second object also used
  unequal X, Y, and Z extents and therefore was a cuboid rather than a regular
  hexahedron.
- **Correction:** preserve every intermediate Q7 product in memory, use equal
  extents for the cube, retain the verified degree-3/30-edge dodecahedron and
  degree-5/30-edge icosahedron tables, and render at 640x400 from two halves of
  a BIOS-defined 640x800 Graphic 0 framebuffer.
- **Verification:** NASM builds the revised COM, and two VAEG captures at
  distinct animation phases show the connected regular tetrahedron, cube,
  regular dodecahedron, and regular icosahedron on a 640x400 reference grid.
- **Evidence:** [M97 report](../agents/reports/m97_sgp_tekumani_commands.md).
- **Commit:** [7df8bc9](https://github.com/nakatamaho/vaeg/commit/7df8bc9529c012695fcb5d38fe677dba984c1eba).

### SGP LINE directions and SCAN behavior diverged from the Technical Manual

- **Status:** fixed in M97; G97 visual-regression verification remains
  pending and real-hardware timing remains unverified.
- **Symptom:** LINE interpreted the documented vertical and horizontal
  direction bits in each other's positions, and both SCAN commands only
  logged a TODO before command processing continued. The original-VA block
  decoder also accepted later-model width, height, and pitch bits that the VA
  descriptor reserves.
- **Root cause:** the SGP implementation carried separate reversed LINE masks,
  unimplemented SCAN command handlers, and one unconditional VA2-style block
  decoder. The local PC-88VA Technical Manual defines common LINE/BLT
  direction bits, complete SCAN results, and the original-VA field widths.
- **Correction:** use `VD=0800h` and `HD=0400h` for LINE, select original-VA
  versus VA2 descriptor profiles explicitly, and execute SCAN RIGHT/LEFT
  incrementally with the documented SET COLOR boundary and destination
  updates. No SGP timing coefficient or save-state layout changed.
- **Verification:** the compiled selftest covers both descriptor profiles,
  all sixteen ROPs, destination-zero transfer, and SCAN first/middle/miss and
  packed-word-boundary cases. Linux debug build/selftest and 83 CTests passed,
  one external-fixture test skipped, and the MinGW cross build passed.
- **Evidence:** [M97 report](../agents/reports/m97_sgp_tekumani_commands.md).
- **Commits:** [ffb8521](https://github.com/nakatamaho/vaeg/commit/ffb85210c62f984108ad9d022f7d046107744f60),
  [7b788ed](https://github.com/nakatamaho/vaeg/commit/7b788edc6e657f2d9e8e48f759c5cab6eb7c4899).

### Historical SGP demo COMs used byte cycles for word-only VA registers

- **Status:** hardware-safe access correction applied; real-hardware
  validation remains open. The bundled COMs are rebuilt from the corrected
  common sources.
- **Symptom:** the SGP pseudo-sprite COM programs could freeze on real
  PC-88VA hardware even though VAEG accepted the same I/O sequence.
- **Root cause:** the demos wrote the 32-bit SGP command-list pointer as four
  byte cycles and wrote FB1 `DSA1` as three byte cycles. The period Tech manual
  and the VA BIOS use word writes at `0500h`/`0502h` and at `022eh`/`0230h`;
  byte access to those word ports is not hardware-safe. The BIOS also
  reasserts `0580h=10h` before each SGP kick.
- **Correction:** all generated SGP stages use word I/O for the command
  pointer and DSA1, byte I/O only for `0504h`/`0506h`, and reassert the BIOS
  GVRAM write mode before each kick. `SET WORK` remains the first command in
  every drawing list.
- **Verification:** NASM rebuilt `SGPDEMO1.COM` through `SGPDEMO6.COM` and
  `SGPD_7A.COM` through `SGPD_7D.COM`; the data-only D88 contains exactly
  those ten byte-matching payloads; the clean macOS CMake build and repository
  encoding/EOL/case checks passed. Real-hardware rerun remains a maintainer
  gate and is not claimed here.
- **Evidence:** [SGP demo investigation](sgp-pseudo-sprite-investigation.md),
  [uPD92017 SGP interface](upd92017-sgp.md), and the Tech-manual/BIOS
  disassembly notes recorded there.
- **Commit:** pending publication; no fixing commit is claimed in this checkout.


### Bootable PC-Engine EMS media froze before reaching the prompt

- **Status:** fixed in M90; G90 human gate remains pending.
- **Symptom:** the generated EMS supplemental D88 was data-only and could not
  boot. After retaining the PC-Engine 1.1 system files, SQEMM98 printed its
  banner but froze before the prompt, and its page test initially failed in
  native VA mode.
- **Root cause:** native VA accesses to `C0000H` through `CFFFFH` bypassed the
  retained EMS page pointers. SQEMM98 also inherited DOS assumptions that did
  not hold for PC-Engine: its memory test used a stale `DI`, its interrupt
  return did not preserve the loader's complete caller state, and it treated
  an incompatible request-header field as a mutable DOS command tail.
- **Correction:** native VA page-frame reads and writes now use the selected
  EMS pages. The SQEMM98 preparation step clears `DI`, preserves caller
  registers and flags, and uses validated built-in defaults. The media builder
  now retains the PC-Engine 1.1 IPL and fixed system files.
- **Verification:** ROM-less native VA mapping selftest, macOS/Linux/MinGW
  builds, Linux CI CTest 81/81, reproducible driver/media generation, and the
  structural validator passed. The selftest now sweeps all 832 logical pages
  and all four page-frame windows at 13MB; removing the native VA correction
  makes that test fail. Disposable VA1 and VA2 headless boots both reached the
  prompt at 13MB and reported a usable 647,168-byte RDEMS `C:` drive. The
  earlier 1MB run also copied/read back its 114-byte `CONFIG.SYS`.
- **Evidence:** [M90 validation report](../agents/reports/m90_va_ems_board.md).
- **Commit:** [a4b6170](https://github.com/nakatamaho/vaeg/commit/a4b6170824dbfb8d602ca1ade19c60778a6eff1c).
- **Regression coverage:** [081a2bb](https://github.com/nakatamaho/vaeg/commit/081a2bbffaa14e4bdcc7a9d5627f147bd00f9970).

### Windows JIS RO was not mapped to the guest underscore key

- **Status:** fixed in the hotfix; Windows manual verification remains a
  maintainer-side check.
- **Symptom:** Shift+RO on a Windows JIS keyboard produced no guest underscore.
- **Root cause:** the SDL2 Windows backend can report the JIS RO key as
  `SDL_SCANCODE_NONUSBACKSLASH` through `VK_OEM_102`, while the JIS mapper only
  recognized the other alias and `SDL_SCANCODE_INTERNATIONAL1`.
- **Correction:** JIS now aliases both `NONUSHASH` and `NONUSBACKSLASH` to the
  guest underscore code `0x33`. `VAEG_KBD_TRACE=1` also writes a Windows GUI
  trace to `vaeg-kbd-trace.log`, or to `VAEG_KBD_TRACE_FILE` when specified.
- **Verification:** Linux normal and VA-model selftests, CTest 2/2, repository
  validators, and the MinGW PE32+ build passed. The keyboard-map selftest now
  checks the `NONUSBACKSLASH` alias, and the trace-file path was exercised by
  the selftest run.
- **Evidence:** [JIS keyboard mapping](../../sdl2/kbdmap.c#L708) and
  [keyboard trace instructions](../../sdl2/README.md#keyboard-mapping).
- **Commit:** [0109944](https://github.com/nakatamaho/vaeg/commit/0109944756fd6049a20838fe340de565f6e061ba).

### SDL keyboard repeat events were discarded by the frontend

- **Status:** fixed in the hotfix; Windows manual verification remains a
  maintainer-side check.
- **Symptom:** holding a guest text or navigation key did not repeat on SDL2
  hosts even though SDL generated repeat keydown events.
- **Root cause:** sdlkbd_keydown() treated every repeat event as captured and
  returned before the keyboard mapper, so no guest make event was generated.
- **Correction:** repeat events now reach kbdmap_keydown() and use the existing
  guest key/action path. KANA and CAPS Lock repeats are intentionally
  suppressed so a held lock key cannot toggle the lock repeatedly; the existing
  guest-side
  KBEX_NONREP policy remains authoritative for non-repeat keys.
- **Verification:** Linux CI build, normal and VA-model selftests, CTest 2/2,
  and the MinGW cross-build passed. The keyboard-map selftest covers KANA lock
  behavior; both KANA and CAPS use the shared repeat-suppression branch.
- **Evidence:** [SDL keyboard dispatch](../../sdl2/sdlkbd.c#L36) and
  [keyboard mapping](../../sdl2/kbdmap.c#L1106).
- **Commit:** [813de6c](https://github.com/nakatamaho/vaeg/commit/813de6cb582fce1d4ae7d365d7aceb21acf078ab),
  [98c25da](https://github.com/nakatamaho/vaeg/commit/98c25dab5b8ffc901b01ed4a9475264492d11f05).

### Retired FMBOARD state payloads were silently accepted

- **Status:** fixed in M85; G85 human gate passed.
- **Symptom:** a state saved with a removed non-VA sound-board option could pass
  the generic FMBOARD version check and report a successful load even though
  the current tree no longer had the corresponding board or payload decoder.
- **Root cause:** after M84 removed the old PCM86, CS4231, AMD98, and related
  C-bus implementations, statsave_check() still used version-only validation
  for FMBOARD. flagload_fm() mapped an unsupported usesound value to an empty
  branch, so the obsolete payload could be skipped without an explicit
  compatibility result.
- **Correction:** M85 validates the usesound word and current payload size
  during preflight. Current VA and retained sound values remain accepted;
  M84-retired values fail before live-state mutation with an explicit
  retired-hardware error.
- **Verification:** the ROM-less selftest mutates a disposable state only at
  the FMBOARD usesound word, requires both check and load failure, and
  verifies unchanged CPU and memory state. Current save/load round-trip and
  HOSTFAT identity tests pass.
- **Evidence:** [M85 state-save report](../agents/reports/m85_state_save_section_cleanup.md).
- **Commit:** [a6493d2](https://github.com/nakatamaho/vaeg/commit/a6493d2f481a3744f528d1a19d5ee3663df59e90).

### OPN/OPNA synthesis state was not restored by save/load

- **Status:** fixed in M96g4; G96g human verification remains pending.
- **Symptom:** loading a state reset the YMFM OPN/OPNA synthesis phase and
  envelope state instead of resuming the saved sound. The VA OPN board also
  had no FM payload beyond its `usesound` word.
- **Root cause:** the FMBOARD section saved the register image and selected
  wrapper state, but not the active YMFM engine state. Its flag selection was
  also limited to `FMBOARD_VA_OPNA`, so `FMBOARD_VA_OPN` state was silently
  incomplete.
- **Correction:** the FMBOARD payload now covers both VA OPN and VA OPNA,
  serializes the complete YMFM bridge through the upstream `save_restore`
  API, restores it after device rebinding, and rejects the previous payload
  format with a state-version bump.
- **Verification:** Linux selftest, including save/load/save section-byte
  stability, passed after the correction; focused encoding, EOL, case, and
  clang-format checks passed. MinGW and the maintainer's sound human check
  remain part of G96g.
- **Evidence:** [FMBOARD state implementation](../../machine/statsave.c) and
  [YMFM bridge state implementation](../../sdl2/ymfmbridge.cpp).
- **Milestone:** M96g4.
- **Commit:** [cc3c9f4](https://github.com/nakatamaho/vaeg/commit/cc3c9f4e4dcc6d6058ef60d847fea088460173e7).

### CP/MVA EXM=1 directory grouping omitted large-program data

- **Status:** fixed in the uncommitted M76 working tree; commit pending because this task prohibits commit and push.
- **Symptom:** CP/MVA hung when running BACKGMMN.COM and CC2.COM; the guest loaded only the first approximately 16 KiB.
- **Root cause:** the writer emitted one directory entry per 16 KiB, but CPMBIOS.MAC has EXM=1 and CP/M groups two 16 KiB sub-extents in one logical entry.
- **Correction:** build_cpm_raw() now encodes grouped EX/RC entries, and parse_cpm_raw() rejects duplicate or gapped logical extents.
- **Verification:** 17 focused installer tests pass; real-data tools and BDS C disks round-trip completely for 12 and 14 files.
- **Evidence:** [CP/MVA installer](../../tools/cpmva/install_cpmva.py#L866), [focused tests](../../tools/cpmva/tests/test_install_cpmva.py#L260), and [setup guidance](../cpmva-setup.md#run-cpmva-in-vaeg).
- **Commit:** pending; no commit or push was performed.

### uPD9002 FF /7 executed a POP-like operation instead of the observed stack push

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** all 5,000 applicable G65 `FF /7` SST cases terminated normally
  but failed final architectural comparison. The expected state decremented
  `SP` by two and wrote the selected `r/m16` value to the stack; the actual
  state followed the inherited POP-like dispatch.
- **Root cause:** the active `FF` ModR/M group table routed `/7` to the
  obsolete `_pop_ea16` helper. The reconstructed M65a evidence proves the
  observable `FF /7` contract is a stack push, including the `r/m = SP` alias
  writing the decremented `SP`.
- **Correction:** `FF /7` now uses an M65a-owned push helper. It preserves the
  existing `FF /6` helper for the later M65d-owned SP-alias residue.
- **Verification:** the focused `vaeg_upd9002_m65a_ff7` test covers register,
  memory, and SP-alias forms. The selective M65a replay ran the exact 5,000
  owned hashes as `5,000 pass / 0 fail`, with zero timeout/crash and an M65d
  guard preserving the exact 144 `FF /6` G65 failures.
- **Evidence:** [M65a report](../agents/reports/m65a_upd9002_ff7.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [15f2ac8e](https://github.com/nakatamaho/vaeg/commit/15f2ac8e861c3cfedbc12acc9ef470925d00716c).

### uPD9002 BOUND used unsigned range comparison

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** 1,244 applicable G65 `62` BOUND SST cases failed final
  architectural comparison. Some cases expected a type-5 event but completed
  normally; others expected normal completion but entered type 5.
- **Root cause:** the active BOUND implementation compared the register
  operand and memory bounds as unsigned 16-bit values. The reconstructed M65b
  evidence proves the observable contract uses signed 16-bit lower and upper
  bounds with inclusive boundaries.
- **Correction:** BOUND now converts the selected register, lower bound, and
  upper bound to `SINT16` before applying the inclusive range decision. It
  continues to use the existing effective-address and synchronous type-5
  event-entry paths.
- **Verification:** the focused `vaeg_upd9002_m65b_bound` test covers
  signed lower/upper inclusivity, negative, positive, and cross-zero ranges,
  segment override, offset wrapping, physical wrapping, and type-5 frame
  preservation. The selective M65b replay ran the exact 1,244 owned hashes as
  `1,244 pass / 0 fail`, with zero timeout/crash, M65a `FF /7` protection,
  M65d `FF /6` guard preservation, and 3,565 former BOUND frame-only hashes
  still passing.
- **Evidence:** [M65b report](../agents/reports/m65b_upd9002_bound.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [d0e01694](https://github.com/nakatamaho/vaeg/commit/d0e01694a9b82b4cd16500743d77e45459c74be1).

### uPD9002 F7 /2 word NOT updated only the low memory byte

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** 1,113 applicable G65 `F7 /2` word NOT memory SST cases failed
  final architectural comparison. The expected RAM contained both complemented
  operand bytes, while the actual RAM complemented only the low byte.
- **Root cause:** the `_not_ea16` memory fast path applied `^= 0xffff` through
  the byte pointer `mem + madr`, so only the low byte was modified. The
  inhibited word path already used the 16-bit memory read/write helpers.
- **Correction:** `_not_ea16` now loads the little-endian word with
  `LOADINTELWORD`, complements all 16 bits, and stores the complete word with
  `STOREINTELWORD` on the direct memory path.
- **Verification:** the focused `vaeg_upd9002_m65c_f72` test covers register
  protection, low-memory word writes, odd-address word-path protection,
  segment override, indexed displacement, offset boundary behavior, high
  memory path preservation, FLAGS, IP, and neighbor preservation. The M65c
  replay ran the exact 1,113 owned hashes as `1,113 pass / 0 fail` and the
  complete selected `F7 /2` population as `5,000 pass / 0 fail`, with zero
  timeout/crash and M65a, M65b, M65d, and M65e guards preserved.
- **Evidence:** [M65c report](../agents/reports/m65c_upd9002_f72.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [8d338a52](https://github.com/nakatamaho/vaeg/commit/8d338a528a7c3b4a18636f2f3a4678ece6dbcd4f).

### uPD9002 FF /6 pushed the old SP value for the SP register operand

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** 144 applicable G65 `FF /6` SST cases failed final RAM
  comparison. All owned cases were register-form `r/m = SP` rows. The
  predecessor decremented `SP` but pushed the pre-decrement SP value.
- **Root cause:** `_push_ea16` read the register operand before invoking the
  push macro. For the SP alias case, the observable pushed value must be the
  decremented SP value produced by the push.
- **Correction:** `_push_ea16` now captures `SP - 2` for the register
  `r/m = SP` case before using the existing push path. Other register and
  memory-source forms remain unchanged.
- **Verification:** the focused `vaeg_upd9002_m65d_ff6` test covers SP alias,
  segment-prefixed SP alias, non-SP register, memory operand, and stack-wrap
  cases. The M65d replay ran the exact 144 owned hashes as
  `144 pass / 0 fail` and the complete selected `FF /6` population as
  `5,000 pass / 0 fail`, with zero timeout/crash and M65a, M65b, BOUND frame,
  M65c, and M65e guards preserved.
- **Evidence:** [M65d report](../agents/reports/m65d_upd9002_ff6.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [5cfc3540](https://github.com/nakatamaho/vaeg/commit/5cfc3540b5f1d78a7aace699d51729d272529552).

### uPD9002 wrapped segment-offset word accesses used contiguous linear bytes

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** the exact ten-case M65e tail failed final architectural
  comparison across `61`, `81 /6`, `FF /5`, `A5`, `9C`, `D1 /6`, `C8`, and
  `C4` forms. The mismatches involved registers, FLAGS, or represented RAM
  when a word operand crossed offset `0xffff`.
- **Root cause:** several inherited word paths treated the second byte as the
  next contiguous physical byte after `segment_base + 0xffff` rather than the
  byte at the same segment base with offset `0x0000`.
- **Correction:** M65e adds segment-offset word helpers and applies them only
  to the proven tail paths: `POPA`, V30 `PUSHF`, wrapped memory word ALU and
  shift operations, `MOVSW`, `LES`/`LDS`, far pointer fetches, and `ENTER`
  frame-copy reads and writes. Generic stack macros remain protected.
- **Verification:** the focused `vaeg_upd9002_m65e_tail10` test covers all
  eight structural tail forms. The M65e replay ran the exact ten owned hashes
  as `10 pass / 0 fail`; the original 7,511-hash G65 architectural residue
  replayed as `7,511 pass / 0 fail`, with zero timeout/crash and all M65a
  through M65d protected populations preserved.
- **Evidence:** [M65e report](../agents/reports/m65e_upd9002_tail10.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [c7bb5ee2](https://github.com/nakatamaho/vaeg/commit/c7bb5ee274441d608096e4a33e2eca5a2d5af3a4).

### uPD9002 segmented word helpers bypassed mapped-memory dispatch

- **Status:** fixed in M68; G68 approved at
  `d1e0225c4edb716893fe5579283fbf0915db72b9`.
- **Symptom:** PC-Engine/MS-DOS text output stopped scrolling normally after
  reaching the bottom row. New output repeatedly overwrote the last text row
  instead of moving existing lines upward.
- **Root cause:** the M65e A5/MOVSW segmented-word path delegated through a
  helper that correctly calculated 16-bit segment wrapping but then
  independently selected a flat `mem[]` fast path. For VA TVRAM and BMS
  mapped regions this bypassed the canonical mapped-memory dispatcher,
  callbacks, dirty/display side effects, and the active backing store such as
  `textmem[]`.
- **Correction:** segmented word helpers now own only segment-offset address
  formation and `0xffff`-to-`0x0000` wrapping. Contiguous words delegate to the
  canonical generic word API, and only the noncontiguous segment-wrap case
  splits into canonical byte accesses. No A5-, TVRAM-, BMS-, or `A0000h`-
  specific special case was added.
- **Verification:** focused M68 mapped-memory probes fail on the predecessor
  for the flat-`mem[]` bypass and pass after the fix for TVRAM, BMS, normal
  RAM, segmented word reads/writes, REP and non-REP MOVSW, DF=0/1, `FFFEh`,
  and `FFFFh -> 0000h` wrapping. The full architectural CI/full and
  fingerprint-full SST profiles preserve the approved G67 counts and digests,
  and the maintainer reported the PC-Engine/MS-DOS manual gate passed.
- **Evidence:** [M68 report](../agents/reports/m68_upd9002_segmented_word_mapped_dispatch.md).
- **Commit:** [90258f26](https://github.com/nakatamaho/vaeg/commit/90258f26207b7ce7dc3473a5df2811da4bb0c19c).

### IDP/TSP 0142H status reads erased stored status flags

- **Status:** fixed in M69; G69 remains pending human review.
- **Symptom:** reading the IDP/TSP `0142H` status port could return only
  `00H` or `40H`. Stored flags such as BUSY at bit 2 were erased, and any
  nonzero stored status falsely produced the VB bit.
- **Root cause:** `tsp_i142()` used the expression
  `tsp.status | (tsp.vsync) ? STATUS_VB : 0`. C parses that expression as
  `(tsp.status | tsp.vsync) ? STATUS_VB : 0`, so the stored status was first
  converted to a Boolean condition and then replaced by `STATUS_VB` or zero.
- **Correction:** the `0142H` input path now starts from `tsp.status` and
  orthogonally ORs `STATUS_VB` when `tsp.vsync` is active. No command timing,
  parameter-port behavior, rendering, state format, or broader IDP/TSP status
  semantics were changed.
- **Verification:** M69 adds a registered-I/O-path regression test covering
  the minimum BUSY/VB truth table, every non-VB single bit, representative
  combinations, VB idempotence, word `IN 0142H`, observable BUSY during
  `CMD_SYNC`, and exhaustive `256 x 2` stored-status/VB composition. The test
  fails on the predecessor with 508 exhaustive row failures and passes after
  the correction. Native non-external CTest, ASan/UBSan focused tests, MinGW
  build, repository invariants, and maintainer PC-Engine/MS-DOS runtime
  validation passed.

### WD33C93 PIO register access advanced the DATA/COMMAND windows

- **Status:** open; M75b2 fixes the demonstrated register-boundary defect,
  while the post-SELECT phase engine remains in progress.
- **Symptom:** the PCPLUS/SCHD low-level `07h Select without ATN` path
  stopped after CSR `11h`/COMMAND request.  The controller model treated the
  selected register as an ordinary auto-incremented byte file, cleared
  Auxiliary Status on `0CC0h` reads, and had no explicit DBR/CIP/BSY
  composition.  This could not reproduce the WD33C93 PIO host contract for
  the fixed DATA window or distinguish device CSR consumption from an 8259
  EOI.
- **Root cause:** the active C-Bus implementation conflated the WD33C93
  register address counter with the fixed COMMAND/DATA windows and used the
  emulated PIC path as the only observable interrupt state.  The complete
  M75a trace separately proves PIO mode (`Control=08h`, `0CC4h=02h`) and a
  missing post-`8Ah` transfer sequence; no DMA behavior is implicated.
- **Correction:** M75b2 adds fixed AR `18h`/`19h` windows, composes Auxiliary
  Status from DBR/CIP/BSY/PE plus the depth-one CSR latch, consumes the device
  CSR only through AR `17h`, and records unsupported 0CC4h DMA strobes and
  NEC AR `32h`/`34h`/`35h` accesses as hardware-pending.  DMA and 0CC6h
  hardware claims remain outside this checkpoint.
- **Verification:** `tools/qa/m75_scsi_controller.py`, the Linux debug
  build, and `vaeg --selftest` pass after the correction.  The guest trace
  still stops before AR `12h`-`14h`, AR `18h=20h`, and AR `19h` CDB transfer;
  the remaining phase-engine gap is tracked by M75c.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [7b5672b](https://github.com/nakatamaho/vaeg/commit/7b5672b9f6823d92f86b17592878f928c133e76b).
- **Evidence:** [M69 report](../agents/reports/m69_upd9002_idp_0142_status_composition.md).
- **Commit:** [6ef4f98e](https://github.com/nakatamaho/vaeg/commit/6ef4f98ec1be20054db2aeb9c4a44c6a3d2e36bf).

### PCPLUS STATUS requests were exposed before target phase readiness

- **Status:** corrected in M75d1; full G75 acceptance remains pending.
- **Symptom:** the PCPLUS/SCHD low-level `07h Select without ATN` path received
  `CSR=8Bh`, but its phase handoff was consumed by the main event pump before
  the foreground transfer setup returned. The guest then did not issue the
  STATUS `AR=18h <- 20h` request.
- **Root cause:** the controller released a pending target phase as soon as
  the preceding CSR was consumed by `AR=17h`. CSR-latch consumption and target
  readiness are separate gates. The raw `8Bh` was normalized correctly; the
  event was simply visible while the previous completion path was still active.
- **Correction:** pending phase requests now hold DBR low until a general
  controller processing event exposes the target REQ. Same-phase PIO
  continuation does not incur a phase-transition event. No guest address,
  CDB, filename, or transfer-count special case was added. REQUEST SENSE is
  implemented with a fixed no-sense DATA IN response because it is observed in
  the PCPLUS probe before INQUIRY.
- **Verification:** the real PCPLUS trace now shows `CSR=8Bh` at the guest
  `1B67h` wait path, `CS:[047Eh]=3Bh` after consumption, `1BA1h -> 1C14h ->
  1C32h`, STATUS data `00h` with `CSR=1Bh`, MESSAGE data `00h` with `CSR=1Fh`,
  and REQUEST SENSE DATA IN beginning with `70h`. Static M75 validation,
  focused CTest, Linux SDL2 build, and `--selftest` pass. The bounded trace
  did not yet reach the later 36-byte INQUIRY golden, so this entry remains
  separate from terminal G75 approval.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commits:** [1cd3edb](https://github.com/nakatamaho/vaeg/commit/1cd3edb2b3bcde572f00b8a1131bd05f66ee3bff),
  [e56a16e](https://github.com/nakatamaho/vaeg/commit/e56a16edf18985f95d78868fed73031a77514e88),
  [9e6919f](https://github.com/nakatamaho/vaeg/commit/9e6919fdb5aa04b2a8d100ace89747c73a5059fc),
  [07c1074](https://github.com/nakatamaho/vaeg/commit/07c1074a2c664e4b88bf4c8731cbcf4c5c4ad666).

- **Follow-up:** [4ab457b](https://github.com/nakatamaho/vaeg/commit/4ab457bd8361bed27fd8e09eaa25bbbe97644ed0)
  preserves DATA IN cursor state across repeated one-byte PIO requests, and
  [dafeae0](https://github.com/nakatamaho/vaeg/commit/dafeae0da3b4657371a6181b72c40874db30905c) resets that
  cursor only at actual phase boundaries.  Compact diagnostic controls are
  provided by [dca6090](https://github.com/nakatamaho/vaeg/commit/dca6090a01076f244697be157f2aa4d80eec3d20) and
  [52f9788](https://github.com/nakatamaho/vaeg/commit/52f9788a5c7d2c86beadddea3e4a135e7536046c).

### State-load rejection feedback disappeared with the State menu

- **Status:** fixed; corrected G55 human gate passed on 2026-07-22.
- **Symptom:** after rebuilding a changed HOSTFAT snapshot, selecting an older
  state correctly refused the load but appeared to do nothing. Reopening the
  State menu was the only way to find the rejection text.
- **Root cause:** the SDL2 frontend stored the preflight error in
  `state_status`, but rendered that string only inside the State menu. Choosing
  a load slot closes that menu before the next frame, so no rejection feedback
  remained visible.
- **Correction:** every rejected state load now opens a root-scope modal and
  blocks guest input until it is dismissed. When a valid state's only blocking
  preflight condition is its HOSTFAT identity (apart from the already accepted
  disk-change warning), the modal offers an explicit `Force load`. That path
  retains the current HOSTFAT mount state and read-only snapshot and warns that
  guest-cached FAT, directory, open-file, or file data may differ.
- **Verification:** Linux and Wine selftests proved strict rejection leaves CPU
  IP and guest memory unchanged. They also proved the explicit override
  restores the saved CPU/memory state without changing the currently mounted
  HOSTFAT digest. A PC-Engine GUI run displayed the mismatch modal, returned to
  the live guest on cancel, and restored the earlier guest state on explicit
  force; the maintainer accepted the focused interaction and explicitly
  declared the corrected G55 human gate passed.
- **Evidence:** [M55 task](../agents/tasks/M55_hostfat_integration.md) and
  [M55 report](../agents/reports/m55_hostfat_integration.md).
- **Commit:** [40b96aca](https://github.com/nakatamaho/vaeg/commit/40b96acaea8b925873d50c33f6fd3fc52dd71eb1).

### HOSTFAT 32 KiB clusters truncated files under PC-Engine

- **Status:** fixed in the M55 human-gate correction; corrected G55 retest
  pending.
- **Symptom:** the proposed 128 MiB HOSTFAT mounted and listed files, but
  PC-Engine reported only about 8 MiB free. More importantly, copying a
  generated 96 KiB file produced only 6144 bytes on writable guest media.
- **Root cause:** M55 used 2048-byte sectors and 16 sectors per cluster, making
  each FAT entry represent 32 KiB. The PC-Engine CONFIG.SYS block-device path
  does not accept that cluster size: the three-entry source chain advanced as
  only 2 KiB per entry. The separate PC-88VA 40 MB SASI layout uses 16 KiB
  clusters and does not establish 32 KiB support for this driver path.
- **Correction:** HOSTFAT now uses 1024-byte sectors and 16 sectors per
  cluster. Its 65,362 visible sectors still yield exactly 4084 FAT12 data
  clusters; reserved cluster identifiers `0FF0H`--`0FF5H` remain unavailable.
  The readable payload limit is 63.71875 MiB.
- **Verification:** a nonzero 96 KiB source copied byte-identically through
  PC-Engine. A separate 4 KiB marker allocated after a 60 MiB filler also
  copied byte-identically, proving that PC-Engine's approximately 8 MiB free-
  space display is not the readable-capacity limit. The snapshot selftest now
  asserts both allocation boundaries, and the generated-driver checker
  requires the corrected BPB.
- **Evidence:** [M55 task](../agents/tasks/M55_hostfat_integration.md) and
  [M55 report](../agents/reports/m55_hostfat_integration.md).
- **Commit:** [14157f7d](https://github.com/nakatamaho/vaeg/commit/14157f7d5888bbc6d1e9243f382506a8ced863a8).

### HOSTFAT Browse opened its popup under a different ImGui ID scope

- **Status:** fixed in M55; corrected G55 retest pending.
- **Symptom:** Configure -> HOSTFAT -> Browse appeared to do nothing on the
  SDL2 frontend.
- **Root cause:** the button called `ImGui::OpenPopup` inside the HOSTFAT child
  region, while `BeginPopupModal` ran later in its parent. ImGui popup IDs are
  relative to the current ID stack, so those two calls addressed different
  popup IDs.
- **Correction:** the child records a one-shot browser request; the parent
  consumes it and calls `OpenPopup` from the same ID scope as
  `BeginPopupModal`.
- **Verification:** GCC, Linux release, and MinGW builds completed, and an
  Xvfb-driven Configure interaction visibly opened the directory selector
  with navigation and Select/Cancel controls.
- **Evidence:** [M55 task](../agents/tasks/M55_hostfat_integration.md) and
  [M55 report](../agents/reports/m55_hostfat_integration.md).
- **Commit:** [5e83dfc9](https://github.com/nakatamaho/vaeg/commit/5e83dfc9a7ab47166c7be46a53c9bcf253307676).

### HOSTFAT discarded host modification timestamps

- **Status:** fixed in the M54 supplemental human-gate correction; maintainer
  timestamp display recheck pending.
- **Symptom:** every HOSTFAT file and directory appeared in PC-Engine with the
  timestamp `80-01-01 00:00` regardless of its host last-write time.
- **Root cause:** the snapshot scanner captured regular-file modification time
  only to detect source mutation, while the directory-entry writer always
  emitted FAT time `0000H` and date `0021H`.
- **Correction:** files, directories, the volume label, `.` and `..` now use
  host local last-write time at FAT's two-second resolution. Values clamp to
  the FAT 1980--2107 range. Directory type and time are checked before and
  after construction so the metadata addition remains transactional.
- **Verification:** GCC, Clang and ASan/UBSan CTest passed with exact FAT-field
  and range-clamp assertions; MinGW compiled the Windows `FILETIME` path and
  its Wine selftest passed. Final hosted and human results are recorded in the
  clean-room report.
- **Evidence:** [M54 clean-room report](../agents/reports/m54_hostfat_cleanroom_reimplementation.md)
  and [M54 task](../agents/tasks/M54_hostfat_readonly_prototype.md).
- **Commit:** [9c707a93](https://github.com/nakatamaho/vaeg/commit/9c707a93bc64ded691c756e205a2b7a0ef42c899).

### HOSTFAT clean-room dispatch emitted unsupported 80386 branches

- **Status:** fixed in the M54 clean-room provenance correction; supplemental
  maintainer gate pending.
- **Symptom:** the independently authored replacement driver initialized far
  enough for PC-Engine to reach `Ready`, but the first HOSTFAT DIR printed the
  initialization message and failed to list the snapshot.
- **Root cause:** NASM's unspecified CPU level relaxed long conditional jumps
  to the 80386 `0F 84H` encoding. NEC V30 does not decode `0F 84H` as that
  conditional branch, so the sector-read command did not reach its handler.
  A same-disk comparison excluded snapshot geometry and host transport; an
  8086-safe build passed with either tested resident-end ordering.
- **Correction:** the source fixes NASM at the 8086 CPU level and implements
  dispatch with short `JNE` plus 8086 `JMP`. The checker decodes all nine
  command edges and rejects all `0F 80H`--`0F 8FH` encodings.
- **Verification:** the final 528-byte driver completed root DIR, TYPE, and
  COPY in a private PC-Engine boot. The 6,780-byte copied file was re-extracted
  from the temporary D88 and matched its source byte-for-byte and by SHA-256.
  Clean GCC, Clang, ASan/UBSan, MinGW/Wine, and hosted results are recorded in
  the clean-room report.
- **Evidence:** [M54 clean-room report](../agents/reports/m54_hostfat_cleanroom_reimplementation.md)
  and [clean-room contract](../agents/research/m54_hostfat_cleanroom_spec.md).
- **Commit:** [bdcbeae8](https://github.com/nakatamaho/vaeg/commit/bdcbeae89b254dd02b8916104baac81c94f94a4d).

### HOSTFAT COPY rejected valid lifecycle requests and misclassified its FAT

- **Status:** fixed in the M54 human-gate correction; remaining G54 media and
  reset checks pending.
- **Symptom:** root-directory listing worked, but PC-Engine COPY first reported
  that the drive's driver could not execute the command. After that rejection
  was isolated, file copying failed during source reads.
- **Root cause:** the driver returned unknown-command status `8103H` for the
  valid `0DH` device-open and `0EH` device-close notifications that bracket
  COPY. Independently, its 8192-sector BPB described 4087 data clusters,
  crossing the 4085-cluster FAT12/FAT16 boundary. PC-Engine consequently read
  the packed FAT12 table as FAT16: cluster 2's successor became `0040H`, and a
  later packed pair became the invalid source LBA `AE18H`.
- **Correction:** open and close are explicit successful no-ops; write and
  write-with-verify remain write-protected. The backing image remains 8192
  sectors, but the BPB and host service expose only 8186 sectors, yielding
  exactly 4084 data clusters. The inaccessible final six sectors are rejected
  by the same pre-transfer range check as every other out-of-range request.
- **Verification:** the generated-driver checker requires both lifecycle
  comparisons, decodes the BPB, and fails if its data-cluster count reaches
  the FAT16 boundary. The ROM-less transport test rejects the first hidden
  sector without modifying guest memory. In a private live PC-Engine boot,
  COPY of neutral `TEST.TXT` returned to `Ready`, and destination DIR reported
  the exact 3958-byte length.
- **Evidence:** [M54 task](../agents/tasks/M54_hostfat_readonly_prototype.md)
  and [M54 report](../agents/reports/m54_hostfat_readonly_prototype.md).
- **Commits:** [bf6896d8](https://github.com/nakatamaho/vaeg/commit/bf6896d801c2d021f44cec43b7070531030c780a),
  [5faa8ca0](https://github.com/nakatamaho/vaeg/commit/5faa8ca0b04aac954a1da3d08c882c32651a0033).

### HOSTFAT used an IBM-sized request layout on PC-Engine

- **Status:** fixed in the M54 human-gate correction; G54 PC-Engine retest
  pending.
- **Symptom:** PC-Engine printed `HOSTFAT read-only drive ready` while loading
  `HOSTFAT.SYS`, then hung before completing CONFIG.SYS processing.
- **Root cause:** both the guest driver and emulator service treated the
  non-IBM block request as an 18-byte packet. PC-Engine uses a 13-byte common
  header, including eight reserved bytes at offsets 5--12, followed by the
  media/unit byte at `0DH`, transfer pointer at `0EH`, count or BPB pointer at
  `12H`, and starting sector at `14H`. The wrong initialization offsets made
  PC-Engine reclaim most of the resident driver and later execute overwritten
  code.
- **Correction:** the driver and host service now use the complete 22-byte
  layout. The ROM-less transport test fills the reserved header bytes with a
  nonzero pattern, and the generated-driver checker verifies the exact field
  displacements in emitted machine code so the former layout fails closed.
- **Verification:** clean GCC, Clang, and ASan/UBSan suites pass all 36 tests
  apart from the configured external-corpus skip; the former generated SYS is
  rejected by the strengthened checker and two independent corrected NASM
  outputs are byte-identical. A private live PC-Engine boot printed the ready
  message and reached the command prompt instead of hanging. G54 retains live
  DIR/TYPE/copy/write-protect and reset checks.
- **Evidence:** [M54 task](../agents/tasks/M54_hostfat_readonly_prototype.md)
  and [M54 report](../agents/reports/m54_hostfat_readonly_prototype.md).
- **Commit:** [a07a8c4a](https://github.com/nakatamaho/vaeg/commit/a07a8c4a764a2b5d8560bdbaea8f5ebc5c0edae4).

### VA mode did not expose the emulator-private value/string channels

- **Status:** fixed in M54; G54 PC-Engine integration review pending.
- **Symptom:** a PC-88VA guest driver using vaeg's established emulator-private
  interface could not exchange the scalar values required by a request-packet
  protocol. The generic and VA I/O paths also disagreed about whether ports
  `07EDH` and `07EFH` were present.
- **Root cause:** `np2sysp_bind()` attached both scalar and string callbacks to
  `07EFH`, so the later string attachment replaced the scalar callback. It
  attached only the generic I/O table even though active VA execution uses the
  separate VA table.
- **Correction:** `07EDH` now carries four-byte values and `07EFH` carries
  command/response strings in both the generic and VA tables. These remain
  emulator-private channels; no physical PC-88VA port was reassigned.
- **Verification:** the M54 ROM-less test sends a version probe through both
  tables, performs a sector transfer through the VA table, and verifies that
  malformed packet, count, LBA, destination, and unmounted-image failures do
  not modify the guest destination. G54 retains a real PC-Engine driver gate.
- **Evidence:** [M54 task](../agents/tasks/M54_hostfat_readonly_prototype.md)
  and [M54 report](../agents/reports/m54_hostfat_readonly_prototype.md).
- **Commit:** [f79b677c](https://github.com/nakatamaho/vaeg/commit/f79b677c1e48071779349a4ac3b404ed291f821a).

### REP-prefixed 0F could enter unverified 80286 protected-mode behavior

- **Status:** fixed by the G47-approved M48 fail-closed policy; G48 human
  review pending.
- **Symptom:** F2/F3-prefixed 0F could run inherited NP2 80286 system handlers;
  in particular, F2/F3 0F 01 F0 could set `MSW.PE`. A saved state with
  `MSW.PE` set could then activate legacy selector processing after import.
- **Root cause:** `v30op_repne[0x0f]` and `v30op_repe[0x0f]` inherited
  `i286c_cts`, while CPU286 state validation accepted `MSW.PE`. M47's source,
  runtime, and state audit demonstrated both active paths. No primary source
  or pinned V20 record establishes that behavior as correct uPD9002/V52
  semantics.
- **Correction:** both dispatch slots now latch an emulator diagnostic and
  restore the complete pre-instruction runtime state before DMA or VA-device
  scheduling. State preflight rejects `MSW.PE` transactionally; dormant
  descriptor residue with PE clear remains opaque and compatible. This is a
  safety policy, not an architectural instruction claim.
- **Verification:** a 522-case regression covers all F2/F3+0F second bytes,
  segment-prefix entry, PE-set runtime state, full CPU-state equality, memory
  equality, and persistent stop. Direct and full-file state tests prove the
  new rejection leaves CPU runtime, compatibility image, PCCORE, UPD9002, and
  memory unchanged. The M48 transition manifest accounts for every graph,
  provenance, and support row while preserving all M42/M43 historical files.
- **Evidence:** [ADR-0013](../agents/DECISIONS/ADR-0013-upd9002-rep0f-correctness.md),
  [M48 task](../agents/tasks/M48_upd9002_rep0f_implementation.md), and
  [M48 report](../agents/reports/m48_upd9002_rep0f_implementation.md).
- **Commits:** [bc00b370](https://github.com/nakatamaho/vaeg/commit/bc00b370480283dbf7f7529fc6345def87a7dc75)
  and [9924b85c](https://github.com/nakatamaho/vaeg/commit/9924b85ca13a87610571392968ca63bd74e85321).

### The M48 diagnostic-stop poll slowed active uPD9002 runtime execution

- **Status:** fixed in M73; G73 human review pending.
- **Symptom:** maintainer runtime testing found that older CI binaries through
  approved G43 remained fast, while the approved G48 binary was slow on the
  same notebook workload. The slowdown was host-runtime visible and did not
  require an SST semantic mismatch.
- **Root cause:** M48 correctly added fail-closed REP-prefixed `0F`
  diagnostics, but the scheduler hot loop tested the diagnostic latch through
  an out-of-line `upd9002_diagnostic_pending()` function after every
  `v30c_step()`. For ordinary execution the result is almost always false, so
  this became a high-frequency call overhead on every uPD9002 instruction.
- **Correction:** the diagnostic state remains a single shared latch, but the
  pending test is now an inline macro reading the latch reason directly. The
  REP+0F fail-closed behavior, message, state atomicity, and diagnostic getter
  remain unchanged.
- **Verification:** the focused M48 diagnostic, M68 mapped-memory, M69 status,
  M70 prefix/string, full ROM-less CTest suite, normal macOS build, and MinGW
  cross build passed. The MinGW executable
  `/tmp/vaeg-m73-inline-diagnostic.exe` had SHA-256
  `cd3cd52fc5b83b9831acebfd7a2b1178b4ad7c18e657f2bd0bbd3e47cc547221`; the
  maintainer reported it was fast.
- **Evidence:** [M73 task](../agents/tasks/M73_upd9002_post_m49_performance_regression.md)
  and [M73 report](../agents/reports/m73_upd9002_post_m49_performance_regression.md).
- **Commit:** [e7ac7e93](https://github.com/nakatamaho/vaeg/commit/e7ac7e930c685e565bff131a42fe48f08c799990).

### Invalid CPU286 state payloads could partially alter the machine

- **Status:** fixed in M44 implementation; G44 human review pending.
- **Symptom:** a malformed CPU286 section, or a legacy payload selecting a CPU
  type other than V30, could be discovered only while loading raw live state,
  after unrelated machine sections had begun to change.
- **Root cause:** the CPU286 section used the generic raw binary statsave path,
  which had no CPU-specific size/type validation and no complete preflight
  before live-section application.
- **Correction:** introduced a dedicated CPU286 serialization adapter and
  statsave handler. It validates the complete temporary payload, requires
  `CPUTYPE_V30`, constructs temporary runtime state, and commits the runtime and
  opaque compatibility image together only after full-file preflight.
- **Verification:** invalid CPU type, malformed declared size, and truncation
  tests preserve CPU runtime, compatibility bytes, PCCORE, UPD9002 registers,
  and memory. Raw-G41/current bidirectional tests preserve every valid CPU286
  and UPD9002 payload byte, including reset and CPU_SHUT behavior.
- **Evidence:** [M44 state-boundary report](../agents/reports/m44_upd9002_state_boundary.md)
  and [M44 task](../agents/tasks/M44_upd9002_state_boundary.md).
- **Commits:** [2895c113](https://github.com/nakatamaho/vaeg/commit/2895c11354c73b1758b6c06fad4b5c5ec8e68570)
  and [8e709db3](https://github.com/nakatamaho/vaeg/commit/8e709db3431a3a7c64f7040c4cf719e5102de559).

### Portable V30/uPD9002 execution did not match the VA CPU path

- **Status:** fixed in M9.
- **Symptom:** the initial portable C core could not reliably execute the VA
  firmware and software paths handled by the frozen V30 core.
- **Root cause:** the portable opcode tables and reset handoff were missing or
  incorrectly mapping V30 behavior, including REPC carry semantics, REPC
  dispatch, POP SP, V30 IRET/trap handling, and preservation of V30 CPU type
  during reset.
- **Correction:** ported the required V30 handlers and table wiring, matched
  uPD9002 POP SP behavior, and restored the legacy V30 reset/mode handoff.
- **Evidence:** [V30/uPD9002 map](../agents/reports/m9_v30_map.md) and
  [M9 boot comparison](../agents/reports/m9_boot_debug.md).
- **Commits:** [b9b0da1](https://github.com/nakatamaho/vaeg/commit/b9b0da147d501e67e14eaba0a90d91f7a05eaf3b),
  [27b1712](https://github.com/nakatamaho/vaeg/commit/27b1712d15c9b1cd1af84bc92e625fc26bfea92e),
  [7765038](https://github.com/nakatamaho/vaeg/commit/77650387f3e62f602ca9fe4410fc22777202f48c),
  [dcb6939](https://github.com/nakatamaho/vaeg/commit/dcb6939492e3fd896a17ad2052484b7bb3b77ccd), and
  [ee1e9e9](https://github.com/nakatamaho/vaeg/commit/ee1e9e91e8fab4a2c4177a09a13def44c3603d03).

### Direct-mode FDD DMA stopped before a sector was transferred

- **Status:** fixed in M9.
- **Symptom:** after switching the VA FDD interface to direct mode, a
  1024-byte Read Data operation transferred only 35 bytes in the portable
  build, preventing normal disk boot.
- **Root cause:** the portable V30 loop used the plain V30 DMA pump instead of
  the VA-aware i286 cadence used by the frozen implementation.
- **Correction:** routed the active V30 execution and step loops through the
  VA-aware DMA pump and honored extended DRQ state.
- **Verification:** differential FDC/DMAC traces showed the same channel,
  range, and bank with the transfer reaching 1024 bytes.
- **Evidence:** [M9 boot comparison](../agents/reports/m9_boot_debug.md).
- **Commit:** [cc7a154](https://github.com/nakatamaho/vaeg/commit/cc7a154d2c4839a1a59d67cbfccb865a18f8f695).

### SDL2 pacing starved host-time work and diverged from legacy frame skip

- **Status:** fixed in M9.
- **Symptom:** FDD motor/seek sound events could remain dead or stuck, and
  automatic frame skipping did not follow the established VAEG cadence.
- **Root cause:** a simplified pending-frame loop stopped pumping the host
  tick while waiting and did not preserve the legacy NOWAIT/fixed/auto-skip
  state machine.
- **Correction:** serviced task and host ticks on every outer loop iteration
  and restored one-frame-at-a-time legacy auto-skip pacing with a bounded
  catch-up limit.
- **Verification:** `--pacelog`, FDD host-event testing, and the G9 timing
  comparison.
- **Evidence:** [M9 boot and pacing analysis](../agents/reports/m9_boot_debug.md).
- **Commits:** [0909547](https://github.com/nakatamaho/vaeg/commit/090954705ac235c1b47692bf9376e87c21b257d0) and
  [733b4fa](https://github.com/nakatamaho/vaeg/commit/733b4faf1804953967b23bce84c533db1e7c9926).

### Runtime handles and pointers were truncated on 64-bit Windows

- **Status:** fixed in M11.
- **Symptom:** MinGW LLP64 builds could truncate `FILE *`, handle, callback,
  and runtime pointer values stored through 32-bit `long`, risking crashes or
  invalid file/state access.
- **Root cause:** ILP32 assumptions inherited from the Win32 tree were used in
  active 64-bit runtime structures and conversions.
- **Correction:** stored runtime file handles as `FILEH`, introduced
  pointer-sized runtime conversions, and removed unsafe pointer-to-long
  comparisons while preserving serialized formats.
- **Verification:** the LLP64 audit classified each conversion and the MinGW
  build completed after the corrections.
- **Evidence:** [M11 LLP64 audit](../agents/reports/m11_llp64_audit.md).
- **Commits:** [3f0f4ce](https://github.com/nakatamaho/vaeg/commit/3f0f4ce8356503b5fd43713e5893aace84354afd),
  [08723d5](https://github.com/nakatamaho/vaeg/commit/08723d561930d88267d5935e6192d891e010467a), and
  [7faae73](https://github.com/nakatamaho/vaeg/commit/7faae736d711faa04e36100c5644dd70be42c1fc).

### VA backup memory was not consistently loaded and saved by SDL2

- **Status:** fixed in M11, with portable lookup aligned again in M19.
- **Symptom:** VA backup state could be missed or not persisted across active
  SDL2 sessions, depending on frontend and state-path selection.
- **Root cause:** the portable main lifecycle did not explicitly load/save VA
  backup memory, and path selection differed from the intended portable
  executable-local/user-state policy.
- **Correction:** connected backup-memory load/save to SDL2 startup/shutdown
  and unified its path priority with active configuration lookup.
- **Verification:** state-path tests and human persistence checks.
- **Evidence:** [M11 portability task](../agents/tasks/M11_mingw_macos.md) and
  [M19 portable runtime task](../agents/tasks/M19_portable_runtime.md).
- **Commits:** [06aaa90](https://github.com/nakatamaho/vaeg/commit/06aaa90a95952932d0f9aaebd2624d28f0863bfd) and
  [4d4f8a0](https://github.com/nakatamaho/vaeg/commit/4d4f8a01d4033f09898305d0d0353aedcb65bb10).


### Configuration and backup memory silently shared host state

- **Status:** fixed in M74.
- **Symptom:** normal launches could silently read configuration and backup
  memory from executable or per-user state directories, and VA and VA2 shared
  one backup-memory filename. Deterministic runs in separate working
  directories could therefore inherit unrelated host state or cross-model
  backup contents.
- **Root cause:** the SDL2 frontend retained executable/user-state fallback
  lookup and selected one `vabkupmem.dat` path before the final boot model was
  known.
- **Correction:** configuration now defaults to `./vaeg.cfg`; backup memory
  defaults to `./vabkupmem.dat` for VA and `./va2bkupmem.dat` for VA2/VA3;
  `--cfg` and `--bkupmem` select exact paths; `--no-cfg` and `--no-bkupmem`
  disable both reads and writes; and no fallback or implicit migration occurs.
- **Verification:** CLI/parser and persistence selftests, ROM-less current-
  directory smoke runs for both models, explicit-path and disabled-mode smoke
  runs, native trace-enabled and trace-disabled builds, and the MinGW cross
  build.
- **Evidence:** [M74 deterministic debug harness report](../agents/reports/m74_debug_harness.md).
- **Commit:** [433e6e3](https://github.com/nakatamaho/vaeg/commit/433e6e31a89ed7af4cbff6a7a6a269c634913faa).

### JIS Kana and punctuation scancodes produced incorrect guest keys


- **Status:** fixed in M14.
- **Symptom:** JIS Yen/pipe and related punctuation could be missing or mapped
  to US physical positions, and the Right Alt Kana-lock path did not behave as
  the selected JIS/Roman mode required.
- **Root cause:** several SDL scancodes needed explicit JIS physical actions;
  the US keytop translation and JIS physical preset could not share all
  punctuation assumptions.
- **Correction:** separated JIS physical mappings from US keytop actions and
  corrected Kana and punctuation bindings without injecting text directly.
- **Verification:** ROM-less mapping tests and the M14 human JIS/US keyboard
  gate.
- **Evidence:** [M14 keyboard mapping task](../agents/tasks/M14_keyboard_mapping.md)
  and [keyboard mapping reference](keyboard-mapping.md).
- **Commits:** [57be6f1](https://github.com/nakatamaho/vaeg/commit/57be6f1a658620a182f0efecacf2cf51aa7c0576) and
  [8bb09b4](https://github.com/nakatamaho/vaeg/commit/8bb09b40854a8e24ac59465d4c7a134f07c134d1).

### VA2/VA3 could silently use the wrong model ROM set

- **Status:** fixed in M18.
- **Symptom:** the flat historical ROM layout allowed VA and VA2/VA3 ROMs with
  overlapping names to be confused, producing model-dependent startup
  failures that were difficult to diagnose.
- **Root cause:** one unsuffixed lookup namespace was used for distinct model
  ROM contents.
- **Correction:** VA keeps unsuffixed files while VA2/VA3 requires MAME-style
  `*_va2.rom` names with no VA fallback; size, CRC32, and SHA-1 diagnostics
  identify mismatches.
- **Verification:** G18 model boot checks and checksum diagnostics.
- **Evidence:** [M18 ROM layout task](../agents/tasks/M18_rom_layout.md).
- **Commit:** [f59c106](https://github.com/nakatamaho/vaeg/commit/f59c106e4789217326cb53153908de49873a9e7b)
  and its linked M18 topic history.

### SDL2 dropped the rightmost VA guest pixel

- **Status:** fixed in M21.
- **Symptom:** the right edge of the 640-pixel VA display was one pixel short.
- **Root cause:** the legacy converter produces a 641-pixel row containing a
  left guard followed by 640 guest pixels, but SDL2 uploaded the first 640
  pixels and therefore displayed the guard while dropping guest pixel 639.
- **Correction:** retained the converter contract but uploaded from the first
  guest pixel and made uniform-frame checks use the same visible span.
- **Verification:** ROM-less checks cover the guard, guest pixels 0 and 639,
  and the 641-pixel backing row; the maintainer confirmed the edge display.
- **Evidence:** [M21 SDL2 display task](../agents/tasks/M21_sdl2_display_effects.md).
- **Commit:** [caaf97c](https://github.com/nakatamaho/vaeg/commit/caaf97c56dd39e861c12537a38b6b31b43bd8722).

### V30 LOOP timing made firmware BEEP delays too short

- **Status:** fixed in M21.
- **Symptom:** BASIC `BEEP` duration was shorter than original VAEG/hardware
  behavior in VA and VA2/VA3 modes.
- **Root cause:** opcode `E2` inherited i286 LOOP timing instead of the V30
  timing used by ROM delay loops.
- **Correction:** added a V30-specific LOOP handler using 17 clocks when taken
  and 5 clocks on termination, leaving i286, PIT, audio, and frozen code
  unchanged.
- **Verification:** ROM-less timing tests and maintainer VA/VA2 BASIC BEEP
  comparison.
- **Evidence:** [M21 SDL2 display task](../agents/tasks/M21_sdl2_display_effects.md).
- **Commit:** [a06ab6e](https://github.com/nakatamaho/vaeg/commit/a06ab6ec24f03116152492fa3a0e3c69d87830ad).

### Restored archive mounts disappeared from the FDD menu

- **Status:** fixed in M22.
- **Symptom:** an archive-extracted FDD image remained mounted after restart
  but its filename was no longer shown in the FDD menu.
- **Root cause:** the menu used transient archive/frontend state instead of
  the live FDD mount path restored from configuration.
- **Correction:** render FDD1/FDD2 labels from live disk state, including the
  delayed insertion path, and preserve full-path hover text.
- **Verification:** M22 persistence gate across restart.
- **Evidence:** [M22 disk-image drop task](../agents/tasks/M22_disk_image_drop.md).
- **Commit:** [afeee5b](https://github.com/nakatamaho/vaeg/commit/afeee5b20e23a27b4f5a1e75024fe1dbb0afe5f).

### Archive loading was unavailable or failed on POSIX Japanese paths

- **Status:** fixed in M22.
- **Symptom:** Linux release builds could report archive support unavailable;
  archives containing UTF-16LE/Japanese entry names could fail with a locale
  conversion error.
- **Root cause:** release dependency availability varied by build host, while
  LibArchive pathname conversion ran under the startup C locale.
- **Correction:** pinned the release archive dependency stack and perform
  POSIX entry-name decoding under a thread-local UTF-8 locale without changing
  global process locale.
- **Verification:** Linux release selftest/smoke and a Japanese 7z basename
  regression case.
- **Evidence:** [M22 disk-image drop task](../agents/tasks/M22_disk_image_drop.md).
- **Commits:** [bd905a2](https://github.com/nakatamaho/vaeg/commit/bd905a2b78fd950b9a8cc76c44427f25150f6445) and
  [c05411e](https://github.com/nakatamaho/vaeg/commit/c05411e5760d57d9530949a18fc4b19550ba0c2c).

### FDD Open exposed managed extraction storage after an archive drop

- **Status:** fixed in M32; macOS arm64 archive-browser check passed.
- **Symptom:** after dragging and dropping a ZIP, 7z, or LZH archive, opening
  FDD1/FDD2 Open started the browser in the managed user-state extraction
  directory instead of the directory containing the source archive.
- **Root cause:** the FDD browser always derived its initial directory from the
  live mounted image path. Archive mounts necessarily replace that path with
  an extracted image path, while the archive loader retained no association
  with the source archive directory.
- **Correction:** retain the source directory per mounted drive and extracted
  image, use it only when the live mount still matches, and store the
  association beside each managed image so it survives application restart
  and is removed by the existing prune lifecycle.
- **Verification:** ROM-less dropmedia tests cover ZIP and 7z source capture,
  metadata reload, drive/path matching, and unrelated-path rejection. Two
  apparent macOS arm64 failures used a stale executable copied to the wrong
  destination and were not valid results. With the `ce26003` build installed
  at the actual test location, the maintainer confirmed that FDD Open starts
  in the source ZIP directory and declared the defect fixed. The macOS
  MacPorts and MinGW cross targets build successfully and the full ROM-less
  selftest passes.
- **Evidence:** [M32 command-line startup task and G32 follow-up](../agents/tasks/M32_cli_startup_overrides.md#g32-archive-browser-follow-up).
- **Commit:** [ce26003782cec9b93639cc34b2e33c5de3e63d8a](https://github.com/nakatamaho/vaeg/commit/ce26003782cec9b93639cc34b2e33c5de3e63d8a).

### VA1 PC-Engine 1.00 selected a stack in banked TVRAM

- **Status:** fixed in M29; focused human boot gate passed.
- **Symptom:** a clean PC-Engine 1.00 system disk booted in VA2/VA3 but failed
  to complete startup in VA mode.
- **Root cause:** system-memory bank 1 exposed the full legacy 256KB
  `A0000H-DFFFFH` text backing to the CPU although VA TVRAM is only 64KB at
  `A0000H-AFFFFH`. The memory probe therefore selected `D000:FFFx` for its
  stack. A VA ROM switch from bank 1 to backup-memory bank 9 hid a pushed map
  value; the subsequent pop restored `FFFFH`, selected bank F, and corrupted
  stack/interrupt state.
- **Correction:** limited VA1 CPU-visible bank-1 TVRAM to 64KB, returned
  open-bus ones, and ignored writes in `B0000H-DFFFFH`, including
  boundary-crossing word access.
- **Verification:** ROM-less aperture tests and successful maintainer VA1
  PC-Engine 1.00 boot.
- **Evidence:** [M29 VA1 TVRAM aperture task](../agents/tasks/M29_va1_tvram_aperture.md).
- **Commit:** [c17d64a](https://github.com/nakatamaho/vaeg/commit/c17d64a71f6f32cf9ce6cd070da7ae3e68899af6).

### The M29 TVRAM clamp regressed VA2 V3 BASIC

- **Status:** fixed during M31 verification.
- **Symptom:** M28 could enter and use VA2 V3 BASIC, while its direct child
  M29 froze after entering BASIC. VA1 PC-Engine 1.00 still required the M29
  aperture correction.
- **Root cause:** the M29 `A0000H-AFFFFH` limit was implemented in shared
  `tvram_*()` handlers without a model check, so it changed the VA2/VA3
  memory path as well as VA1. NEC specifications identify 64KB of TVRAM in
  VA1 and 256KB in VA2/VA3.
- **Correction:** apply the 64KB/open-bus behavior only to `PCMODEL_VA1` and
  preserve the 256KB bank-1 backing behavior for `PCMODEL_VA2`. This matches
  the documented physical TVRAM capacities; the VA2 V3 BASIC regression test
  also exercises the active `A0000H-DFFFFH` mapping.
- **Verification:** ROM-less tests cover both model-specific mappings. Human
  testing confirmed VA2 V3 BASIC and VA1 PC-Engine 1.00; the separate inherited
  VA1 V3 BASIC command failure remains open.
- **Evidence:** [M29 VA1 TVRAM aperture task](../agents/tasks/M29_va1_tvram_aperture.md),
  [M31 CLI boot-model task](../agents/tasks/M31_cli_boot_model.md),
  [NEC PC-88VA specification](https://support.nec-lavie.jp/support/product/data/spec/cpu/b047-1.html),
  [NEC PC-88VA2 specification](https://support.nec-lavie.jp/support/product/data/spec/cpu/b048-1.html),
  [NEC PC-88VA3 specification](https://support.nec-lavie.jp/support/product/data/spec/cpu/b049-1.html),
  and the [PC-88VA hardware comparison](http://www.pc88.gr.jp/~va/va-hard.html#mem).
- **Commit:** [c580222](https://github.com/nakatamaho/vaeg/commit/c5802228f1d8f7cf91b41d1182aaad4ebd30ccea).

### VA BMS bank zero hid 128KB of conventional memory

- **Status:** M30's open-bus interpretation was corrected in M52 and G52
  passed. The post-G90 expansion-memory-default hotfix regressed this policy;
  the follow-up correction described below restores it.
- **Symptom:** enabling I/O Bank Memory with 640KB main memory prevented
  CONFIG.SYS RAM-disk and MSE registration. Reducing main memory to 512KB
  avoided the failure by leaving `80000H-9FFFFH` unused.
- **Affected scope:** CPU byte/word and SGP word access to the 128KB BMS
  aperture, both while BMS was disabled and after the driver reset the bank
  selector to zero.
- **Demonstrated root cause:** the M30 portable handlers treated selector zero
  as expansion bank zero and permanently overlaid `80000H-9FFFFH`. RDBMS 1.21
  instead selects a nonzero bank for each transfer and writes zero in its
  `ResetBank` macro to restore conventional memory.
- **Correction:** selector zero now passes through ordinary main RAM; selectors
  1 through N map one-to-one onto N allocated 128KB banks. Invalid nonzero
  selectors remain open bus. CPU and SGP paths use the same rule.
- **Verification:** the updated ROM-less lifecycle test covers disabled and
  enabled selector-zero pass-through, bank-one isolation, ordinary-reset
  retention, and disable-time restoration. Maintainer guest testing with
  640KB main memory is the G52 gate.
- **Evidence:** [M30 historical task](../agents/tasks/M30_va_bms_window.md) and
  [M52 corrected I/O Bank Memory task](../agents/tasks/M52_io_bank_memory.md).
- **Commit:** [5eb04ae9](https://github.com/nakatamaho/vaeg/commit/5eb04ae91a9900833096bb43b3b599d358c099c5).

### VA bank memory defaulted to the PC-9801 compatibility port

- **Status:** fixed in the M52 implementation; G52 passed.
- **Symptom:** a clean VAEG configuration selected `00ECH`, so a PC-88VA bank
  memory driver configured for the machine-native `01D0H` control port could
  not select the emulated banks without a matching manual configuration
  change.
- **Affected scope:** clean configurations and invalid persisted BMS port
  values. An explicitly saved valid `00ECH` selection remains supported and
  is not migrated.
- **Demonstrated root cause:** the restored portable dialog inherited the
  first generic BMS choice from the frozen frontend. The bundled historical
  specification help identifies `00ECH` as the PC-9801 choice and `01D0H` as
  the PC-88VA-01/02 choice, but the active default still used `00ECH`.
- **Correction:** made `01D0H` the active clean-config default, invalid-value
  fallback, and first GUI choice while retaining `00ECH` as an explicit
  compatibility option. BMS remained disabled by default at M52; the later
  expansion-memory-default hotfix enables it without changing the port choice.
- **Verification:** the ROM-less selftest checks the exact port constants and
  copied runtime default; Linux and MinGW clean release builds pass their
  selftests, including BMS configuration/window lifecycle coverage. A
  black-box run loading `BMS_Port=1234` logs fallback to `01d0` before machine
  startup.
- **Evidence:** [M52 I-O Bank Memory task](../agents/tasks/M52_io_bank_memory.md).
- **Commit:** [e9ad63e3](https://github.com/nakatamaho/vaeg/commit/e9ad63e3d720e8dad14d5a63289f3d3443b54422).

### VA zero-based BMS mapping hid 128KB of 640KB main RAM

- **Status:** fixed in the post-G90 follow-up hotfix.
- **Symptom:** after the expansion-memory-default hotfix, PC-Engine could boot
  with only 512KB visible, and selecting 640KB in the BIOS produced repeated
  invalid-drive-name errors or a damaged startup screen in both VA and VA2
  modes.
- **Affected scope:** CPU byte/word and SGP word access at
  `80000H-9FFFFH`, with or without an installed BMS driver. The 16MB BMS,
  native `01D0H` port, and 13MB EMS defaults remain valid.
- **Demonstrated root cause:** commit `2425f99` changed selector zero from the
  M52 main-RAM pass-through into BMS storage and numbered expansion banks from
  zero. The BIOS still advertised 640KB, while RDBMS selected nonzero banks for
  transfers and wrote zero afterward. Each reset-to-zero operation therefore
  exposed a different 128KB allocation instead of restoring upper main RAM.
- **Correction:** selector zero again reaches ordinary main RAM and selectors
  1 through N map the configured N BMS banks in both CPU and SGP paths. Clean
  configuration continues to provide 640KB main RAM, an enabled 128-bank
  (16MB) BMS at `01D0H`, and 13MB EMS.
- **Verification:** the ROM-less lifecycle selftest covers disabled and
  enabled selector-zero pass-through, bank-one isolation, reset retention,
  disable-time restoration, and unchanged BMS storage. Normal macOS and MinGW
  cross-builds completed. With the reproducibly generated development D88,
  both VA and VA2 booted with 640KB to `Ready`, and `DIR C:` returned the BMS
  RAM disk without the invalid-drive-name failure.
- **Evidence:** [M52 I/O Bank Memory task](../agents/tasks/M52_io_bank_memory.md)
  and [PC-88VA BMS/EMS environment notes](pc88va-utility-media.md#bank-memory-manager).
- **Commit:** [c52bd8d](https://github.com/nakatamaho/vaeg/commit/c52bd8dbc62dfabc5d7bbbc50b4fbfe7bd6deef4).

### Original VA ignored the RDBMS native-port override

- **Status:** fixed in the post-G90 development-disk hotfix.
- **Symptom:** the generated disk detected the 16MB BMS and registered RDBMS
  in VA2 mode, but original-VA mode reported that no BANKRAM board was
  installed and fell back to taking 128KB at segment `8000H`.
- **Affected scope:** RDBMS 1.21 on original-VA PC-Engine boots using the
  native `01D0H` BMS port. VA2 accepted the existing `-P1D0` line.
- **Demonstrated root cause:** RDBMS's own documentation states that some
  `CONFIG.SYS` parameters cannot be set on an original VA. The distributed
  `RDBMS.SYS` embeds the PC-9801-compatible `00ECH` port at offset `001AH`, so
  original-VA boot ignored `-P1D0` and probed a port different from VAEG's
  configured native VA port.
- **Correction:** the development-disk builder preserves the source archive,
  verifies the exact original driver hash and `EC 00` bytes, then changes only
  the generated copy's embedded default to `01D0H`. `CONFIG.SYS` retains
  `-P1D0` for models that accept parameters; at this stage VAEG intentionally
  did not mirror the board at the compatibility port.
- **Verification:** two complete builds produced byte-identical D88 images.
  The disk-extracted driver had the expected patched hash and `D0 01` word.
  Original-VA and VA2 640KB boots both reached `Ready`, and `DIR C:` accessed
  the RDBMS RAM disk without the BANKRAM warning.
- **Evidence:** [PC-88VA BMS/RDBMS environment notes](pc88va-utility-media.md#bank-memory-manager).
- **Commit:** [e08fd93](https://github.com/nakatamaho/vaeg/commit/e08fd938e6157ad9d05bb26dab6acc2842b3d192).

### Fixed BMS driver could not probe the VAEG native-first bank device

- **Status:** fixed in M97e; real VA/VA2 guest confirmation remains pending.
- **Symptom:** with the unchanged development-disk `BMSDRVA.SYS`, the guest
  reported the compatibility `00ECH` probe and only one visible bank even
  though VAEG was configured with the native `01D0H` device and its full
  capacity.
- **Affected scope:** BMS selector I/O during VA and VA2 startup, including
  software that probes `00ECH` before selecting the native VA port.
- **Demonstrated root cause:** the fixed BMS driver probes `00ECH`, while the
  VAEG device had been registered only at the configured `01D0H` address. An
  unhandled compatibility-port read therefore looked like a one-bank device;
  the disk driver was not defective and is intentionally unchanged.
- **Correction:** when BMS is enabled, register the same bank-selector
  callbacks at both `01D0H` and `00ECH`. The native `01D0H` setting remains the
  default and preferred configuration; both aliases share one bank state and
  storage, without adding a second device or modifying `BMSDRVA.SYS`.
- **Verification:** the ROM-less selftest selects banks through each alias in
  both native-preference and compatibility-preference configurations, checks
  that reads report the shared selection, and retains the existing aperture,
  reset, and disable lifecycle checks. Linux debug build, selftest, repository
  encoding/EOL/case checks, and diff validation pass.
- **Evidence:** [M97e BMS dual-port report](../agents/reports/m97e_bms_dual_port.md),
  [PC-88VA BMS/EMS environment notes](pc88va-utility-media.md#bank-memory-manager).
- **Commit:** [8386b4b](https://github.com/nakatamaho/vaeg/commit/8386b4b2986edaeac6660f9622c343d96ec1e50c).

### Z80 state-codec rejection was ignored by the state coordinator

- **Status:** fixed in M39.
- **Symptom:** a save whose embedded Z80 status revision was unsupported could
  continue through `statsave_load()` as though the subsystem CPU had restored
  successfully, leaving a partially loaded machine rather than reporting the
  incompatible state.
- **Affected scope:** revision-1 subsystem state loading under both production
  Z80 selections; valid revision-1 images are unchanged.
- **Demonstrated root cause:** the subsystem C bridge returned `void` and
  discarded `Z80C::LoadStatus()`'s Boolean result; `flagload_subsystemcpu()`
  therefore had no failure to propagate.
- **Correction:** return success/failure from the subsystem save/load bridge
  and convert a codec rejection into `STATFLAG_FAILURE` in the existing state
  coordinator. No scheduler or status-image layout changed.
- **Verification:** the ROM-less state test saves a valid complete state,
  copies it, changes only revision-1 byte 59 to unsupported revision 2, and
  requires top-level `statsave_load()` failure under both `legacy` and
  `suzukiplan`; the original image then loads and re-saves successfully.
- **Evidence:** [M39 integration task](../agents/tasks/M39_z80_integration.md)
  and [M39 integration contract](z80-compatibility-integration.md#state-boundary-and-error-handling).
- **Commit:** [23b7071](https://github.com/nakatamaho/vaeg/commit/23b70711b84deb027a1c8dbf11e6284b65d0d4fe).

### uPD9002 guest-visible FLAGS images lost or loaded reserved bits

- **Status:** fixed in the M60a implementation; G60a candidate review pending.
- **Symptom:** the saved FLAGS word in every executing `INT3`, `INT imm8`, and
  interrupting `INTO` frame lost bits 12 through 15. `POPF` and `SAHF` also
  loaded reserved bits 3 and 5 instead of forcing them to zero.
- **Affected scope:** guest-visible FLAGS materialization for software
  interrupt frames and `PUSHF`, plus the SST-observed loading rules for
  `POPF` and `SAHF`. Interrupt eligibility, vectoring, frame placement,
  stack addressing, `IRET`, and final FLAGS comparison contracts are
  unchanged.
- **Demonstrated root cause:** the software-interrupt path saved
  `REAL_FLAGREG`, which masks the internal FLAGS word to 12 bits. The V30
  `POPF` path copied the popped low FLAGS bits directly, and `SAHF` copied AH
  directly, so neither path applied the target-observed zero rule for bits 3
  and 5.
- **Correction:** construct the interrupt and `PUSHF` images explicitly from
  all 16 stored FLAGS bits plus the split overflow bit, and apply explicit,
  instruction-specific masks when loading `POPF` and `SAHF`. `LAHF` remains
  unchanged.
- **Verification:** focused deterministic tests cover ordinary, 64-KiB
  segment-wrap, and 20-bit physical-wrap frames and all affected bit rules.
  The complete architectural SST population improved from 84,329 to 60,582
  failures: all 19,968 directly targeted records passed, as did 3,565 BOUND
  and 214 divide-fault records that use the corrected saved-FLAGS primitive.
  No hash became newly failing, and all required profiles completed with zero
  timeout or crash. Hosted Linux GCC/Clang/ASan, macOS, Windows-MinGW, Z80
  conformance, repository-invariant, and architectural-ratchet jobs passed.
- **Evidence:** [M60a task](../agents/tasks/M60a_upd9002_flags_materialization.md)
  and [M60a report](../agents/reports/m60a_upd9002_flags_materialization.md).
- **Commit:** [aab78b78](https://github.com/nakatamaho/vaeg/commit/aab78b78a2473ce35b1e28a9af7420e46e72a1c4).

### uPD9002 IRET loaded reserved FLAGS bits 3 and 5

- **Status:** fixed in the M60e implementation; G60e candidate review pending.
- **Symptom:** 3,769 of the 5,000 applicable `CF IRET` SST records failed
  because the restored FLAGS value retained reserved bits 3 and 5 from the
  stack.
- **Affected scope:** real-mode uPD9002 `IRET` FLAGS restoration only. Stack
  word order, logical and physical stack addresses, restored IP and CS, final
  SP, termination, interrupt entry, and other FLAGS instructions are
  unchanged.
- **Demonstrated root cause:** the V30 `IRET` path masked the popped FLAGS word
  with `0x0fff`, which made bits 3 and 5 loadable. Complete pre-fix replay
  showed that the SST-observed rule forces both bits to zero while every other
  observed IRET rule already matched.
- **Correction:** use the IRET-specific `0x0fd7` stack mask, clearing only
  bits 3 and 5. The underdetermined bit 8 and the existing internal high-FLAGS
  representation are deliberately preserved.
- **Verification:** deterministic focused tests cover ordinary, 16-bit
  segment-wrap, and 20-bit physical-wrap stack reads and explicit FLAGS bit
  rules. The complete CF population improved from 1,231 pass / 3,769 fail to
  5,000 pass / 0 fail. Architectural full failures fell from 59,941 to
  56,172, with no newly failing hash, timeout, crash, or protected-form
  regression.
- **Evidence:** [M60e IRET task](../agents/tasks/M60e_upd9002_iret.md) and
  [M60e report](../agents/reports/m60e_upd9002_iret.md).
- **Commit:** [7f815acb](https://github.com/nakatamaho/vaeg/commit/7f815acb26f1be546bbcfd5de12972235dfd175c).

### uPD9002 C6/C7 register forms wrote the ModR/M extension register

- **Status:** fixed in the M61 implementation; G61 candidate review pending.
- **Symptom:** 1,088 C6 and 1,120 C7 applicable SST records failed. The
  encoded destination did not receive the immediate, and a different register
  could change instead.
- **Affected scope:** register-destination forms of `C6 /0 MOV r/m8, imm8` and
  `C7 /0 MOV r/m16, imm16`. Their memory forms and all other instruction
  families are unchanged.
- **Demonstrated root cause:** both register-form paths selected ModR/M bits
  5:3 through `REG8_B53` or `REG16_B53`. Complete pre-fix replay and direct
  code inspection show that the executed SST population selects the
  destination through r/m bits 2:0. The 161 C6 and 154 C7 pre-fix
  register-form passes are exactly the records where those two fields happen
  to name the same register; they are not value-coincidence passes.
- **Correction:** select the byte or word register through `REG8_B20` or
  `REG16_B20`. Immediate fetch, instruction length, FLAGS, termination, and
  the memory paths are unchanged.
- **Verification:** focused tests cover all eight byte-register encodings, all
  eight word-register encodings, immediate edge values, paired-byte and
  unrelated-register preservation, and representative memory displacement,
  segment-override, 16-bit offset-wrap, and 20-bit physical-wrap cases.
  Complete G61 SST results are recorded in the milestone report.
- **Evidence:** [M61 task](../agents/tasks/M61_upd9002_mov_immediate_register.md)
  and [M61 report](../agents/reports/m61_upd9002_mov_imm_register.md).
- **Commit:** [90fa7dec](https://github.com/nakatamaho/vaeg/commit/90fa7dec5d46708a807851f61ae0792ee39e9b8f).

### uPD9002 AAM ignored its encoded radix

- **Status:** fixed in the M62 implementation; G62 candidate review pending.
- **Symptom:** 4,803 of the 5,000 applicable `D4 AAM` SST records failed.
- **Affected scope:** real-mode uPD9002 `AAM imm8` result and FLAGS
  materialization. `D5 AAD` is unchanged.
- **Demonstrated root cause:** the active handler skipped the encoded
  immediate and always divided AL by 10. It also derived SZP from AX rather
  than the final AL and retained unrelated FLAGS state.
- **Correction:** consume the immediate as the SST-observed radix, place the
  quotient and remainder in AH and AL, apply the observed immediate-zero
  normal-result rule, and materialize the exact result FLAGS.
- **Verification:** the complete D4 population is 5,000 pass / 0 fail,
  including radix values 0, 1, 2, 9, 10, 11, 16, and 255; D5 remains 5,000
  pass / 0 fail.
- **Evidence:** [M62 task](../agents/tasks/M62_upd9002_semantics_bundle.md)
  and [M62 report](../agents/reports/m62_upd9002_semantics_bundle.md).
- **Commit:** [c55e5730](https://github.com/nakatamaho/vaeg/commit/c55e57305052b2670f0edf4f1e9bda6041cb0c80).

### uPD9002 ROR4 retained the old AL high nibble

- **Status:** fixed in the M62 implementation; G62 candidate review pending.
- **Symptom:** 4,692 of the 5,000 applicable `0F 2A ROR4` SST records failed.
- **Affected scope:** V30 packed-BCD `ROR4` register and memory operands only.
- **Demonstrated root cause:** the handler merged the source low nibble into
  the old AL high nibble instead of transferring the complete source byte to
  AL. The destination calculation was otherwise structurally correct.
- **Correction:** write the rotated destination first, preserving the AL
  alias case, then transfer the original complete source byte to AL.
- **Verification:** all 5,000 register and memory ROR4 records pass, including
  displacement, prefix, 16-bit offset-wrap, and physical-wrap partitions.
- **Evidence:** [M62 task](../agents/tasks/M62_upd9002_semantics_bundle.md)
  and [M62 report](../agents/reports/m62_upd9002_semantics_bundle.md).
- **Commit:** [e74d814f](https://github.com/nakatamaho/vaeg/commit/e74d814f4397a5d832e7fbef675a93df4160bb2f).

### uPD9002 decimal and ASCII adjust used inherited 286 behavior

- **Status:** fixed in the M62 implementation; G62 candidate review pending.
- **Symptom:** the complete G61 populations contained 34 DAA, 64 DAS, 124
  AAA, and 4,716 AAS architectural failures.
- **Affected scope:** `27 DAA`, `2F DAS`, `37 AAA`, and `3F AAS` only.
  `D4 AAM` and `D5 AAD` are independently governed.
- **Demonstrated root cause:** the inherited handlers did not implement the
  SST-observed V30 high-adjust branch and FLAGS rules. AAA/AAS additionally
  adjusted AX as a word, allowing a carry or borrow across AL/AH where the
  observed behavior adjusts the two bytes independently.
- **Correction:** add V30-specific low/high adjustment decisions, result
  materialization, AF/CF/SZP/OF rules, and byte-local AAA/AAS AH changes.
- **Verification:** each of DAA, DAS, AAA, and AAS is 5,000 pass / 0 fail;
  their exact pre-fix union contains 4,938 hashes.
- **Evidence:** [M62 task](../agents/tasks/M62_upd9002_semantics_bundle.md)
  and [M62 report](../agents/reports/m62_upd9002_semantics_bundle.md).
- **Commits:** [33bec007](https://github.com/nakatamaho/vaeg/commit/33bec0078328fdaf6612188b6341c6e938f6dcb6)
  and [bfd9710b](https://github.com/nakatamaho/vaeg/commit/bfd9710bdac52ec5092871a2f5595a34212df1f2).

### uPD9002 shifts used the inherited normalized-count paths

- **Status:** fixed in the M62 implementation; G62 candidate review pending.
- **Symptom:** 19,139 applicable shift records failed across the C0, C1, D2,
  and D3 `/4` through `/7` subforms.
- **Affected scope:** 8-bit and 16-bit SHL/SAL/SHR/SAR register and memory
  forms. Rotate subforms `/0` through `/3` are unchanged.
- **Demonstrated root cause:** the inherited group paths normalized counts
  and materialized destination and FLAGS with rules that differ from the
  complete V30 SST population, especially at zero, width, and beyond-width
  counts. Subform `/6` also required the evidence-proven SHL behavior.
- **Correction:** route only shift subforms through width-specific raw-count
  helpers with exact destination, CF, OF, AF, SZP, and count-zero
  preservation rules; retain the existing rotate paths.
- **Verification:** all 40,000 shift hashes pass, and all 40,000 protected
  rotate hashes remain architecturally green.
- **Evidence:** [M62 task](../agents/tasks/M62_upd9002_semantics_bundle.md)
  and [M62 report](../agents/reports/m62_upd9002_semantics_bundle.md).
- **Commit:** [2cdaed95](https://github.com/nakatamaho/vaeg/commit/2cdaed95072d74bbf7187ae854fb31d3886c995d).

### uPD9002 DIV and IDIV used inherited result and exception rules

- **Status:** fixed in the M64 implementation; G64 candidate review pending.
- **Symptom:** 12,486 applicable records failed across `F6 /6`, `F6 /7`,
  `F7 /6`, and `F7 /7`.
- **Affected scope:** byte and word unsigned and signed division arithmetic,
  result placement, divide-error decisions, and pre-event FLAGS only.
- **Demonstrated root cause:** the inherited paths used result, overflow, and
  FLAGS rules that differ from the complete V20 SST contract. The word signed
  path also exposed the host-language minimum-signed-value divided by `-1`
  hazard.
- **Correction:** use widened arithmetic with explicit zero and overflow
  checks, materialize the observed quotient/remainder and pre-event FLAGS,
  and retain the G60d-approved type-0 entry machinery unchanged.
- **Verification:** all four 5,000-case populations pass; the exact 214-case
  saved-FLAGS dependency set remains green.
- **Evidence:** [M64 task](../agents/tasks/M64_upd9002_div_idiv.md) and
  [M64 report](../agents/reports/m64_upd9002_div_idiv.md).
- **Commit:** [63f12b4e](https://github.com/nakatamaho/vaeg/commit/63f12b4e2bc38999efec66a43042673111e242fe).

### uPD9002 packed-BCD string operations used incomplete inherited behavior

- **Status:** fixed in the M64 implementation; G64 candidate review pending.
- **Symptom:** 91 `ADD4S` and 304 `SUB4S` architectural records failed, while
  `CMP4S` was monitor-authorized but remained an implementation gap.
- **Affected scope:** `0F20 ADD4S`, `0F22 SUB4S`, and `0F26 CMP4S`.
- **Demonstrated root cause:** the inherited ADD4S/SUB4S paths used
  incompatible decimal-adjust and address-wrap behavior, and no CMP4S
  handler existed.
- **Correction:** apply the independently observed packed-decimal carry,
  borrow, comparison, register-update, and logical/physical wrapping rules;
  implement CMP4S without a destination write.
- **Verification:** all three 1,000-case full populations pass; ROL4 and ROR4
  remain 5,000 pass / 0 fail.
- **Evidence:** [M64 task](../agents/tasks/M64_upd9002_div_idiv.md) and
  [M64 report](../agents/reports/m64_upd9002_div_idiv.md).
- **Commit:** [60385167](https://github.com/nakatamaho/vaeg/commit/60385167cede30a3c06e97373a92646e19021523).

### uPD9002 monitor-authorized bit-operation forms were incomplete

- **Status:** fixed in the M64 implementation; G64 candidate review pending.
- **Symptom:** six exact monitor-authorized forms remained classified as
  implementation gaps even though their byte/word and CL/immediate sibling
  forms were present.
- **Affected scope:** the expanded `TEST1`, `CLR1`, `SET1`, and `NOT1`
  opcodes `0F10` through `0F1F`.
- **Demonstrated root cause:** dispatch and handlers were missing for
  `0F13`, `0F15`, `0F16`, `0F17`, `0F1E`, and `0F1F`.
- **Correction:** add only the evidence-derived word/CL and NOT1 forms,
  including exact bit-index, register/memory, FLAGS, and instruction-length
  behavior, then activate their complete pre-approved structural sets.
- **Verification:** every one of the sixteen expanded 5,000-case populations
  passes. `0FFF BRKEM` is not counted: v20 has metadata but no SST shard, so
  selected and executed coverage is exactly zero.
- **Evidence:** [M64 task](../agents/tasks/M64_upd9002_div_idiv.md) and
  [M64 report](../agents/reports/m64_upd9002_div_idiv.md).
- **Commit:** [99c6388d](https://github.com/nakatamaho/vaeg/commit/99c6388df903dfc69432730cc9fa908a83946774).


### SCSI DATA IN ended without reporting an early phase change

- **Status:** corrected in M75d1; terminal G75 acceptance remains pending.
- **Symptom:** when a host allocation exceeded the target response, the AR19
  pump continued until the host count reached zero and never exposed the
  target's STATUS phase with the residual transfer count.
- **Demonstrated root cause:** DATA IN completion was tested only against the
  host-programmed remaining count; `cmdpos` exhaustion was not converted into
  the WD33C93 unexpected-information-phase status.
- **Correction:** [ca29efb](https://github.com/nakatamaho/vaeg/commit/ca29efb)
  preserves residual TC and emits `0x48 | (phase & 7)` (4Bh for STATUS) when
  response data ends first.  When TC ends first, the unrequested suffix is
  discarded and normal completion advances to STATUS.
- **Verification:** focused M75 QA, Linux SDL2 build, and
  `vaeg_m75_scsi_controller` CTest pass.  Normal-speed guest evidence for
  the short INQUIRY path is still outstanding.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).

### Transfer Info completion encoded the wrong phase and consumed post-count requests

- **Status:** corrected in M75d1; G75 manual acceptance remains pending.
- **Symptom:** TUR completion was logged as `1Ah` while the next request was STATUS (`8Bh`), and the following STATUS/MESSAGE-IN request could be consumed or regenerated instead of remaining available for the next Transfer Info.  Message-In completion and Negate ACK also produced the wrong lifecycle.
- **Demonstrated root cause:** successful completion used `0x10 | MCI` instead of the WD33C93A `1MCI` encoding; REQ and ACK were coupled; the post-count REQ was not an independent retained bus state; Message-In was treated like a normal post-count phase; and command admission rejected Level-I Negate ACK solely because a Level-II state was active.
- **Correction:** derive completion from the next service status (`0x18 | (service_status & 7)`), retain the post-count REQ with its sequence/phase/direction across CSR read, split target REQ and initiator ACK transitions, return `20h` after the Message-In byte while holding ACK, and make Negate ACK clear ACK without directly generating bus-free status.  Transfer-count writes are traced and SBT semantics are explicit.
- **Verification:** compiled controller-path selftest (21 named cases), focused CTest, controller QA, Linux SDL2 selftest/build, MinGW cross-build, and semantic-limit real-ROM trace.  The real trace now shows `1Bh` for TUR STATUS, retained request IDs across `1Fh`, `20h`, and later `85h`, INQUIRY DATA IN with 32 reads and `4Bh` residual completion, and no unexplained `010000h`.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [4e17c6f](https://github.com/nakatamaho/vaeg/commit/4e17c6f3fee67642ca69329147808cd18c71c9a7).

## Open Defects

### M105 — Port 40h sound (CMD SING, game sound) sounds wrong

- **Symptom/scope:** the sound made through port 40h sounds odd, as if its
  timing were off (maintainer report during G105 checks, 2026-10-07). A
  plain `BEEP` (bit 5, the 2400 Hz buzzer) seems fine. `CMD SING` (bit 7,
  the port sound) is less clear than the maintainer remembers, and ordinary
  V1/V2 game sound is clearly wrong. `CMD SING` changes pitch with the CPU
  speed slider, which is expected for software-timed toggling of bit 7. The
  M104 build (main at `2508ccf4`) sounds the same, so it predates M105.
  It happens with the speed and the CPU speed both at 100 % (maintainer,
  2026-10-07), so it is not only the speed-slider effect below.
  Known separately: with the speed slider away from 100 % the BEEP is
  garbled because it is built from CPU-clock on/off events while the host
  consumes samples in real time (`sdl2/README.md`).
- **Status:** not investigated; no root cause is claimed. Moved to a later
  milestone by the maintainer.

### M104 — V3 programs run about 2.2 times faster than on the PC-88VA2

- **Symptom/scope:** the M104 test program G160 redraws its picture 3.0,
  3.4 and 4.25 times per second at 16, 8 and 4 bits in vaeg against 1.4,
  1.6 and 1.9 on a PC-88VA2, at 15.98 and 24.8 kHz alike
  (`pc88va-video-modes.md` section 12.4). V3 mode; other code not measured.
- **Status:** not investigated; no root cause is claimed. The CPU clock
  (7.9872 MHz) matches the board; the ratio being the same at every depth,
  with very different GVRAM write counts, points away from GVRAM waits
  alone. Next step: a hardware benchmark per instruction class (register,
  MUL/DIV, main RAM, TVRAM, GVRAM) compared with vaeg, in a later milestone.

### M103g — V1/V2 voice pitch observation without a demonstrated cause

- **Symptom/scope:** in V1/V2 mode, a PC-8801 game's spoken title voice
  plays slightly low in pitch (maintainer report, G103g check, Windows
  build `1ae6cccf`). A reported missing floppy access sound was the
  drive-sound setting being off, not a defect.
- **Status:** not investigated; no root cause is claimed. Tracked in
  [`v1v2-mode-decisions.md`](v1v2-mode-decisions.md) §9.

### M100g — Configuration save failures were silently ignored

- **Status:** implementation candidate; runtime failure-injection verification pending.
- **Scope:** SDL2 configuration persistence, including the main-RAM menu.
- **Source finding:** ini_write ignored write results and file_close always
  returned success, preventing the frontend from reporting failed persistence.
- **Correction:** check writes and close, replace configuration atomically,
  and preserve the previous menu settings on failure. Do not report a saved
  setting when --no-cfg disables persistence.
- **Verification:** Linux compilation completed; runtime tests NOT RUN.
- **Task/evidence:** [M100g report](../agents/reports/m100g_main_ram_sync.md).
- **Commit:** [fccc71f6](https://github.com/nakatamaho/vaeg/commit/fccc71f63310755b89f367cd4119cc8b1d8aba22).


### Windows native-to-SDL switch stalls the frontend

- **Status:** M99z8 candidate; affected-machine verification pending.
- **Symptom:** audio continues but menus stop responding after librashader
  then SDL selection. Only exiting/relaunching VAEG restores operation.
- **Scope/findings:** native DXGI and the new SDL renderer inherited the same
  HWND; GUI teardown did not explicitly release SDL mouse capture. These
  lifecycle findings do not yet identify the blocked Windows call.
- **Candidate correction:** recreate the host window on return to SDL,
  release GUI mouse capture and ignore retired-window focus/size events.
  Preserve guest state and framebuffer; do not reset CPU or host pacing.
- **Evidence:** [M99z8 report](../agents/reports/m99z8_windows_renderer_switch.md)
  records the ten-cycle SDL window/render/readback test and platform limits.
- **Commit:** [1773322b](https://github.com/nakatamaho/vaeg/commit/1773322beb0093855be9c2bf652ded9283f7cf4a).

### Windows librashader unavailable and black fallback after M99z6

- **Status:** open for maintainer-machine confirmation; M99z7 candidate prepared.
- **Symptom/scope:** the maintainer still reports `CRT unavailable` and black
  guest output with the Windows native renderer, while SDL displays normally.
- **Demonstrated findings:** the packaged official DLL imports the external
  D3DX9 and MSVC runtimes; the previous package audit did not check dependency
  availability on the target. Their absence on the maintainer's machine has
  not yet been established. The pass-through strip is counterclockwise in
  render-target coordinates, but default D3D11 state culls that winding.
- **Correction candidate:** explicitly disable pass-through culling; preserve
  loader, preset and filter errors in UI/logs; check required runtime symbols
  and API/ABI; provide real SDL/librashader selection instead of an enable toggle.
  Remove the unwanted UI-size menu while retaining automatic DPI scaling.
- **Verification:** see [M99z7 report](../agents/reports/m99z7_windows_crt_fallback.md)
  for local tests and the real-rasterizer WARP regression test. WARP is not
  physical GPU or CRT filter-chain acceptance. No guest/core changes.
- **Commit:** [ee58bda0](https://github.com/nakatamaho/vaeg/commit/ee58bda0f3652cc9936f791806d6f2f65e94b2a1).

### Legacy Z80 reset leaves saved undocumented flag bits uninitialized

- **Status:** open; demonstrated during M34 contract capture, with no behavior
  change authorized in that milestone.
- **Symptom:** a revision-1 Z80 state saved immediately after reset can depend
  on an indeterminate `xf` byte, and architectural F bits 3 and 5 can inherit
  that value when flags are materialized.
- **Demonstrated root cause:** `Z80C::Reset()` zeroes the register structure and
  lazy-flag mask but does not initialize member `xf`; `GetAF()` merges `xf`
  into F, and `SaveStatus()` serializes it.
- **Current containment:** the M34 ROM-less legacy fixtures execute `XOR A`
  before capture so their bytes are deterministic. This avoids the defect in
  evidence generation but does not correct production reset behavior.
- **Next step:** decide in a separately authorized correctness milestone
  whether to initialize `xf` in the legacy path or correct it only at the M41
  replacement cutover, then add a reset/save regression test and human gate.
- **Evidence:** [M34 legacy Z80 contract](z80-compatibility-legacy-contract.md#verified-legacy-execution-behavior)
  and [ADR-0011](../agents/DECISIONS/ADR-0011-z80-migration.md#consequences-and-unresolved-risks).

### VA1 N88 BASIC V3.0 commands can enter an apparent hang

- **Status:** open.
- **Symptom:** in the inherited VA1 execution path, commands including
  `FILES`, `LIST`, and `BEEP` can leave the guest executing a repeated path;
  sound-enabled `BEEP` may also expose text-screen corruption.
- **Known exclusions:** it reproduced in original VAEG; Sound Off, suppressing
  BEEP PCM registration, suppressing the BEEP event, correct ROM checksums,
  `clk_mult=2`, M29, and M30 did not establish a fix for this command path.
- **Current evidence:** the CPU remains active and repeated FDC Sense Interrupt
  Status polling has been observed, but the exact guest wait condition is not
  yet demonstrated.
- **Next step:** use an independently authorized, bounded guest-behavior
  investigation to capture the decisive CPU/register/I/O state and correct
  the defect only if its root cause is proven.
- **Evidence:** [M21 diagnostic record](../agents/tasks/M21_sdl2_display_effects.md),
  [build and runtime notes](BUILD.md), and
  [M30 BMS investigation result](../agents/tasks/M30_va_bms_window.md).

### 2D floppy track mapping is corrected; guest acceptance is pending

- **Status:** corrected and host-qualified in M16; FreeDOS guest acceptance is
  pending.
- **Symptom:** native 2D D88 tracks were rejected or mapped to the wrong
  recorded cylinder by the VA FDD path.
- **Demonstrated root cause:** D88 access always applied the 96-TPI double-step
  translation, even when the configured 1D/2D subsystem mode selects 48 TPI.
  The subsystem mode path also left track density at its reset default.
- **Correction:** select track density from the configured subsystem mode and
  map 48-TPI 2D cylinders directly. Keep double stepping for 96-TPI access and
  reject an odd physical half-track. FDC byte pacing now uses the command's
  sector size and sectors-per-track values.
- **Verification:** the ROM-free production-path selftest reads and writes
  synthetic 2D 320/360, 2DD 640/720, 2HC 1.2 MB, and native 2HD regression
  profiles through D88/FDC paths, checks boundary sectors, protection,
  persistence, and concurrent drives. Linux CI build and selftest passed;
  CTest reported 98 passed, one external-input skip, zero failures.
- **Evidence:** [M16 D88/FDC selftests](https://github.com/nakatamaho/vaeg/blob/ce9e7b25a28fbac83f517ef6c0f20f593eed6dc1/sdl2/selftest.c)
  and [fixing commit](https://github.com/nakatamaho/vaeg/commit/ce9e7b25a28fbac83f517ef6c0f20f593eed6dc1).
  The earlier investigation remains at the
  [M23 formatted FDD task](../agents/tasks/M23_formatted_fdd_images.md).


### PCPLUS bus-free completion was left pending after MESSAGE IN

- **Status:** corrected in M75d1; full G75 acceptance remains pending.
- **Symptom:** after PCPLUS consumed `CSR=1Fh` and the MESSAGE IN byte, the
  controller did not deliver the configured ending-disconnect `CSR=85h`.
  The guest could continue without a visible bus-free completion, and the
  terminal controller sequence was incomplete.
- **Demonstrated root cause:** the target-phase helper assumed every phase
  transition would be followed by a host `TRANSFER INFO`.  Bus free has no
  such request, so the pending `85h` status remained not-ready and was never
  scheduled after the `1Fh` CSR was consumed.
- **Correction:** mark only `85h`/`80h` bus-free results target-ready at the
  MESSAGE IN boundary.  The existing depth-one CSR latch, AR17 consumption,
  DBR gating, and processing event for data-bearing transitions are unchanged.
- **Verification:** the normal-speed full trace now shows MESSAGE `00h`,
  `CSR=1Fh`, then IRQ6 `CSR=85h`, `AR17=85h`, and `AR16=00h`.  The focused
  M75 validator and Linux SDL2 build pass.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [bc29d9e7](https://github.com/nakatamaho/vaeg/commit/bc29d9e7cecc426c4da22cbc628ab95f8c7efe8f).

### INQUIRY response payload has not been guest-validated

- **Status:** open; no payload-field change is authorized from static
  inspection alone.
- **Symptom:** the current direct-access response table is 32 bytes while the
  observed INQUIRY allocation is 36 bytes.  This requires the corrected short
  transfer contract, not forced padding.
- **Demonstrated state:** byte4 is now `1Bh`, equal to the 32-byte table length
  minus five, enforced by [4eeacda](https://github.com/nakatamaho/vaeg/commit/4eeacda).
- **Next step:** observe the normal-speed guest branch after the short `4Bh`
  response, then decide whether ANSI level, vendor/product text, or response
  length needs a separately evidenced change.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).

### CPU-multiplier SCSI timing mismatch

- **Status:** reopened in M75d1; the prior `f406b86` closure is superseded.
- **Symptom:** the normal-speed run (the intended CPU/device ratio) reaches
  TUR completion but does not reach the later MODE SENSE DATA IN within the
  180-second bounded run, while the diagnostic `--cpumult 8` run reaches
  INQUIRY, READ CAPACITY, and MODE SENSE but exhibits partial DATA IN.
- **Demonstrated state:** the source audit proves that phase readiness and
  interrupt delivery use emulated-clock events and that AR19 byte access is
  synchronous.  It does not prove that the chosen target-processing quantum
  is long enough for the normal-speed guest to return to its intended wait
  consumer.  Progress under an artificial multiplier therefore cannot be
  classified as expected behavior.
- **Correction:** none yet.  Compare the normal-speed and `--cpumult 8`
  consumer paths for the second SELECT/COMMAND event (`1CCDh` wait path
  versus `1742h`/`1747h` main-pump path), then derive any processing delay from
  the emulated device contract rather than tuning to PCPLUS.
- **Verification:** controller QA, builds, CTest, and selftest pass, but the
  timing defect and normal-speed INQUIRY/MODE SENSE evidence remain open.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md),
  [0c80c447](https://github.com/nakatamaho/vaeg/commit/0c80c447b6b655b81b3d08e5b67c8a1457d5be91),
  and [df2981e](https://github.com/nakatamaho/vaeg/commit/df2981e).


### MODE SENSE page-04 geometry was omitted from the SCSI target

- **Status:** corrected in M75d1; normal-speed SCHD registration remains open.
- **Symptom:** PCPLUS/SCHD issued `1A 00 04 00 24 00` but the target ignored
  the page code and returned only a 12-byte mode header and block descriptor,
  omitting the rigid-disk cylinder/head geometry.
- **Demonstrated root cause:** `scsicmd_datain()` treated every MODE SENSE(6)
  request as the descriptor-only case and capped the response at 12 bytes.
- **Correction:** decode page `04h` and `3Fh`, emit the allocation-bounded
  page-04 geometry with mounted `SXSIDEV` values, validate the geometry
  product, and return CHECK CONDITION/ILLEGAL REQUEST for unsupported pages or
  inconsistent geometry.  REQUEST SENSE exposes and then clears the sense
  data.
- **Verification:** M75 controller QA, Linux SDL2 build, focused CTest, and
  SDL selftest pass; the accelerated real-ROM trace reaches the MODE SENSE
  DATA IN phase.  Normal-speed SCHD registration is not yet demonstrated.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [03d4cd765](https://github.com/nakatamaho/vaeg/commit/03d4cd76541a3058cf32b0c239b499e0c0431627).

### WD33C93 transfer count byte-order hypothesis (superseded)

- **Status:** superseded in M75d1; no low/middle/high production correction is retained.
- **Observed symptom:** the intermediate MinGW run with the low/middle/high experiment produced `TC=060000` for a six-byte CDB, `TC=010000` for a one-byte transfer, and `TC=0a0000` for a ten-byte CDB.
- **Demonstrated correction:** the WSLg `scsitrace` proves PCPLUS/WD33C93 uses AR12/AR13/AR14 as high, middle, low: the expected counts are 6, 1, and 10, while the experiment multiplied them by 65536. The original high/middle/low decode and decrement order is restored.
- **Follow-up commit:** [c959453](https://github.com/nakatamaho/vaeg/commit/c959453a0a482994ac25ab6db0b33e425306a0e9).
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and [M75 task](../agents/tasks/M75_scsi_support.md).
### MODE SENSE block length was written at the reserved-byte offset

- **Status:** corrected in M75d1; SCHD/SCFORM manual confirmation remains pending.
- **Symptom:** after INQUIRY completed and SCHD reported direct-access fixed-media mode, the driver halted during MODE SENSE geometry processing.
- **Demonstrated root cause:** the six-byte MODE SENSE block descriptor places the three-byte block length at response bytes 9--11.  `scsicmd_datain()` wrote it at byte 8, overwriting the reserved byte and shifting the value read by SCHD.
- **Correction:** write the mounted `SXSIDEV` block size at `scsiio.data + 9`; no device-specific or guest-address workaround was added.
- **Verification:** M75 QA, Linux SDL2 build, focused CTest, and SDL selftest pass.  The corrected MinGW/manual SCFORM run is still required to confirm SCHD registration and format completion.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and [M75 task](../agents/tasks/M75_scsi_support.md).


### CSR pending queue admitted target events ahead of the host consumer

- **Status:** corrected in M75d1; later normal-speed guest progress remains open.
- **Symptom:** a target transfer-completion CSR could be requested while the
  previous SELECT/command CSR was still active, filling the runtime pending
  slot and later causing overrun/drop and wrong event ordering.
- **Demonstrated root cause:** the old implementation let target state advance
  on scheduled time while the WD33C93 CSR latch was unread, so the runtime
  state contained an active event, a visible latch, and a successor pending
  event.  The pre-correction WSLg trace records the active `CSR=11h`, a
  concurrent `CSR=1Ah`, `pending=1`, and subsequent `csr-overrun` records.
- **Correction:** remove the CSR pending slot.  Target-origin events are
  pulled from persistent target state only after AR17 consumes the visible
  CSR; host-synchronous transfer-completion events remain serialized by the
  host I/O boundary.  Overlap is fail-closed and trace-visible.  Add a
  deterministic watchdog for unread CSR, stalled target delay, and missing
  DATA IN request handoff.
- **Verification:** fixed 4000/40000-clock stress and five seeded jitter runs
  produced zero `csr-overrun`, `csr-drop`, and `invariant` records.  QA,
  focused CTest, and SDL selftest pass.  A normal-speed bounded run still
  reports an unread later `CSR=8Ah`, so SCHD registration and G75 acceptance
  remain open.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [ccb0666](https://github.com/nakatamaho/vaeg/commit/ccb066695907456314783cc3bb9a28dfad279c55).

### DATA IN TC=1 was incorrectly treated as a phase end

- **Status:** corrected in M75d1; fresh SCFORM/SCHD acceptance remains pending.
- **Symptom:** the WSLg SCFORM/SCHD trace completed TUR and selected ID0, but an INQUIRY DATA IN request programmed `TC=1` after an initial `TC=36`.  The emulator moved to STATUS after the first byte, so SCHD saw an incomplete INQUIRY response and reported that no device was connected.
- **Demonstrated root cause:** `scsiio_data_read()` changed `SCSIPH_DATAIN` to STATUS whenever the host count reached zero, even while `rddatpos < cmdpos`.  PCPLUS uses repeated one-byte TRANSFER INFO requests within the same DATA IN phase.
- **Correction:** keep DATA IN active and call the phase-aware transfer path for the next byte; transition to STATUS only when the target response cursor is exhausted.  No guest address, CDB-order, image-name, or fixed-length workaround was added.
- **Verification:** M75 controller QA, focused CTest, SDL selftest, and MinGW cross-build pass.  The corrected binary requires a new manual SCFORM/SCHD run.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md), [M75 task](../agents/tasks/M75_scsi_support.md), and [84bc2ef](https://github.com/nakatamaho/vaeg/commit/84bc2efe1de9e5661fd28d31ba087a304f1a82ac).

### Transfer Info was not represented as an active Level-II lifecycle

- **Status:** corrected in M75d1; PCPLUS/SCHD integration remains open.
- **Symptom:** after `AR18=20h`, the emulator could treat a REQ-waiting
  Transfer Info as an abandoned legacy phase transfer, causing the guest to
  fall back to single-byte recovery and fail to complete device discovery.
- **Demonstrated root cause:** the Transfer Info path had no single explicit
  lifecycle distinguishing command acceptance, REQ wait, byte pending, and
  post-count REQ.  Service Required could therefore be considered while the
  command was still active, and completion could be emitted without a
  distinct post-count REQ.
- **Correction:** add the explicit lifecycle in `cbus/scsiio.c`; reject
  commands while INT/CSR is pending, preserve active state while waiting for
  REQ, enforce REQ/DBR/ACK byte accounting, emit `4MCI` for early phase
  changes, and emit successful MCI only after the post-count REQ.  An already
  asserted REQ starts transfer immediately.  No guest-address or payload
  workaround was added.
- **Verification:** M75 Transfer Info selftest (10 tests), controller QA,
  focused CTest, Linux SDL2 build, and SDL selftest pass.  The bounded real-ROM
  run still times out before INQUIRY DATA IN, so manual SCHD/SCFORM acceptance
  remains open.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md),
  [M75 task](../agents/tasks/M75_scsi_support.md),
  [f0b14d7](https://github.com/nakatamaho/vaeg/commit/f0b14d71a2015b9469c92ea51abe2b9ebf964b43),
  [9827d09](https://github.com/nakatamaho/vaeg/commit/9827d09756779943d46b0973436f26f32142dced).


### SCSI target LUN isolation and malformed INQUIRY response

- **Status:** corrected in M75d1; SCHD/SCFORM registration remains open.
- **Symptom:** one configured SCSI image could be interpreted through multiple
  logical-unit discovery paths, and the 32-byte INQUIRY response left SCHD
  reading stale bytes for Product Revision.
- **Demonstrated root cause:** the backend did not centrally require both the
  WD Target LUN register and CDB LUN to be zero, and `hdd_inquiry` advertised
  `1Bh` additional length in a 32-byte table with revision bytes in the wrong
  fixed-width offsets.
- **Correction:** centralize LUN0 validation for PIO and Select-and-Transfer;
  return GOOD/36-byte byte-`7Fh` INQUIRY for unsupported LUNs and
  CHECK CONDITION `05/25/00` for other unsupported-LUN commands; replace the
  normal response with the exact 36-byte `NEC`/`NP2-HDD`/`1.00` layout.
- **Verification:** compiled production selftests, M75 QA, focused CTest,
  Linux SDL2, MinGW cross-build, and bounded target/LUN trace pass.  SCHD
  still issues unsupported READ(10), so SCFORM and manual registration are
  not claimed.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [103d59e](https://github.com/nakatamaho/vaeg/commit/103d59e).


### SCSI block commands were missing from the target backend

- **Status:** corrected in M75d2; guest enumeration and persistent filesystem gates remain open.
- **Symptom:** SCHD reached READ(10) during discovery, received CHECK CONDITION `05/20/00`, and later reported that the registered C: drive had no sectors.  The earlier multi-device report is not classified because it lacked a target/LUN/registration matrix.
- **Demonstrated root cause:** `cbus/scsicmd.c` had no common READ/WRITE(6/10) command path, so no SXSIDEV media operation occurred for block access.
- **Correction:** add shared 6-/10-byte LBA/count decoding, overflow-safe range validation, SXSIDEV-backed chunked PIO DATA IN/OUT, complete-write commit accounting, and fixed sense mappings for range, write-protect, and backend errors.  The implementation reuses `sxsi_read()`/`sxsi_write()` and does not special-case SCHD, SCFORM, target IDs, or guest addresses.
- **Verification:** compiled production selftests cover zero-count semantics, 6-/10-byte decoding, range and sense handling, read/write persistence, incomplete writes, chunk boundaries, and LUN isolation.  Normal-speed real-ROM trace now shows READ(10) LBA 0 with 256 DATA IN bytes, one backend block, zero residual, and GOOD status, followed by STATUS/MESSAGE IN.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md), [M75 task](../agents/tasks/M75_scsi_support.md), and [a4d21e9](https://github.com/nakatamaho/vaeg/commit/a4d21e9a5e0a3b31818cc1dfcd8b281b3b62a67d).
- **Commit:** [a4d21e9](https://github.com/nakatamaho/vaeg/commit/a4d21e9a5e0a3b31818cc1dfcd8b281b3b62a67d).


### Legacy SCSI DATA OUT bypassed the backend commit and erased CHECK CONDITION

- **Status:** corrected in M75; fresh SCFORM/FAT persistence remains open.
- **Symptom:** a legacy SCSI DATA OUT request could fill the staging buffer,
  switch directly to STATUS, and report completion without proving that the
  mounted image had been updated.  A backend write failure could also be
  replaced by GOOD during STATUS transfer.
- **Demonstrated root cause:** `cbus/scsiio.c`'s 0CC6h handler wrote the
  buffer directly and emitted `8Bh` without calling the common
  `scsicmd_transinfo()`/`scsicmd_block_dataout_complete()` lifecycle.  The
  STATUS branch in `cbus/scsicmd.c` unconditionally assigned `00h`.
- **Correction:** AR19 and 0CC6h now share DATA OUT payload accounting;
  complete chunks call the common command layer, which alone invokes
  `sxsi_write()`, advances chunks, and selects status.  STATUS preserves the
  command-layer result.  No SCFORM, FAT, target-ID, or guest-address special
  case was added.
- **Verification:** SDL production selftest covers successful 0CC6h commit,
  no direct STATUS completion, failed backend write, and CHECK CONDITION
  persistence; Linux build, SDL selftest, focused CTest, and M75 QA pass.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [d284468](https://github.com/nakatamaho/vaeg/commit/d284468fd256598489e07307fda58fbd1a0aa302).


### Added VHD/FAT16 forensic inspection for the G75 free-space failure

- **Status:** diagnostic tooling added; guest FAT acceptance remains open.
- **Symptom:** freshly formatted guest volumes report no free disk space, but
  the first incorrect on-disk byte was not available in the current sandbox.
- **Correction:** add `tools/inspect_vaeg_fat.py`, which matches the VAEG
  VHD header, respects physical/header offsets, assembles four-block logical
  sectors, validates FAT16 BPB geometry, compares FAT copies, counts free
  clusters, inspects root-directory entries, and reports changed physical-LBA
  ranges.  Generated fixtures reject duplicated/reordered four-block metadata
  and zero-free or mismatched FAT tables.
- **Verification:** the initial seven generated unittest cases and focused CTest
  pass.  At that time the original user images were inaccessible; the later
  read-only copies and their truncation evidence are recorded in the
  superseding forensic entry below.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).


### Supplied SCFORM images were truncated before FAT validation

- **Status:** evidence limitation recorded in M75; G75 remains open.
- **Symptom:** the supplied `scsi40_formatted.hdd` was expected to explain the
  guest's zero-free-space report, but no valid FAT16 BPB could be decoded.
- **Demonstrated finding:** both supplied VHD1.00 headers report a 40 MiB
  image with 163,840 256-byte blocks, while the actual files contain only 3
  and 651 complete data blocks (plus 220-byte tails).  The formatted artifact
  has early IPL/formatter data and partial writes, not a complete filesystem
  image.
- **Correction:** the inspector now reports file size, complete block count,
  truncation, and changed partial tails; it never fabricates missing blocks.
  Original files were opened read-only.
- **Verification:** nine generated inspector tests pass.  No FAT free-cluster
  count or post-fix SCFORM result is claimed from these truncated artifacts.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).

### SCSI VHD creator left declared capacity truncated

- **Status:** corrected in M75; guest SCFORM and filesystem acceptance remain open.
- **Symptom:** VHD1.00 headers declared 163840 256-byte blocks, but newly created backing files ended after the header/initial IPL and SXSIDEV could read past the actual EOF.
- **Demonstrated root cause:** the image creator wrote the header and boot bytes without setting the logical file length; the production `VHDHDR` is 220 bytes, so validation must use `sizeof(VHDHDR)` rather than an inferred 256-byte header.
- **Correction:** `newdisk_vhd_create()` performs checked geometry arithmetic, writes a complete 220-byte-header image through a temporary path, sets the exact sparse logical length, flushes and atomically renames it.  SCSI open/read/write paths now reject incomplete or overlong stores and propagate short I/O/flush failures.
- **Verification:** production selftests cover exact size, sparse zero reads, first/middle/last block boundaries, persistence after reopen, out-of-range rejection, truncated-image rejection, and no-overwrite behavior.  The generated 40 MiB image is recorded in the M75 report.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).


### FAT padding was counted as usable free space and 64KiB PIO positions wrapped

- **Status:** corrected in M75; guest CHKDSK and file lifecycle remain open.
- **Symptom:** FAT inspection reported free entries beyond the valid data-cluster
  range, while exact 64KiB READ(6) transfers could address a repeated data
  window at the 64KiB boundary.
- **Demonstrated root cause:** the inspector counted the physical FAT entry
  capacity rather than `FAT[2]..FAT[ClusterCount+1]`; AR19 and 0CC6h DATA IN
  indexing masked the 32-bit data position with 16-bit and 15-bit masks.
- **Correction:** derive valid cluster and padding ranges from the BPB, and use
  checked unmasked positions for both PIO data paths.  Transfer traces now
  compare backend, staging and delivered bytes, and record TC `010000h` as a
  valid 65,536-byte count.
- **Verification:** compiled SDL selftests cover 65,535/65,536/65,537-byte
  boundaries, READ(6) 256-block semantics and READ(10) exact 64KiB/chunked
  reads; ten FAT-inspector tests pass; the bounded formatted-image run shows
  equal backend/staging/AR19 digests for its first three block reads.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [a7d244d](https://github.com/nakatamaho/vaeg/commit/a7d244d61d93eedaf8498185ec55f8e8ac743926).


### Direct Select-and-Transfer WRITE committed before guest DATA OUT

- **Status:** fixed; G75 remains open for the independent SASI, HOSTFAT, and
  non-SCSI regression gates.
- **Symptom:** PC-Engine issued a successful WRITE(10), but the guest's
  subsequent AR19 DATA OUT bytes arrived after the controller had already
  committed the stale staging buffer. The first write could therefore update
  the image with the preceding command's data and reject the actual DATA OUT
  phase.
- **Root cause:** the direct Select-and-Transfer WRITE path called the
  backend write at command acceptance, before the guest drained the DATA OUT
  window. This was demonstrated by the first WRITE trace: 1024 bytes were
  reported as backend-written before the first guest DATA OUT byte, followed
  by phase-direction-mismatch warnings.
- **Correction:** direct WRITE commands now remain active in DATA OUT, accept
  bytes through AR19, and call the backend write only after the programmed
  byte count is complete. The trace digest equality predicate covers both READ
  and WRITE data paths.
- **Verification:** the isolated guest creation run completed a WRITE(10) at
  LBA 572 for four 256-byte blocks with TC `000400`, AR15h `00h`, AR19 DATA
  OUT, GOOD status, residual zero, commit_count one, and equal backend,
  staging, and delivered digests. The guest read/reopen/delete run printed
  `G75 READ-REOPEN-DELETE OK`; a second boot printed `G75 DELETE PERSISTED`.
  The focused Python tests, `M75_SCSI_CONTROLLER_OK`, Linux build, and
  `git diff --check` passed.
- **Evidence:** `docs/agents/reports/m75_scsi_support.md`, G75b screen and
  trace artifacts retained outside the repository.
- **Commit:** [13c978b](https://github.com/nakatamaho/vaeg/commit/13c978b)

### HOSTFAT configuration could make startup unrecoverable

- **Status:** fixed in M75; HOSTFAT remains read-only and its guest
  filesystem contract is unchanged.
- **Symptom:** changing `HOSTFATDIR` and closing vaeg before the asynchronous
  rebuild/reset completed could leave the configuration pointing at a new
  path while the old snapshot remained mounted. A later startup could then
  fail before the emulator window appeared; deleting the configuration was
  required to recover.
- **Root cause:** the GUI persisted `HOSTFAT`/`HOSTFATDIR` before the worker
  had built and mounted the replacement image, and startup treated a failed
  configured HOSTFAT rebuild as fatal.
- **Correction:** retain pending GUI values until successful mount, commit the
  path only at the mount event, and disable invalid configured HOSTFAT on
  startup while retaining the path so the emulator can boot and the setting
  can be corrected later.
- **Verification:** Linux SDL selftest, HOSTFAT manager failed-rebuild
  retention selftest, invalid configured-directory startup recovery probe,
  `M75_SCSI_CONTROLLER_OK`, all required local builds, and `git diff --check`
  passed.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).
- **Commit:** [bc51051](https://github.com/nakatamaho/vaeg/commit/bc510511326b9fdb3f61018d751dfc598159512a)


### HOSTFAT rejected a Windows Dropbox root path

- **Status:** fixed in M75; links that escape the selected root, special files,
  and containment checks remain rejected.
- **Symptom:** a Windows HOSTFAT directory selected under a Dropbox tree could
  be treated as unavailable when the selected root was exposed as a junction or
  directory reparse point. Paths copied with surrounding quotes were also
  passed to filesystem validation literally.
- **Root cause:** the GUI and snapshot builder did not normalize user-entered
  HOSTFAT paths, and the builder rejected a Windows root reparse point before
  canonicalizing it.
- **Correction:** normalize whitespace/quotes, use `USERPROFILE` for the
  Windows browser start directory, canonicalize the selected Windows root and
  contained links/reparse points, and reject links that escape the snapshot
  tree.
- **Verification:** HOSTFAT snapshot selftest now covers a quoted path and,
  on Windows when permitted, both a root reparse point and a contained
  reparse-point directory. Linux debug and MinGW cross builds pass.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).
- **Commit:** [1ec024b](https://github.com/nakatamaho/vaeg/commit/1ec024b)
- **Follow-up:** [7e6ede7](https://github.com/nakatamaho/vaeg/commit/7e6ede7)


### HOSTFAT GUI hid rebuild failures and overstated capacity

- **Status:** fixed in M75.
- **Symptom:** Configure displayed `127.44 MiB usable` although the current
  PC-Engine FAT12 geometry provides about 63.72 MiB, and asynchronous rebuild
  errors were not visually distinguished from normal status text.
- **Root cause:** the GUI retained a stale capacity label and rendered the
  manager error message with the normal text style.
- **Correction:** display the actual 63.72 MiB limit, render
  `HOSTFAT_MANAGER_ERROR` messages in red directly below the rebuild button
  with their detailed reason, and reopen Configure automatically after an
  asynchronous failure.
- **Verification:** Linux selftest, Linux/macOS/MinGW builds, M75 QA, and
  repository encoding/EOL/case checks passed.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).
- **Commit:** [55800c6](https://github.com/nakatamaho/vaeg/commit/55800c6)
- **Follow-up:** [2515598](https://github.com/nakatamaho/vaeg/commit/2515598)
- **Follow-up:** [eb65a14](https://github.com/nakatamaho/vaeg/commit/eb65a14)


### HOSTFAT rebuild failure was not explicit after OK

- **Status:** fixed in M75.
- **Symptom:** after pressing `Rebuild + reset on OK` and then `OK`, a
  failed asynchronous rebuild could leave the user without an immediately
  obvious error notification.
- **Root cause:** the detailed error was retained in the Configure dialog,
  but there was no dedicated failure notification at the moment the worker
  reported failure.
- **Correction:** show a red `HOSTFAT error` modal for both asynchronous
  worker failures and synchronous rebuild-start failures. After dismissal,
  keep the detailed error directly below the rebuild button and reopen
  Configure for correction.
- **Verification:** Linux debug selftest, `M75_SCSI_CONTROLLER_OK`, Linux
  release, macOS release, MinGW cross build, and `git diff --check` passed.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).
- **Commit:** [024855e](https://github.com/nakatamaho/vaeg/commit/024855e799750d762be0526cb10ada43a573f30d)


### GLASS P4 SGP face spans left localized holes

- **Status:** fixed in M97.
- **Symptom:** the P4 cube contained short diagonal black runs inside otherwise
  solid colored faces.  Regular horizontal blank-raster gaps were a separate
  presentation effect.
- **Root cause:** each convex face was split into two triangles and each span
  was independently rounded inward to packed-4bpp words, leaving an unpainted
  word at the shared diagonal.
- **Correction:** round span endpoints outward to complete packed words.  The
  change remains SGP-only and does not post-process the host framebuffer.
- **Verification:** raw logical G0 inspection changed from 155 interior runs to
  zero; the independent eight-way rectangle alignment matrix passed; the
  fixed PNG was visually inspected; CTest passed 84/84 with one expected skip.
- **Evidence:** [M97 P4 visual-hole report](../agents/reports/m97_p4_visual_holes.md).
- **Commit:** [db8ce8f](https://github.com/nakatamaho/vaeg/commit/db8ce8f)


### GLASS P4 SGP loader returned through a corrupt stack

- **Status:** fixed in M97.
- **Symptom:** leaving the SGP payload with ESC did not reliably return to the
  local loader and could leave a black or unusable screen.
- **Root cause:** the loader saved the pre-continuation stack pointer instead
  of pushing and saving its `loader_return` continuation, so the payload
  `retf` consumed a flags word as a return frame.
- **Correction:** preserve flags, push `cs:loader_return`, save the real
  `ss:sp`, and use the VA Keyboard BIOS documented `BH=0, BL=1Bh` ESC result.
- **Verification:** the VAEG P4 SGP capture returned with exit status 0 after
  the ESC input event; NASM payload build passed.
- **Evidence:** [M97 P4 visual-hole report](../agents/reports/m97_p4_visual_holes.md).
- **Commit:** [db8ce8f](https://github.com/nakatamaho/vaeg/commit/db8ce8f)


### GLASS P4 outward word rounding overfilled polygon edges

- **Status:** fixed in M97.
- **Symptom:** the first hole correction produced horizontal stair-step
  protrusions beyond blue, yellow, and green face boundaries.
- **Root cause:** rounding the logical span outward changed polygon coverage;
  packed-word alignment was incorrectly treated as geometry.
- **Correction:** retain inclusive logical `[x0,x1]` spans, emit only complete
  interior words through SGP, and apply one exact masked CPU word RMW for the
  endpoint words.  The packed 4bpp layout is four pixels per word, with the
  VA byte-swapped mask order recorded in the source.
- **Verification:** independent face oracle reports underfill=0 and overfill=0
  before outlines; exhaustive alignment/endpoint/outside-border tests pass;
  final PNG was inspected; CTest remains 84/84 with one expected skip.
- **Evidence:** [M97 P4 visual-hole report](../agents/reports/m97_p4_visual_holes.md).
- **Commit:** [9a778bc](https://github.com/nakatamaho/vaeg/commit/9a778bc)


### NEON inherited the caller's soft-key guide

- **Status:** fixed in M100; TEXT remains enabled for the NEON overlay.
- **Symptom:** the NEON bottom row showed the caller's `DIR A:`, `COPY`, and
  `TIME` function-key/status guide while the intended NEON text was visible.
- **Root cause:** startup left the VA Text BIOS soft-key producer and Screen
  Editor system-line presentation owned by the loader/editor environment.
- **Correction:** use Text BIOS `INT 83h/AH=2Fh, AL=00h` to stop the producer,
  then Screen Editor `INT 94h/AH=01h, AL=FFh` to hide the system-line display;
  restore both services on exit.
- **Verification:** NASM payload builds for 640x200 and 640x400, VAEG raw
  TVRAM validation (`TEXT_VISIBLE=PASS`, `BOTTOM_GUIDE=PASS`), repository
  case/diff checks, and the dedicated stale-console validator passed.
- **Evidence:** [NEON text-console report](../port/neon3_text_console.md).
- **Commit:** [93ac88b](https://github.com/nakatamaho/vaeg/commit/93ac88b3d98cbb117c688a16d569f93e31b2986e)


### NEON4 320x200 direct-colour spans used a corrupted SGP destination

- **Status:** fixed in M97.
- **Symptom:** the 320x200 x 65536-colour NEON4 profile regressed to sparse
  or missing graphics after the 640x400 packed profile was added; the 16-colour
  profile remained on its separate path.
- **Root cause:** the packed-profile refactor shared a post-address label with
  the direct-colour path.  The second `p5_emit_color_if_needed` call converted
  the colour through `SI`, overwriting the already calculated SGP `CLS`
  destination address before the command was emitted.
- **Correction:** keep the second colour emission conditional on the 4bpp
  path, which reaches that label after address calculation.  The 8bpp and
  16bpp paths emit colour once before calculating the destination and now keep
  `SI` intact.
- **Verification:** fixed 320x200 direct-colour capture reached the stable
  frame-loop checkpoint and rendered the scene on the VAEG VA2 model;
  640x400 packed capture also reached its checkpoint; all three NASM profiles
  built; VAEG selftest passed; CTest passed 84/84 with one expected skip.
  The separate VA-model display path remains outside this regression result.
- **Evidence:** [NEON4 P5 source](../../demos/neon4/src/neon4_p3.asm).
- **Commit:** [1a02f35](https://github.com/nakatamaho/vaeg/commit/1a02f354363b7aedfa699f6a51d73c3745842abd)


### NEON4 640x400 4bpp CPU endpoint writes wrapped at 64 KiB

- **Status:** fixed in M97.
- **Symptom:** the 16-colour profile showed a detached duplicate tetrahedron
  and diagonal fragments at a different location from the rendered face.
- **Root cause:** the exact endpoint read-modify-write path calculated a
  128 KiB page offset in a 16-bit offset register, so rows in the upper half
  wrapped at 64 KiB and wrote endpoint nibbles into unrelated rows.
- **Correction:** retain the exact packed-4bpp endpoint mask, but select the
  next 4 KiB CPU-aperture paragraph when the calculated offset has a non-zero
  high word.  The SGP 24-bit destination path and polygon geometry are
  unchanged.
- **Verification:** disabling endpoint RMW in a diagnostic build removed the
  detached shape, proving the fault was in the CPU endpoint path; the fixed
  4bpp VAEG capture has only the intended central geometry.  The 16-colour and
  65536-colour payloads build successfully, P4 CPU/SGP builds pass, and CTest
  passes 84/84 with one expected skip.
- **Evidence:** [NEON4 P5 source](../../demos/neon4/src/neon4_p3.asm).
- **Commit:** [4c10eb4](https://github.com/nakatamaho/vaeg/commit/4c10eb432befa6d0bff66d19c9246ee7a4bc86b5)


### NEON4 direct-colour scenes used low-colour face indices

- **Status:** fixed in M97.
- **Symptom:** the 65536-colour profile rendered scene faces with grey or
  otherwise incorrect hues around frame 0x1CE, even though the direct
  `pegc_palette_grb` to G6/R5/B5 table was correct.
- **Root cause:** the shared 286 geometry path selected `low_face_theme_pairs`
  (16-colour analog indices such as 3, 5, and 13) before the direct-colour
  backend converted the value.  Those indices are grayscale entries in the
  original 256-colour palette, so the direct profile never reached the blue
  ramp selected by the original cross-area shade calculation.
- **Correction:** direct profiles now retain the original 256-colour shade
  rule (`clamp((projected_cross >> 11) + 8, 8, 31)`) and produce the original
  ramp indices before conversion.  Direct-profile ribbon strips and finale
  blades likewise use their original 256-colour indices; low-colour graph
  colouring and dithering remain confined to the 4bpp profile.
- **Verification:** the source `pegc_palette_grb` table independently converts
  to all 256 RGB332 bytes and all 256 G6/R5/B5 words with zero mismatches;
  NASM builds pass for 4bpp, RGB332, and direct 16bpp profiles; the bootable
  validation disk contains both rebuilt payloads; CTest remains subject to
  the pre-existing `vaeg_upd9002_trace_equivalence` FMBOARD statsave failure.
- **Evidence:** [NEON4 P5 source](../../demos/neon4/src/geom4_low.inc),
  [NEON4 P5 colour report](../port/neon4_p5.md).
- **Commit:** [f86c3c0](https://github.com/nakatamaho/vaeg/commit/f86c3c0de7e224166d6eaadf2b0f56bba8e3e01f)


### NEON4 direct-colour CAT image used a clobbered span-table cursor

- **Status:** fixed in M97.
- **Symptom:** the 65536-colour NEON4 RASTER TRANSFER scene (frames around
  `0620h`-`0700h`) rendered the moving square CAT image with corrupted spans;
  the first row was plausible, but later rows contained misplaced or stretched
  coloured runs.  The 4bpp CPU endpoint path did not reproduce the failure.
- **Root cause:** `low_draw_cat48` and its 200-line overlay walk their source
  span tables with `DI`.  The SGP `p5_emit_span_physical` and
  `p5_emit_line_physical` routines also used `DI` as their command-list cursor
  without preserving the caller's value.  After the first SGP span, the CAT
  walker therefore read x coordinates from command-list words instead of from
  `low4_data.inc`.
- **Correction:** save and restore `DI` in both physical SGP emitters, while
  retaining `p5_list_offset` as the command-list state.  No CAT coordinates,
  image data, colour conversion, or geometry-specific correction was changed.
- **Verification:** the CAT span tables were independently compared with the
  canonical 48-row source (650 spans, zero differences); NASM builds pass for
  4bpp, RGB332, and direct 16bpp profiles; the assembled direct-profile listing
  contains matching `push di`/`pop di` pairs in both emitters; a rebuilt
  bootable validation disk contains the corrected `65536/neon4.com`.
- **Evidence:** [NEON4 P5 source](../../demos/neon4/src/neon4_p3.asm),
  [NEON4 P5 status](../port/neon4_p5.md).
- **Milestone/task:** M97 NEON4 direct-colour CAT span cursor preservation.
- **Commit:** [7c3fb85](https://github.com/nakatamaho/vaeg/commit/7c3fb857b49b90c4f384411125c94d37ce75bf0)


### ZUNDORB cadence demo treated command Return as exit

- **Status:** fixed in M98r; corrected VAEG VA2 candidate accepted by the
  maintainer; real hardware remains pending.
- **Symptom:** the interactive M98r candidate returned immediately to the
  Human prompt after `ZUNDORB` was entered, without displaying the synthetic
  scale animation and without printing an M98R failure diagnostic.
- **Affected scope:** the ZUNDORB guest's interactive keyboard polling; the
  atlas, BMS, SGP, G1, and cadence rendering paths were unchanged.
- **Demonstrated root cause:** the guest accepted Return as a second exit key,
  and its remaining ESC test recognized scan code zero without checking the
  BIOS internal code. A command-confirming Return left in the guest keyboard
  path could therefore request normal cleanup before a visible publication.
- **Correction:** Return is no longer an exit key. Both interactive polling
  paths require the complete INT 82h/AH=09h ESC result: scan code `00h` and
  internal code `1Bh`.
- **Verification:** a release-mode regression injects an additional Return at
  the relocated guest entry and then captures 58 publications. All 58 G1
  buffers match the accepted M98r golden byte-for-byte, with exactly 58 SGP
  sources and destinations. The 131 demo tests, VAEG selftest, all eight
  static VA2 divisor cases, opposite-page V1/V4/V8 runs, selector ladder,
  pause/resume, and missed-slot cases pass.
- **Evidence:** [M98r cadence report](../agents/reports/m98r_zundamon_vblank_cadence.md).
- **Milestone/task:** M98r interactive Return/ESC discrimination.
- **Commit:** [3c3f233](https://github.com/nakatamaho/vaeg/commit/3c3f233305915aa61c594886520764b578ef5025).

### ZUNDORB atlas streaming failed after leaking the saved ES value

- **Status:** fixed in M98t; VAEG VA2 validation passed; real hardware remains
  pending.
- **Symptom:** after the M98t guest grew, the accepted atlas could return from
  the bounded BMS loader with corrupted metadata state and fail before the
  first depth-coupled publication.
- **Affected scope:** the ZUNDORB guest's incremental atlas file-CRC setup.
  The atlas bytes, BMS implementation, SGP implementation, and emulator memory
  map were unchanged.
- **Demonstrated root cause:** `load_atlas_to_bms` saved `ES`, temporarily set
  it equal to `DS` for the header CRC, but did not restore the saved word. The
  unmatched stack word shifted the loader's return path; the larger M98t image
  made the latent imbalance reproducible.
- **Correction:** balance the local `push es` with `pop es` immediately after
  `crc32_update_es`, before streaming payload chunks or reaching cleanup.
- **Verification:** the first corrected bounded VA2 run streamed and CRC-
  checked the 5,912-byte public atlas, completed 64 depth/scale publications,
  and passed the independent BMS, SGP-trace, indexed G0/G1, HUD, and composite
  oracle. The final four 128-publication full/dirty parity runs, eight cadence
  runs, dynamic scheduler cases, 159 host tests, and VAEG selftest pass.
- **Evidence:** [M98t depth/scale and HUD report](../agents/reports/m98t_zundamon_depth_scale_hud.md).
- **Milestone/task:** M98t ZUNDORB bounded atlas loader stack restoration.
- **Commit:** [9440798](https://github.com/nakatamaho/vaeg/commit/9440798d13bd00229b03163f98f9fee7c4caac68).
