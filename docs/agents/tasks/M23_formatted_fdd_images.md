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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M23 - Formatted blank D88 floppy images

Status: complete; G23 passed

Branch: `topic/m23-blank-fdd-images`

Gate: G23 human formatted-media and guest-regression gate

Depends on: M22 implementation.

## Scope

- Add `FDD -> New formatted D88 image` at the bottom of the SDL2 ImGui FDD
  menu.
- Create an empty FAT12 data disk in a D88 container as 2HD (1.232 MB) or 2DD
  (640 KB).
- Let the user edit the destination filename and browse the destination
  directory. Append `.d88` unless that extension is already present.
- Refuse to replace any existing filesystem object.
- Optionally mount the newly created image as FDD1 or FDD2 through the normal
  `diskdrv_setfdd()` path and persist it through `FDD1FILE` / `FDD2FILE`.

## Non-goals

- The generated image is a formatted data disk, not a bootable MS-DOS system
  disk. It contains no operating-system files or proprietary payload.
- 2D creation is deferred. Two G23 attempts produced `sector not found` in the
  active VA FDD subsystem even though older VAEG builds reportedly read 2D
  media. Its compatibility path needs a separate audit before it is exposed.
- No raw disk image, ROM, font, icon, or frozen reference file is changed.
- Archive drop and managed archive storage remain M22 behavior.

## Disk formats

| Choice | D88 type | Geometry | FAT12 BPB | FAT media byte |
|---|---:|---|---|---:|
| 2HD | `0x20` | 77 cylinders, 2 heads, 8 sectors, 1024 bytes/sector | 1 sector/cluster, 192 root entries, 2 sectors/FAT | `0xfe` |
| 2DD | `0x10` | 80 cylinders, 2 heads, 8 sectors, 512 bytes/sector | 2 sectors/cluster, 112 root entries, 2 sectors/FAT | `0xfb` |

Each D88 track contains C/H/R/N sector headers followed by zero-initialized
sector data. Sector zero contains a DOS BPB and FAT12 signature. Both FAT
copies begin with the format's media descriptor and reserved FAT12 entries;
the root directory and data area are empty.

The 2HD layout follows the Japanese NEC/MS-DOS 1.232 MB geometry. The 2DD
layout uses the 640 KB physical geometry with DOS FAT12 logical structures.
Format evidence:

- [D88 format description](https://www.pc98.org/project/doc/d88.html)
- [PC-98 floppy geometry notes](https://www.pc98.org/project/doc/dcp.html)
- [NEC APC MS-DOS System Reference Guide](https://www.bitsavers.org/pdf/nec/APC/NEC_APC_MS-DOS_System_Reference_Guide_Sep83.pdf)
- [GNU mtools standard geometry descriptions](https://www.gnu.org/software/mtools/manual/html_node/geometry-description.html)

## GUI and persistence

The FDD menu has one final submenu with explicit 2HD and 2DD entries.
The dialog opens in the saved FDD directory, provides a full path field, and
allows the format and optional mount target to be changed before creation.
Creation defaults to mounting FDD1. A successful mount uses the same delayed
insert and configuration path as FDD Open and drag-and-drop. Reset and process
restart therefore retain the mounted image until it is ejected or replaced.

## Automated verification

The ROM-less selftest creates both formats and checks:

- exact D88 file size and media type;
- first and last populated track pointers and the zero terminator;
- C/H/R/N sector metadata and sector size;
- BPB bytes/sector, allocation size, FAT count, root entries, total sectors,
  media byte, sectors/FAT, sectors/track, and head count;
- boot signature and initialization of the second FAT;
- refusal to overwrite the generated path.

Repository completion also requires the Linux and MinGW builds, ROM-less
selftest/smoke runs, invariant checks, and an empty frozen-tier diff.

## Agent verification (2026-07-13)

- `cmake --build --preset linux-debug --target vaeg_sdl2` passed.
- Linux debug ROM-less `--selftest` and `--smoke` passed.
- `cmake --preset linux-release` and
  `cmake --build --preset linux-release --target vaeg_sdl2` passed; its
  ROM-less `--selftest` and `--smoke` passed.
- `cmake --preset mingw-cross` and
  `cmake --build --preset mingw-cross --target vaeg_sdl2` passed with the
  existing pinned static SDL2/LibArchive dependency path.
- MinGW PE imports remain limited to Windows system DLLs.
- `check_encoding.py`, `check_eol.py`, `check_case.py`, `git diff --check`,
  and the frozen-tier path diff passed.
- The Linux release binary was copied to the shared test directory as `vaeg`.
  The MinGW binary was copied as `vaeg.exe`; source and destination SHA-256
  were identical.
- macOS was not built because this Linux host has no Darwin SDK.

## Gate feedback

Two G23 attempts reported `sector not found` on generated 2D images. A
double-step adjustment did not restore compatibility, so 2D creation and that
FDD subsystem change were removed from M23. Older VAEG reportedly read 2D
media; recovering that behavior is deferred to a focused compatibility task.

## G23 manual gate

- Create a 2HD image with an edited filename; confirm `.d88` is appended and
  FDD1 mounts it.
- In Japanese MS-DOS, run `DIR`, create/copy a small file, eject and remount,
  and confirm that the file remains readable.
- Repeat creation and `DIR` for 2DD (640 KB).
- Select FDD2 as the post-create target and confirm FDD1 is unchanged.
- Disable post-create mounting and confirm the file is created without
  replacing either mounted drive.
- Attempt to create an existing filename and confirm it is refused without
  changing the existing file.
- Reset and restart after mounting a created image and confirm the FDD menu
  still displays and restores it.
- Confirm existing FDD Open/Eject, direct/archive drag-and-drop, V3 boot, VA
  demo, and OS boot/simple operations remain functional.

## Gate result

G23 passed by user report on 2026-07-13 for the final 2HD/2DD scope. The 2D
compatibility issue remains deferred and is not part of the passed gate.
