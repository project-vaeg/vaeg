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

# PC-8001 (N-BASIC) mode on vaeg

This page describes how vaeg runs PC-8001/PC-8801 N-BASIC in the PC-88VA's
V1/V2 mode, why that needs vaeg extensions, what the VA ROMs do on the way,
and what remains open. Companion documents:
[`v1v2-mode-plan.md`](v1v2-mode-plan.md) (hardware model),
[`v1v2-mode-decisions.md`](v1v2-mode-decisions.md) (decision register, items
C4, C6, C7, C8), task
[`M103h`](../agents/tasks/M103h_v1v2_nbasic.md).

Tags as in the decision register: `[VA-TM]` VA technical manuals, `[ROM]`
VA ROM disassembly or traces, `[X88000]` PC-8801 behaviour as implemented by
X88000 1.5.3, `[MEAS]` real machine, `[DERIVED]` concluded, `[POLICY]`
implementation choice.

## 1. Summary

- The PC-88VA has no N-BASIC ROM and no N mode. `[ROM]` The banner
  "NEC PC-8001 BASIC" appears in none of the VA ROM images
  ([`upd9002-upd70008-mode.md`](upd9002-upd70008-mode.md) Appendix C.6).
- `[MEAS]` (maintainer; I/O magazine, November 1987, "88VA plus N-BASIC")
  N-BASIC runs on a real VA when its ROM image is copied to RAM and run in
  all-RAM mode; the published loader patches N-BASIC's port 31h writes so
  that it never leaves all-RAM mode.
- vaeg instead adds the PC-8801's N-mode ROM switch to its 88-mode memory
  map, so N-BASIC and N-88 BASIC run **unmodified**: no ROM byte is patched.
  The N-BASIC ROM is user-supplied and never part of the repository.
- Status: `NEW ON 1` from V1 S N-88 BASIC starts N-BASIC, and the N
  setting boots straight into N-BASIC without a disk on both models
  (implemented, G103h pending).

## 2. Files and settings

| Item | Where | Notes |
|---|---|---|
| N-BASIC ROM | `n80.1.8.rom`, `n80.1.2.rom`, `n80.rom` or `N80.ROM`, 32 KiB, in the ROM directory | Optional; loaded at every reset: the file chosen in the menu (`V1V2_N80ROM`), else the first found in that order. The versioned names are known dumps (below); `n80.rom` is accepted whatever it holds. About shows the loaded file and its identity. |
| Z80 mode | Emulate > Z80 mode: V2 H, V2 S, V1 H, V1 S, and one N (PC-8001) entry per usable N-BASIC ROM file, labelled with its identity | V1/V2 is the VA memory switch B1FC5h bit 0 (backup memory); H/S is `V1V2_Standard` and N is `V1V2_NMode` in the configuration file. N is available only with an N80 ROM. Selecting an entry resets. About shows `Z80 MODE`. |

Known dumps, registered by SHA-1 at the maintainer's request
(`sdl2/n80rom.c`; other files are shown as "unknown dump"):

| Identity | SHA-1 |
|---|---|
| N-BASIC 1.2 | `063609dd518c124a4fc9ba35d1bae35771666a34` |
| N-BASIC 1.8 | `06dae1db384aa29d81c5b6ed587877e7128fcb35` |

A PC-8801 N80 ROM is 32 KiB: N-BASIC at 0000h–5FFFh and the Debug 8800
monitor bank at 6000h–7FFFh. `[ROM]` In the image tested (PC-8001 BASIC
Ver 1.8) the top 8 KiB is byte-identical to the VA's own Debug 8800 bank
(`varom00` offset 1E000h).

## 3. What the hardware and ROMs do

### 3.1 Memory map

- `[VA-TM]` Port 31h (OUT, V1/V2 only): bit 2 RMODE (0 N-88 ROM, 1
  "monitor ROM"), bit 1 MMODE (0 ROM/RAM, 1 all RAM).
- `[X88000]` On a PC-8801, RMODE (the N/N-88 switch) selects the N-BASIC ROM
  at 0000h–7FFFh.
- `[ROM]` On the VA, N-88 BASIC's MON sets RMODE and keeps executing below
  6000h, so the real VA switches only 6000h–7FFFh to Debug 8800 (decision
  M3).
