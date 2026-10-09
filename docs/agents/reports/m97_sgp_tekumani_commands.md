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

# M97 - SGP Technical Manual command completion report

Evaluated baseline: `79ce89af64958cd85cdffa030890fb24a2af8148`

Status: **G97 human gate passed on 2026-08-24; M97 closed**

## 1. Rejected QA milestone removal

The unmerged `topic/m97-deterministic-qa` branch contained only the rejected
M97/M98 QA foundation after the evaluated `main` baseline. The replacement
branch was recreated from the baseline. No QA source, generated D88, fake
BIOS, capture frontend, guest injection, task, or report from that branch is
part of this candidate.

Maintainer-local untracked references and private media were not removed or
modified.

## 2. Manual-derived implementation matrix

| Area | Manual-derived behavior | M97 action | Hardware status |
|---|---|---|---|
| Command address | Word writes at `0500h` and `0502h`, even address | Preserve | Documented |
| Start/status | Start and BUSY at `0506h` | Preserve | Documented |
| Abort/IRQ | Control at `0504h`, IRQ at END | Preserve; timing deferred | Functional documented; ordering unresolved |
| SET WORK | Even address, stable writable 58-byte area | Preserve address only | Internal layout unresolved |
| Descriptors | Start dot, mode, 12-bit dimensions, aligned pitch/address | Correct original-VA profile | Documented for original VA |
| ROP | Sixteen Boolean functions | Verify current table | Documented |
| `TP=2` | Transfer only where destination pixel is zero | Verify current final-mask path | Documented |
| PATBLT | Repeat source in two dimensions | Preserve and regress | Documented normal case |
| LINE | `VD=0800h`, `HD=0400h` | Correct masks | Documented; raster tie rules unresolved |
| CLS | Fill a contiguous word count | Preserve | Documented normal case |
| SCAN RIGHT | Search boundary color and update width | Implement | Documented normal case |
| SCAN LEFT | Search boundary color and update left edge/width | Implement | Documented normal case |
| Thirteenth command | Manual says thirteen but names twelve | No implementation | Unresolved |
| Timing/contention | No recovered command-cycle table | No change | Hardware pending |

## 3. Evidence corrections

Direct reading of the Technical Manual resolves two stale conclusions in the
existing reconstruction:

- the documented ROP order matches the current VAEG implementation;
- SCAN always searches for SET COLOR and documents first-pixel, found, and
  not-found results; it does not expose an undocumented equality selector.

LINE direction bits also use the same `VD` and `HD` positions as BITBLT and
PATBLT. Exact discrete-line tie breaking remains unresolved.

## 4. Implementation

### 4.1 Descriptor and LINE decoding

`fetch_block()` now selects the descriptor profile from `pccore.model_va`.
The original VA profile masks width and height to 12 bits and framebuffer
pitch to a four-byte boundary, as documented by the Technical Manual block
diagrams. The existing VA2 profile retains its 14-bit width, 16-bit height,
two-byte pitch alignment, and observed R-TYPE compatibility adjustment.

LINE no longer has a separate swapped direction mapping. Its aliases now use
the documented common values `VD=0800h` and `HD=0400h`.

### 4.2 SCAN state machine

`SCAN RIGHT` and `SCAN LEFT` execute one pixel at a time through the existing
asynchronous SGP state machine. They reuse the saved destination runtime
fields, so `_SGP` and the binary `SGP` save-state section did not change.

Each step extracts the selected packed pixel and its corresponding packed SET
COLOR field. A match updates the documented output fields. A miss leaves the
input address, dot, and width unchanged. `SCAN LEFT` advances one pixel right
from the boundary color before recording the scanned region's left edge, as
shown by the manual diagram.

The implementation subtracts one internal scheduler quantum per scan pixel
only to make state-machine progress finite. This is not a recovered hardware
cycle count, and M97 makes no SGP timing or contention claim.

