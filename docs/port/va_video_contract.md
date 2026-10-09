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

# GLASS ORBIT P1: PC-88VA video, SGP, and OPNA contract

Status: P1 research complete. This document authorizes preparation of P2 only;
it does not authorize a PC-88VA graphics or sound implementation.

## 1. Scope and evidence labels

This is the P1 evidence freeze for the GLASS ORBIT PC-88VA port. It defines
what P2 may rely on, separates current VAEG behavior from hardware statements,
and records every missing fact that must not be filled in by guesswork.

| Tag | Meaning |
|---|---|
| `[VA-TM]` | PC-88VA technical-manual material made available locally by the maintainer. |
| `[VA-TEKU]` | The maintainer-local Users Club `tekumani` material. It is useful but not a substitute for hardware measurement. |
| `[SRC:path:line]` | Current VAEG implementation evidence only. |
| `[DERIVED]` | Arithmetic or layout inference from documented fields. |
| `[PC98-COMP]` | PC-9801 comparison material; never authority for PC-88VA behavior. |
| `[UNKNOWN]` | No sufficient source or hardware evidence. |
| `[HARDWARE_PENDING]` | A question that VAEG execution cannot settle. |

The local PC-98 references added for this investigation were
`PC-9800TechnicalDataBook_BIOS_1992.md`,
`PC-9800TechnicalDataBook_HARDWARE1993.md`, and
`PC-Techknow 98V 安井勉 1986_text.md`. They clarify the original PC-98
environment only. They do not define a PC-88VA port, PC-88VA BIOS call, or
PC-88VA I/O value.

## 2. Source-platform boundary

`GLASS286.ASM` enters PC-98 graphics through `INT 18h`, drives the GRCG, uses
the master-GDC status at `0A0h`, and selects PC-98 display/access pages through
`0A4h` and `0A6h`. It also uses DOS `INT 21h` for its command line, messages,
and keyboard polling. `[PC98-COMP]`

Those calls describe why `VIDEO286.INC` cannot be carried into the VA port.
The added PC-98 BIOS and hardware material was consulted to retain that
separation, not to translate PC-98 port numbers. The VA port instead has four
distinct owners: TSP timing, graphics-control registers, framebuffer
descriptors, and SGP drawing.

## 3. P1 contract by required item

