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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98l BMS stream and direct G1 result

Status: **G98l human gate passed on 2026-08-31; M98l closed**

## Result

`M98l RESULT: PASS`

| Internal gate | Automated result |
|---|---|
| G98l-A: reversible BMS mapping probe | PASS |
| G98l-B: bounded one-bank atlas stream | PASS |
| G98l-C: direct BMS-window SGP BITBLT | PASS |

One source-built guest demonstrates the complete path from the public
synthetic `ZUNDORB.BIN` file, through a bounded conventional-memory buffer,
into BMS selector 1, and directly from the selected BMS window into Graphic 1
through the SGP. The independent indexed-GVRAM oracle reported `PASS` with no
errors in VA2 mode. The maintainer subsequently inspected the VA2 result and
explicitly stated that G98l passed on 2026-08-31. This remains emulator
evidence, not physical-machine evidence.

## Git and publication

- Branch: `topic/m98l-zundamon-bms-direct-g1`
- Starting commit: `2a6c3944bab1fb691261fa2f0950dc4a2faeab8c`
- BMS documentation correction:
  `34bbaab8f0683ac8892ee2ecb783cd6c6da407c6`
- Task/consolidation commit:
  `a25e8d44369fa5fc022f6845c28101953c5c8e9a`
- Guest implementation commit:
  `d0a9a1b49a281f1357d99296f43bc06073fd7dd5`
- Exact listing-symbol correction and final evaluated candidate:
  `228f31eb192c2722862691067c46c4db9e4aeb95`
- Push status: all commits through the evaluated candidate were pushed to
  `origin/topic/m98l-zundamon-bms-direct-g1` before this report was written.
- Evidence report commit:
  `ed88898e0f854edae72264a3aed5cca349f263b3`

The gate-closing commit and final pushed remote tip are recorded in the
handoff, because a commit cannot contain its own SHA.

## Files changed

The two task-definition commits changed only the M98 status/task documents and
the already requested BMS selector clarification. The implementation commits
changed these files:

Guest and build:

- `demos/zundamon-orbit/256/build.sh`
- `demos/zundamon-orbit/256/zundamon_orbit_256.asm`
- `demos/zundamon-orbit/build-local-d88.sh`
- `demos/zundamon-orbit/run-vaeg.sh`
- `demos/zundamon-orbit/tools/build_zundamon_orbit_boot_disk.py`
- `demos/zundamon-orbit/zundamon_orbit_m98l.debug`

Host oracle:

- `demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py`
- `demos/zundamon-orbit/tools/verify_zundamon_orbit_guest.py`

Public documentation:

- `demos/zundamon-orbit/README.md`

No emulator source file changed. This result commit adds this report and
advances only the M98 task/master/roadmap status.

## Authoritative BMS contract

The live contract was rechecked in:

- `docs/modernization/GUI-PARITY.md`, which defines selector zero as ordinary
  RAM, selectors 1 through N as 128-KiB BMS banks visible to both CPU and SGP,
  and invalid nonzero selectors as open bus;
- `io/bmsio.h`, which fixes the default port at `01d0h`, the compatibility
  choice at `00ech`, the bank size at `20000h`, and the default count at 128;
- `io/bmsio.c`, which retains the written 8-bit selector, sets the invalid-bank
  state without wrapping, resets to selector zero, and binds only the selected
  configured port;
- `memoryva/memoryva.c`, which routes CPU byte and word access at
  `80000h-9ffffh` to ordinary RAM for selector zero or to the selected one-based
  BMS bank otherwise;
- `io/sgp.c`, which routes the SGP's `08xxxxh-09xxxxh` word access through the
  same selected BMS mapping; and
- `sdl2/selftest.c`, whose BMS configuration/window lifecycle test covers the
  defaults, selected-port behavior, independent banks, reset, and ordinary-RAM
  restoration.

The evaluated configuration and probe result are:

| Item | Value |
|---|---:|
| Selector port | `01d0h`, 8-bit read/write |
| CPU/SGP aperture | `80000h-9ffffh` |
| Logical bank size | 131,072 bytes (`20000h`) |
| Configured capacity | 16,777,216 bytes |
| Configured bank count | 128 |
| Ordinary mapping | selector 0 |
| Independent test selectors | 1 and 2 |
| Last valid selector tested | 128 |
| Boundary selector tested | 129 |
| Boundary behavior | retained selector; reads `ffh`; writes ignored; no wrap |
| Reset/exit mapping | selector 0 |

`00ech` was not treated as an alias. It is a mutually exclusive configuration
choice, while the evaluated run used the native `01d0h` port.

## G98l-A evidence

