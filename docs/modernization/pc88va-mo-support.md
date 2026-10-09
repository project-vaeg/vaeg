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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# PC-88VA 128 MB MO compatibility notes (unsupported in VAEG)

Language: [English](pc88va-mo-support.md) | [日本語 SCSI procedure](scsi-support.ja.md)

## Scope and current result

The PC-88.gr.jp packages for SCHD 1.55T and VA128MO, together with the OSL
STEST 1.15 package, were audited and are retained as optional reference inputs
to [`build-sasi-utility-disks.sh`](../../tools/pc88va/build-sasi-utility-disks.sh).
The builder verifies the original archive bytes, keeps each archive under
`A:\ARCHIVE`, and installs the runnable files and manuals in `A:\SYS`,
`A:\BIN`, and `A:\DOC` on both the VA and VA2 40 MB development HDIs.  The
fixed-HDD `CONFIG.SYS` remains unchanged: an MO option must not silently alter
the known-good boot path. VAEG does not support MO/removable-media operation;
the packages and procedures below are documentation for separately validated
real hardware only. Formatting a real MO is deliberately not automated.

The package listing pages are the provenance references:

- [SCHD155T (gnum 448)](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=448)
- [VA128MO (gnum 449)](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=449)
- [STEST115 (gnum 450)](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=450)
- [STEST115.LZH (OSL driver archive)](https://www2u.biglobe.ne.jp/~pumpkin/hlabo/osl/driver/STEST115.LZH)
- [ST115SRC.LZH (OSL source archive)](https://www2u.biglobe.ne.jp/~pumpkin/hlabo/osl/driver/ST115SRC.LZH)

With the three archives in the shared Auto-generated PC-88VA utility media
cache at `~/.cache/vaeg/auto-generated-pc88va-utility-media/`, the existing
wrapper installs them in both variants alongside the other development
software:

```sh
tools/pc88va/build-sasi-utility-disks.sh \
  --output-dir /private/tmp/pc88va-sasi-mo
```

Use `--mo-schd-archive`, `--mo-va128mo-archive`, or `--mo-stest-archive` when
the archives are stored elsewhere. The wrapper extracts each archive to a
temporary host directory; those extracted files and the generated HDIs are
not tracked in Git.

The verified archive identities are:

| archive | bytes | SHA-256 |
| --- | ---: | --- |
| `SCHD155T.LZH` | 15,360 | `87aebcf7c9bc9c6170a40d0e6ddcce5afdcbb1fa55f1fdeeec815458f7ef065f` |
| `VA128MO.LZH` | 3,584 | `1dc8f366fb56e1761051e9b0c1e8950999ebb6df10ddf1bb91251e2557728a36` |
| `STEST115.LZH` | 107,136 | `6ae981b0010df20a510f85165567add33032241854b147ed47937a59953010bc` |
| `ST115SRC.LZH` | 60,672 | `1192d3a38a4d9444a9b8b021fcd550e61e7b860bc39b67a01868c58e62bc2e51` |

Installed files are:

```text
A:\ARCHIVE\SCHD155T.LZH
A:\ARCHIVE\VA128MO.LZH
A:\ARCHIVE\STEST115.LZH
A:\ARCHIVE\ST115SRC.LZH
A:\SYS\SCHD.SYS
A:\DOC\SCHD.DOC  A:\DOC\SCHD.LOG  A:\DOC\SCHD.TXT
A:\DOC\VA128MO.DOC
A:\BIN\STEST.EXE  A:\BIN\STESTX.COM  A:\BIN\STEST.BAT
A:\DOC\STEST115.DOC  A:\DOC\COMMAND.DOC  A:\DOC\UTILITY.DOC
```

`STEST115.LZH` is the runnable STEST 1.15 distribution and is expanded into
`A:\BIN` with its manuals in `A:\DOC`.  `ST115SRC.LZH` is retained unchanged
under `A:\ARCHIVE` as the corresponding source distribution; it is not
expanded on the fixed-size FDD image.  Both archives are downloaded and
verified by the SASI builder from the OSL URLs above.

`AUTOEXEC.BAT` already puts `A:\BIN` on `PATH`, so `STESTX.COM` and the
other installed utilities are discoverable.  `STEST.BAT` creates the
machine-specific `STEST55S.EXE` name; the package does not ship that generated
copy.  The VA128MO instructions describe `VBUFF -D1 -B11` and
`STEST55S SFORM` as operator actions.  `SFORM` can physically format or
reassign a medium and must remain an explicit, destructive real-hardware
operation.

## Evidence from the package manuals

`VA128MO.DOC` targets a PC-9801-55/92-compatible SCSI interface and a 3.5-inch
128 MB MO.  It requires PCPLUS, SCHD 1.55 or later, VBUFF, SETDMA, and the
STEST family.  Its semi-IBM example uses SCSI ID 0, `SCHD.SYS -I0 -X -D1`,
and an STEST logical format choice named `3.5" IBM-like`.  SCHD's own manual
describes `-I0..7`, removable-media handling, and larger logical sectors; it
also notes that old `-D` handling changed in later releases.  For that reason
the builder installs the packages but does not add historical MO switches to
the default fixed-HDD `SCHD.SYS -I0` line.

The manual's reported maximum LBA is `248825`.  The capacity arithmetic is
consistent with 128 MiB represented by 262,144 512-byte blocks, with a
filesystem reserving part of the medium.  One OCR-ambiguous line says
`2048 KB (512KB)`; it is not sufficient evidence for a 2,048-byte device
block.  The proposed image contract therefore uses an explicit 512-byte
block size and keeps the logical-sector choice in the guest software layer.

## What VAEG supports today

The current code has a type constant for MO (`fdd/sxsi.h:8-19`), but it is
not a working removable-medium implementation.  `fdd/sxsi.c:114-217` parses
SCSI VHD/HDD files as fixed direct-access disks, accepts only 256/512/1024
byte blocks, and stores no media-present, changed, write-protect, or
prevent-removal state.  `cbus/scsicmd.c:60-71` always returns a fixed HDD
INQUIRY descriptor, and the command path currently covers TUR, REQUEST SENSE,
INQUIRY, MODE SENSE(6), READ CAPACITY(10), and READ/WRITE(6/10), but not
START/STOP UNIT, PREVENT/ALLOW MEDIUM REMOVAL, or media-change/unit-attention
semantics.  The generic read/write backend has no removable-media policy.

The SDL2 menu (`sdl2/gui/gui.cpp:2389-2433`) exposes only SASI and SCSI-ID
fixed-disk slots, and `sdl2/ini.c:348-354` persists only one path per SCSI
slot.  Thus attaching a 128 MB file as an ordinary VHD would make it look
like an HDD and would not reproduce the guest-visible MO contract.

## Proposed implementation by layer

### SCSI/device layer

Keep the existing HDD target behavior as the compatibility path and add an
explicit removable target profile.  Do not infer media type from a filename
extension.  A future MO image should carry a distinct signature/profile (for
example, a versioned VMO container) or an explicit per-slot type in the
configuration.  The target state should include:

- 512-byte physical block size and block count (the 128 MiB profile is
  262,144 blocks before filesystem reservations);
- removable, media-present, write-protected, started/stopped, and
  prevent-removal flags;
- media-changed/unit-attention state and a last-sense record;
- an optional bad-block/reassignment map only after the guest contract is
  measured.

Implement the SCSI state machine above the generic `sxsi_read`/`sxsi_write`
file backend.  INQUIRY should identify a removable direct-access optical
device (RMB set), while TUR, REQUEST SENSE, READ CAPACITY, MODE SENSE/SELECT,
READ/WRITE, START/STOP, and PREVENT/ALLOW must expose the state transitions.
GUI eject/load should become media attach/detach events and set unit
attention; it must not silently format an image.  `FORMAT UNIT`/SFORM should
initially be an explicit unsupported or destructive operation, not a fake
success.  Add isolated command and media-state tests before changing the
phase engine.  The existing 256/512/1024-byte fixed-disk validation should
remain unchanged for HDD images.

### Guest software layer

The package installer is the first, non-destructive step.  It preserves the
known-good boot configuration and exposes the manuals and STEST tools as
real-hardware reference material.  A future hardware-only profile may document
an opt-in configuration snippet, but it must not be read as VAEG MO support:

```dos
DEVICE=A:\SYS\PCPLUS.SYS
DEVICE=A:\SYS\SCHD.SYS -I0 -X
VBUFF -D1 -B11
```

The SASI development-disk builder now exposes that choice explicitly.  Its
default assumes the attached 160 MB fixed SCSI disk; no removable-media
policy is added unless requested:

```sh
tools/pc88va/build-sasi-utility-disks.sh --scsi-profile mo-128mb
```

The builder profiles are `fixed`, `fixed-160mb`, `mo-128mb`, and `mo-160mb`.
The `mo-*` profiles are reference-only staging profiles and do not provide MO
support in VAEG.  `--scsi-id 0` through `--scsi-id 7` selects the SCHD target
ID.  The MO profiles generate `SCHD.SYS -I<n> -X -D1` and install a profile
note under `A:\DOC`; the note records the required manual `VBUFF` setting
(`-B11` for a 128 MB medium and `-B12` for a 160 MB target), reboot,
`SCFORM /S` for 128 MB or `SCFORM /SS` for a 160 MB target, and the destructive
`STEST55S SFORM` step.  None of those commands is run while building the 40 MB
SASI support disk.  `fixed-160mb` keeps the fixed-disk SCHD policy and only
records the larger-sector operator procedure.

The exact `-D`/logical-sector combination must follow the installed SCHD
revision and the operator's medium.  `STEST55S SFORM` is intentionally left
manual because it performs physical and logical formatting.  Guest QA should
progress from `scan_scsi`, INQUIRY, TUR, READ CAPACITY, and MODE SENSE to
attach/eject and prevent/allow traces; WRITE and FORMAT require separate
evidence.  The package bytes and the existing PCPLUS/SCHD path are kept
separate from emulator implementation so a guest failure identifies the
first layer that diverges.

### GUI layer

Add a distinct “SCSI MO ID n” entry rather than relabeling an HDD slot.  The
chooser should show media type, capacity, present/ejected, writable/read-only,
and write-protect state.  Provide explicit Load/Attach, Eject, and
Prevent/Allow controls; an eject should be rejected while prevent-removal is
active.  Persist both the path and explicit type, not just a path or extension.
The existing SASI/HDD menu and configuration keys must remain compatible.
New-image creation should offer a clearly labelled preformatted MO profile
only when its on-disk format is specified; never overwrite or format a host
file from the ordinary Open dialog.

## Verification order and boundaries

1. Build both development HDIs and verify the installed file list and package
   hashes.  This is implemented and host-testable now.
2. Add a small explicit VMO fixture and test INQUIRY/TUR/REQUEST SENSE/READ
   CAPACITY, then media-change and START/STOP/PREVENT/ALLOW transitions.
3. Compare guest `STEST` traces in VAEG; only then add WRITE or FORMAT behavior.
4. Add the GUI type/media-state controls and persistence.
5. Perform human PC-88VA and PC-88VA2 checks with a disposable 128 MB MO or
   a hardware-authentic image.  A host image mount or a VAEG command test is
   not real-hardware conformance.

The active SCSI path is currently a fixed HDD path.  No SCSI core or GUI code
was changed by the package-install milestone, so fixed HDD boot behavior and
existing SASI/PCPLUS regressions remain isolated.  The next implementation
milestone should begin with an approved media-state contract and a failing
guest trace, not with filename heuristics or a broad SCSI rewrite.

## Status classification

FACT: the three archives and package manuals are checksum-verified and can be
installed reproducibly into both development HDIs.

FACT: current VAEG exposes no complete removable-MO SCSI state machine; the
existing `SXSITYPE_MO` enum value is unused by the command path.

HYPOTHESIS: the observed “MO does not work” symptom is caused by presenting a
removable medium as a fixed HDD and by missing media-state commands.  This is
the leading implementation hypothesis, not a measured guest root cause.

UNRESOLVED: exact physical MO image container, bad-sector behavior, MODE
SENSE pages required by SCHD, and the real VA/VA2 SCSI timing/format contract.
