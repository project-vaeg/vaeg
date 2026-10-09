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
# V1/V2 mode on the PC-88VA: hardware model and implementation plan

Companion to [`upd9002-upd70008-mode.md`](upd9002-upd70008-mode.md)
(the CPU) and ADR-0016 (the milestone series). Provenance tags are those
of §0 of the CPU document. This page answers one question: **what does
the VA do, in hardware and in firmware, to run PC-8801 software — and
which parts does vaeg still lack?**

Every V1/V2 behaviour that rests on derivation or policy rather than a
manual statement or a measurement, with its approval status, is listed in
[`v1v2-mode-decisions.md`](v1v2-mode-decisions.md). PC-8001 (N-BASIC) mode,
a vaeg extension, is described in [`v1v2-n-basic-mode.md`](v1v2-n-basic-mode.md).

## 1. The model in one paragraph

`[VA-TM]` V1/V2 mode is "a mode that emulates the 88M/F series: not only
the CPU but the memory configuration and the display system run in
emulation mode" (BNN technical manual, chapter 8). Three independent
switches make it: the CPU enters Z80 emulation with `BRKEM2 90h`
(§2.2.2, §8 of the CPU document); the **memory system** is switched to
the 88-mode map by port `153H` bit 6 = 0 (§8.2); and the **I/O trap** is
enabled around the session so that a small set of 8801 ports is emulated
by firmware (§9). Everything else — the 8801 I/O ports, the text and
graphics display, the FDD sub-system — is **VA hardware operating in a
compatibility configuration** that the firmware programs in V3 mode
before the handoff. vaeg has the compatible instruction adapter
(M76–M103a); M103b now decodes `BRKEM2` using the existing entry routine.
The rest of this page covers the machine side still needed to boot.

## 2. Memory: the 88-mode window

`[VA-TM]` ch. 8.1. With `153H` bit 6 = 0, physical `10000h`–`1FFFFh`
("88 MODE area") becomes the 8801's 64 KiB space; the Z80 runs with
`PS = DS0 = 1000h`. The window maps "part of the V3-mode main RAM, VRAM
and ROM" so that the 88M/F memory map appears there. `20000h`–`3FFFFh`
is addressable as PC-8801-02N-compatible extension RAM (ERAM pages).
CPU execution mode does not itself select the memory map: `153H` bit 6
is a separate control, written by native firmware before `BRKEM2`.
Mappings outside the window and native trap-handler accesses need to be
checked against the manual rather than inferred from CPU mode.

Inside the window the 8801's own banking applies, through ports the VA
implements **in hardware** (`[VA-TM]` port table, "V1/V2 モード専用"):

| Port | 8801 meaning | VA status |
|---|---|---|
| `31H` OUT | system port 1: `MMODE` RAM/ROM, `RMODE` N88/N80 ROM, 200/400 line, graphics enable | hardware |
| `32H` | `ROMSL` extension-ROM bank, `TMODE`, `GVAM`, `PMODE`, `AVC` | hardware |
| `34H`, `35H` OUT | GVRAM control (ALU / access mode) | hardware |
| `5CH`–`5FH` | GVRAM plane select into `C000h`–`FFFFh` | hardware |
| `70H`, `71H`, `78H` | text window offset, extension ROM, offset increment | hardware |
| `E2H`, `E3H` | extension RAM select | hardware |
| `E4H`, `E6H` OUT | 8214 interrupt level / mask | hardware |
| `E8H`–`EDH` | kanji ROM | hardware |

`[ROM]` The 8801 ROM payloads are already in the VA ROM set: N88-BASIC
(32 KiB), its four extension banks and Debug 8800 live in `varom1.rom`
at `0x10000` and `0x18000`–`0x1FFFF` and in `varom00.rom` (CPU document
§4.4–§4.6, Appendix C). `[UNKNOWN]` Which physical ROM/RAM pages the
window selects for each `31H`/`32H`/`71H` state is not written in the
sources in hand; it is what M103b must derive from the ROM's own window
setup (the handoff block §8, the shared initialiser `0x2210`) and from
8801 behaviour, and verify by running the ROM.

