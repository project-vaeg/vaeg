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

# M98q page-local dirty-row CLS result

Status: **G98q human gate passed on 2026-09-01; M98q closed**

## Result

`M98q RESULT: PASS`

`G98q FINAL STATUS: PASS`

The M98p full-page baseline reproduced before this change. The M98q renderer
then completed four bounded 116-publication VA2 cases: `A/full`, `A/dirty`,
`B/full`, and `B/dirty`. Every dirty physical G1 page and composed 320x200
frame matched its full-clear counterpart byte-for-byte. Both dirty runs used
exactly two initialization full clears, zero steady-state full clears, 1,069
row CLS commands, 8,407 words, and 16,814 bytes. No framebuffer, guard,
descriptor, SGP, VBLANK, or cleanup mismatch remained.

On 2026-09-01, the maintainer ran `ZUNDORB` in VA2, confirmed that there was
no stale image, horizontal one-pixel streak, anchor wobble, or flicker, and
confirmed successful ESC restoration. The maintainer then explicitly passed
G98q. This remains VAEG/VA2 evidence; physical PC-88VA evidence remains
`REAL_HW_PENDING`.

## Git and predecessor

- Branch: `topic/m98q-zundamon-dirty-rows`
- Starting and accepted M98p gate commit:
  `05df2d2d069f00b8b5d99d80dfc4979d4482757b`
- Accepted M98p implementation:
  `4e9c57975a2e3705bc7cb2c29b3b94e5b88f4bea`
- Accepted M98p report head:
  `7b0102bddf3734d7d440892b3753231033578a17`
- M98q task-definition commit:
  `82249b306da1f1f2ff58036c2fc8803f4cbb6e42`
- Evaluated M98q implementation:
  `6a3f229c74d1ffed9888b279e80334ac76d2e461`
- Report/pushed-head commit: recorded in the final handoff because this file
  cannot contain its own commit SHA.

The maintainer explicitly stated `G98p passed` before M98q began. M98q does
not recreate the direct-transfer work completed by G98l-C. M98m and M98n
remain absorbed reservations, and later milestone numbers are unchanged.

## Preserved dirty-worktree baseline

The following unrelated entries existed before M98q and were neither staged
nor changed by this milestone:

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

All generated M98q binaries, D88 images, GVRAM captures, traces, and reports
remain ignored below `build/generated/zundamon-orbit/`.

## Changed files

