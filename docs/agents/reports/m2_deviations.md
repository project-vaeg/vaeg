# M2 Deviations

Scope: VS2017 / PlatformToolset v141 project creation from the M1 VS2008 project.

No shared source files were edited in M2. The VS2008 `Win9x/np2.vcproj` and
`Win9x/np2.sln` files were not modified.

## Environment

| File | Line | Reason |
|---|---:|---|
| `Win9x/np2_v141.sln` | 1 | Created a separate VS2017 solution instead of editing `Win9x/np2.sln`, so the VS2008 solution remains untouched and buildable. |
| `Win9x/np2_v141.sln` | 5 | Points the VS2017 solution at the new `Win9x/np2.vcxproj` while retaining the M1 project GUID. |
| `Win9x/np2.vcxproj` | 2 | Created an MSBuild C++ project by translating the M1 `Win9x/np2.vcproj`; this Linux workspace has no local `devenv`, `msbuild`, or v141 toolchain. |
| `docs/agents/reports/m2_warnings.txt` | 1 | Full v141 warning capture could not be generated locally because the required Visual Studio tooling is unavailable here; no warning fixes were made. |

## Source Deviations

None.

## Project Deviations

| File | Line | Reason |
|---|---:|---|
| `Win9x/np2.vcxproj` | 26 | Pins `WindowsTargetPlatformVersion` to `10.0.19041.0`, the Windows 10 SDK version confirmed in the user-side VS2017 environment. |
| `Win9x/np2.vcxproj` | 32 | Sets `PlatformToolset` to `v141` for `Release|Win32`. |
| `Win9x/np2.vcxproj` | 38 | Sets `PlatformToolset` to `v141` for `Trace|Win32`. |
| `Win9x/np2.vcxproj` | 44 | Sets `PlatformToolset` to `v141` for `WaveRec|Win32`. |
| `Win9x/np2.vcxproj` | 50 | Sets `PlatformToolset` to `v141` for `Debug|Win32`. |
| `Win9x/np2.vcxproj` | 33 | Sets the VS2017 character set to MultiByte for `Release|Win32`, matching M1 `_MBCS`. |
| `Win9x/np2.vcxproj` | 39 | Sets the VS2017 character set to MultiByte for `Trace|Win32`, matching M1 `_MBCS`. |
| `Win9x/np2.vcxproj` | 45 | Sets the VS2017 character set to MultiByte for `WaveRec|Win32`, matching M1 `_MBCS`. |
| `Win9x/np2.vcxproj` | 51 | Sets the VS2017 character set to MultiByte for `Debug|Win32`, matching M1 `_MBCS`. |
| `Win9x/np2.vcxproj` | 99 | Carries `_MBCS` into the `Release|Win32` preprocessor definitions. |
| `Win9x/np2.vcxproj` | 128 | Carries `_MBCS` into the `Trace|Win32` preprocessor definitions. |
| `Win9x/np2.vcxproj` | 157 | Carries `_MBCS` into the `WaveRec|Win32` preprocessor definitions. |
| `Win9x/np2.vcxproj` | 186 | Carries `_MBCS` into the `Debug|Win32` preprocessor definitions. |
| `Win9x/np2.vcxproj` | 106 | Adds `/source-charset:.932 /execution-charset:.932` for `Release|Win32`; also adds `/Zc:strictStrings-` as a legacy-code conformance relaxation. |
| `Win9x/np2.vcxproj` | 135 | Adds `/source-charset:.932 /execution-charset:.932` for `Trace|Win32`; also adds `/Zc:strictStrings-` as a legacy-code conformance relaxation. |
| `Win9x/np2.vcxproj` | 164 | Adds `/source-charset:.932 /execution-charset:.932` for `WaveRec|Win32`; also adds `/Zc:strictStrings-` as a legacy-code conformance relaxation. |
| `Win9x/np2.vcxproj` | 194 | Adds `/source-charset:.932 /execution-charset:.932` for `Debug|Win32`; also adds `/Zc:strictStrings-` as a legacy-code conformance relaxation. |
| `Win9x/np2.vcxproj` | 107 | Sets `ConformanceMode` to `false` for `Release|Win32`, enabling permissive mode for legacy sources. |
| `Win9x/np2.vcxproj` | 136 | Sets `ConformanceMode` to `false` for `Trace|Win32`, enabling permissive mode for legacy sources. |
| `Win9x/np2.vcxproj` | 165 | Sets `ConformanceMode` to `false` for `WaveRec|Win32`, enabling permissive mode for legacy sources. |
| `Win9x/np2.vcxproj` | 195 | Sets `ConformanceMode` to `false` for `Debug|Win32`, enabling permissive mode for legacy sources. |
| `Win9x/np2.vcxproj` | 121 | Translates the M1 non-incremental linker behavior for `Release|Win32` to the MSBuild `LinkIncremental=false` form. |
| `Win9x/np2.vcxproj` | 150 | Translates the M1 non-incremental linker behavior for `Trace|Win32` to the MSBuild `LinkIncremental=false` form. |
| `Win9x/np2.vcxproj` | 179 | Translates the M1 non-incremental linker behavior for `WaveRec|Win32` to the MSBuild `LinkIncremental=false` form. |
| `Win9x/np2.vcxproj` | 209 | Translates the M1 non-incremental linker behavior for `Debug|Win32` to the MSBuild `LinkIncremental=false` form. |

