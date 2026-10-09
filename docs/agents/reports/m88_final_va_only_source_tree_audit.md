<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

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
-->

# M88: final VA-only source-tree audit

## Status

M88 source cleanup is recorded in
[2fe49c944797ca8508c3cfc53ed39ffdef5014b0](https://github.com/nakatamaho/vaeg/commit/2fe49c944797ca8508c3cfc53ed39ffdef5014b0)
on `topic/m88-final-va-only-source-tree-audit`. The candidate is not merged
to `main`; G88 human validation passed against candidate
[98d7343df9c763354e0775bd04a7b6d8d9c6a291](https://github.com/nakatamaho/vaeg/commit/98d7343df9c763354e0775bd04a7b6d8d9c6a291). M88 is closed;
main merge remains pending.

This report records source/configuration cleanup only. No ROM, font, icon,
disk-image, wave-data, or other binary payload was modified.

## Decisions and deletion boundary

| Area | Decision |
|---|---|
| Old VM/VX configuration | Removed the old model strings and legacy model selection behavior. `pc_model` now exposes the VA1/VA2 choices; unknown legacy values fall back to the VA2 configuration. The unreachable non-VA branches in FDC, PIT, serial keyboard conversion, TSP binding, and runtime information were removed. |
| GDC/CRTC configuration | Removed the serialized non-VA display keys `VRAMwait`, `DispSync`, `Real_Pal`, `RPal_tim`, `uPD72020`, `GRCG_EGC`, `color16b`, `skipline`, `skplight`, and `LCD_MODE`. The corresponding fields remain only as explicitly marked struct padding where positional configuration/state layout still depends on them. |
| GDC/CRTC display implementation | Removed `io/crtc.c`, `io/crtc.h`, `io/gdc.c`, `io/gdc.h`, `io/gdc_cmd.h`, `io/gdc_cmd.tbl`, `io/gdc_pset.c`, `io/gdc_pset.h`, `io/gdc_sub.c`, and `io/gdc_sub.h`. Removed the generic VRAM renderer and timing/palette files under `vram/`: `dispsync`, `makegrph`, `maketext`, `maketgrp`, `palettes`, `scrnbmp`, `scrndraw`, `scrnsave`, `sdraw`, and `sdrawq16`. The active display path is `vramva/`. |
| FM7/X1/X68K fonts | Removed the unused backends in `font/fontfm7.c`, `font/fontx1.c`, and `font/fontx68k.c` in the earlier M88 commits [3fc0c0e](https://github.com/nakatamaho/vaeg/commit/3fc0c0e45b93b848d76685471f70075cd63b066c) and [754b421](https://github.com/nakatamaho/vaeg/commit/754b421576dd7de0201ed1ff7aa1db7a96e64cfa). The remaining PC-88, PC-98, V98, and VA `98font.rom` paths are retained. |
| MPU98II | Retained. [cbus/mpu98ii.c](../../../cbus/mpu98ii.c) remains in the CMake target; `MPU98II` and `CMMPU98` remain in [machine/statsave.tbl](../../../machine/statsave.tbl). |
| SASI/SCSI/FDD/HOSTFAT | Retained. The active C-bus storage sources and state-save sections remain in CMake and [machine/statsave.tbl](../../../machine/statsave.tbl). |

The old display source deletion is intentionally separate from the retained
CPU memory compatibility layer. [io/vramcompat.h](../../../io/vramcompat.h)
contains only the small GRCG-compatible tile state and display-dirty state
needed by [cpu/upd9002/memory.c](../../../cpu/upd9002/memory.c) and
[cpu/upd9002/egcmem.c](../../../cpu/upd9002/egcmem.c). It is not a GDC command processor, renderer, timing
engine, or configurable display backend. `NEVENT_GDCSLAVE` remains only as a
reserved event number so later event IDs and save-state compatibility do not
shift; no GDC slave callback is registered.

## Retained active ownership

- [machine/pccore.c](../../../machine/pccore.c) initializes the VA BIOS, VA
  TSP/SGP/video path, FDD path, and active VA event callbacks.
- [vramva/](../../../vramva/) owns the rendered text, sprite, and graphics
  surfaces. The generic `vram/` backing memory remains because the CPU memory
  layer and EGC-compatible operations use it; the deleted files were the
  obsolete generic renderer, not the backing storage.
- [font/](../../../font/) retains only the PC-88/PC-98/V98 loaders and the
  VA-compatible font memory layout. `98font.rom` remains available from the
  Screen -> Font menu.
- [cbus/mpu98ii.c](../../../cbus/mpu98ii.c), [cbus/sasiio.c](../../../cbus/sasiio.c),
  [cbus/scsiio.c](../../../cbus/scsiio.c), and [cbus/scsicmd.c](../../../cbus/scsicmd.c)
  remain active storage/device paths.
- The FDC, HOSTFAT, keyboard, mouse, sound, and state-save paths remain in
  the active CMake graph and were not removed by this cleanup.

## Validation

| Check | Result |
|---|---|
| `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null git diff --check` | PASS |
| `tools/repo/check_encoding.py --expect utf8` | PASS; 0 violations |
| `tools/repo/check_eol.py --enforce` | PASS; 0 violations |
| `tools/repo/check_case.py` | PASS; 0 findings |
| Linux Debug configure/build | PASS; `sdl2/vaeg` linked |
| Linux CI Clang configure/build | PASS; 263/263 build steps |
| Isolated Linux CI CTest | PASS; 83/83 tests, one external corpus test skipped |
| MinGW cross configure/build | PASS; 739/739 build steps; PE32+ x86-64 |
| MinGW artifact | `build/mingw-cross/sdl2/vaeg.exe`; SHA-256 `eb3327a77ad63ae31f7c508ab511aa6c31896af9ec5f2d669f8f93ebcbe43d2e` |
| Binary payload audit | PASS; no binary payload changed |
| Active reference scan | PASS; no current reference to deleted GDC/CRTC/rendering files or FM7/X1/X68K backends |
| Hosted GitHub Actions | Run [31573711804](https://github.com/nakatamaho/vaeg/actions/runs/31573711804): 8 jobs passed; Windows MinGW compatibility failed during `Configure` (job [94041018759](https://github.com/nakatamaho/vaeg/actions/runs/31573711804/job/94041018759)); no compile or test step ran in that job |

The first CTest invocation without the isolated Git environment produced
false failures in existing Git-history validators because the sandbox denied
access to the maintainer's global Git configuration. Re-running the unchanged
suite with `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null` passed all
83 tests. This is the recorded result.

The hosted run did not establish a source-level failure: the Windows
compatibility job stopped at the `Configure` step before compilation or
tests, while the local MinGW cross build passed. Its unauthenticated job log
was not available from the public API, so this report does not speculate about
the Configure cause or rerun the unchanged hosted job. The maintainer reported
completion of the standard VA human gate against the candidate; G88 passed.
No correctness bug was introduced or fixed by this source-tree cleanup, so no
entry was added to the permanent bug-fix ledger.

## G88 result

After a clean checkout of the candidate, perform the standard VA gate:
boot V3 mode, run the bundled VA demo, boot an OS, and perform simple FDD,
SASI/SCSI, keyboard, display, and state-save operations. Also verify that the
Screen menu exposes only the retained font choices and that MPU98II remains
selectable. The maintainer reported that this gate passed for candidate
`98d7343df9c763354e0775bd04a7b6d8d9c6a291`.
