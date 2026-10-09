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
# PC-88VA SGP Pseudo-Sprite Investigation

> **Status:** M1 hardware inventory is complete; M5 implementation and G5
> are complete; the corrected M6 stress/counter implementation is ready for
> its human gate
>
> **Date:** 2026-08-14 (JST)

## Result

The proposed architecture is valid for the documented PC-88VA programming
model and for the current VAEG implementation:

- single-plane, two-screen mode divides the 256 KiB GVRAM into a 128 KiB
  Graphic 0 region and a 128 KiB Graphic 1 region;
- a packed 320x200x4-bpp surface occupies 32,000 bytes (`0x7d00`);
- two such surfaces occupy 64,000 bytes and fit in Graphic 1;
- `0x7d00` is both four-byte aligned and a multiple of the Graphic 1 BIOS
  display-start granularity of 128 bytes;
- framebuffer 1 has a writable 18-bit display start address (`DSA1`), so the
  two Graphic 1 surfaces can be selected without copying them; and
- VAEG exposes vertical blank at TSP status port `0142h`, bit 6, and samples
  framebuffer state when the next display interval begins.

No fallback architecture is required by the inventory. This is a static
conclusion only. Real guest execution, image comparison, and flicker/tearing
observation remain gates for later milestones.

## Evidence policy

This report distinguishes three kinds of evidence:

- **Documented:** the repository-local PC-88VA Users Club technical text,
  consulted read-only, and the reconstructions linked below;
- **Implemented:** behavior traced in the active VAEG source at the evaluated
  commit; and
- **Proposed:** exact choices for the later NASM demo that have not yet been
  exercised in a guest.

The Users Club material is stronger evidence for hardware fields and
restrictions. VAEG source is used to establish what the emulator currently
does, not to invent hardware behavior.

## 1. Existing graphics-related files

The relevant tracked files are:

- [`pc88va-video-modes.md`](pc88va-video-modes.md): `GRMODE`, `GRRES`,
  framebuffer descriptors, pitch, and the 320x200 field relationship;
- [`upd72022-tsp.md`](upd72022-tsp.md): TSP timing and `0142h` status;
- [`videova.c`](../../io/videova.c): graphics registers, composition,
  transparency masks, palettes, and framebuffer I/O handlers;
- [`makegrphva.c`](../../vram/makegrphva.c): G0/G1 address selection and
  raster fetch; and
- [`scrndrawva.c`](../../vram/scrndrawva.c): priority composition and
  transparent-color rejection.

[`gvramva.c`](../../memoryva/gvramva.c) owns `grphmem[0x40000]`, a 256 KiB
GVRAM image. [`memctrlva.c`](../../io/memctrlva.c) implements port `0153h`:
bit 4 selects single-plane graphics and bank 4 maps GVRAM into the CPU system
memory window. [`memoryva.c`](../../memoryva/memoryva.c) places that window at
CPU physical address `A0000h`.

## 2. Existing SGP implementation files

[`sgp.c`](../../io/sgp.c) and [`sgp.h`](../../io/sgp.h) implement the SGP
command engine, 22-bit address dispatch, commands, busy state, and timing.
[`upd92017-sgp.md`](upd92017-sgp.md) is the repository's reconstructed SGP
specification and records remaining implementation uncertainties.

The VAEG SGP maps `200000h-23ffffh` to `grphmem`, decodes `END`, `NOP`,
`SET_WORK`, `SET_SOURCE`, `SET_DESTINATION`, `SET_COLOR`, `BITBLT`, `PATBLT`,
`LINE`, `CLS`, and the two scan opcodes, and executes only while single-plane
mode is active. The scan commands remain unimplemented; they are not needed
by this demo.

## 3. Existing graphics demos and NASM conventions

No tracked PC-88VA guest graphics or SGP demo source was found. The historical
Users Club BIOS text contains graphics-mode, buffer-definition, composition,
PUT/BITBLT, and page-exchange examples, but not this layered SGP demo.

The existing NASM `.COM` examples are
[`rep0f_probe.asm`](../../tools/hardware/pc88va_rep0f/rep0f_probe.asm) and
[`r2fprobe.asm`](../../tools/pc88va/hostfs/r2fprobe.asm). They use English
source, the repository's 2-clause BSD header, `bits 16`, and `org 0x100`.
The top-level [`CMakeLists.txt`](../../CMakeLists.txt) discovers `nasm`, writes
guest artifacts below the build directory's `guest/` subdirectory, and uses
custom targets. The demo should follow that convention rather than write a
binary into the source tree.

