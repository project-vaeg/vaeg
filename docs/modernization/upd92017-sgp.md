<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
---
title: "PC-88VA uPD92017 Super Graphics Processor Specification Reconstruction"
short_title: "PC-88VA uPD92017 SGP Reconstruction"
filename: "docs/modernization/upd92017-sgp.md"
document_status: "Reconstructed, non-authoritative specification"
language: "en"
version: "0.4-en.2"
date: "2026-08-23"
target_system: "NEC PC-88VA family"
device_identifier: "uPD92017; original-VA package marking D92017-002"
later_device_identifier: "uPD92046; VA2 package marking D92046GD-001"
---

# PC-88VA uPD92017 Super Graphics Processor Specification Reconstruction

> **Status:** Reconstructed, unofficial specification
> **Target:** PC-88VA Super Graphics Processor (SGP)
> **Original-VA device:** `μPD92017`, package marking `D92017-002`, schematic
> reference IC75 and label `VDP`
> **VA2 successor:** `μPD92046`, package marking `D92046GD-001`
> **Corrected identification:** The original-VA circuit diagrams place the CPU
> slave interface, main-memory bus-master interface, GVRAM master interface,
> completion interrupt, DMA, ready, and wait signals on IC75. This uniquely
> matches the software-visible SGP. IC76 `D65200GD-054` is a separate `GAL-3`
> GVRAM sequencer; it is not the SGP.

## 0. Purpose and interpretation

This document reconstructs the software-visible PC-88VA SGP specification by
comparing surviving technical documentation, community research, and emulator
implementations.

It covers CPU I/O, the private 4MiB address space, in-memory command streams,
the twelve identifiable opcodes, transfers, Boolean raster operations,
transparency, line drawing, clearing, scan/paint assistance, termination,
interrupts, abort, contention, model differences, and hardware questions.

It is not a semiconductor data sheet. The original-VA schematic now identifies
the device and its external bus roles, but internal microarchitecture,
electrical limits, process, and an authoritative command-cycle table have not
been recovered.

### 0.1 Evidence labels

| Label | Meaning |
|---|---|
| **[CONFIRMED]** | Explicit in period technical material and consistent with another source or implementation |
| **[DOCUMENTED]** | Explicit in a period source but not independently verified on hardware |
| **[IMPLEMENTATION]** | Present in vaeg or MAME; not automatically hardware behavior |
| **[INFERENCE]** | Reasoned from multiple pieces of evidence |
| **[CONFLICT]** | Sources, or source and implementation, disagree |
| **[UNKNOWN]** | Insufficient evidence; primary documentation or hardware is required |
| **[CAPTURED]** | Retained from an earlier source capture that could not be fetched again |

Confirmed and documented behavior should be implemented first. Conflicts and
unknowns should remain visible through tests, model profiles, or deliberately
named compatibility choices.

## 1. Sources and evidence hierarchy

### 1.1 Period and near-primary documentation

**[TM] BNN, *PC-88VA Technical Manual***

