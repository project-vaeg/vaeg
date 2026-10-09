<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M90: VA EMS board

## Status

M90 is implementation-complete at candidate
[`498b283`](https://github.com/nakatamaho/vaeg/commit/498b283b67bc0e68dcd6f507260e190974c07f9f)
on `main`. The maintainer passed G90 on 2026-08-13 against the final M90
history at
[`f16d6af`](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9),
and M90 is closed.

The predecessor is the G89-integrated `main` commit
[`c65853c`](https://github.com/nakatamaho/vaeg/commit/c65853cfd2f5ff5318c1a11fec384961037bfdbb).

## Delivered behavior

- `Device / EMS Board...` appears immediately below I/O Bank Memory.
- EMS is disabled by `ExMemory=0`; enabled capacities are 1 through 13MB in
  1MB units, with a clean-configuration default of 1MB.
- Applying a changed capacity persists it and resets the guest. Cancel and an
  unchanged value do not reset the guest.
- The retained `08E1H`, `08E3H`, `08E5H`, `08E7H`, and `08E9H` interface is
  attached to both VA and compatibility I/O dispatch.
- The four 16KB windows remain at `C0000H`, `C4000H`, `C8000H`, and `CC000H`;
  target zero restores ordinary memory mapping. Native VA memory accesses now
  reach the selected EMS pages as well as compatibility-mode accesses.
- EMS and I/O Bank Memory remain independent configuration mechanisms.
- The supplemental-disk builder retains EMMVA15A and RDEMS152 archives,
  installs their three redistributable SYS files, builds the PC-88VA
  `SQEMM98.SYS` manager with pinned Open Watcom, and installs the manuals and
  licenses.
- The generated root `CONFIG.SYS` loads `EMMVA01.SYS`, `SQEMM98.SYS`,
  `EMMVA02.SYS`, and `RDEMS.SYS -P40 -A` in that order. The supplemental
  media retains the PC-Engine 1.1 IPL and four fixed system files and boots
  that complete stack directly.
- SQEMM98 initialization and diagnostic messages use PC-Engine Text BIOS
  `INT 83H/AH=02H`. The generated binary contains no IBM video BIOS `INT 10H`
  and no DOS `INT 21H/AH=09H` string-output path. It preserves the PC-Engine
  loader's caller state and uses validated built-in defaults rather than
  interpreting the incompatible request-header command-tail field.
- CI builds hash-pinned `HOSTFAT.SYS` and `SQEMM98.SYS` from source and adds
  the same two drivers, licenses, instructions, and checksums to every normal
  Linux, macOS, and Windows binary artifact and tagged release package.

## Commit chain

1. [`27fd023`](https://github.com/nakatamaho/vaeg/commit/27fd0238f03fa92f223164fd8c31248be79de9e4)
   defines the M90 task and ROADMAP entry.
2. [`34945a6`](https://github.com/nakatamaho/vaeg/commit/34945a6fdef115acbb4600694848ae4ccfc521ca)
   connects the EMS board, adds GUI/configuration handling, and adds the
   ROM-less mapping/configuration selftest.
3. [`624e74a`](https://github.com/nakatamaho/vaeg/commit/624e74a6560effe324acb6d11c5422043547ba66)
   installs EMMVA/RDEMS in the supplemental-disk workflow and documents the
   required guest stack.
4. [`235e5cb`](https://github.com/nakatamaho/vaeg/commit/235e5cbaf4f49ddd4e6c64f9c44425f7306bae07)
   records the initial EMS validation and G90 handoff.
5. [`ef699ea`](https://github.com/nakatamaho/vaeg/commit/ef699ead3a43b57e5b51f616e277beb9e536851f)
   adds the reproducible PC-88VA SQEMM98 build and validator, routes messages
   through PC-Engine BIOS, and adds the complete stack and `CONFIG.SYS` to the
   supplemental media workflow.
6. [`f5a7fab`](https://github.com/nakatamaho/vaeg/commit/f5a7fab35f77fa57313dbe28ef4ee7465236ae8b)
   records the independently reproduced SQEMM98 and supplemental-media
   identities.
7. [`a4b6170`](https://github.com/nakatamaho/vaeg/commit/a4b6170824dbfb8d602ca1ade19c60778a6eff1c)
   retains the boot-only PC-Engine layout, fixes native VA EMS page-frame
   access, and makes SQEMM98 safe for the PC-Engine device-loader contract.
8. [`34fb837`](https://github.com/nakatamaho/vaeg/commit/34fb8376f988b9c121ade3774f0b038b2d110fb4)
   gives the EMS selftest fresh I/O tables so it does not reuse tables that
   the preceding state-save test destroyed.
9. [`0cb4f22`](https://github.com/nakatamaho/vaeg/commit/0cb4f2247df839e3c40c6061d9d75bcc4f1a0187)
   extends the separately generated PC-88VA development-disk environment.
10. [`081a2bb`](https://github.com/nakatamaho/vaeg/commit/081a2bbffaa14e4bdcc7a9d5627f147bd00f9970)
    expands the EMS selftest to every logical page at the 13MB maximum while
    preserving the pre-test extended-memory allocation.
11. [`5ace0a8`](https://github.com/nakatamaho/vaeg/commit/5ace0a82bde65b9db76c9687f128804a7e997a1a)
    records the 13MB guest validation and stale-binary provenance finding.
12. [`498b283`](https://github.com/nakatamaho/vaeg/commit/498b283b67bc0e68dcd6f507260e190974c07f9f)
    builds and distributes the HOSTFAT/SQEMM98 pair through CI and release
    packaging.

## Validation

| Check | Result |
|---|---|
| UTF-8, LF, and path-case repository checks | PASS; 0 violations/findings |
| `git diff --check` | PASS |
| macOS MacPorts configure/build | PASS; `vaeg` built successfully |
| Linux Debug configure/build | PASS |
| Linux CI GCC configure/build and CTest | PASS; 81/81 tests passed, 1 expected external-SST skip |
| Hosted CI | PASS; final distribution [run 31679653128](https://github.com/nakatamaho/vaeg/actions/runs/31679653128), all 10 jobs succeeded |
| ROM-less `--selftest` | PASS; all 832 logical EMS pages and four native VA page-frame windows passed at 13MB |
| ROM-less `--smoke` | PASS in documented reduced-scope mode |
| MinGW cross configure/build | PASS; PE32+ x86-64 GUI executable |
| SQEMM98 Open Watcom assembly/link | PASS; 13,253-byte `EMMXXXX0` character device |
| SQEMM98 reproducibility | PASS; two independent builds have SHA-256 `eb3d443d7c12b6eb204e03a7ebac4b68a69d4690d0899ca93a37ad7a546d4930` |
| SQEMM98 structural validation | PASS; entry points, caller-state preservation, zeroed memory-test offset, one `INT 83H`, no `INT 10H`, and no DOS `AH=09H` output |
| SQEMM98 validator negative check | PASS; the earlier caller-state-unsafe build is rejected as `SQEMM98_CHECK_CALLER_STATE` |
| Supplemental builder shell syntax | PASS |
| Supplemental media generation | PASS; four fixed system files plus 39 payload files, 222,208 bytes free |
| Supplemental `CONFIG.SYS` | PASS; four expected CRLF lines extracted from the generated D88 |
| Supplemental SQEMM98 identity | PASS; D88 copy matches the independently generated driver byte-for-byte |
| Source-media protection | PASS; disposable source copy was unchanged before/after |
| Reproducibility | PASS; two independently generated outputs were byte-identical |
| Headless PC-Engine boot | PASS; 1MB stack validation passed, then VA1 and VA2 independently reached `Ready` at 13MB after SQEMM98 tested all 832 pages |
| Headless RAM-disk operation | PASS; copied the 114-byte `CONFIG.SYS` to `C:` and read back all four lines |
| 13MB RAM-disk operation | PASS on VA1 and VA2; `dir c:` reported 647,168 bytes available |
| Guest-driver distribution build | PASS; clean NASM/Open Watcom builds produced the pinned HOSTFAT/SQEMM98 pair and six-file bundle |
| Guest-driver bundle manifest | PASS; `HOSTFAT.SYS`, `SQEMM98.SYS`, instructions, both licenses, and `SHA256SUMS` validated |
| Normal hosted artifact layouts | PASS; downloaded Linux GCC, Linux Clang, macOS, Windows release, and Windows compatibility artifacts contain the same validated bundle |
| Tagged release workflow | NOT RUN; no release was published; YAML parsing and local staged Linux/macOS/Windows package-layout validation passed |
| Binary payload audit | PASS; no generated D88, ROM, font, or other binary payload is tracked |

The evaluated macOS executable has SHA-256
`87fe18cab0fd0063f07640656a5a78b997fe5dfb965e09dca053d773768874b0`.
The evaluated MinGW executable at `build/mingw-cross/sdl2/vaeg.exe` has
SHA-256
`1eba1d061add6ee96b675732372d37c61850b15243747d2a17fa12d94ba990d4`.
The Windows MSYS2 release executable from final hosted run `31679653128` has
SHA-256
`0fde2feca9d6b3d442a8db00cb0469d57561c416bb96a43e21cba837a6c74766`.
Its downloaded GitHub artifact ZIP has SHA-256
`a6a0cd8abea301e304d5cb00d2cf64690b70e835ff44fd7f28989ee50cecd7fe`.

The first hosted run after the bootable-media correction,
[31674777821](https://github.com/nakatamaho/vaeg/actions/runs/31674777821),
exposed a test-fixture defect: the state-save selftest destroyed the global
I/O tables, then the EMS selftest called `emsio_bind()` on their stale base
pointers. Linux GCC/Clang segfaulted and ASan identified a heap-use-after-free
in `getextiofunc`; Windows compatibility failed on the same unit-test step.
The corrected behavior candidate builds isolated I/O tables inside the EMS
test and destroys them on cleanup. The three affected local CTests, the
complete 81-test suite, a macOS ASan/UBSan selftest, and all nine jobs in the
then-final hosted run passed after that correction.

A later maintainer-side 13MB run reported the expected 832-page capacity but
then failed SQEMM98's memory test. The reported executable identity did not
match any retained M90 hosted Windows artifact examined. The same supplemental
D88 and `ExMemory=13` configuration pass with behavior candidate `081a2bb` and
its unchanged emulator code in distribution candidate `498b283` on both VA1
and VA2, including a usable RDEMS `C:` drive. A controlled negative build that
removed the native VA page-frame correction from `a4b6170` failed the ROM-less
selftest with `native VA page-frame access did not map`. The observed report is
therefore consistent with an executable that omits that correction; the exact
provenance of the maintainer-side binary remains unresolved, and no new D88 or
SQEMM98 defect was demonstrated.

The generated supplemental media and all raw validation logs remain outside
Git. The maintainer-supplied source media was used only through a disposable
copy; its private checksum is not recorded here. The canonical generated
output has SHA-256
`477d7cd94ccc2a70ffac9e47d8c9cb3f59ba8d1de8183c53239e92cbc96e0cc9`.

The headless boot used a disposable copy of that generated disk. PC-Engine
displayed EMMVA01, SQEMM98, EMMVA02, and RDEMS messages; SQEMM98 reported a
successful full-page memory test and 1.0MB capacity. RDEMS registered its
640KB RAM disk as `C:` because `B:` remains the second floppy. Guest commands
listed `C:`, copied root `CONFIG.SYS` to `C:\M90TEST.TXT`, listed the new
114-byte file, and read all four driver lines back. The disposable D88 was
unchanged after the run.

The 13MB follow-up used two additional disposable copies, one each for VA1 and
VA2. Both copies retained the canonical D88 identity after execution. Each
guest reached `Ready`; `dir c:` then reported 647,168 bytes available, proving
that RDEMS remained registered after SQEMM98's complete 832-page memory test.

## G90 human gate

From a clean checkout and clean configuration:

1. Complete the standard V3-mode, bundled-demo, OS-boot, and simple-operation
   gate.
2. Confirm EMS Board appears below I/O Bank Memory, defaults to 1MB, accepts
   1 through 13MB, persists, and resets only when applying a change.
3. Boot the generated supplemental disk and verify SQEMM98's PC-Engine BIOS
   messages, configured capacity, and distinct data in multiple 16KB pages.
4. Verify `RDEMS.SYS` loads after SQEMM98 and supports RAM-disk read/write.
5. Enable I/O Bank Memory concurrently and verify both mechanisms.
6. Disable EMS Board and verify normal V3/OS operation.

The maintainer explicitly reported G90 passed on 2026-08-13 against
[`f16d6af`](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9).
M90 is closed.
