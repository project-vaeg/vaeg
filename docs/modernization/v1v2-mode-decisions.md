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

# V1/V2 mode: implementation decisions register

This register lists every place where vaeg's V1/V2 mode follows a choice
that is not a direct statement of a VA manual or a measurement on a real
PC-88VA: derivations from ROM code, behaviour borrowed from the PC-8801
side, and plain implementation policy. It is the list to review at the
human gates and the list of things to check on a real machine. Hardware
model and milestone plan:
[`v1v2-mode-plan.md`](v1v2-mode-plan.md); CPU side:
[`upd9002-upd70008-mode.md`](upd9002-upd70008-mode.md).

## How to read it

Basis tags:

- `[VA-TM]` stated by the BNN technical manual or the electronic
  technical manual (`tekumani`).
- `[NEC-GIHO]` NEC Giho article on the PC-88VA (summarised, not quoted).
- `[UPD72022]` NEC µPD72022 data book (the generic part behind the VA
  TSP), as reconstructed in [`upd72022-tsp.md`](upd72022-tsp.md).
- `[ROM]` read from VA2 ROM code or traced while it runs.
- `[X88000]` PC-8801 behaviour as implemented by X88000 1.5.3 (public
  domain), used as a behavioural reference and re-implemented.
- `[DERIVED]` concluded from the above; not stated anywhere.
- `[POLICY]` an implementation choice with no hardware basis.
- `[MEAS]` measured on a real machine.

Status:

- **Approved** — accepted by the maintainer at the named gate.
- **Pending** — waiting for the named gate.
- **Open** — known gap or doubt; no decision yet.

"Real-machine check" says what would settle the point.

PC-8001 (N-BASIC) mode is described in full in
[`v1v2-n-basic-mode.md`](v1v2-n-basic-mode.md).

