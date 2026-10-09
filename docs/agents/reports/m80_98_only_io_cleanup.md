<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

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

# M80: 98-only I/O cleanup report

## Status

M80 is closed after the G80 human gate passed on 2026-08-11. The
implementation checkpoint is
[`a1291121604af6ca27c690214bed337704976fbe`](https://github.com/nakatamaho/vaeg/commit/a1291121604af6ca27c690214bed337704976fbe).
The candidate was approved at G80 and is merged to `main` by the
gate-closure commit.

The candidate starts from M79/`main` at
[`1e19c4c539fd99dcc7dcd4a92770a51aef93aad1`](https://github.com/nakatamaho/vaeg/commit/1e19c4c539fd99dcc7dcd4a92770a51aef93aad1).

## Evidence boundary

The PC-98 reference material in `docs/98io` was decoded from its original
CP932 text for this audit. The PC-88VA technical material in `docs/tekumani`
was decoded from Shift-JIS for this audit. These source documents were used
read-only and were not copied into the candidate branch.

The active dispatcher has two maps. `iomode_va` selects the VA map; the
common map remains relevant when the compatibility path selects it. Therefore
a device being documented as a PC-98 device is not, by itself, proof that its
common-map implementation is unreachable from the VA product.

## Candidate decisions

| Candidate | Decision | Evidence and reason |
| --- | --- | --- |
| `epsonio` | Already absent | M72 already removed `io/epsonio.c/.h`; no active source or CMake entry remains. |
| `nmiio` | Removed | `docs/98io/io_nmi.txt` describes PC-9800/PC-H98 NMI flip-flop ports 0050h/0052h. The active implementation only stored `nmiio.enable`; no active source read that field. Its only remaining references were the lifecycle table, CMake, and its own state-save entry. |
| `emsio` | Retained | `docs/98io/io_mem.txt` identifies the EMS board targets, but the implementation changes active CPU EMS frame mappings through `CPU_SETEMM`. The common-map/V1-V2 compatibility dependency is not disproven. |
| `printif` | Retained | The 98 reference documents a common 0040h printer interface. The VA manual documents the VA printer at 0010h/0040h, while the source has a separate VA `sysportva` map and a common `printif` map. Removing the common handler without a V1/V2 mode proof would be speculative. |
| `necio` | Retained | The implementation changes `CPU_ITFBANK`, and the active BIOS memory path consumes that bank for the F8000h-FFFFFh range. It cannot be removed as an inert registration. |
| `artic` | Retained | The callback is called from the active `pccore_exec` frame loop, and the dispatcher has an explicit ARTIC 005Ch-005Fh word-read path. `docs/98io/io_tstmp.txt` also documents this counter interface. |
| `fdd320` | Deferred | The task explicitly defers this candidate. `docs/98io/io_2d.txt` documents 320KB/2D FDD hardware, and the VA material documents 5-inch 2D support, so a separate PC-88-side audit is required. |

## State-save compatibility

The removed `NMIIO` entry was a raw four-byte binary section. The current
state loader already treats an unknown section as a warning and continues.
For an explicit compatibility check, a state produced by the candidate
selftest was copied to a disposable test artifact and a four-byte `NMIIO`
section was inserted immediately before `NP2SYSPORT`. The candidate loader
accepted that state with a warning rather than `STATFLAG_FAILURE`; the
remaining selftest completed successfully. New states no longer emit
`NMIIO`.

No ARTIC, EMSIO, or NECIO state section was removed.

## Validation

The following completed on the M80 candidate:

```text
cmake --preset linux-debug                         PASS
cmake --build --preset linux-debug --clean-first -j4 PASS
build/linux-debug/sdl2/vaeg --selftest             PASS (all tests passed)
ctest --test-dir build/linux-debug --output-on-failure
  No tests were found
synthetic old-NMIIO state load                      PASS (warning, not failure)
python3 tools/repo/check_encoding.py               PASS (0 findings)
python3 tools/repo/check_eol.py                    PASS
python3 tools/repo/check_case.py                   PASS
python3 tools/qa/upd9002_rename.py                 PASS
rg nmiio/NMIIO/_NMIIO in active source             PASS (no references)
git diff --check                                   PASS
cmake --preset mingw-cross                         PASS
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4 PASS
  build/mingw-cross/sdl2/vaeg.exe: PE32+ x86-64
  SHA-256: 883c4b9ac8a92ae475efeb22114f6f3efbc91c80cc3ab25e6394cb41a97e2c65
```

## Gate status

G80 human gate passed by the maintainer on 2026-08-11. The required
clean-checkout V3 boot, bundled VA demo, OS/device operation, and human review
were completed. The candidate is approved and merged to `main`.