### 4.3 Focused regression coverage

The compiled selftest now verifies:

- original-VA and VA2 descriptor decoding separately;
- the common LINE direction masks;
- all sixteen documented Boolean ROP values;
- `TP=2` destination-zero masking;
- SCAN RIGHT first-pixel, later-pixel, miss, and packed-word-boundary cases;
- SCAN LEFT first-pixel, later-pixel, and miss cases, including returned left
  address/dot and width.

### 4.4 LINE visual gate program

At the maintainer's request, M97 adds the isolated
`demos/sgp-wireframe/sgp_wireframe.asm` visual test. It builds the DOS 8.3 name
`SGPWIRE.COM` and displays a regular tetrahedron, cube, regular dodecahedron,
and regular icosahedron in four viewports. The CPU performs signed fixed-point
rotation, perspective projection, and command-list generation. SGP CLS clears
the hidden half of a 640x800 Graphic 0 framebuffer and SGP LINE draws the grid
and all 78 solid edges before a vertical-blank DSA0 page exchange.

The first visual candidate incorrectly used `DX` to preserve an intermediate
rotation product even though the next one-operand 16-bit `IMUL` overwrote
`DX:AX`. This made valid edge tables appear as malformed solids. The corrected
projection stores each intermediate product before executing the next `IMUL`.
The former rectangular cuboid was also replaced with an actual cube.

Each solid has independent X/Y angular rates and a sinusoidal scale. Edge
brightness follows the projected endpoint depth, providing a simple depth cue
without claiming polygon filling. The documented SGP command set has no
general polygon or flood-fill command.

Graphics BIOS mode function 0 uses `BX=a000h` and `CL=4` for single-plane,
one-screen, 640x400, 4-bpp output. Function 1 defines Graphic 0 as 640x800;
its two 128,000-byte display halves occupy 256,000 bytes of the 256 KiB GVRAM.
The demo uses the existing hardware-safe startup contract: SET WORK begins
every list, `0500h`/`0502h` and the two DSA0 registers receive word writes,
and the GVRAM write-mode latch is restored before each kick. No generated COM
or disposable disk image is tracked.

### 4.5 16-bpp 320x200 page exchange correction

The direct-color teaching track under `demos/sgp-wireframe/65536/` uses the
PC-Engine 1.00-derived `GRMODE=0xB462` / `GRRES=0x1313` profile with a 320x400 source
surface (`FBW=640`, `FBL=400`) and a 320x200 displayed window. Two contiguous
320x200 pages occupy byte offsets `0` and `0x1f400`; DSA0 is changed only
after SGP completion and the VBLANK wait. The earlier horizontal layout put
the pages in alternating halves of every row and required 200 row-sized CLS
commands. That was functionally correct but an unnecessarily large command
overhead. The vertical layout makes the hidden page linear, so one CLS of
`0xfa00` words clears it. All rendering remains in SGP and the command list
remains in main RAM. The projection keeps its Y coordinate in the 320x200
logical space; the earlier extra Y shift remains removed because it distorted
the aspect of the 200-line display raster.

### 4.6 Direct-color triangle scan exercise

The new `demos/sgp-scan/65536/sgptri.asm` is a separate direct-color teaching
exercise. It selects the 320x200 logical G0 view used by the existing 16-bpp
track, keeps two contiguous 320x200 pages in the 320x400 source surface, and
allows one through four independently rotating projected triangular plates to
be selected with the arrow keys. The CPU performs the fixed-point rotation,
projection, and shade choice. SGP performs the clear, background grid, SCAN
probes, and triangle outlines on the selected hidden 16-bpp frame.

Polygon filling is intentionally disabled again. The command set has no
documented polygon-fill opcode, and direct-color behavior of the Graphics BIOS
polygon/Paint services remains under evaluation. The demo does not claim that
an INT 87h/AH=15h Paint call emits `SCAN_RIGHT` or `SCAN_LEFT`; it emits both
SGP commands directly as harmless probes so VAEG tracing can verify that the
implemented SCAN handlers are reached.