The guest starts with selector zero, saves eight bytes in ordinary RAM just
below the aperture and eight bytes beneath the aperture, and installs these
guards:

```text
7ffe0h: 5a a5 3c c3 69 96 0f f0
80010h: a5 5a c3 3c 96 69 f0 0f
```

It saves the eight-byte probe range at offset `ffe0h` in selectors 1, 2, and
128; writes three different signatures; reselects and verifies all three;
then selects 129, checks eight open-bus bytes, attempts ignored writes, and
reselects selector 1 to prove that the invalid selection did not alias it.
Every saved BMS byte is restored. Selector zero is restored and both ordinary
guards are verified after the mapping probe, after atlas streaming, and after
the SGP transfer. Their original bytes are restored before page publication
and on all guest error exits.

The successful phase-A checkpoint is `3000:3000` with
`AX=98a1h`, `BX=01d0h`, `CX=0080h`, `DX=0002h`, `SI=a55ah`,
`DI=0081h`, and `BP=0000h`. The checkpoint is reachable only after the three
bank signatures, selector-129 boundary behavior, restored BMS probe bytes,
selector-zero restoration, and both ordinary guards have passed.

## G98l-B evidence

The atlas is the deterministic public fixture generated by the accepted M98j
pipeline. It is a generated, ignored integration artifact; no maintainer input
or private material is involved and no `ZUNDORB.BIN` is tracked.

| Atlas item | Value |
|---|---:|
| Format | little-endian `ZUNDORB` version 1 |
| Indexed format | VA 8-bpp `GGGRRRBB` |
| Transparent index | `00h` |
| Pose/scale count | 1 / 30 |
| Metadata bytes | 1,024 |
| Payload bytes | 4,888 (`1318h`) |
| Complete file bytes | 5,912 (`1718h`) |
| Required bank count | 1 |
| First selector | 1 |
| File SHA-256 | `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa` |
| Payload CRC32 | `0113f299h` |
| File CRC32 | `f2053e49h` |

The complete 4,888-byte payload is only 3.73 percent of the 131,072-byte
logical bank and cannot spill into a second bank. The selected level-30 cell
is 23x19, pitch 24, anchor `(11,9)`, bank slot 0, bank offset `1150h`, file
offset `1550h`, and payload length 456 bytes. Its frame CRC32 is `b88de405h`
and its indexed-pixel SHA-256 is
`30faefca75c76459d8e47371deff06cb8761aa1a48f1199284a6332c80043db0`.

The guest retains only the 1,024-byte metadata table and one 4,096-byte
staging array in conventional memory. The staging array is at `3000:35c0` in
the NASM listing. Two DOS reads transfer 4,096 and 792 payload bytes. Every DOS
call occurs with selector zero; each completed chunk is copied through the
selected BMS aperture; an extra one-byte read proves exact EOF. Incremental
file and payload CRCs match the header, a complete BMS readback CRC matches the
payload, and the selected frame CRC also matches. The phase-B checkpoint
reports `AX=98b1h`, `BX=0002h`, `CX=1318h`, `DX=0000h`, `SI=1000h`,
`DI=1718h`, and `BP=0000h`.

The program contains neither `incbin` nor a generated atlas include. Its
17,600-byte COM consists of executable code, small state, the metadata table,
and the single staging array; it contains no second complete 5,912-byte atlas.
Before any SGP submission the complete staging array is overwritten with
`a5h` and verified. It is verified again after SGP completion. Both poison
checks and both BMS CRC checks must succeed before the transfer checkpoint can
be reached.

## G98l-C evidence

M98l reuses the established 320x200 G0/G1 8-bpp path from M98k and the
256-color pseudo-sprite reference: BIOS mode `e00eh`, pixel-size word `0808h`,
nonzero G0 checkerboard, transparent G1 value zero, 320-byte pitch, G1 page-A
SGP base `220000h`, DSA `020000h`, bounded SGP busy wait, and VBLANK-driven
static idle checkpoints. The six required pseudo-sprite files were consulted;
none of the ray-trace payload bytes was reused.

The sole SGP submission contains one G1 clear and exactly one transparent
BITBLT. Its fixed fields are:

| Descriptor item | Value |
|---|---:|
| Operation | transparent BITBLT `0105h` |
| Source selector | 1 |
| Source address | `081150h` |
| Source geometry | 23x19, pitch 24, 8-bpp |
| Destination | G1 page A at `(148,90)` |
| Destination address | `227114h` |
| Destination pitch | 320 |

The generic SGP trace contains exactly one matching source row and one matching
destination row:

```text
SGP_SCAN: SET_SOURCE addr=081150 dot=0 mode=2 width=23 height=19 fbw=24
SGP_SCAN: SET_DEST addr=227114 dot=0 mode=2 width=23 height=19 fbw=320
```

The CPU does not copy the selected cell to G1. Selector 1 remains selected
from the pre-submission BMS CRC through the bounded SGP completion wait. Only
after completion are the poisoned staging array and BMS payload checked again,
selector zero and ordinary guards restored, and page A published.

The successful transfer checkpoint at `3000:3020` records
`AX=98c1h`, destination `BX=0094h`, `CX=005ah`, mode `DX=0105h`, source
`DI:SI=0008:1150h`, and completion/restoration marker `BP=0101h`. The SGP busy
bound is four outer passes of at most 65,535 status polls. The observed run
completed without a timeout or frame-limit event.

## Oracle and VA2 evidence

The final event frames were 2217 for the mapping checkpoint, 2262 for the
loaded-atlas checkpoint, 2291 for the completed transfer and first settled
capture, and 2292 for the second settled capture. The complete indexed GVRAM
images from frames 2291 and 2292 are byte-identical. The rendered BMPs are also
byte-identical and nonblack.

The oracle result was:

```text
status=PASS
errors=[]
mode=320x200, 8-bpp, G0+G1
bms_source_count=1
sgp_submission_count=1
destination=(148,90)
g1_nonzero_count=73
g1_nonzero_bbox=(149,91)-(169,107)
gvram_stable=true
screen_stable=true
staging=3000:35c0, 4096 bytes, 2 chunks, poison a5h
```

The G1 oracle independently builds the expected 320x400 surface from the
validated atlas cell and transparent-zero rule, while separately constructing
the nonzero G0 background. It therefore rejects wrong offsets, pitches,
layers, transparency, extra copies, stale pixels, dummy all-black output, and
unstable frames. The trace oracle requires the one exact BMS-window source and
G1 destination. The source/listing oracle requires one BITBLT, one SGP
submission, one 4,096-byte staging array, no embedded atlas, and no VA2-invalid
`0F 8xh` near conditional branches.

Ten focused oracle tests start from a passing fixture and isolate probe
signature, G1 pixels, direct source, extra BITBLT, staging-size, VA2 instruction
set, unstable frame, black frame, and timeout failures. The exact staging-label
test also prevents `poison_staging_buffer` from being mistaken for the actual
`staging_buffer` symbol.

## Reproduction and validation

Preflight found the existing build trees rather than assuming one:

```text
./build/mingw-cross/CMakeCache.txt
./build/macos-macports/CMakeCache.txt
```

The selected worker was `build/macos-macports/sdl2/vaeg`, run explicitly in
VA2 mode. Reproduction uses neutral placeholders for local integration paths:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98l-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py

NASM=/opt/local/bin/nasm \
  demos/zundamon-orbit/256/build.sh \
  build/generated/zundamon-orbit/m98l/ZUNDORB.COM \
  build/generated/zundamon-orbit/m98l/ZUNDORB.LST

atlas_root=$(mktemp -d /tmp/vaeg-m98l-atlas.XXXXXX)
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_pipeline.py \
  --fixture-output "$atlas_root"
demos/zundamon-orbit/build-local-d88.sh \
  <local-bootable-2hd-template> \
  "$atlas_root/zundorb.bin" \
  build/generated/zundamon-orbit/m98l/zundamon-orbit-m98l.d88

VAEG_ZUNDAMON_MODEL=va2 demos/zundamon-orbit/run-vaeg.sh \
  <local-bootable-2hd-template> \
  build/macos-macports/sdl2/vaeg \
  <local-rom-directory> \
  build/generated/zundamon-orbit/m98l-va2-final-2

python3 demos/zundamon-orbit/tools/verify_zundamon_orbit_guest.py \
  --atlas build/generated/zundamon-orbit/m98l-va2-final-2/ZUNDORB.BIN \
  --trace build/generated/zundamon-orbit/m98l-va2-final-2/sgp-trace.log \
  build/generated/zundamon-orbit/m98l-va2-final-2

cmake --build build/macos-macports --target vaeg -j2
build/macos-macports/sdl2/vaeg --selftest
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

All ten M98 host test programs passed: 71 tests total, consisting of the 61
pre-existing M98b-M98j tests and the ten M98l oracle tests. The VAEG selftest
ended with
`selftest: all tests passed`, including `VA BMS config/window lifecycle ok` and
`SGP manual commands ok`. The macOS target was up to date. Encoding, EOL, case,
and whitespace checks passed. NASM was 3.01 and CMake was 3.31.12. Two fresh
local D88 builds compared byte-for-byte with each other and with the evaluated
image.