## NASM Custom Builds

All 34 M1 NASM custom build steps are carried into the v141 project. The commands
still use `nasm` on `PATH`, start from `$(ProjectDir)`, and emit COFF objects
under `$(IntDir)`.

| File | Line | Configuration | Reason |
|---|---:|---|---|
| `Win9x/np2.vcxproj` | 474 | `.\x86\PARTS.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 479 | `.\x86\PARTS.X86` `Trace|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 484 | `.\x86\PARTS.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 489 | `.\x86\PARTS.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 496 | `..\I286X\DMAP.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 501 | `..\I286X\DMAP.X86` `Trace|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 506 | `..\I286X\DMAP.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 511 | `..\I286X\DMAP.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 518 | `..\I286X\EGCMEM.X86` `Release|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 524 | `..\I286X\EGCMEM.X86` `Trace|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 530 | `..\I286X\EGCMEM.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 536 | `..\I286X\EGCMEM.X86` `Debug|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 544 | `..\I286X\MEMORY.X86` `Release|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 550 | `..\I286X\MEMORY.X86` `Trace|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 556 | `..\I286X\MEMORY.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 562 | `..\I286X\MEMORY.X86` `Debug|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 570 | `.\x86\OPNGENG.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 575 | `.\x86\OPNGENG.X86` `Trace|Win32` | Carries over the M1 NASM object build step with the `.cod` listing output. |
| `Win9x/np2.vcxproj` | 580 | `.\x86\OPNGENG.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step with the `.cod` listing output. |
| `Win9x/np2.vcxproj` | 585 | `.\x86\OPNGENG.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 592 | `.\x86\CPUTYPE.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 597 | `.\x86\CPUTYPE.X86` `Trace|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 602 | `.\x86\CPUTYPE.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 607 | `.\x86\CPUTYPE.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 614 | `.\DCLOCKD.X86` `Release|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 619 | `.\DCLOCKD.X86` `Trace|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 624 | `.\DCLOCKD.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 629 | `.\DCLOCKD.X86` `Debug|Win32` | Carries over the M1 NASM object build step. |
| `Win9x/np2.vcxproj` | 636 | `.\x86\MAKEGRPH.X86` `Release|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 642 | `.\x86\MAKEGRPH.X86` `Trace|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 648 | `.\x86\MAKEGRPH.X86` `WaveRec|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 654 | `.\x86\MAKEGRPH.X86` `Debug|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 662 | `..\CPUXVA\MEMORYVA.X86` `Release|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |
| `Win9x/np2.vcxproj` | 670 | `..\CPUXVA\MEMORYVA.X86` `Debug|Win32` | Carries over the M1 NASM object build step and `NP2ASM.INC` dependency. |

## Verification Performed Here

```text
Win9x/np2.vcxproj: XML parse OK
Win9x/np2.vcxproj: CRLF only
Win9x/np2_v141.sln: CRLF only
Win9x/np2.vcxproj: 261 project items
Win9x/np2.vcxproj: 9 CustomBuild inputs
Win9x/np2.vcxproj: 34 per-configuration NASM Command elements
Win9x/np2.vcxproj: PlatformToolset v141 in all 4 configurations
Win9x/np2.vcxproj: WindowsTargetPlatformVersion is pinned to 10.0.19041.0
Win9x/np2.vcxproj: /source-charset:.932 and /execution-charset:.932 in all 4 configurations
Win9x/np2.vcxproj: _MBCS in all 4 C/C++ configurations
Win9x/np2.vcproj and Win9x/np2.sln: unchanged in M2
```

Build status: not run in this Linux workspace because VS2017/v141 and VS2008
tooling are unavailable. VS2008 re-verification is not required here because M2
made no shared-source edits and did not modify the VS2008 project files.
