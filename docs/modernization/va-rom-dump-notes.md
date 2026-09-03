<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# VA ROM Dump Notes and Reference Checks

This document records the current author-confirmed ROM identities and the
VA1-specific read uncertainty observed while dumping `varom00.rom`. It does
not distribute ROM data.

## Current dumps confirmed by the author

These are the current author-confirmed ROM dumps. VA1 uses the unsuffixed
filenames; VA2/VA3 uses the `_va2` filenames for its model ROMs. The extra
subsystem ROM uses the common runtime filename `vasubsys.rom` for both models.

| Model | ROM filename | SHA-1 | MAME SHA-1 match? |
|---|---|---|---|
| VA1 | `vadic.rom` | `5ba1f3578d0aaacdaf7194a80e6d520c81ae55fb` | Yes |
| VA2/VA3 | `vadic_va2.rom` | `3665db538598abb45d9dfe636423e6728a812b12` | Yes |
| VA1 | `vafont.rom` | `a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f` | No |
| VA2/VA3 | `vafont_va2.rom` | `a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f` | Yes |
| VA1 | `varom00.rom` | `e7fc344b12ab0573a5229c7b43feb64bd329e57b` | No |
| VA2/VA3 | `varom00_va2.rom` | `bcaea28c58816602ca1e8290f534360f1ca03fe8` | Yes |
| VA1 | `varom08.rom` | `7e6591cd465cbb35d6d3446c5a83b46d30fafe95` | Yes |
| VA2/VA3 | `varom08_va2.rom` | `47e5f89f8b0ce18ff8d5d7b7aef8ca0a2a8e3345` | Yes |
| VA1 | `varom1.rom` | `54536dc03238b4668c8bb76337efade001ec7826` | Yes |
| VA2/VA3 | `varom1_va2.rom` | `dd4f4521bfbb068f15ab3bcdb8d47c7d82b9d1d4` | Yes |
| VA1 | `vasubsys.rom` | `a9375aa480f85e1422a0e1385acb0ea170c5c2e0` | Yes |
| VA2/VA3 | `vasubsys.rom` | `a9375aa480f85e1422a0e1385acb0ea170c5c2e0` | Yes |

The `No` entries are the known VA1 author-readback identities that differ
from MAME's declared SHA-1 values. VAEG uses those author-confirmed VA1
values for its ROM-set check. The MAME comparison in this table is SHA-1 only.

The VA and VA2/VA3 ROM sets are not interchangeable. Do not create a
`*_va2.rom` dump by renaming an unsuffixed file.

## Dumping source and handling

