<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
--->

# M81: VA BIOS reachability audit

## Status

This is the final M81 report. The audit starts from the approved M80
checkpoint at [`c44569bd8c47c87c19c6e59bfb735ce7431102bd`](https://github.com/nakatamaho/vaeg/commit/c44569bd8c47c87c19c6e59bfb735ce7431102bd).
The approved cleanup removes the VA-facing INT18h-INT1Fh common BIOS entries
and the common GDC/LIO compatibility layer. VA display code is provided by
`biosva/`, `vramva/`, and `videova/`; no active VA path requires
`bios/bios18.c` or `lio/`. It retains the FDD bootstrap/equipment helpers, the
FDD wait hook, and the shared SCSI/SASI backend used by bootstrap and C-Bus
paths. G81 human gate passed on 2026-08-11.

## What Tekumani documents

`docs/tekumani/600INDEX.TXT` and `2.TXT` describe the VA public BIOS family.
The documented primary vectors include INT80h FDD, INT81h HDD, INT82h
keyboard, INT83h text, INT84h sprite, INT87h extended graphics, INT88h
animation, INT89h printer, INT8Ah communication, INT8Bh sound, INT8Ch
calendar-clock, INT8Dh Japanese FEP, INT8Eh line editor, and INT8Fh
graphics-screen control. The vector map also lists INT92h font, INT93h
popup, INT94h screen editor, INT97h BASIC internal, INT98h editor, INT9Eh
monitor/debugger, INT9Fh shell, and the numeric services INT A0h-A8h and
INT B0h-B7h.

## Current BIOS entries not listed as those Tekumani VA vectors

The following are the requested not-listed entries. The right-hand column is
the closest documented VA service where one exists; it is not an assertion
that the implementations are ABI-compatible.

| Current entry | Current role | Closest Tekumani VA entry | M81 decision |
| --- | --- | --- | --- |
| `BIOSOFST_09` / `bios0x09` | Common keyboard service | INT82h keyboard | Retain; common compatibility path is active and the VA keyboard path is separate. |
| `BIOSOFST_0c` / `bios0x0c` | Common serial interrupt service | INT8Ah communication | Retain; not proven 98-only. |
| `BIOSOFST_12` / `bios0x12` | FDC service/result path | INT80h FDD | Retain; FDC is an active VA dependency. |
| `BIOSOFST_13` / `bios0x13` | FDC service/result path | INT80h FDD | Retain; FDC is an active VA dependency. |
| `BIOSOFST_18` / `bios0x18` | Common text/graphics dispatcher and GDC display helpers | VA1/VA2 final IVTs use a common default target for INT18h | Remove the dispatcher, helpers, and LIO compatibility path; VA display code remains in the VA-specific renderer. |
| `BIOSOFST_19` / `bios0x19` | Common RS-232C dispatcher | VA1/VA2 final IVTs use a common default target for INT19h | Remove the unused dispatcher and source file; retain the separate active VA serial path. |
| `BIOSOFST_CMT` / `bios0x1a_cmt` | Cassette subentry | VA1/VA2 final IVTs use a common default target for INT1Ah | Remove the unused cassette subentry. |
| `BIOSOFST_PRT` / `bios0x1a_prt` | Common printer subentry | VA1/VA2 final IVTs use a common default target for INT1Ah | Remove the unused printer subentry. |
| `BIOSOFST_1b` / `bios0x1b` | Common disk dispatcher | VA1/VA2 final IVTs use a common default target for INT1Bh | Remove the INT1Bh dispatcher and its legacy subdispatch; retain bootstrap, equipment, wait, and shared storage helpers. |
| `BIOSOFST_1c` / `bios0x1c` | System and interval timer service | VA1/VA2 final IVTs use a common default target for INT1Ch | Remove the unused timer subentry. |
| `BIOSOFST_1f` / `bios0x1f` | Extension/memory-move service | VA1/VA2 final IVTs use a common default target for INT1Fh | Remove the unused extension subentry. |

Thus, the M81 cleanup set is INT18h-INT1Fh: the common display/GDC and LIO
path, INT19h serial dispatcher, INT1Ah cassette/printer subentries, INT1Bh
disk dispatcher, INT1Ch timer subentry, and INT1Fh extension subentry. The
INT1Ah notation follows the former implementation names `bios0x1a_cmt` and
`bios0x1a_prt`; these were simulated common BIOS subentries, not claims about
a Tekumani VA vector number. INT09h, INT0Ch, INT12h, and INT13h remain
outside this cleanup.

## Emulator-internal hooks also absent from the manual

These are not additional public BIOS APIs, but they are current emulator
interception points that do not appear in the Tekumani public vector list:

- simulated reset and boot entries at `FD80:0080` and `FD80:0084`;
- the FDD wait entry at `FD80:00B4`;
- bootstrap-load interception at physical `0xFFFE8` and `0xFFFEC`; and
- the ITF-bank early-return rule in `biosfunc()`.

Current VAEG additions also outside the Tekumani public BIOS description are
the M75 SCSI backend extensions and the read-only HOSTFAT driver path. HOSTFAT
uses its own VA port/driver protocol; its `INT83h` is a driver interrupt and
must not be confused with the Tekumani text BIOS INT83h.

## Reachability decision

The complete VA1 IVT dump at `0000:0000`-`0000:03FF` shows INT18h-INT1Fh
(offsets `0x60`-`0x7F`) all pointing to `F000:19A5`; the supplied VA1 dump of
that target begins with `CF` (`IRET`). The complete VA2 IVT dump shows the
same eight entries all pointing to the common default target `F000:2329`. The active
VA-specific services are in the separate INT80h-and-above map; the
common INT18h-INT1Fh entries do not provide an active VA service in either
final IVT.

The raw VA1/VA2 ROMs contain byte encodings that resemble calls to some of
these legacy services. That is lower-bound byte-scan evidence, not proof
that the final VA interrupt vectors reach them. The stronger runtime
evidence here is the complete final IVT and the default IRET target. The
user-approved correction therefore removes these eight common interrupt
entries while retaining internal routines that have independent callers.

The simulated BIOS resource table in `bios/biosfd80.res` maps all eight
INT18h-INT1Fh slots to the default IRET stub at offset `0x015A`; the fixed
vector slots remain present but have no active handler. `biosfunc()` no longer
dispatches these offsets. The common display helpers, LIO dispatcher, LIO ROM
payload, and the `0xF9950`-`0xF9994` interception were removed. The assembly
source keeps only NOP reservation through `0x00B4` so the FDD wait entry keeps
its fixed offset. The standalone INT19h, INT1Ah, INT1Ch, and INT1Fh handlers
were also removed. In `bios1b.c`, only the INT1Bh dispatcher and its
unreachable legacy subdispatch were removed; `fddbios_equip()`,
`bootstrapload()`, `bios0x1b_wait()`, and `boot_hd()` remain. This keeps the
shared storage backend used by bootstrap and C-Bus SCSI paths out of the
cleanup.

## Source evidence

- [`bios/bios.h`](../../../bios/bios.h) defines the common offsets and roles.
- [`bios/bios.c`](../../../bios/bios.c) installs and dispatches the simulated
  BIOS, bootstrap hooks, and wait hook.
- [`pccore.c`](../../../pccore.c) invokes common BIOS initialization during reset.
- [`io/iocore.c`](../../../io/iocore.c) and [`io/np2vasup.c`](../../../io/np2vasup.c)
  show the runtime common/VA I/O-map boundary.
- [`io/fdc.c`](../../../io/fdc.c) contains the active VA FDC path.
- [`bios/bios1b.c`](../../../bios/bios1b.c) and [`bios/sxsibios.c`](../../../bios/sxsibios.c)
  contain the active disk/SASI/SCSI boundary.
- [`bios/biosfd80.res`](../../../bios/biosfd80.res) records the simulated
  INT18h-INT1Fh vector targets.
- [`io/serial.c`](../../../io/serial.c) contains the VA keyboard and RS-232C
  bindings.
- [`romimage/bios/biosfd80.asm`](../../../romimage/bios/biosfd80.asm) and
  [`romimage/makefile.w32`](../../../romimage/makefile.w32) retain the fixed
  FDD wait layout while removing the obsolete INT18h-INT1Fh and LIO build
  inputs.
- [`docs/tekumani/600INDEX.TXT`](../../tekumani/600INDEX.TXT) and
  [`docs/tekumani/2.TXT`](../../tekumani/2.TXT) are the read-only VA manual
  sources used for the vector comparison.

## Validation

Repository checks run on the M81 source-cleanup candidate:

```text
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/qa/upd9002_rename.py
git diff --check
cmake --preset linux-debug
cmake --build --preset linux-debug --clean-first -j4
build/linux-debug/sdl2/vaeg --selftest
ctest --test-dir build/linux-debug --output-on-failure
```

The Linux Debug source-cleanup build completed successfully. `build/linux-debug/sdl2/vaeg --selftest` exited 0 with `selftest: all tests passed`. `ctest --test-dir build/linux-debug --output-on-failure` exited 0 and reported no tests found. The repository encoding, EOL, case, uPD9002 rename, and diff checks all passed after the cleanup commit. The build emitted only pre-existing warnings and linker warnings. A macOS Cocoa VA smoke launch was previously attempted, but the headless environment failed before guest execution with the platform appearance error `SystemAppearance not found`; this is an environment limitation, not a BIOS pass. The G81 human gate passed on 2026-08-11. M81 is closed.
