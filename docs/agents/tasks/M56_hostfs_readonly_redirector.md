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
# M56: Read-only HOSTFS DOS redirector

Status: **G56 administratively closed because the prerequisite is absent; no
HOSTFS implementation was completed**

M56 starts from the G55-approved SHA
`df47b5f829d7b8cc9c02f45d9a00e16c4b43dad4` on
`topic/m56-hostfs-readonly-redirector`.

## Pre-implementation gate result

The original task assumed that PC-Engine DOS forwards public DOS redirection
functions to the `INT 2FH/AH=11H` network-redirector multiplex. The
non-resident M56 probe disproved that assumption in the accepted emulator
environment:

```text
DOS=02.00
INT2F/1100 before hook AX=1100 CF=0
INT2F/1100 after hook AX=11FF CF=0
INT21/5D06 AX=0001 CF=1
INT21/5F02 AX=0001 CF=1 CALLS=0000 LAST=0000 STACK=0000
INT21/5F03 AX=0001 CF=1 CALLS=0000 LAST=0000 STACK=0000
RESULT=NO_DOS_REDIRECTOR_BRIDGE
```

The probe temporarily installed an `AH=11H` handler, made a read-only
redirection-list query and a deliberately rejected assignment request, then
restored the original vector. Neither DOS call reached the handler. No drive,
CDS entry, resident code, host path, or guest-media write was created.

Consequently the `HOSTFS.COM` implementation described below cannot provide
transparent `DIR`, `TYPE`, or program loading on this PC-Engine environment.
M56 stops at the evidence gate. Implementing or patching a replacement DOS
file-service bridge, intercepting PC-Engine internals, or changing to a
non-transparent utility protocol is a materially different task and requires
explicit maintainer approval before work begins. The detailed evidence is in
[`m56_pcengine_redirector_probe.md`](../research/m56_pcengine_redirector_probe.md).

On 2026-07-23 the maintainer administratively closed G56 at approved SHA
`b72e641733ddea6f0e8faef2507093f7c3aee5a4`. The closure accepts the
disposition "M56 stopped because the prerequisite is absent." It is not
evidence of a successful HOSTFS implementation and does not authorize an
alternate file-service bridge.

## Goal

Add an independently authored, two-clause-BSD `HOSTFS.COM` DOS redirector
that exposes a selected host directory as a read-only DOS drive through
file-level operations. Unlike HOSTFAT, HOSTFS must not synthesize FAT, BPB,
cluster, or sector data. A directory rescan may make newly added host files
visible without rebuilding a disk image or resetting the guest.

This goal remains the historical requested design. It is not implementable
through the authorized interface under the probe result above and is not an
active authorization to bypass the evidence gate.

## Clean-room and licensing boundary

- New M56 source files carry the repository two-clause-BSD header and are
  independently authored from this task, the M56 protocol specification, and
  cited public DOS interface documentation.
- Do not copy, translate, disassemble, structurally imitate, or derive code
  from dormant `np2tool/hostdrv.asm`, `generic/hostdrv.c`,
  `generic/hostdrvs.c`, or `generic/hostdrv.tbl`.
- Those historical files remain unmodified and are not licensing evidence.
  They may establish only the factual observation that old NP2 trees once
  offered a host-drive feature.
- Do not use another emulator implementation as proof of DOS behavior.

## Authorized implementation

- Add a TSR-style `HOSTFS.COM` for PC-Engine/MS-DOS. It hooks the DOS network
  redirector multiplex (`INT 2FH`, `AH=11H`) only after validating the DOS
  environment required by its independently documented structure adapter.
- Support a conservative read-only subset sufficient for ordinary DOS
  `DIR`, `TYPE`, program load, and copy from HOSTFS to writable guest media:
  installation query, drive/current-directory queries, open, read, seek,
  close, get attributes, find-first/find-next, and disk-space reporting.
- Chain every request outside the exact owned drive and supported function
  set. Mutating operations on the owned drive return DOS write-protect or
  access-denied without reaching the host filesystem.
- Add a versioned bounded RPC over the existing private `07EDH`/`07EFH`
  transport. Packets contain only fixed-width little-endian values and DOS
  relative paths; no guest pointer is retained after a transaction.
- The frontend host service uses canonical-root-relative paths, rejects
  absolute paths, drive prefixes, `.`/`..`, malformed encodings, symlinks,
  reparse points, hard-link aliases that escape policy, non-regular files,
  and every result that cannot be represented safely in DOS 8.3 form.
