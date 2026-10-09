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

# M97 - Complete documented SGP command semantics

Status: **G97 human gate passed on 2026-08-24; M97 closed**

Branch: `topic/m97-sgp-tekumani`

Commit prefix: `M97:` or `M97<stage>:`

## Goal

Implement only PC-88VA SGP behavior that can be derived from the local
PC-88VA Technical Manual without real-hardware timing assumptions. Correct
the current LINE direction decoding, implement the documented SCAN commands,
and add deterministic functional regression coverage.

The rejected deterministic-QA/fake-BIOS design is not part of M97. M97 does
not add a ROM-less launcher, synthetic FDD BIOS, audiovisual recorder, golden
corpus, or non-free integration layer.

## Authority

1. Local PC-88VA Technical Manual, display-system SGP chapter
   (`docs/tekumani/4.TXT`). This reference is maintainer-local and is cited
   only in this tracked report/task, never from source comments.
2. Current VAEG SGP source and existing VA software command streams.
3. MAME only as comparison evidence; it is not a hardware oracle.

## In scope

- Preserve the word-oriented `0500h`/`0502h` command-address interface.
- Preserve END, NOP, SET WORK, SET SOURCE, SET DESTINATION, SET COLOR,
  BITBLT, PATBLT, LINE, and CLS where they already match the manual.
- Decode original-VA block width, height, and framebuffer width from the
  documented 12-bit/12-bit/word-aligned fields while retaining the existing
  later-model profile separately.
- Correct LINE `VD=0800h` and `HD=0400h`.
- Implement `SCAN RIGHT` and `SCAN LEFT` normal functional semantics:
  comparison with SET COLOR, maximum count from destination width, zero
  result when the first pixel matches, unchanged descriptor on no match,
  and the documented destination updates on a match.
- Verify all sixteen documented ROP values and destination-zero transfer
  mode through focused selftests.
- Correct stale SGP documentation where direct manual text resolves it.
- Add one DOS 8.3-compatible LINE visual test, at the maintainer's request,
  with rotating regular tetrahedron, cube, centered regular octahedron,
  regular dodecahedron, and regular icosahedron geometry in 640x400 mode.
  The CPU may project vertices and generate the main-RAM command list; every
  animated edge must be drawn by SGP LINE.

## Out of scope

- Inventing the unnamed thirteenth command.
- Inventing the internal format or write pattern of the 58-byte work area.
- Defining power-on drawing state or behavior before SET WORK.
- SGP cycle accuracy, bus contention, arbitration, or performance claims.
- Zero width/height behavior, 4MiB wrap, start-while-busy, or partial-word
  abort semantics.
- Guessing reserved `TP=3` behavior.
- Changing TSP, framebuffer, SGP speed controls, existing demos, ROMs, or disk
  images. The new isolated LINE visual test in M97e is the sole demo exception.
- Real-hardware validation. Functional results remain manual-derived until a
  later hardware campaign is possible.

## Implementation stages

### M97a - Evidence correction

- Record the command coverage and manual-derived behavior.
- Correct the stale ROP and SCAN conclusions in the SGP reconstruction.

### M97b - Descriptor and LINE decoding

- Apply the original-VA documented descriptor masks without changing the
  existing VA2 profile.
- Use the documented LINE direction bits.

### M97c - SCAN commands

- Add incremental right/left scanning to the existing asynchronous command
  state machine.
- Preserve the destination descriptor when the boundary color is absent.
- Update only the fields documented for each direction.

### M97d - Regression coverage

- Add focused selftests for model-specific descriptors, all ROPs,
  destination-zero transfer, scan first-pixel/middle/not-found/word-boundary
  behavior, and left/right result updates.
- Run all repository validators and supported local builds/tests.

### M97e - LINE visual gate program

- Add `demos/sgp-wireframe/sgp_wireframe.asm`, which builds the DOS 8.3 name
  `SGPWIRE.COM` out of tree.
- Use the existing hardware-safe word access for the command-address and
  display-start ports and begin every command list with SET WORK.
- Render five independently rotating and pulsating solids into the hidden
  half of a 640x800 Graphic 0 framebuffer and exchange its two 640x400 halves
  through DSA0 during vertical blank.