- vaeg (decision C7): with an N80 ROM present, RMODE in 88 mode with MMODE
  clear shows the whole ROM at 0000h–7FFFh; writes go to the RAM below, as
  with the N-88 ROM. Without the file the VA behaviour (M3) is unchanged.
  Because the ROM's top 8 KiB is the same Debug 8800 bank, N-88 BASIC's MON
  behaves the same either way.

### 3.2 `NEW ON 1`

- `[ROM]` N-88 BASIC's `NEW ON` is in extension ROM bank 2 (77A7h). Odd
  arguments call 39C2h, which reads IN 31h and needs bit 7 = 1 (V1) and
  bit 6 = 0 (standard speed); otherwise error 33, "Feature not available".
  On success it builds `OUT (31h),A / JP (HL)` at 847Ah with RMODE set and
  jumps to 0000h, the N-BASIC entry (`DI / LD SP,FFFFh / JP 003Bh`).
- `[VA-TM]` IN 31h reads memory switch B1FC6h: bit 7 MS27 (V1/V2), bit 6
  MS26 "unused, always 1". `[X88000]` On a PC-8801 bit 6 reports high speed.
  So on a real VA `NEW ON 1` always fails.
- vaeg (decision C6): under V1 S, IN 31h clears bit 6 in 88 mode. CPU timing
  is not changed.

### 3.3 V1/V2 and H/S selection

- `[ROM]` The VA2 ROM writes memory switch B1FC5h to port 1C6h at reset
  (F000:12E4); port 150h then reports the mode (active low: FFFEh V1, FFFDh
  V2), and the ROM recomputes MS27 from it (F000:14BC). The original VA's
  ROM reads 150h (F000:0A77) but never writes 1C6h; vaeg starts 150h from
  B1FC5h bit 0 at reset (decision C8, ledger entry).
- `[ROM]` Before that the ROM checks the record's checksum B1FCDh after
  setting B1FC6h bit 7 (F000:23B7). vaeg's checksum had not followed that
  rule, so saved settings were restored to defaults at each boot (ledger
  entry, fixed in M103h).

### 3.4 Text display in standard speed

- In S mode N-88 BASIC disables the fast TVRAM (port 32h TMODE set) and keeps
  its text in main RAM F000h–FFFFh, which the PC-8801's CRTC DMA reads. The
  VA always runs as H and never shows text from there.
- vaeg (decision C6): with TMODE set in 88 mode, the TSP 3301 emulation reads
  the 4 KiB text window from 88-mode main RAM. After `NEW ON 1` port 32h stays
  as N-88 BASIC left it, so N-BASIC's text, written to the same area, is shown
  the same way.

## 4. Starting N-BASIC with `NEW ON 1` (implemented)

1. Put the N80 ROM in the ROM directory; check About (`ROM(N80): exist`).
2. Emulate > Z80 mode > V1 S (resets).
3. Boot V1/V2 N-88 BASIC from a V1/V2 disk (the VA ROM enters V1/V2 only
   from such media on the VA2).
4. Eject the disk from drive 1, then type `NEW ON 1`.

`[ROM]` N-BASIC probes the disk subsystem at start-up and, if drive 1 holds
a disk, reads track 0 sector 1 and runs it. An N-88 system disk's boot code
then takes over (black screen or an apparent reset). With drive 1 empty,
N-BASIC prints "NEC PC-8001 BASIC Ver 1.8" and `Ok`, and runs programs.
`[DERIVED]` A PC-8801 in N mode boots a disk in drive 1 the same way.

`[ROM]` N-BASIC writes code FCh into the text RAM right after the `Ok`
prompt (3C7Dh) and later searches for it to find the start of the input
line (1EADh). In the VA, VA2 and PC-8801 fonts FCh is blank.

## 5. Implementation (no ROM patches)

