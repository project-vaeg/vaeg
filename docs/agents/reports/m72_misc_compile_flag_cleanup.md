# M72 misc compile-flag cleanup report

## Status

M72 is in progress on branch:

```text
topic/m72-misc-compile-flag-cleanup
```

Starting predecessor:

```text
24950894eca79e308afae8d574d43c8f393bb483
```

Current HEAD before inactive media/debug cleanup:

```text
2c193def7cd4a292f9fb4113619468986c9c525c
```

G72 has not been declared passed.

## Scope updates from maintainer direction

The maintainer clarified the storage cleanup scope after the initial inactive
flag audit:

- `SUPPORT_SCSI` is required behavior and should be folded to the active side
  rather than kept as a compile-time switch.
- `SUPPORT_BMS` is always enabled and should be folded to the active side
  rather than kept as a compile-time switch.
- `HOSTFAT` is the intended host-folder feature and must be preserved.
- Legacy `HOSTDRV` is not `HOSTFAT`; it is the old NP2 DOS host-shared-drive
  system-port service and should be removed.

The M72 task and ROADMAP were updated to record this distinction.

## Current cleanup inventory

| Area | Result |
| --- | --- |
| `OSLANG_SJIS` | Removed inactive source branches. |
| `OSLANG_EUC` | Removed inactive source branches. |
| `OSLINEBREAK_CR` | Removed inactive source branches. |
| `OSLINEBREAK_CRLF` | Removed inactive source branches. |
| `OSLANG_UTF8` | Preserved as the active text-language path. |
| `OSLINEBREAK_LF` | Preserved as the active newline behavior. |
| `SUPPORT_EUC` | Removed inactive EUC string backend. |
| `SUPPORT_ANK` | Removed inactive ANK string backend. |
| `SUPPORT_UTF8` | Folded to the active side and removed as a compile-time switch. |
| `milstr_*` dispatch | Folded to the active UTF-8 backend. |
| `BEEPCOUNTEREX` | Folded as always enabled in `io/pit.c`. |
| `SUPPORT_BMS` | Folded to the enabled side and removed as a compile-time switch. |
| `SUPPORT_SCSI` | Folded to the enabled side and removed as a compile-time switch. |
| `SUPPORT_SASI` | Folded to the enabled side and removed as a compile-time switch. |
| `SUPPORT_HOSTDRV` | Removed; not added to CMake. |
| `HOSTFAT` | Preserved in `io/hostfat.c`, `sdl2/hostfat_*`, `io/np2sysp.c`, `statsave.c`, `sdl2/ini.c`, and GUI paths. |
| Legacy `HOSTDRV` implementation | Removed from active source and legacy tool roots. |
| `SUPPORT_16BPP` | Folded to the active SDL2 RGB565 path and removed as a compile-time switch. |
| `SCREEN_BPP` | Removed; default offscreen VRAM creation now resolves directly to 16bpp. |
| `SUPPORT_CRT15KHZ` | Folded to the enabled side and removed as a compile-time switch. |
| `SUPPORT_SWSEEKSND` | Folded to the enabled side and removed as a compile-time switch. |
| `SUPPORT_CRT31KHZ` | Removed inactive Fellow-style 31kHz CRT branch. |
| `SUPPORT_8BPP` | Removed inactive display-output branch. |
| `SUPPORT_24BPP` | Removed inactive display/offscreen helper branch; 24-to-16 icon resizing remains covered by `resize.c`. |
| `SUPPORT_32BPP` | Removed inactive display-output branch. |
| `SUPPORT_NORMALDISP` | Removed inactive extended/normal display split; SDL2 reports no extended surface. |
| `SUPPORT_MP3` | Removed inactive optional MP3 sample decoder branch. WAV sample loading remains. |
| `SUPPORT_OGG` | Removed inactive optional OGG sample decoder branch. WAV sample loading remains. |
| `SUPPORT_S98` | Removed inactive S98 sound-register logging, including no-op init/sync calls. |
| `SUPPORT_KEYDISP` | Removed inactive key-display overlay and no-op instrumentation calls from sound/MIDI paths. |
| `SUPPORT_SOFTKBD` | Removed inactive software-keyboard overlay and LED callbacks. |
| `SUPPORT_PC9801_119` | Removed inactive PC-9801-119 software-keyboard alternate branch. |
| Legacy `embed/` menu directory | Removed. It was not in the active CMake source list and is separate from the active `cmake/embed_binary.cmake` asset embedding helper. |
| `SUPPORT_WAVEREC` | Deferred from M72 by maintainer direction. WAV recording remains for a later dedicated audit. |
| `SUPPORT_OPRECORD` | Deferred from M72 by maintainer direction. Operation recording remains for a later dedicated audit because it has state-save and device-observation hooks. |
| `io/fdd320.c` / `io/fdd320.h` | Retained. FDD320 is legacy-looking, but 5-inch 2D behavior may still be relevant to the PC-88 side of the VA environment and needs a later focused audit before removal. |

## Files removed by completed cleanup commits