| Item | P1 result | Evidence and restriction |
|---|---|---|
| 1. 640x200, 16-color packed target | Available only in single-plane mode. `0100h` bit 10 and `0152h` bit 12 select single-plane; `0102h` G0 bit 4 selects 640 dots; `0100h` bits 1:0 select 200 lines; `0102h` G0 bits 1:0 value `01b` selects 4 bpp. | `[VA-TEKU:4.TXT §4.4.1, §4.4.3]` The manual says the display and drawing single-plane controls should normally agree. A complete V3 enter/leave register sequence is not established here; see section 4. |
| 2. GVRAM placement | The VA has 256 KiB GVRAM. In V3 CPU mapping it occupies `A0000h` through `DFFFFh`; in SGP space it is `200000h` through `23FFFFh`. | `[VA-TEKU:4.TXT §4.4.1, §4.4.6]` Current VAEG allocates `grphmem[0x40000]`. `[SRC:memoryva/gvramva.c:16]` |
| 3. Packed 4bpp representation | Single-plane 4bpp uses one nibble per pixel and palette codes 0 through 15. A 640-pixel row is 320 bytes. The manual's word diagram numbers dot positions from the left/MSB, so P2 must keep the high-nibble-first interpretation under a focused P3 pixel test. | `[VA-TEKU:4.TXT §4.4.3, §4.4.6]` The 320-byte pitch is `[DERIVED]`; no CPU pixel writer is authorized before GA-2 verifies this on VAEG. |
| 4. Double-buffer capacity | A 640x200 packed-4bpp page is 64,000 bytes (`0xFA00`). One-screen single-plane mode assigns all 256 KiB to G0, so a 640x400 G0 source surface consumes 128,000 bytes and contains two contiguous 640x200 regions. | `[VA-TEKU:4.TXT §4.4.3, §4.4.5]` `[DERIVED]` This is a capacity/layout proof only. P3 must verify a visible page exchange. |
| 5. Vertical blank | TSP status port `0142h`, bit 6 (`VB`), identifies the vertical blanking interval. | `[VA-TEKU:2.TXT TSP status-port diagram]` Exact frame rate and safe monitor profile remain `[HARDWARE_PENDING]`; do not use VAEG timing as performance evidence. |
| 6. Palette | Packed 4bpp addresses 16 palette codes. The machine has two banks of 16 palette entries at word ports `0300h`--`031Eh` and `0320h`--`033Eh`; the entries carry RGB component fields. | `[VA-TEKU:4.TXT §4.5]` P2 may use one 16-entry palette. It must separately specify V3 palette mode and composition, rather than inherit DOS/BIOS state. |
| 7. SGP operation | SGP is usable only in single-plane mode. It executes an even-addressed, word-oriented command table in main RAM, uses a 58-byte work area, and sees GVRAM at SGP `200000h`--`23FFFFh`. `SET_WORK`, `SET_COLOR`, `LINE`, `CLS`, and `END` are documented. | `[VA-TEKU:4.TXT §4.4.6]` The command-list maximum length, host/SGP contention rule, and exact real-machine throughput are `[UNKNOWN]` / `[HARDWARE_PENDING]`. |
| 8. Payload entry | The final GLASS loader is a local PC-Engine validation wrapper; it does not establish a silicon-level bare-payload load contract. | `[SRC:demos/glass-orbit/src/glass_orbit_loader.asm]` `3000:0000` is the tested payload target for this demo only. `[UNKNOWN]` |
| 9. Main RAM, stack, list, and work placement | SGP main-RAM addresses `000000h`--`09FFFFh` correspond to CPU main RAM `00000h`--`9FFFFh`; the work area is normally there. | `[VA-TEKU:4.TXT §4.4.6]` Exact non-overlapping payload, stack, command-list, and work addresses depend on item 8 and are deferred to P2. `[UNKNOWN]` |
| 10. VA2 OPNA | VA2/VA3 Music BIOS supports YM2608 via interrupt `8Bh`; Music BIOS register functions correspond to OPNA port pairs `44h/45h` and `46h/47h`. Direct VA sound documentation requires status polling around register selection/data writes. | `[VA-TEKU:611MUSIC.TXT §6.11]` `[VA-TEKU:5.TXT §5.11]` Current VAEG binds these pairs for OPNA. `[SRC:io/boardsb2.c:201-236]` A bare-payload Music-BIOS dependency versus direct-I/O design is an explicit P2/P6 decision. |

## 4. Video-mode and framebuffer contract

### 4.1 Required ownership and ordering

The manual establishes fields, not a complete GLASS-specific mode-switch
recipe. P2 must therefore use the following dependency order, while marking
any not-yet-documented register value as unresolved rather than inventing it:

1. choose a documented TSP `SYNC` profile and program its command/parameters;
2. set the display and drawing controls consistently to single-plane;
3. select G0 640-dot, 200-line, 4bpp interpretation;
4. define FB0 source geometry and its displayed sub-screen;
5. load the selected 16-entry palette and select its composition path;
6. set the SGP work area once, submit a command list, and poll `0506h` bit 0;
7. wait for TSP `0142h` bit 6 before changing the displayed source window.

Steps 1--5 are a design dependency order, not an assertion that the hardware
requires these exact writes in this exact order. The required global
blank/enable and restoration sequence remains `[UNKNOWN]`.

The existing timing reconstruction records documented 200-line TSP `SYNC`
vectors, but it is explicitly non-authoritative and does not turn VAEG clock
constants into a hardware timing proof. `[SRC:docs/modernization/pc88va-video-modes.md]`

### 4.2 G0 double-buffer layout proposed for P2

P2 may design against this geometry:

| Property | Value | Basis |
|---|---:|---|
| physical display | 640 x 200 | `[VA-TEKU:4.TXT §4.4.1]` |
| source pitch | 320 bytes | `[DERIVED]` `640 / 2` at packed 4bpp |
| source height | 400 lines | `[DERIVED]` two pages |
| source size | 128,000 bytes | `[DERIVED]` `320 * 400` |
| page A source offset | `0x00000` | `[DERIVED]` |
| page B source offset | `0x0FA00` | `[DERIVED]` `320 * 200` |
| SGP page A/B bases | `200000h` / `20FA00h` | `[VA-TEKU:4.TXT §4.4.6]` plus offsets |

