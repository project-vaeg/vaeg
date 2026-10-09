<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# NEON RELAY 4 PC-88VA P1 contract

Status: P1 research complete. This document authorizes P2 design work only;
it does not authorize a NEON4 VA implementation.

Implementation extension: the completed NEON4 source also provides the
requested 640x400 16-colour profile. That profile uses packed 4bpp G0 and
the VA 4096-colour palette; it is documented separately in
`demos/neon4/README.md` and does not change the direct RGB332 contract below
for the original 256-colour profile.

The first implementation target remains the 286 scene path in `demos/neon4`.
The two requested physical modes are 320x200 and 640x200, both single-plane,
packed 8bpp, with two displayable source pages where the descriptor and real
machine tests permit it. SGP is the drawing engine. PC-98 GRCG, EGC, GDC,
PEGC, DOS services, PC-98 IRQ code, and OPL3 are not part of this contract.

## 1. Evidence labels

| Tag | Meaning |
|---|---|
| `[VA-TM]` | BNN PC-88VA technical-manual material in this checkout. |
| `[VA-TEKU]` | Maintainer-local Users Club/technical material under `docs/tekumani/`. |
| `[SRC:path:line]` | Current VAEG or existing VA payload source evidence. |
| `[DERIVED]` | Arithmetic or field interpretation derived from documented values. |
| `[IMPLEMENTATION]` | Behaviour implemented by VAEG; not by itself a silicon claim. |
| `[UNKNOWN]` | No sufficient source or measurement is available. |
| `[HARDWARE_PENDING]` | Requires a PC-88VA/VA2 run or electrical measurement. |

The existing `demos/neon3` and `demos/sgp-pseudo-sprite` payloads are the
primary reusable VA examples. Their proven 4bpp details must not be silently
generalized to 8bpp when the descriptor or mode value has not been calibrated.

## 2. Contract summary

| Item | P1 result | Status and restriction |
|---|---|---|
| Color representation | Direct packed RGB332, not a palette index | **Resolved.** One guest byte is `gggrrrbb`: blue is bits 1:0, red bits 4:2, green bits 7:5. No frame palette animation is allowed. `[SRC:vram/scrndrawva.c:21-39,311-420]`, `[SRC:vram/makegrphva.c:398-485]`, `[DERIVED]` |
| 320x200 / 640x200 mode setup | Use VA graphics BIOS/TSP/GRMODE/GRRES/FB descriptors as one transaction | 320x200 4bpp BIOS entry is proven by the sprite payload; 8bpp `CX` values and the complete 640-dot BIOS argument are `[UNKNOWN]` and must be calibrated before P3. `[SRC:demos/sgp-pseudo-sprite/sgp_m7.asm:360-396]`, `[SRC:docs/modernization/pc88va-video-modes.md:320-358]` |
| GVRAM address and size | SGP GVRAM `200000h-23FFFFh`, 256 KiB; CPU aperture is modelled separately | 320x200x8bpp = 64000 bytes/page; 640x200x8bpp = 128000 bytes/page. Two 640x200 pages consume the complete 256 KiB SGP GVRAM window; page validity remains `[HARDWARE_PENDING]`. `[SRC:docs/modernization/upd92017-sgp.md:370-382]`, `[SRC:memoryva/gvramva.c:1-16]`, `[DERIVED]` |
| Display-source switching | FB0/FB2 descriptors carry FSA/DSA and source geometry | Existing DSA1 exchange in the 4bpp G1 demo proves one VA page-switch pattern, not FB0 8bpp. NEON4 FB0 two-page switching is `[HARDWARE_PENDING]`. `[SRC:demos/sgp-pseudo-sprite/sgp_m7.asm:260-340]`, `[SRC:docs/modernization/pc88va-video-modes.md:358-455]` |
| VBLANK | TSP status port `0142h`, bit 6 (`VB`) | Port/bit are established by existing payloads and TSP reconstruction; exact hardware cadence for both requested modes is `[HARDWARE_PENDING]`. `[SRC:demos/sgp-pseudo-sprite/sgp_m7.asm:45-58]`, `[SRC:docs/modernization/upd72022-tsp.md]` |
| Packed byte order | Increasing logical X consumes increasing GVRAM bytes; words are little-endian | In 320-dot host presentation VAEG duplicates each source byte for display scaling; guest storage remains one byte/pixel. `[SRC:vram/makegrphva.c:398-485]`, `[IMPLEMENTATION]` |
| SGP operation | `SET_WORK`, `SET_SOURCE`, `SET_DESTINATION`, `SET_COLOR`, `BITBLT`, `LINE`, `CLS`, `END` | Command-list ports and busy protocol are established. 8bpp descriptor lane/width encodings and real `BITBLT` GVRAM-to-GVRAM behaviour remain `[UNKNOWN]`/`[HARDWARE_PENDING]`. `[SRC:docs/modernization/upd92017-sgp.md:393-455,1065-1105]` |
| OPNA | VA Music BIOS `INT 8Bh`, YM2608 register services | Use the VA BIOS path only. Exact NEON4 score lifecycle and rhythm/SSG requirements are deferred to P8. `[VA-TEKU:611MUSIC.TXT §6.11]`, `[SRC:docs/port/neon3_va_video_contract.md:92]` |
| Payload/RAM/cache | NASM flat `.COM`, existing VA payload entry/return convention, command/work/data in main RAM | SGP main-RAM addresses `000000h-09FFFFh` are available in the documented map. Initial CAT/raster assets stay in main RAM; 640x200x8bpp leaves no unused GVRAM page tail. Exact loader stack reservation is selected in P2/P3. `[SRC:demos/sgp-pseudo-sprite/sgp_m7.asm:1-20]`, `[SRC:docs/modernization/upd92017-sgp.md:370-382]` |

