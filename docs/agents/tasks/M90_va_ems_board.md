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

# M90: Enable the VA EMS board

Status: **G90 human gate passed; M90 closed**

Predecessor: G89 passed; M89 is integrated into `main` at
`c65853cfd2f5ff5318c1a11fec384961037bfdbb`.

Branch: `topic/m90-va-ems-board`

Commit prefix: `M90:`

Gate result: `G90` passed on 2026-08-13 at
[`f16d6af14039359e5b617d757f906a47f45b1ad9`](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9).

Implementation candidate:
[`498b283b67bc0e68dcd6f507260e190974c07f9f`](https://github.com/nakatamaho/vaeg/commit/498b283b67bc0e68dcd6f507260e190974c07f9f)

Validation record:
[`reports/m90_va_ems_board.md`](../reports/m90_va_ems_board.md)

## Goal

Restore the retained EMS page-frame implementation as a configurable PC-88VA
expansion board, provide a reproducible Open Watcom build of a PC-88VA SQEMM
manager, and generate a supplemental disk with the complete EMMVA/SQEMM98/
RDEMS load stack and `CONFIG.SYS` on a bootable PC-Engine 1.1 disk.

## Required behavior

- Expose `Device / EMS Board...` immediately below `I/O Bank Memory...`.
- Keep the existing persisted `ExMemory` key and state-save sections.
- Treat `ExMemory=0` as disabled and values 1 through 13 as installed
  capacity in 1MB units. A clean configuration retains the existing 1MB
  default.
- Apply changed EMS capacity through the normal guest-reset path. Cancel must
  leave the configuration and running machine unchanged.
- Attach the retained `08E1H`, `08E3H`, `08E5H`, `08E7H`, and `08E9H`
  EMS interface to the active VA I/O dispatcher and compatibility dispatcher.
- Preserve four 16KB page-frame windows at `C0000H`, `C4000H`, `C8000H`,
  and `CC000H`; target zero restores the ordinary memory mapping.
- Keep EMS and I/O Bank Memory independently configurable.
- Extend the ROM-less selftest for disabled behavior, VA I/O reachability,
  target/page mapping, capacity bounds, reset mapping, and configuration
  round trip.
- Update `tools/pc88va/build-softlib-archive-disk.sh` to retain the verified
  EMMVA15A and RDEMS152 archives and extract their redistributable drivers and
  documentation to appropriate supplemental-disk directories.
- Build SQEMM98 MAX from pinned upstream source with the pinned Open Watcom
  image. Implement the VA target/page protocol, preserve 1 through 13MB
  capacity detection, and validate the generated DOS character device.
- Route all SQEMM98 initialization, success, and error messages through the
  PC-Engine Text BIOS `INT 83H/AH=02H` service. Do not use IBM `INT 10H` or
  DOS `INT 21H/AH=09H` for those messages.
- Install `EMMVA01.SYS`, `SQEMM98.SYS`, `EMMVA02.SYS`, and `RDEMS.SYS` on
  the supplemental disk and create root `CONFIG.SYS` in that exact load
  order. Retain the source PC-Engine 1.1 IPL and four fixed system files so
  that the generated supplemental disk remains bootable.
- Build media only from a disposable copy of the maintainer-supplied
  PC-Engine 1.1 source D88; never modify or track the source D88 or generated
  media.
- Document the hardware model, EMMVA/SQEMM98 installation order, RDEMS
  dependency, source links, Open Watcom build, and generated contents in
  `docs/modernization/pc88va-utility-media.md`.
- Generate a redistributable HOSTFAT/SQEMM98 guest-driver bundle in CI and
  include the same drivers, licenses, instructions, and checksums in every
  normal binary artifact and tagged release package.

## Non-goals and invariants

- Do not add 128KB EMS-capacity increments; M90 uses 1MB units.
- Do not bundle or download a commercial EMM manager such as EMM4J or MELEMM.
  EMMVA remains an adapter; the generated SQEMM98 driver fills the manager
  position between its two components.
- Do not add an emulator-private EMS API, BIOS service, or direct guest-memory
  injection path.
- Do not alter BMS numbering, main-memory capacity, binary payloads, CPU
  semantics, or unrelated storage and display behavior.
- Do not synthesize or relocate PC-Engine system files. Preserve the verified
  boot-only source layout and install the supplemental payload around it.

## Commit order

1. M90 task and ROADMAP definition.
2. EMS core/VA-I/O connection, GUI/configuration, and automated tests.
3. Supplemental-disk builder and EMS software-environment documentation.
4. M90 validation record and gate handoff.
5. SQEMM98 Open Watcom build, PC-Engine BIOS output, generated `CONFIG.SYS`,
   and refreshed validation record.
6. HOSTFAT/SQEMM98 CI and release-package distribution.

## Automated validation

- Run the ROM-less selftest and focused EMS configuration/mapping checks.
- Generate the supplemental disk from a disposable source-D88 copy, validate
  its FAT contents and extracted `CONFIG.SYS`, and compare the source D88
  before and after.
- Build SQEMM98 twice, verify byte-for-byte reproducibility, and run the
  structural driver validator. Confirm one PC-Engine `INT 83H` output path,
  no IBM `INT 10H`, and no DOS `AH=09H` output path.
- Build and validate the HOSTFAT/SQEMM98 distribution bundle from source, then
  validate the staged Linux, macOS, and Windows artifact layouts and tagged
  release-package layouts.
- Run Linux Debug and CI builds/tests plus the MinGW cross-build.
- Run repository invariant and diff checks.
- Confirm commit scope/order and that no generated binary asset is tracked.

## Human gate G90

From a clean checkout and clean configuration:

1. Complete the standard VA gate: V3 mode, bundled demo, OS boot, and simple
   operations.
2. Confirm EMS Board appears below I/O Bank Memory, defaults to 1MB, accepts
   1 through 13MB, persists, and resets only on applying a change.
3. Boot the generated supplemental disk and confirm that SQEMM98 messages
   appear through PC-Engine.
   Confirm the configured capacity and distinct data in more than one 16KB
   page.
4. Confirm RDEMS152 loads after SQEMM98 and supports RAM-disk read/write.
5. Enable I/O Bank Memory concurrently and confirm both mechanisms work.
6. Disable EMS Board and confirm normal V3/OS operation remains intact.

The maintainer explicitly reported G90 passed on 2026-08-13 against the final
M90 history at
[`f16d6af14039359e5b617d757f906a47f45b1ad9`](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9).
M90 is closed.
