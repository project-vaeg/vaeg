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
# Reconstructed Formal Specification of the PC-88VA uPD72022 TSP

> **Status:** Reconstructed, non-authoritative specification
> **Version:** 0.1
> **Date:** 2026-07-16 (JST)
> **Target:** NEC PC-88VA-series use of the NEC μPD72022 Intelligent Display
> Processor
> **PC-88VA name:** TSP, meaning Text/Sprite Processor
> **Repository path:** `docs/modernization/upd72022-tsp.md`

> [!WARNING]
> This document is not an original NEC data sheet and must not be treated as
> an electrical replacement specification. It reconstructs the programming
> model relevant to PC-88VA emulation by reconciling the NEC μPD72022 data
> book, PC-88VA technical material, Tekumani material, and emulator sources.
> Where sources disagree or leave gaps, the uncertainty is explicit.

The μPD72022 is a display processor. It is unrelated to the PC-88VA3 2TD
floppy controller. The close functional match between the retail μPD72022 and
the VA TSP does not by itself prove that every PC-88VA model contains an
electrically identical, unmodified commercial part.

## 1. Purpose and scope

This is an emulator-oriented behavioral specification for the μPD72022
programming model as used by the PC-88VA family. The original PC-88VA is the
baseline profile. VA2/VA3 differences are stated separately where evidence
exists and otherwise remain unresolved.

The reconstructed PC-88VA profile covers:

- host I/O at ports `0142h` and `0146h`;
- command, parameter, result, and status behavior;
- native text rendering and four horizontal split-screen descriptors;
- the six PC-88VA text attribute modes;
- hardware character codes, ANK/Kanji selection, and 40/80-column operation;
- 32 hardware sprites, pattern formats, priority, clipping, overflow, and
  collision detection;
- cursor operation through the sprite engine;
- the PC-88VA-specific μPD3301 emulation command `EMUL` (`8Ch`);
- board-level registers that compose TSP output with two graphics screens;
- documented PC-88VA timing profiles;
- known MAME and vaeg implementation gaps; and
- conformance tests and unresolved hardware questions.

Electrical limits, package dimensions, pin timing, DRAM refresh waveforms,
DMA waveforms, and bus-arbitration waveforms remain defined by the original
NEC data book and are not reproduced in full.

## 2. Normative vocabulary and evidence labels

The words **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, and **MAY**
describe implementation requirements in this reconstructed specification.

| Label | Meaning |
|---|---|
| `[CONFIRMED]` | Agrees across the NEC device data and PC-88VA-specific documentation, or follows from an unambiguous bit layout. |
| `[DOCUMENTED]` | Explicitly stated in one authoritative or near-authoritative source, but not independently cross-checked. |
| `[IMPLEMENTATION]` | Behavior found in emulator source code; evidence of software assumptions, not automatically hardware behavior. |
| `[INFERENCE]` | Derived from several documented facts or necessary address/format arithmetic. |
| `[CONFLICT]` | Two sources, or two sections of one source, disagree. |
| `[UNKNOWN]` | The surviving material does not determine the behavior. |

Confirmed and documented behavior should be implemented first. Conflicts and
unknowns should be isolated behind named helpers, tests, or deliberately named
compatibility switches rather than silently guessed.

## 3. Terminology

NEC names the uPD72022 an **Intelligent Display Processor (IDP)**. The active
vaeg tree names the corresponding PC-88VA functional block the **Text Sprite
Processor (TSP)** in `io/tsp.c` and `io/tsp.h`.

These names describe substantially overlapping responsibilities:

- CRT raster and synchronization control;
- text and semigraphics display;
- text attributes and cursor control;
- sprite attributes, display, grouping, and collision status;
- display-memory addressing and host transfers.

The uPD72022 documentation is therefore a strong semantic reference for the
VA TSP. It must not yet be treated as proof that every PC-88VA model contains
an unmodified commercial uPD72022. Contemporary PC-88VA descriptions call its
VDP and IDP custom LSIs, and vaeg contains comments recording behavior that
differs from the uPD72022 data sheet. Board markings, an NEC VA schematic, or a
VA-specific parts list are still needed to establish the exact silicon
relationship.

## 4. Source inventory and evidence hierarchy

### 4.1 Primary and near-primary sources

The best available source is NEC Electronics Inc., *Intelligent Peripheral
Devices (IPD) Data Book*, 1989-1990 edition, June 1989, Document No. 50051.
The complete uPD72022 chapter occupies printed pages 3-57 through 3-96.

