<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# NEON RELAY 4 PC-88VA profiles

The NEON4 source under `src/` builds both profiles from one NASM tree.  The
scene geometry and the complete eight-scene, 3072-frame timeline are shared;
both published profiles loop back to scene 0 after the final scene until ESC.
The profile value is selected by `NEON4_P5_PROFILE`:

| Profile | Executable directory | VA mode | Colour representation |
| --- | --- | --- | --- |
| `16` | `16/neon4.com` | 640x400, packed 4bpp G0 | 16 indices, each selected from the 4096-colour palette |
| `65536` | `65536/neon4.com` | 320x200, direct 16bpp G0 | native VA direct colour |

Both profiles keep the VA TEXT plane enabled.  Stage 8 takes ownership of the
console with Text BIOS `INT 83h/AH=2Fh, AL=00h` and Screen Editor BIOS
`INT 94h/AH=01h, AL=FFh`, removing the inherited function-key/system-line
guide without hiding NEON4's own text.  The live title, profile, scene, frame,
and ESC status are refreshed through the same BIOS overlay sequence used by
NEON3.  Startup clears all TVRAM rows while the display is hidden, hides the
text cursor, and updates dynamic fields as one group immediately after each
VBLANK edge.  Exit restores the normal ten-entry guide (`AL=0Ah`) and cursor
mode.

## 16-colour palette mode

The 640x400 profile is not RGB332.  Each pixel is a four-bit palette index,
and the sixteen palette entries are 12-bit colours selected from the VA's
4096-colour space.  The word written to `$SetPal` follows the PC-88VA
technical manual layout documented in `docs/tekumani/PC88VA_テクニカルマニュアル_BNN.md`:

```text
G[3:0] -> bits 15..12
R[3:0] -> bits 9..6
B[3:0] -> bits 4..1
```

The source table is stored as G/R/B nibbles in `src/low4_data.inc`; the VA
backend converts those nibbles at startup through BIOS `INT 8Fh`, `$PalCtl`
(`AX=0900h`), and `$SetPal` (`AX=0800h`, `AL=index`, `CX=value`).  Graphics
composition is then selected with `$Compose` G0-only (`AX=0300h`,
`CX=0003h`).  No RGB332 conversion or per-frame palette animation is used by
this profile.

The 4bpp framebuffer uses the validated FB0 layout: a 640x400 page is 128,000
bytes (`320` bytes per row), two pages occupy the 256 KiB single-plane GVRAM
window, and the second page begins at SGP address `21F400h` / DSA offset
`01F400h`.  The `$DefBuf` pixel-size (`DOT`) field is `4`, meaning packed
four-bit pixels; it is not a direct-colour value.  Logical scene coordinates
are already 640x400, so this profile does not halve X or Y at the primitive
boundary.

## Build

Build either raw loader payload directly:

```sh
NEON4_P5_PROFILE=16 ./build.sh /absolute/path/neon4-16.com
NEON4_P5_PROFILE=65536 ./build.sh /absolute/path/neon4-65536.com
```

`build.sh` wraps the final payload with the validated VA loader return
continuation.  `NEON4_P5_PROFILE=16` is the 640x400 palette build; the numeric
name is a profile identifier, not a 16bpp mode.

## D88 images

`build-d88.sh` creates one non-bootable distribution data disk containing only
the two freely distributable payloads:

```text
A:\16\neon4.com
A:\65536\neon4.com
```

It also writes the compressed companion to `demos/disks/` using the raw
output basename (for example, `demos/disks/neon4-distribution.d88.xz`).  The
only generated binary allowed to be tracked is the reproducible distribution
image; raw D88 images and bootable validation disks remain local artifacts
outside Git.  The source 2HD template is used only for geometry and is never
modified.

```sh
./build-d88.sh /path/to/pcengine110-bootonly.d88 \
    /absolute/path/neon4-distribution.d88
```

For local VAEG or real-machine validation, use the bootable builder.  It keeps
the PC-Engine system files from the supplied local template and installs the
same two payload directories:

```sh
./build-bootable-d88.sh /path/to/pcengine110-bootonly.d88 \
    /private/tmp/neon4-bootable.d88
```

The bootable image is deliberately not a repository artifact.

## Verification boundary

The 16-colour mode setup and palette writes are exercised by the existing SGP
VAEG path.  The palette word layout is derived from the Tekumani BNN manual
and the VAEG `adjustcolor12()` implementation.  Final acceptance on PC-88VA
hardware remains a separate hardware gate; VAEG functional output does not
claim silicon timing or colour-converter equivalence.
