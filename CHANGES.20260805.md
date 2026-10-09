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

# 88VA Eternal Grafx Rel.260805

This release contains the user-facing storage and PC-Engine workflow changes
made after `rel-260713`.

## PC-88VA storage

- Added PC-9801-55-compatible SCSI HDD attachment, image creation, and GUI
  slots for two SCSI targets.
- Added SCSI LUN, INQUIRY, geometry, READ, WRITE, and Transfer Info handling
  required by the PC-Engine `SCSIBIOS`/`SCHD.SYS` path.
- Added persistent SCSI and SASI image mounting and automated file lifecycle
  checks for creation, readback, deletion, close/reopen persistence, and the
  second SCSI target.
- Corrected complete FAT backing, exact large SCSI reads, direct SCSI writes,
  and the data-transfer paths used by the guest.
- Preserved the existing SASI and non-SCSI disk paths.

See the [SCSI setup guide](docs/modernization/scsi-support.md) for image
creation, support-disk preparation, target IDs, and SCFORM setup.

## HOSTFAT

- Added transactional HOSTFAT rebuild/reset handling so an invalid rebuild
  leaves the prior usable mount in place and reports the error visibly in the
  GUI.
- Restored HOSTFAT text entry and improved Windows path handling, including
  Dropbox roots and contained reparse points.
- Added HOSTFAT guest read regression coverage and generic headless input
  scripts for PC-Engine workflows.
- The release packages include the generated `HOSTFAT.SYS` driver. The
  current driver is named HOSTFAT; the removed legacy HOSTDRV driver is not
  part of this release.

See the [HOSTFAT setup guide](docs/modernization/hostfat.md) for release
installation, GUI rebuild/reset behavior, DOS checks, and troubleshooting.

## Validation

- Linux, macOS, and MinGW release packages run the smoke and unit-test gates.
- SASI, SCSI, dual-SCSI, and HOSTFAT storage regressions are automated where
  the required PC-Engine assets are available locally.
- ROMs and disk images remain external user-supplied files and are not
  included in the release packages.

## Known limitations

- Full guest-media lifecycle regression requires the PC-Engine ROM and
  support-disk assets and is therefore a local test rather than a hosted-CI
  test.
- `HOSTFAT.SYS` is read-only by design; writes are rejected by the driver.