```text
generic/hostdrv.c
generic/hostdrv.h
generic/hostdrv.tbl
generic/hostdrvs.c
generic/hostdrvs.h
generic/keydisp.c
generic/keydisp.h
generic/keydisp.res
generic/softkbd.c
generic/softkbd.h
generic/softkbd.res
generic/softkbd1.res
generic/softkbd2.res
generic/softkbd3.res
np2tool/hostdrv.asm
np2tool/hostdrv.inc
sound/getsnd/getmp3.c
sound/getsnd/getogg.c
sound/s98.c
sound/s98.h
```

The legacy `embed/` menu-source directory was also removed:

```text
embed/menu/dlgabout.c
embed/menu/dlgabout.h
embed/menu/dlgcfg.c
embed/menu/dlgcfg.h
embed/menu/dlgscr.c
embed/menu/dlgscr.h
embed/menu/filesel.c
embed/menu/filesel.h
embed/menu/menustr.c
embed/menu/menustr.h
embed/menubase/menubase.c
embed/menubase/menubase.h
embed/menubase/menudeco.inc
embed/menubase/menudlg.c
embed/menubase/menudlg.h
embed/menubase/menuicon.c
embed/menubase/menuicon.h
embed/menubase/menumbox.c
embed/menubase/menumbox.h
embed/menubase/menures.c
embed/menubase/menures.h
embed/menubase/menusys.c
embed/menubase/menusys.h
embed/menubase/menuvram.c
embed/menubase/menuvram.h
embed/readme.txt
embed/vramhdl.c
embed/vramhdl.h
embed/vrammix.c
embed/vrammix.h
```

The `np2tool/makefile.w32` `hostdrv.com` target was removed with the tool
source.

## HOSTFAT preservation

The active HOSTFAT path remains separate from HOSTDRV:

- `io/hostfat.c`
- `io/hostfat.h`
- `io/np2sysp.c` commands `check_hostfat` and `read_hostfat1`
- `sdl2/hostfat_snapshot.cpp`
- `sdl2/hostfat_manager.cpp`
- `sdl2/gui/gui.cpp` HOSTFAT mount UI
- `sdl2/ini.c` `HOSTFAT` and `HOSTFATDIR`
- `statsave.c` HOSTFAT snapshot identity checks

## Validation performed

```text
git diff --check
exit: 0
```

```text
cmake --build --preset linux-debug
exit: 0
notes: existing warnings remain; HOSTFAT.SYS and R2FPROBE.COM guest-driver
targets were disabled because NASM was not found.
```

```text
git diff --cached --check
exit: 0
```

```text
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
exit: 0
output: 0 finding(s)
```

```text
ctest --test-dir build/linux-debug --output-on-failure
exit: 0
output: No tests were found!!!
```

## Remaining validation

Before G72 review, rerun the required M72 validation set after the final commit
ordering is complete:

```text
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
```

Also record unavailable GCC, Clang, ASan/UBSan, MinGW, hosted CI, and protected
milestone checks as required by the task.

## Risks and notes

- SASI, SCSI, BMS, CRT15kHz display handling, and FDD seek sound are now
  unconditional active code paths instead of compile-time switches. This
  intentionally keeps the active paths that were already present in the source
  list.
- The non-document, non-test tree no longer contains `SUPPORT_BMS`,
  `SUPPORT_SCSI`, `SUPPORT_SASI`, `SUPPORT_HOSTDRV`, `OSLANG_SJIS`,
  `OSLANG_EUC`, `OSLINEBREAK_CR`, `OSLINEBREAK_CRLF`, `SUPPORT_EUC`,
  `SUPPORT_ANK`, `SUPPORT_UTF8`, `milank_*`, `mileuc_*`, `SUPPORT_8BPP`, `SUPPORT_16BPP`,
  `SUPPORT_24BPP`, `SUPPORT_32BPP`, `SUPPORT_NORMALDISP`, `SCREEN_BPP`,
  `SUPPORT_CRT15KHZ`, `SUPPORT_SWSEEKSND`, or `BEEPCOUNTEREX` references.
- Display output remains the existing SDL2 `RGB565` path. This cleanup removes
  compile-time alternatives; it does not change the VA guest-side color
  composition model.
- HOSTDRV state-save support was removed with the legacy HOSTDRV
  implementation. HOSTFAT state identity support is unchanged.
- S98 was sound-register logging, not sound playback. Removing it deletes
  inactive logging hooks and no-op calls while preserving VA sound generation
  and output.
- MP3 and OGG were inactive optional sample decoder branches. WAV sample
  loading through `sound/getsnd/getwave.c` remains active.
- KEYDISP and SOFTKBD were inactive overlays. Removing them deletes overlay
  instrumentation/no-op calls without changing active VA keyboard input.
- The removed `embed/` directory was the legacy embedded-menu source tree.
  It is not the active asset embedding system. `cmake/embed_binary.cmake`,
  `assets/vaeg.bmp`, `assets/NotoSansJP-Regular.ttf`, and `assets/vaeg.ico`
  remain unchanged.
- `SUPPORT_WAVEREC` and `SUPPORT_OPRECORD` are intentionally not M72
  removals. They remain present and require a later dedicated audit.
- `io/fdd320.c` and `io/fdd320.h` remain present. They are not treated as
  M72 inactive-removable code because 5-inch 2D behavior may still matter for
  the PC-88 side of the VA environment.
- Historical reports and old milestone tasks that mention HOSTDRV were not
  rewritten.