## 3. Direct RGB332 is the color contract

NEON4 must not carry the original PEGC palette-animation design into the VA
backend. The VAEG direct-color composition path reconstructs an 8-bit pixel as
follows:

```text
pixel byte:  ggg rrr bb
green = (pixel >> 5) & 7
red   = (pixel >> 2) & 7
blue  = pixel & 3
```

`vram/scrndrawva.c` builds the same mapping for all 256 byte values before
compositing a direct 8bpp screen (`pixelmode == 2`). The direct-color branch
does not index `videova.palette`; it converts the byte through `rgb8to16`.
`vram/makegrphva.c:398` reads packed bytes from GVRAM in address order. In the
320-dot host path it duplicates each byte only when expanding the guest image
into the common 640-dot host surface. Therefore the NEON4 guest framebuffer is
one RGB332 byte per logical pixel at both widths.

P2 consequence:

```text
source 256-colour ramp -> RGB332 byte values
frame update            -> write pixel bytes, not VA palette registers
face/ribbon/finale shade -> choose a direct RGB332 value
```

The source 256-colour ramp is still useful as a color-selection table, but it
is not a palette upload. Any code named `palette16_prepare_city_animation`
must be replaced by direct pixel-value generation at the VA abstraction.

## 4. Mode and framebuffer contract

### 4.1 Graphics mode fields

`GRMODE` selects the vertical interpretation (200-line field is `VW=10b` in
the reconstructed register model). `GRRES` is a separate 16-bit register at
`0102h-0103h`:

| Field | Values | NEON4 use |
|---|---|---|
| `PM0` bits 1:0 | `00=1`, `01=4`, `10=8`, `11=16` bpp | Set G0 to `10` for packed 8bpp. |
| `HW0` bit 4 | `0=640`, `1=320` logical dots | Set/clear for the selected build mode. |
| `PM1` bits 9:8 | Same mapping for G1 | Keep G1 disabled or configure only if the presentation design needs it. |
| `HW1` bit 12 | Same width selection for G1 | Do not assume G1 is needed for a G0-only NEON4 image. |

These field positions are documented/reconstructed in
`docs/modernization/pc88va-video-modes.md:320-358`. The complete 8bpp VA
Graphics BIOS call (`INT 8Fh`, function 0) is not present in an existing
payload; the proven `BX=E00Eh`, `CX=0404h` call is specifically the 320x200
two-screen 4bpp case and must not be copied as an 8bpp value.

