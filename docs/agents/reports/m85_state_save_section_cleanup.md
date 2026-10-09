# M85: state-save section cleanup and compatibility report

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

## Scope and status

M85 starts from the G84-approved M84 merge at
[9aeb6512](https://github.com/nakatamaho/vaeg/commit/9aeb6512e59da7e794ffede50b7a184f601d137e).
The implementation candidate was completed on
topic/m85-state-save-section-cleanup. G85 human gate passed; M85 was closed
and merged to `main` at
[0b663304](https://github.com/nakatamaho/vaeg/commit/0b6633041e2fb8bae8de7efa1a1768dc6c3e5cba).

The audit covers the current writer table in
[statsave.tbl](../../../statsave.tbl), the preflight/load coordinator in
[statsave.c](../../../statsave.c), and the existing state boundary tests in
[sdl2/selftest.c](../../../sdl2/selftest.c). It distinguishes top-level state
sections from board-specific payload choices embedded in FMBOARD.

## Current section inventory

The current writer emits these sections. UPD9CPU, UPD9Z80, and HOSTFAT
are the literal serialized names behind the corresponding source constants;
CGWINDOW has one of two identical table entries selected at compile time.

| Boundary | Current sections | Disposition |
|---|---|---|
| Container and CPU | PCCORE, UPD9CPU, UPD9Z80, MEMORY, EXTMEM, TERMINATE | Retained. These are the current machine header, uPD9002 runtime/register compatibility images, memory, and terminator. |
| Shared display and timing | ARTIC, CGROM, CGWINDOW, CRTC, CRTC2, EGC, GDC1, GDC2, VRAMCTRL, TEXTRAM, GAIJI, EVENT, CALENDAR, PALEVENT, uPD4990 | Retained. These objects remain initialized or referenced by shared machine paths; removal would change the common state contract rather than merely remove dead 98-only data. |
| Interrupt, I/O, and input | DMAC, FDC, EMSIO, PIT, MOUSE, NECIO, NP2SYSPORT, PIC, RS232C, SYSTEMPORT, KEYSTAT | Retained. VA FDC/DMA/PIC/PIT/input paths and common initialization/reset still depend on these objects. M80 explicitly retained the EMSIO/NECIO boundary. |
| MIDI and communication | MPU98II, CMMPU98, CMRS232C | Retained. The current build still initializes the MPU/communication objects and M72/M76 preserve the sound and MIDI boundary. |
| Storage and sound | DISK, FMBOARD, BEEP, MUSICGEN, SASI, SCSI | Retained. FDD, SASI, SCSI, HOSTFAT-adjacent state, and VA OPN/OPNA behavior remain active. FMBOARD receives the compatibility tightening described below. |
| BMS | BMSMEM, BMSIO | Retained. The VA BMS configuration and mapped window remain current functionality. |
| VA video and memory | TVRAMVA, GVRAMVA, BKUPMEMVA, MEMORYVA, SYSPORTVA, VIDEOVA, GACTRLVA, TSP, CGROMVA, SGP, MOUSEVA | Retained. These are the VA display, memory, TSP/SGP, and input state boundary. |
| VA subsystem and boards | SUBSYSIF, SUBSYS, SUBCPU, BOARDSB2, VA91 | Retained. The FDC-facing uPD780 subsystem and VA Sound Board II state are current paths. |
| Host integration | HOSTFAT v1, 36-byte payload | Retained with its existing identity and mounted/unmounted checks. |

No additional top-level section is proven to be a retired 98-only section after
the M80, M81, and M84 closures. In particular, the presence of a common
GDC/CRTC/EGC object in the table is not evidence that it can be removed:
the common renderer and initialization path still own those objects, while the
VA renderer has separate VA state sections.

## Retired state and compatibility boundary

The following are not current writer sections:

- CPU286 is an obsolete predecessor CPU section. The existing narrow
  transitional loader bridge remains: a CPU286 payload is accepted only with
  the current UPD9002 format marker; CPU286-only or wrong-identity state is
  rejected before mutation.
- NMIIO was removed by M80. The generic loader policy accepts an unknown
  section with a warning and skips it, so an old state containing NMIIO is
  not silently treated as current NMIIO state.
- HOSTDRV was removed by M72 and is unrelated to the retained HOSTFAT
  snapshot section. HOSTFAT keeps its strict version, size, mounted-state,
  and identity checks.
- AMD98, PCM86, CS4231, and the deleted C-bus board implementations did
  not have independent current top-level tags. Their old data was selected by
  FMBOARD's embedded usesound value. M84 removed the old implementations
  and payload branches.

M85 makes the last boundary explicit. FMBOARD preflight now permits only the
current values 0x0000, 0x0001, 0x0020, 0x0040, 0x0100 (VA1 OPN), and
0x0200 (VA2 OPNA). It validates the payload size against the current writer
layout. The removed old values 0x0002, 0x0004, 0x0006, 0x0008, 0x0014,
and 0x0080 fail with FMBOARD state uses retired sound hardware.

This is intentional incompatible-state rejection, not migration. The check is
performed by statsave_check() before statsave_load() opens the live-state
load pass. A retired FMBOARD state therefore cannot reset the sound board and
then appear to load successfully, and a malformed current FMBOARD payload
cannot be accepted by version alone.

## Verification

The implementation is
[a6493d2](https://github.com/nakatamaho/vaeg/commit/a6493d2f481a3744f528d1a19d5ee3663df59e90).
The selftest creates a disposable current state, changes only the FMBOARD
usesound word to retired value 0x0004, and requires:

- statsave_check() failure with the retired-hardware diagnostic;
- statsave_load() failure;
- unchanged CPU_IP and memory at 0400H;
- normal save/check/load/save round-trip success;
- existing HOSTFAT identity mismatch and explicit override behavior unchanged.

The following candidate checks passed:

| Check | Result |
|---|---|
| cmake --preset linux-ci-gcc | PASS |
| CCACHE_DISABLE=1 cmake --build --preset linux-ci-gcc -j4 | PASS |
| SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-gcc/sdl2/vaeg --selftest | PASS; all tests passed |
| Same selftest with --model va | PASS; all tests passed |
| ctest --test-dir build/linux-ci-gcc --output-on-failure -R '^(vaeg_romless_tests|vaeg_m75_transfer_info_compiled)$' | PASS, 2/2 |
| python3 tools/repo/check_encoding.py | PASS |
| python3 tools/repo/check_eol.py | PASS |
| python3 tools/repo/check_case.py | PASS; 0 findings |
| cmake --preset mingw-cross; CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4 | PASS |
| MinGW artifact | PASS; PE32+ build/mingw-cross/sdl2/vaeg.exe; SHA-256 8ac93c914b719a7968e11b527f73ea5ab9311f543683eba9a8e9cc6937095872 |
| M84/M85 source inventory and diff checks | PASS; no state payload binaries changed |

G85 human gate passed; the maintainer confirmed the required manual
clean-checkout, V3/demo, OS, and save/load validation.

## Non-goals and deferred items

M85 does not remove common display/I/O sections merely because they originated
in the historical PC-98 implementation. It does not change VA device
dispatch, state ordering, HOSTFAT identity policy, or the active VA sound
layouts. Any future removal of retained common sections requires a focused
dependency audit and a separate milestone.
