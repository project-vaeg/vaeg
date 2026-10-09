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
# PC-88VA SCSI Support Disk

Language: [English](scsi-support.md) | [日本語](scsi-support.ja.md)

This document is an independently written summary of the PC-88VA SCSI setup
described by the original software documentation and the PC88.gr.jp forum.
It does not reproduce the forum post or package manuals. The repository does
not contain or redistribute PC-Engine, PCPLUS, SCHD, VBUFF, or SCFORM
binaries.

## Preservation Scope

VAEG models the PC-9801-55-compatible SCSI control and data interface needed
by the PCPLUS/SCHD software path. The board firmware ROM is deliberately
disconnected by default. This follows `SCSI55.TXT`, which states that the
board ROM may be disconnected without affecting operation on a PC-88VA.
VAEG therefore does not claim that the board ROM window is part of the VA
guest memory map.

The purpose is preservation: record the public package locations and exact
download identities, retain an independently written English setup procedure,
and provide a reproducible way to assemble the software while the public
downloads or a verified local cache remain available. A generated disk also
keeps the original package manuals together for use with real PC-88VA
hardware or a future SCSI-capable implementation.

## MO support status

VAEG does **not** support magneto-optical (MO) or other removable SCSI media.
The SCHD, VA128MO, and STEST packages may be retained on a development disk
as historical reference material, but VAEG's supported SCSI path is for fixed
disks only. The `mo-128mb` and `mo-160mb` builder profiles stage reference
files; they do not make MO media usable in VAEG and are not a support claim.
The physical-MO commands below apply only to separately validated hardware,
outside the VAEG emulator.

## Current VAEG setup (Rel.260805)

The following is the supported user workflow for the current VAEG release.
The historical register and package notes below are retained as preservation
evidence; they are not required to attach a disk in VAEG.

### 1. Prepare the support disk

VAEG does not redistribute PC-Engine ROMs, `PCPLUS.SYS`, `SCHD.SYS`,
`SCFORM.COM`, or other proprietary guest files. Start with a PC-Engine 1.1
system D88 obtained from a lawful source and build a disposable SCSI support
disk:

```sh
sudo apt-get install curl dosbox lhasa python3 coreutils

tools/pc88va/scsi-support.sh \
  --source /path/to/pcengine-1.1.d88 \
  --output /path/to/pcengine-scsi.d88
```

The command downloads or reuses the checksum-verified support packages,
patches PCPLUS as required by the documented workflow, and writes a new D88.
It never modifies the source D88. Use `--scsi-id 0` through `--scsi-id 7` to
choose the default target ID written to `CONFIG.SYS`.

For two SCSI targets, add both driver lines to the generated disk's
`CONFIG.SYS` before booting it:

```dos
DEVICE = A:\PCPLUS.SYS
DEVICE = A:\SCHD.SYS -I0
DEVICE = A:\SCHD.SYS -I1
```

`-I0` and `-I1` select SCSI target IDs 0 and 1. The supported target-ID
range is 0 through 7; the driver line must exist for each target that should
be registered. Keep a backup before running `SCFORM`, because formatting
initializes the selected target.

### 2. Create and attach a VAEG SCSI image

The GUI creates VHD-format SCSI images. Choose **HardDisk -> New SCSI
image...**, select a size, and assign it to **SCSI ID 0** through **SCSI ID 6**.
The usual first two targets are SCSI ID 0 and SCSI ID 1. Existing
images can be attached through the corresponding **Open** menu item. Reset
the guest after changing an attachment so PCPLUS/SCHD re-enumerates it.

The same operation can be prepared from a shell:

```sh
# Existing images; options use the actual SCSI target IDs 0 through 6.
./vaeg --scsi0 /path/to/scsi-id0.hdi \
       --scsi1 /path/to/scsi-id1.hdi \
       --scsi2 none --scsi3 none \
       --scsi4 none --scsi5 none --scsi6 none
```

From a VAEG source checkout, a blank image can also be created with:

```sh
python3 tools/create_vaeg_scsi_hdd.py \
  --output /path/to/scsi-id0.hdd --size-mib 40 --executable ./vaeg
```

