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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# M54: Read-only HOSTFAT prototype

Status: **original implementation passed G54 at `19626dc`; clean-room driver
replacement complete with supplemental human revalidation pending**

Starting SHA: `e5741b83fe12f121bb2eb955cbd7b8f0d2af61f2`

## Goal

Provide a behaviorally isolated proof that PC-Engine can access a host-folder
snapshot through a normal CONFIG.SYS block device without relying on the
undocumented MS-DOS INT 2FH redirector, SDA, CDS, or DOS-version internals.

The guest-visible device is a fixed read-only FAT disk. It is not a live host
directory and it never writes to the host.

## Required behavior

- Add a session-only `--hostfat-dir <path>` option. Do not persist it.
- Before machine startup, copy one constrained host directory tree into an
  immutable 8 MiB FAT12 snapshot.
- Use the RDBMS-compatible geometry: 1024-byte sectors, two sectors per
  cluster, no reserved sectors, two seven-sector FATs, 128 root entries,
  8186 DOS-visible sectors, and media ID F0H. Retain an 8192-sector backing
  allocation but hide its final six sectors so PC-Engine counts 4084 data
  clusters and selects FAT12 rather than FAT16.
- Accept only deterministic ASCII 8.3 names in M54. Fold lowercase ASCII to
  uppercase and fail on collisions or unsupported names; never omit a source
  entry silently.
- Reject links, special files, excessive depth, excessive entry count, files
  that change while copied, and content that cannot fit the fixed image.
- Add a versioned emulator-private sector service over the existing documented
  07EDH value channel and 07EFH string channel.
- Validate the complete guest request, LBA range, count, destination range,
  mounted-image state, and protocol version before modifying guest memory.
- Add a clean-room `HOSTFAT.SYS` block driver using the factual request-packet
  contract recorded in
  `docs/agents/research/m54_hostfat_cleanroom_spec.md`.
- Support initialization, media check, Build BPB, read, removable-query, and
  deterministic unsupported-command handling.
- Return write-protect for write and write-with-verify without invoking the
  host service.
- Keep the image mounted across ordinary guest resets within the same session.
- Display a concise PC-Engine initialization success or unavailable message.
- Provide a reproducible NASM build and deterministic structural check for the
  driver. Do not commit the generated SYS binary.

## Prototype limits deferred to M55

- No Configure GUI and no persistent path.
- No live refresh, directory watching, or automatic remount.
- No save-state snapshot identity or cross-session restoration guarantee.
- No long-file-name, arbitrary Unicode, or generated `~N` aliases.
- No writable mode, delete, rename, metadata update, or host write-back.
- No embedding of host paths or private snapshot identities in tracked files.

## Invariants

- HOSTFAT is disabled unless `--hostfat-dir` is supplied.
- Existing FDD, SASI, BMS, MSE, RDBMS, CPU, SGP, DMA, timing, and state
  behavior is unchanged when disabled.
- The 07EDH/07EFH private protocol is bound in the VA I/O table as well as the
  existing generic I/O table; no real PC-88VA port is repurposed.
- All source remains UTF-8/LF and generated binaries remain outside Git.
- The frozen reference tier is untouched.

## Automated validation

- Deterministic FAT generation including root files and one subdirectory.
- Host local last-write time encoded at FAT two-second resolution for files
  and directories, with defined 1980--2107 clamping.
- Byte-identical regeneration from unchanged input.
- FAT-copy identity, chain, directory, file-content, and capacity checks.
- Fail-closed unsupported-name, collision, link, range, and overflow checks.
- Full 07EDH/07EFH probe and read transaction through the VA I/O table.
- Invalid requests leave the guest destination unchanged.
- CLI parsing, disabled-path regression, reset retention, and unmount checks.
- Reproducible HOSTFAT.SYS assembly and device-header validation.
- Linux GCC/Clang, ASan/UBSan, MinGW/Wine where available, ROM-less selftest,
  smoke, repository invariant checks, and clean final tracked worktree.

## Human gate G54

The maintainer passed this gate for the original generated driver at
`19626dc6be4337eb4666181d0a42f0bfcb2f38ce`. Because the clean-room rewrite
changes every generated driver binary, repeat items 2--8 for the replacement
before beginning M55.

1. Build from a clean checkout and create a host folder containing only ASCII
   8.3 files plus one ASCII 8.3 subdirectory.
2. Build `HOSTFAT.SYS`, place it on a PC-Engine system disk, and add
   `DEVICE=HOSTFAT.SYS` to CONFIG.SYS.
3. Start vaeg with `--hostfat-dir <folder>` and confirm the driver reports
   `HOSTFAT read-only drive ready`.
4. Identify the drive letter assigned after existing block devices; run DIR
   in the root and subdirectory, TYPE a text file, and copy files from HOSTFAT
   to a writable disk or RAM disk.
5. Compare copied contents with the host originals.
   Confirm DIR reports the host last-write timestamps at FAT's two-second
   resolution rather than the fixed date `80-01-01`.
6. Attempt create, overwrite, and delete operations on HOSTFAT; confirm a
   write-protect error and confirm the host folder is byte-for-byte unchanged.
7. Reset the guest and confirm the same snapshot remains readable.
8. Start vaeg without `--hostfat-dir`; confirm HOSTFAT reports unavailable or
   installs zero units and ordinary V3, VA demo, OS, RDBMS, and MSE operation
   remains unchanged.

Stop after presenting this checklist. G54 passes only when the maintainer says
so. Do not begin M55 in this session.
