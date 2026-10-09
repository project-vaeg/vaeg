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
# PC-88VA 2TD Floppy Research

This note records the available evidence for the PC-88VA 9.3MB 2TD floppy
system and the changes that would be required to emulate it in vaeg. It is a
research note. The active vaeg tree does not currently emulate a 2TD drive.

## Hardware Summary

NEC's [PC-88VA3 specification](https://support.nec-lavie.jp/support/product/data/spec/cpu/b049-1.html)
lists one built-in 3.5-inch 2TD drive with a formatted capacity of 9.3MB in
addition to the two standard 5.25-inch 2D/2HD drives. It also states that an
external 9.3MB drive requires the PC-88VA-22 interface board and that a VA3 can
use only one external drive.

Contemporary and later descriptions give the following geometry:

| Property | Reported value |
|---|---:|
| Form factor | 3.5 inch |
| Unformatted capacity | 12.5MB |
| Formatted capacity | 9.3MB marketing value |
| Surfaces | 2 |
| Cylinders | 240, numbered 0 through 239 |
| Sectors per track per surface | 38 |
| Bytes per sector | 512 |
| Encoding | MFM |
| Rotation | 360rpm |
| Formatted byte count | 9,338,880 bytes |

The byte count follows directly from the geometry:

```text
2 surfaces * 240 cylinders * 38 sectors * 512 bytes = 9,338,880 bytes
```

This is approximately 9.12MiB. The 12.5MB and 9.3MB figures are respectively
unformatted and formatted decimal marketing capacities.

