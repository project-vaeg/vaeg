# M0 Repository Inventory

Date: 2026-07-03

Scope: report-only inventory for M0. No source, build, binary, rename, encoding, or EOL fixes were made. New files are under `docs/agents/reports/` only.

Interpreter note: this environment has `/usr/bin/python3` but no `python` executable. The required scripts were first attempted with `python` and failed with `python: command not found`; the successful raw outputs below were produced with `python3`. The failed literal attempts are preserved as `*.python-missing.txt` in the raw directory.

## Raw Outputs

| File | Lines |
|---|---:|
| `docs/agents/reports/raw/check_encoding.txt` | 1073 |
| `docs/agents/reports/raw/check_eol.txt` | 1025 |
| `docs/agents/reports/raw/check_case.txt` | 3417 |
| `docs/agents/reports/raw/find_unreferenced.txt` | 55 |
| `docs/agents/reports/raw/check_encoding.python-missing.txt` | 1 |
| `docs/agents/reports/raw/check_eol.python-missing.txt` | 1 |
| `docs/agents/reports/raw/check_case.python-missing.txt` | 1 |
| `docs/agents/reports/raw/find_unreferenced.python-missing.txt` | 1 |

Successful commands:

```text
python3 tools/repo/check_encoding.py --report > docs/agents/reports/raw/check_encoding.txt 2>&1
python3 tools/repo/check_eol.py --report > docs/agents/reports/raw/check_eol.txt 2>&1
python3 tools/repo/check_case.py --report > docs/agents/reports/raw/check_case.txt 2>&1
python3 tools/repo/find_unreferenced.py --report > docs/agents/reports/raw/find_unreferenced.txt 2>&1
```

## Project Files

Tracked project/build roots matched by the M0 patterns and the reference script:

| Path | Kind | `SOURCE=` entries | M3 reachability/root note |
|---|---|---:|---|
| `Win9x/np2.dsp` | VC6 project | 261 | Yes: canonical Win32 VA build root. |
| `Win9x/np2.dsw` | VC6 workspace | - | Yes: workspace wrapper for canonical `np2.dsp`. |
| `Win9x/Makefile` | Cygwin Makefile | - | No for VA canonical reachability; legacy Win32/GCC plain NP2 build using `I286C`, proposed M3 prune/keep triage. |
| `Win9xC/np2c.dsp` | VC6 project | 169 | No: plain NP2/PC-9801, `I286C` C core, no VA subsystem; proposed M3 prune/keep triage. |
| `Win9xC/np2c.dsw` | VC6 workspace | - | No: workspace wrapper for frozen `Win9xC` project; proposed M3 prune/keep triage. |
| `sdl/Makefile.win` | SDL Makefile | - | Yes per M0 reference-root instruction; SDL Win32/GCC plain NP2 build using `I286C`, no VA subsystem. |
| `sdl/Makefile.zau` | SDL Makefile | - | Yes per M0 reference-root instruction; SDL Zaurus plain NP2 build using `I286C`, no VA subsystem. |
| `sdl/sdlw32s.dsp` | VC6 project | 186 | Likely yes if M3 treats all `sdl/` build files as roots; plain NP2/`I286C`, no VA subsystem. |
| `sdl/sdlw32s.dsw` | VC6 workspace | - | Likely yes if M3 treats all `sdl/` build files as roots; workspace wrapper for `sdlw32s.dsp`. |
| `NP2TOOL/MAKEFILE.W32` | nasm makefile | - | No for emulator reachability; builds helper `.COM` tools from `NP2TOOL/*.ASM`, proposed M3 prune/keep triage. |
| `ROMIMAGE/MAKEFILE.W32` | nasm/bin2txt makefile | - | Special: ROM/resource regeneration root; do not treat `ROMIMAGE/` deletion as a script-only decision. |
| `WinCE/np2.dsp` | VC6 project | 182 | No: frozen legacy backend; proposed M3 prune/keep triage. |
| `WinCE/np2.dsw` | VC6 workspace | - | No: workspace wrapper for frozen WinCE backend; proposed M3 prune/keep triage. |
| `Mona/mona.dsp` | VC6 project | 138 | No: frozen legacy backend; uses `I286X` but no VA subsystem; proposed M3 prune/keep triage. |
| `Mona/mona.dsw` | VC6 workspace | - | No: workspace wrapper for frozen Mona backend; proposed M3 prune/keep triage. |
| `accessories/bin2txt.dsp` | VC6 project | 4 | No for emulator reachability; accessory tool, proposed M3 prune/keep triage. |
| `accessories/bin2txt.dsw` | VC6 workspace | - | No: workspace wrapper for accessory tool, proposed M3 prune/keep triage. |
| `accessories/lzxpack.dsp` | VC6 project | 4 | No for emulator reachability; accessory tool, proposed M3 prune/keep triage. |
| `accessories/lzxpack.dsw` | VC6 workspace | - | No: workspace wrapper for accessory tool, proposed M3 prune/keep triage. |

