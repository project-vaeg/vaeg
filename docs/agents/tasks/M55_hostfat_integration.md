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
# M55: HOSTFAT GUI, save-state identity, and safety finish

Status: **implementation and automated validation complete; G55 pending**

The maintainer accepted the clean-room M54 replacement at
`e0bafbaa3cc0b12f945e18c231c843fc17ff0392`. M55 starts exactly from that
approved SHA.

## Goal

Turn the accepted M54 read-only block-device prototype into a persistent,
reviewable frontend feature while preserving its immutable snapshot and
write-protect contract. Expand the fixed image to the practical FAT12 limit
without changing the 16-bit PC-Engine block-request LBA contract.

## Authorized scope

- Use this fixed FAT12-max geometry:
  - 1024 bytes per logical sector;
  - 16 sectors per cluster (16 KiB);
  - zero reserved sectors, two seven-sector FAT copies, and 128 root entries;
  - 65,362 DOS-visible sectors in a 65,536-sector backing image;
  - 4,084 DOS-visible data clusters, remaining below the 4,085-cluster FAT16
    threshold;
  - allocation only through cluster `0FEFH`, with the six geometrically
    present tail clusters marked reserved in both FAT copies.
- Report both capacities accurately: the DOS-visible volume is 66,930,688
  bytes (63.830078125 MiB), while at most 66,813,952 bytes (63.71875 MiB) of
  cluster payload can be allocated before directory and per-file rounding.
- The G55 human run disproved the original 2048-byte-sector/32 KiB-cluster
  proposal: a 96 KiB file occupied three FAT entries but PC-Engine copied only
  6144 bytes. Use the 1024-byte-sector/16 KiB-cluster geometry demonstrated by
  the PC-88VA 40 MB SASI layout. The corrected HOSTFAT copied all 96 KiB
  byte-identically and read a marker allocated beyond 60 MiB.
- Document that unpatched PC-Engine reports free space using 2 KiB per free
  FAT entry even though 16 KiB cluster reads work. Its approximately 8 MiB
  `DIR` figure is therefore not the HOSTFAT readable-capacity limit.

- Add an enable switch and host-folder picker to Configure.
- Persist the setting using the portable executable-local/user-state policy.
- Build snapshots without freezing the SDL2/ImGui UI and report progress and
  precise failures.
- Define an explicit unmount/rebuild/reset refresh workflow; never expose a
  changing FAT view to a mounted guest.
- Give each snapshot a deterministic identity and define transactional
  save-state load behavior for matching, missing, and mismatched snapshots.
- Show every state-load rejection in a persistent modal. Keep strict identity
  matching as the default, but permit an explicit, warned `Force load` only
  when bypassing the HOSTFAT identity check makes the remaining preflight
  acceptable. The override retains the current mount state and read-only
  snapshot; it does not restore or silently substitute the saved snapshot.
- Complete canonical-root containment, symlink/reparse-point rejection,
  race-resistant file-copy validation, depth/count/size limits, and diagnostics.
- Add deterministic 8.3 alias generation and an explicitly documented policy
  for host Unicode names that cannot be represented safely.
- Add GUI, state, security, cross-platform, and human regression coverage.

## Hard non-goals

- No host writes, delete, rename, or metadata updates.
- No INT 2FH redirector or dependency on MS-DOS SDA/CDS internals.
- No live sector synthesis from a directory that can change while mounted.
- No silent state substitution when snapshot identity does not match. The
  maintainer-approved, explicitly confirmed `Force load` is the sole override.
- No further geometry change without reporting the observed PC-Engine
  behavior and obtaining maintainer approval.

## Planned G55 human gate

- Configure, persist, restart, rebuild, unmount, and reset the host drive.
- Confirm UI responsiveness during snapshot creation.
- Confirm PC-Engine accepts the 1024-byte-sector FAT12-max BPB and can DIR,
  TYPE, and COPY a file spanning more than one 16 KiB cluster.
- Confirm a marker allocated after at least 60 MiB of preceding file data can
  be copied byte-identically. Note the known approximately 8 MiB free-space
  display rather than treating it as the readable capacity.
- Confirm matching save-state continuation, fail-closed mismatch handling,
  visible rejection feedback, cancel-without-mutation, and the warned explicit
  override while retaining the current read-only HOSTFAT snapshot.
- Repeat root/subdirectory reads and host write-protection checks on Linux and
  Windows, followed by the standard V3/VA-demo/OS gate.