The image creator refuses to overwrite an existing file. SCSI images use the
`.hdd` extension and 256-byte physical blocks; SASI images are a separate
`.hdi` format and must be created through the SASI workflow.

### 3. Format a fixed-disk target in the guest

Boot the generated support D88 as the floppy drive. Confirm that PCPLUS is
loaded before SCHD and that each intended SCSI ID receives a drive letter.
Run `SCFORM` only against a disposable or backed-up target:

```dos
SCFORM
```

For a partition requiring 2048-byte logical sectors, first update the boot
disk buffer and reboot:

```dos
VBUFF -D1 -B11
SCFORM /S
```

`VBUFF -D1` selects drive A. The `/S` option belongs to SCFORM and exposes
its logical-sector-size choices; it is unrelated to SCHD's `-S` geometry
override. Leave the generated floppy as the boot medium: this workflow does
not make the SCSI target bootable.

After formatting, reboot once and check the new drive with `DIR`. Create a
small test file, read it back, delete it, and reboot again before putting
important data on the image. The VAEG automated G75 checks cover this
lifecycle for one target and for target IDs 0 and 1.

### 4. Troubleshooting

- A second disk appearing in `SCFORM` but not in DOS normally means that its
  `SCHD.SYS -I<n>` line is missing from `CONFIG.SYS`.
- Reset after adding, removing, or formatting an image. A reset rebuilds the
  guest SCSI/SxSI registration path.
- Use the exact SCSI ID configured in the `-I` option. VAEG does not silently
  remap an incorrect initiator ID.
- Keep the support D88 and ROMs outside the VAEG release archive. The support
  disk builder and the release executable are separate from those guest
  assets.

MO media is outside this workflow. Do not interpret the retained MO packages
or the `mo-*` builder profiles as emulator support.

## Sources and Software

The primary setup note is the PC88.gr.jp forum topic
[Connecting a SCSI hard disk to the PC-88VA][forum-501]. The required public
packages are:

- [PCPLUS 1.08][pcplus], which supplies the PC-9801-55-compatible SCSI BIOS
  service (`$SCSIBIOS`) used by the other software. `PCPLUS.SYS` is the
  software SCSI BIOS layer; it is not dependent on VAEG mapping a board ROM
  window.
- [The PCPLUS 1.08 correction][pcplus-patch], which fixes its DMA-mask setup.
- [BDIFF/BUPDATE 1.28][bdiff], used only on the host under DOSBox to apply
  that correction reproducibly.
- [SCHD 1.55T][schd], the PC-Engine block-device driver for SCSI hard disks
  and magneto-optical media in the original hardware environment. VAEG only
  supports the fixed-disk path here.
- [VBUFF 1.02][vbuff], which changes the maximum logical-sector buffer size
  recorded in a PC-Engine system disk's IPL.
- [SCFORM 1.24][scform-topic], an interactive SCSI initialization and
  partitioning utility distributed as the forum attachment
  `SCF124.LZH`.

Every downloaded archive and the resulting patched `PCPLUS.SYS` are checked
against fixed SHA-256 values before the disk is created.

The package manuals remain the authority for command details, hardware
limitations, and redistribution conditions. The generated disk retains the
relevant original manuals under `A:\DOC`.

## Port and SCSIBIOS Evidence

### WD33C93 host contract

The PC-9801-55-compatible controller uses the WD33C93-family two-stage host
interface: `0CC0h` selects the controller register and `0CC2h` accesses the
selected register.  Reading `0CC0h` returns auxiliary status.  For PIO, DBR
is the data-ready handshake; CBSY, CIP, and INT are separate controller
status bits.  AR `19h` is a fixed DATA window.  CDB registers `03h`-`0Eh`
are ordinary sequential registers, and the NEC extension range `30h`-`35h`
must not be collapsed into the ordinary `00h`-`1Fh` file.