No `*.sln`, `*.vcproj`, `*.vcxproj`, or `CMakeLists.txt` files were found by the M0/project-root scan. No `Makefile*` files were found under `WinCE/`, `Mona/`, or `MacOS9/`; uppercase `MAKEFILE.W32` roots exist in `NP2TOOL/` and `ROMIMAGE/` and are matched by the current reference script case-insensitively.

## Canonical Win32 Project

`Win9x/np2.dsp` is the canonical VC6 project. The inventory confirms 261 `SOURCE=` entries.

Subsystem/source-entry census for `Win9x/np2.dsp`:

| Area | Entries |
|---|---:|
| `(root)` | 10 |
| `BIOS` | 12 |
| `BIOSVA` | 1 |
| `CBUS` | 15 |
| `COMMON` | 9 |
| `CPUCVA` | 4 |
| `CPUXVA` | 1 |
| `FDD` | 7 |
| `FONT` | 9 |
| `GENERIC` | 9 |
| `I286X` | 9 |
| `IO` | 28 |
| `IOVA` | 24 |
| `LIO` | 6 |
| `SOUND` | 27 |
| `VRAM` | 8 |
| `VRAMVA` | 10 |
| `Win9x` | 72 |

VA-specific entries are present: `IOVA` (24), `VRAMVA` (10), `CPUCVA` (4), `CPUXVA` (1), `BIOSVA` (1), plus the `I286X` CPU core (9). This distinguishes the canonical VA build from `Win9xC/np2c.dsp`, `sdl/Makefile*`, and `sdl/sdlw32s.dsp`, which use the plain `I286C` C core and do not reference the VA subsystem.

Full `Win9x/np2.dsp` source list:

```text
..\COMMON\_MEMORY.C
..\COMMON\BMPDATA.C
..\COMMON\LSTARRAY.C
..\COMMON\MILSTR.C
..\COMMON\MIMPIDEF.C
.\x86\PARTS.X86
..\COMMON\PROFILE.C
..\COMMON\STRRES.C
..\COMMON\TEXTFILE.C
..\COMMON\WAVEFILE.C
..\I286X\DMAP.X86
..\I286X\EGCMEM.X86
..\I286X\I286X.CPP
..\I286X\I286XADR.CPP
..\I286X\I286XCTS.CPP
..\I286X\I286XREP.CPP
..\I286X\I286XS.CPP
..\I286X\MEMORY.X86
..\I286X\V30PATCH.CPP
..\BIOS\BIOS.C
..\BIOS\BIOS09.C
..\BIOS\BIOS0C.C
..\BIOS\BIOS12.C
..\BIOS\BIOS13.C
..\BIOS\BIOS18.C
..\BIOS\BIOS19.C
..\BIOS\BIOS1A.C
..\BIOS\BIOS1B.C
..\BIOS\BIOS1C.C
..\BIOS\BIOS1F.C
..\BIOS\SXSIBIOS.C
..\SOUND\VERMOUTH\MIDIMOD.C
..\SOUND\VERMOUTH\MIDINST.C
..\SOUND\VERMOUTH\MIDIOUT.C
..\SOUND\VERMOUTH\MIDTABLE.C
..\SOUND\VERMOUTH\MIDVOICE.C
..\SOUND\GETSND\GETSMIX.C
..\SOUND\GETSND\GETSND.C
..\SOUND\GETSND\GETWAVE.C
..\SOUND\ADPCMC.C
..\SOUND\ADPCMG.C
..\SOUND\BEEPC.C
..\SOUND\BEEPG.C
..\SOUND\CS4231C.C
..\SOUND\CS4231G.C
..\SOUND\FMBOARD.C
..\SOUND\FMTIMER.C
..\SOUND\OPNGENC.C
.\x86\OPNGENG.X86
..\SOUND\PCM86C.C
..\SOUND\PCM86G.C
..\SOUND\PSGGENC.C
..\SOUND\PSGGENG.C
..\SOUND\RHYTHMC.C
..\SOUND\S98.C
..\SOUND\SOUND.C
..\SOUND\SOUNDROM.C
..\SOUND\TMS3631C.C
..\SOUND\TMS3631G.C
..\FDD\DISKDRV.C
..\FDD\FDD_D88.C
..\FDD\FDD_MTR.C
..\FDD\FDD_XDF.C
..\FDD\FDDFILE.C
..\FDD\NEWDISK.C
..\FDD\SXSI.C
..\LIO\GCIRCLE.C
..\LIO\GLINE.C
..\LIO\GPSET.C
..\LIO\GPUT1.C
..\LIO\GSCREEN.C
..\LIO\LIO.C
..\FONT\FONT.C
..\FONT\FONTDATA.C
..\FONT\FONTFM7.C
..\FONT\FONTMAKE.C
..\FONT\FONTPC88.C
..\FONT\FONTPC98.C
..\FONT\FONTV98.C
..\FONT\FONTX1.C
..\FONT\FONTX68K.C
.\DIALOG\D_ABOUT.CPP
.\DIALOG\D_BMP.CPP
.\DIALOG\D_BMS.CPP
.\DIALOG\D_CLND.CPP
.\DIALOG\D_CONFIG.CPP
.\DIALOG\D_DISK.CPP
.\DIALOG\D_MPU98.CPP
.\DIALOG\D_OPRECORD.CPP
.\DIALOG\D_SCREEN.CPP
.\DIALOG\D_SERIAL.CPP
.\DIALOG\D_SOUND.CPP
.\DIALOG\DIALOGS.CPP
.\DIALOG\NP2CLASS.CPP
.\DEBUGUTY\DEBUGCTRL.CPP
.\DEBUGUTY\VIEW1MB.CPP
.\DEBUGUTY\VIEWASM.CPP
.\DEBUGUTY\VIEWCMN.CPP
.\DEBUGUTY\VIEWER.CPP
.\DEBUGUTY\VIEWGACTRLVA.CPP
.\DEBUGUTY\VIEWMEM.CPP
.\DEBUGUTY\VIEWMENU.CPP
.\DEBUGUTY\VIEWREG.CPP
.\DEBUGUTY\VIEWSEG.CPP
.\DEBUGUTY\VIEWSND.CPP
.\DEBUGUTY\VIEWSUBASM.CPP
.\DEBUGUTY\VIEWSUBMEM.CPP
.\DEBUGUTY\VIEWSUBREG.CPP
.\DEBUGUTY\VIEWVABANK.CPP
.\DEBUGUTY\VIEWVIDEOVA.CPP
.\CMMIDI.CPP
.\CMPARA.CPP
.\CMSERIAL.CPP
.\COMMNG.CPP
.\x86\CPUTYPE.X86
.\DCLOCK.CPP
.\DCLOCKD.X86
.\DD2.CPP
.\DOSIO.CPP
.\EXTROMIO.CPP
.\FONTMNG.CPP
.\INI.CPP
.\JOYMNG.CPP
.\JULIET.CPP
.\MENU.CPP
.\MOUSEMNG.CPP
.\NP2.CPP
.\NP2.RC
.\NP2ARG.CPP
.\SCRNMNG.CPP
.\SOUNDMNG.CPP
.\SSTP.CPP
.\SSTPMSG.CPP
.\SUBWIND.CPP
.\SYSMNG.CPP
.\TASKMNG.CPP
.\TIMEMNG.CPP
.\TOOLWIN.CPP
.\TRACE.CPP
.\WINKBD.CPP
.\WINLOC.CPP
..\IO\ARTIC.C
..\IO\BMSIO.C
..\IO\CGROM.C
..\IO\CPUIO.C
..\IO\CRTC.C
..\IO\DIPSW.C
..\IO\DMAC.C
..\IO\EGC.C
..\IO\EMSIO.C
..\IO\EPSONIO.C
..\IO\FDC.C
..\IO\FDD320.C
..\IO\GDC.C
..\IO\GDC_PSET.C
..\IO\GDC_SUB.C
..\IO\IOCORE.C
..\IO\MOUSEIF.C
..\IO\NECIO.C
..\IO\NMIIO.C
..\IO\NP2SYSP.C
..\IO\NP2VASUP.C
..\IO\PIC.C
..\IO\PIT.C
..\IO\PRINTIF.C
..\IO\SERIAL.C
..\IO\SYSPORT.C
..\IO\UPD4990.C
..\CBUS\AMD98.C
.\BOARD118.C
..\CBUS\BOARD14.C
..\CBUS\BOARD26K.C
..\CBUS\BOARD86.C
..\CBUS\BOARDSPB.C
..\CBUS\BOARDX2.C
..\CBUS\CBUSCORE.C
..\CBUS\CS4231IO.C
..\CBUS\IDEIO.C
..\CBUS\MPU98II.C
..\CBUS\PC9861K.C
..\CBUS\PCM86IO.C
..\CBUS\SASIIO.C
..\CBUS\SCSICMD.C
..\CBUS\SCSIIO.C
..\VRAM\DISPSYNC.C
.\x86\MAKEGRPH.X86
..\VRAM\MAKETEXT.C
..\VRAM\MAKETGRP.C
..\VRAM\PALETTES.C
..\VRAM\SCRNDRAW.C
..\VRAM\SCRNSAVE.C
..\VRAM\SDRAW.C
..\VRAM\VRAM.C
..\GENERIC\CMJASTS.C
..\GENERIC\CMNDRAW.C
..\GENERIC\DIPSWBMP.C
..\GENERIC\HOSTDRV.C
..\GENERIC\HOSTDRVS.C
..\GENERIC\KEYDISP.C
..\GENERIC\NP2INFO.C
..\GENERIC\SOFTKBD.C
..\GENERIC\UNASM.C
..\VRAMVA\MAKEGRPHVA.C
..\VRAMVA\MAKESPRVA.C
..\VRAMVA\MAKETEXTVA.C
..\VRAMVA\PALETTESVA.C
..\VRAMVA\SCRNDRAWVA.C
..\VRAMVA\SDRAWVA.C
..\CPUXVA\MEMORYVA.X86
..\BIOSVA\BIOSVA.C
..\IOVA\BKUPMEMVA.C
..\IOVA\BOARDSB2.C
..\IOVA\CGROMVA.C
..\IOVA\FDSUBSYS.C
..\IOVA\GACTRLVA.C
..\IOVA\I8255.C
..\IOVA\IOCOREVA.C
..\IOVA\MEMCTRLVA.C
..\IOVA\MOUSEIFVA.C
..\IOVA\SGP.C
..\IOVA\SUBSYSTEM.CPP
..\IOVA\SUBSYSTEMIF.C
..\IOVA\SUBSYSTEMMX.C
..\IOVA\SYSPORTVA.C
..\IOVA\TSP.C
..\IOVA\UPD9002.C
..\IOVA\VA91.C
..\IOVA\VIDEOVA.C
..\CPUCVA\GVRAMVA.C
..\CPUCVA\Z80c.cpp
..\CPUCVA\Z80diag.cpp
..\BREAKPOINT.C
..\CALENDAR.C
..\DEBUGSUB.C
..\KEYSTAT.C
..\NEVENT.C
..\OPRECORD.C
..\PCCORE.C
..\STATSAVE.C
..\TIMING.C
..\BREAKPOINT.H
..\IOVA\FDSUBSYS.H
..\IOVA\GACCESS.H
..\VRAMVA\MAKESPRVA.H
..\IO\NP2VASUP.H
..\VRAMVA\PALETTESVA.H
..\VRAMVA\SCRNDRAWVA.H
..\VRAMVA\SDRAWVA.H
..\IOVA\SGP.H
..\IOVA\SUBSYSTEM.H
..\IOVA\TSP.H
..\IOVA\VIDEOVA.H
.\DEBUGUTY\VIEWVIDEOVA.H
..\CPUCVA\Z80if.h
.\ICONS\NEKOP2.BMP
.\ICONS\Np2.ico
.\ICONS\NP2DEBUG.ICO
.\ICONS\NP2TOOL.BMP
.\ICONS\NP2TOOL2.BMP
.\ICONS\Fddseek.wav
.\ICONS\Fddseek1.wav
```