- [Bitsavers scan](https://www.bitsavers.org/components/nec/_dataBooks/1989_NEC_Intelligent_Peripheral_Devices_Data_Book.pdf)
- [Internet Archive mirror with OCR text](https://archive.org/details/bitsavers_necdataBootPeripheralDevicesDataBook_33483764)
- [Searchable Bitsavers mirror](https://ftpmirror.your.org/pub/misc/bitsavers/pdf/nec/_dataBooks/1989_NEC_Intelligent_Peripheral_Devices_Data_Book.pdf)
- [Standalone data-sheet index](https://datasheet4u.com/datasheet/NEC-Electronics/UPD72022-521631)

The Internet Archive item supplies the original 570-page PDF and generated OCR
text. The scan is preferable to third-party summaries because it includes the
pinout, command parameters, timing diagrams, electrical characteristics, and
package drawings.

A contemporary Japanese design article also exists:

- I. Kozono et al., "Intelligent display processor uPD72022," *NEC Technical
  Journal*, Vol. 39, No. 10, October 1986, pp. 64-69.

The article's bibliographic record is preserved in the non-patent citations of
[WO2008076950A3](https://patents.google.com/patent/WO2008076950A3/en). A freely
accessible scan of the article was not located during this investigation.

No uPD72022 file is present in the
[ChipDB NEC index](https://datasheets.chipdb.org/NEC/) that hosts the V20/V30
and V40/V50 manuals. For this device, Bitsavers is the more complete source.

The platform-specific sources are:

1. **PC-88VA Technical Manual**, preserved by the
   [Internet Archive](https://archive.org/details/PC88VA/page/76/mode/2up).
2. **PC-88VA Tekumani material**, especially `2.TXT`, `4.TXT`,
   `604TEXT.TXT`, and `605SPR.TXT`. These provide the I/O map, status and
   text-control registers, TVRAM and descriptor formats, native attributes,
   TSP command profiles, timing vectors, Text BIOS behavior, and Sprite BIOS
   policy.

The NEC data-book chapter is primary for the generic device. The PC-88VA
technical manual and Tekumani material take precedence for board wiring,
machine-specific extensions, installed memory, and external composition.

### 4.2 Implementation sources

Implementation evidence comes from:

- vaeg [`io/tsp.c`](../../io/tsp.c) and
  [`io/videova.c`](../../io/videova.c);
- MAME
  [`pc88va.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp)
  and
  [`pc88va_v.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_v.cpp).

MAME explicitly describes its PC-88VA video support as incomplete, and vaeg
implements only part of the command set. Neither implementation is a hardware
oracle.

The [Inside PC-88VA Wiki](http://www.pc88.gr.jp/inside88va/wiki/index.php?FrontPage)
is useful as a preservation and cross-reference index, but has lower authority
than the device and machine technical material.

## 5. Published device capabilities and executive summary

The data book gives the following headline capabilities:

| Property | uPD72022 specification |
|---|---|
| Display modes | Text, semigraphics, graphics |
| Display split | Four horizontal screen areas |
| Scrolling | Smooth horizontal and vertical scrolling |
| Sprites | Attribute-based sprite display, grouping, collision and overflow status |
| Colors | 16-color output, 4 bits per dot |
| Attributes | Up to seven attributes |
| Maximum horizontal resolution | 640 dots |
| Maximum vertical resolution | 512 dots |
| Maximum dot rate | 22MHz |
| Addressable display memory | Up to 256K x 16-bit words |
| Memory support | DRAM refresh and optional dual-port RAM |
| Host transfers | Programmed I/O and DMA |
| Synchronization | Programmable CRT timing, external horizontal/vertical sync |
| Commands | 22 display, memory, and sprite commands |
| Process and supply | CMOS, single +5V supply |
| Packages | 80-pin plastic miniflat or 68-pin PLCC |

The maximum addressable display memory is 512KiB. This is a device limit, not a
statement that every PC-88VA exposes 512KiB of text/sprite RAM. The documented
VA configuration is 64KiB of TVRAM on VA and 256KiB on VA2/VA3; see
[`virtual-machine-architecture.md`](virtual-machine-architecture.md).

`[CONFIRMED]` The TSP emits one four-bit color code. Sprite pixels have
intrinsic priority over text pixels. PC-88VA board logic can then classify the
combined output into sprite-color and text-color classes and insert a graphics
screen between those classes in the external priority mixer.

`[DOCUMENTED]` The original VA native text system uses a 64KiB CPU-visible
TVRAM window divided into a 32KiB character-code side and a 32KiB attribute
side. Text buffers, four screen descriptors, 32 sprite descriptors, and sprite
patterns share this RAM. VA2/VA3 install 256KiB, but their complete TSP-side
decode is not fully reconstructed here.

`[CONFIRMED]` The sprite engine has 32 descriptors. Each sprite independently
selects packed 4-bpp color or 1-bpp monochrome pattern data, with programmable
position and size. Collision and per-scanline overflow detection are hardware
functions.

> [!IMPORTANT]
> The generic device limits and examples such as `256 x 192` or `512 x 192`
> do not establish PC-88VA system modes. PC-88VA material documents 200, 204,
> 400, and 408 active-line profiles. External 40/80-column circuitry changes
> horizontal text presentation independently of those generic examples.

## 6. Functional architecture

The data book divides the uPD72022 into three units:

1. **Host processor interface unit**: exchanges commands, parameters, data,
   and status; controls the asynchronous bus, DMA, and interrupts.
2. **Control processor unit**: decodes commands and operates on display data,
   display addresses, and screen-control state.
3. **Display control unit**: produces display-memory addresses, pixel output,
   CRT synchronization, blanking, and internal display timing.

This separation is useful for future vaeg work. Command decoding, display
memory operations, and raster scheduling should not be collapsed into one
immediate register-write side effect when BUSY, DMA, interrupt, or command
completion timing is guest-visible.

The PC-88VA uses the TSP for native text, sprites, and dynamic μPD3301
attribute expansion. The two graphics screens are produced by separate
graphics circuitry and are composited with the TSP output afterward. Normal
native software writes TVRAM directly through the CPU memory map; generic
`RDAT`, `WDAT`, `BLKTOT`, and `BLKTIN` operations are not required for that
path.

## 7. Command-set overview

The complete documented command set is:

| Category | Command | Code | Purpose |
|---|---|---:|---|
| Initialization | `SYNC` | `10h` | Select mode and define scan timing |
| Display | `DSPON` | `12h` | Enable the display controller |
| Display | `DSPOFF` | `13h` | Disable the display controller |
| Display | `DSPDEF` | `14h` | Define screen layout and display format |
| Display | `CURDEF` | `15h` | Define cursor format |
| Display | `ACTSCR` | `16h` | Select the active screen area |
| Display | `LPNR` | `1Ah` | Read the light-pen position |
| Display | `CURS` | `1Eh` | Move the cursor |
| Sprite | `SPRRD` | `80h` | Read sprite attributes |
| Sprite | `SPROV` | `81h` | Read collision and line-overflow status |
| Sprite | `SPRON` | `82h` | Enable sprite display and set its base/options |
| Sprite | `SPROFF` | `83h` | Disable sprite display |
| Sprite | `SPRWR` | `84h` | Write sprite attributes |
| Sprite | `SPRSW` | `85h` | Enable or disable one sprite |
| Memory | `EXIT` | `88h` | Terminate a display-memory operation |
| Memory | `MASK` | `89h` | Set the display-memory write mask |
| Memory | `DPRD` | `8Ah` | Read the display-memory data pointer |
| Memory | `DPLD` | `8Eh`/`8Fh` | Load a display-memory address or offset |
| Memory | `RDAT` | `90h`-`93h` | Read display memory through the host interface |
| Memory | `WDAT` | `94h`-`97h` | Write display memory through the host interface |
| Memory | `BLKTOT` | `99h`-`9Bh` | Transfer display memory to the host using DMA |
| Memory | `BLKTIN` | `9Dh`-`9Fh` | Transfer host data to display memory using DMA |

The multiple command codes select transfer width or data-pointer update
behavior. Their bit-level meanings and parameter formats should be taken from
the command descriptions on printed pages 3-78 through 3-88 rather than
inferred from this summary.

The PC-88VA adds `EMUL` (`8Ch`), a persistent μPD3301-compatible dynamic
attribute expansion mode that is absent from the generic data-book command
list. Generic names `DSPOF` and `SPROF` are commonly written `DSPOFF` and
`SPROFF` in PC-88VA material. Opcode `84h` is officially `SPRWR`; `SPWR` and
`SPRDEF` are implementation aliases.

## 8. PC-88VA relationship

Contemporary PC-88VA descriptions identify two display processors:

- a VDP for graphics operations;
- an IDP for text, display control, and sprites.

A surviving summary of the April 1987 *Monthly ASCII* PC-88VA article describes
the IDP as providing Japanese text and text-side sprites. A later October 1987
summary attributes screen splitting, smooth scrolling, display composition,
and up to 32 sprites to the combined VDP/IDP display architecture:

- [April 1987 PC-88VA article summary](https://eccmemberblog2.seesaa.net/article/2021-09-23.html)
- [October 1987 PC-88VA article summary](https://eccmemberblog2.seesaa.net/article/2022-01-21.html)

The published uPD72022 capabilities closely match the TSP command vocabulary
used by the VA ROM and current vaeg implementation. This is strong functional
evidence. It is not sufficient physical evidence to claim that the VA's custom
IDP and retail uPD72022 are electrically identical.

## 9. Address, byte order, and host I/O

### 9.1 Original-VA TVRAM address conversion

`[CONFIRMED]` The original VA CPU sees TVRAM as bytes while the TSP addresses
character, attribute, and sprite-pattern storage as 16-bit words. The two
physical 32KiB halves occupy non-contiguous TSP word ranges:

| Logical region | TSP word address | CPU byte offset | CPU address |
|---|---:|---:|---:|
| Character-code side | `0000h-3FFFh` | `0000h-7FFFh` | `A0000h-A7FFFh` |
| Attribute side | `8000h-BFFFh` | `8000h-FFFFh` | `A8000h-AFFFFh` |

~~~c
static uint32_t tsp_word_to_va1_cpu_offset(uint16_t address)
{
    if ((address & 0x8000u) != 0)
        return 0x8000u + ((uint32_t)(address & 0x3fffu) << 1);

    return (uint32_t)(address & 0x3fffu) << 1;
}
~~~

| TSP word | CPU bytes, high/low as printed | CPU byte base |
|---:|---:|---:|
| `0000h` | `0001h / 0000h` | `0000h` |
| `0001h` | `0003h / 0002h` | `0002h` |
| `3FFFh` | `7FFFh / 7FFEh` | `7FFEh` |
| `8000h` | `8001h / 8000h` | `8000h` |
| `BFFFh` | `FFFFh / FFFEh` | `FFFEh` |

Words are little-endian in CPU memory. `[UNKNOWN]` Original-VA behavior for
TSP word ranges `4000h-7FFFh` and `C000h-FFFFh` is not determined; an emulator
must not invent RAM or aliases there without a machine test.

The VA2 and VA3 install 256KiB of TVRAM and the current memory layer exposes a
larger CPU bank-1 window. The product capacity is documented, but the full
TSP-side page/address decode is not. Code should therefore keep the original
VA mapping helper separate from a future verified VA2/VA3 mapping.

### 9.2 Address units by structure

| Item | Address unit |
|---|---|
| CPU access to TVRAM | Byte address |
| Character, attribute, and sprite-pattern fetch | TSP word address translated to machine TVRAM |
| Sprite `SPDA` | TSP word address |
| `DSPON` table base | 256-byte page number |
| Screen-table `VSA` and `RSA` | 19-bit byte offset from TVRAM base |
| `DSPDEF.ATROFF` | Signed 19-bit byte offset |
| `DPLD` pointers | Generic 19-bit display-memory address |

`[DOCUMENTED]` Sprite X is a ten-bit modular coordinate and sprite Y is a
nine-bit modular coordinate: `-1` is encoded as `1023` and `511`
respectively. `[INFERENCE]` The ten-bit split-screen `RXP` should likewise be
sign-extended when negative horizontal placement is requested.

### 9.3 Host-visible ports

| Port | Direction | Specification |
|---:|---|---|
| `0142h` | Write | Command byte |
| `0142h` | Read | Status byte |
| `0146h` | Write | Parameter or streaming input byte |
| `0146h` | Read | Result or streaming output byte |

Before every command and every parameter byte, PC-88VA software tests status
mask `05h` (`IBF | BUSY`) and waits for zero. Output-producing commands set
`OBF`; software waits for `OBF=1` before reading `0146h`. Reading consumes the
byte and clears `OBF`. The generic data book warns that `READY` alone is not a
substitute for all read-command conditions.

### 9.4 Status register

| Bit | Mask | Generic name | PC-88VA name | Meaning |
|---:|---:|---|---|---|
| 7 | `80h` | `LP` | `LP` | Light-pen position latched; documented unavailable on PC-88VA |
| 6 | `40h` | `VB` | `VB` | Vertical blank active |
| 5 | `20h` | `SC` | `SC` | Sprite-over or collision condition latched |
| 4 | `10h` | `ER` | `ER` | Command, parameter-count, value, or sequence error |
| 3 | `08h` | Reserved | `EMEN` | PC-88VA μPD3301 expansion active |
| 2 | `04h` | `BUSY` | `BUSY` | Command processing active |
| 1 | `02h` | `OBF` | `OBF` | Unread output byte available |
| 0 | `01h` | `IBF` | `IBF` | Unconsumed command/parameter byte present |

`IBF` is set by a host write and clears when the command processor consumes
the byte. `OBF` is set when a result is produced and clears when the host reads
it; a new command clears a stale generic `OBF` condition. `BUSY` spans accepted
command work. Streaming commands require explicit parser/FIFO state and cannot
be modeled as unrelated immediate commands.

On malformed input, `ER` stops command processing until `EXIT` restores the
command-wait state. `SC` is updated at vertical blank and is cleared by
`SPROV`. `EMEN` is a PC-88VA extension that tracks persistent `EMUL`
expansion.

## 10. Detailed command behavior

### 10.1 `SYNC` (`10h`)

`SYNC` accepts 14 bytes, stops display-controller operation, and installs the
memory-interface, raster, interrupt, clock, and CRT timing configuration.

| Byte | Bits | Field | Meaning |
|---:|---:|---|---|
| 0 | 7:6 | `RM` | Raster mode |
| 0 | 5 | `EL` | Light-pen interrupt enable |
| 0 | 4 | `EV` | Vertical-blank interrupt enable |
| 0 | 3 | `VM` | Static-picture access: 0 random, 1 serial |
| 0 | 2 | - | Reserved, zero |
| 0 | 1 | `DPM` | Dual-port selector |
| 0 | 0 | `ILM` | Interleave selector |
| 1 | 7 | - | Reserved, zero |
| 1 | 6 | `RF` | VRAM refresh enable |
| 1 | 5 | `EC` | Internal divided dot-clock output when set |
| 1 | 4 | `ES` | Output HSYN/VSYN when set; otherwise use external sync |
| 1 | 3 | `RV` | Global text foreground/background reverse |
| 1 | 2:0 | `RS` | Dot-time divider/resolution selector |
| 2 | 5:0 | `LBL` | Left blanking |
| 3 | 5:0 | `LBR` | Left border |
| 4 | 7:0 | `HAD` | Horizontal active display |
| 5 | 5:0 | `RBR` | Right border |
| 6 | 5:0 | `RBL` | Right blanking |
| 7 | 5:0 | `HS` | Horizontal sync |
| 8 | 5:0 | `TBL` | Top blanking |
| 9 | 5:0 | `TBR` | Top border |
| 10 | 7:0 | `VAD[7:0]` | Vertical active display low byte |
| 11 | 6 | `VAD[8]` | Vertical active display high bit |
| 11 | 5:0 | `BBR` | Bottom border |
| 12 | 5:0 | `BBL` | Bottom blanking |
| 13 | 5:0 | `VS` | Vertical sync |

The memory-interface combinations are:

| DPM | ILM | Mode |
|---:|---:|---|
| 0 | 0 | Standalone |
| 0 | 1 | Interleave; used by documented PC-88VA profiles |
| 1 | 0 | Dual-port, valid only with `VM=1` |
| 1 | 1 | Invalid/disabled |

Raster modes are:

| RM | Generic name and example |
|---:|---|
| `00b` | Noninterlace, canonical 640x400/24kHz example |
| `01b` | Interlace, canonical 640x400/15kHz example |
| `10b` | Vertical magnification, canonical 640x200/24kHz example |
| `11b` | Normal, canonical 640x200/15kHz example |

These names describe raster and display-address progression, not a complete
PC-88VA mode register. The machine combines `RM` with explicit timing,
external clock/sync selection, and sprite-line interpretation.

The generic 20MHz source-clock examples are:

| RS | Divider | Example | HAD | VAD |
|---:|---:|---:|---:|---:|
| `000b` | /4 | 256x192 | 63 | 192 |
| `001b` | /3 | 320x200 | 79 | 200 |
| `010b` | /2 | 512x192 | 127 | 192 |
| `011b` | /1.5 | 640x200 | 159 | 200 |
| `100b` | /1 | 640x400 | 159 | 400 |
| other | disabled | no dot-clock output | - | - |

Documented PC-88VA vectors use `RS=111b` with external dot-clock input, so
this table is not a list of machine modes.

For the generic device, each horizontal interval is documented as
`(field + 1) * TCK` and each TCK is four dots:

~~~text
H_total_TCK =
    (LBL + 1) + (LBR + 1) + (HAD + 1) +
    (RBR + 1) + (RBL + 1) + (HS + 1)
H_total_dots = H_total_TCK * 4
~~~

Generic constraints include `HS >= 04h`, `LBL >= 03h`, odd `HAD`,
`VS >= 04h`, `VAD >= 4` and even, `BBR + BBL >= 2`, and top blank/border
totals of at least `10h` without sprites or `20h` with sprites.

`[CONFLICT]` vaeg's real-machine-fit comment omits `+1` for `LBR` and `RBR`:

~~~text
H_total_TCK_PC_candidate =
    (LBL + 1) + LBR + (HAD + 1) +
    RBR + (RBL + 1) + (HS + 1)
~~~

This remains a measurable PC-88VA timing hypothesis, not part of the generic
formula.

### 10.2 Documented PC-88VA `SYNC` vectors

These 14-byte vectors are copied from the Tekumani material:

| Lines | H profile | Raster/sprite interpretation | Bytes |
|---:|---:|---|---|
| 200 | 15.98kHz | Noninterlace, 200-line sprites | `C1 57 1C 00 9F 00 10 0F 25 00 C8 00 0F 08` |
| 200 | 15.73kHz | Normal raster, 200-line sprites | `C1 47 1C 00 9F 00 12 11 24 00 C8 00 17 04` |
| 200 | 24.8kHz | 200-line vertically magnified | `81 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 204 | 15.98kHz | Noninterlace, 204-line sprites | `C1 57 1C 00 9F 00 10 0F 23 00 CC 00 0D 08` |
| 204 | 15.73kHz | Normal raster, 204-line sprites | `C1 47 1C 00 9F 00 12 11 22 00 CC 00 15 04` |
| 204 | 24.8kHz | 204-line vertically magnified | `81 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |
| 400 | 15.73kHz | Interlace, 200-line sprite data | `41 47 1C 00 9F 00 12 11 24 00 C8 00 17 04` |
| 400 | 24.8kHz | Noninterlace, 200-line sprite data | `01 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 400 | 24.8kHz | Noninterlace, 400-line sprite data | `C1 57 10 00 9F 00 10 0F 19 00 90 40 07 08` |
| 408 | 15.73kHz | Interlace, 200-line sprite data | `41 47 1C 00 9F 00 12 11 22 00 CC 00 15 04` |
| 408 | 24.8kHz | Noninterlace, 200-line sprite data | `01 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |
| 408 | 24.8kHz | Noninterlace, 400-line sprite data | `C1 57 10 00 9F 00 10 0F 19 00 98 40 07 08` |

An emulator must decode the raw bytes rather than infer TSP behavior only from
the human-readable profile. In particular, the 15.98kHz and 15.73kHz profiles
must not be collapsed until tests prove that software cannot observe their
VRTC, raster, or monitor-selection differences.

### 10.3 Display-definition commands

`DSPON` (`12h`) accepts a screen-table page low byte, upper page bits, and a
four-bit backdrop color. The effective generic byte address is
`page_number * 256`. The conventional original-VA table at `7F00h` uses
parameters `7Fh, 00h`; the third byte is normally zero because board-level
composition supplies the backdrop. `DSPON` starts text; sprites require a
separate `SPRON`.

`DSPOF/DSPOFF` (`13h`) has no parameters and stops both text and sprite
controllers while retrace timing continues. To hide only text while retaining
sprites, software uses board port `0148h.TD`.

`DSPDEF` (`14h`) accepts six bytes:

| Byte | Encoding |
|---:|---|
| 0 | `ATROFF[7:0]` |
| 1 | `ATROFF[15:8]` |
| 2 | `PITCH[2:0]` in bits 6:4; `ATROFF[18:16]` in bits 2:0 |
| 3 | `MRA` in bits 4:0 |
| 4 | `HRA` in bits 4:0 |
| 5 | `BR` in bits 7:3 |

`ATROFF` is a signed 19-bit byte displacement. Native original-VA text usually
uses `8000h`. Encoded `PITCH=0` disables it; `1..7` means a one- through
seven-byte pitch, with `2` normal for native character words. `MRA+1` is the
character height; `MRA=07h, 0Fh, 1Fh` select 8, 16, or 32 rasters, while
`00h..06h` disable character display. `HRA=0` is the first raster and
underline uses the maximum raster.

For `BR=1..31`, attribute blink is bright for `BR*24` fields and dark for
`BR*8`; cursor phases are each `BR*8` fields. Generic `BR=0` wraps to 32:
768/256 attribute fields and 256/256 cursor fields.

### 10.4 Cursor, active screen, and light pen

`CURDEF` (`15h`) accepts one byte: cursor sprite number in bits 7:3, cursor
enable in bit 1, and blink enable in bit 0. `CE` is retained as cursor state;
it is independent of the descriptor's ordinary sprite switch (`SW`). When
enabled, the selected descriptor supplies the cursor image and position even
if its `SW` bit is clear. `SPRON` and the descriptor contents must still be
valid.

`ACTSCR` (`16h`) accepts the split number in bits 6:5 (equivalently
`split * 32`) and selects the virtual coordinate space used by `CURS` and
`LPNR`.

`CURS` (`1Eh`) accepts Y low/high followed by X low/high. The generic device
clamps coordinates beyond the virtual screen's lower or right edge and updates
`DPTR0` to the corresponding memory address.

`LPNR` (`1Ah`) returns Y then X in the same four-byte order, clears `LP`, and
updates `DPTR0`. PC-88VA material marks light-pen use unavailable, so the board
profile may expose a stable unasserted state pending verified hardware.

### 10.5 `EMUL` (`8Ch`), PC-88VA extension

`EMUL` accepts:

1. split number multiplied by 32;
2. character count;
3. μPD3301 attribute count;
4. row count.

`[DOCUMENTED]` The TSP dynamically expands μPD3301-style character/attribute
rows into native TSP attribute interpretation. Expansion persists until
another command or `EXIT` terminates it, and `EMEN` reports the state. A
conforming implementation must not turn this into a one-shot memory rewrite
without contrary hardware evidence.

`[UNKNOWN]` Malformed row data, attribute-count overflow, and exact state reset
at split boundaries require N88 V1/V2 hardware tests.

### 10.6 `EXIT` and generic display-memory commands

`EXIT` (`88h`) aborts partial parameter acceptance, terminates a streaming
display-memory command, clears generic error state, and terminates `EMUL`. It
does not necessarily cancel a non-stream command after every parameter has
already been accepted.

`DPLD` (`8Eh/8Fh`) loads a 19-bit `DPTR0` or `DPTR1`; `DPTR1` may be a signed
two's-complement increment. `DPRD` (`8Ah`) returns `DPTR0` as three bytes.
`MASK` (`89h`) loads the 16-bit write mask:

~~~c
new_value = (old_value & (uint16_t)~mask) | (input_value & mask);
~~~

The transfer opcode's `MOD` selects pointer update:

| MOD | Update after access | Host stream | DMA stream |
|---:|---|---|---|
| `00b` | None | Enabled | Disabled |
| `01b` | Add `ATROFF` | Enabled | Enabled |
| `10b` | Add `PITCH` | Enabled | Enabled |
| `11b` | Add signed `DPTR1` | Enabled | Enabled |

`RDAT` (`90h-93h`) and `WDAT` (`94h-97h`) continuously read or masked-write
words through the host port. `BLKTOT` (`99h-9Bh`) and `BLKTIN` (`9Dh-9Fh`)
are DMA equivalents. `[UNKNOWN]` The abbreviated surviving material does not
make every 16-bit stream byte order sufficiently explicit for conformance
tests; the existing low-byte-first PC convention should remain isolated in a
testable helper.

### 10.7 Sprite commands

`SPRON` (`82h`) accepts the sprite-table page low byte, upper page bits, and:

| Byte 2 bits | Field | Meaning |
|---:|---|---|
| 7:3 | `HSPN` | Per-scanline accepted sprite threshold |
| 2 | `ESP` | Generic sprite interrupt enable |
| 1 | `MG` | Vertically double every sprite |
| 0 | `GR` | Detect collision only between different color groups |

The conventional table at `A7E00h` uses page `7Eh` relative to the original
VA TVRAM window. Sprite-over occurs when more than `HSPN+1` descriptors occupy
a scanline. `[UNKNOWN]` Generic `ESP` wiring to a usable PC-88VA interrupt is
not established.

`SPROF/SPROFF` (`83h`) stops sprites and therefore also the cursor.
`SPRSW` (`85h`) selects sprite number in bits 7:3 and writes descriptor enable
from bit 1.

`SPRRD` (`80h`) and `SPRWR` (`84h`) begin with a sprite/attribute-byte
selector, then stream descriptor bytes 0 through 7 and continue into the next
descriptor. The byte sequence is Y low; Y-size/enable/Y high; X low;
X-size/mode/X high; pattern address low/high; color control; reserved.

`SPROV` (`81h`) returns:

| Bit | Field | Meaning |
|---:|---|---|
| 6 | `SO` | Sprite-over detected |
| 5 | `C/CD` | Collision detected |
| 4:0 | `OVS` | First sprite number exceeding `HSPN` |

The read clears `SC`. PC-88VA material requires `SPROV` within 160
microseconds after VRTC becomes active for valid overflow detail.

## 11. TVRAM and screen-control table

### 11.1 Installed memory and conventional original-VA layout

The original VA CPU window is `A0000h-AFFFFh` after the system-memory bank
selects TVRAM. That bank selection is outside the TSP.

| CPU address | Size | Conventional use |
|---:|---:|---|
| `A0000h-A7FFFh` | 32KiB | Character-code side, tables, optional patterns |
| `A8000h-AFFFFh` | 32KiB | Attribute side, optional patterns |
| `A7E00h-A7EFFh` | 256B | 32 sprite descriptors |
| `A7F00h-A7F7Fh` | 128B | Four 32-byte screen entries |
| `A7F80h-A7FFFh` | 128B | Reserved portion of the 256-byte control page |

Unused space is shared by frame buffers, sprite/cursor/mouse patterns, and
documented BIOS work. A sprite pattern may be entirely on either side of the
`A8000h` physical boundary but must not cross it.

### 11.2 Four screen descriptors

`DSPON` selects a 256-byte-aligned page. Four consecutive 32-byte entries
describe horizontal split screens:

| Offset | Bits | Field | Meaning |
|---:|---:|---|---|
| `00h` | 18:0 | `VSA` | Virtual-buffer start byte address |
| `04h` | 10:0 | `VH` | Virtual height in character rows |
| `08h` | 9:0 | `VW` | Virtual width in bytes |
| `0Ah` | 15:12 | `BG` | Screen background |
| `0Ah` | 11:8 | `FG` | Screen foreground |
| `0Ah` | 4:0 | `MODE` | PC-88VA attribute mode 0 through 5 |
| `0Ch` | 12:8 | `RASTER_OFFSET` | Vertical smooth-scroll offset |
| `10h` | 18:0 | `RSA` | Displayed source byte address |
| `14h` | 8:0 | `RH` | Real split height in rasters |
| `16h` | 9:0 | `RW` | Real split width in dots |
| `18h` | 8:0 | `RYP` | Real split Y in rasters |
| `1Ah` | 9:0 | `RXP` | Real split X in dots/modular signed form |

The unused words at `06h`, `0Eh`, `1Ch`, and `1Eh` and all reserved bits are
zero. All fields are little-endian. `VSA`/`RSA` high bits are in the low three
bits of the following word.

~~~text
virtual_row(y) = VSA + y * VW
cell(x, y)     = virtual_row(y) + x * 2
~~~

`RSA` provides coarse scrolling. `RASTER_OFFSET` moves the first source row
up by 0 through 31 rasters; larger vertical motion advances `RSA` by `VW` and
reduces the offset modulo character height. `RXP` provides sub-cell horizontal
placement.

The real strips should be contiguous. `RH` is even and at least 16; their sum
normally covers the active text height. `RYP` is even. `RW` is a multiple of
32. `RW=0` with `RXP=03F0h` is the documented single-split blank convention.

`[DOCUMENTED][CONFLICT]` The PC-88VA manual says that `RW/8 + 2` cells are
fetched/displayed. Those extra cells plausibly cover scrolling edges, but the
exact fetch/clipping rule is not defined. A renderer should fetch sufficient
edge coverage and clip every output pixel to the real split rectangle.

If the source rectangle leaves the virtual buffer, the TSP does not wrap.
`[UNKNOWN]` The emitted value outside the buffer is not specified; transparent
output is a testable initial policy, not a confirmed hardware rule.

## 12. Native text engine

### 12.1 Cell fetch and hardware character codes

For every source cell, the renderer:

1. fetches a 16-bit hardware character code;
2. adds signed `ATROFF` to obtain the attribute word;
3. decodes its low byte according to the split's `MODE`;
4. selects ANK, Kanji, or RAM-character pattern data;
5. applies shape, blink, color, reverse, and geometry; and
6. clips the result to the real split rectangle.

Native defaults are `PITCH=2`, `ATROFF=8000h`, an eight-dot body, and commonly
`MRA=0Fh` for a 16-raster cell.

A zero high byte denotes a single-byte ANK code. The two-byte Japanese
hardware code is:

~~~text
bit 15     half: 0=left, 1=right
bits 14:8  JIS second byte
bit 7      0
bits 6:0   JIS first byte - 20h
~~~

A full-width character occupies a left/right pair of eight-dot cells.
MAME's surviving Kanji-ROM address transform is an implementation detail and
must be tested independently from this cell format.

### 12.2 Font and 40/80-column modes

Board port `0148h.ANKM` selects an 8x16 ANK body when clear and 8x8 when set.
Japanese full-width output is documented unavailable in 8x8 mode.

Port `0030h` bit 0 (`80CM`) selects 40 or 80 text columns. Forty-column mode is
external horizontal pixel doubling, not the TSP `DWID` attribute. In native V3
mode, the even word supplies the enlarged left half and the odd word the right
half; software stores the same character in both words. Sprite coordinates
remain on the 640-dot basis in either mode.

### 12.3 Attribute storage and shape controls

Each character has a 16-bit attribute word, but native software uses the low
byte and writes zero to the high byte.

| Control | Meaning |
|---|---|
| `SECRET` | Suppress the glyph |
| `BLINK` | Show or hide by programmed blink phase |
| `REVERSE` | Swap cell foreground/background |
| `HLINE` | Draw at `HRA` |
| `ULINE` | Draw at the maximum raster |
| `DWID` | Horizontally double one selected glyph half |
| `DWIDC` | Select left (0) or right (1) half for `DWID` |

### 12.4 Attribute modes 0 through 5

Mode 0:

~~~text
bits 7:4  background 0..15
bits 3:0  foreground 0..15
~~~

Mode 1:

~~~text
bits 7:4  foreground
bit 3     HLINE
bit 2     REVERSE
bit 1     BLINK
bit 0     SECRET
~~~

Its background comes from the screen table. Mode 2 uses both screen-table
colors:

~~~text
bit 7  DWIDC
bit 6  DWID
bit 5  ULINE
bit 4  HLINE
bit 3  reserved
bit 2  REVERSE
bit 1  BLINK
bit 0  SECRET
~~~

Mode 3 is a stateful stream. A color record has bit 3 set:

~~~text
bits 7:4  foreground 0..15
bit 3     1
bits 2:0  background 0..7
~~~

A shape record has bit 3 clear and uses the mode-2 shape bits. Color and shape
states persist independently until another record of the same type. A single
attribute byte cannot update both.

`[UNKNOWN]` Mode-3 reset and persistence at row, split, `RSA`, and frame
boundaries are not completely specified. The policy must be explicit and
test-covered.

Modes 4 and 5 share:

~~~text
bit 7     BLINK
bits 6:4  background 0..7
bits 3:0  foreground 0..15
~~~

Mode 5 additionally asserts `HLINE` when `(foreground & 7) == 1`, so colors 1
and 9 select the line.

Recommended application order is: select/update color and mode-3 state; fetch
the raw glyph; apply secret/blink visibility; apply per-cell reverse and
`SYNC.RV`; add HLINE/ULINE foreground pixels; apply `DWID/DWIDC` geometry;
then apply external transparency and class separation.

`[UNKNOWN]` Ordering of line pixels against every secret/reverse combination
requires a hardware discriminator test.

## 13. Sprite engine

### 13.1 Descriptors, ownership, and priority

There are 32 eight-byte descriptors. Lower descriptor addresses have higher
priority, so sprite 0 is highest and a software overwrite renderer draws 31
down to 0. Every opaque sprite pixel has intrinsic priority over TSP text.

Hardware `CURDEF.CURN` may select any sprite as cursor. Sprite BIOS policy
reserves 0 for the mouse, 1 through 30 for callers, and 31 for the text cursor;
this is not a silicon restriction.

| Offset | Bits | Field | Meaning |
|---:|---:|---|---|
| `00h` | 15:10 | `YSIZE` | Encoded vertical size |
| `00h` | 9 | `SW` | Enable |
| `00h` | 8:0 | `YP` | Y coordinate |
| `02h` | 15:11 | `XSIZE` | Encoded horizontal size |
| `02h` | 10 | `MD` | 0 packed color, 1 monochrome |
| `02h` | 9:0 | `XP` | X coordinate |
| `04h` | 15:0 | `SPDA` | Pattern TSP word address |
| `06h` | 7:4 | `FG` | Monochrome foreground |
| `06h` | 3 | `BC` | Zero bit: transparent (0) or color 8 (1) |

Reserved bits are zero.

### 13.2 Size and pattern formats

Both modes use:

~~~text
height_pixels = 4 * (YSIZE + 1)
row_bytes     = 4 * (XSIZE + 1)
pattern_bytes = row_bytes * height_pixels
~~~

Packed color width is `8 * (XSIZE + 1)`, from 8 through 256 dots. Each byte's
high nibble is the left pixel and low nibble the right; color 0 is transparent.

Monochrome width is `32 * (XSIZE + 1)`. Bits are MSB first; one emits `FG`,
zero emits transparent for `BC=0` or color 8 for `BC=1`.

`[CONFLICT]` The descriptor field can encode monochrome widths through 1024,
while a later capability section states a practical 32-through-256-dot range.
Decode the full field but retain a hardware-limit conformance test.

`SPDA` uses the word-to-byte translation in section 9. A pattern must not cross
the original-VA `A8000h` physical boundary.

### 13.3 Position, clipping, and magnification

The sprite coordinate system is always 640 dots horizontally. Left, right, and
bottom overflow are clipped. Top overflow is asymmetric: the manual directs
software to advance `SPDA` to the first visible source row while retaining
`YSIZE`. An emulator must preserve this rule until hardware disproves it.

The PC-88VA documents 200/204- and 400/408-line presentations. Interlace raster
mode 1 cannot use a true 400-line sprite pattern. `SPRON.MG` vertically doubles
every sprite to reuse 200-line data in a 400-line display.

### 13.4 Per-line limit, overflow, and collision

The hardware describes 32 sprites, but scanline capacity also depends on
descriptor, pattern, text, refresh, CPU/SGP, and command memory traffic. The
manual gives approximately 256 total packed-color sprite dots per line as a
normal capacity.

`HSPN` is a separate object-count threshold. More than `HSPN+1` sprites on one
line sets `SC`/`SO` and records the first excess sprite in `OVS`.

Collision rules are:

1. only active-display pixels participate;
2. both pixels must be opaque;
3. without grouping, all sprite pairs can collide;
4. with grouping, only different groups collide;
5. the first collision in a frame is latched; and
6. `SC` updates at vertical blank.

Emitted colors 1 through 7 are group 0, 8 through 15 group 1, and 0 is
transparent. Packed-color grouping can vary per pixel. The manual describes
monochrome grouping as per sprite, but does not unambiguously resolve a sprite
whose `FG` and opaque `BC=1` background fall in different groups. Using the
foreground high bit is only a hypothesis.

## 14. Board-level composition

These controls are outside the generic μPD72022 command registers.

### 14.1 Text control and model-sensitive TVRAM mode

Port `0148h` contains:

| Bit | Name | Baseline meaning |
|---:|---|---|
| 7 | `TD` | Disable text while sprites may remain visible |
| 6:4 | `VALT` | TVRAM access limit; zero unrestricted, nonzero documented as value times four accesses |
| 3 | `ATM` | 0 native V3, 1 V1/V2 compatibility |
| 2 | `ANKM` | 0 8x16 ANK, 1 8x8 ANK |
| 1 | `TVWM` | 0 byte organization, 1 word organization |
| 0 | model-sensitive | Written as 1 in the original-VA profile; vaeg interprets 0 as an unsupported 256KiB TVRAM mode |

`VALT` is not the CPU TVRAM bank selector. `[UNKNOWN]` Its access-accounting
interval remains unresolved. Bit 0 must not be globally labeled fixed across
the family: the original-VA material and vaeg's later-model comment need a
model-specific test.

### 14.2 Color classes, priority, and transparency

Port `0111h` upper nibble supplies boundary `N`:

- colors 1 through `N` become the sprite class;
- colors `N+1` through 15 become the text class;
- color 0 is transparent;
- `N=0` disables the sprite class;
- `N=15` disables the text class.

This classification does not change intrinsic sprite-over-text priority.

Each nibble of port `0106h` assigns palette priority slots 0 through 3, with
slot 0 highest:

| Value | Source |
|---:|---|
| `0h` | Disabled |
| `8h` | Text class |
| `9h` | Sprite class |
| `Ah` | Graphics screen 0 |
| `Bh` | Graphics screen 1 |

Direct-color slots 4 and 5 are controlled separately at `0108h` and apply to
graphics screens.

Port `012Eh` supplies one transparency bit per TSP color: one means externally
transparent. Color 0 remains transparent regardless of software programming.
Palette and palette-bank controls map TSP color codes to final RGB and belong
in the board mixer, not descriptor decoding.

### 14.3 Resolution ownership and viewport rule

The complete cross-block control map, including ports `0100h`, `0102h`, the
four framebuffer descriptors at `0200h-027fh`, SGP pitch, and worked mode
relationships, is reconstructed in
[PC-88VA Video-Mode and Framebuffer Control](pc88va-video-modes.md).

The generic μPD72022 can generate examples such as 256x192, 320x200,
512x192, 640x200, and 640x400 by changing `SYNC.RS`, `HAD`, and `VAD`.
That does not establish those as complete PC-88VA machine modes.

The known PC-88VA organization is:

- the TSP normally uses an external dot clock;
- graphics horizontal selection at port `0102h` is 640 or 320;
- native vertical profiles at port `0100h` are 200, 204, 400, or 408;
- D65101-side composition combines TSP, graphics screen 0, and graphics
  screen 1 under a common display timing; and
- sprite coordinates and external class boundaries remain in the common
  640-dot coordinate system.

Therefore, "`uPD72022 alone can generate 256x192`" is not evidence of an
official 256x192 PC-88VA mode composited with GVRAM. A TSP-only 256-dot active
width risks disagreement with graphics fetch, sprite coordinates, and mixer
boundaries.

Arbitrary logical sizes should initially be treated as a viewport inside a
documented physical raster. For example:

~~~text
physical raster      640 x 400
graphics source rows 640 dots wide
content rectangle    384 x 256
display position     (128, 72)
outside area         backdrop or transparent
~~~

The safe first test retains a 640-dot source pitch, stores the 384-dot content
at X=128 in each row, and uses framebuffer `DSP`/`DSH` for the 72-line vertical
offset and 256-line visible height. No proven graphics destination-X register
equivalent to TSP `RXP` has been identified; `OFX` is a source offset. A tightly
packed 384-dot pitch is a later discriminator test, not a documented native
fetch width. TSP `SYNC` remains at the known 640x400 timing.

The first discriminator target should be 320x200 represented as a logical
screen enlarged 2x2 within 640x400 output. Once that works, tests can separate
SGP pitch, GVRAM read format, `0102h` horizontal scaling, `0100h` vertical
mode, TSP timing, and D65101 composition without introducing an unknown sync
frequency.

## 15. Initialization and required state

A robust native sequence is:

1. reset command, status, and output state;
2. select the monitor/scan profile;
3. send one documented `SYNC` vector;
4. program complete `DSPDEF` state;
5. initialize four screen descriptors;
6. initialize sprite descriptors and patterns;
7. select cursor state if required;
8. issue `SPRON` and `DSPON`;
9. program board text mode, 40/80 mode, color boundary, transparency, palette,
   and priority; and
10. update TVRAM in safe periods or under a verified contention policy.

A complete core state includes command/parser buffers, input/output flags,
display and sprite enables, persistent `EMUL` state, all 14 `SYNC` bytes,
19-bit table/pointer state, signed `ATROFF`, pitch/raster/blink state, active
split, cursor state, sprite limits and grouping, collision/overflow latches,
`DPTR0`/`DPTR1`, and the write mask. The PC-88VA wrapper separately owns
`TD`, `VALT`, `ATM`, `ANKM`, `TVWM`, 40/80 mode, boundary, transparency,
priority, and palette selection.

## 16. Generic device versus PC-88VA profile

| Area | Generic μPD72022 | PC-88VA profile |
|---|---|---|
| Display paths | Text, semigraphics, graphics, sprites | Native text, sprites, and `EMUL`; graphics is separate |
| Addressable memory | Up to 256K x 16-bit words | 64KiB installed on VA, 256KiB on VA2/VA3 |
| Mode examples | 256x192 through 640x400 examples | 200/204/400/408 active lines; external 40/80 columns |
| Light pen | Supported | Documented unavailable |
| Opcode `8Ch` | Not in generic list | Persistent `EMUL` |
| Status bit 3 | Reserved | `EMEN` |
| Backdrop | `DSPON.BC` | Normally board-level composition |
| Host VRAM access | Commands, DMA, arbitration | Normal software maps TVRAM directly |
| Text/sprite output | One four-bit stream | External boundary splitter creates two priority classes |

## 17. MAME implementation audit

`[IMPLEMENTATION]` The examined MAME source recognizes `SYNC`, `DSPON`,
`DSPOFF`, `DSPDEF`, `CURDEF`, `ACTSCR`, `CURS`, `EMUL`, `EXIT`, `SPRON`,
`SPROFF`, `SPRSW`, `SPROV`, and opcode `84h` under `SPWR`. It partially
decodes four splits, text, Kanji/ANK, sprites, and external priority.

Known gaps and divergences are:

1. status read exposes only `VB` and omits `IBF`, `OBF`, `BUSY`, `ER`,
   `EMEN`, `SC`, and `LP`;
2. `SPRRD`, `LPNR`, `MASK`, `DPLD`, `DPRD`, `RDAT`, `WDAT`, `BLKTOT`, and
   `BLKTIN` are TODOs;
3. screen-table FG/BG assignment appears reversed relative to the documented
   high-background/next-foreground nibbles;
4. `VSA` is not fully integrated into address bounds;
5. mode 3 is not implemented as independent persistent color/shape state;
6. blink, double width, underline, and horizontal line are incomplete;
7. collision, overflow, `SC` timing, and `SPROV` are incomplete; and
8. several priority/palette paths remain unverified.

MAME is a regression reference for booting software, not the sole
specification.

## 18. Current vaeg implementation

### Ownership and rendering path

The active implementation is divided as follows:

| File | Current responsibility |
|---|---|
| [`io/tsp.c`](../../io/tsp.c) | Command/parameter ports, TSP state, SYNC decoding, raster timing |
| [`io/tsp.h`](../../io/tsp.h) | TSP state shared with the display and renderer paths |
| [`vram/maketextva.c`](../../vram/maketextva.c) | Text and attribute rendering from `textmem` |
| [`vram/makesprva.c`](../../vram/makesprva.c) | Sprite table interpretation and sprite rendering |
| [`machine/pccore.c`](../../machine/pccore.c) | Display/VBlank event scheduling and blink progression |
| [`io/sysportva.c`](../../io/sysportva.c) | System-port view of the display synchronization state |

The guest-facing TSP and the host renderer are separate. SDL2 effects and
window scaling must not alter TSP timing or guest display state.

### VA I/O mapping

`tsp_bind()` currently exposes:

| VA port | Direction | Current use |
|---:|---|---|
| `0142h` | Read | Status, including BUSY/VBlank approximation |
| `0142h` | Write | Command byte |
| `0143h` | Read | Unimplemented; returns `FFh` |
| `0146h` | Write | Command parameter or sprite-attribute data |
| `0146h` | Read | Not exposed by the current vaeg binding; result/streaming reads remain unimplemented |

These are PC-88VA board-level I/O assignments. They are not package pin
numbers or universal uPD72022 host addresses. The VA ROM boot sequence writes
commands and parameters through this interface as described in
[`pc88va-boot-sequence.md`](pc88va-boot-sequence.md).

### Implemented command coverage

The command decoder currently provides functional handling for:

- `SYNC` (`10h`);
- `DSPON` (`12h`);
- `DSPDEF` (`14h`);
- `CURDEF` (`15h`);
- `SPRON` (`82h`);
- `SPRWR`-like sequential sprite-table writes under the local name `SPRDEF`
  (`84h`);
- `SPRSW` (`85h`), which updates one descriptor's enable bit;
- `EXIT` (`88h`).

`DSPOFF` (`13h`) and `SPROFF` (`83h`) are implemented as no-parameter state
transitions. `DSPOFF` disables both TSP text and sprite output; `SPROFF`
disables sprite output while leaving text state unchanged. `SPRSW` (`85h`) is
implemented as a one-parameter descriptor-enable update. All other commands
fall through the unknown-command path. The implementation therefore does not
yet model active-screen selection, cursor movement, light-pen reads, sprite
status/read operations, display-memory programmed I/O, DMA, masking, or
controller interrupts. This coverage statement is intentionally separate from
the generic data-book command list: a command being documented by NEC does not
establish that the PC-88VA wrapper has implemented it.

The 22-command data-book chapter is useful not only as background but as a
concrete completeness checklist for the TSP core.

### Cross-source coverage matrix

The following matrix separates the generic μPD72022 programming model from the
PC-88VA subset documented by BNN and from the current vaeg implementation. The
generic source contains OCR/name variants (and an inconsistent summary entry)
such as `DSPOF`/`DSPOP`; use opcode `12h`/`13h` and the BNN names
`DSPON`/`DSPOFF` when resolving them. `SPRAD`/`SPWR` are the corresponding
`SPRRD`/`SPRWR` naming variants.
The BNN table is the VA-specific authority for the normal PC-88VA path; an
omitted generic command is not automatically a VA defect when native software
uses the CPU TVRAM map instead of the generic host-transfer interface.

| Command | Generic `uPD72022.md` | PC-88VA BNN material | Current vaeg state | Classification |
|---|---|---|---|---|
| `SYNC (10h)` | 14-byte mode/timing setup | 14-byte TSP initialization | Accepts all 14 bytes, but only selected timing/display fields affect behavior | Partial |
| `DSPON (12h)` | Screen-table base plus backdrop color | 3 bytes; VA table uses the high address byte and zero reserved bytes | Enables text and stores only the first address byte; backdrop and wider address profiles are not modeled | Partial |
| `DSPOFF (13h)` | Stop display and sprite controller | Stops text and sprites | Disables text and sprites | Basic VA match |
| `DSPDEF (14h)` | Attribute offset, pitch, MRA, HRA, blink | Six-byte screen-format definition | Basic offset/height/line/blink handling; `PITCH`, full field masks, and wider offsets are incomplete | Partial |
| `CURDEF (15h)` | Cursor sprite, CE, BE | Cursor sprite, cursor enable, blink enable | Retains CURN, CE, and BE separately from descriptor SW | Basic VA match |
| `ACTSCR (16h)` | Select active split | Select cursor split screen | Unknown command path | Missing |
| `CURS (1Eh)` | Set virtual Y/X and update cursor address | Set cursor Y then X | Unknown command path | Missing |
| `LPNR (1Ah)` | Read light-pen position | VA material marks light pen unavailable | Not implemented | Intentional VA omission pending evidence |
| `EMUL (8Ch)` | Not in generic list | Dynamic μPD3301 attribute expansion, used by V1/V2 | Unknown command path | Missing |
| `EXIT (88h)` | Abort memory operation / clear error | Abort partial command and stop EMUL | Clears BUSY only | Partial |
| `SPRON (82h)` | Enable sprites and set table/options | Table address, `HSPN`, `MG`, `GR` | Basic table/options and enable state; no overflow/collision result path | Partial |
| `SPROFF (83h)` | Stop sprite controller | Stop sprites and cursor sprite | Disables sprites | Basic VA match |
| `SPRSW (85h)` | Toggle one sprite | Toggle one sprite | Updates one descriptor enable bit | Basic VA match |
| `SPRRD (80h)` | Read sprite attributes | Not listed in the BNN VA command subset | Unknown command path | Generic command missing |
| `SPRWR (84h)` | Selector plus sequential attribute stream | Not listed in the BNN VA command subset | A sequential write exists under local name `SPRDEF`, but selector/stream semantics are not complete | Partial generic match |
| `SPROV (81h)` | Read overflow/collision status | Read sprite-over/collision information | Unknown command path | Missing |
| `MASK`, `DPLD`, `DPRD`, `RDAT`, `WDAT`, `BLKTOT`, `BLKTIN` | Generic display-memory and DMA operations | Not part of the normal BNN VA command subset | Unknown command path | Generic commands missing; separate VA scope |

The BNN command list is at [`PC88VA_テクニカルマニュアル_BNN.md`](../tekumani/PC88VA_テクニカルマニュアル_BNN.md), while the generic command list and command semantics are in [`uPD72022.md`](../tekumani/uPD72022.md). The implementation coverage above is derived from [`io/tsp.c`](../../io/tsp.c), not from the presence of a command name in either document.

### Host ports and status coverage

| Interface | Generic μPD72022 / BNN definition | Current vaeg state | Result |
|---|---|---|---|
| `0142h` read | Status: `LP`, `VB`, `SC`, `ER`, `EMEN` (VA), `BUSY`, `OBF`, `IBF` | Only `VB` and `BUSY` are exposed | Status ABI incomplete |
| `0142h` write | Command byte | Implemented | Basic match |
| `0146h` write | Parameter or streaming input | Implemented for the local parameter parser | Basic input path only |
| `0146h` read | Result or streaming output | Not bound | Result-producing commands cannot work |
| `IBF`/`BUSY` sequencing | BNN examples poll `IBF | BUSY` before writes | `BUSY` is modeled; `IBF` is not | Partial |
| `OBF` sequencing | Set when a result is available; cleared by read | Not modeled | Missing |
| `ER` recovery | Malformed input sets error; `EXIT` clears it | Unknown commands simply clear BUSY | Missing |
| `SC`/`SPROV` | Sprite-over/collision latch and result | No latch or result | Missing |
| `EMEN`/`EMUL` | VA-specific persistent expansion state | No state | Missing |

BNN explicitly documents the status layout at the `0142h` table and uses
`TEST AL,5` (`IBF | BUSY`) in its command/parameter examples. The generic data
book calls bit 3 reserved; BNN's `EMEN` meaning is therefore a VA-specific
extension, not a contradiction. Likewise, the generic light-pen command is
documented but BNN marks the VA light pen unavailable.

### Rendering and model-level consequences

Command decoding is not the whole TSP contract. The current renderer still
has documented gaps in text attribute modes and mode-3 state persistence,
`VSA`/`VH` virtual-buffer handling, sprite clipping, `HSPN` overflow,
collision groups, `SC`/`SPROV`, and the VA2/VA3 256-KiB TVRAM profile. These are
separate from the generic host-transfer/DMA commands and must not be silently
counted as implemented merely because text or sprites are visible.

Therefore the current status is:

```text
BNN PC-88VA TSP subset: partially implemented
Generic uPD72022 command set: substantially incomplete
Status/result-port ABI: incomplete
Documented source audit: consistent with these limitations
Hardware conformance: not established
```

### Critical correctness gaps

The earlier audit described the following `tsp_i142()` expression as a
remaining bug:

   ~~~c
   return (tsp.status | tsp.vsync) ? STATUS_VB : 0;
   ~~~

That description is stale for the current source. The active implementation
already uses the intended grouping:

   ~~~c
   dat = tsp.status;
   if (tsp.vsync) {
       dat |= STATUS_VB;
   }
   return dat;
   ~~~

The remaining gaps are:

1. `IBF`, `OBF`, `ER`, `EMEN`, `SC`, command-result reads at `0146h`, and
   output streaming are absent.
2. `DSPDEF.PITCH` is commented out, and only the low 16 bits of signed 19-bit
   `ATROFF` are retained.
3. Blink uses a heuristic shift rather than a verified hardware correction.
4. `vram/maketextva.c` lists modes 2 through 5, underline, and double width
   as TODOs; mode 3 cannot yet retain independent color and shape state.
5. Screen rendering does not enforce the complete `VSA/VH` virtual-buffer
   contract.
6. Sprite rendering lacks `HSPN` overflow, collision groups, `SC/SPROV`, and
   complete clipping.
7. Port `0148h` forces bit 0 to one and only warns on the path labeled 256KiB
   TVRAM mode, so VA2/VA3 is not yet a complete TSP display profile.

### Timing model

`tsp_updateclock()` derives horizontal and vertical periods from the 14-byte
`SYNC` parameter block. It uses VA-specific dot-clock values for the supported
24.8kHz, 15.73kHz, and 15.98kHz modes, then schedules display and VBlank events
through `machine/pccore.c`.

The implementation deliberately contains VA-specific behavior. Its horizontal
width calculation states that left and right border fields appear on real
hardware not to receive the `+1` treatment described by the uPD72022 data
sheet. This is direct evidence that the data book is a reference rather than a
drop-in executable specification for every VA detail.

Other timing comments identify unresolved approximations:

- cursor/text blink timing is adjusted because the documented value appears
  too slow when used directly;
- the original VA can initialize a 200-line SYNC block while operating in a
  24kHz state, requiring a system-port VBlank compatibility adjustment;
- one 15kHz dot-clock value is tuned toward observed real-machine timing.

These behaviors should be preserved until replaced by better measurements,
not mechanically overwritten with generic uPD72022 equations.

### Implementation priorities already identified

The source and data book expose several areas that need focused tests before
the TSP implementation is expanded. The safe order is deliberately from
state transitions that already have a direct vaeg field, toward operations
that require a new result/status path:

1. Record and test no-parameter display-state transitions (`DSPOFF`, then
   `SPROFF`) without changing the generic command parser.
2. Verify command and parameter buffering against ROM traces for all supported
   commands.
3. Verify `0142h` BUSY and VBlank status independently. The status expression
   grouping is already corrected in the current source; preserve that fix.
4. Determine the meaning of the unimplemented `0143h` read path from the VA
   technical manual or ROM reads.
5. Distinguish VA-specific `SPRDEF` behavior from the documented `SPRWR`
   command and its sprite-number/attribute-number parameters.
6. Add `SPRSW` only after the global sprite-off transition is independently
   tested.
7. Add `ACTSCR`, cursor, sprite status, and collision/overflow behavior based
   on software that exercises them.
8. Keep display-memory DMA and interrupt support separate from immediate text
   rendering so command completion remains observable.
9. Compare VA, VA2, and VA3 TVRAM size and address decoding without assuming
   the uPD72022 maximum memory size is fully installed.

The generic uPD72022 transcription (`uPD72022.md`, printed pages 3-78 through
3-88) is the semantic source for this order: `DSPOF/DSPOFF` has no parameter
and disables both display controllers; `SPROF/SPROFF` has no parameter and
disables sprites (and therefore the cursor); `SPRSW` has one selector byte and
changes one descriptor's enable bit. These statements are generic-device
semantics, not a claim that every VA model is electrically identical to the
retail part. VA-specific behavior remains subject to ROM or hardware evidence.

The active code also deserves ROM-less unit coverage for command parameter
counts, status transitions, sprite-table bounds, malformed command streams,
and timing arithmetic. No implementation changes are made by this document.

## 19. Conformance requirements

A minimum native implementation must provide ports `0142h` and `0146h`,
coherent command acceptance, the display/cursor/sprite control commands, `VB`
and `BUSY`, explicit original-VA and later-model TVRAM profiles, all four
screen descriptors, all six attributes, ANK and Japanese codes, 40/80-column
expansion, both sprite formats, and board-level classification and mixing.

The full PC-88VA profile additionally requires `EMUL/EMEN`, `SC`, collision,
sprite-over and `SPROV` timing, cursor blink, all documented `SYNC` profiles,
and sufficient access/contention modeling for software that observes `VALT`
or raster timing.

Generic-device light pen, semigraphics/graphics, serial/dual-port memory,
host/DMA memory commands, refresh, arbitration, and interrupt behavior should
remain a separable profile rather than being silently mixed into the board
wrapper.

## 20. Conformance test plan

### 20.1 Command, status, and address

- reset leaves no stale `OBF`, `ER`, `SC`, or `EMEN`;
- every fixed command consumes exactly its documented byte count;
- `EXIT` aborts a partial command without consuming stale bytes;
- malformed input sets `ER` without corrupting TVRAM;
- synthetic overflow/collision makes `SPROV` set `OBF`, return one byte, and
  clear `SC` on read;
- entering/leaving `EMUL` tracks `EMEN`; and
- original-VA TSP words `0000h`, `0001h`, `3FFFh`, `8000h`, and `BFFFh` map
  to CPU offsets `0000h`, `0002h`, `7FFEh`, `8000h`, and `FFFEh`.

### 20.2 Splits, attributes, and fonts

Test one and four contiguous splits; independent `VSA/VW/VH`; `RSA` coarse
scroll; every raster offset; negative `RXP`; out-of-bounds source fill;
FG/BG orientation; every mode and shape bit; mode-3 state at row/split
boundaries; modes 4/5 at colors 1 and 9; global/per-cell reverse; every line,
blink, and double-width transition; ANK and Japanese pairs; and both column
modes.

### 20.3 Sprites and composition

Test packed nibble and monochrome bit order, size extremes, priority, all edge
clips, both sides of the `A8000h` pattern boundary, `MG`, `HSPN`, first
overflow sprite, collision transparency and grouping, `SC` frame latching,
the 160-microsecond `SPROV` window, every boundary extreme, graphics between
text/sprite classes, every transparency bit, color 0, `TD`, and `DSPOFF`.

### 20.4 Resolution and timing

For every documented `SYNC` vector, capture total dots/lines, horizontal and
frame frequency, VRTC start/duration, `VB`, `LBR/RBR` semantics, and raster
address progression. Then test in this order:

1. known 640x400/24.8kHz with a one-dot grid;
2. 640x200 by changing only documented vertical controls;
3. 320x200 through `0102h.HW0`, checking physical two-dot horizontal pixels;
4. framebuffer pitch independently from visible width;
5. candidate 320x400 to test horizontal/vertical independence; and
6. a 384x256 viewport while retaining 640x400 `SYNC`.

`[MEAS]` M104 also stepped the screen-table `MODE` field (word `0Ah` bits
4:0) of the main split through undocumented values with
[`TSPMODE`](../../tools/pc88va/vtiming/README.md#tspmode-undocumented-tsp-screen-modes)
on a PC-88VA2 (2026-10-06). The test block held character codes 00h-FFh
with attribute F0h. Values 0-5 behaved as the documented attribute modes
(0: white background, 1: black, 2-3: nothing visible with F0h, 4-5: as 0
with blinking). Undocumented values:

| `MODE` | Observation |
|---:|---|
| 6 | as 4-5, the block grey instead of white |
| 7 | as 6 without blinking |
| 8, 9 | an unidentified pattern instead of characters |
| 10, 11 | the same pattern, finer |
| 12 | the pattern on the left, the right blank |
| 13 | the pattern wavy |
| 14 | the pattern yellow and wavy |
| 15, 18, 19 | only one cursor visible, the rest black |
| 16, 20 | broken characters split left and right, white background |
| 17 | as 16 with the original black background |
| 21-23 | as 16-17: every other character of each row, the row shown twice side by side (`Ready` appears as `Ray`) |
| 24-30 | as 8-14 respectively (28: pattern on the left only; 30: yellow) |
| 31 | as 15 |

`[MEAS]` A second complete pass (0-31) on 2026-10-07 at 24.8 kHz
reproduced the first and filled in 21-31; photographs:
[0](m104-photos/va2-tspmode-00.jpg),
[7](m104-photos/va2-tspmode-07.jpg),
[8](m104-photos/va2-tspmode-08.jpg),
[10](m104-photos/va2-tspmode-10.jpg),
[12](m104-photos/va2-tspmode-12.jpg),
[13](m104-photos/va2-tspmode-13.jpg),
[14](m104-photos/va2-tspmode-14.jpg),
[15](m104-photos/va2-tspmode-15.jpg),
[16](m104-photos/va2-tspmode-16.jpg),
[17](m104-photos/va2-tspmode-17.jpg),
[28](m104-photos/va2-tspmode-28.jpg). In 8-14 and 24-30
the whole main split, not only the test block, becomes a fine
multicoloured dot pattern with coarse colour regions; the function-key
split (`MODE` 1) stays text.

`[DERIVED]` Bit 4 repeats the effect of bits 3:0 with characters fetched
at twice the stride; bit 3 replaces character display by a non-character
pattern, which makes 8-14 (and 24-30) the candidates for the uPD72022's
semigraphics or graphics static-picture modes; the low bits then change
the dot size or format. The pattern's source and format are not yet
identified; a test with known TVRAM contents is needed.

`[MEAS]` M104 then ran
[`TSPFILL A`](../../tools/pc88va/vtiming/README.md#tspfill-known-data-under-the-undocumented-modes)
on the PC-88VA2 (2026-10-07, 24.8 kHz, filmed; built from
[`e8652f69`](https://github.com/nakatamaho/vaeg/commit/e8652f696b5ffb3ad373c1af372665a205f38cc6)),
which fills TVRAM from split 0's start to `7F00h` and the attribute area
with known data under `MODE` 8, 12, 14, 10 and 13. Under `MODE` 8:

| Fill | Result | Still |
|---|---|---|
| `P0` (00h), `P3` (characters 00h, attributes FFh) | the same multicoloured pattern as before the fill | [P0](m104-photos/va2-tspfill-08-p0.jpg) |
| `P1` (FFh), `P2` (characters FFh, attributes 00h) | the whole split white | [P1](m104-photos/va2-tspfill-08-p1.jpg) |
| `P4` (byte ramp) | vertical stripes stepping through the colours from left to right, identical on every raster, over the left third of the screen; black to the right | [P4](m104-photos/va2-tspfill-08-p4.jpg) |
| `P5` (value changes every 16 bytes) | yellow vertical stripes, a white area at the left | [P5](m104-photos/va2-tspfill-08-p5.jpg) |
| `P6`, `P7` | the `P0` pattern with parts changed | - |

The other modes behaved alike (12: left part only; 14: yellow). The result
depends on the character bytes, not visibly on the attribute bytes, but
an all-zero fill still shows the original pattern, so part of the source
lies outside the filled range.

`[DERIVED]` The undocumented modes are not a TSP crash: the result is
reproducible between runs and days, depends on the data, disappears in
the next frame when `MODE` is restored, and the function-key split
(`MODE` 1) of the same frame stays correct. The likely reading is that
these `MODE` values put the TSP into its semigraphics or graphics fetch
(signalled on its DM pins), for which the VA board has no circuit, so the
fetched data reach the text path unchanged; the per-raster address does
not appear to advance (`P4` stripes are the same on every raster). M104
stops here: the modes give no usable picture format. A maintainer
hypothesis to test later: in these modes the board may present font or
other ROM data, not TVRAM, on the TSP's data path, which would explain the
pattern that remains with an all-zero fill; it can be checked by rendering
the font ROM as 4-bit dots in vaeg and comparing with the stills (the ROM
contents themselves stay private).

`[MEAS]` M104 measured TSP frames outside the documented vectors on real
hardware: 15.98 kHz `VAD` of 224 and 240 lines (262- and 278-line frames)
are displayed, 24.8 kHz frames of up to 495 lines (`VAD` 464) are displayed,
and 507- and 511-line frames were out of range for the monitor used. Details
are in
[PC-88VA Video-Mode and Framebuffer Control](pc88va-video-modes.md#12-measured-non-native-timings);
the test program is [`V480PAT`](../../tools/pc88va/vtiming/README.md).

Do not begin with arbitrary `SYNC` values: out-of-range CRT timing is unsafe
and combines too many unknowns. Hardware captures must identify model, monitor
setting, clock source, probe point, and measurement uncertainty.

## 21. Unresolved questions

The highest-priority questions are:

1. PC-88VA `LBR/RBR` field-versus-field-plus-one behavior;
2. mode-3 reset boundaries;
3. `VSA` out-of-bounds fill and exact `RW/8+2` fetch/clipping;
4. `DWID/DWIDC` source advance and Japanese-pair interaction;
5. top-sprite fetch behavior and monochrome maximum width;
6. `VALT` accounting and port `0148h` bit-0 model behavior;
7. sprite interrupt wiring and `SPROV` latch lifetime;
8. streaming byte order;
9. original-VA undefined word ranges and VA2/VA3 TSP address decode; and
10. exact model differences.

## 22. Verification sources still needed

The following evidence would resolve the remaining architectural uncertainty:

- readable VA/VA2/VA3 motherboard photographs showing the display LSI marks;
- NEC schematics or parts lists identifying the TSP/IDP package;
- the original 1986 NEC Technical Journal article;
- VA technical-manual pages defining ports `0142h`, `0143h`, and `0146h`;
- logic-analyzer traces for command writes, BUSY, VBlank, interrupt, and DMA;
- software that exercises all sprite collision, overflow, active-screen, and
  display-memory transfer commands;
- model-by-model tests of text/sprite RAM wrapping and open-bus behavior.

Until that evidence is available, the 1989 data book should be cited as the
primary semantic reference for the VA TSP while VA-specific observed behavior
retains priority where the two differ.

## 23. References

1. NEC Electronics, *μPD72022 Intelligent Display Processor*, in
   *Intelligent Peripheral Devices Data Book*, Document 50051, June 1989,
   printed pages 3-57 through 3-96.
2. BNN, *PC-88VA Technical Manual*,
   [Internet Archive scan](https://archive.org/details/PC88VA/page/76/mode/2up).
3. *PC-88VA Tekumani*, especially `2.TXT`, `4.TXT`, `604TEXT.TXT`, and
   `605SPR.TXT`.
4. vaeg [`io/tsp.c`](../../io/tsp.c) and
   [`io/videova.c`](../../io/videova.c).
5. MAME [`pc88va.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp)
   and [`pc88va_v.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_v.cpp).
6. [Inside PC-88VA Wiki](http://www.pc88.gr.jp/inside88va/wiki/index.php?FrontPage).

## 24. Change log

### Version 0.1 - 2026-07-16

- Replaced the initial source survey with a reconstructed programming
  specification.
- Separated generic μPD72022 behavior, original-VA board behavior, and
  unresolved VA2/VA3 extensions.
- Added status and command semantics, PC-88VA timing vectors, TVRAM and screen
  formats, all text attributes, sprites, and external composition.
- Added the fixed-raster viewport rule and a safe resolution-test sequence.
- Linked the cross-block video-mode control reconstruction and clarified that
  the first 384x256 viewport test retains a 640-dot graphics pitch.
- Added MAME and vaeg audits, conformance tests, and prioritized unknowns.
