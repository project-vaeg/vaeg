# M1 Deviations

Scope: VS2008 baseline project generation for `Win9x/np2.dsp`.

## Environment

| File | Line | Reason |
|---|---:|---|
| `Win9x/np2.vcproj` | 2 | Created VS2008/VC9 project from the canonical VC6 `Win9x/np2.dsp`. This environment has no `devenv`, `vcbuild`, `msbuild`, or `wine`, so the project file was generated from the `.dsp` rather than converted by a local VS2008 IDE run. |
| `Win9x/np2.sln` | 1 | Created VS2008 solution wrapper for `np2.vcproj`; same local VS2008 tooling limitation as above. |

Build status: not run in this Linux workspace because VC9 tooling is unavailable. G1 remains a human/user build gate with VS2008 Win32 Release.

Required assembler: NASM 2.00 or newer, installed as `nasm` on `PATH`. The old `c:\bin\nasm\nasmw` path is intentionally not required.

Required legacy DirectX import libraries: `dxguid.lib` and `DSOUND.LIB` must be available through `$(DXSDK_DIR)Lib\x86`, `$(DXSDK_DIR)\Lib\x86`, `$(DXSDK_DIR)Lib`, `$(DXSDK_DIR)\Lib`, the Windows SDK lib path, or the VS2008 Platform SDK lib path. DirectX SDK June 2010 is a suitable provider for these libraries. `ddraw.lib` is not required because `DirectDrawCreate` is loaded dynamically from `ddraw.dll`.

## Source Deviations

| File | Line | Reason |
|---|---:|---|
| `Win9x/NP2.RC` | 10 | Replaced the MFC-only resource header path with the standard Windows SDK `windows.h`; the VS2008 resource compiler environment reported RC1015 for both `afxres.h` and `winres.h`, while the project does not otherwise use MFC. |
| `Win9x/NP2.RC` | 11 | Added a local `IDC_STATIC` fallback because that dialog-control ID previously came from the MFC resource header. |
| `Win9x/NP2.RC` | 819 | Updated the matching `TEXTINCLUDE` resource so future Resource Editor regeneration keeps the same non-MFC Windows SDK header and `IDC_STATIC` fallback. |
| `Win9x/DD2.CPP` | 3 | Added a local `DirectDrawCreate` loader using `LoadLibrary("ddraw.dll")` and `GetProcAddress` because DirectX SDK June 2010 provides `ddraw.h` but not `ddraw.lib`. |
| `Win9x/DD2.CPP` | 60 | Replaced the import-library call to `DirectDrawCreate` with the local loader; behavior still fails through the existing error path when DirectDraw is unavailable. |
| `Win9x/SCRNMNG.CPP` | 3 | Added the same local `DirectDrawCreate` loader for the primary screen manager, for the same missing `ddraw.lib` reason. |
| `Win9x/SCRNMNG.CPP` | 406 | Replaced the import-library call to `DirectDrawCreate` with the local loader; behavior still fails through the existing error path when DirectDraw is unavailable. |
| `I286X/MEMORY.X86` | 40 | Converted external-symbol address aliases from `equ` to `%define` so modern NASM expands them before COFF relocation; this resolves unresolved aliases such as `_bmsio_nomem`, `VIDEOMEMORY`, and `_extmemmng_ptr` without changing the target addresses. |
| `I286X/EGCMEM.X86` | 37 | Converted `VIDEOMEMORY` from an external-symbol `equ` alias to `%define` for the same NASM/COFF relocation reason. |
| `Win9x/x86/OPNGENG.X86` | 114 | Converted `ENVCURVE`, `SINTABLE`, and `ENVTABLE` from external-symbol `equ` aliases to `%define` for the same NASM/COFF relocation reason. |
| `Win9x/x86/MAKEGRPH.X86` | 18 | Converted the `GDC_S_*` address aliases from external-symbol `equ` aliases to `%define` for the same NASM/COFF relocation reason. |

## Project Deviations