## File Census

Tracked file count at the time of the M0 script runs: 1073.

Encoding report summary from `check_encoding.txt`:

| Class | Count |
|---|---:|
| `ASCII` | 612 |
| `CP932` | 402 |
| `UTF8` | 10 |
| `BINARY` | 48 |
| `UNKNOWN` | 1 |

Notable encoding finding: `MacOS9/np2.r` is the only `UNKNOWN` file.

EOL report summary from `check_eol.txt` (binary files skipped by the tool):

| Class | Count |
|---|---:|
| `LF` | 968 |
| `CRLF` | 55 |
| `MIXED` | 2 |

Mixed-EOL files:

```text
MIXED MacOS9/Carbon.r
MIXED MacOS9/np2.r
```

Case report summary from `check_case.txt`:

| Finding kind | Count |
|---|---:|
| `COLLISION` | 0 |
| `UPPERCASE` | 1048 |
| `INC-CASE` | 2368 |
| Total findings | 3416 |

No case-fold path collisions were reported. Uppercase paths and include-case mismatches are expected transitional findings for M4.

## Reference Graph

Raw graph header: `# roots: 19, sources: 839, reached: 785, unreferenced: 54`

Important root-set caveat: `tools/repo/find_unreferenced.py --report` currently uses every tracked `*.dsp`, `*.dsw`, `*.sln`, `*.vcproj`, `*.vcxproj`, `CMakeLists.txt`, and `Makefile*` as a root. Its `--root` option only appends roots; it does not replace the default root set. Therefore the raw M0 graph used all 19 project/build roots listed above, including frozen/backend/tool roots, not only `Win9x/np2.dsp` plus SDL build files.