The existing disposable-media workflow is
[`pcengine_disk.py`](../../tools/pc88va/pcengine_disk.py): create a copy from a
user-supplied source D88 and install the generated guest file into that copy.
The source image must never be modified.

## 4. GVRAM layout and the two-page hypothesis

The documented single-plane two-screen layout and VAEG's `addr18()` setup
agree:

| Owner | CPU address with GVRAM bank selected | SGP address | Size |
|---|---:|---:|---:|
| Graphic 0 | `a0000h-bffffh` | `200000h-21ffffh` | 128 KiB |
| Graphic 1 | `c0000h-dffffh` | `220000h-23ffffh` | 128 KiB |

For 320x200x4 bpp:

~~~text
bytes per line = 320 * 4 / 8 = 160 = 0x00a0
bytes per page = 160 * 200   = 32000 = 0x7d00
two pages                         64000 = 0xfa00
~~~

The proposed exact allocation is:

| Use | CPU address | SGP address | End address | Size |
|---|---:|---:|---:|---:|
| G0 background | `a0000h` | `200000h` | `207cffh` | `0x7d00` |
| G1 sprite page A | `c0000h` | `220000h` | `227cffh` | `0x7d00` |
| G1 sprite page B | `c7d00h` | `227d00h` | `22f9ffh` | `0x7d00` |
| unused G1 workspace | `cfa00h` | `22fa00h` | `23ffffh` | `0x10600` |

Framebuffer addresses are stored as `CPU address - A0000h`. Consequently,
the values proposed for framebuffer 1's `DSA` are `20000h` for page A and
`27d00h` for page B. These are full 18-bit register values, not offsets from
the Graphic 1 base.

The hardware DSA field is four-byte aligned. The graphics BIOS additionally
restricts the Graphic 1 display start to a 128-byte multiple in two-screen
mode. Both proposed starts satisfy both restrictions. Graphic 1 has no
horizontal or vertical wraparound; each selected page is nevertheless a
complete 160-byte-pitch, 200-line display and does not require wraparound.

**Hypothesis verdict:** two 320x200x4-bpp sprite pages can coexist in Graphic 1
and can be selected with `DSA1`. This is supported by period documentation and
the active VAEG readout path.

## 5. Graphic 0/1 composition

`GRMODE` bit 10 selects single-plane operation and bit 11 selects two graphics
screens. `GRRES` selects 4 bpp and 320 logical dots independently for G0 and
G1. The graphics BIOS provides the complete mode transition, so the proposed
M2 initialization is the documented `INT 8fh` function 0 call with `AH=00h`,
`BX=e00eh`, and `CX=0404h`. The `BX` fields select single-plane, two screens,
display enabled, 320 dots on both screens, and 200 lines; `CL=4` and `CH=4`
select 4-bpp pixels for G0 and G1. The call's return in `AX` must be checked
before touching GVRAM. This BIOS transition avoids constructing an incomplete
TSP timing transition from guessed register constants.

For direct register terminology, priority slot 0 is highest. The documented
palette-source codes are `Ah` for G0 and `Bh` for G1. Thus a G1-over-G0-only
composition is represented by `COLCOMP=00abh`: G1 in slot 0, G0 in slot 1,
and unused lower-priority slots. VAEG consumes the low nibble first and selects
the first nontransparent pixel, matching this ordering.

Port `0126h` is Graphic 1's 16-bit transparent-color mask. Setting bit 0 makes
G1 color index 0 transparent at composition time. Port `0124h` controls the
corresponding G0 mask; the background will leave all of its 16 colors opaque.

## 6. SGP BITBLT and transparency semantics

The SGP uses a private 22-bit address space. Main RAM is at
`000000h-09ffffh`, with CPU and SGP physical addresses equal, and GVRAM is at
`200000h-23ffffh`. Command tables must reside in main RAM and all commands and
parameters are 16-bit words. `SET_WORK` names a live, even-addressed 58-byte
work area.

Each source or destination block descriptor contains:

1. pixel mode and starting dot within the first 16-bit word;
2. width in pixels;
3. height in pixels;
4. byte pitch (`FBW`);
5. low address word; and
6. high address word.

The documented raw `BITBLT` mode uses `TP-MOD=1` to skip source pixels whose
value is zero and logical operation 5 to copy the source. With forward
vertical and horizontal traversal, the exact mode word is `0105h`. Period
documentation requires `HD=0` when transparency is enabled, which this mode
satisfies. VAEG implements the same source-zero mask and ROP 5 behavior.

Two independent zero-color mechanisms are required:

1. `BITBLT TP-MOD=1` prevents zero pixels in the RAM sprite bitmap from
   overwriting the hidden G1 page; and
2. Graphic 1 transparent-mask bit 0 makes the zero-filled portions of the
   displayed G1 page reveal Graphic 0.

No CPU mask or pixel-copy loop is needed.

## 7. SGP command and completion flow

The proposed per-frame list in main RAM is:

~~~text
SET_WORK       main-RAM 58-byte work area
SET_COLOR      0000h
CLS            hidden G1 page, 0x3e80 words
for each sprite in painter order:
    SET_SOURCE RAM bitmap descriptor
    SET_DESTINATION hidden-page descriptor
    BITBLT     0105h
END
~~~

The Users Club technical text `4.TXT` section 4.10 defines the four CLS
parameters as a 32-bit start address followed by a 32-bit region size in
words. `0x3e80` is therefore 16,000 words, exactly one 32,000-byte page; it is
not a guessed count-minus-one encoding. Sprite X positions are encoded using
the aligned destination word address plus the descriptor's start-dot field;
CPU code does not touch destination pixels.

The CPU writes the even command-list physical address as two word I/O cycles:
the low word at `0500h` and the high word at `0502h`. It starts the SGP by
writing 1 to byte port `0506h`, and polls `0506h` bit 0 until it clears. The
word-cycle requirement is confirmed by the VA BIOS disassembly; VAEG's byte-lane
I/O model does not reproduce a hardware hang caused by byte access to these
ports.

The same BIOS start sequence writes `10h` to byte port `0580h` immediately
before waiting for SGP idle and programming `0500h`/`0502h`. In the display
logic this selects CPU-data GVRAM writes (bits 4:3 = `10`); the port is also
read as the system `RBUSY` status in other paths. The demos therefore reassert
`0580h=10h` before every SGP kick, while keeping the command pointer and start
access widths unchanged. This is a hardware-state precaution derived from the
BIOS sequence, not an emulator timing assumption.

VAEG clears busy when `END` executes. The documented completion interrupt at
level 8 exists, but polling is the simpler first correctness path and is fully
implemented in VAEG.

## 8. Vertical blank and page exchange

TSP status port `0142h`, bit 6 (`40h`), is the documented vertical-blank
status. VAEG sets it when the display interval ends and clears it when the next
display interval begins. VAEG rebuilds graphics raster state, including FB1
`DSA`, at that display start.

The proposed flip sequence is therefore:

~~~text
wait for SGP busy = 0
wait until (IN 0142h AND 40h) = 0
wait until (IN 0142h AND 40h) != 0
write DSA1 as words at ports 022eh (low) and 0230h (high)
swap front/back variables
~~~

Waiting for a complete low-to-high transition prevents a late SGP completion
from flipping partway through an already active vertical blank. The DSA write
then precedes VAEG's next display-start sampling. No TSP hardware sprites are
involved.

The system status port `0040h` also exposes a VRTC signal and VAEG has a
separate CRT interrupt path. The first demo should poll `0142h.VB` because it
is both the direct documented TSP vertical-blank status and the state used by
VAEG's display scheduling. No VBLANK interrupt behavior is assumed.

## 9. Original proposed implementation path

The next milestones should use a new directory such as
`demos/sgp-pseudo-sprite/` and a CMake-generated
`guest/sgpdemo.com` artifact:

1. **M2 video bring-up:** use graphics BIOS function 0 for the complete
   320x200, 4-bpp, single-plane, two-screen transition; initialize palettes,
   buffers, windows, composition, and transparency; draw only the diagnostic
   G0 checkerboard and a simple G1 overlay.
2. **M3 transparent BITBLT:** add an even-addressed main-RAM command list,
   work area, and an 8x8 or 16x16 4-bpp bitmap; prove `0105h` source-zero
   transparency into G1.
3. **M4 multiple sprites:** add English-named sprite records containing
   position, velocity, dimensions, bitmap pointer, and priority; update
   coordinates on the CPU and emit SGP commands in painter order.
4. **M5 double buffering:** render `CLS` plus every sprite into the hidden
   page, wait for SGP completion, then switch `DSA1` during `0142h.VB`.
5. **M6 stress and counters:** enable at least 16 sprites plus configurable
   bullets and report frames, commands, BITBLTs, transferred pixels/bytes,
   sprite count, flips, and detectable missed vertical blanks on exit.

