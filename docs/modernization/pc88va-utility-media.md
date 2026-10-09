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
# Auto-generated PC-88VA Utility Media

This note summarizes the external PC-88VA HDD environment recipe centered
on PCEPAT, MSE, and PCPLUS. It is a source-reference note for future vaeg
SASI/HDD workflow work. Public third-party inputs are fetched and verified by
the builders, but their binaries are generated into untracked D88/HDI images
and are not stored in this repository.

All PC-88VA media builders in this note share one host-side input cache:
`~/.cache/vaeg/auto-generated-pc88va-utility-media/`. Verified public
archives used to create utility media are stored directly in that directory,
whether the result is a floppy D88 disk or a SASI HDD HDI. There are no
separate development-disk or Softlib cache directories, and the builders no
longer read the former `VAEG_*` cache/archive environment variables. Generated
media remains a local artifact at the path supplied by the relevant output
option and is never added to Git.

## Source Notes

The main setup recipe is the TomoRetroPC article "PC-88VA HDD internal
environment setup memo":

- https://tomoretropc.blogspot.com/2019/04/pc-88va-hdd.html

The article assumes a SASI HDD boot environment already exists, then adds
the PC-Engine compatibility support and MS-DOS tool environment. It points
to PC88.gr.jp for most PC-88VA software and to "Madoushi no Atochi" for
MSE 3.52.

The old "Madoushi no Atochi" site is best read through the Internet
Archive:

- https://web.archive.org/web/20071017185024/http://hp.vector.co.jp/authors/VA015636/

The archived page exposes MSE 3.52a and a 3.52a-to-b diff archive. The
PC88.gr.jp link used by the article for PCEPAT is:

- http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=330

That PC88.gr.jp page is PCEPAT, not MSE. It lists `PCEPAT.COM` for V3
mode PC-Engine environments.

The PCEPAT archive includes `PCEPAT.DOC`, which identifies the package as
`PCEPAT for PC-Engine v1.05/1.1 Rev.50916`, copyright 1991-1992 mami.

The bank-memory support package referenced by the MSE documentation is
Vector's BMS Driver:

- https://web.archive.org/web/20190326051933/https://www.vector.co.jp/download/file/dos/hardware/fh090419.html
- https://web.archive.org/web/20190326051933/https://www.vector.co.jp/soft/dos/hardware/se090419.html

Vector describes it as `BMS Driver 1.50 Rev 0.20`, a Bank Memory Driver
for PC-98x1/88VA. The archived download is `bms15020.tgz`.