Framebuffer descriptors have 18-bit FSA/DSA byte addresses relative to CPU
`A0000h`, require four-byte alignment, and require
`DSA = FSA + OFX + FBW * OFY`, `FBW > OFX`, and `FBL > OFY`. FB0 can therefore
describe the 320-byte by 400-line source and select either 200-line half by
the matched `OFY`/`DSA` pair. `[VA-TEKU:4.TXT §4.4.5]`

This does not assert that a raw DSA change alone is a valid flip. P3 GA-6 must
write all coupled fields that the chosen descriptor requires and compare the
two visible pages. The software must not use FB1 as an interchangeable second
buffer: its FSA/FBL/OFX/OFY are restricted by the documented descriptor model.

### 4.3 SGP submission contract

The command table and its 58-byte work area must live in main RAM. Every SGP
address in a descriptor or start register is in SGP address space, not CPU
address space. The table begins on an even address; ports `0500h`--`0503h`
receive its address, `OUT 0506h,1` starts execution, and `IN 0506h` bit 0 is
zero only when the SGP is stopped. An `END` command ends the list. `[VA-TEKU:4.TXT §4.4.6]`

For 640x200 packed 4bpp, one complete page is `0x7D00` words. This follows the
manual's 640x400 `CLS` example (`0xFA00` words) by halving the height.
`[VA-TEKU:4.TXT §4.4.6] [DERIVED]`

P2 must serialize CPU star plotting after SGP completion unless an independent
hardware source establishes simultaneous CPU/SGP access to the same page as
safe. Current VAEG's timing constants and contention comments are
implementation behavior, not that evidence. `[SRC:io/sgp.c:363-387]`

## 5. Sound contract for the later P6 milestone

The original GLASS OPNA code is a PC-98 probe/control implementation and is
not portable by renaming its ports. The two evidence-backed alternatives are:

| Route | What is established | What remains open |
|---|---|---|
| Music BIOS, `INT 8Bh` | `AH=00h` initializes Music BIOS and YM2608; `AH=1Dh` adds rhythm initialization; `AH=04h` / `1Eh` provide low/high register access. | Payload queue ownership, interrupt ownership, and whether the bare runtime may depend on this ROM service. |
| Direct OPNA I/O | Low pair `0044h/0045h`; high pair `0046h/0047h` on OPNA. A status/busy poll is required around accesses. | The complete GLASS instrument/timer initialization sequence and its VA2 behavior. |

P6 must choose one route before moving `GLASS_OPNA.INC`. It may not combine
the PC-98 detection/control ports with the VA contract.

## 6. Explicit unknowns carried into P2

| ID | Question | Status |
|---|---|---|
| P1-U1 | Complete, safe V3 enter/leave sequence including blank/enable and restoration. | `[UNKNOWN]` |
| P1-U2 | Bare-payload loader, entry CS:IP, segment registers, stack, and exit/return contract. | `[UNKNOWN]` |
| P1-U3 | Maximum SGP command-list size and behavior on an overlong list. | `[UNKNOWN]` |
| P1-U4 | CPU/SGP same-page contention semantics and the safety of overlap. | `[HARDWARE_PENDING]` |
| P1-U5 | SGP throughput, page-flip latency, and acceptable frame rate on real VA hardware. | `[HARDWARE_PENDING]` |
| P1-U6 | Exact 4bpp nibble write validation on a real VA. | `[HARDWARE_PENDING]`; VAEG GA-2 may test only the emulator path. |
| P1-U7 | Music-BIOS availability and interrupt assumptions in the final bare runtime. | `[UNKNOWN]` |

## 7. P2 inputs and disposition

P2 may now estimate command-list capacity using the documented 640x200
geometry, choose a staged CPU-versus-SGP verification harness, and present the
single outstanding design choice for packed-4bpp span ends. It must not claim
real-machine timing, loader ABI, or shared-page contention behavior.

**P1 disposition: P2 may be prepared. P3 and later implementation remain
blocked until the maintainer approves P1 and P2.**
