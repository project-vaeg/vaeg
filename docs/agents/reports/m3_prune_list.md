# M3 Prune Triage List

Phase 1 scope: proposal only. No files were deleted in the Phase 1
session.

Candidate list regenerated with:

```text
python3 tools/repo/find_unreferenced.py --report
# roots: 23, sources: 839, reached: 787, unreferenced: 52
```

The 52 candidates below are triaged conservatively. Phase 2 must delete only
paths explicitly approved in the `DELETE` section by the user.

## DELETE

These files are not named by any scanned build root, are not protected payloads
or documentation, and have no basename references outside the generated M0/M3
reports.

| Path | Reason |
|---|---|
| `DEBUGSUB386.C` | Alternate 386 debug helper not present in the canonical VS2008/v141 projects or SDL roots; `DEBUGSUB.C` is the built debug helper. |
| `VRAM/MAKEGREX.C` | PC-9821 extended graphics source not present in any scanned build root; no `MAKEGREX.C` references outside reports. |
| `Win9x/PCIFUNC.H` | Standalone `pcidebug.dll` header with no include/build references outside reports. |
| `Win9x/x86/OPNGENG2.X86` | Alternate NASM OPN generator not custom-built by any project and not included by another NASM source. |

## KEEP

These are reported by the scanner, but should not be proposed for individual
deletion in M3 Phase 2.

| Path | Reason |
|---|---|
| `NP2TOOL/GETBIOS.ASM` | Built by `NP2TOOL/MAKEFILE.W32` through implicit suffix rules; keep unless the user approves pruning the whole `NP2TOOL/` root. |
| `NP2TOOL/HOSTDRV.ASM` | Built by `NP2TOOL/MAKEFILE.W32` through implicit suffix rules; keep unless the user approves pruning the whole `NP2TOOL/` root. |
| `NP2TOOL/HOSTDRV.INC` | Include used by the `NP2TOOL/` assembly sources; keep with the tool root. |
| `NP2TOOL/NP2TOOL.INC` | Include used by the `NP2TOOL/` assembly sources; keep with the tool root. |
| `NP2TOOL/NP2TOOL.X86` | Shared assembly include/body for `NP2TOOL/`; keep with the tool root. |
| `NP2TOOL/PWOFF.ASM` | Built by `NP2TOOL/MAKEFILE.W32` through implicit suffix rules; keep unless the user approves pruning the whole `NP2TOOL/` root. |
| `ROMIMAGE/BEEP.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/DATASEG.INC` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/DIPSW.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/FIRMWARE.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/HDDBOOT.ASM` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/IDEBIOS.ASM` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/ITF.ASM` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/ITF.INC` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/ITFSUB.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/KEYBOARD.INC` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/KEYBOARD.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/LIO.ASM` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/MEMCHK.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/MEMSW.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/NP2.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/PC98.INC` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/SASIBIOS.ASM` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/SCSIBIOS.ASM` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/SSP.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/SSP_DIP.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/SSP_MSW.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/SSP_RES.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/SSP_SUB.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/STARTUP.ASM` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |
| `ROMIMAGE/TEXTDISP.X86` | `ROMIMAGE/` is a standing exclusion; also part of ROM/resource generation workflows. |

## UNSURE

These are not safe for Phase 2 deletion without a separate user decision or a
more specific parser/root decision.

| Path | Reason |
|---|---|
| `CBUS/ATAPICMD.C` | Not named by current roots, but `CBUS/IDEIO.C` references `atapicmd_a0` under optional IDE/ATAPI code. |
| `GENERIC/MEMDBG32.C` | Not named by current roots, but `Win9x/SUBWIND.CPP` references `memdbg32_*` functions under optional `CPUCORE_IA32`/`SUPPORT_MEMDBG32` code. |
| `I286A/I286A.C` | Referenced by frozen WinCE `.vcp` files not rooted by the scanner; also part of the whole `I286A/` root question below. |
| `I286A/I286A.INC` | ARM/WinCE CPU-core include; scanner cannot classify the include chain without treating the `.vcp` files as roots. |
| `I286A/I286AALU.INC` | ARM/WinCE CPU-core include; scanner cannot classify the include chain without treating the `.vcp` files as roots. |
| `I286A/I286AEA.INC` | ARM/WinCE CPU-core include; scanner cannot classify the include chain without treating the `.vcp` files as roots. |
| `I286A/I286AIO.INC` | ARM/WinCE CPU-core include; scanner cannot classify the include chain without treating the `.vcp` files as roots. |
| `I286A/I286AMEM.INC` | ARM/WinCE CPU-core include; scanner cannot classify the include chain without treating the `.vcp` files as roots. |
| `I286A/I286AOP.INC` | ARM/WinCE CPU-core include; scanner cannot classify the include chain without treating the `.vcp` files as roots. |
| `I286A/I286ASFT.INC` | ARM/WinCE CPU-core include; scanner cannot classify the include chain without treating the `.vcp` files as roots. |
| `IO/PCIDEV.C` | Not named by current roots, but `IO/IOCORE.C` references `pcidev_*` symbols and `WinCE/np2ppcv.vcp` names the file. |
| `MacOS9/DIALOG/D_RESUME.CPP` | Frozen MacOS9 backend file; MacOS9 project files are outside the scanner root set and the whole root needs user direction. |
| `MacOS9/MACKBD.CPP` | Frozen MacOS9 backend file; MacOS9 project files are outside the scanner root set and the whole root needs user direction. |
| `MacOS9/MACOSSUB.CPP` | Frozen MacOS9 backend file; MacOS9 project files are outside the scanner root set and the whole root needs user direction. |
| `MacOS9/NP2OPEN.CPP` | Frozen MacOS9 backend file; MacOS9 project files are outside the scanner root set and the whole root needs user direction. |
| `WinCE/ARM/SDRAW.INC` | Included by WinCE ARM assembly sources; scanner does not root the WinCE `.vcp` files. |
| `WinCE/WCE/NP2PPCV.RC` | Named by `WinCE/np2ppcv.vcp`, which is outside the scanner root set. |