- Use depth-cued edges rather than claim polygon filling: the documented SGP
  command set has LINE but no general polygon or flood-fill command.
- Keep generated COM and disposable disk images outside Git.

#### M97e recorded implementation and color-depth extension plan

The existing wireframe visual test is part of the M97 candidate and must be
treated as the baseline for the later scan/fill experiments:

- `demos/sgp-wireframe/sgp_wireframe.asm` builds `SGPWIRE.COM`, a 16-color
  640x400 single-plane test.
- `demos/sgp-wireframe/sgp_wireframe256.asm` builds `SGP256.COM`, a separate
  8-bpp direct-color test using the existing 320x400 logical viewport.
- The next wireframe extension is implemented as three color-depth tracks under
  `demos/sgp-wireframe/{16,256,65536}/`. The 16-color and 256-color tracks
  preserve the existing 640x400 and 320x400 layouts respectively. The
  65536-color track uses the VA direct-color layout established by the
  PC-Engine 1.00 demonstration trace: a 320x400 source framebuffer at 16 bpp
  (`FBW=640`, `FBL=400`) with a 320x200 display window. Two contiguous
  320x200 pages are stored at source offsets `0` and `1f400h`; DSA0 selects
  the displayed page during VBLANK. The program explicitly writes
  `GRMODE=0xB462` and `GRRES=0x1313`, then programs FB0's pitch, source
  height, display start, and displayed height.
- The CPU performs fixed-point rotation and perspective projection and builds
  the command list in main RAM. SGP performs `CLS` and every animated edge
  through `LINE`.
- The 16-color version uses two 640x400 halves of a 640x800 Graphic 0
  framebuffer. The 256-color version uses two 320x400 halves of a 320x800
  Graphic 0 framebuffer.
- The 65536-color version uses two contiguous 320x200 16-bpp pages in the
  320x400 Graphic 0 source surface. Since the hidden page is linear, one SGP
  CLS command clears its `0xfa00` words before drawing the grid and all edges.
  The display page changes only after SGP completion and the VBLANK wait; the
  projection keeps the vertical coordinate in the 320x200 logical space so
  the display raster does not distort the solids.
- Video BIOS is used only for mode/framebuffer/window/composition setup and
  restoration. SGP command submission, display-page selection, GVRAM mode,
  and VBLANK polling use the verified direct interfaces.
- The test intentionally remains wireframe-only. It does not claim polygon
  filling, SCAN-to-PATBLT chaining, or real-hardware timing equivalence.

The pseudo-sprite extension under `demos/sgp-pseudo-sprite/65536/` is a separate
double-buffered direct-color teaching track. `SGP655S.COM` uses the same
320x400 source / 320x200 display setup as the 16-bpp wireframe, renders up to
128 moving 24x24 transparent 16-bpp BITBLT spheres, and exchanges the two
contiguous pages through the FB0 DSA registers. The 16 source bitmaps cycle
through HSV hues; each remains monochromatic while using supersampled
Phong-style shading. The background is plain black by default; SPACE toggles a
direct-SGP white square grid with 16-pixel cells. The deterministic records
use sixteen integer velocity directions. SPACE toggles it. UP/DOWN (or `+`/`-`)
selects one to 128 active spheres.
Rendering stays on the hidden page, so the earlier single-page partial-draw
tearing is not exposed; it does not alter the M6 4-bpp baselines.

The generated COM files and disposable disk images remain out of the
repository. This recorded baseline must continue to build and run before any
SGPSCAN stage is evaluated.

For local distribution, `demos/sgp-wireframe/build-d88.sh` builds the three
tracks with the common `SGPWIRE.COM` basename and installs them into one
bootable 2HD D88 under `16/`, `256/`, and `65536/`. It copies rather than
modifies the source system image, refuses to overwrite an existing output,
and leaves the generated D88 outside the repository.

### M97f - staged SCAN/PATBLT demonstration family

M97f extends the recorded wireframe baseline with a deliberately staged set
of direct-SGP experiments. The eight stages are implemented on three parallel
color-depth tracks:

```text
demos/sgp-scan/16/SGPSCAN1.COM ... SGPSCAN8.COM      16-color track
demos/sgp-scan/256/SGPSCAN1.COM ... SGPSCAN8.COM     256-color track
demos/sgp-scan/65536/SGPSCAN1.COM ... SGPSCAN8.COM  65536-color track
```

The identical DOS 8.3 basenames are intentional; the parent directory is the
track identity. Thus there are eight functional stages and three color-depth
variants per stage, for 24 generated COM artifacts. Each program is an
independently buildable teaching and regression artifact. No stage may
silently replace an earlier stage.

The initial resolution policy is deliberately capacity-driven and may be
refined only by evidence from the corresponding video-mode probe:

| Track | Initial mode | Page size | Intended exchange |
|---|---|---:|---|
| `16` | 640x400, 4bpp | 128,000 bytes | Existing single-plane Graphic 0 double buffer |
| `256` | 320x400, 8bpp | 128,000 bytes | Existing single-plane Graphic 0 double buffer |
| `65536` | 320x400 source, 320x200 display, 16bpp | 128,000 bytes/page | Single-plane G0 double buffer with contiguous vertical pages |

The 65536-color mode must not be expanded to a 640-dot source unless a
separate capacity and display-path investigation proves that additional
storage and fetching exist. The SGP descriptor supports the 16bpp pixel mode in VAEG, but
the exact hardware direct-color encoding remains a documented verification
item; the demo must not label an emulator-only RGB layout as a hardware fact.

All stages follow the same ownership rule:

- Video BIOS performs only video mode, framebuffer/window, composition,
  palette (where applicable), and restoration setup.
- The CPU performs geometry, face ordering, seed selection, shade selection,
  and command-list construction in main RAM.
- SGP performs `SET_WORK`, `SET_SOURCE`, `SET_DESTINATION`, `SET_COLOR`,
  `SCAN_RIGHT`, `SCAN_LEFT`, `PATBLT`, `LINE`, and `END` as appropriate.
- CPU pixel loops, BIOS polygon drawing, Sprite BIOS, and emulator-only APIs
  are prohibited.
- Every command list begins with `SET_WORK` and uses hardware-safe word
  writes for the SGP command-address and display-start registers.

#### SGPSCAN1 - command and descriptor probe

Create a one-line or one-rectangle probe that draws a known boundary color,
executes `SCAN_RIGHT` and `SCAN_LEFT`, and makes the resulting descriptor
state visible through a following simple operation. Each color-depth variant
uses its track-specific validated mode. This stage must establish:

- first-pixel match;
- later-pixel match;
- no-match behavior;
- packed-word boundary behavior; and
- whether a following SGP operation can consume the updated internal
  destination descriptor.

No animated geometry is allowed in this stage.

#### SGPSCAN2 - SCAN to PATBLT handoff

Use a solid one-pixel source pattern and a single `PATBLT` operation after a
SCAN operation. Demonstrate that only the discovered span is filled. If the
hardware-derived command semantics do not support direct handoff, record the
failure and keep this stage as a diagnostic; do not emulate the handoff with a
CPU pixel loop.

#### SGPSCAN3 - one convex polygon

Draw one closed triangle or quadrilateral with `LINE`, locate its horizontal
spans using both scan directions, and fill them with `PATBLT`. Redraw the
outline after filling. The CPU may generate one command-list record per
scanline, but it may not write individual pixels.

#### SGPSCAN4 - multiple convex polygons

Render several non-overlapping convex polygons in painter order. Preserve
outline visibility, avoid boundary-color collisions, and verify that old spans
are cleared on the hidden page before the page is displayed. This is the first
stage that exercises multiple SCAN/PATBLT spans per frame.

#### SGPSCAN5 - shaded polyhedron baseline

Complete the 16-color visual track using a rotating convex polyhedron or a
small collection of convex faces. CPU-side face normals or a fixed lighting
table select palette indices; SGP performs the face spans and outlines. Use
the existing 640x400 double-buffered Graphic 0 arrangement and VBLANK page
exchange. The 256-color and 65536-color directories contain the equivalent
stage in their own pixel formats. This stage must remain visibly distinct
from the wireframe-only `SGPWIRE.COM` baseline.