The TSP `SYNC` vector, graphics enable/disable ordering, and matching
`GRMODE`/`GRRES` writes are one mode transition. The repository records complete
200-line PC-88VA timing vectors, but P1 does not authorize inventing a new
vector or claiming that a 640-dot 8bpp BIOS call has been validated.

### 4.2 Framebuffer descriptor fields

Each framebuffer descriptor is a `20h`-byte block. FB0 is `0200h-021Fh`; FB2
is the second G0 descriptor. The fields used by NEON4 are:

| Offset | Field | Meaning |
|---:|---|---|
| `+00h` | FSA | Virtual framebuffer start, 18-bit aligned address |
| `+04h` | FBW | Source-line pitch, four-byte aligned |
| `+06h` | FBL | Virtual final line / wrap extent in the VAEG model |
| `+08h` | DOT | Source pixel lane |
| `+0Ah` | OFX | Source X offset |
| `+0Ch` | OFY | Source Y offset |
| `+0Eh` | DSA | Displayed source start |
| `+12h` | DSH | Displayed source height |
| `+16h` | DSP | CRT destination Y |

The descriptor offsets and field roles are from
`docs/modernization/pc88va-video-modes.md:358-455` and
`docs/modernization/more-screen-resolutions.md:200-218`. For tightly packed
8bpp data the arithmetic pitches are:

```text
320 dots: 320 bytes/line,  320 * 200 =  64000 bytes/page (0x0FA00)
640 dots: 640 bytes/line,  640 * 200 = 128000 bytes/page (0x1F400)
```

Two 320x200 pages require `0x1F400` bytes. Two 640x200 pages require
`0x3E800` bytes, exactly the `0x40000`-byte single-plane SGP GVRAM window with
`0x1800` bytes unused. This is capacity arithmetic only; the exact FB0/FB2
source-page selection and display timing must be checked on VA and VA2.

### 4.3 CPU and SGP address spaces

The SGP map reconstructed in `upd92017-sgp.md` places main RAM at
`000000h-09FFFFh` and GVRAM at `200000h-23FFFFh`. Existing 4bpp payloads use
the `A000:0000` CPU graphics aperture after the BIOS transition and convert
COM `DS:offset` values to physical addresses for SGP command parameters. The
same conversion pattern is reusable, but the 8bpp mode must independently
confirm its aperture and descriptor base.

## 5. VBLANK and page publication

Existing VA payloads poll:

```text
TSP status: port 0142h
vertical-blank bit: bit 6 (VB)
```

The `VB` bit and port are established by `upd72022-tsp.md` and the SGP sprite
payload. VAEG derives approximately 61.46 Hz for one 200-line timing profile
and approximately 59.95 Hz for another; these are emulator timing results,
not a PC-88VA/VA2 hardware guarantee.

The intended NEON4 presentation sequence is:

```text
render non-displayed source page
wait for SGP idle and finish CPU work
wait for VB edge
switch the validated FB0 display source
```

The existing sprite demo's FB1 DSA exchange (`022Eh`/`0230h`) is evidence for a
working G1-specific pattern only. It does not prove that writing those ports
selects an FB0 8bpp page. P2 must choose and instrument the correct FB0/FB2
descriptor path; no guessed page-flip port is authorized here.

## 6. SGP operation contract

The SGP command list resides in main RAM, is word-addressed/little-endian, and
is submitted through the following interface:

| Port | Use |
|---:|---|
| `0500h` | low word of command-list physical address |
| `0502h` | high word of command-list physical address |
| `0504h` | control/interrupt/abort |
| `0506h` | start and busy status |
| `0508h` | implementation-specific read; do not use as a hardware contract |
| `0580h` | GVRAM bus/readiness and CPU-data write mode (`10h` in existing payloads) |