### 2.0 Initial decoder implementation (partial)

M103b now overlays physical `10000h`–`17FFFh` reads with the N88-BASIC
payload at `rom1mem[10000h..17FFFh]` when 153h bit 6 is clear and port
31h MMODE/RMODE (bits 1/2) are both clear. The payload location is backed
by the byte comparison in the CPU document Appendix C. Port 31h OUT is
latched independently of its existing IN handler. MMODE=1 removes this
ROM overlay. Reset clears the new latch; optional state section
`MEM88SYS`, version 0, preserves it without changing `MEMORYVA`.

This is an initial decoder, **not the complete hardware map**. Writes
retain the existing physical main-RAM backing, including under ROM.
`[NEC-GIHO]` Figures 3 and 4 label physical 10000h–1FFFFh as the 88-mode
main RAM, with both 32KiB halves present as main RAM and the ROMs mapped
over the lower half, so this backing matches the manufacturer's
description (§2.3).
RMODE=1 also removes this overlay but does not yet supply the monitor
ROM. ERAM and TVRAM mappings remain unimplemented; independent GVRAM
plane access is described in §2.2.

Extension-ROM selection is now implemented: port `71h` bit 0 is XEROM
(1=disabled, 0=enabled; readback upper bits are ones). When enabled with
N88 ROM selected, physical `16000h`–`17FFFh` reads use
`rom1mem[18000h + (port32 & 3)*2000h + offset]`. The four 8KiB banks use
the existing port 32h latch and its saved state; new `MEM88EXT` version 0
stores XEROM. Reset/old-state default is disabled. MMODE/RMODE still take
precedence. Register meanings come from the manual's 0032h/0071h tables;
the ROM payload layout is documented in the CPU document Appendix C.

Synthetic tests cover all four banks, readback, disable, and word reads
across both extension boundaries. The automatic-FDD integration trace
writes `FEh` to 71h at compatible `456Fh`, then reads the pointer at
`6045h` as `6B55h` through the selected extension bank. The 1800-frame
capture exits 0; it does not establish successful BASIC disk boot. No new decoder is added
to the CPU adapter: byte/word reads go through the common VA decoder,
including split reads at both ROM edges.

Synthetic tests verify ROM read selection, RAM under the overlay,
MMODE switching, restoration of V3 reads, and words crossing physical
0FFFFh/10000h and 17FFFh/18000h. A local automatic-FDD run with
integration-basic now executes `DI; LD SP,E1A0h; JP 3BE5h; IN A,(30h)`
instead of zero bytes. The 600-frame capture completes with exit 0;
this is execution-prefix evidence, not proof of a BASIC prompt or G103b.

### 2.0.1 RAM access window

Port 70h now holds the high byte of the N88 RAM-window origin; OUT 78h
increments it modulo 256 irrespective of the output value. In N88
ROM/RAM mode, compatible 8000h–83FFh accesses map to the 1KiB RAM window
starting at `port70 * 100h`, wrapping the RAM offset at 64KiB. The default
origin is 8000h. All-RAM mode and V3 bypass this window.

`[NEC-GIHO]` Figure 4 places a 1KiB memory window at physical
18000h–183FFh, i.e. compatible 8000h–83FFh, so the window size and
position are no longer only policy. The reset origin 80h remains
implementation policy: neither the VA manual's port table nor the
journal states it. The backing is physical 10000h–1FFFFh as in §2.0. Accesses deliberately
bypass ROM overlays when addressing RAM through this window. This is not
the separate TVRAM mapping in §2.1, nor real-machine validation of VA RAM
aliasing. `MEM88WIN` version 0 stores the offset; reset and missing old
sections use 80h. Full state-file round trips remain pending.

Tests cover offset readback, value-independent increment, FFh→00h wrap,
RAM reads behind ROM, boundary word read/write, all-RAM bypass and V3
bypass. No claim of BASIC boot completion follows from these tests.