## Question: Whole Non-Canonical Roots (Not In DELETE)

The following roots are intentionally not placed in `DELETE` by this Phase 1
proposal. Should a later triage round propose deleting any of these roots
entirely?

| Root | Current note |
|---|---|
| `Win9xC/` | Frozen plain NP2 / PC-9801 Win32 backend; not canonical VA. |
| `WinCE/` | Frozen WinCE backend; contains `.vcp` roots not parsed by `find_unreferenced.py`. |
| `MacOS9/` | Frozen MacOS9 backend; project files are outside the scanner root set. |
| `Mona/` | Frozen Mona backend; currently treated as a build root by `Mona/mona.dsp`. |
| `I286A/` | ARM CPU core used by frozen WinCE `.vcp` files; not used by canonical VA. |
| `Win9x/Makefile` | Legacy Win32/GCC plain NP2 build using `I286C`, not canonical VA. |
| `NP2TOOL/` | Helper `.COM` tool sources built by `NP2TOOL/MAKEFILE.W32`; not part of emulator runtime builds. |

If the user approves pruning a whole root, Phase 2 should delete that build root
first, re-run `find_unreferenced.py --report`, and include newly exposed files
in the approved deletion list.

## Phase 2 Execution Summary

Commit 1 removed only the user-approved individual files and whole roots:

- `DEBUGSUB386.C`
- `VRAM/MAKEGREX.C`
- `Win9x/PCIFUNC.H`
- `Win9x/x86/OPNGENG2.X86`
- `WinCE/`
- `MacOS9/`
- `Mona/`
- `I286A/`
- `Win9xC/`
- `Win9x/Makefile`

No surviving project/build file referenced an approved deleted path, so no
project/build files were edited in the deletion commit.

### Machine Checks

| Check | Before Phase 2 | After deletion commit |
|---|---:|---:|
| `find_unreferenced.py --report` roots | 23 | 16 |
| `find_unreferenced.py --report` sources | 839 | 635 |
| `find_unreferenced.py --report` reached | 787 | 598 |
| `find_unreferenced.py --report` unreferenced | 52 | 37 |
| `check_encoding.py --report` ASCII | 628 | 442 |
| `check_encoding.py --report` BINARY | 48 | 41 |
| `check_encoding.py --report` CP932 | 403 | 336 |
| `check_encoding.py --report` UNKNOWN | 1 | 0 |
| `check_encoding.py --report` UTF8 | 10 | 10 |

Canonical build/project reference check after the deletion commit:

- `Win9x/np2.dsp`: no approved deleted-path references
- `Win9x/np2.vcproj`: no approved deleted-path references
- `Win9x/np2.vcxproj`: no approved deleted-path references

## Phase 2b candidates

These files were newly exposed by the approved Phase 2 deletion commit. They
are not deleted in Phase 2; this section is a proposal for later approval.

### DELETE

| Path | Reason |
|---|---|
| `COMMON/UCSCNV.C` | Newly exposed after frozen-root deletion; no scanned root or external textual reference outside reports, only the paired header/self references. |
| `COMMON/UCSCNV.H` | Paired header for `COMMON/UCSCNV.C`; no surviving build root or external textual reference outside reports. |
| `Win9x/NP2RES.RC` | Orphan alternate Win9x resource script; no scanned build root or external textual reference outside reports. |

### KEEP

None.

### UNSURE

None.