| File | Line | Reason |
|---|---:|---|
| `Win9x/np2.vcproj` | 113 | `.\x86\PARTS.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 116 | `.\x86\PARTS.X86` custom build for `Trace|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 119 | `.\x86\PARTS.X86` custom build for `WaveRec|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 122 | `.\x86\PARTS.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 137 | `..\I286X\DMAP.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 140 | `..\I286X\DMAP.X86` custom build for `Trace|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 143 | `..\I286X\DMAP.X86` custom build for `WaveRec|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 146 | `..\I286X\DMAP.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 151 | `..\I286X\EGCMEM.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 154 | `..\I286X\EGCMEM.X86` custom build for `Trace|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 157 | `..\I286X\EGCMEM.X86` custom build for `WaveRec|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 160 | `..\I286X\EGCMEM.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 175 | `..\I286X\MEMORY.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 178 | `..\I286X\MEMORY.X86` custom build for `Trace|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 181 | `..\I286X\MEMORY.X86` custom build for `WaveRec|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 184 | `..\I286X\MEMORY.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 257 | `.\x86\OPNGENG.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 260 | `.\x86\OPNGENG.X86` custom build for `Trace|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; preserves original .cod listing output. |
| `Win9x/np2.vcproj` | 263 | `.\x86\OPNGENG.X86` custom build for `WaveRec|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; preserves original .cod listing output. |
| `Win9x/np2.vcproj` | 266 | `.\x86\OPNGENG.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 413 | `.\x86\CPUTYPE.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 416 | `.\x86\CPUTYPE.X86` custom build for `Trace|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 419 | `.\x86\CPUTYPE.X86` custom build for `WaveRec|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 422 | `.\x86\CPUTYPE.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 429 | `.\DCLOCKD.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 432 | `.\DCLOCKD.X86` custom build for `Trace|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 435 | `.\DCLOCKD.X86` custom build for `WaveRec|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 438 | `.\DCLOCKD.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 585 | `.\x86\MAKEGRPH.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 588 | `.\x86\MAKEGRPH.X86` custom build for `Trace|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 591 | `.\x86\MAKEGRPH.X86` custom build for `WaveRec|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 594 | `.\x86\MAKEGRPH.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 649 | `..\CPUXVA\MEMORYVA.X86` custom build for `Release|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 652 | `..\CPUXVA\MEMORYVA.X86` custom build for `Debug|Win32` uses `nasm -f win32` on `PATH` instead of hardcoded `c:\bin\nasm\nasmw`; command starts with `cd /d "$(ProjectDir)"` so single-quoted NASM includes such as `x86/np2asm.inc` resolve from the same working directory semantics as the VC6 project; object output only, matching .dsp. |
| `Win9x/np2.vcproj` | 151 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 154 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 157 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 160 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 175 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 178 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 181 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 184 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 585 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 588 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 591 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 594 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 649 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 652 | Added `Win9x/x86/NP2ASM.INC` as a custom-build dependency for NASM inputs that include `x86/np2asm.inc`; this is dependency tracking only and does not change assembler input or output. |
| `Win9x/np2.vcproj` | 19 | Explicitly pins the VS2008 configuration to Win32 multi-byte character set (`_MBCS`/`CharacterSet="2"`) as required by M1 instead of relying on IDE defaults. |
| `Win9x/np2.vcproj` | 38 | Explicitly pins the VS2008 configuration to Win32 multi-byte character set (`_MBCS`/`CharacterSet="2"`) as required by M1 instead of relying on IDE defaults. |
| `Win9x/np2.vcproj` | 57 | Explicitly pins the VS2008 configuration to Win32 multi-byte character set (`_MBCS`/`CharacterSet="2"`) as required by M1 instead of relying on IDE defaults. |
| `Win9x/np2.vcproj` | 76 | Explicitly pins the VS2008 configuration to Win32 multi-byte character set (`_MBCS`/`CharacterSet="2"`) as required by M1 instead of relying on IDE defaults. |
| `Win9x/np2.vcproj` | 25 | Explicitly pins VC9 runtime library (`/MT` for non-Debug, `/MTd` for Debug) because VS2008 does not retain VC6 single-threaded CRT defaults; this is a project-setting conversion deviation, not a source change. |
| `Win9x/np2.vcproj` | 44 | Explicitly pins VC9 runtime library (`/MT` for non-Debug, `/MTd` for Debug) because VS2008 does not retain VC6 single-threaded CRT defaults; this is a project-setting conversion deviation, not a source change. |
| `Win9x/np2.vcproj` | 63 | Explicitly pins VC9 runtime library (`/MT` for non-Debug, `/MTd` for Debug) because VS2008 does not retain VC6 single-threaded CRT defaults; this is a project-setting conversion deviation, not a source change. |
| `Win9x/np2.vcproj` | 82 | Explicitly pins VC9 runtime library (`/MT` for non-Debug, `/MTd` for Debug) because VS2008 does not retain VC6 single-threaded CRT defaults; this is a project-setting conversion deviation, not a source change. |
| `Win9x/np2.vcproj` | 29 | Explicitly adds `$(DXSDK_DIR)Lib\x86;$(DXSDK_DIR)\Lib\x86;$(DXSDK_DIR)Lib;$(DXSDK_DIR)\Lib;$(WindowsSdkDir)Lib;$(VCInstallDir)PlatformSDK\Lib` to the `Release|Win32` linker library search path for `dxguid.lib` and `DSOUND.LIB`, and removes unavailable `ddraw.lib` after `DirectDrawCreate` was moved to dynamic loading. |
| `Win9x/np2.vcproj` | 48 | Applies the same DirectX SDK library search path and `ddraw.lib` removal to `Trace|Win32`. |
| `Win9x/np2.vcproj` | 67 | Applies the same DirectX SDK library search path and `ddraw.lib` removal to `WaveRec|Win32`. |
| `Win9x/np2.vcproj` | 86 | Applies the same DirectX SDK library search path and `ddraw.lib` removal to `Debug|Win32`. |

## Verification Performed Here

```text
Win9x/np2.vcproj: XML parse OK
Win9x/np2.vcproj: CRLF only
Win9x/np2.sln: CRLF only
Win9x/np2.vcproj: 261 <File> entries
Win9x/np2.vcproj: 34 per-configuration NASM command lines across 9 .X86 inputs
Win9x/np2.vcproj: 4 linker tools with explicit DirectX SDK and VS2008/Windows SDK library search paths
Win9x/np2.vcproj: 0 ddraw.lib dependencies
I286X/MEMORY.X86, I286X/EGCMEM.X86, Win9x/x86/OPNGENG.X86, Win9x/x86/MAKEGRPH.X86: external address aliases use NASM %define instead of relocatable equ aliases
Win9x/np2.vcproj: no c:\bin\nasm\nasmw references
```