### 2.3 Manufacturer description (`[NEC-GIHO]`)

The 1987 NEC Technical Journal article on the PC-88VA (CPU document
reference [25]) gives the system-level picture, consulted through a
maintainer-supplied OCR transcription. Facts relevant to the series:

- In V1/V2 mode, 0–FFFFh RAM and ROM areas 0/1 are as in V3; only
  10000h–1FFFFh becomes the 88-mode area, equivalent to the 8801's 64KiB
  space with the same memory-window and access conditions. Its TVRAM and
  GVRAM are parts of the V3 TVRAM/GVRAM; its monitor ROM, N88-BASIC ROM
  and EROM are parts of ROM areas 0/1. This matches §2.0–§2.2.
- Figure 4 lists, for the 88-mode area: main RAM 32KiB in each half, the
  1KiB window at 18000h–183FFh, TVRAM 4KiB at the top, GVRAM0–2 at 16KiB
  each, monitor ROM 32KiB, N88-BASIC ROM 32KiB, EROM 8KiB×4 (bank
  switched), ERAM 32KiB×12 (pages 0–2, banks 0–3) and option RAM 32KiB as
  ERAM page 3. Boundaries the figure does not give numerically are not
  inferred here.
- **ERAM backing**: pages 0–2 use physical 20000h–7FFFFh (128KiB per
  page, four 32KiB banks each), also directly addressable at those
  addresses; 80000h–9FFFFh doubles as ERAM page 3. The standard machine is
  therefore an 8801 with 384KiB of extension RAM. `[DERIVED]` The likely
  bank address is `20000h + page*20000h + bank*8000h`; the bank order
  inside a page and the `E2h`/`E3h` encoding still need `[VA-TM]` or
  `[ROM]` confirmation before ERAM is implemented.
- The V1/V2 supervisor is firmware in ROM area 1, entered through the I/O
  trap when software executes I/O meant for the µPD3301/µPD8257; it
  decodes and converts the instruction transparently. This matches the
  VA2 handler at `F000:1944` (§3, task file).
- The CPU runs in µPD70008-compatible mode (§4.1 of the article), the
  name this project uses for the main-CPU compatible mode.
- The V1/V2 I/O space is "system area 0" (0000h–00FFh); new VA ports sit
  at 100h and above (Figure 5).
- Keyboard: an intelligent keyboard sends serial data to the sub-CPU
  (7811), which converts it to the 8801 software-sense I/O port data, so
  ports 00h–0Eh are produced by hardware, as `io/serial.c` models.
- Text: the TSP's 3301-compatible mode treats TVRAM as a byte image with
  256 ANK characters and emulates the µPD3301 attributes; it is the V1/V2
  text mode (M103c).

### 2.1 TVRAM in 88 mode

`[VA-TM]` ch. 8.2.1. 4 KiB of the V3 TVRAM — `A6000h`–`A6FFEh` — is
mapped to `1F000h`–`1FFFFh`. The TSP is put into byte-access mode and the
area is used in **µPD3301 VRAM format** (character codes and 8801
attribute codes); the TSP converts the attribute format automatically,
and the 8 text colours become colour codes 8–15. `A0000h`–`AFFFFh` is not
accessible as TVRAM in this mode and DMA cannot reach it. Not emulated by
the TSP: alternate-line display, block-cursor reverse, special control
characters and their interrupts, horizontal-counter clear, light pen,
attribute-less mode, non-transparent monochrome attribute mode. One of
the four split screens shows the 8801 text; its parameters must be set in
V3 mode beforehand (the firmware does this). 40-column mode doubles the
even characters and hides the odd ones; attribute columns are still
counted in 80.

### 2.2 GVRAM in 88 mode