Sprite bitmap widths should initially be multiples of four pixels, keeping
their 4-bpp row pitch even and simplifying source descriptors. Destination X
may still be arbitrary because the start-dot field handles pixel alignment.
Draw order will define priority; later records overwrite earlier nonzero
pixels on the hidden page.

## 10. M4 input and FPS timing extension

The period keyboard BIOS material identifies `INT 82h` as the keyboard
service. Primitive function `0ah` senses whether a key is available without
waiting, and function `09h` consumes one key and returns its scan code in
`AH`. Cursor Up is scan `3ah` and cursor Down is scan `3dh`. M4 uses those
calls to adjust the active sprite count between 1 and 32; `+` and `-` are
printable fallbacks. No PC-compatible extended-key encoding is assumed.

The period calendar BIOS material defines `INT 8ch`, function `02h`, as the
read-current-time call and returns seconds in `DH`. M4 counts completed frames
between changes of that value, discards the first partial interval, and clamps
the three-digit display to 999.

The counter value only selects six main-RAM bitmap pointers. The `FPS` label
and three digits are emitted after the balls as transparent SGP BITBLTs into
Graphic 1 at the upper right. The CPU does not draw the glyph pixels, and this
extension does not write Graphic 1's display-start address.

## 11. M5 implementation update

The buildable NASM source now implements the proposed two-page path.
Graphic 1 page A is SGP `220000h` with DSA1 `020000h`; page B is SGP
`227d00h` with DSA1 `027d00h`. Both pages are 320x200x4-bpp surfaces
of `0x7d00` bytes inside the 128-KiB Graphic 1 region.

Each frame selects the page opposite `draw_page_index`, emits SGP CLS
and transparent BITBLTs there, and waits for SGP busy to clear. It then
waits for port `0142h` bit 6 to make a low-to-high VBLANK transition and
writes DSA1 low and high words through `022eh` and `0230h`. The page
index is toggled only after that write. Startup renders both pages before
display enable, so no uninitialized page is exposed.

The ball bitmap is a pre-rendered 24x24 raster sphere with a smooth light
gradient, white upper-left highlight, dark lower-right edge, and a transparent
source-zero silhouette. Sixteen hue variants are stored in main RAM; color 0
remains transparent and all sphere pixels still come from SGP BITBLT. The CPU
does not mask or copy them. The FPS label is followed by a C label and four
ball-count digits, and the active prefix can grow from 1 to 256 records.
Records 17 through 48 use an 8x8 transparent bullet bitmap; later records
reuse the shaded 24x24 ball variants for the remaining stress capacity.

The command buffer is 4,283 words (8,566 bytes) at the 256-record limit; the
initial 16-sprite frame emits 443 words and 27 BITBLTs. The 48-record
ball-plus-bullet prefix emits 955 words and 59 BITBLTs, with 11,572 source
pixels and 5,786 source bytes. The 256-record frame transfers 131,380 source
pixels and 65,690 source bytes; these are source rectangles presented to SGP,
not a claim that every pixel is nonzero after transparency rejection.

The M5 build passed NASM assembly, the dedicated CMake guest target, and
headless VAEG execution on a disposable D88. G5 human measurement recorded
about 57 FPS at 26 active 24x24 spheres and about 28 FPS at 27 spheres.
The current M5 gate is complete; this workload cliff is retained as an M6
optimization input rather than treated as a VAEG correctness defect.

## 12. M6 stress and instrumentation update

M6 keeps the M5 page architecture and adds a bounded stress prefix of 256
records. The first 16 records are the established shaded spheres, the next
32 are 8x8 4-bpp transparent bullets, and records 49 through 256 are further
spheres. Up/+/Down/- still changes only the active prefix, so a tester can
measure the transition from balls to bullets without changing the command-list
format or using a CPU pixel loop.

The command builder counts command words and BITBLTs in the current frame. For
each BITBLT it also counts source pixels (`width * height`) and source bytes
(`pitch * height`); the CLS and command-list control words are not included in
those transfer counters. Completed frames, VBLANK-synchronized page flips,
and both per-frame and 32-bit cumulative transfer counters are retained in
main RAM. A VBLANK counter increments only when the bounded low/high polling
window is exhausted. On ESC, after restoring the saved video state, DOS prints
all counters and the active sprite count. The cumulative values wrap at
`0xffffffff`, while the per-frame values are 16-bit and sufficient for 256
records.

Machine checks for this M6 candidate were:

- NASM assembled the default M6 `sgp_sprite_demo.asm` to a 20,734-byte COM;
- the dedicated `sgpsprite_com` CMake target rebuilt successfully;
- a normal 16-record headless VAEG run on a disposable copy of
  `docs/disks/pcengine110-bootonly.d88` completed and produced a screen dump;
