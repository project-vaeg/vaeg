# PC-88VA archive binary extraction

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This note records which runnable 16-bit files are unpacked from the
development-media archives.  The archive bytes remain available in the
supplemental Softlib disk; extraction only adds a directly runnable copy.

## Development FDD and SASI HDD

`tools/pc88va/build-utility-disk.sh` now unpacks these packages into the
development payload:

| source archive | installed files |
| --- | --- |
| `EMACSVA.LZH` | `A:\BIN\EMACS.EXE` and its help/config files under `A:\DOC` |
| `CPMVA.LZH` | `A:\BIN\CPMBIOS.COM`, `CPMVA.EXE`, `DO.COM`, `EXIT.COM`, `FCONV.COM`, `RDCPM.EXE` |
| `TDC10.LZH` | `A:\BIN\TDC.COM` |
| `BENCH003.LZH` | `A:\BIN\BENCH.EXE` |

The executable files are passed through the existing DIET step.  The payload
D88 is the source for `build-sasi-utility-disks.sh`, so both the VA and
VA2 40 MB SASI images receive the same extracted files.  The SASI builder's
LSI-C and CP/M integrations remain unchanged.

The existing development-disk archives are either source-only, already
expanded by the builder (`JFPPAT`, `2HCDRV`, `ISHARC`, and `TFD`), or are
self-extracting scene packages whose full expansion does not fit the fixed
floppy payload.  No archive-only executable in that payload is silently
omitted by this change.

## Supplemental Softlib FDD

`tools/pc88va/build-softlib-archive-disk.sh` also extracts the runnable files
that were previously present only inside its retained archives:

```text
A:\BIN\EMACS.EXE       A:\BIN\CPMBIOS.COM    A:\BIN\CPMVA.EXE
A:\BIN\DO.COM          A:\BIN\EXIT.COM      A:\BIN\FCONV.COM
A:\BIN\RDCPM.EXE       A:\BIN\TDC.COM       A:\BIN\BENCH.EXE
A:\BIN\2HCDRV.COM      A:\BIN\FDFORM.COM    A:\BIN\VBUFF.COM
A:\BIN\PLUSTAKE.EXE    A:\SYS\JFPPAT.SYS    A:\SYS\RDPCM.SYS
```

`PLUSTAKE.EXE` is the DOS 8.3 name for the `plustakerva.exe` member in
`PRJVA.ZIP`.  The 32-bit Info-ZIP variants and the complete LSI-C tree are not
duplicated on this floppy: the former require a DOS extender and the latter
exceeds the fixed FAT12 free space.  The original archives remain available
under `A:\ARCHIVE`; the complete LSI-C tree is installed on the SASI HDD by
the separate `--lsic-archive` path.

## Reproducibility

Both builders fetch or validate their pinned source archives by SHA-256 and
write generated images outside the repository.  A generated raw D88 is a
private build artifact; only the source scripts and this inventory are
tracked.