`[VA-TM]` ch. 8.3. Multi-plane mode, screen 0 only, 640 × 200 (15 kHz) or
640 × 400 (24.8 kHz). Each plane is 16 KiB at `1C000h`–`1FFFFh`,
selected by `OUT 5CH`/`5DH`/`5EH` and backed by V3 planes 0–2 at
`A4000h`–`A7FFFh`; plane 3 is unused and must be masked by the screen
switch in 4-bit pixel mode. Frame buffer 0 only; no split, scrolling
allowed. Palette 0 emulates the M/F palette, palette 1 is the 8-colour
direct table; **M/F-format palette writes and the background-colour
register are emulated by the I/O trap** because the register formats
differ (ch. 8.4.2). Composition: backdrop (mode 0), graphics screen 0,
text; sprites unused but not disabled by hardware.

M103b now implements the independent-plane memory path: OUT 5Ch/5Dh/5Eh
selects planes 0/1/2, OUT 5Fh disables selection, and IN 5Ch returns
`F8h | (1 << plane)` or F8h when disabled, per the manual's port table.
In 88 memory mode, physical 1C000h–1FFFFh maps to the existing GVRAM
storage at `plane * 10000h + 4000h + offset`. The path uses direct byte
storage, not compatible ALU operations or an asserted hardware wait-state
model. V3 bypasses it; deselection restores the previous RAM path.

`MEM88GFX` version 0 saves the selected plane (3=disabled); reset and
missing old sections disable it. Existing GVRAM storage remains in its
existing saved region. Synthetic tests verify all three planes are
independent, RAM underneath survives, and word operations cross both
window boundaries correctly. Full state-file tests remain pending.

An 1800-frame integration BASIC capture completes with exit 0, but neither
BASIC boot nor a correction of the earlier repeated-initialization symptom
is established. The possible RAM corruption from formerly ignored plane
selection was a hypothesis, not a demonstrated cause of that symptom.
Rendering, compatible ALU and timing validation remain future work.

## 3. I/O: hardware versus trap

`[VA-WIKI]`/`[ROM]` (CPU document §9) The trap windows are `50h`–`5Bh`
and `60h`–`6Fh`, programmed at reset, enabled (`FFEFh ← 03h`) just before
`BRKEM2` and disabled after `RETEM`. The handler acts on `50h`–`53h`
(µPD3301 CRTC, background/border, screen overlay), `60h`–`68h` (µPD8257
DMAC, which the 8801 uses to feed the CRTC) and `6Eh`–`6Fh`. Everything
else in the 8801 port space is hardware:

| 8801 ports | Device | In vaeg today |
|---|---|---|
| `00h`–`0Eh` | keyboard matrix scan (88M/F compatible interface) | present (`io/serial.c`); V1/V2 guest input remains to be verified |
| `10h` | calendar/printer strobe | present |
| `20h`, `21h` | µPD8251 serial | present |
| `30h`, `31h` IN | DIP switches / system port | present (31h IN only) |
| `31h` OUT, `32h`, `34h`, `35h` | memory mode, extension ROM, GVRAM control | compatibility banking missing; existing port handlers require individual audit |
| `40h` | system port: strobe, VRTC, CMT, beep | present |
| `44h`–`47h` | YM2203/2608 OPN | present |
| `50h`–`53h`, `60h`–`68h`, `6Eh`, `6Fh` | trapped (firmware) | **trap hardware missing** |
| `5Ch`–`5Fh`, `70h`, `71h`, `78h`, `E2h`, `E3h` | banking | **missing** |
| `E4h`, `E6h` | 8214 interrupt controller | **missing** |
| `E8h`–`EDh` | kanji ROM | **missing** |
| `FCh`–`FFh` | µPD8255 to the FDD sub-CPU | present (`io/subsystem.cpp`, `UPD780C`) |

`[DERIVED]` The FDD side needs no new device: the VA's disk sub-system
*is* an 8801-style sub-CPU with its own `disk.rom` (§4.6), and vaeg runs
it already for V3.

## 4. Interrupts in compatible mode