Unreferenced candidates reported by the raw tool:

```text
CBUS/ATAPICMD.C
DEBUGSUB386.C
GENERIC/MEMDBG32.C
I286A/I286A.C
I286A/I286A.INC
I286A/I286AALU.INC
I286A/I286AEA.INC
I286A/I286AIO.INC
I286A/I286AMEM.INC
I286A/I286AOP.INC
I286A/I286ASFT.INC
IO/PCIDEV.C
MacOS9/DIALOG/D_RESUME.CPP
MacOS9/MACKBD.CPP
MacOS9/MACOSSUB.CPP
MacOS9/NP2OPEN.CPP
Mona/x86/NP2ASM.INC
NP2TOOL/GETBIOS.ASM
NP2TOOL/HOSTDRV.ASM
NP2TOOL/HOSTDRV.INC
NP2TOOL/NP2TOOL.INC
NP2TOOL/NP2TOOL.X86
NP2TOOL/PWOFF.ASM
ROMIMAGE/BEEP.X86
ROMIMAGE/DATASEG.INC
ROMIMAGE/DIPSW.X86
ROMIMAGE/FIRMWARE.X86
ROMIMAGE/HDDBOOT.ASM
ROMIMAGE/IDEBIOS.ASM
ROMIMAGE/ITF.ASM
ROMIMAGE/ITF.INC
ROMIMAGE/ITFSUB.X86
ROMIMAGE/KEYBOARD.INC
ROMIMAGE/KEYBOARD.X86
ROMIMAGE/LIO.ASM
ROMIMAGE/MEMCHK.X86
ROMIMAGE/MEMSW.X86
ROMIMAGE/NP2.X86
ROMIMAGE/PC98.INC
ROMIMAGE/SASIBIOS.ASM
ROMIMAGE/SCSIBIOS.ASM
ROMIMAGE/SSP.X86
ROMIMAGE/SSP_DIP.X86
ROMIMAGE/SSP_MSW.X86
ROMIMAGE/SSP_RES.X86
ROMIMAGE/SSP_SUB.X86
ROMIMAGE/STARTUP.ASM
ROMIMAGE/TEXTDISP.X86
VRAM/MAKEGREX.C
Win9x/PCIFUNC.H
Win9x/x86/NP2ASM.INC
Win9x/x86/OPNGENG2.X86
WinCE/ARM/SDRAW.INC
WinCE/WCE/NP2PPCV.RC
```

M3 triage cautions from this inventory:

- `Win9x/x86/NP2ASM.INC` is listed as unreferenced, but it is a live canonical NASM include reached through single-quoted `%include` directives that the current graph parser does not match.
- `NP2TOOL/*` and many `ROMIMAGE/*` assembly files are listed as unreferenced even though their makefiles use implicit suffix rules or generated-resource workflows. These require human M3 triage, not direct deletion.
- The raw graph includes frozen backends as roots. If M3 wants canonical-plus-SDL reachability only, the script/root protocol needs an override or a separate filtered run.

## Assembly And Generated Code

Canonical `Win9x/np2.dsp` NASM custom build steps use `c:\bin\nasm\nasmw -f win32`. The task-named live dependencies are confirmed, and the canonical project also builds additional Win9x `.X86` helpers.

