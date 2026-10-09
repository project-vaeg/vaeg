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
title: "GDC and SGP capabilities and limits"
short_title: "GDC/SGP capability matrix"
filename: "docs/modernization/gdc-sgp-capabilities.md"
document_status: "Evidence-based comparison"
language: "en"
version: "0.1-en.2"
date: "2026-08-23"
target_system: "PC-98 GDC documentation and PC-88VA SGP"
---

# GDC and SGP capabilities and limits

## Scope and terminology

The word **GDC** is ambiguous in the source material. `docs/98io/io_disp.txt`
describes the PC-98 μPD7220/7220A/72020 text and graphics GDCs. It is not a
PC-88VA SGP specification. In the PC-88VA circuit documentation, the device
with the historical "GDC" label is the D72022 IDP/TSP text and sprite
processor; the PC-88VA graphics drawing processor is the SGP (uPD92017).

The matrix below therefore compares:

- **GDC:** the PC-98 GDC interface documented by `docs/98io/io_disp.txt`;
- **SGP:** the PC-88VA SGP described by `docs/tekumani/4.TXT`, with current
  VAEG behavior called out separately; and
- **AGDC warning:** geometric commands in `docs/98io/io_agdc.txt` belong to
  the PC-H98 μPD72120 AGDC. They must not be attributed to either the
  ordinary PC-98 GDC or the PC-88VA SGP.

`DOCUMENTED` means that the period documentation states the behavior.
`IMPLEMENTATION` means that the behavior is present in VAEG but is not, by
itself, proof of real-hardware behavior. `NOT ESTABLISHED` means that the
requested source set does not support the claim.

## O/X summary

This compact table is the requested yes/no view. `O` means the capability is
documented for that device. `X` means the capability is not documented for
that device in the cited source set. `?` means that the source lists a related
command, but its detailed behavior is deferred to another data sheet or is
not established for this comparison.

| Capability | PC-98 GDC | PC-88VA SGP | Note |
|---|:---:|:---:|---|
| Text write/read and DMA | O | X | GDC `WRITE`/`DMAW`/`READ`/`DMAR`; SGP is a pixel engine. |
| Text cursor and character layout | O | X | GDC cursor, pitch, zoom and text commands. |
| Vector/line drawing | ? | O | GDC lists `VECTW`/`VECTE`; SGP has documented `LINE`. |
| Rectangular BITBLT | X | O | SGP uses source/destination block descriptors. |
| Repeated pattern transfer | X | O | SGP `PATBLT`. |
| Contiguous solid clear | X | O | SGP `CLS`; it is a word-range fill, not a rectangle fill. |
| 16 Boolean raster operations | X | O | SGP 4-bit logical-operation field. |
| Source-zero transparent transfer | X | O | SGP BITBLT transparent mode. |
| 1-bpp source color expansion | X | O | SGP `SET_COLOR` expansion; left-to-right restriction applies. |
| Circle/arc/ellipse primitives | X | X | AGDC has them, but AGDC is a different PC-H98 controller. |
| Dedicated flood-fill command | X | X | SGP has scan assistance plus `PATBLT`, not one flood-fill opcode. |
| Main-RAM word command list | ? | O | Explicit for SGP; not established as equivalent for GDC here. |
| Framebuffer windows/composition/palette | X | X | PC-88VA graphics BIOS owns these, not SGP; PC-98 GDC notes do not define the VA compositor. |
| Display-position scrolling | O | X | GDC lists `SCROLL`; VA display position is a graphics-BIOS function. |
| SGP completion BUSY/interrupt | X | O | SGP `0506h` BUSY and optional completion interrupt. |
| VBLANK generation | O | X | PC-98 GDC VSYNC status/trigger; SGP completion is not VBLANK. |
| Guaranteed framebuffer-boundary clipping | ? | X | SGP explicitly does not check framebuffer boundaries. |

## Capability matrix