`[VA-TM]` BNN manual §5.2.2 settles this. At reset the interrupt system
is in 8214 mode (port 158H switches to 8259 mode until reset). In 8214
mode a µPD8214-compatible controller is the ICU's slave with eight
levels — INT0 RS-232C receive, INT1 VRTC, INT2 general timer 2 (the
8801-compatible 600 Hz timer, which has no 8259-mode line), INT3/INT5
bus UINT0/UINT1, INT4 sound, INT6 general timer 3 (mouse timer), INT7
SGP — and it selects the vector type from the CPU mode: compatible code
receives a µPD780 vector (00h, 02h, … 0Eh, the Z80 `IM 2` low byte) and
takes an ordinary Z80 interrupt; native code receives V30 vector
40h–47h through the IVT. `E4h` is the current-status register (level in
bits 2–0, bit 3 disables the comparison; initialised with `OUT E4H,7`)
and `E6h` the mask (bit 0 general timer 2, bit 1 VRTC, bit 2 RXRDY,
1 = enable).

The earlier `[DERIVED]` assumption here — that firmware's native
handlers rewrite the saved frame to deliver interrupts into Z80 code —
was wrong and is withdrawn. Implementation is M103c.

## 5. Boot path

`[ROM]`/`[VA-TM]` §8: `000Dh` bit 2 is the active-low **PC key**, not
an independent memory-mode switch. `io/serial.c` already binds keyboard
rows `00h`–`0Eh`; scan code `7Ah` maps to row `0Dh`, bit 2. An unpressed
key reads as one. The VA2 ROM then takes the candidate V1/V2 path at
`F000:136Ch`, but that alone does not guarantee reaching `BRKEM2`.

The next decision is **SW7**, port `0040h` bit 3. The manual defines
zero as boot from the intelligent FDD, and one as do not boot from it.
`io/sysportva.c::sysp_i040()` currently returns zero for this bit. With
zero the ROM calls `1F90h` to try the FDD boot path; with one it skips
that call. Successful IPL loading and subsequent disk-dependent paths
must not be conflated with a direct CPU-mode switch.

A local vaeg diagnostic run with SW7 temporarily forced to one and the
PC key unpressed observed this VA2 ROM sequence (emulator observation,
**not real-machine measurement**):

| Address in segment F000h | Action |
|---|---|
| `13EAh` | keyboard read returns `FFh` at port `000Dh` |
| `136Ah`–`1375h` | PC-key branch and SW7 branch skip the FDD call |
| `1375h` → `18E7h` | write `03h` to `FFEFh`, enabling the I/O trap |
| `1378h`–`138Ch` | probe RAM at physical `80000h`; conditionally write `15Ah` |
| `138Dh`–`139Dh` | clear bit 1 at offsets `0121h`, `0129h`, `0131h` in segment A000h |
| `13A0h`–`13A7h` | read word at port `152h`, clear bit 14, write it back (153h bit 6) |
| `13A8h`–`13AFh` | set DS to `1000h` and ES to zero |
| `13B1h` | execute `0F FE 90`, the unsupported `BRKEM2 90h` entry |

The pre-implementation diagnostic reports `reserved 0f fe` at `13B2h`, the position of the
second opcode byte, not the instruction start. It does **not** establish
successful compatible-mode entry or BASIC execution. The temporary SW7
force and diagnostic prints are not production changes. This bypass
must not be presented as normal V1/V2 disk selection.

### 5.1 Automatic FDD selection, not a mode selector

The maintainer confirms that the real machine automatically selects
V1/V2 from the FDD. The experimental `v1v2_boot` GUI/INI setting added
in `a4d85e64835ab0274731682c92332bb0bab4d368` is therefore withdrawn.
Keep the existing PC-key input and SW7=0, and follow the firmware's disk
path. A forced-SW7 handoff is decoder diagnostics, not a boot gate.

`[ROM]` Static inspection of the VA2 firmware shows why describing
`1F90h` as only a V3 IPL loader was incorrect:

- `1372h` calls `1F90h`; `1FA3h` calls `1FA7h`, with a `RET` at `1FA6h`.
- After the drive-ready exchange, `2011h` starts further FDD commands.
  The helper `20D0h` sends disk-mode command `1Fh`, a read command `02h`,
  and status command `06h`; it shifts returned AH bit 0 into carry.