| File | Purpose |
|---|---|
| `docs/agents/ROADMAP.md` | Record the automated result without advancing M98r. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Reconcile M98q and retain M98m/M98n reservations. |
| `docs/agents/tasks/M98q_zundamon_dirty_rows.md` | Define the fixed M98q scope and gate. |
| `docs/agents/reports/m98q_zundamon_dirty_rows.md` | Record this evidence and pending human gate. |
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Add independent page rectangles, row CLS batching, transactions, and counters. |
| `demos/zundamon-orbit/256/build.sh` | Build release and bounded full/dirty A/B variants. |
| `demos/zundamon-orbit/build-local-d88.sh` | Build a non-overwriting interactive M98q disk. |
| `demos/zundamon-orbit/run-vaeg.sh` | Run one bounded VA2 comparison against a selected full golden. |
| `demos/zundamon-orbit/README.md` | Document the dirty-row and QA workflows. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_dirty_debug.py` | Generate bounded two-cycle capture scripts. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_dirty_guest.py` | Verify GVRAM, composition, trace order, counters, and full/dirty equality. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_dirty_guest.py` | Cover geometry, page state, batching, transactions, and negative cases. |
| `io/sgp.c` | Extend the existing opt-in generic SGP trace with CLS address and word count. |
| `sdl2/debug_harness.c` | Raise the bounded debug-script capacity and self-test a 301-action script. |

No guest-visible emulator behavior was changed. The two emulator-side changes
are generic diagnostics/QA capacity needed to distinguish and capture the
bounded row-CLS sequence.

## Display, page, and BMS contract

- Mode: 320x200 VA direct-color 8-bpp (`GGGRRRBB`).
- G1 pitch and backing surface: 320 bytes, 320x400.
- Page A: SGP `220000h`, DSA1 `020000h`, 64,000 bytes.
- Page B: SGP `22fa00h`, DSA1 `02fa00h`, 64,000 bytes.
- DSA1: word ports `022eh`/`0230h`, values in byte-address units.
- Guards: host oracle checks the complete 262,144-byte captured GVRAM and
  rejects changes before A, between A/B, or beyond B-owned ranges.
- BMS: base port `01d0h`, selector 0 ordinary RAM, selector 1 atlas, aperture
  `080000h-09ffffh`, one 128 KiB bank.
- Atlas: version 1, 5,912 bytes, 4,888 occupied payload bytes, exactly 30
  descriptors, SHA-256
  `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa`.

Every SGP source is `080000h + descriptor.bank_offset`. The guest restores
selector 0 after each completed draw and on cleanup. Bank selection occurs
only while the SGP is idle and is held through BITBLT completion.

## CLS semantics and batching

The live SGP CLS interface takes a physical byte address and an exact 32-bit
word count. The count is not an inclusive terminal index: the implementation
writes one word, advances the destination by two bytes, and decrements the
count until zero. Thus the half-open logical rectangle is converted once:

```text
x0 = dst_x
x1 = dst_x + width
clear_x0 = x0 & ~1
clear_x1 = (x1 + 1) & ~1
clear_words = (clear_x1 - clear_x0) / 2
row_address = page_base + y * 320 + clear_x0
```

The guest command buffer holds 64 words. Six fixed list words plus five words
per row allow at most 11 row CLS commands per list. The largest public frame
is 19 rows, so it uses at most two bounded clear lists. All lists complete
before the one new transparent BITBLT. The page remains hidden throughout.

## Page-local state and transaction

Each physical page owns `valid_old_rect`, logical `x`, `y`, `width`, `height`,
`old_scale_id`, and publication count. Both pages are fully cleared during
initialization and begin invalid. The first hidden use therefore skips dirty
clearing; later uses consult only that page's prior published rectangle.

A pending rectangle is calculated before the draw but is committed only after
all old rows and the BITBLT complete, a fresh VBLANK low-to-high edge is
observed, and DSA1 publishes the page. Only then are page roles swapped and
the scale sequence advanced. Any failure retains the prior visible page,
discards the pending rectangle, leaves the sequence unchanged, and enters the
single cleanup path.

## Descriptor destinations and rounded clear spans

The fixed target anchor is `(160,100)`. The table shows the descriptor's new
logical destination, the outward-rounded half-open X interval used when that
rectangle later becomes dirty, its per-row word count, and its BMS source.

| ID | Size | Destination | Rounded X | Words/row | BMS source |
|---:|---:|---:|---:|---:|---:|
| 1 | 1x1 | 160,100 | `[160,162)` | 1 | `080000h` |
| 2 | 1x1 | 160,100 | `[160,162)` | 1 | `080010h` |
| 3 | 2x2 | 159,99 | `[158,162)` | 2 | `080020h` |
| 4 | 3x2 | 159,99 | `[158,162)` | 2 | `080030h` |
| 5 | 4x3 | 158,99 | `[158,162)` | 2 | `080040h` |
| 6 | 4x4 | 158,98 | `[158,162)` | 2 | `080050h` |
| 7 | 5x4 | 158,98 | `[158,164)` | 3 | `080060h` |
| 8 | 6x5 | 157,98 | `[156,164)` | 4 | `080080h` |
| 9 | 7x6 | 157,97 | `[156,164)` | 4 | `0800b0h` |
| 10 | 7x6 | 157,97 | `[156,164)` | 4 | `0800e0h` |
| 11 | 8x7 | 156,97 | `[156,164)` | 4 | `080110h` |
| 12 | 9x7 | 156,97 | `[156,166)` | 5 | `080150h` |
| 13 | 10x8 | 155,96 | `[154,166)` | 6 | `0801b0h` |
| 14 | 10x9 | 155,96 | `[154,166)` | 6 | `080210h` |
| 15 | 11x9 | 155,96 | `[154,166)` | 6 | `080280h` |
| 16 | 12x10 | 154,95 | `[154,166)` | 6 | `0802f0h` |
| 17 | 13x10 | 154,95 | `[154,168)` | 7 | `080370h` |
| 18 | 13x11 | 154,95 | `[154,168)` | 7 | `080410h` |
| 19 | 14x12 | 153,94 | `[152,168)` | 8 | `0804c0h` |
| 20 | 15x12 | 153,94 | `[152,168)` | 8 | `080580h` |
| 21 | 16x13 | 152,94 | `[152,168)` | 8 | `080640h` |
| 22 | 16x13 | 152,94 | `[152,168)` | 8 | `080710h` |
| 23 | 17x14 | 152,93 | `[152,170)` | 9 | `0807e0h` |
| 24 | 18x15 | 151,93 | `[150,170)` | 10 | `080900h` |
| 25 | 19x15 | 151,93 | `[150,170)` | 10 | `080a30h` |
| 26 | 19x16 | 151,92 | `[150,170)` | 10 | `080b60h` |
| 27 | 20x17 | 150,92 | `[150,170)` | 10 | `080ca0h` |
| 28 | 21x17 | 150,92 | `[150,172)` | 11 | `080e00h` |
| 29 | 22x18 | 149,91 | `[148,172)` | 12 | `080fa0h` |
| 30 | 23x19 | 149,91 | `[148,172)` | 12 | `081150h` |

All rectangles are within 320x200. Atlas occupied end is `01318h`, below the
one-bank boundary `20000h`; no frame crosses the aperture.

## Full-clear reproduction and bounded VA2 evidence

Before implementation, both accepted 58-publication M98p parity runs were
rerun unchanged. Their counters and every settled page identity matched the
accepted M98p report. One initial capture had only a nondeterministic settled
frame-number gap; the immediate bounded retry produced consecutive identical
settled frames and passed without changing guest or emulator code.

The final four 116-publication cases used identical atlas bytes, sequence,
target anchor, background, page order, VBLANK gates, and BITBLTs. Only the
steady clear mode differed.

| Counter | A/full | A/dirty | B/full | B/dirty |
|---|---:|---:|---:|---:|
| render starts/completions | 116/116 | 116/116 | 116/116 | 116/116 |
| initial full-page clears | 2 | 2 | 2 | 2 |
| steady full-page clears | 116 | 0 | 116 | 0 |
| dirty rectangle clears | 0 | 114 | 0 | 114 |
| dirty row CLS commands | 0 | 1,069 | 0 | 1,069 |
| transparent BITBLTs | 116 | 116 | 116 | 116 |
| page flips | 116 | 116 | 116 | 116 |
| page A/B publications | 59/58 | 59/58 | 58/59 | 58/59 |
| cycles/reversals | 2/4 | 2/4 | 2/4 | 2/4 |
| shrink/grow publications | 60/56 | 60/56 | 60/56 | 60/56 |
| BMS selections/switches | 116/232 | 116/232 | 116/232 | 116/232 |
| source bytes | 18,136 | 18,136 | 18,136 | 18,136 |
| SGP command lists | 117 | 275 | 117 | 275 |
| SGP commands | 817 | 2,128 | 817 | 2,128 |
| dirty/full mismatches | 0 | 0 | 0 | 0 |
| guard failures | 0 | 0 | 0 | 0 |
| SGP/VBLANK errors or timeouts | 0 | 0 | 0 | 0 |
| cleanup runs | 1 | 1 | 1 | 1 |

The initial visible-page publication is counted separately; it produces the
59/58 or 58/59 split over 116 measured flips. All 30 scale IDs, both physical
pages, both directions for IDs 2-29, both endpoint reversals, first use, page
reuse, and the second-cycle 29-to-30 wrap matched exactly.

Settled frame pairs were consecutive, identical, and non-black:

| Case | Settled frames | Oracle SHA-256 |
|---|---:|---:|
| A/full | 2561, 2562 | `93e24260f7bafce646ca9ab4dc32c6884ea321f32cdbde69907e058f4ded59ba` |
| A/dirty | 2434, 2435 | `73e23c6dfd70e79265197f49b7ae4369ca4e29bbc11529c1d3c4045eff8fb507` |
| B/full | 2527, 2528 | `edf2a8ca04b672e392af81bce5636e5689a799c21b4924fe9377308a9bc49e13` |
| B/dirty | 2421, 2422 | `0418d33b6896ae06a528df43cebef6ea6c891ffaa2e9d41537a19d8f723d66f0` |

The oracle compares every 64,000-byte physical G1 page, complete composited
frame, transparent-hole samples, old-only shrink regions, immediate outside
pixels, GVRAM guards, page identity, and publication order. It reported zero
mismatches across all corresponding full/dirty publications.

## Logical clear accounting

Each 116-frame full baseline clears:

```text
116 * 64000 = 7,424,000 bytes = 3,712,000 words
```

Each dirty run clears 1,069 rows totaling 16,814 bytes or 8,407 words. The
dirty logical volume is 0.226481681% of the steady full-clear baseline, a
99.773518319% reduction. This measures logical work only. It is not an
elapsed-speed or physical-hardware performance claim; per-row command overhead
may offset some or all of the volume reduction.

## Negative and fail-closed coverage

The 109 focused host tests start from passing public fixtures and cover all
required cases:

1. global old-state reuse and A/B state swapping are rejected by page-local
   golden comparisons;
2. zero, negative, overflowing, and out-of-bounds old rectangles fail before
   submission;
3. odd/even X with odd/even width, one-pixel/one-row cases, and X boundaries
   0 and 320 produce checked word spans;
4. inclusive terminal-count substitution and scanline/page address overflow
   are detected;
5. command-capacity overflow is split at 11 rows per list;
6. first, middle, and last dirty-batch faults, BITBLT failure, and premature
   BITBLT all retain the visible page;
7. BMS change while busy, VBLANK-low/high timeout, partial publication,
   visible-page write, early rectangle commit, and early scale advance all
   retain page/scale state and run cleanup once;
8. stale old-only pixels, over-clear outside the rounded interval, unexpected
   steady full CLS, and dirty/full framebuffer mismatch are detected.

Every modeled failure has zero publication/sequence advancement, retains the
prior visible page, restores the ordinary BMS selector and video state, and
records one cleanup. Fault controls exist only in host tests; the release
guest contains no runtime bypass.

## Build, test, and reproducibility results

The principal commands were:

```sh
cmake --build build/macos-macports --target vaeg -j4
build/macos-macports/sdl2/vaeg --selftest
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98q-pycache \
  python3 -m unittest discover -s demos/zundamon-orbit/tools -p 'test_*.py'
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
sh -n demos/zundamon-orbit/256/build.sh \
  demos/zundamon-orbit/build-local-d88.sh demos/zundamon-orbit/run-vaeg.sh
