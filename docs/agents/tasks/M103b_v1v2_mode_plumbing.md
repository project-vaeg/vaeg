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

# M103b - V1/V2 mode plumbing: BRKEM2, boot select, 88-mode window, I/O trap

Status: **complete; G103b passed on 2026-10-03**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103b-v1v2-plumbing` off the
integration branch `topic/v1v2-mode`. Following the maintainer's M103b
request, these branches were created at `33f268cc5487e4feffdde75bcb495c4657cdcf94`
on top of the pending M103a branch, not `main`. This does not mark G103a
passed or authorize a main merge; reconcile the dependency before merging.
Commit prefix `M103b:`. Plan and hardware model:
[`docs/modernization/v1v2-mode-plan.md`](../../modernization/v1v2-mode-plan.md).

## Goal

Make the VA2 boot ROM take its own V1/V2 path end to end: boot selection,
display/memory setup in V3, `153H` bit 6 ← 0, I/O trap on, `BRKEM2 90h`,
and N88-BASIC running from `1000:0000` inside the 88-mode window until it
waits for the keyboard. No display or keyboard work in this milestone; the
gate is trace-based. Follow the real machine's automatic FDD selection;
there is no user-facing V1/V2 boot override. The BRKEM2 decoder does not
select the memory map or force a boot path. Verify unchanged V3 disk
boot and automatic selection of a compatible disk at the human gate.

## Scope

1. **`BRKEM2 imm8` (`0F FE nn`)** in `cpu/upd9002/`: enter the
   uPD70008-compatible mode through vector `nn` exactly as `BRKEM` does
   (ADR/CPU document §2.2.2: no architectural difference has been found;
   the machine-level switch is the `153H` write four instructions earlier).
   Keep one implementation and record the entry encoding in a trace
   event for `brkem2-target` (§15.2) should a difference appear later.
   Decode in the existing `0F` dispatcher; self-test in the M76 harness.
   There is no native instruction disassembler in the active tree (the
   uPD780 disassembler is unrelated); no disassembler entry is added.
2. **Boot selection.** Preserve the existing active-low PC-key input
   (`000Dh` bit 2) in `io/serial.c`. Port `40h` bit 3 is SW7: zero tries
   intelligent-FDD boot; one skips it. The diagnostic trace with SW7=1
   and the PC key unpressed reaches `BRKEM2` (plan §5), but bypasses the
   disk-identification path and is not an acceptance test. Keep SW7=0
   and trace the FDD command/result branches through `1F90h`. Do not
   replace automatic media selection with a GUI/INI mode selector.
3. **`153H` bit 6 — system memory mode.** `io/memctrlva.c`: store the bit,
   read it back, and switch `memoryva` between the V3 map and the 88-mode
   map for physical `10000h`–`1FFFFh` (and `20000h`–`3FFFFh` as ERAM
   pages). Save state.
4. **88-mode window.** In `memoryva/`, a window model for `10000h`–`1FFFFh`
   driven by the hardware banking ports (audit and extend existing
   handlers rather than attaching over them): `31H` OUT (`MMODE`, `RMODE`, `VW1`, `GDENO`, `PM00`), `32H`
   (`ROMSL`, `TMODE`, `GVAM`, `PMODE`, `AVC`), `5CH`–`5FH` (plane select
   into `C000h`–`FFFFh`), `70H`/`71H`/`78H`, `E2H`/`E3H`, with N88-BASIC
   and its extension banks taken from the existing VA ROM images
   (`varom1.rom` `0x10000` and `0x18000`–`0x1FFFF`; CPU document §4.6 and
   Appendix C). **Derive the exact page selection from the ROM**: trace
   the V3-side setup in the handoff block and the shared initialiser
   (`0x13A8`/`0x2210`, §8) and the first instructions N88-BASIC executes,
   and record the derivation with `[ROM]`/`[DERIVED]` tags in
   `v1v2-mode-plan.md` §2. TVRAM (`1F000h`) and GVRAM (`1C000h`) backing
   are stubs in this milestone (plain RAM that M103c/M103d replace), but
   the port-side state (`5CH`–`5FH`, `34H`/`35H`, `TMODE`) is kept.
5. **I/O trap hardware** (§9): registers `FFE0h`–`FFE7h` (byte and word
   writes) and `FFEFh`; matching on the low byte in word mode; in both
   CPU modes, a trapped `IN`/`OUT` performs no access and raises native
   interrupt `7Ch`/`7Dh` with `CS:IP` at the I/O instruction and the
   interrupt flag cleared, exactly the frame 98IOE and the ROM handler
   expect (§9.2). Ports `FFE0h`–`FFFFh` never trap. The compatible-mode
   adapter's `In`/`Out` must route through the same check before
   `iocore_inp8`/`iocore_out8`. Unit test with a tiny native handler that
   advances the saved IP; adapter self-test with a Z80 `OUT (50h),A`.
6. **8214 (`E4H`/`E6H`) and kanji ROM (`E8H`–`EDH`) ports**: no stubs were
   needed. BASIC's writes to `E4h`/`E6h` reach the default handler, which
   ignores them without fault, and the traced path reads no kanji port.
   Their semantics belong to M103c/M103d; adding state-only stubs now
   would only create save-state fields to migrate later.
7. **Trace.** Extend the compat trace with the mode-entry encoding, the
   `153H` state and trap events, so the gate can be checked from a log.
   Done: `compat-entry` CPU events, `io-trap-in`/`io-trap-out` compat
   events, the existing `banktrace` line for `153h`, and the
   `VAEG_UPD70008_TRACE_SKIP` window offset.

Out of scope: text/graphics rendering, keyboard matrix, interrupt
delivery into Z80 code (M103c), FDD under V1/V2 (M103d), V1 mode.

## Deliverables

- Code as above with save-state coverage (new fields versioned per M85).
- `docs/modernization/v1v2-mode-plan.md` §2 window map derived from the
  ROM; CPU document §15.2 `bootsel-000d` and `brkem2-target` updated with
  what the trace shows.
- Tests: `BRKEM2` self-test, trap unit/self-tests, window-map unit tests
  against the derived table, ROM-driven automatic disk-selection trace
  test (romful, skipped without the private ROMs and media).
- Ledger entries for concrete defect corrections under AGENTS.md, including
  V1/V2 defects; feature-only additions do not require entries.

## Gate G103b (human)

Standard gate for V3 (clean build, V3 boot, VA demo, OS boot) unchanged,
plus, with the VA2 ROM set and compatible boot media, without overrides:

- the trace shows the ROM reading `000Dh` with the PC key unpressed,
  taking the `0x136C` path, reading SW7=0 at `40h` and executing the FDD
  boot call; record the media-dependent branch and return to the handoff.
  The reset/handoff sequence must show programming `FFE0h`–`FFE7h`, writing `FFEFh ← 03h`, clearing `153H`
  bit 6, executing `BRKEM2 90h`, and Z80 execution starting at
  `1000:0000`;
- N88-BASIC's initialisation runs inside the window (ROM-bank switching
  through `31H`/`32H`/`71H` visible in the trace), disk BASIC loads from
  the media, and execution reaches the stable key-input wait loop. That
  loop polls the interrupt-filled key buffer, not ports `00h`–`0Eh`;
  keyboard scanning runs in the interrupt path delivered in M103c. The
  text RAM must hold the disk BASIC banner and first input prompt
  (display itself is M103c);
- no trap fires outside `50h`–`5Bh`/`60h`–`6Fh`, and every trap that
  fires returns through the ROM handler with the IP advanced.

## Summary for G103b

Machine-verifiable parts (local, `linux-ci-gcc`): full CTest has no
failures (108 entries; the external SST and, without private paths
configured, `vaeg_m103b_basic_boot` are skipped); V3 `--smoke` with the
VA2 ROM set passes; `tools/qa/m103b_basic_boot.py run` passes with the
VA2 ROMs and V2 BASIC media and fails closed for media that takes the
native IPL branch. Human-gate items that remain: the standard V3 gate
(VA demo, OS boot and operation), saving and resuming a live
compatible-mode session, and acceptance of the documented implementation
policies (shared BRKEM/BRKEM2 entry, the RAM-window reset origin 80h,
byte/word-port trap matching, low-byte compatible matching). The 88-mode
RAM backing and the 1KiB window at 18000h–183FFh are now corroborated by
`[NEC-GIHO]` Figures 3 and 4 (plan §2.3) rather than being policy.
M103a passed G103a and was merged to `main` at
[23dd911](https://github.com/nakatamaho/vaeg/commit/23dd911e7ca1b14d757444ae4db0be37b273de30);
this branch was rebased onto that merge, so it includes the UPD9MODE
compatible-mode state section and the IRET F-flag correction.

Deferred beyond M103b: monitor ROM (RMODE=1), ERAM (`E2h`/`E3h`), 88-mode
TVRAM/TSP and interrupt delivery (M103c), compatible GVRAM ALU and
rendering (M103d), DD/FD-prefixed compatible I/O traps and trap timing.

## Implementation progress

- `0F FE imm8` now shares the existing `BRKEM` entry routine; this is an
  implementation policy, not a measured proof of complete architectural
  equivalence. No memory-mode bit is changed by the instruction.
- `compat-entry` CPU trace events record the second opcode byte in
  `address` (`FE` or `FF`) and the vector in `value`. No persistent CPU
  field or save-state format change is needed for this diagnostic.
- The production-adapter test runs both entry encodings, checks the
  six-byte native return frame and preserved DS/SS, then exercises
  compatible instructions, CALLN, nested native interrupt/IRET, RETEM,
  and compatible-state restore. This is ROM-less testing, not G103b.
- The SW7 GUI/INI option from `a4d85e64835ab0274731682c92332bb0bab4d368`
  is withdrawn following maintainer clarification: normal hardware selects
  V1/V2 automatically from the FDD. The forced-SW7 trace only isolated
  entry decoding. Memory mapping and I/O trapping remain pending.
- Bounded traces now confirm automatic FDD selection for synthetic 256-
  and 512-byte-sector 2D media and maintainer-provided BASIC media (plan
  §5.2). BASIC reaches BRKEM2 without SW7 changes, but then fetches zero
  bytes instead of BASIC ROM. All instrumented runs timed out after
  capturing this prefix; G103b has not passed. The next implementation
  target is the 88-mode memory decoder, not a boot-selection override.
- The memory-mode latch has now been recovered from the saved work,
  without the withdrawn SW7 override. Port `153h` bit 6 latches and reads
  back; reset selects V3 (`41h`). The production I/O regression covers
  byte writes, the ROM's word read/write at `152h`, and reset after an
  88-mode request. The initial latch-only step is now extended with the
  partial ROM overlay described below.
- New optional binary state section `MEM88MODE`, version 0, stores one
  byte (0=V3, 1=88-mode request). The old `MEMORYVA` structure and its
  reserved bytes remain untouched. State loading calls `iocore_reset`
  before section loading, so an old state without `MEM88MODE` keeps V3.
  Complete old/new state-file round trips remain unverified; current
  tests cover the I/O/reset path, not the full loader.
- Initial N88 ROM overlay is implemented for reads at physical
  `10000h`–`17FFFh`, selected by the mode latch and port 31h bits 1/2.
  Optional `MEM88SYS` saves the port 31h latch. Synthetic tests cover
  both word boundaries, MMODE switching, and RAM writes under ROM.
  Physical RAM backing is provisional; monitor ROM, ERAM and video
  mappings remain pending (plan §2.0).
  An automatic-FDD BASIC run now executes the ROM entry instructions,
  including `LD SP,E1A0h` and `JP 3BE5h`; 600-frame capture exits 0 but
  does not establish a BASIC prompt. Full state-file testing is pending.
- Normal CPU/DMA accesses already pass through the byte/word VA decoder
  in `memoryva/memoryva.c`; only the isolated SST flat-memory seam
  bypasses it. Add the window there, including word-boundary handling,
  rather than introducing a second decoder in the compatible adapter.
- Local build and all 106 CTest entries complete without failures (one
  external SST skipped). Human boot acceptance is not inferred from
  these tests. The original named stash remains as a recovery copy.

### Extension-ROM stage

- Port 71h XEROM and existing port 32h bits 1–0 now select four 8KiB
  extension banks at compatible 6000h–7FFFh. XEROM reads as FEh/FFh and
  resets to disabled; `MEM88EXT` version 0 preserves the new bit.
- Synthetic tests cover all banks and word reads crossing both edges.
  Integration BASIC executes `OUT 71h,FEh` and reads extension pointer
  `6045h` → `6B55h`. An 1800-frame run completes (exit 0), but no BASIC
  boot completion is claimed. Earlier short trace limits ended in delay
  loops and were not evidence that those loops were stuck.

### RAM-window stage

- Port 70h offset and value-independent port 78h increment drive the
  1KiB window at compatible 8000h–83FFh in N88 ROM/RAM mode. RAM offsets
  wrap at 64KiB and bypass ROM overlays; V3/all-RAM mode bypass the
  window. Physical backing and reset origin 80h remain implementation
  policy, not established VA hardware equivalence (plan §2.0.1).
- Optional `MEM88WIN` version 0 stores the offset. Production tests cover
  register readback, increment wrap, RAM behind ROM, boundary word reads
  and writes, mode bypass, and reset. Full file round trips are pending.

### Independent GVRAM plane mapping

- Ports 5Ch–5Fh select one of three 16KiB planes or disable access;
  IN 5Ch reports selection bits. The 88-mode C000h–FFFFh window now uses
  existing GVRAM storage (plane offset 4000h), without modifying underlying
  RAM. Byte splitting preserves both word boundaries. V3 bypasses it.
- `MEM88GFX` version 0 saves the plane latch; reset/missing old sections
  disable access. Full state-file checks remain pending. Synthetic tests
  cover plane isolation, status, RAM preservation, word edges and reset.
- This is independent storage access only. Compatible ALU, rendering and
  hardware timing remain out of scope for this step. An 1800-frame BASIC
  capture exits 0 but proves neither boot completion nor the suspected
  cause of initialization repeating (plan §2.2).

### FDD fast-transfer data path

- After the GVRAM plane mapping, integration BASIC no longer restarted
  within the traced range and reached its RAM-resident disk loader. The
  loader uses the fast protocol: main writes `93h` to port FFh (8255
  ports A and B input) and reads two bytes per handshake from FCh and FDh.
  The first two header words read as `FF00h`, then the loader waited
  forever for more data (sub CPU side had stopped).
- Demonstrated cause: only main B→sub A and sub B→main A were wired;
  sub A→main B and main A→sub B were absent, and the main side had no
  `IN FDh` or `OUT FCh` handler. V3 transfers use only the wired paths.
- Correction: complete the cross-wiring (main A↔sub B, main B↔sub A)
  through the existing `busout`/`businport` callbacks and attach main
  `IN FDh`/`OUT FCh`. The saved `_I8255` structures are unchanged.
- Verification: a new production-seam test in the uPD780 integration
  suite exchanges distinct bytes over all four data paths. It failed
  without the correction (`sub-to-main data ports are not cross-wired`)
  and passes with it inside `vaeg --selftest`. With the correction the
  BASIC loader receives header `8800h`/`1600h`, completes its transfer
  (BC reaches zero) and proceeds to a delay loop. Boot completion is not
  claimed. `vaeg --selftest` runs in CTest as `vaeg_romless_tests` (an
  earlier note here wrongly said it was not a CTest case).

### I/O trap register stage

- `FFE0h`–`FFE7h` and `FFEFh` writes now latch in a separate
  `UPD9002_IOTRAP` object. Optional state section `UPD9TRAP`, version 0,
  preserves these nine bytes without altering the legacy `UPD9002`
  section. Reset disables traps and clears ranges; missing old-state
  sections therefore retain disabled defaults. Full state-file testing
  remains pending. No undocumented register readback is added.
- The production I/O test compares the ROM's descending byte writes
  with ascending word writes, checks reset, and checks that disabling
  the control preserves ranges. The subsequent compatible interception
  stage below now consumes this register state.
- ROM inspection establishes a compatible-specific handler: VA2 vectors
  7Ch/7Dh both point to `F000:1944`. It saves eight words before assigning
  BP, reads the saved far instruction address at `[BP+10h]`, and decodes
  the compatible opcode (including `DBh` and ED forms), not native x86
  IN/OUT opcodes. At `1958h`/`195Bh` it increments saved IP twice, then
  restores registers and executes IRET. This supports trapping before
  executing a two-byte compatible I/O instruction; prefix and block-I/O
  details still need explicit tests rather than assumptions.

### Compatible I/O interception stage

- Immediate `DB`/`D3` and ordinary ED IN/OUT forms (`40/41` through
  `78/79`, including `70/71`) are checked before execution. Enabled
  directions match the low-byte inclusive ranges. A match transfers to
  native vector 7Ch/7Dh, saving the original instruction IP and disabling
  interrupts/tracing flags after saving the frame. No device I/O occurs.
  The existing pending-return mechanism resumes through native IRET.
- The adapter test exercises all 18 encodings under both BRKEM entries,
  using spy ports to assert zero device callbacks and checking the saved
  IP, stack depth, interrupt disable, unchanged accumulator and IRET
  restoration. The synthetic handler intentionally retries the same IP
  to test successive forms. Real-ROM traces separately demonstrate IP+2:
  BASIC `1000:3BD3` (`D3 53`) → `F000:1944` → `1000:3BD5`.
- A 600-frame automatic-FDD BASIC run exits 0 and captures trap/return
  events; this does not establish a BASIC prompt. Raw logs stay outside
  Git. Native interception and block I/O are added in the next stage;
  redundant DD/FD prefixes and accurate trap timing remain pending.
  Compatible interception currently uses the low port byte and shared
  CALLN transition timing as an explicit partial implementation policy.

### Native interception and compatible block I/O

- Native `E4`–`E7` and `EC`–`EF` check the trap before any access. A match
  raises vector 7Ch/7Dh through the core interrupt path with the saved IP
  at the instruction start (`upd9002_step_start_ip`, the core's fault
  restart point, so a redundant segment prefix is included) and IF/TF
  cleared. FFEFh bit 4 clear compares the full 16-bit port with the
  16-bit ranges; bit 4 set compares the low byte only (§9.3). Ports
  FFE0h–FFFFh never trap. The byte/word-port reading follows the CPU
  document's interpretation of CoBit's source, not a measurement.
- Compatible ED block I/O (`A2/A3/AA/AB/B2/B3/BA/BB`) now traps on the
  low byte of BC. The VA2 ROM handler's jump table at `F000:1996`
  decodes exactly these eight forms and repeats the R forms itself.
  Redundant DD/FD-prefixed compatible I/O stays untrapped: the ROM
  handler would misdecode a prefix byte, and hardware behaviour is
  unknown.
- Tests: the native test covers all eight encodings at both range edges
  with no device callback, a prefixed form, an out-of-range port, a
  disabled direction, the byte-mode 16-bit miss, word-mode low-byte
  matches, and an `OUT FFEFh` that executes even when the range covers
  it. The compatible test now covers 26 encodings. Each test fails when
  its interception is removed and passes when restored.
- V3 smoke without media still passes. The automatic-FDD BASIC path is
  unchanged through the loader transfer (67 compatible trap events in
  the first million compatible instructions).

### Full state-file round trip of the new sections

- `vaeg_romless_tests` now saves a complete state with nondefault values
  in all six optional sections (`MEM88MODE`, `MEM88SYS`, `MEM88EXT`,
  `MEM88WIN`, `MEM88GFX`, `UPD9TRAP`), resets, loads it through
  `statsave_load`, and checks every value through the production I/O
  paths. It then removes those six sections from the file to emulate an
  older build's state and verifies that loading it over nondefault
  values yields the V3 defaults. Removing `MEM88WIN` from the state
  table makes the test fail (`new sections did not round-trip`).
- This closes the previously pending full state-file check for these
  sections. It does not save or resume a live compatible-mode BASIC
  session; that remains a G103b human-gate item.

### Disk BASIC reaches its first input prompt

- With the corrections above, a local diagnostic run (temporary trace
  skip and RAM dump, removed before commit) shows that after the loader
  BASIC settles from about three million compatible instructions onward
  into a stable 38-instruction loop: `DI`, a key-buffer head/tail compare
  at `45F8h`, `EI`, a branch back when empty, plus the cursor routine at
  `7780h`. No I/O is performed in this loop.
- A dump of compatible 0000h–FFFFh at that point shows the text area at
  `F3C8h` holding the disk BASIC version banner, the first input prompt
  (number of files) and the function-key line. Nothing is displayed
  because 88-mode TVRAM mapping and TSP rendering belong to M103c.
- This is emulator evidence from maintainer-provided media; the raw dump
  stays outside Git. Keyboard input cannot be exercised until interrupt
  delivery (M103c).

### Automated key-wait acceptance test

- `VAEG_UPD70008_TRACE_SKIP=M` (new, with `VAEG_UPD70008_TRACE=N`) passes
  over the first M compatible-mode trace events before recording N, so a
  bounded window can sit late in a long run. It costs nothing when
  `VAEG_UPD70008_TRACE` is unset.
- `tools/qa/m103b_basic_boot.py run` copies the media to a temporary
  directory, boots the VA2 ROM set without any override for 6000 frames,
  records 20000 compatible instructions after skipping 30 million, and
  requires: every record in segment 1000h, every saved native frame
  `13B4/F000/...` (BRKEM2 at F000:13B1), and an address set inside the
  observed 38-address key-wait loop containing its anchors. Stable error
  codes: `M103B_NO_TRACE_WINDOW`, `M103B_ENTRY_FRAME_MISMATCH`,
  `M103B_SEGMENT_MISMATCH`, `M103B_NOT_KEY_WAIT`, `M103B_WORKER_FAILED`.
- CTest: `vaeg_m103b_basic_boot_selftest` (romless) checks one passing
  fixture and six single-mutation fixtures, each against its exact code.
  `vaeg_m103b_basic_boot` runs only when the cache variables
  `VAEG_V1V2_ROM_DIR` and `VAEG_V1V2_BASIC_MEDIA` point at
  maintainer-local files, and otherwise reports skipped (exit 77).
- Local results: integration BASIC passes in about 70 s (20000 records,
  38 addresses); the synthetic 512-byte-sector media, which takes the
  native IPL branch, fails with `M103B_NO_TRACE_WINDOW`. The address set
  is specific to this ROM/media pair and to the absence of interrupt
  delivery; M103c is expected to change the test.

## Research notes for the implementer

- Hardware-versus-trap port table and the TVRAM/GVRAM mapping: plan §2–§3
  (BNN manual ch. 8; CPU document §9).
- The ROM's V1/V2 path and resume block are disassembled in CPU document
  §8/§8.2 (VA2 offsets; VA1 counterparts given).
- X88000 is the reference for what 8801 software expects from `31H`,
  `32H`, `5CH`–`5FH`, `70H`/`71H`, `E2H`/`E3H` (ADR-0016: behaviour only,
  derivations recorded).

### G103b (passed 2026-10-03)

- Maintainer results: (1) standard V3 gate OK; (3) the screen does not
  change, which is the expected M103b result because 88-mode TVRAM/TSP
  rendering belongs to M103c; (5) the implementation policies are
  approved: shared BRKEM/BRKEM2 entry, RAM-window reset origin 80h,
  byte/word-port native trap matching, low-byte compatible matching.
- Items 2, 4 and the behavioural part of 3 are machine-verified on Linux
  at `18599ef661a23888456741ad01f95d82e5429fc3` (the branch after the
  rebase onto the M103a merge): full CTest 108 entries without failures
  (external SST and the private-media `vaeg_m103b_basic_boot` skipped in
  CTest); `tools/qa/m103b_basic_boot.py run` with the VA2 ROMs and the V2
  BASIC media reaches the key-input wait; the earlier text-RAM dump shows
  the disk BASIC banner and first prompt.
- No separate Windows gate binary was built for G103b; the maintainer's
  checks used builds already in hand.