- Calls with BX=`0001h` (`202Dh`) and BX=`0029h` (`203Fh`) branch on
  carry clear to `2075h`. Calls with BX=`0002h` and BX=`0023h` instead
  branch to `2094h`. Which actual media produce these responses is
  still to be traced; do not label them solely from these constants.
- `2075h` exchanges commands `20h` and `1Fh`, then calls `20F4h` with
  DH=`09h`. `2093h` returns through `1FA6h` to `1375h`, the trap-enable
  and BRKEM2 setup. Thus the FDD path itself can return to the compatible
  handoff without forcing SW7.
- `2094h` instead receives bytes into `3000:0000`, calls `2210h`, then
  far-jumps to `3000:0000` at `20BDh`. Other loader branches also reach
  that native target; entering `1F90h` alone proves neither outcome.

The control-flow findings above were initially static. The following
bounded emulator traces now connect the responses to the branch targets;
none is a successful OS boot. Private media and raw traces remain outside
Git.

### 5.2 Bounded automatic-selection traces

The manual's disk-mode table identifies `01h` as 1D/2D with 256-byte
sectors, `02h` as 1D/2D with 512-byte sectors, `29h` as 1HDs/2HDs, and
`23h` as 1HD/2HD with 1024-byte sectors. The maintainer also confirmed
these meanings. Runtime tests below cover only `01h` and `02h`.

A local worker based on `756e10cde7da58aea198e55015c9953bc2389316`
used temporary, bounded native CS:IP/register prints at the ROM branch
points, plus the existing FDD and compatible-mode traces. No mode override
or ROM modification was used. The FDC used the normal uPD780 firmware
backend, not the `fdsubsys` mock. Two generated, write-protected, zero-filled
2D D88 images had 40 cylinders and two heads: one with 16 sectors of 256
bytes per track, the other with eight sectors of 512 bytes. A working copy
of maintainer-supplied BASIC media was the third input.

| Neutral input | Status AH before `20F1h` shift | Observed branch path |
|---|---|---|
| synthetic-256 | `C0h` on first trial | `2033h` CF=0 → `2075h` → `2093h` → `1375h` → `13B1h` → compatible `1000:0000` |
| synthetic-512 | `81h` on first trial, `C0h` on second | `2033h` CF=1; `203Dh` CF=0 → `2094h` → `20BDh` (far jump to native `3000:0000`) |
| integration-basic | `C0h` on first trial | same branch path as synthetic-256, including compatible `1000:0000` |

At `20F1h`, the ROM shifts AH right once; the branch tests its original
bit 0 through carry, not whether the whole status is zero. `C0h` must
therefore not be described as an error merely because it is nonzero.

Both compatible entries have saved native frame `13B4/F000/F044` and
initial instruction bytes `00/00`. Thus automatic FDD selection reaches
the adapter, but the absent 88-mode ROM mapping prevents BASIC execution.
The temporary native trace code was removed after capture.

All three instrumented runs used a 600-frame screenshot target and a
35-second wall timeout and ended with exit 124. They establish the listed
prefix of execution only; they do not establish full boot, timing, or a
hang cause. Earlier 1800-frame attempts likewise are not acceptance
results. Worker/patch/log identities and raw diagnostic output are kept
in the maintainer-local task directory outside Git.

## 6. What vaeg has and lacks

Status as of M103f (2026-10-04; G103d–G103f pending). Policies behind
these rows are listed in [`v1v2-mode-decisions.md`](v1v2-mode-decisions.md).

