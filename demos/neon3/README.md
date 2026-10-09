<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# NEON3 PC-88VA port

This directory is the P3 work tree for the `NEON3286` migration.  The source
under `src/` includes the original NEON3 fixed-point geometry from
`demos/neon3_1_5/98/` without modifying or copying that source.

The current payload is the P3-B geometry/SGP harness.  It reuses the original
PC-98 projection and nine-scene, 6144-frame timeline, while replacing the
original raster backend with VA BIOS mode entry and a bounded VA SGP command
list.  LINE and complete-word CLS records are submitted and waited on once
per logical frame.  Partial 4bpp endpoint read-modify-write and water-raster
CPU pixels are deliberately not part of this increment.

Both logical profiles are built with the same geometry:

```text
./build.sh 200 /absolute/path/neon200.bin
./build.sh 400 /absolute/path/neon400.bin
```

For a bounded local smoke payload, set `NEON_BUILD_FRAME_LIMIT` to a positive
decimal value.  The default remains the original 6144-frame timeline:

```text
NEON_BUILD_FRAME_LIMIT=1 ./build.sh 200 /absolute/path/neon200-one.bin
```

Profile 200 maps logical Y to the 640x200 display window.  Profile 400 keeps
the full 640x400 window.  Both profiles use the same logical scene and SGP
command format; the physical Y mapping and clear-page word count are the only
profile differences.

The files produced by `build.sh` are raw payloads.  For the local bootable
PC-Engine validation disk, wrap each raw payload with
`src/neon_payload_loader.asm` before installing it.  The wrapper copies the
payload to the loader-owned segment and records the validated return
continuation; installing the raw payload directly leaves the loader stub in
control and produces a blank/hung run.  A bootable PC-Engine validation disk
is a local test artifact and is not a repository deliverable.

For example:

```sh
nasm -f bin -O2 \
  -dNEON_PAYLOAD_FILE=\"/absolute/path/NEON200.raw.COM\" \
  -I demos/neon3/src/ demos/neon3/src/neon_payload_loader.asm \
  -o /absolute/path/NEON200.COM
```

## SGP command-list capacity experiment

The default build keeps a 20 KiB command list in the payload segment.  That
layout is intentionally unchanged.  To test whether the late-scene stop is
capacity-related without growing the COM image, an opt-in build places the
list in main RAM segment `2000h` and selects a larger capacity:

```sh
NEON_BUILD_SGP_EXTERNAL_LIST=1 \
NEON_BUILD_SGP_LIST_CAPACITY=24576 \
./build.sh 200 /absolute/path/neon200-external24k.raw
```

The external-list mode is a diagnostic experiment, not a new default hardware
contract.  It must be compared with the default payload using the same D88,
frame limit, and status capture.  A static 24 KiB list in the payload is
rejected by the `E000h` loader-reserve guard instead of silently producing an
oversized payload.  It is incompatible with the optional
`NEON_SAFE_BIOS_STACK` build because that mode also reserves segment `2000h`.

## Distribution disk

[`neon3-distribution.d88.xz`](../disks/neon3-distribution.d88.xz) is the distributable
non-bootable data disk.  Its root directory contains only the source-built
`NEON200.COM` and `NEON400.COM` validation payloads; it contains no PC-Engine
system files, ROMs, or private media.  The raw D88 is a local generation
intermediate and is not distributed.  Rebuild the two wrapped payloads with
the loader command above, create an empty data disk from a vanilla validation
image with `tools/pc88va/pcengine_disk.py data`, install the two files with
`pcengine_disk.py install`, and compress the resulting D88 with `xz -c -9`.
The checked-in `demos/disks/neon3-distribution.d88.xz` must pass an `xz`
round-trip comparison before it is updated.

## P3 human gate

The P3 gate disk contains four independent COM programs.  Run them from the
PC-Engine prompt and record the visible result and the status screen.  The
programs use VA BIOS services (INT 8Fh for video, INT 83h for text/soft-key
control, INT 94h for system-line visibility, and INT 82h for keyboard); they
do not use DOS INT 21h services.  Startup uses Text BIOS function 2Fh with
`AL=00h` and Screen Editor function 01h with `AL=FFh` to suppress the inherited
function-key/system-line guide.  TEXT remains enabled for the NEON overlay.
On exit the payload restores the ten-entry guide with `AL=0Ah`.

* `NEONINI.COM` performs VA V3/G0 initialization only.  Press `ESC` to
  verify the VA BIOS leave path and return to the prompt.
* `NEONSMK.COM` clears G0 and submits a deterministic white rectangle through
  SGP `CLS`/`LINE`.  The rectangle must be visible.  Press `ESC` after the
  idle state is reached.
* `NEON200.COM` continuously loops the complete 640x200-profile timeline
  (6144 logical frames).  Startup clears TVRAM once through the VA Text BIOS,
  then writes the static overlay labels once.  After each completed frame and
  VBLANK page flip, only the fixed-width frame values are overwritten; the
  scene title field is updated only when the scene changes.  The cursor is
  hidden with Text BIOS `INT 83h/AH=25h, AL=00h` while the demo runs.  Press
  `ESC` to leave the loop.
* `NEON400.COM` continuously loops the same 6144-frame timeline with the
  640x400 logical profile and the same incremental live text overlay.
For each run, check:

1. the graphics result is visible before the status screen;
2. the status screen is readable and `BIOS` is `0000` (no failure marker);
3. `BIOS RC` is `0000` and `SGP` is `0001` after a completed SGP list;
4. `ESC` returns through the loader without a reset or hang;
5. after returning, the prompt accepts another command.

The VAEG headless capture is only a diagnostic aid.  A black or missing
capture is reported as `UNCONFIRMED`; it is not a human-gate pass.  Real
PC-88VA/VA2 observation is the acceptance criterion for P3.

## Double buffering

`NEON200.COM` and `NEON400.COM` render each SGP command list to the hidden
FB0 page and present it only after SGP completion at a VBLANK boundary.  The
200-line profile exchanges pages at `0xFA00` bytes (SGP bases `0x200000` and
`0x20FA00`); the 400-line profile exchanges pages at `0x1F400` bytes (SGP
bases `0x200000` and `0x21F400`).  FB0 DSA word ports `020Eh/0210h` are used
for the display source.  The 400-line BIOS descriptor remains one 640x400
window; the second physical page is selected through the DSA/SGP offset pair
because the VA BIOS rejects an 800-line descriptor.  These are the same
documented two-page values used by the existing VA SGP wireframe contract.
Real PC-88VA/VA2 timing remains hardware-pending until observed on each
machine.

The SGP command buffer is a bounded per-batch queue.  Dense scenes may emit
more commands than fit in one queue; the backend closes the current batch
with `END`, waits for SGP idle, then starts another batch with `SET_WORK` and
the current colour before continuing the same hidden page.  A command is
reserved as a whole, so neither `LINE` nor `CLS` can be split across batches.
The compiled timeline length remains `0x1800` (6144).  Release payloads wrap
back to frame zero after that point and continue until `ESC`; a shorter limit
is available through `NEON_BUILD_FRAME_LIMIT`.  Diagnostic builds that stop
at the selected limit set `NEON_BUILD_LOOP=0` as well.