No VAEG change or emulator regression test was needed. During guest bring-up,
the first VA2 trace isolated a guest-only instruction-set problem: a distant
conditional branch had assembled as the 386 `0F 82` encoding. The final guest
declares CPU 286, implements distant conditions as short inverse branches plus
ordinary near jumps, and the oracle rejects any `0F 8xh` opcode in the listing.
The direct BMS/SGP path then passed without an emulator workaround.

## Artifact identities

All artifacts remain ignored below `build/generated/zundamon-orbit/`; none is
tracked or pushed.

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| macOS VAEG worker | 8,155,976 | `88c15c79a7ad7abde048e5afbd9d543003f39353d024d8446fa0a0be58bffc98` |
| `ZUNDORB.BIN` | 5,912 | `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa` |
| `ZUNDORB.COM` | 17,600 | `c38c6c31b7392e903e916b453ba0e73e400fb7c1599433d7130ef015d5d4a1f2` |
| `ZUNDORB.LST` | 106,046 | `2cbbdae8d145816399e3391c52738d5236efe5ac977c35e7a74948ad449c1402` |
| bootable M98l D88 | 1,338,960 | `7441437254fefc5c205dd6670a299e85465826d38b29d184f7f5d5dff2b3eb8a` |
| `events.tsv` | 376 | `f67d1d06ccd75c18d2eb492ac1e56c78ce2d514394253e8e9cc6658f6d261b0f` |
| probe registers | 245 | `f9615c3d33ea032cf26ef6df2c5be074815d8f6bf754afcb40c32b28f96b6c35` |
| load registers | 245 | `0dc25f0cf6a9e910fc3bbf3873d22c767a5f7534608dc481e9a3b1cff4bda085` |
| transfer registers | 245 | `02d9b6e6a7daf96fb1a7eb10263369367add1c79042887bfbe49013882dcb7f5` |
| settled-A GVRAM | 262,144 | `1b4415d6821391cec15e8ebf9df391297bc3ccbe0aa8935bf16cb90b790ab2d7` |
| settled-B GVRAM | 262,144 | `1b4415d6821391cec15e8ebf9df391297bc3ccbe0aa8935bf16cb90b790ab2d7` |
| settled-A BMP | 1,080,442 | `27ad22bafb42c5ee04c2209a1dfbeed5fd93841ad38fde9f1e22961a34c6e97a` |
| settled-B BMP | 1,080,442 | `27ad22bafb42c5ee04c2209a1dfbeed5fd93841ad38fde9f1e22961a34c6e97a` |
| `sgp-trace.log` | 648 | `374e2b273034af1173a35836bf5081ecfe0eeade2b186e47a2601e234a7e50f1` |
| `m98l-oracle.json` | 4,366 | `7d7e86e2ac5fdee07a0b968f229601d1eeefcabb7b5dad653d768686a4c120ed` |

## GUI/headless limitation and preserved state

The automated VA2 run used SDL's software renderer with
`SDL_VIDEODRIVER=dummy`. A created BMP is not considered visual success. PASS
comes from exact indexed GVRAM, BMS/SGP phase signatures, the generic source
trace, bounded completion, and two stable nonblack composed frames. A human
GUI inspection subsequently passed G98l. Physical PC-88VA behavior and timing
remain `REAL_HW_PENDING`.

The pre-existing unrelated dirty and untracked entries were preserved exactly
and were not staged:

```text
 M docs/modernization/bug-fixes.md
 M docs/modernization/pc88va-archive-binary-extraction.md
 M tools/pc88va/build-development-disk.sh
 M tools/pc88va/build-softlib-archive-disk.sh
 M tools/pc88va/stage-development-tools.sh
?? .dosbox-colima-bin/
?? docs/98io/
?? docs/agents/reports/m97f_bms_selected_port.md
?? docs/cpmva/
?? docs/disks/
?? docs/neon/
?? docs/roms/
?? docs/tekumani/
?? tools/pc88va/softlib-fdd-manifest.tsv
?? va2bkupmem.dat
?? vabkupmem.dat
```

## Deferred scope and human gate

M98m and M98n are explicitly reserved because their former scopes were
absorbed into M98l; later milestones retain their numbers. Runtime scale
selection, animation, double buffering, page flipping, multiple objects,
controls, performance measurement, maintainer-supplied artwork, and physical
machine validation remain deferred. No proprietary or ROM-derived material is
tracked.

The maintainer explicitly stated `G98l passed` on 2026-08-31 after the VA2
human-gate run. M98l is closed. M98o remains unassigned and was not started in
this session.