| Piece | Status |
|---|---|
| Z80 emulation mode (uPD70008-compatible adapter, R1–R19, CALLN/RETEM, live IVT, alternate set) | done (M76–M103a) |
| `BRKEM2` (`0F FE nn`) | implemented with shared BRKEM entry policy; both encodings pass ROM-less round-trip tests; real-machine equivalence unmeasured |
| `153H` bit 6 memory mode | latched/read back; reset selects V3; optional `MEM88MODE` save section; selects the partial N88 overlay |
| 88-mode window at `10000h`–`1FFFFh` with 8801 banking ports | N88 32KiB overlay, port 31h latch, four extension banks, 70h/78h RAM window, Debug 8800 monitor bank under RMODE (M103c); extended RAM `E2h`/`E3h` and the dictionary ROM window `F0h`/`F1h` (M103e) |
| TVRAM `1F000h` mapping, TSP byte mode and 3301 attribute conversion | implemented (M103c): TMODE-selected window, EMUL, 8Eh/8Fh/97h (`DPLD`/`WDAT`), 3301 row rendering; byte-mode address rule corrected (M103f); semigraphics, 40-column and text after a 3301 RESET beyond local 1000h pending |
| GVRAM plane select `5Ch`–`5Fh` into `1C000h` | independent planes, extended access with ALU and comparison read (`32h`/`34h`/`35h`), display through port 31h, palette mode, colour and 1 bit/pixel display, 640×400 monochrome (M103d); GVRAM wait states pending |
| I/O trap (`FFE0h`–`FFEFh`, vectors `7Ch`/`7Dh`, §9.2 semantics) | registers, native IN/OUT and compatible plain/block IN/OUT interception implemented; DD/FD-prefixed compatible forms and timing pending |
| keyboard matrix interface `00h`–`0Eh` | validated with N-88 BASIC (M103c), including release pacing for synthetic host taps |
| 8214 `E4h`/`E6h`, kanji ROM `E8h`–`EDh` | 8214 mode implemented (M103c, §4); kanji ROM ports implemented (M103e); level-2 rows 7xxxh not converted |
| boot inputs: `000Dh` bit 2 / PC key, `40h` bit 3 / SW7 | PC key present; SW7 reads zero; making it configurable was withdrawn in M103b (V1/V2 is selected from the boot media); automatic FDD selection traced (§5.2); no mode override |
| FDD sub-CPU, OPN, 8251, printer, system ports `30h`/`40h` | present; the sub-CPU interface 8255 is fully cross-wired for the fast transfer protocol (M103b); disk BASIC FILES/SAVE/LOAD/KILL work (M103d); 2HD media and multi-image D88 selection (M103e/M103f); sound under V1/V2 driven but not yet confirmed by ear |

## 7. Milestones (ADR-0016; V2 first)

| Milestone | Scope | Observable step |
|---|---|---|
| M103b | `BRKEM2`; boot selection; `153H` bit 6 and the 88-mode window with the hardware banking ports; I/O trap hardware; ROM-driven derivation of the window map | the VA2 ROM takes the V1/V2 path, hands off with `BRKEM2 90h`, disk BASIC loads and reaches its key-input wait (trace-verified, automated in `vaeg_m103b_basic_boot`; no display yet) |
| M103c | interrupt delivery into Z80 code (ROM analysis), keyboard matrix, TVRAM 88-mode mapping and TSP emulation-mode text rendering | `Ok` prompt displayed, typed characters echoed |
| M103d | GVRAM plane mapping and 88-mode graphics (palette modes, 200/400 lines, backdrop), FDD path under V1/V2, remaining ports (`E8h`–`EDh`, `34h`/`35h`) | **N88-DISK BASIC V2** boots from a disk image and operates; graphics statements draw |
| M103e | compatibility against the **PC-8801mkIISR DEMO** and further PC-8801 software: kanji ROM, extended RAM, dictionary ROM window, D88 track-table fix | the DEMO runs (implemented; G103e pending) |
| M103f | TSP byte-mode address rule, multi-image D88 selection | (implemented; G103f pending) |
| later | V1 mode (needs V1 media), open items in `v1v2-mode-decisions.md` | — |

X88000 (public domain) is the behavioural reference for the 8801 side of
each row — port semantics, µPD3301 and attribute behaviour, keyboard
matrix layout, GVRAM ALU — never a source of the VA-side mechanism.