The primary register reference for this boundary is the
[WD33C93A data sheet and application notes](http://www.bitsavers.org/components/westernDigital/WD33C93A_Data_Sheet_and_Application_Notes_Nov1990.pdf).
Its indirect-addressing rules explicitly exclude Auxiliary Status, DATA, and
COMMAND from address auto-increment.  Its Control register table defines
DMA mode `000b` as polled I/O, where the host polls DBR before each DATA
access.  The M75b2 implementation follows this PIO contract and does not
invent DMA-channel behavior.

For the low-level SELECT path, the host-visible completion sequence is
expected to be event-driven:

```text
11h SELECT complete -> 8Ah COMMAND request -> 89h/88h DATA request
-> 8Bh STATUS request -> 8Fh MESSAGE IN request -> 85h disconnect
```

This is a register/interrupt contract.  The physical REQ/ACK wire protocol
is handled by the controller and need not be exposed as a separate guest
port.  A timer-based injection of `8Ah` is not equivalent to a target phase
event and is not an acceptable correction.  NP2's simplified implementation
is useful only as historical context, not as the WD33C93 specification.

M75a provides a disabled-by-default `--scsitrace` diagnostic that records
every access to `0CC0h`-`0CC6h`, selected AR, data, `CS:IP`, controller phase
and status, auxiliary status, and SCSI IRQ assertion/EOI clear.  The raw
trace remains a local diagnostic artifact and is not committed.

The supplied `SCSI55.TXT` is explicit about the PC-88VA board configuration:
the standard board I/O addresses are `0CC0h`, `0CC2h`, and `0CC4h`. It does
not independently document `0CC6h`. M75 retains the inherited `0CC6h`
byte-stream handler as the data-transfer leg of the controller phase engine
and registers that leg in the VA I/O map. This is an implementation boundary,
not a claim that `0CC6h` is separately specified by `SCSI55.TXT`; its guest
use remains subject to PCPLUS/SCHD validation.

The supplied `SETDMA.ASM` provides a separate and important distinction:
`0CCh` is the software interrupt number for the `$SCSIBIOS` service, not the
`0CC6h` I/O port. `SETDMA.COM` first calls DOS `INT 21h/AH=35h, AL=0CCh`,
then compares six bytes at the returned handler's `ES:000Ah` against
`PCPLUS`. If PCPLUS is installed it calls:

```asm
MOV AX,82C0h
MOV BL,01h
INT 0CCh
```

to request SCSIBIOS DMA mode. The utility does not program a DMA channel and
does not access `0CC6h` directly. This confirms that normal PCPLUS operation
is programmed-I/O (PIO); DMA is an optional mode requested through the
PCPLUS software service. The VA guidance identifies only DMA channels 0 and
3 as expansion-slot choices and warns that SASI and 2TD consume them.

The emulator must therefore keep the following claims separate:

- `0CC0h/0CC2h/0CC4h`: documented VA board configuration ports;
- `INT 0CCh`: PCPLUS-provided software SCSIBIOS entry point;
- `0CC6h`: retained compatibility byte stream; the supplied PCPLUS/SCHD trace
  confirms it is unused by the active low-level path;
- DMA: optional PCPLUS mode, not the default PIO path.

M75b2 records `0CC4h <- 02h` as the DMER reset strobe.  TCIR, TCMR, TCMS,
and DMES remain hardware-pending; unsupported strobes produce a diagnostic
warning rather than changing transfer state.  Reading `0CC0h` does not clear
the device interrupt latch.  Only reading AR `17h` consumes the latched SCSI
status; an 8259 EOI is a separate PIC operation.  AR `32h`, `34h`, and `35h`
remain explicit unsupported/open-register reads and writes until PCPLUS/SCHD
or board documentation supplies evidence for their NEC-specific behavior.

The register progression is part of the contract: AR `17h` is an ordinary
auto-incremented status register, so a status read leaves AR at `18h` for the
next COMMAND write.  AR `18h` and `19h` themselves are fixed windows.  AR
`12h`-`14h` consequently accept a three-byte transfer count without special
address handling.  Undefined AR `1Ah`-`2Fh` values are held and warned about;
no wrap or speculative register is exposed.  The VA IRQ request is gated by
the memory-bank register's IRE1 bit (bit 2), while the internal CSR latch is
preserved when that system IRQ gate is closed.  LCI (bit 6) and PE (bit 1) of
Auxiliary Status are currently defined as zero/unmodeled.

M75c1 now separates SELECT completion from the target COMMAND-phase request:
the `11h` CSR is read first, then `8Ah` is delivered as a second service
event.  The first observed host transfer count is `000006h`, followed by
`AR=18h <- 20h`; Transfer Info remains deliberately held at that boundary
until M75c2 implements the AR=19h PIO byte pump.

M75c2 now accepts the host-programmed 24-bit transfer count and pumps bytes
through fixed AR `19h` with DBR.  Count exhaustion emits CSR `1Ah` and leaves
the controller in COMMAND; it does not decode the CDB or generate later
DATA/STATUS/MESSAGE phases.  M75c3 adds trace-only transfer classification to
record whether each observed count is consumed by this COMMAND path or by
the legacy `0CC6h` path.  Current evidence classifies the observed `000024h`
and `000008h` transfers as COMMAND/AR=19h, not as implemented DATA IN.

VAEG's built-in software SCSI BIOS helper and the C-Bus phase engine are
different internal layers. Rel.260805 has passed the G75 guest lifecycle
gate for SCSI image creation, FAT visibility, file creation, readback,
deletion, close/reopen persistence, and the two-target ID 0/1 path. The
remaining low-level notes in this section document the preserved controller
boundary and are useful when investigating new hardware or guest-driver
compatibility; they are not a prerequisite for the setup procedure above.

## SCHD Driver Evidence

The supplied `SCHD.SYS`, `SCHD.DOC`, `SCHD.LOG`, and `SCHD.TXT` identify
`SCHD` as a DOS block-device driver for PC-88VA, PC-88VA2/3, and PC-Engine
systems. `PCPLUS.SYS` must be loaded before `SCHD.SYS`; the driver then
registers SCSI hard-disk or magneto-optical media as a DOS block device. The
documented `-I0` through `-I7` option selects the SCSI target ID. `-C` and
`-S` override geometry, `-B` selects the larger sector buffer, and `-X`
changes the removable-media policy. These options are guest-driver policy,
not additional emulator I/O ports.

The revision log records that SCHD's SCSIBIOS interface was split from the
driver, that packet/address transfers were changed to word accesses, and that
an earlier `REP MOVSW`/`REP STOSW` implementation error was corrected. A
byte-level inspection of the supplied `SCHD.SYS` contains five `CD CC`
(`INT 0CCh`) call sites and no `CD 1Bh` calls. It also contains no literal
`MOV DX,0CC0h/0CC2h/0CC4h/0CC6h` setup sequence. This is consistent with the
documented architecture: SCHD calls the PCPLUS software SCSIBIOS entry point
and does not establish a separate direct `0CC6h` VA contract. The byte scan is
evidence of the call boundary, not a substitute for a complete disassembly of
the proprietary driver.

Accordingly, a VA SCSI implementation must first make the `INT 0CCh`
PCPLUS/SCSIBIOS path observable and correct. Direct registration of the
legacy NP2 `0CC6h` stream remains unsupported unless a guest trace or an
authoritative VA board document demonstrates that SCHD uses it. Normal PIO is
the expected path; `SETDMA.COM` can request optional PCPLUS DMA mode after the
software BIOS has been installed, but SCHD's presence alone does not require
DMA emulation.

## Building the Disk

[`tools/pc88va/scsi-support.sh`](../../tools/pc88va/scsi-support.sh) takes a
user-supplied PC-Engine 1.1 D88 system disk and produces a new bootable D88.
The source image and generated image are local artifacts and must not be
added to Git.

On Debian or Ubuntu, install the host-side dependencies with:

```sh
sudo apt-get install curl dosbox lhasa python3 coreutils
```

Build a disk for a SCSI target whose ID is zero:

```sh
tools/pc88va/scsi-support.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pc88va-scsi-support.d88
```

Use `--scsi-id 0..7` when the target does not use ID 0:

```sh
tools/pc88va/scsi-support.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pc88va-scsi-id-3.d88 \
  --scsi-id 3
```

The destination must not already exist. Public input archives use the shared
`~/.cache/vaeg/auto-generated-pc88va-utility-media/` cache. A cached file with
the wrong checksum is rejected and is not silently replaced.

The builder first uses
[`create-vanilla-system-disk.sh`](../../tools/pc88va/create-vanilla-system-disk.sh)
to retain only the PC-Engine 1.1 IPL and required system files. It then
installs the SCSI drivers, utilities, and original documentation:

```text
A:\
  AUTOEXEC.BAT
  CONFIG.SYS
  PCPLUS.SYS
  SCHD.SYS
  ENGINEIO.SYS
  PCENGINE.SYS
  ADVGBIOS.SYS
  PCENGINE.COM

A:\BIN\
  SCFORM.COM
  VBUFF.COM

A:\DOC\
  PCPLUS.DOC
  PCPLUS.TXT
  SCSI55.TXT
  SCHD.DOC
  SCHD.LOG
  SCHD.TXT
  SCFORM.DOC
  SCFORM.LOG
  VBUFF.DOC
  VBUFF.LOG
```

`AUTOEXEC.BAT` adds `A:\BIN` to the command path. For target ID 0, the
generated `CONFIG.SYS` is:

```dos
FILES = 20
BUFFERS = 10
DEVICE = A:\PCPLUS.SYS
DEVICE = A:\SCHD.SYS -I0
```

PCPLUS must load before SCHD. The value after `-I` is the target's SCSI ID
and is generated from `--scsi-id`.

The SASI development-disk builder assumes the attached 160 MB fixed SCSI
target by default (the generated SCHD line remains the normal fixed-target
line).  For an external removable medium, build an explicit profile rather
than changing the normal boot disk in place:

```sh
tools/pc88va/build-sasi-utility-disks.sh --scsi-profile mo-128mb
```

`--scsi-profile mo-128mb` or `mo-160mb` adds `-X -D1` to the generated SCHD
line and writes a manual setup note under `A:\DOC`.  `fixed-160mb` keeps the
fixed-disk policy while documenting the larger-sector procedure.  The image
produced by this command remains the 40 MB SASI boot/support disk; the profile
prepares its guest software for a separate SCSI target.  VBUFF, SCFORM,
STEST55S SFORM, and SETDMA are deliberately not run by the host builder.

## Interface-Board Setup

Read `A:\DOC\SCSI55.TXT` before configuring a PC-9801-55 or compatible
board. Its PC-88VA-specific guidance includes these points:

- Use the board's standard I/O ports `0CC0h`, `0CC2h`, and `0CC4h`.
- Select an interrupt that does not conflict with installed hardware.
  The document identifies the PC-88VA's `INT0` through `INT3` choices and
  notes existing 2TD and SASI assignments.
- Programmed I/O is the normal transfer mode. Only DMA channels 0 and 3 are
  exposed to expansion slots, and those channels can conflict with SASI and
  2TD hardware. The forum note additionally warns that a board supporting
  only bus-master transfer is unlikely to work.
- The board ROM normally occupies `0DC000h-0DCFFFh`. The PCPLUS note says
  that this range is otherwise unused on a PC-88VA, but it must not overlap
  an EMS page frame. Disabling the board ROM is supported and is the VAEG
  default. VAEG does not copy an embedded or host `scsi.rom` image into
  `D2000h` or another VA system-memory window.

These are software-configuration notes, not electrical-installation
instructions. Follow the interface board and target-device manuals for
termination, cabling, and hardware switch settings.

## Logical-Sector Buffer

An unmodified PC-Engine system normally handles logical sectors no larger
than 1024 bytes. SCFORM 1.24 can create a partition of roughly 1 through
64 MB with that logical-sector size.

Larger partitions require a larger PC-Engine buffer. To change the system on
drive A to a 2048-byte maximum logical sector, boot the support disk and run:

```dos
VBUFF -D1 -B11
```

VBUFF drive numbering is `0` for the current drive, `1` for A, `2` for B,
and so on. VBUFF changes a value in the selected system disk's IPL, so make
a backup first and select the drive containing the PC-Engine system files,
not the SCSI target. Reboot after changing it. A larger buffer also consumes
more guest memory.

VBUFF is deliberately not run by `AUTOEXEC.BAT`: 1024-byte partitions need
no IPL change, and altering the wrong boot disk would be an unsafe default.

## Initializing and Partitioning the Target

SCFORM writes SCSI disk metadata and can destroy existing partitions and
files. Back up the target before running it.

For a partition of at most approximately 64 MiB, start the interactive
formatter with:

```dos
SCFORM
```

After applying the 2048-byte VBUFF setting, start SCFORM with its
logical-sector-size extension:

```dos
SCFORM /S       ; up to approximately 128 MiB (2048-byte sectors)
SCFORM /SS      ; 129-256 MiB (4096-byte sectors)
```

The SCFORM 1.24 manual defines the option as `-S` and demonstrates the
slash form in its `SCFORM /MS` example. The forum topic says `-B` at this
point, but `-B11` belongs to VBUFF. Each additional `S` doubles the logical
sector size: `/S` exposes 2048-byte sectors, `/SS` exposes 4096-byte sectors,
and `/SSS` exposes 8192-byte sectors. The option expands the choices; it does
not select a value automatically. Therefore a 129-256 MB region requires
`SCFORM /SS`, not `/S`.

In SCFORM's menus:

1. Select the target by SCSI ID.
2. Initialize the device only if its current contents may be discarded.
3. Allocate a region, selecting a logical-sector size supported by the
   current VBUFF setting.
4. Leave no more than four desired regions active; SCHD exposes the first
   four active regions.
5. Exit and reboot before using the new drive.

SCFORM's documented approximate partition ranges are 1-64 MB with
1024-byte logical sectors, 65-128 MB with 2048-byte logical sectors, and
129-256 MB with 4096-byte logical sectors. Prefer the smallest logical
sector size that can represent the required partition.

## Driver Overrides and Limitations

SCHD normally discovers the cylinder count and blocks per cylinder. If the
target does not report usable geometry, add `-C<number>` for the cylinder
count or `-S<number>` for blocks per cylinder to its `CONFIG.SYS` line. When
both are supplied, SCHD gives `-S` precedence. This SCHD `-S` option is
unrelated to SCFORM's `/S` logical-sector option.

The historical setup has these important limitations:

- It cannot boot PC-Engine from the SCSI disk. Keep the generated floppy as
  the boot medium.
- The forum reports file corruption after roughly 30 MiB of use with some
  old PC-Engine system disks. It does not identify the first known-safe
  release. This builder intentionally requires the documented PC-Engine 1.1
  layout, but that structural check is not proof against the reported
  historical defect.
- SCHD rejects a partition whose logical-sector size exceeds the buffer
  recorded in the boot system's IPL.
- A larger logical-sector buffer consumes additional conventional memory.
- Geometry overrides should be copied from authoritative target-device
  specifications. Guessing `-C` or `-S` values can make the disk
  inaccessible.

## Verification

The builder verifies downloads, the patched PCPLUS result, the source
PC-Engine 1.1 filesystem layout, and the generated FAT12 structure. The
current VAEG build additionally validates SCSI image attachment, ROM-backed
startup, and the controller phase contract. The following guest-level checks
remain the manual M75 gate because they require the PC-Engine support disk and
an observation of the PCPLUS/SCHD software path:

1. Boot the generated disk in V3 mode.
2. Confirm that PCPLUS loads before SCHD and that SCHD reports the intended
   SCSI ID.
3. Run `SCFORM` only on a disposable or backed-up target.
4. Reboot, confirm that each active region receives a drive letter, then
   create, read, and delete test files.
5. For a large partition, cross the 30 MiB usage point with nonessential test
   data before trusting the environment.

[forum-501]: http://www.pc88.gr.jp/forum/viewtopic.php?t=501
[pcplus]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=378
[pcplus-patch]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=451
[bdiff]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=328
[schd]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=448
[vbuff]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=452
[scform-topic]: http://www.pc88.gr.jp/forum/viewtopic.php?t=502