- [Internet Archive item](https://archive.org/details/PC88VA)
- [User-specified viewing position](https://archive.org/details/PC88VA/page/76/mode/2up)
- BNN Corporation, first edition, 25 June 1987
- ISBN-10 **4-89369-024-8**

The check digit is valid; no DOI is expected. OCR and extracted text broadly
agree, but bit positions derived from diagrams remain vulnerable to OCR error.

**[SCH] Original-VA full-circuit-diagram ownership audit, 18 July 2026**

- The maintainer read the complete circuit-diagram section of [TM] and checked
  each page title, reference designator, printed part-number label, and signal
  group against repeated OCR.
- IC75 is marked `D92017-002`, labelled `VDP`, and carries the CPU-slave,
  main-memory bus-master, GVRAM-master, `VINT`, `VOPRDY`, `VODMA`, and `WAIT`
  interfaces required by the software-visible SGP.
- IC76 is independently present on the same diagrams as `D65200GD-054`,
  labelled `GAL-3`, and carries GVRAM sequencing, serial clocks, display
  transfer, and arbitration signals.
- The identification is therefore based on distinct devices and net roles,
  not on similarity between an emulator model and an isolated OCR token.

The scan quality leaves the middle digits of IC80's `D65070GD-084` marking
slightly uncertain and leaves IC82's marking unresolved. Neither ambiguity
affects the IC75/IC76 separation. Exact schematic page and full-pin
transcriptions remain desirable for independent review.

**[VC] *PC-88VA Technical Manual, 88VA Users Club Edition, beta 1.0,
1992-01-20***

- historical [distribution page](http://www.iris.dti.ne.jp/~nano/88va/tekumani.html)
- canonical archive name: `tekumani.lzh`

The investigation copy was a `tar.xz` repackaging of extracted files. That
wrapper is not the canonical historical distribution, and the original LZH
SHA-256 has not been verified. A raw extracted-member hash and a UTF-8
converted-text hash identify different byte sequences and must not be
misrepresented as the original archive hash.

| File | Subject |
|---|---|
| `TEKUMANI.DOC` | Provenance, target systems, cautions, editorial status |
| `2.TXT` | I/O map, SGP control ports, interrupt table |
| `3.TXT` | Memory wait states and TVRAM/character-generator contention |
| `4.TXT` | Display system and SGP commands |

The Users Club text is an independently researched beta manual, not an official
NEC publication. Actual hardware takes precedence.

### 1.2 Emulator implementations

vaeg evidence:

- [repository](https://github.com/nakatamaho/vaeg)
- [`io/sgp.c`](../../io/sgp.c)
- [`io/sgp.h`](../../io/sgp.h)

The local implementation contains BITBLT, PATBLT, LINE, CLS, SCAN_RIGHT,
SCAN_LEFT, dispatch, interrupts, and memory access. Kanji handlers and some
memory regions are incomplete; timing and contention contain provisional
constants; and later-model descriptor widths are reconstruction evidence.

MAME evidence:

- [`pc88va.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp)
- [`pc88va_sgp.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_sgp.cpp)
- [`pc88va_sgp.h`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_sgp.h)

MAME confirms integration points such as I/O and device address mapping, but
its existence does not establish every command semantic.

### 1.3 Inside PC-88VA Wiki

- [root](http://www.pc88.gr.jp/inside88va/wiki/)
- [FrontPage](http://www.pc88.gr.jp/inside88va/wiki/index.php?FrontPage)

The Wiki is a separate community source. During this revision, relevant pages
could not be fetched reliably. Earlier-captured statements are marked
**[CAPTURED]** and cannot alone establish normative behavior.

One captured claim assigns 4MHz to the original VA SGP and 8MHz to VA2/VA3.
The repository implements nominal 3.9936MHz and 7.9872MHz defaults with the
same 2:1 ratio. The absolute hardware clock remains provisional until tied to
a recoverable page or primary source.

### 1.4 Default evidence order

1. NEC primary source or repeatable real-hardware measurement;
2. BNN technical manual [TM];
3. Users Club investigation [VC];
4. implementation comments explicitly reporting hardware comparison;
5. observable vaeg or MAME behavior;
6. inference in this reconstruction.

Agreement between [TM] and [VC] is useful but not necessarily independent.

## 2. Hardware identity, role, and display-system boundaries

**[CONFIRMED]** The original PC-88VA SGP is IC75, `D92017-002`. The circuit
diagrams label it `VDP`; the software documentation calls the programming
interface `SGP`. Its interfaces identify those names as two views of the same
device. The SGP is a memory-oriented drawing processor, distinct from the
display-timing controller, and provides rectangular transfer, repeated
patterns, 16 Boolean ROPs, packed 1/4/8/16-bpp pixels, one-bit color expansion,
line drawing, range fill, scan assistance, command-list execution, and END
interrupts.

~~~text
Main CPU
    |
    | ports 0500h..0507h
    v
+------------------------------------------+
| uPD92017 / D92017-002                    |
| schematic label: VDP                    |
| software-visible role: SGP              |<--- command list in main RAM
| command fetch, descriptors, ROP, drawing|
+------------------------------------------+
    |                    |
    | main-memory master | GVRAM master
    v                    v
Main/expansion RAM       shared GVRAM bus
                             ^
                             |
                   D65200GD-054 GAL-3
                   display transfer/sequencing
~~~

**[DOCUMENTED]** The SGP is usable only in single-plane graphics mode. Starting
it in multi-plane mode is **[UNKNOWN]** and should be diagnosable rather than
silently assigned invented behavior.

### 2.1 Original-VA custom-LSI ownership

The full schematic audit gives the following ownership map. Part numbers are
recorded as printed package patterns; `μPD` family names are used only where
the identity is established.

| Ref | Package marking | Diagram name | Function established by signal groups |
|---|---|---|---|
| IC83 | `μPD9002` | CPU/interrupt area | CPU; 15.9744MHz crystal divided to 8MHz, i8259A slave, and SA/SD bus generation through LS373/ALS245 |
| IC77 | `D65042GD-093` | `GAL-1` | Memory/bus controller; MA, DRAM RAS/CAS, ROM/dictionary banking, ready bundle, and `X8V3` |
| IC74 | `D65101GD-055` | `GAL-2` | Video composition/output; combines GVRAM serial data, IDP `TXVD`, expansion video, and GPD before RGB latches and three D6901C DACs |
| IC76 | `D65200GD-054` | `GAL-3` | GVRAM sequencer; GA, chip select, RAS/CAS/WE/DTOE, serial clocks, display transfer, and drawing/display arbitration |
| IC75 | `D92017-002` | `VDP` | **SGP**; CPU slave, main-memory bus master, GVRAM master, interrupt, DMA, ready, and wait |
| IC79 | `D72022G` | GDC/IDP | Text/sprite processor (TSP/IDP), `VAD`, `TXVD`, and sync interfaces |
| IC80 | `D65070GD-084` (OCR caveat) | IDP attribute control | 64KiB TVRAM control, attribute expansion, and character-ROM address generation |
| IC81 | `D65012GF-042` | IDP pipeline | Later text-pipeline serialization using VAD/TDB paths |
| IC78 | `PCZ80-27` | V1/V2-mode emulator | PC-8801 compatibility I/O/peripheral emulation, keyboard paths, mode/reset, and legacy port decode |
| IC14 | `D65013GF-124` | FDD interface/mode switch | Z80 disk-subsystem to main-system bus bridge, DMA, and interrupt paths |
| IC1 | `D65006GF-067` | CPU clock/FDD area | Disk-side memory control, FDD signals, and dual-oscillator clock generation |
| IC82 | `SLA6050C0R` (uncertain OCR) | Digital-RGB page | Keyboard receive paths; vendor/part identity remains unresolved |

Three OCR passes read IC80 as `65070`, but the scan cannot fully exclude
`65010`. IC82's marking does not fit the surrounding NEC numbering and may be
an OCR error. These caveats do not touch IC75 or IC76.

The generic devices corroborate the partitioning: a `μPD780C-1`, `μPD765A`,
`μPD71066`, two D41416 DRAMs, and D23C64 form the intelligent FDD subsystem.
The remaining support logic includes two 8255s, 8251, 8259A, keyboard
`μPD7811CW-655`, YM2203/YM3014 sound, MC1377 video encoding, `μPD4990`, and
three D6901C video DACs.

### 2.2 Why D92017-002 is the SGP

The IC75 pin profile matches every integration property of the documented SGP:

| Interface | IC75 signal groups | SGP implication |
|---|---|---|
| CPU slave | `AD0-15`, `BS0-2`, `A16-19` | CPU programming and status interface |
| Main-memory master | `MAD0-15`, `MA16-19`, `XMADH`, `XMADL`, `MHDIR` | Command fetch and BITBLT access beyond the CPU's current logical window; physical support for the documented 22-bit space |
| GVRAM master | `GDB0-15`, `GA00-17`, `XCSW0-7`, RAS/CAS/WE/DTOE | Direct packed-pixel drawing and read-modify-write access |
| Flow/interrupt | `VINT`, `VOPRDY`, `VODMA`, `WAIT` | Completion IRQ 8, DMA/bus ownership, readiness, and CPU stall paths |

No other original-VA device has this combination. In particular, IC76
`D65200GD-054` is visible at the same time as IC75 and has a different role:
`GA00-17`, `XCSW0-7`, `XGRAS`, `XGCAS`, `XGWE0-3`, `XDTOE0-3`, `SCLK0-7`,
`ILCLK`, `XSEN`, `XSOE`, `RSM`, `WAM`, and `SYNCM` form the GVRAM display
sequencer and arbitration block.

IC75 and IC76 both drive or observe GA/XCSW because the drawing master and
display-refresh sequencer share the GVRAM bus. Shared nets are evidence of
arbitration, not evidence that `D65200` implements the drawing engine. GAL-1's
`TRDY`, `VRDY`, `IRDY`, `ERDY`, and `NSPD` bundle is the physical place to
trace wait insertion. Whether `VRDY` is driven directly from the IC75 ready
path still requires a complete pin-to-net transcription.

### 2.3 Original VA to VA2 correspondence

| Function | Original VA | VA2 schematic |
|---|---|---|
| Memory/bus control | `D65042GD-093` | `D65042GD-246`, redesigned for the no-wait model |
| Video composition/output | `D65101GD-055` | `D65101GD-055`, same package pattern |
| GVRAM sequencer | `D65200GD-054` | `D65200GD-076` |
| VDP/SGP | `D92017-002` | `D92046GD-001` |
| IDP | `D72022G` | `D72022GF` |
| TVRAM/attribute control | `D65070GD-084` plus `D65012GF-042` | Daughter-board `D65101GD-107`; correspondence is coarse after the 256KiB TVRAM redesign |
| FDD interface | `D65013GF-124` plus `D65006GF-067` | Apparently integrated into `D65101GD-110` |
| V1/V2-mode emulator | `PCZ80-27` | Outside the available VA2 diagram scope |
| VA2-only block | none | `D92044GD-001`; likely expansion-slot/bus control, not yet established |

This lineage independently separates the 92-series VDP/SGP from the 65200
GAL-3 family: both devices advance to distinct VA2 patterns. It also explains
why a VA2 diagram can show `D92046GD-001` generating GVRAM addresses while a
65200-series device remains present on the shared bus.

### 2.4 Resolution ownership

The complete cross-block register map and worked mode relationships are in
[PC-88VA Video-Mode and Framebuffer Control](pc88va-video-modes.md). This SGP
document retains only the boundary needed to describe drawing behavior.

The display blocks have separate responsibilities:

| Block | Responsibility |
|---|---|
| `μPD92017` VDP/SGP | Where pixels are drawn and which framebuffer pitch is used |
| `D65200GD-054` GAL-3 | GVRAM display transfer, serial clocks, sequencing, and drawing/display arbitration |
| Port `0102h` | Native 640/320 horizontal selection and pixel format |
| Port `0100h` | 200/204/400/408 vertical and scan selection |
| `μPD72022` TSP/IDP `SYNC` | Text/sprite processing and actual sync, blanking, and active intervals |
| `D65101GD-055` GAL-2 | Compose TSP, G0, G1, and expansion video in the common 640-dot coordinate system |

The generic μPD72022 can produce 256x192 or other examples by changing
`SYNC.RS`, `HAD`, and `VAD`. That does not establish an official PC-88VA
256x192 mode composited with GVRAM. A TSP-only 256-dot width risks disagreement
with graphics fetch, sprite coordinates, and composition boundaries.

### 2.5 Arbitrary sizes are viewports

An arbitrary logical size such as 384x256 should first be implemented as a
viewport in a documented physical raster:

~~~text
physical raster      640 x 400
graphics source rows 640 dots wide
content rectangle    384 x 256
display position     (128, 72)
outside area         backdrop or transparent
~~~

For the safe first test, retain a 640-dot graphics-source pitch, place the
384-dot content at X=128 in each source row, use `DSH=256` and `DSP=72`, and
keep TSP timing at known 640x400 values. `OFX` changes the source position and
is not a proven destination-X placement control. A tightly packed 384-dot
pitch is a later hardware discriminator, not a documented native fetch width.

### 2.6 Safe hardware-test order

Do not begin with arbitrary `SYNC`: an out-of-range CRT signal is unsafe and
makes failures ambiguous.

1. Establish known 640x400/24.8kHz with a one-dot grid.
2. Change only documented vertical controls for 640x200.
3. Select `0102h.HW0=1` for 320x200 and verify that logical pixels become two
   physical dots wide.
4. Change `FBW` independently; determine whether line stepping still uses the
   old 640-dot pitch.
5. Try candidate 320x400 to test horizontal/vertical independence; do not call
   it an official mode without hardware evidence.
6. Test a 384x256 viewport while retaining 640x400 timing.

The first implementation target is 320x200 as a logical screen enlarged 2x2
inside 640x400 output. Success separates pitch, graphics readout, horizontal
and vertical scaling, TSP timing, and final composition.

## 3. SGP address space

### 3.1 Width and alignment

**[CONFIRMED]** The SGP uses a private 22-bit, 4MiB space:

~~~text
000000h .. 3FFFFFh
~~~

Command addresses are SGP physical addresses, not CPU logical or bank-window
offsets. Most data is 16-bit and starts at even addresses.

### 3.2 Reconstructed original-VA map

| SGP address | Size | Resource | Notes |
|---:|---:|---|---|
| `000000h-09FFFFh` | 640KiB | Main RAM | CPU and SGP physical addresses coincide |
| `0A0000h-0FFFFFh` | 384KiB | Expansion memory | Population/banking dependent |
| `100000h-13FFFFh` | 256KiB | Kanji ROM 1 | Normally read-only source |
| `140000h-17FFFFh` | 256KiB | Kanji ROM 2/later overlay | Model dependent |
| `180000h-18FFFFh` | 64KiB | Original-VA TVRAM | Display contention |
| `190000h-1FFFFFh` | 448KiB | Reserved | Behavior unknown |
| `200000h-23FFFFh` | 256KiB | GVRAM | Principal packed drawing destination |
| `240000h-3FFFFFh` | 1792KiB | Reserved | Behavior unknown |

The Users Club text describes this full layout. MAME and vaeg differ in
expansion attachment, later overlays, Kanji region widths, and broad TVRAM
handlers. Those implementations must not replace the original-VA baseline.

Kanji ROM is a legal source but not a meaningful destination. Reserved reads,
writes, and wrap at `3FFFFFh` remain **[UNKNOWN]**. Returning `FFFFh` is an
emulator policy, not established hardware.

## 4. CPU I/O interface

### 4.1 Registers

The word-oriented interface is in `0500h-0507h`:

| Port | Direction | Reconstructed function |
|---:|---|---|
| `0500h` | Write (word) | Low 16 bits of command-list start address |
| `0502h` | Write (word) | High 16 bits of command-list start address; bit 0 of the combined address is forced even |
| `0504h` | Read/write | Interrupt enable and abort control |
| `0506h` | Read/write | Start/attention and busy |
| `0508h` | Read in vaeg | Unknown; current implementation returns 1 |
| `0580h` | Read/write | Read-side `RBUSY`; write-side GVRAM access mode (`10h` = CPU data write) |

The low and high command-address fields are word I/O registers, not four
independent byte ports. This is established by the period Tech manual and by
the VA BIOS sequence that executes `OUT DX,AX` at `0500h`, advances to `0502h`,
and executes a second `OUT DX,AX`. The start/status access at `0506h` remains
byte-wide.

The local implementation masks `0504h` to interrupt enable `04h` and abort
`02h`. OCR wording that appears to say “bit 4” conflicts with diagrams/examples
indicating value `04h`; the latter is stronger evidence.

Writing busy bit 0 at `0506h` while idle loads the programmed command pointer
and starts. END clears busy and optionally requests interrupt 8 in vaeg. Abort
clears busy and follows the current completion-interrupt helper; exact hardware
abort/interrupt ordering remains unresolved.

Before certain reads, software ensures `0580h.RBUSY=0`. This bus/register
readiness is separate from SGP command busy. The VA BIOS also writes `10h` to
`0580h` immediately before each command-list kick; the demos mirror that write
to reassert CPU-data GVRAM write mode without changing the byte access width.

### 4.2 Recommended start sequence

1. Build the complete list in main RAM.
2. Reserve and initialize required descriptors and work memory.
3. Wait until SGP busy is clear.
4. Program the low word at `0500h` and the high word at `0502h`.
5. Configure interrupt enable at `0504h`.
6. Start through `0506h`.
7. Poll busy or wait for completion interrupt.
8. Observe `0580h.RBUSY=0` before sensitive status reads.

Self-modifying active lists and start-while-busy are not known coherent.

## 5. Command execution and state

The SGP reads little-endian 16-bit opcodes and command-specific parameters.
State-setting commands update work, source, destination, and color state;
drawing commands consume it; END terminates.

At minimum, state contains command PC, 58-byte work-area address, source and
destination descriptors, full color word, busy, interrupt enable/pending,
abort, and intermediate command progress.

The work area must be writable and stable during execution. Its internal
58-byte layout is not recovered and should not be invented unless software
observes SGP writes there.

Power-on values for all fields are **[UNKNOWN]**; software initializes required
state explicitly.

## 6. Pixel representation

| Mode | Bits/pixel | Pixels per 16-bit word |
|---|---:|---:|
| 0 | 1 | 16 |
| 1 | 4 | 4 |
| 2 | 8 | 2 |
| 3 | 16 | 1 |

Words are little-endian in memory, but CPU byte order alone does not establish
visual pixel lane order. Tests must determine least-significant lane,
start-dot selection, boundary crossing, and reverse traversal in every mode.

The BNN manual describes the `SET COLOR` value as being consumed in units that
match the selected pixel mode ([BNN] `PC88VA_テクニカルマニュアル_BNN.md`,
§4.5.6(4.6), pp. 97-98):

| Pixel mode | Color bits consumed for one word | Interpretation |
|---|---:|---|
| 1 bpp | 1 bit × 16 pixels | The selected bit is used for all 16 lanes |
| 4 bpp | 4 bits × 4 pixels | The selected 4-bit value is repeated for 4 lanes |
| 8 bpp | 8 bits × 2 pixels | The selected 8-bit value is repeated for 2 lanes |
| 16 bpp | 16 bits × 1 pixel | The complete color word is used |

This applies to 1-bpp source expansion, `LINE`, `CLS`, and the `SCAN` color
comparison. It does not by itself settle which byte or bit is the leftmost
visual lane; that remains a separate pixel-order question. Host-endian fill
shortcuts are unsafe without lane tests.

## 7. Block descriptors

A descriptor contains pixel mode, start dot, width, height, framebuffer pitch
`FBW`, and even 22-bit start address. `FBW` advances scan lines and is not the
transfer rectangle width.

The BNN block diagrams explicitly specify `FBW` as a byte count and require its
low two bits to be zero ([BNN] `PC88VA_テクニカルマニュアル_BNN.md`,
§4.5.6(3.2), p. 95). Width and height are pixel counts; they must not be
derived from `FBW` or rounded to a word count. The start-dot field is bounded by
the pixel mode: 0..15 for 1 bpp, 0..3 for 4 bpp, 0..1 for 8 bpp, and 0 for
16 bpp.

The aligned word address and start-dot offset allow unaligned rectangles while
first/last-word masks preserve neighboring pixels. Horizontal and vertical
direction select top-left, top-right, bottom-left, or bottom-right traversal,
which is required for overlapping copies.

**[IMPLEMENTATION]/[INFERENCE]** vaeg reads width as 14 bits and height as 16
bits with comments identifying VA2 capability. This is not yet a verified
original-VA rule and must be model-specific rather than generalized to VA3.

Zero width/height semantics, exact original field widths, signed `FBW`, and
overflow remain hardware-test targets.

## 8. Command-set summary

The documentation says thirteen command types exist, but only twelve opcodes
are presently identified:

| Opcode | Name | Function |
|---:|---|---|
| `0001h` | END | Terminate and optionally interrupt |
| `0002h` | NOP | No operation |
| `0003h` | SET_WORK | Set 58-byte work-area address |
| `0004h` | SET_SOURCE | Load source descriptor |
| `0005h` | SET_DESTINATION | Load destination descriptor |
| `0006h` | SET_COLOR | Load color state |
| `0007h` | BITBLT | Rectangular source-to-destination transfer |
| `0008h` | PATBLT | Repeated two-dimensional pattern transfer |
| `0009h` | LINE | Draw a straight line |
| `000Ah` | CLS | Fill a word range |
| `000Bh` | SCAN_RIGHT | Scan right to a color condition |
| `000Ch` | SCAN_LEFT | Scan left to a color condition |

Possible explanations are a missing opcode, an internal/reserved operation, an
editorial count error, or a later-model addition. Unknown opcodes must not be
silently aliased to NOP.

## 9. State-setting and control commands

### 9.1 END (`0001h`) and NOP (`0002h`)

END stops execution, clears busy, and generates a completion interrupt when
enabled. Exact post-END command pointer and completion-latch behavior remain
unknown. NOP advances without changing drawing state.

### 9.2 SET_WORK (`0003h`)

SET_WORK loads an even address for the 58-byte writable work area. Software
must keep it valid and must not reuse it while the SGP is busy.

### 9.3 SET_SOURCE (`0004h`) and SET_DESTINATION (`0005h`)

Each loads a block descriptor: address, start dot, pixel mode, width, height,
and pitch. Source may be RAM or Kanji ROM. Destination normally targets
writable RAM, TVRAM, or GVRAM.

### 9.4 SET_COLOR (`0006h`)

SET_COLOR loads the full 16-bit color word used by expansion, LINE, CLS, and
scan comparison. The BNN lane rule is not an optional display-depth
conversion: the relevant 1-, 4-, or 8-bit field is repeated across the word,
while 16 bpp consumes the complete 16-bit value. It must not be truncated to
the current display depth or interpreted as a host-endian pixel buffer.

## 10. BITBLT/PATBLT mode and Boolean ROP

The mode word contains source alignment/format `SF`, vertical direction `VD`,
horizontal direction `HD`, transparent-processing `TP`, a four-bit operation,
and reserved bits.

The Technical Manual defines:

| Field | Mask |
|---|---:|
| `SF` | `1000h` |
| `VD` | `0800h` |
| `HD` | `0400h` |
| `TP` | `0300h` |
| operation | `000Fh` |

The current vaeg BITBLT/PATBLT definitions use the same masks. LINE uses the
same documented bit positions, so a separate swapped LINE direction mapping is
not supported by this source.

`VD`/`HD` choose traversal direction and safe overlap order. The BNN definition
of `SF` is explicit ([BNN] `PC88VA_テクニカルマニュアル_BNN.md`,
§4.5.6(4.7), p. 99): `SF=0` shifts the source to the destination dot
position; `SF=1` transfers the source without that alignment shift. Each
start-dot offset and word-boundary crossing needs a raw-vector test.

### 10.1 Transparency

At minimum, the recovered modes distinguish:

- ordinary source transfer;
- skip when the source pixel is zero;
- transfer only where destination is zero;
- one-bit source color expansion.

The BNN table defines only `TP-MOD=0`, `1`, and `2`; it does not define
`TP-MOD=3` ([BNN] `PC88VA_テクニカルマニュアル_BNN.md`, §4.5.6(4.7),
p. 99). Therefore `TP=3` is currently **[UNKNOWN]**, not a fourth documented
transparency mode. Some VAEG helper paths historically treat it like
destination-zero; that is an implementation compatibility choice and must not
be presented as hardware behavior.

The same BNN section gives two cross-format restrictions which callers and
negative tests must preserve:

* a differing source/destination pixel format is supported only for 1-bpp
  source to multi-bpp destination expansion;
* 1-bpp-to-multi-bpp expansion requires `HD=0` (left-to-right), and `HD=1`
  requires `TP-MOD=0`.

### 10.2 ROP ordering

The Technical Manual gives all sixteen Boolean functions in this order:

| ROP | vaeg result |
|---:|---|
| `0` | `0` |
| `1` | `S & D` |
| `2` | `~S & D` |
| `3` | `D` (write mask cleared) |
| `4` | `S & ~D` |
| `5` | `S` |
| `6` | `S ^ D` |
| `7` | `S | D` |
| `8` | `~(S | D)` |
| `9` | `~(S ^ D)` |
| `A` | `~S` |
| `B` | `~S | D` |
| `C` | `~D` |
| `D` | `S | ~D` |
| `E` | `~(S & D)` |
| `F` | all ones |

Current vaeg implements this same order. Focused tests must still cover the raw
ROP nibble, source word, initial destination, and partial-word masks so a
future refactor cannot silently reorder the table.

## 11. BITBLT (`0007h`)

BITBLT walks a rectangular source and destination using independent pitch and
start-dot alignment. For every destination pixel it obtains source, applies
transparency, reads destination when the ROP needs it, evaluates the operation,
and writes only selected bits. First- and last-word masks preserve neighboring
pixels.

The ordinary case uses equal pixel modes. A 1-bpp source can expand to a
multi-bit destination using the loaded color and `TP`; the source bit may emit
foreground/background, skip, or combine through ROP depending on mode.

Overlapping regions require the direction that preserves unread source.
All four direction combinations must be compared against a snapshot-based
reference copy.

**[IMPLEMENTATION]** vaeg replaces destination width/height with the source
dimensions for BITBLT. Whether this is the precise hardware rule must be tied
to period wording or raw hardware tests.

## 12. PATBLT (`0008h`)

PATBLT repeats the source block as a two-dimensional tile across the
destination:

~~~text
source_x = destination_relative_x mod source_width
source_y = destination_relative_y mod source_height
~~~

It then applies transparency and ROP. `PATBLT` is the canonical spelling here;
`PTNBLT` is retained as a search synonym.

Tests include 1x1, non-divisor pattern sizes, source word boundaries, reverse
directions, one-bit expansion, and a Kanji-ROM pattern.

## 13. LINE (`0009h`)

LINE uses a destination descriptor, color, ROP, direction, and discrete slope
state. Behavior is consistent with an integer Bresenham-family accumulator,
but endpoint inclusion, tie breaking, major-axis choice, and initial error are
hardware-visible.

**[DOCUMENTED]** LINE uses `VD=0800h` and `HD=0400h`, the same direction-bit
positions as BITBLT and PATBLT. Current vaeg's separate
`LINE_VD=0400h`/`LINE_HD=0800h` definitions are reversed. Implementations must
test asymmetric lines in all four direction combinations. Do not use a host
graphics-library line routine.

Required cases are horizontal, vertical, 45-degree, shallow, steep, every
octant, one-pixel, reversed endpoints, word boundaries, and descriptor edges.
Transparency and all ROPs apply per selected pixel.

The local source contains two hardware-fit observations that remain evidence,
not formal rules: a one-count error adjustment for a 7-by-6 case, and suspected
large destructive behavior for zero-by-zero dimensions.

## 14. CLS (`000Ah`)

CLS is a contiguous word-range fill. Current vaeg reads a 32-bit even start
address, a 32-bit word count, and repeatedly stores the current 16-bit color.

A 640x400 4-bpp buffer occupies 128000 bytes or 64000 words, but software must
use the documented encoding rather than assume whether a field is count,
count-minus-one, or terminal address.

Odd starts, zero count, abort during fill, and 22-bit boundary crossing require
tests.

## 15. SCAN_RIGHT (`000Bh`)

SCAN_RIGHT advances pixel by pixel until it finds the SET COLOR value. The
destination descriptor supplies the starting pixel, pixel mode, and maximum
pixel count. If the starting pixel already has the selected color, the result
width is zero. If a later pixel matches, the destination width becomes the
number of pixels from the start to that boundary. If no pixel matches within
the maximum count, the destination width remains unchanged. Current vaeg
executes the scan asynchronously through the SGP state machine. The
emulator-side SCAN sanity test covers first-pixel, later-pixel, miss,
packed-word, nearest-boundary, and adjacent-boundary cases.

## 16. SCAN_LEFT (`000Ch`)

SCAN_LEFT applies the same color and count rules while moving left. On a
match, it additionally updates the destination start address and start-dot to
the left edge of the scanned region. A first-pixel match produces width zero;
a miss leaves the descriptor unchanged. Current vaeg implements this behavior
in the same asynchronous state machine as SCAN_RIGHT.

Tests must distinguish initial-pixel stopping, one-pixel-away stopping,
packed-word crossing, x=0, rightmost start, no match, and left/right symmetry.

SCAN is a documented functional command and should not be omitted merely
because common software may implement flood fill on the CPU.

## 17. Restrictions and undefined cases

1. SGP operation is intended for single-plane mode.
2. Portable command lists should reside in main RAM.
3. The 58-byte work area must stay writable and stable.
4. Word addresses should be even.
5. Reserved bits should be zero.
6. ROM is source-only.
7. Reserved-space writes and reads are undefined.
8. 4MiB boundary crossing is unverified.
9. Zero width/height semantics are unverified.
10. Descriptor fields may differ by model.
11. Active command-list modification is not known coherent.
12. Start-while-busy is undefined.
13. Abort may leave a partially updated read-modify-write word.
14. Unknown opcodes must remain visible in diagnostics.
15. CPU, SGP, and display access to shared memory may contend.
16. `FBW` is a byte pitch and its low two bits must be zero.
17. Cross-format transfer is restricted to documented 1-bpp-to-multi-bpp
    expansion; the documented direction/transparent-mode restrictions apply.
18. `TP-MOD=3` is not defined by the BNN command table and remains unknown.

## 18. Memory contention and timing

The Users Club material documents waits and contention involving TVRAM,
character/Kanji resources, GVRAM, and display activity. CPU wait tables
constrain relative costs but are not automatically SGP command-cycle counts.

~~~text
command time =
    decode/setup
  + source reads
  + destination reads required by ROP
  + destination writes
  + line or scan stepping
  + bus/display arbitration
~~~

No authoritative per-command clock table has been recovered. Command fetch,
setup, pixel/word cost, ROP differences, transparency skip, active-versus-blank
cost, model clock, abort latency, and interrupt latency remain unresolved.

**[IMPLEMENTATION]** vaeg has detailed historical command costs and deducts a
fixed four CPU clocks for every SGP memory access, with a source comment that
four has no basis and real contention should apply only when CPU access
coincides. The M20 speed work deliberately preserved these values. They must
not be described as measured hardware timing.

A staged emulator strategy is:

1. **Functional:** execute deterministically, schedule estimated completion,
   keep busy until then.
2. **Contention-aware:** route memory through a shared bus scheduler and define
   save-state boundaries.
3. **Research:** trace fetches, accesses, arbitration, and provisional costs
   against hardware captures.

## 19. Source and implementation differences

| Topic | Period documentation | Current vaeg | Reconstruction decision |
|---|---|---|---|
| Device identity | Software name SGP; schematic IC75 `VDP`, `D92017-002` | SGP | Original VA is `μPD92017`; VA2 successor is `D92046GD-001`; `D65200` is the separate GAL-3 |
| Command count | Says 13, lists 12 | Dispatches 1 through C | Do not invent or hide an opcode |
| Address space | 4MiB original-VA map | Broadly modeled; incomplete handlers | Period map is baseline; overlays are model profiles |
| Command table | Main RAM for portable software | Fetch backend can address SGP space | Require main RAM until hardware proves otherwise |
| Work area | 58 bytes | Address retained | Preserve size; do not invent layout |
| SET_COLOR lane expansion | Mode-dependent 1/4/8/16-bpp lane use is documented | Full color is retained, but lane-level conformance remains a test target | Add mode-specific raw vectors |
| `SF` source alignment | `0=shift to destination dot`, `1=no shift` | Alignment path exists | Verify every start-dot and word-boundary combination |
| `TP-MOD=3` | Not defined in BNN | Some helpers treat it like destination-zero | Keep as explicit unknown; do not generalize |
| `FBW` | Byte pitch; low two bits zero | Parsed and masked per model | Add invalid-pitch negative tests |
| BITBLT/PATBLT | Documented | Implemented; edge cases and hardware conformance remain incomplete | Period semantics first |
| LINE | Documented | Implemented; direction conflict | Raw-bit hardware test |
| CLS | Documented | Word-count fill | Verify count encoding |
| SCAN_RIGHT / SCAN_LEFT | Documented | Implemented and covered by emulator-side sanity tests | Real-hardware conformance remains open |
| Kanji ROM source access | Address regions are present in the SGP map | `knj1w_rd()` and `knj2w_rd()` are TODO | Required only for SGP transfers sourced from Kanji ROM |
| Kanji ROM writes | ROM is not a normal writable target | `knj2w_wt()` is a TODO no-op | Confirm write-protection behavior before changing it |
| Unknown command after `000Ch` | Manual says thirteen commands but identifies twelve opcodes | `000Dh` and above are rejected as unknown | Do not invent the missing opcode |
| VA2 descriptor width | Not fully established | 14-bit width/16-bit height comments | Separate later-model profile |
| Clock | No recovered primary table | Nominal 3.9936/7.9872MHz | Captured/provisional provenance |
| Contention | Shared-memory waits | Fixed approximate CPU deduction | Do not claim cycle accuracy |

### 19.1 Functions outside the SGP command set

The following items must not be reported as missing SGP opcodes. They are
either higher-level software operations or responsibilities of another VA
device/layer:

| Function | SGP status | Correct interpretation |
|---|---|---|
| Dedicated rectangle-fill opcode | Not identified | A solid rectangle can be composed with a 1x1 source and PATBLT; CLS is a linear word fill, not a rectangle primitive |
| Flood fill / PAINT | No dedicated SGP command | BIOS or guest software can combine SCAN_LEFT/SCAN_RIGHT with PATBLT |
| Polygon fill | Not an SGP command | Requires CPU/BIOS edge setup and repeated SGP operations |
| Circle / ellipse | Not an SGP command | Must be approximated by CPU-generated LINE records or another graphics layer |
| Sprite management | Not an SGP responsibility | Owned by TSP/pseudo-sprite software |
| Scaling and rotation | Not an SGP responsibility | Performed by CPU-side geometry or sprite hardware/software |
| Alpha blending and z-buffering | Not an SGP command | No corresponding SGP state or opcode is identified |
| 3D transformation | Not an SGP responsibility | CPU-side operation |

This distinction is important: the absence of these functions from SGP is a
hardware-command-set property, not an assertion that VAEG is missing an SGP
implementation. Conversely, Kanji ROM access, the unresolved thirteenth
command, and hardware-accurate timing remain genuine SGP audit items.

MAME must be checked directly for individual semantics before attributing a
behavior to it. Its device and map are useful implementation evidence, not a
blanket confirmation.

## 20. Reference execution model

This pseudocode describes separation of concerns, not an implementation oracle:

~~~c
typedef struct {
    uint32_t address;
    uint16_t framebuffer_width;
    uint16_t block_width;
    uint16_t block_height;
    uint8_t start_dot;
    uint8_t pixel_mode;
} SgpBlock;

typedef struct {
    uint32_t command_pc;
    uint32_t work_address;
    SgpBlock source;
    SgpBlock destination;
    uint16_t color;
    bool busy;
    bool interrupt_enabled;
    bool interrupt_pending;
    bool abort_requested;
} SgpState;

static uint32_t sgp_mask_address(uint32_t address)
{
    return address & 0x003fffffU;
}

static void sgp_execute_command_list(SgpState *state)
{
    state->busy = true;
    state->interrupt_pending = false;

    while (state->busy && !state->abort_requested) {
        uint16_t opcode = sgp_read_word(state->command_pc);
        state->command_pc = sgp_mask_address(state->command_pc + 2U);

        switch (opcode) {
        case 0x0001:
            state->busy = false;
            state->interrupt_pending = state->interrupt_enabled;
            break;
        case 0x0002:
            break;
        case 0x0003:
            sgp_decode_work_address(state);
            break;
        case 0x0004:
            sgp_decode_source_descriptor(state);
            break;
        case 0x0005:
            sgp_decode_destination_descriptor(state);
            break;
        case 0x0006:
            sgp_decode_color(state);
            break;
        case 0x0007:
            sgp_execute_bitblt(state);
            break;
        case 0x0008:
            sgp_execute_patblt(state);
            break;
        case 0x0009:
            sgp_execute_line(state);
            break;
        case 0x000a:
            sgp_execute_cls(state);
            break;
        case 0x000b:
            sgp_execute_scan_right(state);
            break;
        case 0x000c:
            sgp_execute_scan_left(state);
            break;
        default:
            sgp_report_unknown_opcode(state, opcode);
            state->busy = false;
            break;
        }
    }

    if (state->abort_requested)
        sgp_finish_abort(state);
}
~~~

Command decoding, pixel/ROP semantics, SGP address access, timing/arbitration,
and machine-model differences should be separate layers.

## 21. Command-list examples

These examples are schematic; descriptor packing must be replaced by verified
raw fields before use as binary vectors.

### 21.1 Work area

~~~text
0003h               ; SET_WORK
<low address word>
<high address bits>
0001h               ; END
~~~

The programmed area occupies 58 writable bytes.

### 21.2 Clear a 640x400 4-bpp framebuffer

~~~text
640 * 400 * 4 / 8 = 128000 bytes
128000 / 2         = 64000 words

0003h               ; SET_WORK
...
0006h               ; SET_COLOR
0000h               ; black packed word
000Ah               ; CLS
<destination address>
<documented count/range>
0001h               ; END
~~~

Do not assume the field stores exactly 64000 until count versus count-minus-one
versus terminal-address semantics are verified.

## 22. Conformance-test plan

### 22.1 Minimal function

1. `NOP; END` clears busy and optionally interrupts.
2. SET_WORK preserves the exact even address.
3. SET_SOURCE/DESTINATION do not alter unrelated state.
4. SET_COLOR retains every bit.
5. CLS fills exactly the requested range.
6. BITBLT copies 1x1 and an unaligned rectangle.
7. PATBLT repeats a non-power-of-two tile.
8. LINE draws every octant.
9. SCAN RIGHT/LEFT produce symmetric results.
10. Abort stops a long command at a reproducible boundary.
11. Unknown opcodes are reported rather than hidden.

### 22.2 Pixel order and ROP

For every pixel mode, place distinct values in every lane, copy one pixel from
each start-dot, repeat in reverse, cross a word boundary, and compare both raw
bytes and displayed pixels.

For every ROP nibble, use asymmetric source and destination values and partial
edge masks. This resolves the two ordering tables in section 10.

### 22.3 LINE direction discriminator

Use a non-square, unequal-slope line and run raw modes:

~~~text
VD=0 HD=0
VD=0 HD=1
VD=1 HD=0
VD=1 HD=1
~~~

Capture raw command words, memory before/after, every changed coordinate, busy
duration, and final command pointer.

### 22.4 Descriptor widths and zero dimensions

On each model, test the highest documented original width/height, the next
value, candidate 14/16-bit limits, zero, and guarded large transfers. Observe
ignore, wrap, extension, alias, or another-field effects.

### 22.5 SCAN, abort, and unknown opcode

SCAN tests cover target on initial/next/final pixel, absent target, packed-word
boundary, x=0/right edge, equality/inequality, and left/right symmetry.

Abort each command during fetch, source read, destination read/write, line
step, and scan step. Observe partial word, busy, interrupt, pointer, and restart.

Unknown-opcode probes require controlled hardware, guard memory, a following
END, watchdog, and power-cycle recovery. Record status, pointer, interrupt, and
all memory changes.

### 22.6 Resolution and viewport

Use the safe sequence in section 2.6. For 320x200, distinguish physical 2x2
expansion from a new sync mode. For 384x256, keep 640x400 timing and test only
pitch, logical height, display position, masking, and backdrop. Record raw
registers and GVRAM so SGP drawing errors are not confused with fetch or mixer
errors.

## 23. Unresolved issues and priorities

### P0: functional correctness

1. Resolve LINE `VD`/`HD`.
2. Recover exact SCAN parameters and results.
3. Resolve thirteen stated commands versus twelve opcodes.
4. Verify raw descriptor bit layout.
5. Determine zero width/height.
6. Establish first/last-pixel masks.
7. Verify every `TP` encoding.
8. Resolve the Boolean ROP nibble ordering conflict.

### P1: likely software compatibility

1. Complete an independently reviewable IC75/IC76 page, pin, and net
   transcription; trace `VRDY`, `VOPRDY`, `VODMA`, and `WAIT` end to end.
2. Verify later-model descriptor extensions.
3. Confirm Kanji ROM/RAM overlays by model.
4. Test command lists outside main RAM.
5. Establish start-while-busy.
6. Establish abort and partial-write rules.
7. Determine reserved-region read values.
8. Verify pointer readback/post-END value.
9. Recover and cite Wiki pages individually.
10. Verify 320x400 and fixed-raster viewport behavior without calling them
   official modes prematurely.

### P2: timing

1. Determine actual clock by model.
2. Measure opcode setup and per-word/per-pixel costs.
3. Measure ROP/transparency cost differences.
4. Measure CPU/SGP/display arbitration.
5. Compare active display and blanking throughput.
6. Measure interrupt and abort latency.
7. Define safe save-state boundaries.

## 24. Emulator implementation recommendations

Use explicit profiles such as:

~~~text
original_va_documented
va2_candidate
va3_candidate
mame_compatibility
vaeg_legacy_compatibility
hardware_research
~~~

Profiles select descriptor widths, memory overlays, timing, LINE direction,
ROP ordering until resolved, reserved-bit policy, and undefined-space behavior.

The command engine should request abstract operations such as:

~~~text
read_word(address)
write_word(address, value, mask)
read_pixel(descriptor, x, y)
write_pixel(descriptor, x, y, value)
consume_cycles(count)
request_bus(resource)
~~~

The backend owns model maps, ROM suppression, contention, CPU arbitration,
tracing, and save-state serialization.

For every conflict, retain documented interpretation, observed vaeg/MAME
interpretation, selected default, provenance, and a falsifying test. Implement
SCAN rather than treating its current absence as specification evidence.

Every timing constant should carry:

~~~text
source = measured | manual | implementation | estimate
machine = va | va2 | va3
confidence = confirmed | provisional | unknown
~~~

## 25. Consolidated specification summary

The original PC-88VA SGP is the `μPD92017` at IC75, package marking
`D92017-002` and schematic label `VDP`. Its VA2 successor is
`D92046GD-001`. The concurrently present `D65200GD-054` is GAL-3, a separate
GVRAM display sequencer and arbitration device.

The SGP is a memory drawing coprocessor controlled through the `0500h` range.
It fetches little-endian commands from a programmed list, retains work/source/
destination/color state, and accesses a private 22-bit, 4MiB space. The IC75
main-memory-master pin group physically corroborates this reach, while its
GVRAM-master and `VINT` interfaces corroborate drawing and IRQ 8 integration.

The recoverable command set is END, NOP, SET_WORK, SET_SOURCE,
SET_DESTINATION, SET_COLOR, BITBLT, PATBLT, LINE, CLS, SCAN_RIGHT, and
SCAN_LEFT. The stated thirteen-command count remains unresolved.

It supports packed 1/4/8/16-bpp pixels, rectangle and pattern transfer,
one-bit expansion, sixteen Boolean functions, lines, range fill, and scan
assistance. The strongest functional conflicts are the LINE direction symbols,
ROP ordering, missing SCAN detail, and absent thirteenth opcode.

The SGP does not own display timing. Arbitrary logical resolutions should be
viewports inside documented raster modes until TSP, graphics fetch, scaling,
and D65101 composition are independently verified.

A faithful emulator implements period semantics first, isolates machine
profiles, exposes unresolved behavior through traceable choices, and does not
treat vaeg or MAME as a hardware oracle.

## 26. References

1. BNN Corporation, *PC-88VA Technical Manual*, first edition, 25 June 1987,
   ISBN-10 **4-89369-024-8**.
   <https://archive.org/details/PC88VA>
2. *PC-88VA Technical Manual, 88VA Users Club Edition*, beta 1.0,
   1992-01-20, historically distributed as `tekumani.lzh`.
   <http://www.iris.dti.ne.jp/~nano/88va/tekumani.html>
3. vaeg repository and local [`io/sgp.c`](../../io/sgp.c) /
   [`io/sgp.h`](../../io/sgp.h).
4. MAME [`pc88va.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp).
5. MAME [`pc88va_sgp.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_sgp.cpp)
   and [`pc88va_sgp.h`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_sgp.h).
6. [Inside PC-88VA Wiki](http://www.pc88.gr.jp/inside88va/wiki/).
7. Maintainer full-circuit-diagram ownership audit, 18 July 2026, based on the
   complete original-VA schematic section of reference 1 and recorded in
   sections 1.1 and 2 of this document.

Wiki statements must be cited page by page when recoverable. Captured claims
are not primary evidence without a retrievable page or corroboration.

## 27. Change log

### 0.4-en.2 - 2026-08-23

- Added BNN-backed `SET_COLOR` lane expansion rules for 1/4/8/16 bpp.
- Made the BNN `SF` shift/no-shift semantics explicit.
- Recorded the documented `FBW` byte-pitch and low-two-bit alignment rule.
- Separated the documented `TP-MOD=0..2` modes from the undefined `TP-MOD=3`
  implementation compatibility behavior.
- Recorded the BNN cross-format and direction restrictions as negative-test
  requirements.

### 0.4-en.1 - 2026-07-18

- Corrected the original-VA SGP identity to `μPD92017`, package marking
  `D92017-002`, schematic reference IC75 and label `VDP`.
- Identified `D65200GD-054` as the separate IC76 `GAL-3` GVRAM sequencer and
  documented the shared drawing/display GVRAM bus.
- Added the complete original-VA custom-LSI ownership table and signal-based
  SGP identification evidence.
- Added the VA1-to-VA2 correspondence, including `D92046GD-001` as the VA2
  VDP/SGP successor and `D65200GD-076` as its distinct GAL-3 contemporary.
- Recorded the `VINT`, ready, DMA, wait, main-memory-master, and GVRAM-master
  implications for emulator interrupt, address-space, and contention work.
- Renamed the document from `upd65200-sgp.md` to `upd92017-sgp.md` and updated
  its cross-document reference.

### 0.3-en.1 - 2026-07-16

- Added a complete English reconstructed specification.
- Preserved distinctions among period documentation, community research,
  vaeg, MAME, and captured Wiki evidence.
- Identified `tekumani.lzh` as the canonical distribution and kept its original
  SHA-256 unverified.
- Retained the then-unverified `uPD65200` hypothesis; version 0.4 supersedes
  it with the full-schematic IC75/IC76 identification.
- Preserved the twelve-opcode/thirteen-command, LINE direction, ROP ordering,
  and SCAN conflicts.
- Added fixed-raster viewport guidance, safe resolution experiments,
  conformance tests, and implementation profiles.
- Linked the cross-block video-mode control reconstruction and corrected the
  first 384x256 experiment to retain a 640-dot graphics-source pitch.