- Host access is read-only. No create, write, truncate, rename, delete,
  directory creation/removal, timestamp update, or attribute update path may
  be present in the host backend.
- Bound path length, entry count, open-handle count, enumeration count, read
  size, packet size, and all guest-memory ranges. Invalid input fails closed.
- Keep stable emulator-owned tokens rather than host pointers or native
  handles in guest-visible data.
- Add deterministic tests for protocol parsing, containment, 8.3 mapping,
  handle lifetime, directory refresh, read bounds, mutation rejection, and
  malformed guest requests.

## HOSTFAT coexistence and UI policy

- Retain all accepted HOSTFAT implementation, command-line options,
  configuration keys, save-state sections, strict/forced load policy, and
  generated `HOSTFAT.SYS` for compatibility. No HOSTFAT deletion is
  authorized.
- When HOSTFS is built and its integration checks pass, show HOSTFS in the
  normal Configure UI and hide the legacy HOSTFAT controls there. Existing
  HOSTFAT CLI/configuration remains honored and must not be silently migrated.
- HOSTFS and HOSTFAT must not claim the same configured drive concurrently.
  Startup/configuration conflicts fail with a clear diagnostic before guest
  execution.

## Save-state policy

- Save no native host handle, pointer, absolute host path, or private host
  metadata.
- Serialize only versioned logical HOSTFS state needed for deterministic
  guest continuation: mount identity, DOS drive, open relative paths and
  offsets, and active enumeration cursors/identities.
- State load preflight reopens and validates every active logical object in
  temporary storage. Any missing or changed active object rejects the entire
  load before live guest or HOSTFS state changes.
- A mounted but idle HOSTFS may accept unrelated host additions; additions
  alone must not invalidate a state that has no active dependency on them.
- M56 does not add a force-load bypass for active HOSTFS object mismatches.

## Hard non-goals

- No SMB/CIFS wire protocol, emulated NIC, packet driver, or server process.
- No writable host sharing.
- No FAT snapshot generation inside HOSTFS.
- No raw `INT 25H`/`INT 26H` sector interface, CHKDSK compatibility, volume
  formatting, or block-device semantics.
- No modification of the frozen reference tier or dormant legacy HOSTDRV.
- No unrelated CPU, timing, sound, video, media, or state changes.

## Historical implementation validation

The following validation list applied to the requested HOSTFS implementation.
It is not runnable while the prerequisite bridge is absent.

- Reproducible NASM build and deterministic structural check for
  `HOSTFS.COM`.
- GCC, Clang, ASan/UBSan, Linux release, and MinGW release builds and CTest.
- Wine selftest where available; tests-disabled production-symbol check.
- ROM-less transport and backend tests, including malformed packets and
  complete mutation rejection.
- PC-Engine human checks for installation, `DIR`, `TYPE`, executable load,
  copy-out, subdirectories, directory refresh, and write protection.
- HOSTFAT CLI/config/save-state regression checks while its normal GUI is
  hidden.
- Standard V3, bundled VA demo, OS, keyboard, FDD, sound, save/load, and reset
  human gate.
- Repository encoding, EOL, case, unreferenced-file, and diff checks.

## Unexecuted HOSTFS implementation checklist

This implementation checklist was not executed. G56 was administratively
closed because the prerequisite bridge is absent, not because the requested
HOSTFS implementation or its human validation succeeded.

1. Build from a clean checkout and copy `HOSTFS.COM` to writable guest media.
2. Boot PC-Engine with `LASTDRIVE` high enough, run `HOSTFS <drive>`, and
   confirm one successful installation with no HOSTFAT success banner.
3. Confirm `DIR`, `TYPE`, program loading, and copy from HOSTFS to FDD/HDD.
4. Add a host file, rescan/reopen the directory, and confirm it appears
   without rebuilding a FAT image or resetting vaeg.
5. Confirm create, overwrite, rename, delete, mkdir, rmdir, and metadata
   changes fail and do not change the host tree.
6. Save/load with no active HOSTFS handle, then with an active file or search;
   confirm exact match succeeds and changed active dependencies reject before
   guest state mutation.
7. Confirm Configure shows HOSTFS but not the legacy HOSTFAT controls, while
   an existing `--hostfat-dir` invocation still works.
8. Complete the standard V3/VA demo/OS/keyboard/FDD/sound/reset gate.

The requested design stopped at the failed prerequisite evidence gate. The
administrative G56 closure does not approve this unimplemented design.