The safe existing flow is: build a complete list, initialize a stable even
58-byte work area and descriptors, wait idle, write `0500h`/`0502h`, configure
`0504h`, start via `0506h`, and poll busy bit 0. This is documented in
`docs/modernization/upd92017-sgp.md:393-455` and exercised by
`demos/sgp-pseudo-sprite/sgp_sprite_demo.asm:1168-1230`.

NEON4 may use:

```text
SET_WORK       0003h
SET_SOURCE     0004h
SET_DESTINATION 0005h
SET_COLOR      0006h
BITBLT         0007h
LINE           0009h
CLS            000Ah
END            0001h
```

The repository identifies packed 8bpp as a supported SGP pixel mode and
identifies BITBLT as the replacement for the source EGC VRAM copies. It does
not provide a validated 8bpp `SET_SOURCE`/`SET_DESTINATION` lane encoding or a
real-machine GVRAM-to-GVRAM BITBLT result. Those are P2/P6 test items, not
values to infer from the 4bpp sprite descriptors. SGP maximum list length,
command-count limits, same-page CPU access during execution, and completion
latency are also `[HARDWARE_PENDING]`; VAEG can supply functional command
counts, not hardware timing.

## 7. OPNA boundary

The PC-88VA port uses YM2608 through the VA Music BIOS `INT 8Bh`. The existing
documentation and NEON3 contract identify the low-bank and high-bank register
pairs as `44h/45h` and `46h/47h`; `io/boardsb2.c:201-236` maps those addresses
in VAEG's Sound Board II model. NEON4 must call the BIOS service rather than
emitting PC-98 direct-port candidates from the source tree (`0088h`, `0188h`,
and similar values).

P1 does not decide whether the NEON4 score needs SSG, rhythm, `Initialize2`, or
another timer lifecycle. That is P8. The only P1 invariant is OPNA-only and
clean BIOS-owned shutdown; OPL3 and its runtime switches are excluded.

## 8. Payload, stack, and image-cache boundary

The reusable payload contract is the NASM flat `.COM` route used by the existing
VA demos (`org 100h`, private stack/segment setup, VA BIOS entry, bounded SGP
polling, and the existing loader continuation). P1 does not introduce an
arbitrary bare-payload ABI.

Command lists, the 58-byte SGP work area, geometry scratch, and the CAT/raster
assets should initially live in main RAM. For 320x200x8bpp, two pages leave
main-RAM/GVRAM placement flexibility. For 640x200x8bpp, two pages consume the
entire single-plane GVRAM range, so no GVRAM tail may be reserved for the
source images. P2 must produce an explicit map including stack, command-list
high-water bound, image cache, and loader-reserved addresses before P3 writes
any descriptor.

## 9. P1 disposition and restrictions

**Disposition: GO WITH RESTRICTIONS.**

1. The color path is fixed to direct RGB332 bytes (`gggrrrbb`); no palette
   animation or PEGC palette register writes may be used.
2. The 286 geometry/scenes remain the source of logical coordinates. The VA
   backend owns physical X/Y conversion and packed 8bpp writes.
3. Existing neon3 and pseudo-sprite mode/page examples may be reused only
   where their pixel mode and descriptor match. The proven 4bpp `INT 8Fh`
   arguments are not 8bpp arguments.
4. Exact 8bpp Graphics BIOS arguments, FB0/FB2 two-page display selection, and
   8bpp BITBLT descriptors require P2/P3 calibration and, where applicable,
   real VA/VA2 measurement.
5. BITBLT is the planned replacement for the one EGC VRAM-copy scene, but it
   is not claimed hardware-conformant until its 8bpp descriptors pass a
   minimal test.
6. SGP timing, list-size limits, and CPU/SGP same-page contention are not
   performance inputs from VAEG; record them as `hardware_pending`.
7. OPNA is VA Music BIOS only. OPL3, DOS `INT 21h`, PC-98 IRQ2, GRCG, EGC,
   GDC, and PEGC runtime paths remain out of scope.

P2 may now design the 320x200 CPU/SGP control path, direct RGB332 ramp mapping,
command-count measurement, and the first mode-calibration payload. No P3 code
generation is implied by this report.