#### SGPSCAN6 - direct-color single-face validation

Port the proven convex-face path to the direct-color layout of each track.
For 256 colors, validate descriptor start-dot, source pattern packing, PATBLT
span width, and the 3:3:2 color encoding with one animated face before adding
multiple faces. For 65536 colors, validate the 16bpp word color, explicit G0
`GRRES`, and the 320x400 single-plane G0 single-page arrangement. The 16-color track retains the
same stage number as its corresponding single-face validation.

#### SGPSCAN7 - direct-color shaded polyhedron

Add multiple visible faces, painter-order overlap, depth-based shade choice,
and outline restoration to the 256-color and 65536-color tracks. The
implementation must not assume that the 16-color framebuffer geometry,
palette behavior, or 8bpp packing transfers unchanged to another track.

#### SGPSCAN8 - final scan/fill stress and teaching example

Combine the validated scan/fill path with rotating and pulsating convex
polyhedra, bounded command-list capacity checks, ESC exit, hidden-page
redraw, and diagnostic counters. Keep all three color-depth variants in their
separate directories. This is the final visual candidate, not a claim of real
PC-88VA throughput.

#### SGPSCAN stage acceptance

For every stage and all three tracks, record:

- exact NASM command and output path;
- command-list opcodes used;
- whether SCAN-to-PATBLT handoff is direct or explicitly marked unsupported;
- mode, logical resolution, bpp, framebuffer/page addresses;
- VAEG selftest/build result;
- visual result and known limitations.

The final M97 visual gate must run the existing wireframe baselines and the
three new wireframe tracks, followed by all available `SGPSCAN1.COM` through
`SGPSCAN8.COM` variants in all three color-depth directories. A failed
later stage must not be reported as a failure of an earlier stage that already
passed. SCAN semantics remain manual-derived and timing claims remain out of
scope.

Tracked source paths must remain lowercase and separated by color track, for
example `demos/sgp-wireframe/65536/sgp_wireframe.asm` and
`demos/sgp-scan/256/sgpscan1.asm`. The uppercase COM names are generated DOS
artifacts and remain outside Git unless a later release task explicitly
authorizes distribution media.

## Validation

```text
GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null git diff --check
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/clang_format.py
python3 tools/repo/find_unreferenced.py --report
cmake --preset linux-debug
cmake --build --preset linux-debug -j
GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null ctest --preset linux-debug
build/linux-debug/sdl2/vaeg --selftest
```

Also run the repository-supported MinGW cross-build discovered from the
current preset/CI configuration.

## G97 human gate

No real PC-88VA is required for this gate.

From a clean checkout of the candidate:

1. boot VAEG in the normal VA configuration;
2. run the existing SGP pseudo-sprite demo and verify its background,
   transparency, sprite overlap, animation, and clean exit;
3. run the three wireframe tracks under `demos/sgp-wireframe/16`,
   `demos/sgp-wireframe/256`, and `demos/sgp-wireframe/65536`. Verify the
   regular tetrahedron, cube, centered regular octahedron, regular dodecahedron,
   and regular icosahedron in each track's documented resolution and color
   depth; verify rotation,
   scale changes, connected dim/bright edges, and LINE direction in all
   visible octants;
4. run each available `SGPSCAN1.COM` through `SGPSCAN8.COM` from all three
   tracks. Confirm the documented stage progression: probe,
   SCAN-to-PATBLT handoff, one convex polygon, multiple convex polygons,
   shaded geometry in the track's color depth, single-face validation where
   applicable, multi-face shading, and the final bounded stress example. If a
   stage is intentionally diagnostic or marked unsupported, verify that its
   diagnostic result is explicit rather than treating it as a visual pass;
5. verify normal V3/OS boot and display operation are unchanged.

Automated tests establish manual-derived functional behavior. The human gate
checks visual regression only; it does not claim real-hardware equivalence.

The maintainer stated that G97 passed on 2026-08-24. M97 is now a historical
record and must not be started again. The gate remains a VAEG visual-regression
acceptance only; it does not establish real-PC-88VA equivalence or timing
behavior.