| Change | Files | Commit |
|---|---|---|
| Memory-switch checksum as the ROM checks it | `io/bkupmemva.c` | [d7257d77](https://github.com/nakatamaho/vaeg/commit/d7257d77122fa42f857c1ab8eec321bf3bae73cb) |
| Z80 mode menu, IN 31h bit 6 under S | `sdl2/gui/gui.cpp`, `io/memctrlva.c`, `machine/pccore.*`, `sdl2/ini.c` | [6c82bb8d](https://github.com/nakatamaho/vaeg/commit/6c82bb8d56bdbb2a03b4f7ba29ebc485412e50d9) |
| 3301 text from main RAM under TMODE | `vram/maketextva.c` | [0bc3b87f](https://github.com/nakatamaho/vaeg/commit/0bc3b87f30e64a1a577b0cb9eb34839b80f60951) |
| N80 ROM under RMODE | `bios/romva.c`, `memoryva/memoryva.*` | [81924227](https://github.com/nakatamaho/vaeg/commit/8192422754d36801919c267e4c598c1f68ab2d00) |
| Port 150h at reset (original VA) | `io/sysportva.c` | [827a3a67](https://github.com/nakatamaho/vaeg/commit/827a3a6799a0868ad80a6a5fc3b844c750d0e9df) |
| About: ROM(N80), Z80 MODE | `generic/np2info.c`, `sdl2/gui/gui.cpp` | [3d3eadef](https://github.com/nakatamaho/vaeg/commit/3d3eadefcce5190fdaa901ea8ab20e59956cecf9) |
| Versioned ROM names, `n80.rom` fallback | `bios/romva.*`, `memoryva/memoryva.*`, `sdl2/ini.c` | [ff8901e0](https://github.com/nakatamaho/vaeg/commit/ff8901e0a2d37027c019ad1b226bc17ec91f37c5) |
| SHA-1 identities, menu list | `sdl2/n80rom.*`, `sdl2/gui/gui.cpp`, `generic/np2info.c` | [0f8b0078](https://github.com/nakatamaho/vaeg/commit/0f8b0078a77fae621fd1956eb26c45492e0abb92) |

Romless tests (`vaeg_romless_tests`): the memory-switch checksum and V1/V2
selection, IN 31h under H/S, port 150h after reset, the N80 mapping with and
without RMODE/MMODE, About's entries, and 3301 text from main RAM.

## 6. Booting directly in N mode (implemented, decision C9)

On a PC-8801 the N/N-88 switch makes the machine start in N-BASIC without a
disk. vaeg's N setting does the same with the unmodified VA ROMs:

1. **RMODE at hand-off.** RMODE is set right after `romva_initialize` loads
   the ROMs (the I/O reset runs before the N80 ROM is known), so the VA ROM's
   `BRKEM2 90h` hand-off to 1000:0000 starts N-BASIC.
2. **Entering V1/V2 without a disk.** `[ROM]` The original VA falls back to
   V1/V2 after its V3 boot search fails, also with no disk. The VA2 shows
   "insert the correct disk" instead; with port 40h bit 3 reading 1 (the
   original VA's SW7 OFF, "skip the V3 search") it enters V1/V2. vaeg reports
   that bit only in N mode; M103b's withdrawn SW7 setting stays withdrawn.
3. **Text display.** `[ROM]` Both VA ROMs start V1/V2 with port 148h bit 7
   set (text off; shadow at 0040:00E8h, written at F000:14FE on VA2) and
   clear it only when compatible code writes port 53h with bit 0 clear (trap
   handler F000:1D9D on VA2, 1459h on VA). N-88 BASIC writes 53h; N-BASIC,
   written for the PC-8001, never does, while a PC-8801 resets 53h to 0.
   At the first compatible-mode entry vaeg clears bit 7 in both places once,
   the PC-8801 reset state (maintainer-approved option A). No ROM code is
   changed; the shadow address is the same in both ROMs.

Without the N80 ROM the setting has no effect. With a disk in drive 1,
N-BASIC boots it, as on a PC-8801. `[ROM]` N-BASIC reads port 40h bit 3 as
"no disk unit" (00CBh) and then skips the disk boot, so vaeg reports the
bit only until the first compatible-mode entry (fixed in M104,
[39e9df39](https://github.com/nakatamaho/vaeg/commit/39e9df3919902cd378851f710506896067a18bce); before
that, N mode never booted a disk).

| Change | Files | Commit |
|---|---|---|
| N-mode start | `io/memctrlva.*`, `io/sysportva.c`, `machine/pccore.*`, `cpu/upd9002_upd70008.cpp`, `sdl2/ini.c` | [c4827130](https://github.com/nakatamaho/vaeg/commit/c4827130c885ea3416c719734aa9e9d76e45377f) |
| Menu and About | `sdl2/gui/gui.cpp`, `generic/np2info.c` | [cbc24e5a](https://github.com/nakatamaho/vaeg/commit/cbc24e5a02eadbfc08d7f5a276a4b7b27f29bb4e) |

Related fixes found with N mode: the original VA's ROM positions the cursor
with TSP CURS, which vaeg now implements (decision X8, ledger), and Copy
screen text reads the 3301 text rows.

## 7. Open items

- Resolved: the maintainer's "\\" after `Ok` is N-BASIC's FCh marker drawn
  with the PC-98 font (`98font.rom`), whose FCh glyph is a backslash. The VA
  font, like the PC-8801 font, has a blank FCh. Reproduced headless with
  that font setting; the maintainer confirmed the mark disappears with the
  VA font. Not a defect.
- The function-key row shows control-code pictures (for example HT and CR);
  the PC-8801 font has the same pictures at 01h–1Fh, so this is likely
  correct, but unconfirmed.
- N80SR and the N80 (PC-8001mkII) mode are not pursued (§8).

## 8. N80SR and N80 modes: not pursued

Maintainer decision (2026-10-05, M103i): vaeg does not add N80SR (N80 V2,
PC-8001mkIISR) or the N80 (PC-8001mkII) mode, because neither display can
be reached on a real VA, even with a custom I/O-trap handler. The study
below is a desk study; nothing in it was run on hardware or in vaeg.
Hiragana display was excluded from the start.

Display models, from `[X88000]` 1.5.3 behaviour:

- **N80SR 320×200**: two 8-colour pages in the B/R/G planes, page 0 at
  plane addresses 0000h–1FFFh and page 1 at 2000h–3FFFh, 40 bytes a line.
  Port 33h bit 2 selects the front page; colour 0 of the front page shows
  the back page; port 53h bits 1 and 2 turn each page off.
- **N80 320×200**: four colours in plane 0 alone, two adjacent bits per
  dot. 640×200 is plane 0 at one bit per dot.
- Both modes give port 31h their own meaning (320-dot mode, colour,
  palette, and the extra 8 KiB ROM bank) and switch an 8 KiB ROM at
  6000h–7FFFh with port 71h bit 0.

What a custom trap handler on a real VA could do:

- `[VA-WIKI]`/`[SRC]` Vectors 7Ch/7Dh are ordinary IVT entries and the
  handler runs native code, so a loader can install its own handler,
  chain unhandled ports to the ROM's (VA2 F000:1944), and program the VA's
  own display registers. Compatible code can reach native code with
  `CALLN`.
- `[VA-WIKI]` The hardware has two trap ranges, both used by the ROM
  (50h–5Bh, 60h–6Fh). Covering 31h, 33h and 71h as well means ranges such
  as 31h–33h and 50h–71h; the second traps the GVRAM plane-select ports
  5Ch–5Fh, and trapped I/O runs tens to hundreds of times slower.
- The N80SR/N80 ROMs would run from RAM in all-RAM mode, as N-BASIC did
  in the I/O 1987-11 article, with OUT 31h absorbed by the handler.
  Switching the extra 8 KiB ROM would mean copying 8 KiB on each OUT 71h.

What it could not do:

- `[VA-TM]` §4.5.2: multiplane mode supports only graphics screen 0;
  two graphics screens exist only in single-plane mode, whose pixel
  format differs from the 88 planes. **N80SR's page overlay is therefore
  impossible**; one page at a time could be shown as a 320-dot, 3-bit
  multiplane screen (102h bit 4, 110h G3MSK) with its start address on
  the selected page `[DERIVED]`, untested in 88 mode.
- `[VA-TM]` The VA displays multiplane 1, 3 or 4 bits and single-plane
  1, 4, 8 or 16 bits per pixel; **no format packs two bits per dot in one
  plane**, so the N80 320×200 four-colour screen cannot be shown. Its
  640×200 two-colour screen could be.
- GVRAM writes are memory accesses and cannot be trapped, so neither
  screen can be converted in software.

Text and BASIC would likely run in both modes; the graphics would not.
