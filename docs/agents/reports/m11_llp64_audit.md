# M11 LLP64 audit

Scope: shared core plus `sdl2/`, looking for FILEH/pointer values stored
through 32-bit `long`/`int`/`UINT32` paths on 64-bit Windows MinGW
(LLP64). The native crash class was reproduced at config load when a
truncated `FILE *` reached `fseek`.

## Search method

Commands used:

```sh
rg -n "\(long\)\s*FILEH|\(FILEH\).*->fh|->fh\s*=\s*\(long\)|long\s+fh|long\s+hdl|\(FILEH\).*hdl|hdl\s*=\s*\(long\)" common fdd generic sdl2 io iova cbus sound vram bios biosva pccore.c statsave.c
rg -n "\(long\)|\(LONG\)|\(UINT32\)|\(int\)" common fdd generic sdl2 io iova cbus sound vram bios biosva pccore.c statsave.c | rg "\*|hdl|fh|ptr|proc|flag|arg|vpItem|fontrom|buf|snd"
```

The first command after the fixes leaves only `generic/hostdrv*`
FILEH-through-`long` hits plus non-handle `common/textfile.h:4`
(`fhpos`, a file offset). The second command is intentionally broad and
includes numeric casts that are not pointer/handle storage.

## Defects fixed

| File:line | Hit | Verdict |
| --- | --- | --- |
| `common/textfile.h:3`, `common/textfile.c:13,16,43,70` | `_TEXTFILE.fh` stored `FILEH` in `long`; `textfile_open()` truncated `FILE *` on LLP64. | Fixed: `_TEXTFILE.fh` is now `FILEH`. `_TEXTFILE` is runtime-only and is not serialized, so no persistence impact. Legacy v141 is ILP32; `FILEH`/`HANDLE` and `long` are both 32-bit there. |
| `common/wavefile.h:38`, `common/wavefile.c:21,29,38,44,56,88,136,138` | `_WAVEWR.fh` stored `FILEH` in `long`. | Fixed: `_WAVEWR.fh` is now `FILEH`. Runtime WAV writer only; no statsave/on-disk structure uses `_WAVEWR`. Legacy v141 effective storage remains 32-bit. |
| `fdd/sxsi.h:91`, `fdd/sxsi.c:65,203,247-249,263-267,280-282,357,364,387,394,420,431` | `_SXSIDEV.fh` stored disk image `FILEH` in `long`. | Fixed: `_SXSIDEV.fh` is now `FILEH`. `statsave.c` saves disk paths and timestamps via `DISKST`, not `_SXSIDEV`, so no state-file layout change. Legacy v141 effective storage remains 32-bit. |
| `statsave.c:138-164` | `proc2num()` / `num2proc()` treated function-pointer fields as `long`. | Fixed: convert through `VAEG_INTPTR`, a pointer-sized runtime type. The serialized field was already pointer-sized in the in-memory structures; this does not change a persisted field type. Legacy v141 maps `VAEG_INTPTR` to `LONG_PTR`, 32-bit under Win32. |
| `statsave.c:536-548` | EGC pointer offsets used `(long)egc.buf`. | Fixed: offset packing/unpacking now uses `VAEG_INTPTR`. This preserves the existing "store offset in pointer field" format for the current architecture without LLP64 truncation. |
| `sdl2/commng.h:62`, `sdl2/commng.c:51`, `generic/cmver.c:323,354`, `generic/cmjasts.c:64`, `statsave.c:1191,1244`, `win9x/commng.h:38` | `COMMNG.msg` used `long` for pointer payloads; `COMMSG_GETFLAG` returned a heap pointer through `long`. | Fixed: the message return/parameter type is `VAEG_INTPTR`. `generic/cmver.c` is used by the portable build; `win9x/commng.h` keeps the legacy declaration type-equivalent on ILP32. |
| `common/lstarray.c:117` | Pointer equality was tested by casting both pointers to `long`. | Fixed: compare pointers directly. No persistence impact. |
| `sound/getsnd/getwave.c:83,112,133` | PCM decode stored a small shift count in `snd->snd` via `(void *)(long)` and read it through `(long)snd->snd`. | Fixed: use `VAEG_INTPTR` for this existing tagged-pointer slot. No persistence impact. |

## Remaining hits and verdicts

| File:line | Hit | Verdict |
| --- | --- | --- |
| `common/textfile.h:4` | `fhpos` is `long`. | Benign: this is a file offset passed to `file_seek(FILEH,long,int)`, not a pointer or handle. |
| `common/_memory.c:149,162` | MEMTRACE prints tracked pointers with `%08lx` and `(long)ptr`. | Benign for M11 runtime: diagnostics only, compiled only with `MEMTRACE`; no pointer is stored or passed back. A future cleanup can switch this to `%p`/pointer-sized formatting if MEMTRACE is revived on 64-bit. |
| `statsave.c:465-477` | `CGWND_FONTPTR` path offsets font pointers through `(long)fontrom`. | Benign in current builds: `CGWND_FONTPTR` is not defined by the portable CMake build or the v141 legacy configuration. If that configuration is ever enabled on LLP64, it should receive the same `VAEG_INTPTR` offset treatment as EGC. |
| `generic/hostdrv.h:13`, `generic/hostdrv.c:353,380,522,523,834,885,1161,1402`, `generic/hostdrvs.c:361,364,365,379,397` | `_HDRVFILE.hdl` stores `FILEH` in `long`. | Persisted-layout blocker, unchanged by instruction. `hostdrv_sfsave()` writes `_HDRVFILE` records with `sizeof(_HDRVFILE)` and `hostdrv_sfload()` reads them back; changing `hdl` to `FILEH` would change the state-file section layout on LLP64. Portable M11 does not define `SUPPORT_HOSTDRV`, so this is not in the MinGW crash path. Legacy v141 remains ILP32 and unaffected. A future HOSTDRV-on-LLP64 change needs a versioned state-section migration or a runtime handle table outside the serialized `_HDRVFILE`. |
| `cbus/scsicmd.c:233`, `fdd/fdd_d88.c:131`, `statsave.c:245,256`, `generic/hostdrv.c:354,381` | Numeric file positions/sizes cast to `long`. | Benign for the searched class: these are file offsets or transfer sizes matching the current `file_seek()` API, not pointer/handle casts. They may still be large-file limits, but not LLP64 pointer truncation. |
| `common/profile.c:564,569,574,586`, `sdl2/ini.c:120,132`, `iova/fdsubsys.c:652,696,724`, `io/fdc.c:702,1620,1912`, `pccore.c:397,412` | Numeric config, DMA/FDC, or memory-size casts to `UINT32`/`int`. | Benign: guest-visible values are 32-bit by design; no host pointer or FILEH is carried. |

## Build observation

After the fixes, `cmake --build --preset mingw-cross --target vaeg_sdl2`
links `sdl2/vaeg.exe`. The previous MinGW warnings of the form
`cast from pointer to integer of different size` / `cast to pointer from
integer of different size` are absent from the rebuilt files. Existing
non-LLP64 warnings remain outside this audit.