- a temporary, uncommitted build with `SPRITE_INITIAL_COUNT` set to 256
  completed the same VAEG launch without an emulator or SGP error; and
- the source still contains no CPU sprite-pixel copy or masking loop.

The 26-at-about-57-FPS and 27-at-about-28-FPS human measurements remain the
M5 workload reference. M6's 256-record launch is a correctness/stability
stress check, not a claim that 256 24x24 BITBLTs will sustain display rate.
This section remains an implementation candidate until the maintainer passes
the M6 visual/human gate.

## 13. M6 rebuild correction and milestone binaries

The first M6 distribution candidate exposed two separate packaging/runtime
errors during the human review. The D88 had been made with the `vanilla`
workflow, so it retained the four PC-Engine system files. The replacement
artifact is made with `pcengine_disk.py data` and an install payload containing
only the six milestone programs:

~~~text
A:\SGPDEMO1.COM
A:\SGPDEMO2.COM
A:\SGPDEMO3.COM
A:\SGPDEMO4.COM
A:\SGPDEMO5.COM
A:\SGPDEMO6.COM
~~~

The compressed distribution is `demos/disks/sgp-pseudo-sprite.d88.xz`.
It is a data disk, not a standalone boot disk; a separate bootable system D88
is mounted in FDD1 for launch. No `ENGINEIO.SYS`, `PCENGINE.SYS`,
`ADVGBIOS.SYS`, or `PCENGINE.COM` is included in the distribution image.

The first COM also used long conditional branches for the sprite and FPS
loops. NASM encoded those branches as `0f 85`, but the uPD9002 instruction
model treats the `0fh` prefix as its reserved/extended instruction family,
not as an 8086 near conditional branch. As a result, the command list stopped
after one sphere and one `F` glyph. This was a demo instruction-selection bug;
no VAEG core behavior was changed. The corrected source keeps the loop counter
in RAM, uses a short conditional exit, and uses an unconditional near jump for
the long back edge.

The source now accepts `-dMILESTONE_STAGE=1` through `-dMILESTONE_STAGE=6`.
The reproducible helper `build_milestone_coms.sh` emits the six runnable
names. M1 is a text hardware-inventory diagnostic; M2 initializes the video
mode and leaves the checkerboard visible; M3 uses one transparent SGP BITBLT;
M4 uses multiple animated records; M5 uses the hidden-page path with 1-256 ball records and no M6 bullet
stress prefix; and M6 is the full stress/counter build.

The resulting COM sizes are:

~~~text
SGPDEMO1.COM 20718
SGPDEMO2.COM 20660
SGPDEMO3.COM  8690
SGPDEMO4.COM  8692
SGPDEMO5.COM 20746
SGPDEMO6.COM 20734
~~~

A traced run of the corrected M6 COM emitted 16 sphere BITBLTs plus 11 FPS/
count glyph BITBLTs (27 BITBLTs per frame), with all 16 sphere source addresses
present before the glyph commands. The trace produced 37,709 `END` commands and
1,018,143 source/BITBLT commands during the bounded run. This is the machine
check for the earlier one-sprite/`F`-only symptom; visual M6 acceptance remains
a human gate.

## 14. Limitations and open checks

- The M1 inventory itself did not build a NASM program or run VAEG; the
  M5 and M6 updates above record the later machine checks.
- The tracked video reconstruction labels direct 320x200 field programming as
  a candidate pending hardware tests. The documented graphics BIOS mode is
  therefore the safer initialization interface for M2.
- VAEG's `TP=2/3` handling and SGP scan commands are incomplete. The proposed
  demo uses only documented `TP=1`, which is implemented.
- VAEG's SGP timing includes provisional contention costs. Smoothness and
  maximum sprite count must be measured rather than inferred.
- Some palette/composition registers are not readable through current VAEG
  I/O handlers. M2 must establish a BIOS-based exit/reset sequence before
  claiming that display and palette state are restored safely.
- The historical advanced-graphics BIOS has its own full-scene page-exchange
  service, but it does not by itself prove the required persistent G0
  background plus G1 sprite-layer arrangement. Direct, documented `DSA1`
  exchange preserves that arrangement and is the selected path.
- The optional 1-bpp expansion path is documented and present in VAEG, but it
  remains outside the first six SGP demo milestones and must not delay the
  4-bpp demo.

No VAEG defect was demonstrated by this static inventory. Any later mismatch
must be classified with a minimal guest reproducer before emulator behavior is
changed.