| Input | Built by canonical `Win9x/np2.dsp` | Note |
|---|---|---|
| `CPUXVA/MEMORYVA.X86` | Yes | Task-named VA memory/banking dependency; no `MEMORYVA.C`/`.CPP` replacement found. |
| `I286X/DMAP.X86` | Yes | Task-named I286X DMA/memory dependency. |
| `I286X/EGCMEM.X86` | Yes | Task-named I286X EGC memory dependency; includes `x86/np2asm.inc`. |
| `I286X/MEMORY.X86` | Yes | Task-named I286X memory dependency; includes `x86/np2asm.inc`. |
| `Win9x/x86/PARTS.X86` | Yes | Canonical Win9x helper assembled by `Win9x/np2.dsp`. |
| `Win9x/x86/OPNGENG.X86` | Yes | Canonical sound generator helper assembled by `Win9x/np2.dsp`. |
| `Win9x/x86/CPUTYPE.X86` | Yes | Canonical CPU type helper assembled by `Win9x/np2.dsp`. |
| `Win9x/DCLOCKD.X86` | Yes | Canonical debug clock helper assembled by `Win9x/np2.dsp`. |
| `Win9x/x86/MAKEGRPH.X86` | Yes | Canonical graphics helper assembled by `Win9x/np2.dsp`; includes `x86/np2asm.inc`. |

The tree contains `CPUXVA/MEMORYVA.H` and `CPUXVA/MEMORYVA.X86`; no `MEMORYVA.C`, `MEMORYVA.CPP`, or other portable-C `memoryva` implementation was found.

Other tracked assembly/include groups:

| Path/group | Build status observed | Note |
|---|---|---|
| `Win9x/x86/NP2ASM.INC` | Canonical NASM include | Included with single-quoted `%include` by `CPUXVA/MEMORYVA.X86`, `I286X/EGCMEM.X86`, `I286X/MEMORY.X86`, and `Win9x/x86/MAKEGRPH.X86`; raw reference graph lists it unreferenced because the script does not parse single-quoted NASM includes. |
| `Win9x/x86/OPNGENG2.X86` | No build reference found | Listed as `UNREFERENCED` by the raw graph. |
| `Mona/x86/PARTS.X86`, `Mona/x86/MAKEGRPH.X86` | Built by frozen Mona project | `Mona/mona.dsp` custom-builds both with `nasmw`; `Mona/x86/NP2ASM.INC` is included by `MAKEGRPH.X86`. |
| `I286A/*.S and I286A/*.INC` | Built only by frozen WinCE `.vcp` files found outside the M0 project-file glob | `WinCE/np2ppc*.vcp` and `WinCE/np2sig3.vcp` reference the ARM assembly files; the raw reference graph does not root `.vcp` files. |
| `WinCE/ARM/*.S and WinCE/ARM/SDRAW.INC` | Built only by frozen WinCE `.vcp` files found outside the M0 project-file glob | `SDRAW.INC` is included by `SDRAW.S`/`SDRAWQ16.S`; raw graph lists `SDRAW.INC` unreferenced. |
| `NP2TOOL/*.ASM, *.X86, *.INC` | Built by `NP2TOOL/MAKEFILE.W32` through implicit suffix rules | Raw graph lists these as unreferenced because the makefile does not enumerate source dependencies in a form the script follows. |
| `ROMIMAGE/*.ASM, *.X86, *.INC` | Partly built by `ROMIMAGE/MAKEFILE.W32` through explicit and implicit rules | Raw graph marks many ROMIMAGE files unreferenced; M3 must not treat that as a deletion verdict, especially under the repository rule protecting ROMIMAGE payloads. |
| `EMBED/MENUBASE/MENUDECO.INC` | Live include | Included by embed menu C files and referenced by SDL makefiles. |

Tracked assembly-like file census (`*.asm`, `*.x86`, `*.inc`, `*.s`):

| Top directory | Count |
|---|---:|
| `CPUXVA` | 1 |
| `EMBED` | 1 |
| `I286A` | 18 |
| `I286X` | 3 |
| `Mona` | 3 |
| `NP2TOOL` | 6 |
| `ROMIMAGE` | 34 |
| `Win9x` | 7 |
| `WinCE` | 7 |

## No Fixes Applied

Findings above are recorded only. Encoding, EOL, case, unreferenced-file deletion, and project/build fixes belong to M1-M6 and were not performed in M0.