The archive disk also includes Vector's [Memory Mapper for PC
1.3](https://www.vector.co.jp/soft/dos/hardware/se128128.html). Vector
describes `X8MAP130.LZH` as a free MS-DOS utility for PC-98, PC-88VA, and
AT-compatible machines that reports CPU and memory usage, including
SYSTEM/EMS/XMS/UMB/BMS and VA-only SMM information. The published file is
12,641 bytes and dated 2003-02-06.

[RDPCM.SYS 0.01](http://www.pc88.gr.jp/softlib/?action=list_file&anum=2&gnum=388)
is the Softlib `RDPCM001.LZH` package. It turns the 256KB ADPCM sample RAM on
a PC-88VA Sound Board II into a PC-Engine RAM disk.

[RESET for PC-Engine Rev.51028](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=340)
is the Softlib `RESET.ZIP` package. It adds a warm-reset key chord to
PC-Engine v1.05/1.1.

[TSCLVA](http://www.pc88.gr.jp/softlib/?action=list_file&anum=2&gnum=309)
is a PC-Engine screen-editor BIOS accelerator. The base Softlib page labels it
Rev.50702, while both files inside `TSCLVA.ZIP` identify that payload as
Rev.50703. The adjacent
[TSCLBDF update](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=346)
provides `TSCLVA.BDF`, which advances both `TSCLVA.SYS` and `TSCLVA.DOC` from
Rev.50703 to Rev.51127. The builder applies and verifies that update.

[SCFORM](http://www.pc88.gr.jp/forum/viewtopic.php?t=502) is included from
the forum's `scf124.lzh` download. It provides the SCSI formatter command and
its original documentation; the development-disk builder installs
`SCFORM.COM` in `A:\BIN`.

[VIEW480](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=404)
is built from `V480SRC.LZH` (`VIEW480.ASM`) with the pinned Open Watcom
`wasm`/`wlink` container in TASM-compatibility mode. The resulting
`VIEW480.COM` is installed in `A:\BIN`, while the source archive is retained
under `A:\ARCHIVE`.

[JWasm v2.20](https://github.com/Baron-von-Riedesel/JWasm/releases/tag/v2.20)
is a free MASM-compatible assembler with a DOS real-mode executable. The
SASI development-disk builder pins `JWasm_v220_dos.zip` at SHA-256
`e4cab76e0cdc038e4bc284be136cbd0e5116b02a0a2a76fc4a12cad326224723`, keeps
the original package as `A:\ARCHIVE\JWASM220.ZIP`, and installs
`A:\BIN\JWASMR.EXE`. Its readme and license are installed as
`A:\DOC\JWASM.TXT` and `A:\DOC\JWASM.LIC`. `JWASMR.EXE` is the real-mode
16-bit DOS tool; it is not loaded by `CONFIG.SYS`, because `A:\BIN` is already
on `PATH`.

[JFPPAT](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=307)
is a PC-Engine patch driver. The builder installs `JFPPAT.SYS` in `A:\SYS`,
loads it immediately after `PCEPAT.SYS`, and retains `JFPPAT.ZIP` and its
documentation. The related [2HCDRV package](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=306)
is installed as `2HCDRV.COM` and `FDFORM.COM`; `FDFRMSRC.LZH` is retained as
the formatter source archive.

[ISHARC](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=69)
is the VA-patched ISH 1.12c archive utility package. The builder extracts
`ISHVA.COM`, `PKPAK.EXE`, and `PKUNPAK.EXE` into `A:\BIN`, keeps the original
self-extracting `ISHARC.COM` under `A:\ARCHIVE`, and installs both the Softlib
note and the package README under `A:\DOC`.

[TENIM3](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=74)
is the V3 PC-Engine sample "3D mouse tennis" package. Its self-extracting
`TENIM3.COM` and its Softlib note are retained under `A:\ARCHIVE` and
`A:\DOC`. The archive is kept intact because expanding its image and pattern
assets would exceed the fixed FAT12 space of the development disk; copy it to
a working directory and run it there when the complete sample tree is needed.

[TFD12](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=348)
is the free TFD 1.2 floppy-speed adapter. The builder installs `TFD.SYS` in
`A:\SYS`, retains `TFD12.LZH` under `A:\ARCHIVE`, and installs both the
Softlib note and the full manual under `A:\DOC`. TFD is not enabled in the
generated `CONFIG.SYS` by default: its manual says that EMS must be loaded
before TFD and that EMS-backed operation is not supported by emulators. A
hardware user can add `DEVICE = A:\SYS\TFD.SYS -MT-` after the EMS drivers.

## Core Components

The development environment assembled below uses this baseline `CONFIG.SYS`:

```dos
FILES   = 20
BUFFERS = 30
DEVICE = A:\SYS\EMMVA01.SYS
DEVICE = A:\SYS\SQEMM98.SYS
DEVICE = A:\SYS\EMMVA02.SYS
DEVICE = A:\SYS\PCPLUS.SYS
DEVICE = A:\SYS\BMSDRVA.SYS /P
DEVICE = A:\SYS\SCHD.SYS -I0
DEVICE = A:\SYS\HOSTFAT.SYS
DEVICE = A:\SYS\PCEPAT.SYS
DEVICE = A:\SYS\JFPPAT.SYS
DEVICE = A:\SYS\RESET.SYS
DEVICE = A:\SYS\TSCLVA.SYS
DEVICE = A:\SYS\MSE352B.COM /A /B
DEVICE = A:\SYS\RDBMS.SYS -P1D0 -S2
DEVICE = A:\SYS\RDEMS.SYS -P128 -A
DEVICE = A:\SYS\RDPCM.SYS
```

Here `A:` is the booted VA environment. The exact drive letter can differ if
the HDD/FDD boot layout differs.

`PCEPAT.SYS` is produced by running `PCEPAT.COM` inside PC-Engine. The
article describes it as a PC-Engine bug-fix and function-extension layer.
The PCEPAT documentation is more specific: it says to add
`DEVICE=PCEPAT.SYS` to `CONFIG.SYS` and place it before the MSE driver.
The example in that document uses:

```dos
FILES   = 20
BUFFERS = 30
DEVICE = PCEPAT.SYS
DEVICE = MSE312.SYS
```

`MSE352B.COM` is the MS-DOS application emulator for PC-Engine. The
archived MSE package provides 3.52a, and the 3.52b form is produced by
applying the archived diff with WSP. The included MSE documentation says
MSE can be loaded either as a command-line resident program or as a
`DEVICE=` line in `CONFIG.SYS`. The development disk passes `/A` to place
the Alias data in BMS and `/B` to swap part of MSE's code data into BMS.
It does not pass `/X`, because the disk does not install an XMS manager.

Useful MSE-side tools in the archived package include:

- `MSET.COM`: changes MSE interrupt/emulation settings after boot.
- `ALIAS.COM`: inspects, adds, or removes MSE aliases.
- `MSECUST.COM`: embeds aliases and mode rules into an MSE executable.
- `MSE350.DEF`: an example custom definition file.

`PCPLUS.SYS` is built from the PCPLUS archives referenced by the article:
Softlib group 2-378 `PCP108.LZH` plus the group 2-451 `PCP108P.LZH` bug-fix
patch. Group 2-451 describes the latter as a PCPLUS v1.08 bug fix, and the
patched driver still identifies the overall package as v1.08. Its embedded
`$INTTRG` service identifies itself as v1.09, while this patch advances the
embedded `$SCSIBIOS` service from v1.07 to v1.08. The article treats PCPLUS
as another PC-Engine extension layer.

The development disk also installs the two commands supplied under
`PCP108/BIN`: `SMSTAT.COM` reports Sound Memory Manager allocation, and
`SETDMA.COM` supports the PC-9801-55-compatible SCSI DMA setup. Both pass
through the same DIET stage as every other `.COM` file in `A:\BIN`. The
PCPLUS redistribution terms, main manual, and SCSI notes are retained as
`PCPLUS.DOC`, `PCPLUS.TXT`, and `SCSI55.TXT` in `A:\DOC`.

## BMSDRVA and TSCLVA Load Order

The BMS Driver 1.50 Rev 0.20 archive contains `BMSDRVA.COM` and a
`BMSDRSYS.WUP` conversion patch. The builder applies that patch with the
original WSP utility, verifies the resulting `BMSDRVA_.SYS`, installs it as
`A:\SYS\BMSDRVA.SYS`, and loads it immediately after PCPLUS. The COM form
remains in `A:\BIN` for its interactive status and management commands and is
processed by DIET with the other BIN executables. The main manual, history, and
package header are retained in `A:\DOC`.

TSCLVA Rev.51127 accelerates the PC-Engine v1.05 screen-editor BIOS text-output
services. Its manual reports a resident size of `0430h` bytes on the original
PC-88VA, PC-88VA2/3, and a PC-88VA with the version-up board. The manual
requires PCEPAT to precede TSCLVA and places TSCLVA before MSE. The
development-disk order therefore uses:

```dos
DEVICE = A:\SYS\PCPLUS.SYS
DEVICE = A:\SYS\BMSDRVA.SYS /P
...
DEVICE = A:\SYS\PCEPAT.SYS
DEVICE = A:\SYS\TSCLVA.SYS
DEVICE = A:\SYS\MSE352B.COM /A /B
...
DEVICE = A:\SYS\RDEMS.SYS -P128 -A
```

RDEMS is the EMS-backed RAM disk and intentionally loads after TSCLVA. TSCLVA
itself does not provide EMS or a RAM disk.

## RESET Warm-reset Driver

RESET Rev.51028 is a character-device driver for PC-Engine v1.05/1.1. Pressing
`Ctrl`+`GRPH`+`DEL` invokes its warm-reset path. The manual reports a resident
size of `0200h` bytes on the PC-88VA, PC-88VA2/3, and a PC-88VA with the
version-up board, and requires PCEPAT to be loaded first. The development disk
therefore places RESET immediately after PCEPAT and before TSCLVA:

```dos
DEVICE = A:\SYS\PCEPAT.SYS
DEVICE = A:\SYS\RESET.SYS
DEVICE = A:\SYS\TSCLVA.SYS
```

The reset path reinitializes the memory map, video state, floppy interface,
keyboard, TSP, and related I/O before transferring control to the firmware
reset vector. The manual explicitly notes that it does not initialize the
YM2608 sound device. `RESET.SYS` and `RESET.DOC` are installed in `A:\SYS` and
`A:\DOC`; `RESET.ASM` remains only in the preserved original archive. The
package contains no EXE or COM file, so it adds no new DIET target. The builder
still reprocesses every existing `.EXE` and `.COM` under `A:\BIN`.

## RDPCM ADPCM-memory RAM Disk

`RDPCM.SYS` 0.01 uses the Sound Board II ADPCM sample RAM as a volatile
PC-Engine block device. The driver exposes about 256KB, with 253KB available
for file data after its two FAT copies and root directory. Its manual documents
1024-byte sectors, one sector per cluster, 32 root-directory entries, and a
warm-boot signature that preserves the RAM-disk contents across reset. A power
cycle or any software that overwrites the PCM RAM destroys those contents.

The original PC-88VA needs an added PC-88VA-12 Sound Board II; later machines
need the corresponding ADPCM memory. The manual warns that RDPCM cannot coexist
safely with FM/ADPCM software, including music TSRs and keyboard-click tools,
and that byte-at-a-time I/O makes it unusually slow. The development disk loads
it last so it cannot change the ordering requirements of the HDD, MSE, and EMS
stacks:

```dos
DEVICE = A:\SYS\RDPCM.SYS
```

The generated disk retains `RDPCM.SYS` in `A:\SYS` and `RDPCM.DOC` in
`A:\DOC`. The source `RDPCM.ASM` remains in the preserved original archive
rather than occupying additional boot-disk space. The package has no EXE or
COM file; the builder still applies its normal DIET pass to every existing
`.EXE` and `.COM` under `A:\BIN`.

## PCEPAT PC-Engine Patch

PCEPAT is a small resident patch for PC-Engine v1.05 and v1.1. Its
documented resident sizes are:

- PC-88VA, PC-Engine v1.05: `0B50h` bytes.
- PC-88VA2/3, PC-Engine v1.1: `05E0h` bytes.
- PC-88VA with PC-88VA-91, PC-Engine v1.1: `0580h` bytes.

Its fixes fall into four practical groups.

First, it improves PC-Engine command execution. On PC-88VA v1.05,
child-process launches through `PCENGINE.COM` inherit the parent
attribute mode instead of resetting it to zero. `PCENGINE /C` can execute
internal and external commands; on PC-88VA2/3/-91 with PC-Engine v1.1, it
also supports batch command execution.

Second, it broadens executable compatibility. The documentation says it
allows compressed EXE files, self-extracting archives, and Turbo Pascal
v4-or-later EXE files to run under PC-Engine. The listed examples include
PKLITE, LZEXE, PKZIP, PKPAK, LHARC, and LHA generated executables.

Third, it patches PC-Engine command and DOS-service behavior. On
PC-88VA v1.05 it extends the internal `CLS` and `BASIC` commands:

```dos
CLS 1
CLS 2
CLS 3
BASIC /G
BASIC <file name> /G
```

`CLS 1` clears text, `CLS 2` clears graphics, and `CLS 3` clears both.
`BASIC /G` starts BASIC without clearing the graphics screen. PCEPAT also
fixes file deletion, parts of the `FOR` batch command, environment owner
path handling, and pieces of the directory/file creation APIs
corresponding to DOS functions `39h`, `3Ch`, `5Ah`, and `5Bh`.

Fourth, it fixes stability and register-preservation bugs. The
documentation calls out an interrupt-safety fix for a stack-pointer update
that could stall depending on interrupt timing; `SHELL`/`EXIT` cleanup
fixes for user traps and work areas; a BASIC startup stall when
`ADVGBIOS.SYS` is absent; MBIOS `INT 33h AH=00h` preserving `BX`, `CX`,
and `DX`; and CMBIOS `INT 8Ah AH=08h` preserving `DX`.

PCEPAT is therefore not just optional decoration. For an HDD-based
PC-Engine/MSE environment, it should be treated as the first compatibility
layer loaded before MSE.

## Bank Memory Manager

`BMS Driver` is the Bank Memory Specification driver used by some VA
software and by MSE's optional memory features. The archived Vector detail
page describes it as a bank-memory manager for PC-98x1/88VA, with support
for users of PCM8/WAV playback, AVE, the New JIS emulator, and RAM-disk
style uses.

The downloaded `bms15020.tgz` archive contains VA-side files including:

- `bmsdrva.com`: VA executable/resident driver.
- `bmsaddva.com`: VA non-device compatibility driver.
- `bmsgsva.com` and `bmsgsva.asm`: VA sample program and source.
- `bms15020.doc`, `bms15020.hed`, `bms15020.his`: documentation,
  archive header, and history.
- `bmsdrsys.wup`: WUP diff for the SYS driver forms.

The MSE documentation says its Alias and bank-memory swap features depend
on BMSDR. The development disk therefore enables `/A` and `/B` only after
loading BMSDRVA. The separate `/X` option additionally needs an XMS manager
and remains disabled. A minimal environment can omit BMS, but then these MSE
features should not be enabled.

VAEG defaults to 640KB of main RAM at `00000H`-`9FFFFH`. I/O Bank Memory uses
`80000H`-`9FFFFH` as a temporary 128KB aperture without reducing that main
memory: selector zero restores the ordinary upper 128KB, while selector values
1 through N map N independent 128KB BMS banks. The disabled device remains at
selector zero, so CPU and SGP access continues to reach main RAM. This
one-based mapping matches RDBMS, which selects a nonzero bank for each transfer
and writes zero afterward to restore conventional memory.

A clean VAEG configuration enables BMS with 128 banks (16MB) at the native
PC-88VA `01D0H` port. The corresponding persisted values are
`Use_BMS_=true`, `BMS_Port=01d0`, and `BMS_Size=128`; `BMS_Size` is a bank
count, not a megabyte count. The `00ECH` PC-9801-compatible port and smaller
capacities remain selectable. Explicit values in an existing `vaeg.cfg`,
including an off setting, are preserved rather than migrated.

When BMS is enabled, VAEG binds only the explicitly selected port. `01D0H`
remains the native and default configuration choice; selecting `00ECH` is the
explicit PC-9801-compatible mode. This keeps the menu selection authoritative
and avoids exposing an unintended second device address. The persisted
`BMS_Port` value remains the selected port shown by the configuration UI.

## EMS Board, EMMVA, and RDEMS

The [PC-88VA FAQ EMS article](http://www.pc88.gr.jp/vafaq/view.php/article/88va/vafaq/5)
distinguishes EMS from I/O Bank Memory and states that the two mechanisms can
coexist. EMS exposes expansion RAM through a page frame in the expansion-ROM
area. M90 connects the retained four 16KB windows at `C0000H`, `C4000H`,
`C8000H`, and `CC000H` to the active VA I/O dispatcher. In vaeg, select
`Device / EMS Board...` immediately below I/O Bank Memory and install memory
in 1MB units; zero disables the board and 1 through 13MB are supported.
Clean configurations use the full 13MB (`ExMemory=13`) by default. As with
BMS, an explicitly saved capacity or disabled value in an existing
configuration is preserved.

[EMMVA15A](http://www.pc88.gr.jp/softlib/?action=list_file&anum=2&gnum=351)
is an adapter, not a complete EMM manager. Its included `EMMVA150.DOC` says
version 1.5 uses `EMMVA01.SYS` and `EMMVA02.SYS` as a pair, adds the EMS
open-handle compatibility operation, and includes a version-up-board
dictionary-ROM collision workaround. A compatible EMM manager must be supplied
and loaded between the pair. The historical FAQ gives commercial EMM4J as
one example, but the M90 workflow does not use or distribute EMM4J.

M90 instead provides `SQEMM98.SYS`, a PC-88VA port of the open-source SQEMM
0.8 MAX driver. It is reproducibly built from pinned source with Open Watcom,
drives vaeg's `08E1H` through `08E9H` EMS interface directly, and detects the
configured 1 through 13MB capacity. Its startup, success, and error messages
are displayed by the PC-Engine Text BIOS `INT 83H/AH=02H` service. PC-Engine
does not provide SQEMM's DOS request-header command-tail contract, so this
port deliberately uses its validated defaults: 255 handles, all detected EMS
pages, the fixed `C000H` page frame, zero page offset, and the startup memory
test. The active stack is:

```dos
DEVICE = A:\SYS\EMMVA01.SYS
DEVICE = A:\SYS\SQEMM98.SYS
DEVICE = A:\SYS\EMMVA02.SYS
```

`SQEMM98.SYS` is the EMS manager created for this M90 workflow. `RDEMS.SYS`
is not that manager: it is the existing third-party RAM-disk driver from the
RDEMS152 package. The three EMMVA/SQEMM98 lines load first; RDEMS consumes
that service later, after TSCLVA.

[RDEMS152](http://www.pc88.gr.jp/softlib/?action=list_file&anum=2&gnum=270)
is a PC-88VA EMS RAM-disk driver and must be loaded after the EMM stack.
Its included manual requires EMM version 3.2 or later and documents 40 EMS
pages, or 640KB, as the default. The development disk explicitly requests
128 pages (2MB) instead:

```dos
DEVICE = A:\SYS\RDEMS.SYS -P128 -A
```

The development-disk builder keeps both original LZH archives and installs
`EMMVA01.SYS`, `SQEMM98.SYS`, `EMMVA02.SYS`, and `RDEMS.SYS` in `A:\SYS`.
It places the three-line manager stack at the start of root `CONFIG.SYS` and
RDEMS after TSCLVA, with the EMMVA/RDEMS manuals, SQEMM98 notes, and combined
licenses in `A:\DOC`. The source must be a verified PC-Engine 1.1 disk; the
builder retains
its IPL and fixed `ENGINEIO.SYS`, `PCENGINE.SYS`, `ADVGBIOS.SYS`, and
`PCENGINE.COM` placement, then installs the supplemental payload around those
files. The resulting D88 is therefore a PC-Engine 1.1 boot disk rather than a
data-only installation template. The earlier M90 four-driver supplemental disk
validated RDEMS read/write at 1MB and 13MB. That result predates the expanded
development-disk order and is not used as proof for BMSDRVA, TSCLVA, or RDPCM.

## Support Tools

The recipe also needs DOS utilities, which run through MSE:

- [LHA 2.55b](https://www.vector.co.jp/soft/dos/util/se002413.html) for LZH
  extraction. The builder applies Vector's official 2.55b BDF to the 2.55
  executable with BUPDATE.
- BDIFF/BUPDATE for BDF diffs.
- WSP for WUP diffs, including the MSE 3.52a-to-b patch.
- DIET 1.44 for executable compression on the generated development disk.
- [Memory Mapper for PC 1.3](https://www.vector.co.jp/soft/dos/hardware/se128128.html)
  as `X8MAP.COM`, for displaying SYSTEM/EMS/XMS/UMB/BMS and VA SMM usage.
- K-Launcher as a two-pane file manager.
- PMD and VA-specific generated PMD players for music playback tests.

The article explicitly warns that archive extraction is safer on the
PC-88VA side than on Windows. The practical reason is timestamp
preservation: some diff tools can reject inputs whose file dates changed
during host-side extraction.

## Suggested HDD Layout

A practical emulator-side HDD image should keep the boot and tool layers
simple:

```text
A:\
  CONFIG.SYS
  AUTOEXEC.BAT
  PCEPAT.SYS
  MSE352B.COM
  PCPLUS.SYS
  PCENGINE.COM

A:\BIN\
  BMSDRVA.COM
  BMSADDVA.COM
  LHA.EXE
  BUPDATE.EXE
  WSP.COM
  MSET.COM
  ALIAS.COM
  MSECUST.COM
  KLL.COM

A:\TMP\
```

`AUTOEXEC.BAT` should at least set a DOS-like tool path and a temporary
directory:

```dos
PATH A:\BIN
SET TMP=A:\TMP
```

K-Launcher extension execution may also need `COMSPEC` pointed at
PC-Engine, matching the TomoRetroPC note:

```dos
SET COMSPEC=A:\PCENGINE.COM
```

## Reproducible Boot-Floppy Builder

The repository provides two shell-script entry points. First,
[`tools/pc88va/create-vanilla-system-disk.sh`](../../tools/pc88va/create-vanilla-system-disk.sh)
creates a `FORMAT /S`-like PC-Engine 1.1 disk containing only the original IPL
and required `ENGINEIO.SYS`, `PCENGINE.SYS`, `ADVGBIOS.SYS`, and
`PCENGINE.COM`. Second,
[`tools/pc88va/build-utility-disk.sh`](../../tools/pc88va/build-utility-disk.sh)
creates that vanilla disk in a temporary directory and installs the development
environment on top of it.

Neither script contains, copies into Git, or identifies the private source
image. The common helper validates the public PC-Engine 1.1 filesystem layout
by system-file names, sizes, and starting clusters instead of recording the
source image's filename or checksum.

On Debian or Ubuntu, install the host-side build dependencies with:

```sh
sudo apt-get install curl lhasa dosbox nasm python3 coreutils tar unzip
```

Docker or Podman is also required to build `SQEMM98.SYS` with the pinned Open
Watcom environment. Select Podman by setting `CONTAINER_ENGINE=podman`.

To create only the vanilla system disk, run:

```sh
tools/pc88va/create-vanilla-system-disk.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pcengine-1.1-vanilla.d88
```

To create the complete development disk, run:

```sh
tools/pc88va/build-utility-disk.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pc88va-development.d88
```

The destination must not already exist. Public input archives are fetched and
verified in the shared `~/.cache/vaeg/auto-generated-pc88va-utility-media/`
directory. Every public input archive is pinned by SHA-256 in the script. An
existing cache file with different contents is rejected rather than replaced.

For maintainer-local preservation, verified copies of the public inputs used
for this image are stored under the Git-ignored
`docs/archives/pc88va-development-disk/` directory. This includes the LHA
2.55 executable and 2.55b patch, `X8MAP130.LZH`, `EMMVA15A.LZH`,
`RDEMS152.LZH`, `RDPCM001.LZH`, `RESET.ZIP`, `TSCLVA.ZIP`, `TSCLBDF.ZIP`,
`BMS15020.TGZ`, both `PCP108.LZH` and `PCP108P.LZH`, `SCF124.LZH`,
`V480SRC.LZH`, `JFPPAT.ZIP`, `2HCDRV.ZIP`, `FDFRMSRC.LZH`, `ISHARC.COM`,
`TENIM3.COM`, `TFD12.LZH`, and the pinned
SQEMM source archive.
These archive copies and the generated D88 remain outside Git.

The complete build performs these operations:

1. Create the minimal vanilla system disk while retaining the IPL and the
   original fixed system-file chains.
2. Fetch and verify PCEPAT, SCFORM, ISHARC, TENIM3, TFD12, JFPPAT, 2HCDRV,
   FDFRMSRC, and V480,
   together with BMS Driver 1.50 Rev 0.20, PCPLUS 1.08 and its
   group 2-451 bug-fix patch, SCHD 1.55t, RDBMS 1.21, RDPCM 0.01, TSCLVA
   Rev.50703 and its Rev.51127 update, RESET Rev.51028, BDIFF/BUPDATE 1.28,
   MSE 3.52a and the 3.52b patch, WSP 1.50, LHA 2.55 and its official 2.55b
   patch, DIET 1.44, Memory Mapper for PC 1.3, EMMVA 1.5a, RDEMS 1.52,
   K-Launcher 1.30, TEEN 0.30p, VBUFF 1.02, FATMAP 1.1, FORG 2.03, the VA
   RAMDISK self-extracting archive, and the GNU File Utilities 3.12 MS-DOS
   rev B executable archive.
3. Extract the packages with the host `lha`, `tar`, and `unzip` commands.
4. Run the original DOS `WSP.COM` and `BUPDATE.EXE` under headless DOSBox to
   produce LHA 2.55b, `MSE352B.COM`, the patched `PCPLUS.SYS`, the generated
   `BMSDRVA.SYS`, the updated TSCLVA Rev.51127 files, and the PC-88VA
   K-Launcher files `KLL.COM`, `KLVA.EXE`, and `KLCUST.EXE`.
5. Assemble and validate the repository's clean-room `HOSTFAT.SYS` with NASM.
6. Run `tools/pc88va/stage-development-tools.sh` to create the normalized
   common-tool tree and its SHA-256 manifest.  The FDD builder selects the
   compact profile from that tree; the SASI builder selects the full profile.
   Both backends therefore use the same extraction and destination rules.
7. Extract the ISHARC self-extracting package and the TFD archive, then stage
   their tools, driver, and documentation. TENIM3 remains as its intact
   self-extracting archive because its expanded sample assets do not fit in
   the fixed FAT12 development disk.
8. Assemble and link `VIEW480.ASM` from `V480SRC.LZH` with the pinned
   Open Watcom image, then install the resulting `VIEW480.COM`.
9. Build and validate `SQEMM98.SYS` from pinned source with Open Watcom, then
   install the complete EMMVA/SQEMM98/RDEMS driver stack.
10. Verify the generated files against known public-package checksums.
11. Extract the RAMDISK self-extracting archive and stage its driver, helper
   commands, and documentation separately.
12. Run every `.EXE` and `.COM` under `BIN` through DIET 1.44. COM files retain
   their COM form; files for which DIET cannot reduce byte size remain
   unchanged.
13. Add the `SYS` drivers, compressed `BIN` utilities, source archives,
    documentation, and an empty `TMP` directory to the vanilla FAT12
    filesystem.

The PC-Engine disk has a valid FAT12 allocation structure but no conventional
DOS BPB, so normal `mtools` commands reject it as non-DOS media. The builder
therefore contains a narrowly scoped D88/FAT12 writer for the known 80-cylinder,
two-head, eight-sector, 1024-byte PC-Engine 1.1 layout. It never relocates the
existing `ENGINEIO.SYS` or `PCENGINE.SYS` boot chains. The vanilla builder
clears all unreferenced data clusters, and new directory entries use a fixed
DOS date, so repeated builds from the same source are byte-for-byte
reproducible. The builder's final file count, payload size, and free-space
summary include the SCFORM, VIEW480, ISHARC, TENIM3, TFD12, JFPPAT, 2HCDRV,
and formatter-source additions described below.

The development disk is organized as follows. `KLVA.EXE`, `KLCUST.EXE`,
`KL.CFG`, and `KLJPN.HLP` are also kept in `BIN` because `KLL.COM` needs the
VA-specific executable and configuration files.

```text
A:\
  CONFIG.SYS
  AUTOEXEC.BAT
  PCENGINE.COM

A:\SYS\
  EMMVA01.SYS
  SQEMM98.SYS
  EMMVA02.SYS
  PCPLUS.SYS
  BMSDRVA.SYS
  SCHD.SYS
  HOSTFAT.SYS
  PCEPAT.SYS
  JFPPAT.SYS
  TFD.SYS
  RESET.SYS
  TSCLVA.SYS
  MSE352B.COM
  RAMDISK.SYS
  RDPCM.SYS
  RDBMS.SYS
  RDEMS.SYS

A:\BIN\
  BIOSFREE.COM
  BMSDRVA.COM
  BMSADDVA.COM
  LHA.EXE
  DIET.EXE
  BUPDATE.EXE
  WSP.COM
  MSET.COM
  ALIAS.COM
  MSECUST.COM
  SMSTAT.COM
  SETDMA.COM
  X8MAP.COM
  KLL.COM
  KLVA.EXE
  KLCUST.EXE
  KL.CFG
  KLJPN.HLP
  MSE350.DEF
  TEEN.COM
  TEENM.COM
  TEEN.DEF
  TOPEN.EXE
  TCLOSE.EXE
  TLOG.COM
  TLOGBMS.COM
  VBUFF.COM
  SCFORM.COM
  VIEW480.COM
  2HCDRV.COM
  FDFORM.COM
  ISHVA.COM
  PKPAK.EXE
  PKUNPAK.EXE
  FATMAP.EXE
  FATMAP_E.COM
  FORG.EXE
  FORG.DAT
  SETID.COM
  SETIPL.COM
  CHMOD.EXE
  COPYING
  CP.EXE
  DD.EXE
  DF.EXE
  DI.EXE
  DU.EXE
  INSTALL.EXE
  LS.EXE
  MKD.EXE
  MV.EXE
  RM.EXE
  RMD.EXE
  TOUCH.EXE
  VDIR.EXE

A:\DOC\
  BMS15020.DOC
  BMS15020.HED
  BMS15020.HIS
  DIET144.DOC
  DIETREAD.DOC
  TEEN.DOC
  TEENUPDT.DOC
  TEENREAD.DOC
  TLOG.DOC
  VBUFF.DOC
  VBUFF.LOG
  SCFORM.DOC
  SCFORM.LOG
  JFPPAT.DOC
  2HCDRV.DOC
  FDFORM.DOC
  ISHARC.DOC
  ISHVA.DOC
  TENIM3.DOC
  TENMAIL.DOC
  TFD12.DOC
  TFD12.MAN
  FATMAP.MAN
  FATMREAD.DOC
  FORG.DOC
  FORGREAD.DOC
  RAMDISK.DOC
  RAMREAD.ME
  RDPCM.DOC
  RESET.DOC
  SCHD.DOC
  SCHD.LOG
  SCHD.TXT
  RDBMS.DOC
  PCPLUS.DOC
  PCPLUS.TXT
  SCSI55.TXT
  EMMVA150.DOC
  RDEMS152.MAN
  TSCLVA.DOC
  SQEMM.LIC
  SQEMM98.TXT
  X8MAP130.SMP
  X8MAP130.TXT

A:\ARCHIVE\
  V480SRC.LZH
  JFPPAT.ZIP
  2HCDRV.ZIP
  FDFRMSRC.LZH
  ISHARC.COM
  TENIM3.COM
  TFD12.LZH

A:\TMP\
```

The three archive `README.DOC` files are renamed to `TEENREAD.DOC`,
`FATMREAD.DOC`, and `FORGREAD.DOC` to avoid collisions in the flat `DOC`
directory. `RAMDISK.COM` is an LHA-compatible self-extracting archive rather
than the driver itself. The builder expands it on the host and installs
`RAMDISK.SYS` in `SYS`, `BIOSFREE.COM`, `SETID.COM`, and `SETIPL.COM` in
`BIN`, and its `RAMDISK.DOC` and `README` as `RAMDISK.DOC` and `RAMREAD.ME`
in `DOC`. The self-extracting archive is not copied onto the D88.

Every `.EXE` and `.COM` in `BIN`, including the two BMS utilities, is passed
through DIET 1.44 with byte-size comparison enabled. `-XC` keeps compressed
COM files in COM form. DIET leaves `BIOSFREE.COM`, `BMSADDVA.COM`, `DIET.EXE`,
`KLL.COM`, `SETDMA.COM`, `SETID.COM`, and `VBUFF.COM` unchanged because
compression would not reduce their size; the remaining reducible executables
are stored in DIET form. This includes reducing `SMSTAT.COM` from 1,944 to
1,725 bytes, `X8MAP.COM` from 10,373 to 7,203 bytes, and the retained
interactive `BMSDRVA.COM` from 24,812 to 4,351 bytes. The generated
`BMSDRVA.SYS` is a device driver under `SYS`, not a BIN executable, and is not
passed through DIET.
`DIET.EXE` and its primary documentation are included so these files can be
inspected or restored in the guest.

The GNUish catalog identifies `fut312bx.zip` as the executable distribution of
GNU File Utilities 3.12 for DOS. The builder extracts all 15 entries from its
`BIN` directory, including the GPL `COPYING` file, into `A:\BIN`. Its formatted
manuals are not installed. Since the resulting `BIN` directory has more than
30 ordinary entries, the FAT12 writer allocates and links multiple directory
clusters.

The hidden/system `ENGINEIO.SYS`, `PCENGINE.SYS`, and `ADVGBIOS.SYS` files
remain in the root as required for boot. `CONFIG.SYS` is:

```dos
FILES   = 20
BUFFERS = 30
DEVICE = A:\SYS\EMMVA01.SYS
DEVICE = A:\SYS\SQEMM98.SYS
DEVICE = A:\SYS\EMMVA02.SYS
DEVICE = A:\SYS\PCPLUS.SYS
DEVICE = A:\SYS\BMSDRVA.SYS /P
DEVICE = A:\SYS\SCHD.SYS -I0
DEVICE = A:\SYS\HOSTFAT.SYS
DEVICE = A:\SYS\PCEPAT.SYS
DEVICE = A:\SYS\JFPPAT.SYS
DEVICE = A:\SYS\RESET.SYS
DEVICE = A:\SYS\TSCLVA.SYS
DEVICE = A:\SYS\MSE352B.COM /A /B
DEVICE = A:\SYS\RDBMS.SYS -P1D0 -S2
DEVICE = A:\SYS\RDEMS.SYS -P128 -A
DEVICE = A:\SYS\RDPCM.SYS
```

The EMMVA/SQEMM98 manager stack loads first. PCPLUS follows it, then the
WSP-generated BMSDRVA device-driver form. PCPLUS still precedes the target-zero
SCHD block driver. HOSTFAT is available when vaeg has a read-only host folder
configured. RESET loads immediately after its required PCEPAT dependency;
PCEPAT, JFPPAT, RESET, and TSCLVA all precede MSE. JFPPAT is the optional
PC-Engine patch layer and is loaded immediately after PCEPAT. MSE uses `/A` for BMS-backed Alias
data and `/B` for partial BMS code-data swap; `/X` remains omitted because no
XMS manager is installed.
RDBMS explicitly selects the PC-88VA I/O Bank Memory port `01D0H`. Its
documented defaults start at bank 1 and use 15 banks when `-S` and the bank
count are omitted. The development disk passes `-S2` so bank 1 remains
reserved for MSE's `/A /B` code and Alias swap, while RDBMS uses banks 2
through 16. With VAEG's default 640KB main RAM, selector zero restores
the ordinary `80000H`-`9FFFFH` upper 128KB after every RDBMS transfer, while
the RAM disk occupies BMS selectors starting at 1.
RDBMS 1.21 also documents that an original VA may not receive `CONFIG.SYS`
parameters. Its distributed driver has `00ECH` embedded at file offset
`001AH`, so `-P1D0` alone is insufficient on that model. The disk builder
verifies the original extracted driver SHA-256
`7ead949be781303f12c3fc1bf499de3d59a504acea69747d90e21bb4109d5d49`,
checks the `EC 00` word, and changes only the generated disk copy to `D0 01`.
The resulting `RDBMS.SYS` SHA-256 is
`8a4e09f9f2b1b1363a3d07a1edeb36ae744665324a7de9a1c628e6480a5f0289`.
The archived `rdbms121.lzh` remains byte-for-byte original, and the explicit
`-P1D0` remains in `CONFIG.SYS` for models that do pass parameters.
VAEG binds only the selected BMS port, so software using the fixed driver's
compiled `00ECH` default must select `BMS_Port=00ec`; native-VA software uses
the default `BMS_Port=01d0`. The development disk and its BMS driver remain
unchanged.
The EMMVA adapter pair encloses the Open Watcom-built SQEMM98 manager. RDEMS
loads after TSCLVA and allocates a 128-page (2MB) EMS RAM disk. The BMS VA
device driver is resident, while its COM form remains available for management.
RDPCM loads last and claims the Sound Board II PCM RAM as another RAM disk;
remove that line when running software that uses FM or ADPCM audio.
`AUTOEXEC.BAT` uses neither `ECHO OFF` nor `PROMPT`; it only establishes the
requested tool environment:

```dos
PATH A:\BIN
SET TEEN=A:\BIN\TEEN.DEF
SET TMP=A:\TMP
SET COMSPEC=A:\PCENGINE.COM
```

The `TEEN` variable follows TEEN's documentation and points the network stack
at its configuration file. TEEN, TLOG, VBUFF, FORG, and RAMDISK are not run
automatically. In particular, FORG modifies FAT allocation and should only be
used after making a backup as directed by its documentation.

The resulting disk is intended for PC-Engine 1.1 on a PC-88VA2/VA3 or the
corresponding upgraded VA environment. The script proves structural
bootability by retaining the original IPL and fixed system-file placement. A
bounded normal-speed VA2 boot of the reordered 98-file image at the default
1MB EMS setting reached PC-Engine `Ready` and left its disposable D88 copy
unchanged. The automated `DIR` text injection reached the guest as only `R`;
the immediately preceding development disk produced the same control result.
The run therefore establishes boot completion without attributing that input
limitation to this hotfix, but it is not a pass for BMSDRVA, TSCLVA, RDEMS,
RDPCM, RESET-key execution, MSE utility execution, or K-Launcher. Those remain
PC-88VA/vaeg human checks.

## 40 MB SASI Development HDI Builder

For a hard-disk development environment, use
[`tools/pc88va/build-sasi-utility-disks.sh`](../../tools/pc88va/build-sasi-utility-disks.sh).
It creates two separate 40 MB SASI HDIs with the same development payload:
the VA image keeps the PC-Engine 1.05 boot files, and the VA2 image keeps the
PC-Engine 1.1 boot files. The source D88 images are read-only inputs and are
never modified. The payload D88 is transplanted with its `ARCHIVE`, `BIN`,
`DOC`, `SYS`, and `TMP` directories into both HDIs. The SASI wrapper also
stages the verified LSI-C, ISH/PKPAK, 16-bit Info-ZIP, EMACSVA, CPMVA, TDC,
and BENCH packages, so an older payload D88 cannot silently omit those tools.
The SASI builder deliberately regenerates `CONFIG.SYS` and `AUTOEXEC.BAT` so
the documented VA load order is identical on both variants.
COM/EXE utilities found on each matching PC-Engine source D88 are also added
to that variant's `A:\BIN`; the boot `PCENGINE.COM` remains at the root.
The package extraction is shared with the FDD builder through
[`tools/pc88va/stage-development-tools.sh`](../../tools/pc88va/stage-development-tools.sh).
It emits a normalized tree plus an optional tab-separated manifest containing
the selected profile, relative 8.3 path, SHA-256, and byte count.  The SASI
injector validates that manifest before merging the tree, while the FDD
injector verifies every compact-profile source path before copying it.  The
FDD profile intentionally omits the full Info-ZIP and UNIX-like collection
because the fixed FAT12 medium cannot hold it and because those commands
overlap historical `BIN` names.  The SASI profile includes the complete
Vector UNIX-like tools 4.12h package under `A:\UNIX\BIN`, manuals under
`A:\UNIX\MAN` and `A:\UNIX\DOC`, and the original `UXTL412H.TGZ` under
`A:\ARCHIVE`; `AUTOEXEC.BAT` appends `A:\UNIX\BIN` after the existing paths.
The package is the free DOS/Windows 95/98 UNIX-like tools distribution from
[Vector's se010511 page](https://www.vector.co.jp/soft/win95/util/se010511.html),
downloaded as `uxtl412h.tgz` (SHA-256
`dfc2b671cdbf7287cd845cf8166317ad0ecb13e720e50f8da858d3292316396f`).

The SASI-only source and library set also includes Softlib groups 2-400,
2-306, 2-331, 2-403, 2-436, and 2-382.  The builder retains the verified
archives as `2HCDRSRC.LZH`, `2HCDRV.ZIP`, `PCPATSRC.ZIP`, `TSCLVSRC.LZH`,
`S88VALSI.LZH`, and `S88VA250.LZH` under `A:\ARCHIVE` without expansion.
The separate package notes and the 2HCDRV/FDFORM documentation are in
`A:\DOC`; the runnable `2HCDRV.COM` and `FDFORM.COM` are installed in
`A:\BIN`.  `2HCDRV.COM` is not loaded by the generated `CONFIG.SYS` because
its resident startup path can reset VAEG; it remains available for explicit
manual testing.  These source/library archives are intentionally not passed
to the full FDD profile.

The SASI image also uses the maintained STEST 1.15 package from the
[OSL driver archive](https://www2u.biglobe.ne.jp/~pumpkin/hlabo/osl/driver/STEST115.LZH)
and keeps its companion source archive
[`ST115SRC.LZH`](https://www2u.biglobe.ne.jp/~pumpkin/hlabo/osl/driver/ST115SRC.LZH)
under `A:\ARCHIVE`.  `STEST115.LZH` is expanded to `A:\BIN\STEST.EXE`,
`A:\BIN\STESTX.COM`, and `A:\BIN\STEST.BAT`, with its manuals under
`A:\DOC`; `ST115SRC.LZH` remains a source archive and is not added to FDD.

The command-line wrapper accepts explicit paths, which is useful because the
system and development D88 images are normally kept outside Git:

```sh
tools/pc88va/build-sasi-utility-disks.sh \
  --source-va "/path/to/PC-Engine 1.05.d88" \
  --source-va2 "/path/to/PC-Engine 1.1.d88" \
  --payload-d88 "/path/to/pc88va-development.d88" \
  --lsic-archive "/path/to/LSIC330C.LZH" \
  --jwasm-archive "/path/to/JWasm_v220_dos.zip" \
  --output-dir /private/tmp/pc88va-sasi
```

The result is:

```text
/private/tmp/pc88va-sasi/pc88va-sasi-40mb-va.hdi
/private/tmp/pc88va-sasi/pc88va-sasi-40mb-va2.hdi
```

The output directory is created if necessary, but an existing output file is
never overwritten. To build one variant directly, use
`build-sasi-utility-disk.py` with `--variant va` or `--variant va2` and
the same `--source`, `--payload-d88`, and `--output` options. Without
`--payload-d88`, the direct builder intentionally creates only the small
system-plus-`BIN` layout used for layout tests; it is not the complete
development environment. Both builders accept the verified
`LSIC330C.LZH` archive and an extracted tree; the wrapper extracts the archive
automatically and installs the archive at `A:\ARCHIVE\LSIC330C.LZH` and the
complete compiler tree below `A:\LSIC86`. This preserves the original `BIN`,
`INCLUDE`, `LIB`, `MAN`, and `SRC` layout so the compiler tools can be run
under MSE. The original `_LCC` configuration expects this `A:\LSIC86` root.
`AUTOEXEC.BAT` adds `A:\LSIC86\BIN` to `PATH` and sets `LSIC86`,
`INCLUDE`, and `LIB`. The default archive is read from the shared utility-media
cache; `--lsic-archive` selects an explicit copy. Extracted files are staged
in a temporary host directory and are not tracked.

`JWasm_v220_dos.zip` is fetched automatically into the local verified cache
unless `--jwasm-archive` selects an explicit copy. The wrapper passes the
verified archive to both VA and VA2 builders, so each resulting HDI contains
the same `JWASMR.EXE` tool under `A:\BIN`. The wrapper likewise
verifies and stages `ISHARC.COM`, `UNZ532X3.EXE`, `ZIP22X.ZIP`,
`EMACSVA.LZH`, `CPMVA.LZH`, `TDC10.LZH`, and `BENCH003.LZH`; runnable files
are placed in `A:\BIN`, manuals in `A:\DOC`, and original archives in
`A:\ARCHIVE`.

The generated HDI has the HDFORM-compatible 40 MB geometry (4096-byte header,
256-byte SASI blocks, 33 sectors, 8 surfaces, and 615 cylinders). The builder
validates the fixed PC-Engine IPL/system-file placement, writes matching FAT
copies, and preserves exact logical file contents while allocating new HDI
clusters. The generated configuration is:

```dos
FILES   = 20
BUFFERS = 30
DEVICE = A:\SYS\EMMVA01.SYS
DEVICE = A:\SYS\SQEMM98.SYS
DEVICE = A:\SYS\EMMVA02.SYS
DEVICE = A:\SYS\PCPLUS.SYS
DEVICE = A:\SYS\BMSDRVA.SYS /P
DEVICE = A:\SYS\SCHD.SYS -I0
DEVICE = A:\SYS\HOSTFAT.SYS
DEVICE = A:\SYS\PCEPAT.SYS
DEVICE = A:\SYS\JFPPAT.SYS
DEVICE = A:\SYS\RESET.SYS
DEVICE = A:\SYS\TSCLVA.SYS
DEVICE = A:\SYS\MSE352B.COM /A /B
DEVICE = A:\SYS\RDBMS.SYS -P1D0 -S2
DEVICE = A:\SYS\RDEMS.SYS -P128 -A
DEVICE = A:\SYS\RDPCM.SYS
```

`AUTOEXEC.BAT` sets `PATH A:\BIN;A:\LSIC86\BIN`, `LSIC86`, `INCLUDE`, `LIB`,
`TEEN`, `TMP`, and `COMSPEC` for the PC-Engine command environment. `TFD.SYS`
remains installed in `SYS` but is not loaded by default, as required by its
documentation; it can be enabled manually after EMS setup on real hardware.

The same builder can install the [CP/M program EXEcutor 0.8
(`cpm08.zip`)](https://www.vector.co.jp/soft/dl/dos/util/se000015.html) and the
preserved CP/M tools/source disks (and, when supplied, the development disk).
The verified archive is 29,761 bytes with SHA-256
`691e51dda202ab97b7c8c947ca7c9bf2d93d822f3e315362fcc7840199b8d6f7`; it is a
local input and is not committed. The wrapper accepts the archive and the
CP/M D88 inputs (`cpmva-tools.d88` and `cpmva-source.d88`, plus optional
`cpmva-dev.d88`) through `--cpm-archive`, `--cpm-tools-d88`,
`--cpm-source-d88`, and `--cpm-dev-d88`. Their files are expanded below
`A:\CPM\TOOLS`, `A:\CPM\SOURCE`, and, when supplied, `A:\CPM\DEV`; the
emulator and its `XCCP.CPM` support file are under
`A:\CPM\BIN`, with sources and documentation under `A:\CPM\SRC` and
`A:\CPM\DOC`. When enabled, `AUTOEXEC.BAT` adds `A:\CPM\BIN` to `PATH` and
sets `CPM=A:\CPM`. The CP/M D88 reader accepts both the current EXM=1 layout
and the historical one-16-KiB-extent layout used by the preserved source disk.

The builder itself is host-side and does not require DOSBox. For a structural
and VAEG mount smoke check after generation, run both models with the matching
HDI (using a local ROM-less VAEG build):

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-debug/sdl2/vaeg --model va \
  --sasi1 /private/tmp/pc88va-sasi/pc88va-sasi-40mb-va.hdi \
  --no-cfg --no-bkupmem --smoke --mute

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-debug/sdl2/vaeg --model va2 \
  --sasi1 /private/tmp/pc88va-sasi/pc88va-sasi-40mb-va2.hdi \
  --no-cfg --no-bkupmem --smoke --mute
```

These checks establish HDI geometry, header, FAT, and mount behavior. They do
not replace a real PC-88VA/VA2 boot and driver-operation check.

## Supplemental Softlib Archive Disk

[`tools/pc88va/build-softlib-archive-disk.sh`](../../tools/pc88va/build-softlib-archive-disk.sh)
creates a separate bootable PC-Engine 1.1 D88 containing additional PC-88VA
software archives. It validates `pcengine110-bootonly.d88`, retains its IPL
and four fixed system-file chains, clears every other FAT/root/data entry,
and installs the supplemental payload in the remaining space. The source D88
is opened read-only and the output must be a new path.
The FDD payload is defined by
[`tools/pc88va/softlib-fdd-manifest.tsv`](../../tools/pc88va/softlib-fdd-manifest.tsv),
which mirrors the historical `pc88va-softlib-archives.d88` file set.

Run it with:

```sh
tools/pc88va/build-softlib-archive-disk.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pc88va-softlib-archives.d88
```

The output must not already exist. The script uses the shared utility-media
cache, pins every public file by SHA-256, rejects mismatched cache entries,
and validates the manifest byte counts before installation. Drivers and the
larger extracted tool collections are installed by the SASI development
builder, not duplicated on this FDD.

The requested Softlib groups and files are:

| Group | Files stored verbatim |
|-------|-----------------------|
| [2-452](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=452) | `VBUFF102.LZH` |
| [2-390](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=390) | `ALGO_VA.DOC`, `ALGO_VA.LZH` |
| [2-400](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=400) | `2HCDRSRC.LZH` |
| [2-435](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=435) | `EMACSVA.LZH`, `EMACSVA.DOC` |
| [2-424](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=424) | `CPMVA.LZH` |
| [2-401](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=401) | `FDFRMSRC.LZH` |
| [2-396](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=396) | `RDPCM001.LZH`, `RDPCM001.DOC` |
| [2-306](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=306) | `2HCDRV.ZIP` |
| [2-351](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=351) | `EMMVA15A.LZH` |
| [2-307](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=307) | `JFPPAT.ZIP` |
| [2-270](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=270) | `RDEMS152.LZH` |
| [2-201](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=201) | `TDC10.LZH` |
| [2-389](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=389) | `BENCH003.DOC`, `BENCH003.LZH` |

The EMMVA and RDEMS archives are retained verbatim; their extracted drivers
remain SASI-only.

Group 2-306 appeared twice in the requested URL list and is intentionally
stored once. The disk also contains the 409,884-byte `LSIC330C.LZH` archive
from the [LSI C-86 3.30c trial-version page](https://www.vector.co.jp/soft/maker/lsi/se001169.html).
The complete [PRJ_PLUS repository](https://github.com/mazone-ma3/PRJ_PLUS) is
too large for this floppy. The builder instead pins commit
[`ed4036bf70a8e03d926d0b8a943208e909810f2a`](https://github.com/mazone-ma3/PRJ_PLUS/commit/ed4036bf70a8e03d926d0b8a943208e909810f2a),
downloads its root `LICENSE` and
`README.md` plus the seven-file `PC88VA` selection, and packages them as the
113,062-byte `PRJVA.ZIP`. The ZIP uses stored entries, fixed metadata, and a
commit-identifying archive comment so its bytes do not depend on a host zlib
version. [`tools/pc88va/create-stored-zip.py`](../../tools/pc88va/create-stored-zip.py)
performs this packaging, and the generated archive is also checksum-pinned.

The disk also provides the free 16-bit DOS executables from
the GNUish DOS-only distributions of
[Info-ZIP UnZip 5.32 and Zip 2.2](https://www.ibiblio.org/pub/micro/pc-stuff/freedos/mirrors/gnuish/dos_only/).
The [GNUish collection](https://www.math.utah.edu/docs/info/gnuish_6.html)
was organized for small 8088- and 80286-based DOS systems. Its distributions
contain both 16-bit and 32-bit programs; the builder deliberately installs
`unzip.exe` and `zip.exe`, not `unzip32.exe` or `zip32.exe`. It also installs
their copying terms and primary manuals. The original `UNZ532X3.EXE` and
`ZIP22X.ZIP` distributions remain in the verified host cache but are not
duplicated on the D88.

The supplemental payload contains 26 files totaling 895,144 bytes. Together
with the four retained PC-Engine system files, the generated bootable disk
contains 30 files. The files are organized as follows:

```text
A:\
  ENGINEIO.SYS
  PCENGINE.SYS
  ADVGBIOS.SYS
  PCENGINE.COM
  ARCHIVE\
    2HCDRSRC.LZH
    2HCDRV.ZIP
    ALGO_VA.DOC
    ALGO_VA.LZH
    BENCH003.DOC
    BENCH003.LZH
    CPMVA.LZH
    EMACSVA.DOC
    EMACSVA.LZH
    EMMVA15A.LZH
    FDFRMSRC.LZH
    JFPPAT.ZIP
    LSIC330C.LZH
    PRJVA.ZIP
    RDEMS152.LZH
    RDPCM001.DOC
    RDPCM001.LZH
    TDC10.LZH
    VBUFF102.LZH
  BIN\
    UNZIP.EXE
    ZIP.EXE

  DOC\
    COPYING
    UNZDOS.TXT
    UNZIP.DOC
    ZIP.DOC
    ZIPREAD.TXT

```

The resulting disk has 304,128 bytes (297 KiB) free with the PC-Engine 1.1
source system image. Two builds from the same
source and verified cache are byte-for-byte identical. A headless DOSBox check
confirmed that the included 16-bit Info-ZIP executables can test and extract
`PRJVA.ZIP` and create a valid ZIP archive.

## vaeg Implications

This recipe is useful for guest-side development and validation, but it is
not a host-file-transfer solution by itself. MSE lets DOS tools run inside
the PC-88VA environment; it does not make a host directory visible to the
guest.

For vaeg, the practical workflow target is:

1. Create or obtain a SASI-compatible HDD image.
2. Boot a PC-88VA DOS environment and install system files to the HDD.
3. Install PCEPAT, BMS Driver if MSE memory features are needed, MSE
   3.52b, PCPLUS, and the DOS utility set inside the HDD image.
4. Use the HDD image as the stable guest development environment.
5. Prefer guest-side archive extraction and diff application when file
   timestamps matter.

The longer-term ergonomic improvement is still a dedicated file-exchange
path, either through a restored host-drive bridge or through explicit HDD
image management tools. The PCEPAT/MSE setup complements that work but
does not replace it.

The SDL2 GUI parity list currently keeps HardDisk open/remove support as a
later item. When that lands, this document is the expected software
environment to test against.

## Distribution Caution

Do not vendor these third-party archives or generated binaries into the
vaeg repository. Keep them as user-supplied software, documented by URL
and checksum if a release workflow ever needs reproducible external
inputs. The generated development D88 is also a private build artifact
containing third-party software and must never be staged or committed.