| Function | PC-98 GDC | PC-88VA SGP | Evidence and limits |
|---|---|---|---|
| Primary role | Text GDC generates CRT sync in master mode; graphics GDC runs as a slave synchronized to it. | Dedicated graphics drawing processor and GVRAM/main-memory bus master. | `io_disp.txt`; `4.TXT`; the PC-88VA circuit mapping is summarized in [`88va_circuit.md`](88va_circuit.md). |
| CPU interface | Byte command/parameter and status/data ports: text GDC `0060h/0062h`, graphics GDC `00A0h/00A2h`. Status exposes FIFO, drawing, DMA, HBLANK and VSYNC bits. | Word command table in main RAM; start address at `0500h-0503h`, attention/status at `0506h`, interrupt/abort control at `0504h`. | [`io_disp.txt`](../98io/io_disp.txt) and [`4.TXT`](../tekumani/4.TXT). The SGP command pointer must be even. |
| Command storage and execution | Command bytes and parameters are sent through the GDC FIFO interface. DMA read/write commands exist, but these sources do not establish an SGP-style main-RAM word command list. | Sequential word command table in main RAM. SGP runs until `END`; it may be polled through BUSY or completed with the documented interrupt. | `io_disp.txt` command-port descriptions; `4.TXT` sections on command setup and command words. |
| Text write/read and DMA | `WRITE`, `DMAW`, `READ`, and `DMAR` are listed. | Not a text renderer; it transfers or draws pixel blocks. | GDC command list in [`io_disp.txt`](../98io/io_disp.txt); SGP command list in [`4.TXT`](../tekumani/4.TXT). |
| Cursor and text layout | `CSRW`, `CSRR`, `CSRFORM`, `PITCH`, `ZOOM`, `TEXTW`, and `SCROLL` are listed for GDC operation. | No text cursor, character-cell, or text-GDC command is documented. | GDC command list in `io_disp.txt`; SGP has only pixel-block descriptors. |
| Vector/line drawing | `VECTW` and `VECTE` are listed, but `io_disp.txt` defers detailed semantics to the μPD7220 data sheet. | `LINE` draws a straight line from a destination block and `SET_COLOR`; direction, ROP, and transparency fields are defined. | `io_disp.txt` marks GDC command availability as model-dependent; `4.TXT` defines SGP `LINE`. |
| Rectangular block transfer | No SGP-style `SET_SOURCE`/`SET_DESTINATION`/`BITBLT` descriptor is established by `io_disp.txt`. | `SET_SOURCE` + `SET_DESTINATION` + `BITBLT` transfer a rectangular block. | `4.TXT` explicitly defines source/destination blocks and `BITBLT`. |
| Repeated pattern transfer | Not established for the ordinary GDC interface. | `PATBLT` repeats a source block two-dimensionally over a larger destination. | `PATBLT` is command `0008h` in `4.TXT`. |
| Solid clear/fill | Not established as a GDC command in `io_disp.txt`; do not infer it from EGC documentation. | `CLS` fills a contiguous word range with `SET_COLOR`. It is not a pitch-aware rectangle primitive. | `4.TXT` `CLS` description and example. Because SGP does not check framebuffer boundaries, callers must split or constrain ranges. |
| Boolean raster operations | Not established by the ordinary GDC command list. | `BITBLT`, `PATBLT`, and `LINE` carry a 4-bit logical operation field with the 16 Boolean operations. | `4.TXT` BITBLT/PATBLT/LINE fields; VAEG implementation is in [`io/sgp.c`](../../io/sgp.c). |
| Transparent transfer | Not established for the ordinary GDC interface. | BITBLT has source-zero transparent mode and destination-zero mode. The documented restrictions include no right-to-left 1-bpp expansion and no right-to-left transparent transfer. | `4.TXT` `TP-MODE` and SGP restrictions. Current VAEG handling is implementation evidence, not a hardware benchmark. |
| 1-bpp color expansion | Not established by the ordinary GDC interface. | A 1-bpp source can expand to a multi-bit destination using `SET_COLOR`; the period document requires left-to-right transfer for this mode. | `4.TXT` `SET_COLOR`, pixel modes, and usage restrictions. |
| Pixel formats | The supplied GDC notes identify VRAM and command interfaces but do not establish the SGP packed 1/4/8/16-bpp descriptor model. | Block descriptors support 1, 4, 8, and 16 bits per pixel; start-dot ranges depend on that mode. | `4.TXT` pixel-position and block sections. |
| Circles, arcs, ellipses | Not established for the ordinary GDC. | No circle, arc, or ellipse command is listed. | Circle/ellipse opcodes in [`io_agdc.txt`](../98io/io_agdc.txt) are μPD72120 AGDC (PC-H98), not this GDC or the VA SGP. |
| Flood fill / paint | Not established for the ordinary GDC. | No single flood-fill command is listed. `SCAN_RIGHT`/`SCAN_LEFT` locate a boundary color; the document describes following a scan with `PATBLT` as paint assistance. | `4.TXT` SCAN sections. Current VAEG implements both scan commands as asynchronous operations; hardware conformance remains open. See [`upd92017-sgp.md`](upd92017-sgp.md) and [`io/sgp.c`](../../io/sgp.c). |
| Framebuffer/page ownership | PC-98 GDC documentation describes separate graphics VRAM planes and display/drawing plane selection on applicable machines. | SGP draws into the addressed destination; it does not select the visible composition page. | GDC plane selection in `io_disp.txt`; VA framebuffer/window/display control is owned by the graphics BIOS. |
| Windowing, composition, scrolling and palette | GDC `SCROLL`/`TEXTW` affect its command stream, but the supplied PC-98 notes do not define the PC-88VA G0/G1 compositor. | Not an SGP responsibility. PC-88VA graphics BIOS provides framebuffer definition, windows, composition priority, absolute/relative display position, masks, palette, and display enable. | [`606GRP.TXT`](../tekumani/606GRP.TXT) functions 0-11. SGP only writes the addressed memory. |
| VBLANK/VSYNC generation | Text GDC status exposes VSYNC; port `0064h` triggers a one-shot INT 0Ah at the next VSYNC on documented PC-98 systems. | SGP completion is separate from display VSYNC: `END` can request INT 10h level 8, and `0506h` reports SGP BUSY. | `io_disp.txt` VSYNC section and `4.TXT` SGP status/interrupt sections. Page exchange must use the VA display mechanism, not SGP BUSY as a VBLANK signal. |
| Addressing and wrap behavior | PC-98 plane and VRAM addresses are platform-specific. | SGP uses a 4 MiB SGP address space, even word addresses, and does not perform framebuffer-boundary checks. Horizontal/vertical wrap must be split by software; zero dimensions are not guaranteed. | `4.TXT` SGP memory map and restrictions. |
| Required initialization | GDC reset/start/sync sequencing is model-specific and not interchangeable with SGP setup. | `SET_WORK` must be issued before drawing; it reserves 58 writable bytes. | `4.TXT` explicitly requires `SET_WORK`; VAEG also loads this state in `io/sgp.c`. |