The first implementation used signed 16-bit edge intersection and division in
the guest command builder. That path could stop before the SGP kick on malformed
projected edges. It was replaced with clipped min/max spans and bounded
addition/shift-only midpoint spans. Those fill paths are no longer called; the
current build emits only outline LINE records. This is emulator evidence only;
BIOS equivalence and real-hardware behavior remain unconfirmed.

## 5. Validation

| Check | Result |
|---|---|
| `git diff --check` | PASS |
| UTF-8 / EOL / path-case validators | PASS, zero findings |
| Targeted clang-format 22 check for changed C files | PASS |
| Repository-wide clang-format check | PRE-EXISTING FAIL in unchanged `sdl2/np2.c` and `sdl2/scrnmng.c` |
| Unreferenced-source report | Completed; 40 pre-existing candidates, none added by M97 |
| Linux debug configure/build | PASS |
| `build/linux-debug/sdl2/vaeg --selftest` | PASS, including `SGP manual commands ok` |
| CTest | 83 PASS, 1 external-fixture SKIP, 0 FAIL |
| MinGW cross release build | PASS, PE32+ x86-64 GUI executable |
| `SGPWIRE.COM` deterministic NASM build | PASS, 5,663 bytes |
| VAEG wireframe smoke at two distinct frames | PASS, 640x400; four connected regular solids with changed pose/scale |

The task's initial `ctest --preset linux-debug` spelling was corrected because
the repository has configure/build presets but no CTest preset. The executed
command was `ctest --test-dir build/linux-debug --output-on-failure`.

MinGW artifact:

```text
build/mingw-cross/sdl2/vaeg.exe
SHA-256 83c5d1190bac41dcd9b8fcc604433e0030ad5ab54169ca55fc9055facfc15698

build/linux-debug/guest/sgpwire.com
SHA-256 eda9ee6650179cfabb75759631c67240bcdfb1212850b5f9d0e8ef05737f12b0
```

No ROM, disk, font, icon, cursor, wave, or maintainer-local reference file was
modified. No real-hardware test was performed or claimed.

## 6. Commits

| Stage | Commit |
|---|---|
| M97 definition | `eafcb77c2b47459a7e044d6074d454b01b07f82a` |
| M97a evidence correction | `da9981cc7bde84057c76a5d87081e4955dfbb8b8` |
| M97b descriptor/LINE/ROP/TP2 | `ffb85210c62f984108ad9d022f7d046107744f60` |
| M97c SCAN implementation | `7b788edc6e657f2d9e8e48f759c5cab6eb7c4899` |
| M97e LINE visual-gate scope | `23c04210eb37e06356b978e752cab9c70bfaa608` |
| M97f SGP wireframe demo | `bf59f1f567f4e815dd0bd671fa174cb1e422a92f` |
| M97g initial visual-validation record | `92b04a4` |
| M97f1 640x400 geometry correction | `7df8bc9529c012695fcb5d38fe677dba984c1eba` |

## 7. Remaining unknowns

- the unnamed thirteenth command;
- the internal 58-byte SET WORK format;
- zero extents and other explicitly undefined descriptor cases;
- LINE tie-breaking details not stated by the recovered diagram;
- SCAN command timing, SGP/CPU bus arbitration, and status timing;
- reserved `TP=3` behavior;
- real-PC-88VA equivalence.

## 8. Human gate

The maintainer passed G97 on 2026-08-24 against the extended candidate
`7ab4fabfff8ace9f7ae8648cee4004ad6d507b94`. It is a VAEG visual-regression
gate and does not require or claim a real-hardware run. `SGPWIRE.COM` remains
the LINE-specific visual part of that gate rather than an optional unavailable
fixture. M97 is closed; real-PC-88VA equivalence and timing remain unproven.
