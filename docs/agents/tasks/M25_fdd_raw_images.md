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
# M25 - D88 and raw formatted FDD images

Status: complete; G25 passed

Branch: `topic/m25-fdd-raw-images`

Depends on: M23 formatted D88 creation and M24 gate completion.

## Scope

- Rename `FDD -> New formatted D88 image` to `New FDD image`.
- Let the creation dialog select `D88` or `IMG (raw)` independently of the
  existing Japanese MS-DOS 2HD/2DD geometry selection.
- Keep the M23 FAT12 contents and D88 bytes unchanged for D88 output.
- Write IMG as contiguous logical sectors without D88 track/sector headers.
- Add `.img` to direct/archive drag-and-drop image recognition.
- Let the active raw loader mount the generated 640 KB 2DD image in addition
  to the already-supported 1.232 MB 2HD geometry.

## Formats

| Disk | Geometry | D88 | IMG raw |
|---|---|---|---:|
| 2HD | 77 cylinders, 2 heads, 8 sectors, 1024 bytes/sector | D88 type `0x20` | 1,261,568 bytes |
| 2DD | 80 cylinders, 2 heads, 8 sectors, 512 bytes/sector | D88 type `0x10` | 655,360 bytes |

Both containers hold the same FAT12 BPB, two FAT copies, empty root directory,
and data area. IMG starts with the FAT12 boot sector and is directly usable by
sector-oriented host tools. For example:

```sh
mdir -i disk.img ::
mcopy -i disk.img host-file.txt ::
mcopy -i disk.img ::guest-file.txt .
```

D88 is not directly usable by mtools because its global and per-sector
metadata are interleaved with sector data.

## Compatibility

The legacy `newdisk_fdd_msdos()` API remains a D88-producing wrapper.
`newdisk_fdd_msdos_ex()` selects the container so existing callers retain
their previous behavior. The raw loader continues to identify headerless
images by exact geometry and now includes 160 tracks x 8 sectors x 512 bytes
as 2DD. Unknown-size raw files remain rejected.

2D remains excluded because G23 testing found `sector not found`; M25 does not
change that deferred compatibility issue. Generated images are formatted data
disks, not bootable system disks, and contain no proprietary OS files.

## Automated verification

ROM-less selftests create both containers for both geometries and verify:

- existing-file overwrite refusal;
- D88 header, track table, sector metadata, and active D88 loader acceptance;
- exact raw size and active raw loader geometry;
- FAT12 BPB fields and second FAT initialization in both containers;
- `.img` drag-and-drop extension recognition.

Linux debug/release and MinGW cross builds, selftest/smoke, repository
invariants, and a frozen-tier empty diff remain required.

## G25 human gate

- Create 2HD D88 and confirm existing M23 mount/read/write behavior.
- Create 2HD IMG, mount it in vaeg, and read/write it from Japanese MS-DOS.
- Use `mdir` and `mcopy` on the unmounted 2HD IMG and confirm exchanged files
  remain visible after remounting in vaeg.
- Repeat vaeg and mtools checks with a 2DD IMG.
- Switch D88/IMG in the dialog and confirm `.d88`/`.img` path normalization.
- Confirm existing filenames are never overwritten.
- Confirm immediate FDD1/FDD2 mounting, reset persistence, and restart restore.
- Drag a direct `.img` and an archive containing `.img`; confirm normal sorted
  FDD1/FDD2 assignment.
- Confirm FDD Open/Eject, D88/archive handling, V3 boot, VA demo, and OS boot.

## Gate result

G25 passed by user report on 2026-07-13 for D88/IMG creation and mtools/raw
interoperability. The deferred 2D compatibility issue remains outside M25.