## What this means for PC-88VA software

For the current pseudo-sprite and wireframe demos, the safe division of labor
is:

1. Use the PC-88VA graphics BIOS (`606GRP.TXT`) for mode, framebuffer,
   window, composition, display-position, palette, and display-enable setup.
2. Put an even-addressed SGP command list in main RAM, issue `SET_WORK`, and
   use `SET_SOURCE`, `SET_DESTINATION`, `SET_COLOR`, `BITBLT`, `PATBLT`,
   `LINE`, and `CLS` according to the documented descriptor restrictions.
3. Use SGP zero-source transparent BITBLT for pseudo-sprites; use `CLS` only
   for a contiguous clear or use an opaque zero-source transfer for a
   pitch-aware dirty rectangle.
4. Synchronize page presentation with the VA display/VBLANK mechanism. SGP
   BUSY only says whether the drawing command stream has finished.
5. Do not copy PC-H98 AGDC circle, ellipse, sector, trapezoid, or paint
   opcodes into a PC-88VA SGP command list. Those commands are from a
   different controller and address map.

## VAEG implementation status

The period documentation is the hardware capability source. The current VAEG
implementation is a separate, model-dependent layer:

- `BITBLT`, `PATBLT`, `LINE`, and `CLS` are dispatched in [`io/sgp.c`](../../io/sgp.c).
- The emulator models the 16 logical operations and the two transparent
  transfer modes, but its exact timing coefficients are provisional.
- `SCAN_RIGHT` and `SCAN_LEFT` are implemented as asynchronous command
  entries and covered by emulator-side sanity tests; this does not establish
  real-PC-88VA conformance or a complete BIOS-level flood-fill operation.
- No VAEG timing result should be reported as a PC-88VA hardware throughput
  measurement.

## Sources

- [`docs/98io/io_disp.txt`](../98io/io_disp.txt): PC-98 text and graphics GDC
  ports, status, and command names.
- [`docs/98io/io_agdc.txt`](../98io/io_agdc.txt): PC-H98 μPD72120 AGDC; included
  only to mark the circle/fill/paint commands as a different device.
- [`docs/tekumani/4.TXT`](../tekumani/4.TXT): PC-88VA SGP memory map, command
  table, parameters, restrictions, and examples.
- [`docs/tekumani/606GRP.TXT`](../tekumani/606GRP.TXT): PC-88VA graphics BIOS
  for framebuffer, window, composition, display position, and palette control.
- [`upd92017-sgp.md`](upd92017-sgp.md): VAEG SGP reconstruction and evidence
  labels.
- [`io/sgp.c`](../../io/sgp.c): current VAEG command dispatch and execution
  status, not an authoritative hardware specification.