```

The macOS CMake build and VAEG selftest passed. The demo test suite passed all
109 tests. Encoding, EOL, lowercase-path, shell-syntax, Python compile, and
`git diff --check` checks passed. The accepted tree has existing whole-tree
clang-format findings outside the M98q edit spans; M98q did not reformat those
unrelated lines, while its changed C/C++ lines pass the pinned formatter.

The four VA2 cases were built and run through `run-vaeg.sh` with
`VAEG_ZUNDAMON_CLEAR_MODE=full` or `dirty`, opposite initial pages, two QA
cycles, and the matching full golden directory. All four returned
`M98Q_ORACLE_PASS`.

Two clean release builds were byte-identical:

- `ZUNDORB.COM`: 22,320 bytes, SHA-256
  `8f416e0063f0e5f24eec5226b44c0750e8e49a8fad96444878c0e8e3269a077e`.
- `ZUNDORB.LST`: 183,186 bytes, SHA-256
  `57ac4c9927ee4c5dbb4efae8a67908ce77208df5888ddebb0ce7fac07206097f`.
- VAEG executable used: 8,155,976 bytes, SHA-256
  `13109ea163c6c708e0e79df0b149b1ff317c04381f9ad119e90ac35bc8e73d46`.

The pristine interactive candidate is:

```text
build/generated/zundamon-orbit/m98q-va2-candidate/
zundamon-orbit-m98q-pristine.d88
```

It is 1,338,960 bytes with SHA-256
`cb6bee28e8a2dd55961dd3a183144945957edcb6c22bcf3893535452a63f93c4`.
The local builder refused an attempted overwrite. The image is ignored and
not tracked.

An interactive release build ran for at least three complete scale cycles in
VA2 before a clean injected-exit smoke check. It retained two initialization
full clears, zero steady full clears, and zero reported error counters. Manual
ESC behavior remains part of the human gate rather than an automated claim.

## Artifact identities

| Case | Events SHA-256 | SGP trace SHA-256 | QA guest SHA-256 |
|---|---|---|---|
| A/full | `1b76960515bf51cb045dc6d101947c2ef3d063362c67c29a8441a0434c0860a2` | `c86aadea6c2b13e1bbfd8b02605891db1664f23a45d4b903b4531a9c08c4fa64` | `e18e370cb040a1b0a4afce3c3fbc417f35971ee932fe0e8340a50cd4509117b5` |
| A/dirty | `4e0a80e2c82fffed88440f739e5e70fb38080b051e3049459e8c661bf0a7617b` | `c8661e62d2fdffe9c301c49c924fdd447453a932f6151385248b2adba0847649` | `5ebcd69b9ae43b6bc19a3ab6d5dffb6db0074b320bceec3a0cf00626f5ca58ff` |
| B/full | `2d1b66cd0423127d4620f2c97fae1a91f6cc1c238079c294a51e88d53fd92f5a` | `0d436ae40a73384dd16ccd9db1593c1bf9d6cefb508c37a3ef5450cd01f0a9d9` | `658cdfcc4039535843819ace2ebd98b13ac9fd569bda78ed85a5f008eb599565` |
| B/dirty | `2576d9345f5f3224231284dcc5116597bcafaba6b36394d2388900c3f52f9e9c` | `350af1f3081427889846cedd278d3d2c209623cc5df6b5332d9e56ced8fe51fc` | `5a7e6e33881b617568c196a0d11ba1b4c9dbb7254bf7a607bb68af6bcbe04eaa` |

No `.COM`, `.BIN`, D88, screenshot, GVRAM capture, trace, save state, backup
RAM, private asset, or ROM-derived material is tracked by M98q.

## Human gate and limitations

The maintainer launched the pristine D88 in VA2 with the accepted ROM
selection and ran:

```text
ZUNDORB
```

The observed centered 30-to-1-to-29 shrink/grow cycle had no stale larger
silhouette, horizontal one-pixel streak, anchor wobble, or flicker. ESC
returned normally with display restoration. This closes the human gate.

M98q supports exactly one homogeneous G1 object. It adds no cadence selector,
orbit/depth behavior, private image integration, multiple instances, dirty-row
interval unions, or physical-hardware performance evidence. M98r and later
work remain unassigned.

`REAL_HW_PENDING`

`G98q FINAL STATUS: PASS`