## 1. CPU and mode entry

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| C1 | The alternate set (`AF'`…`HL'`) lives in machine state, survives CALLN/RETI, RETEM→BRKEM and adapter reset. | `[MEAS]` ALTPRB for the CALLN round trip; `[POLICY]` for the rest | Approved G103a | `cpu/upd9002/`, `cpu/upd9002_upd70008.cpp` | — |
| C2 | Hardware reset sets all alternate pairs to FFFFh. | `[MEAS]` third-party VA2 `mon` dump shows `A'F'`, `B'C'` all ones; `DE'`/`HL'` assumed the same | Approved G103c | same | cold power-on vs. warm reset, then V2 BASIC → `mon` → `x` |
| C3 | `BRKEM2` (`0F FE nn`) uses the same entry as `BRKEM`. | `[POLICY]`; ROM-less tests of both encodings | Approved G103b | `cpu/upd9002/` | — |
| C4 | No boot override (SW7 option withdrawn); the ROM selects V1/V2 from the boot disk. V1 versus V2 follows the memory switch B1FC5h bit 0, which the Emulate > Z80 mode menu sets as the VA's setup would (M103h, maintainer request), with the ROM-rule checksum. | `[ROM]` F000:12E4 (B1FC5h → port 1C6h), F000:14BC (port 150h → MS27), F000:23B7 (checksum) | Approved G103b; menu approved by the maintainer (2026-10-05, G103h review) | `io/bkupmemva.c`, `sdl2/gui/gui.cpp` | — |
| C6 | H/S: the VA always reports high speed (MS26 = 1). As a vaeg extension, S clears IN 31h bit 6 in 88 mode (`V1V2_Standard`); CPU timing is unchanged (to be revisited after measuring the real VA's H and S speeds; the M105 CPU speed slider slows the CPU by hand meanwhile). `NEW ON 1` needs V1 S. In S, N-88 BASIC sets port 32h TMODE and keeps its text in main RAM F000h–FFFFh, which the 3301 emulation then reads, as the PC-8801 CRTC DMA does. | `[VA-TM]` MS26; `[X88000]` bit 6 = high speed; `[POLICY]` | Approved by the maintainer (2026-10-05, G103h review) | `io/memctrlva.c`, `vram/maketextva.c` | — |
| C8 | Port 150h starts from memory switch B1FC5h bit 0 at reset, so the original VA (whose ROM never writes 1C6h) follows the V1/V2 selection. Bug fix, in the ledger. | `[ROM]` VA F000:0A77, VA2 F000:12E4; original-VA hardware mode source `[POLICY]` | Approved by the maintainer (2026-10-05, G103h review) | `io/sysportva.c` | how a real original VA selects V1/V2 |
| C9 | N mode (vaeg extension, maintainer option A): with `n80.rom` and `V1V2_NMode`, RMODE is set after the ROMs load, port 40h bit 3 (SW7 OFF) reads 1 so the VA2 enters V1/V2 without a disk, and at the first compatible-mode entry the ROM's emulated port 53h gets the PC-8801 reset state (0040:00E8h and port 148h bit 7 cleared, text on). | `[ROM]` both ROMs keep 148h at 0040:00E8h and enable text only on OUT 53h; `[POLICY]` | Approved by the maintainer (option A, 2026-10-05) | `io/memctrlva.c`, `io/sysportva.c` | — |
| C10 | Cassette tape (vaeg extension): in 88 mode port 30h bits 5-4 select the 8251 cassette channel and bit 3 the motor, as on a PC-8801; port 40h bit 2 reports the carrier; raw and T88 images; bytes delivered 16 times faster by default (`CMT_Fast`). | `[VA-TM]` the VA has no cassette (8251 is RS-232C only, 30h bits 2-7 undefined); `[ROM]` N-88 BASIC cassette code (7EDFh carrier wait, 3167h receive interrupt, 7FD0h transmit); `[X88000]` channel encoding; `[POLICY]` | Approved by the maintainer (2026-10-05, G103j) | `io/cmt.c` | — |
| C11 | Tape leader (vaeg extension, M105): without fast load, when the motor starts for reading, 1200 Hz sounds for one second and then 2400 Hz for one second, and no byte is delivered until they end, so BASIC prints `Found` after them. None with fast load or while recording. | `[POLICY]` maintainer's memory of a PC-8801 ("low, then high, then Found"); no document at hand | Approved by the maintainer (2026-10-07, G105) | `io/cmt.c` | — |
| C12 | PC-8801-style V1/V2 display (vaeg extension, M105): with Emulate > モニタ > "V1/V2 表示: PC-8801 風" (`V1V2_8801Display`, default off), in 88 mode at 24.8 kHz the 3301-emulated text is drawn from the VA font's 8x8 ANK glyphs (41000h) with each row on two rasters, and 200-line graphics with RSM = 01 are shown as RSM = 00 (odd rasters blank). 400-line graphics, 15 kHz and V3 mode are unchanged. Display only. | `[MEAS]` (maintainer) a real VA in V1/V2 at 24 kHz shows 8x16 text and gapless 200-line graphics; a PC-8801 on a 24 kHz monitor shows gapless text and 640 x 200 with gaps; `[POLICY]` | Approved by the maintainer (2026-10-07, G105) | `io/videova.c`, `vram/maketextva.c`, `machine/pccore.c` | — |
| C7 | N-BASIC (vaeg extension): an optional user-supplied 32 KiB `n80.rom` is shown at 0000h–7FFFh under port 31h RMODE in 88 mode (ROM/RAM mode), as on a PC-8801; without it M3 applies. | PC-8801 N mode; `[ROM]` NEW ON 1 at 847Ah; `[MEAS]` (maintainer) N-BASIC runs on a VA from RAM | Approved by the maintainer (2026-10-05, G103h review) | `bios/romva.c`, `memoryva/memoryva.c` | — |
| C5 | IRET returning to compatible mode keeps F bits 1, 3, 5 as `(flag & 0FFFh) | F000h`. | `[MEAS]` probe results | Approved G103a | `cpu/upd9002/` | — |

## 2. Memory map and banking

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| M1 | The 64 KiB 88-mode space is VA physical 10000h–1FFFFh, RAM-backed there. | `[NEC-GIHO]` memory figures | Approved G103b | `memoryva/memoryva.c` | — |
| M2 | The 1 KiB RAM window (ports 70h/78h) sits at 8000h–83FFh, backed at 18000h; its origin resets to 80h. | `[NEC-GIHO]` for the location; `[POLICY]` for the reset origin | Approved G103b | `io/memctrlva.c` | read 70h after power-on |
| M3 | RMODE (31h bit 2) maps only 6000h–7FFFh to the Debug 8800 bank (varom00 1E000h); 0000h–5FFFh stays N88-BASIC. | `[ROM]` MON keeps executing below 6000h after setting RMODE `[DERIVED]` | Approved G103c | `memoryva/memoryva.c` | dump 0000h–5FFFh with RMODE set |
| M4 | Extended RAM: E2h holds RE (bit 0) / WE (bit 4) and reads back inverted; E3h holds page (bits 3–2) and bank (bits 1–0). | Both manuals print the page/bank field under E2h and RE/WE under E3h; `[X88000]` and a PC-8801MA2 loader use the PC-8801 numbering, which the manuals' fields match with the port numbers exchanged `[DERIVED]` | Approved by the maintainer (2026-10-04, G103e review) | `io/memctrlva.c` | write E3h, read E2h and E3h |
| M5 | Page p, bank b of extended RAM is VA RAM `20000h + p·20000h + b·8000h`; page 3 exists only with the RAM board. | `[NEC-GIHO]`, BNN V1/V2 memory map | Approved by the maintainer (2026-10-04, G103e review) | `memoryva/memoryva.c` | write through ERAM, read the VA address in V3 |
| M6 | Dictionary ROM window: F0h selects a 16 KiB bank, F1h bit 0 clear maps it read-only at C000h–FFFFh. | Not in the VA manuals or X88000; a PC-8801MA2 loader checks for `44 10` at C000h, which is how the VA2 dictionary ROM image begins `[DERIVED]` | Approved by the maintainer (2026-10-04, G103e review) | `io/memctrlva.c`, `memoryva/memoryva.c` | read C000h after `OUT F0h,0 / OUT F1h,0` |
| M7 | When the dictionary window is mapped it takes precedence over GVRAM/TVRAM in C000h–FFFFh. | `[POLICY]` | Approved by the maintainer (2026-10-04, G103e review) | `memoryva/memoryva.c` | — |

## 3. I/O trap

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| T1 | Compatible-mode trap matching uses the low port byte; native matching uses byte or word ports per FFEFh bit 4; FFE0h–FFFFh are never trapped. | `[VA-TM]` registers; `[POLICY]` for compatible low-byte matching | Approved G103b | `cpu/upd9002_upd70008.cpp`, `cpu/upd9002/` | — |
| T2 | A trapped compatible instruction transfers to vector 7Ch/7Dh with the instruction's own IP; the ROM handler advances IP. | `[ROM]` handler at F000:1944 | Approved G103b | same | — |
| T3 | DD/FD-prefixed compatible I/O and trap timing are not modelled. | — | Open | — | — |

## 4. Interrupts, timers and keyboard

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| I1 | 8214 mode at reset; port 158H selects 8259 mode until reset. | `[VA-TM]` §5.2.2; `[ROM]` V3 writes 158H, V1/V2 does not | Approved G103c | `io/pic.c` | — |
| I2 | 8214 acceptance and re-arm: accept only above the current status (or any level with XSGS), no further acceptance until E4h is written. | Intel 8214 / PC-8801 behaviour; the VA manual documents only the registers | Approved G103c | `io/pic.c` | — |
| I3 | General timer 2 (600 Hz) runs only while E6h bit 0 enables it; the phase of its first tick is arbitrary. | `[POLICY]` | Approved G103c | `io/pic.c` | — |
| I5 | When compatible-mode code sets IFF1 (EI, RETI/RETN) and a request is pending, the CPU time slice ends one instruction later so the interrupt is taken in the enabled window, as the native STI does. Bug fix, in the ledger. | `[POLICY]` matching the existing native behaviour | Approved by the maintainer (2026-10-04, G103g review) | `cpu/upd9002_upd70008.cpp` | — |
| I4 | Key-matrix releases are delayed until the key has been seen across two VRTC starts. | `[POLICY]` (one would do for N-88 BASIC; two is a margin) | Approved G103c | `io/serial.c` | — |

## 5. Text display (TSP 3301 emulation)

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| X1 | The VA2 ROM writes split-screen table fields with the generic memory commands: 8Eh `DPLD` loads DPTR0 (19-bit byte address, three bytes), 8Fh `DPLD` loads DPTR1 (signed increment; the ROM sends `01 00 00`, +1), 97h `WDAT` with MOD = 11b writes each parameter and then adds DPTR1, until the next command. vaeg writes one byte per parameter at TVRAM byte DPTR0 and always advances by one; the DPTR1 value and `MASK` (89h) are not modelled. | `[UPD72022]` §10.6 (the commands are not in the BNN VA command list); `[ROM]` traces. M103c had inferred the 8Eh/97h roles and left 8Fh unexplained. One byte per parameter in byte-access mode is `[DERIVED]`. | Approved G103c (basis now documented) | `io/tsp.c` (its comment still calls 8Fh unknown) | — |
| X2 | 3301 attribute pairs follow X88000's transparent-mode rule (memory order, shifted when the first column is non-zero, colour/secret/blink/reverse carry across rows, lines reset per row, start white). Colour attribute mode is assumed. | `[X88000]` | Approved G103c | `vram/maketextva.c` | — |
| X3 | Byte-mode address: local L = start field / 2; TVRAM byte = `((L & F000h) << 1) | (L & 0FFFh)`. | `[VA-TM]` §8.2.1 tabulates 3000h–3FFFh and B000h–BFFFh; the rule fits both `[DERIVED]` (M103c used L + 3000h, which fits only the first) | Approved by the maintainer (2026-10-04, G103f review) | `vram/maketextva.c` | — |
| X4 | Rows of the emulated screen that start outside the usable byte-mode ranges are not displayed. After a 3301 RESET the ROM moves the screen to local 0800h, so the text stays blank while the 3301 display is stopped (GAME-A no longer shows a dashed line). | `[VA-TM]` §8.2.1 calls other ranges unusable; blanking them is `[POLICY]` (maintainer choice "a", 2026-10-04) | Approved by the maintainer (2026-10-04, G103f review) | `vram/maketextva.c` | photograph GAME-A after its opening |
| X8 | TSP ACTSCR records the split; CURS moves the CURDEF cursor sprite to the dot position of its row and column (X cells from RXP, Y rows of lineheight from RYP, halved with doubled sprites). Clamping and DPTR0 not modelled. Bug fix, in the ledger. | uPD72022 data book; `[ROM]` positions match the VA2 ROM's own descriptor writes | Approved by the maintainer (2026-10-05, G103h review) | `io/tsp.c` | — |
| X5 | Not modelled: monochrome attribute mode, CURS-positioned cursor. | — | Open | — | — |
| X6 | Semigraphics: colour attribute bit 4 (with bit 3) draws each code as 2×4 blocks, bits 0–3 the left column and bits 4–7 the right column from the top, in the attribute colour; the flag carries with the colour. | `[X88000]`; BNN §8.2.1 does not list semigraphics among the 3301 functions left out | Approved by the maintainer (2026-10-04, G103g review) | `vram/maketextva.c` | — |
| X7 | 40 columns (port 30h 80CM clear) under 3301 emulation: each even byte column is shown twice as wide over the next one. | `[X88000]` skips odd columns; `[DERIVED]` | Approved by the maintainer (2026-10-04, G103g review) | `vram/maketextva.c` | — |

## 6. Graphics

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| G1 | Ports 34h and 35h read back the last value written. | The manuals list them as output only; `[ROM]` the text trap handler saves them with IN and restores them with OUT, which only works if they read back `[DERIVED]` | Approved by the maintainer (2026-10-04, G103d review) | `io/memctrlva.c` | `OUT 35h,95h : IN 35h` from V2 BASIC (`INP(&H35)`) |
| G2 | Port 31h bits PM00, GDEN0 and VW1 drive GrRes (102h) bit 0 and GrMode (100h) bits 15 and 1 in 88 mode. | `[VA-TM]` the bit names coincide; `[ROM]` sets 100h with GDEN0 clear and BASIC only writes 31h `[DERIVED]` | Approved by the maintainer (2026-10-04, G103d review) | `io/memctrlva.c` | — |
| G3 | 10Ch PLTM2 = 0 selects the palette mode from 32h PMODE and 31h PM00. | `[VA-TM]` | Approved by the maintainer (2026-10-04, G103d review) | `io/videova.c` | — |
| G4 | Multiplane 1 bit/pixel: the OR of the planes switched on in 110h is drawn in the text cell's colour (after reverse), in place of a separate graphics screen. | `[VA-TM]` ch. 4 §2.1; colour/reverse handling as `[X88000]` | Approved by the maintainer (2026-10-04, G103d review) | `vram/makegrphva.c`, `vram/scrndrawva.c` | — |
| G5 | With 110h 88MD set and 400 lines, plane 0 shows the upper and plane 1 the lower 200 lines, over twice the frame-buffer height. | `[ROM]` SCREEN 2 leaves the frame buffer at 200 lines and switches on planes 0 and 1; PC-8801 640×400 format `[DERIVED]` | Approved by the maintainer (2026-10-04, G103d review) | `vram/makegrphva.c` | — |
| G6 | In colour mode `COLOR f,b` leaves the background black: the ROM writes the backdrop, but graphics colour 0 is opaque. | `[ROM]` register values; matches `[X88000]` colour mode | Approved by the maintainer (2026-10-04, G103d review) | — | photograph `COLOR 7,4` in SCREEN 0 |
| G7 | Not modelled: 320-dot mode in 1 bit/pixel, GVRAM wait states. | — | Open | — | — |
| G8 | 110h bit 7 (G3MSK) clear keeps plane 3 out of 4 bit/pixel display. Bug fix, in the ledger. | `[VA-TM]` ch. 4 and §8.3.2 | Approved by the maintainer (2026-10-04, G103f review) | `vram/makegrphva.c` | — |
| G9 | Monitor (Emulate > モニタ): SW1 selects 24 kHz or 15 kHz at reset; the digital RGB option thresholds each colour component at a quarter of full scale (8 colours), display only. | `[VA1]` schematic pp. 261, 263 (digital R/G/B = analog drive through 1 kΩ, no conversion logic); `[MEAS]` VA demo almost white on a digital monitor (maintainer); threshold `[POLICY]` | Pending G103i | `io/videova.c`, `vram/scrndrawva.c` | the digital monitor threshold |

## 7. Character ROM

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| K1 | Kanji ports E8h/E9h (level 1) and ECh/EDh (level 2): OUT writes a word address, IN even port = right half, odd port = left half. | `[VA-TM]` | Approved by the maintainer (2026-10-04, G103e review) | `io/cgromva.c` | — |
| K2 | The word address indexes the VA font image directly, with byte-offset bit 14 inverted from offset 8000h (kanji area). | `[ROM]` the extension-ROM BASIC computes `((jis1 & 1Fh) | (jis2 & 60h)) << 9 | (jis2 & 1Fh) << 4`; the VA image's JIS decoder uses the opposite sense of bit 14 `[DERIVED]` | Approved by the maintainer (2026-10-04, G103e review) | `io/cgromva.c` | kanji text in SR-DEMO and N88-日本語BASIC |
| K3 | Level-2 kanji in the 7xxxh JIS rows (stored differently in the VA image) are not converted. | — | Open | — | — |

## 8. Media

| ID | Decision | Basis | Status | Where | Real-machine check |
|---|---|---|---|---|---|
| D1 | D88 table entries at or beyond the first track (lowest entry 0–159 at or above 2A0h) are ignored. Bug fix, in the ledger. | Image format | Approved by the maintainer (2026-10-04, G103e review) | `fdd/fdd_d88.c` | — |
| D2 | `--fdd1-image N` / `--fdd2-image N` choose a disk inside a multi-image D88. The choice is not stored in the configuration or in state files; no GUI control. | `[POLICY]` | Approved by the maintainer (2026-10-04, G103f review) | `fdd/`, `sdl2/` | — |
| D3 | Formatting is refused unless the D88 file holds a single image. | `[POLICY]` (growing a track would move later images) | Approved by the maintainer (2026-10-04, G103f review) | `fdd/fdd_d88.c` | — |

## 9. Not yet covered and open observations

Open observations from the G103d–G103g checks, kept separate from the
gate results (maintainer decision, 2026-10-04); no cause is established:

- GAME-A's spoken title voice plays slightly low in pitch. Candidates to
  check: compatible-mode CPU speed against the real machine (sample
  playback driven by CPU loops) and the FM timer periods.
- The SR-DEMO scene timing differs by a few hundred frames between runs.


- V1 mode: no V1 media available; nothing V1-specific has been run.
- Sound under V1/V2: SR-DEMO drives the OPN through 44h/45h, but audible
  output has only the G103e listening check.
- DD/FD-prefixed trapped I/O and trap timing (T3); text and graphics
  features listed in X5 and G7; level-2 kanji 7xxxh rows (K3).

## 10. Real-machine checks, in order of value

1. `INP(&H34)`, `INP(&H35)` after `OUT` in V2 BASIC (G1).
2. GAME-A screen after its opening: are there stray text rows (X4)?
   (vaeg now shows none.)
3. `INP(&HE2)`, `INP(&HE3)` after writing E3h (M4).
4. `mon` → `x` after cold power-on and after warm reset (C2).
5. `COLOR 7,4` in SCREEN 0: black or green background (G6).
