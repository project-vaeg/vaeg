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

# SGP wireframe demo

`SGPWIRE.COM` is a 16-bit real-mode PC-88VA visual test for the SGP `LINE`
command. It displays a regular tetrahedron, cube, centered regular
octahedron, regular dodecahedron, and regular icosahedron in 640x400 16-color
single-plane mode. Every edge is drawn by SGP `LINE`; the CPU only rotates and
projects vertices and builds the main-RAM command list.

The solids rotate on two axes and pulse at independent rates. Edges nearer the
viewer use a bright palette index and edges farther away use a dim index. The
demo is intentionally SGP `LINE`-only: no polygon-fill path is active. Space is
reserved and ignored in this build; ESC exits.

Each command list also contains invisible one-pixel `SCAN_RIGHT` and
`SCAN_LEFT` probes at the first destination pixel. They are not part of the visual
effect; with a trace-enabled VAEG build they provide a direct headless check
that both SGP scan command routes are fetched and executed.

Graphics BIOS defines one 640x800 Graphic 0 framebuffer. Its two 640x400 4-bpp
halves exactly fit in 256,000 of the 262,144 single-plane GVRAM bytes. The SGP
clears the hidden half, redraws a dark reference grid and all five solids, and
then selects that half through DSA0 during vertical blank. The command list
begins with `SET WORK`, and the command-address and display-start ports are
accessed as words for real-hardware safety.

Build with:

```sh
NASM=/opt/local/bin/nasm demos/sgp-wireframe/build.sh /tmp/SGPWIRE.COM
```

Copy `SGPWIRE.COM` to a bootable PC-88VA DOS disk and run it from the command
line. Press Escape to restore the previous video mode and return to DOS.

## 256-color variant

`SGP256.COM` is a separate direct-color variant. It selects a 320x400
single-screen, single-plane display mode with G0 at 8 bpp, registers one 320x800 G0
framebuffer (the full 256000-byte single-plane G0 allocation), assigns G0 to
the direct-color priority path, and uses its two 128000-byte halves as pages
at SGP addresses `200000h` and `21f400h`.
G1 is disabled in this variant, so the two-screen G1 384-pixel alignment
constraint is not involved.
The 320-pixel logical viewport is expanded by the VA display path to the
640-pixel host surface. The vertex projection therefore doubles its vertical
excursion so that the wireframe solids retain a near-square appearance.
The existing 16-color `SGPWIRE.COM` and its Graphic 0
implementation are not changed. The 8-bit line colors use the PC-88VA 3:3:2
RGB encoding; no palette BIOS calls are required for this variant.

Build it with:

```sh
NASM=/opt/local/bin/nasm demos/sgp-wireframe/build256.sh /tmp/SGP256.COM
```

## Color-depth teaching tracks

The same reviewed LINE implementation is also available in separate DOS 8.3
tracks. The wrapper sources keep the 16-color and 256-color programs identical
to the established M97e baselines while giving each track an independent build
entry point:

```sh
NASM=/opt/local/bin/nasm demos/sgp-wireframe/16/build.sh /tmp/SGPWIRE.COM
NASM=/opt/local/bin/nasm demos/sgp-wireframe/256/build.sh /tmp/SGP256.COM
NASM=/opt/local/bin/nasm demos/sgp-wireframe/65536/build.sh /tmp/SGP65536.COM
```

`demos/sgp-wireframe/65536/` is a direct-color 16-bpp test with a 320x400
source framebuffer and a 320x200 display window. Its source pitch is 640
bytes per line. Two contiguous 320x200 pages occupy the upper and lower
halves of the source surface, at byte offsets `0` and `1f400h`; DSA0 selects
the displayed page during VBLANK. Because the hidden page is contiguous, one
linear SGP CLS command clears it instead of issuing one command per row. The
program selects the 200-line G0 mode, writes the explicit VA
`GRMODE=0xB462` and `GRRES=0x1313` values, and programs FB0 with
`FBW=640`, `FBL=400`, `DSH=200`, and `DSP=0`. The CPU builds command lists in
main RAM, and SGP performs the page clear, reference grid, and every animated
edge through CLS and LINE. This avoids exposing a partially redrawn page
while retaining the 320x200 aspect correction.

The 16-bit direct-color words follow the existing VAEG direct-color convention
for this visual test. The track does not make an independent claim about the
component naming of the hardware word format. All three tracks use the
hardware-safe word access already established by the baseline and begin each
SGP list with SET WORK.

## Non-bootable D88 containing all tracks

`build-d88.sh` builds all three variants with the same DOS basename and
installs them into separate directories on one non-bootable 2HD data image:

```text
A:\16\SGPWIRE.COM
A:\256\SGPWIRE.COM
A:\65536\SGPWIRE.COM
```

The source image is copied before installation and is never modified. The
output image must not already exist.

```sh
NASM=/opt/local/bin/nasm demos/sgp-wireframe/build-d88.sh \
    /path/to/pcengine110-bootonly.d88 /tmp/sgp-wireframe.d88
```

The first argument is used only as a local 2HD/FAT12 geometry template. The
generator first creates an empty data disk, so the output contains no
`ENGINEIO.SYS`, `PCENGINE.SYS`, `ADVGBIOS.SYS`, or `PCENGINE.COM` and cannot
boot by itself. The command writes `/tmp/sgp-wireframe.d88` and its compressed
companion to `demos/disks/sgp-wireframe.d88.xz`. Generated COM files and the
raw D88 remain local build artifacts. The `.d88.xz` companion may be checked in
only when it contains the freely distributable wireframe payloads; the source
template and any bootable or PC-Engine system image remain outside the
repository.