The geometry and FAT12 use are independently summarized by the
[2TD technical entry](https://www.wdic.org/w/TECH/2TD) and a
[floppy technology overview](https://electrelic.com/electrelic/node/832).
The latter also notes that the unusually high track count required feedback
head positioning rather than a conventional open-loop stepper arrangement.
The later
[JIS X 6227 10MB 90mm media standard](https://kikakurui.com/x6/X6227-1997-01.html)
specifies 360rpm, MFM recording, and recorded servo positioning for the same
general class of high-capacity media. It specifies a later 255-track format,
however, so it corroborates the technology but not the exact VA3 geometry.
The exact drive mechanism, recording-density tolerances, and controller timing
must still be verified from a hardware manual or drive specification before
they are treated as emulator constants.

At 360rpm, one revolution takes about 166.67ms. Carrying 38 512-byte sectors
plus address marks, CRCs, and gaps in that period is consistent with a 1Mbit/s
controller data rate. This is an engineering inference, not yet a confirmed
PC-88VA controller specification.

## Model and Expansion Configuration

The VA3 contains one 2TD drive. The original VA and VA2 could use an external
PC-88T31 drive through the PC-88VA-22 9.3MB microfloppy interface board. The
PC-88VA-22 and PC-88T31 pairing is listed in a
[PC-8800 option-board index](https://www.je1vuj.net/pc88/boad.html), while the
VA3 external-drive requirement is stated directly in NEC's specification.

A [PC-88VA-22 hardware entry](http://pc88pc98.web.fc2.com/pc-8801etc/pc-88va-22.html)
adds that the PC-88T31-01 was a one-drive expansion for the PC-88T31 and
explicitly says that a VA3 cannot connect a second PC-88T31. Taken together,
this supports the following maximum configuration:

| Model | Built-in 2TD | External 2TD configuration | Apparent maximum |
|---|---:|---|---:|
| VA | 0 | PC-88T31 plus PC-88T31-01 expansion | 2 |
| VA2 | 0 | PC-88T31 plus PC-88T31-01 expansion | 2 |
| VA3 | 1 | One external PC-88T31; no second external unit | 2 total |

The source is a retrospective hardware catalog rather than an NEC manual, so
the two-drive limit for VA and VA2 should still be checked against a surviving
PC-88VA-22 or PC-88T31 manual. It is nevertheless stronger evidence than the
previous assumption that the optional path exposed only one 2TD drive.

An individual [VA3 hardware report](https://www.asayan-town.com/nec/pc-88va3/)
records the following resource assignment:

| Resource | Assignment in the report |
|---|---|
| Internal 2TD interrupt | INT1 |
| External 2TD interrupt | INT0 |
| 2TD DMA channel | DMA3 |
| Standard 5.25-inch FDD DMA channel | DMA2 |

This table is useful implementation evidence but is not a substitute for the
PC-88VA-22 or VA3 circuit documentation. In particular, the exact external
board I/O ports, interrupt-selection behavior, and whether every model uses
DMA3 identically remain unverified.

## Compatibility with Conventional Media

The 2TD drive could read conventional 3.5-inch 2HD and 2DD media. The
[PC-88VA2/3 retrospective](https://akiba-pc.watch.impress.co.jp/docs/column/retrohard/1319404.html)
reports this read compatibility. Secondary references state that conventional
2HD/2DD media was read-only in the 2TD drive.

This behavior should be modeled as a drive capability rather than as a property
of every mounted image:

- 2TD media: read and write, subject to image write protection.
- Conventional 2HD/2DD media in the 2TD drive: read-only.
- Standard 5.25-inch VA drives: retain their current independent behavior.

The exact set of accepted conventional 3.5-inch geometries must be verified.
It should not be inferred from all formats accepted by vaeg's existing generic
FDD layer.

## Current vaeg FDD Architecture

The current implementation models the standard VA 5.25-inch subsystem, not the
VA3 2TD device:

- `io/fdsubsys.c` is explicitly a mock-up PC-88VA FD subsystem. It exposes
  two drives and implements a subset of the intelligent subsystem commands.
- `io/fdc.c` and the global `fdc` state implement one uPD765A-style controller
  path for 2D, 2DD, and 2HD media.
- `fdd/fddfile.h` defines only `DISKTYPE_2D`, `DISKTYPE_2DD`, and
  `DISKTYPE_2HD` media classes.
- `fdd/fdd_d88.c` handles standard D88 images and rejects track indexes at or
  above 164.
- `fdd/newdisk.c` creates only 77-cylinder 2HD and 80-cylinder 2DD MS-DOS
  images.
- `machine/pccore.h` distinguishes VA1 and VA2 only. The active frontend presents the
  latter as `VA2/VA3`; there is no runtime `PCMODEL_VA3` hardware distinction.

The standard subsystem uses ports `0FCh` through `0FFh` for its main/subsystem
handshake and the VA FDC control path around `01B0h`. No separate 2TD controller
or image slot is bound.

The local `docs/tekumani/601FDD.TXT` material describes conventional floppy
BIOS operations and 2D/2DD/2HD modes. A search of the local technical-manual
set found no 2TD description. The required register-level information must
therefore come from another manual, ROM analysis, bus traces, or board reverse
engineering.

The current MAME PC-88VA source likewise defines `pc88va` and `pc88va2`, but no
VA3/2TD machine or device is present in the
[current PC-88VA driver](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp).
MAME therefore does not currently provide a ready 2TD implementation to use as
a semantic reference.

## D88 Image Limitation

Standard D88 is not large enough to describe a complete 2TD disk. Its header
contains 164 track pointers, enough for up to 82 double-sided cylinders. A 2TD
disk requires 480 physical track entries:

```text
240 cylinders * 2 surfaces = 480 tracks
```

This is visible directly in `fdd/d88head.h`, where `_D88HEAD.trackp` has 164
entries, and in `fdd/fdd_d88.c`, where track indexes of 164 or greater are
rejected. Reusing standard D88 would either truncate the disk or require a
private incompatible extension and should not be done silently.

The initial sector-image candidate should therefore be a dedicated fixed-size
raw format containing exactly 9,338,880 bytes in cylinder/head/sector order.
An explicit extension such as `.2td` is preferable to guessing from every
generic `.img` file. The final ordering and sector numbering must be confirmed
against an actual disk image before the format is frozen.

A raw image cannot preserve weak bits, unusual address marks, per-sector CRC
errors, duplicated IDs, exact gap layout, or servo information. Preservation
of original media and copy protection needs a flux or other timing-aware image
format as a separate later scope.

## Required Emulator Boundary

The 2TD drive should not be added as a third entry that shares the existing
global FDC state without further analysis. The real machine can operate its
standard 5.25-inch subsystem and 2TD controller with different interrupt and
DMA resources. A correct design needs independently owned controller state.

A staged implementation should use these boundaries:

1. Add an explicit VA3 distinction or a separate 2TD hardware configuration.
   This must also represent the optional PC-88VA-22 path for VA and VA2.
2. Add a dedicated 2TD controller/device state with its own reset, IRQ, DMA,
   motor, seek, ready, write-protect, and rotational timing state.
3. Determine the actual register and command interface from the VA2/VA3 ROM,
   PC-88VA-22 documentation, or real-hardware traces before binding I/O ports.
4. Add up to two separately addressable 2TD media slots and a fixed-geometry
   raw backend. Do not overload the two standard FDD menu entries or
   `DISKTYPE_2HD`. Model the VA/VA2 external two-drive topology separately from
   the VA3 internal-plus-external topology.
5. Route the verified interrupt and DMA resources independently from the
   standard FDD subsystem.
6. Implement 2TD sector read first, followed by write and format after the
   on-disk FAT12/BPB layout is verified from genuine media.
7. Add conventional 2HD/2DD read-only compatibility only after the accepted
   geometries and controller mode-switch behavior are known.
8. Add rotational latency, 240-cylinder seek behavior, and format timing after
   functional controller compatibility passes.

The controller chip used by the VA3 has not been established by this research.
The later NEC uPD72070 documentation shows that a uPD765-compatible controller
can support 2TD, but its 1991 specification is not evidence that the 1988 VA3
used that chip. It may be used only as general controller background until the
VA3 hardware is identified.

## Image and Filesystem Acquisition

A reliable implementation needs at least one legally obtained known-good 2TD
image. Before using it as a reference, record:

- source hardware and drive model;
- imaging hardware and software;
- whether the capture is sector-level or flux-level;
- full file size and cryptographic hash;
- sector numbering and interleave;
- boot-sector BPB values and FAT type;
- whether both surfaces and all 240 cylinders were read;
- bad sectors, CRC errors, deleted-data marks, and retries;
- write-protect state and whether the disk is an original or user-formatted
  medium.

The published geometry identifies FAT12 use but does not prove the exact BPB,
media descriptor, FAT size, root-entry count, sectors per cluster, boot code,
or interleave used by PC-Engine. `newdisk.c` must not invent these values from
generic MS-DOS defaults.

## Automated Verification

ROM-less tests should cover:

- exact 9,338,880-byte image-size acceptance and all other sizes rejected;
- CHS/LBA conversion at the first and last sectors;
- cylinder 239, head 1, sector 38 access without integer truncation;
- independent standard-FDD and 2TD controller state;
- independent selection and bounds for both possible 2TD drives;
- reset, ready, motor, write-protect, IRQ, and DMA transitions;
- rotational wrap at 360rpm;
- 2TD read/write bounds and short-file handling;
- conventional 2HD/2DD write rejection in the 2TD slot;
- malformed-image handling without guest or host memory corruption.

The human gate should include:

- VA3 boot with the 2TD drive empty;
- mounting and listing a known-good 2TD system or data disk;
- reading files from the first and last cylinders;
- writing, remounting, and verifying a disposable 2TD image;
- formatting only after the genuine FAT12 layout is known;
- reading supported 2HD and 2DD media and rejecting writes to them;
- simultaneous use of the standard 5.25-inch drives and 2TD drive;
- both 2TD drives on VA/VA2, and the internal-plus-external pair on VA3;
- interrupt/DMA conflict checks for internal and external configurations;
- no behavior change when 2TD hardware is disabled.

## Open Questions

- What controller and drive model are used in the VA3 and PC-88T31?
- What are the 2TD controller I/O ports, registers, commands, and status bits?
- How do internal and external configurations select INT0/INT1 and DMA3?
- What are the exact 1Mbit/s clock, gap, interleave, seek, and servo timings?
- Which 2HD and 2DD geometries are accepted, and how is read-only behavior
  reported to software?
- What is the exact PC-Engine FAT12 BPB and boot-sector layout?
- Which image format can preserve original 2TD media beyond plain sectors?
- Is a distinct `PCMODEL_VA3` required, or should 2TD presence remain an
  independently configurable peripheral for VA/VA2 external-drive support?

Until these are answered, 2TD should remain a documented future device rather
than an approximate extension of the existing D88/FDC path.