ROMs must be dumped from hardware owned by the operator. Tools such as
`getromva` are available in `VAEGTOOL070422.LZH` from the
[project-vaeg r080406 release](https://github.com/project-vaeg/vaeg/releases/tag/r080406).
The dump should be kept as a separate source artifact, hashed before any
normalization, and compared with the identities above. Keep ROM bytes and
private dump files outside the public source tree.

## Uncertainty of `varom00.rom` (VA1 only; not applicable to VA2/VA3)

This section applies only to the unsuffixed VA1 `varom00.rom`. It does not
apply to `varom00_va2.rom` or to the VA2/VA3 ROM set.

### Sample identity

The original dated private dump names are intentionally normalized to neutral
sample labels. The comparison glob excluded `docs/roms/varom00.rom`, which is
the selected working copy of `sample3`.

| Sample | SHA-1 |
|---|---|
| `sample1` | `732cd6b78762466036e608082fabc03df3f869e2` |
| `sample2` | `2595bab29556d4efebb6608d70b31fb651eb7789` |
| `sample3` | `e7fc344b12ab0573a5229c7b43feb64bd329e57b` |
| MAME-derived reference | `1266ba969959ff25433ecc900a2caced26ef1a9e` |

The MAME-derived reference is recorded by SHA-1 only. No CRC, byte values,
byte-level comparison, or difference locations for that reference are
recorded here.

### Majority result and working reference (VA1 only; not applicable to VA2/VA3)

For the three author read samples, `sample3`
(`e7fc344b12ab0573a5229c7b43feb64bd329e57b`) is the selected majority-result
SHA-1 and current working reference. At every observed read-to-read
difference, the `sample3` byte agrees with one of the other two samples. This
is a reproducibility choice, not proof of a perfect electrical capture.

The selected `sample3` image is 512 KiB with CRC-32 `df7f8a74` and the SHA-1
shown above. These are the full-image values used by the current VA1 ROM-set
check. The selected working reference does not match the MAME-derived
reference by SHA-1.

### Populated-bank reference (VA1 only)

`varom00.rom` is a 512 KiB ROM0 image divided into eight 64 KiB banks. The
selected per-bank SHA-1 values for populated banks 0 through 5 are:

| ROM0 bank | SHA-1 |
|---:|---|
| 0 | `35d37a6a1ecf70025a9d1f9a892f3a4d1f6e1d62` |
| 1 | `da5eaedcf34259a406946984e23e80c3fbf9d1a1` |
| 2 | `e3e3ed4a7e0241dcd669b07c2d7b29a4c94744c2` |
| 3 | `67fc27525e2ced658925289b3736002320f3dcdd` |
| 4 | `463e586b9911bc6fc35f79f0a0bd3a43414460a0` |
| 5 | `1d225f958bdc4719e83873d2a66515622d6b2dc0` |

For reference, the combined SHA-1 of banks 0 through 5 (`0x00000` through
`0x5FFFF`) is
`24a7a5091846ec1f177c97711e4837573bc40b42`.

### Reserved-bank observation (VA1 only)

The selected `varom00.rom` dump has the following contents in ROM0 banks 6
and 7:

- bank 6: `0x60000–0x6ffff`
- bank 7: `0x70000–0x7ffff`
- both banks are byte-for-byte identical
- the entire contents are the repeating pattern `AA 0E AA 0E ...`
- each bank is 65536 bytes
- each bank has SHA-1 `c13a76090102c5ac408ed0cbbcbe3707f8234786`

The beginning and end of both banks are the same:

```text
aa 0e aa 0e aa 0e aa 0e ...
...
aa 0e aa 0e aa 0e aa 0e
```

In this dump, banks 6 and 7 appear to be fixed-pattern regions rather than
meaningful code or data.

### Observed read-to-read differences (VA1 only)

`sample1` and `sample2` differ at eight byte positions. The eight positions
below are the union of the two reported four-position comparisons with
`sample3`; the original byte values are not recorded here.

| Pair | Difference count | File offsets and ROM0 bank-local offsets |
|---|---:|---|
| `sample1` vs `sample2` | 8 bytes | bank 6: `0x6448`, `0x6BF2`, `0x852F`, `0xA9F5`, `0xD976`; bank 7: `0x7344`, `0x7AF6`, `0xB18E` |
| `sample1` vs `sample3` | 4 bytes | file `0x66BF2` (bank 6 + `0x6BF2`), `0x6852F` (bank 6 + `0x852F`), `0x6A9F5` (bank 6 + `0xA9F5`), `0x77AF6` (bank 7 + `0x7AF6`) |
| `sample2` vs `sample3` | 4 bytes | file `0x66448` (bank 6 + `0x6448`), `0x6D976` (bank 6 + `0xD976`), `0x77344` (bank 7 + `0x7344`), `0x7B18E` (bank 7 + `0xB18E`) |

All reported read-to-read differences are in banks 6 and 7. The bank number
is the file offset divided by `0x10000`.

### Interpretation and VAEG check behavior

The PC-88VA technical manual's ROM area 0 selector table assigns selectors 0
through 5 to `ROM00` through `ROM05`, and marks selectors 6 and 7 as reserved:
[PC-88VA technical manual ROM area 0 table](../tekumani/PC88VA_テクニカルマニュアル_BNN.md:10869).

The observed read-to-read variation is therefore consistent with changes in
the positions documented as reserved. For this VA1 capture set, banks 6 and 7
are not required to match when the populated banks 0 through 5 match. This
does not prove that every timing-dependent variation is harmless or establish
the electrical or sampling mechanism.

When VA1 is selected, VAEG first checks the full `varom00.rom` SHA-1. If it
differs from the selected working reference, VAEG calculates the SHA-1 of
banks 0 through 5, reports the differing populated banks, and points back to
this document. VA2/VA3 does not use this VA1-specific uncertainty rule.
