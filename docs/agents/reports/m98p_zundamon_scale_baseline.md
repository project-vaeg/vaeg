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

# M98p 30-scale full-page-CLS zoom baseline result

Status: **G98p human gate passed on 2026-08-31; M98p closed**

## Result

`M98p RESULT: PASS`

`G98p FINAL STATUS: PASS`

The public synthetic atlas completed the exact 58-publication
`30..1..29` sequence in two bounded VA2 runs with opposite initial visible
pages. All 116 scale/page observations matched the independent indexed-GVRAM
oracle. Each update cleared the complete hidden 64,000-byte G1 page, issued
one transparent direct-BMS BITBLT, completed the SGP batch, observed a fresh
VBLANK low-to-high edge, and then published DSA1. No timeout, descriptor,
trace, framebuffer, counter, or stable-frame mismatch remained.

On 2026-08-31, the maintainer confirmed that the candidate displayed the
expected single-marker shrink-and-grow sequence and explicitly stated
`G98p passed`. M98p is closed. This remains VAEG evidence; physical PC-88VA
evidence remains `REAL_HW_PENDING`.

## Git and predecessor

- Branch: `topic/m98p-zundamon-scale-baseline`
- Starting commit: `543e06114a63c5f7c9f678806d11c221da96ed94`
- Accepted M98o implementation:
  `ddc70c692ecb65066269c9894eb4b14f702fd2d9`
- Accepted M98o report head:
  `71bcdf3467a26dc4eaeb5ca0167fe9e01a26ef20`
- M98o closure/human-gate record:
  `543e06114a63c5f7c9f678806d11c221da96ed94`
- M98p task-definition commit:
  `77bd0920aa001ac1d79a52485306bff6159bf2fe`
- Evaluated M98p implementation:
  `4e9c57975a2e3705bc7cb2c29b3b94e5b88f4bea`
- Report/pushed-head commit: recorded in the final handoff because this file
  cannot contain its own commit SHA.

The maintainer explicitly passed G98o before M98p began. G98l-B already owns
atlas streaming, so M98p did not recreate the obsolete standalone loader
milestone. M98m and M98n remain absorbed reservations and later milestones
retain their numbers.

## Preserved dirty-worktree baseline

These unrelated entries existed before M98p and were not staged or changed by
this milestone:

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

All M98p runtime artifacts remain ignored below
`build/generated/zundamon-orbit/`.

## Changed files

| File | Purpose |
|---|---|
| `docs/agents/ROADMAP.md` | Record M98p assignment and automated result without renumbering. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Reconcile M98p and retain the M98m/M98n reservations. |
| `docs/agents/tasks/M98p_zundamon_scale_baseline.md` | Define the task, fixed contract, and human gate. |
| `docs/agents/reports/m98p_zundamon_scale_baseline.md` | Record this evidence and pending gate. |
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Validate and render the 30-level anchored zoom cycle. |
| `demos/zundamon-orbit/256/build.sh` | Build release or bounded A/B QA variants deterministically. |
| `demos/zundamon-orbit/build-local-d88.sh` | Build the interactive M98p disk without overwrite. |
| `demos/zundamon-orbit/run-vaeg.sh` | Run one bounded VA2 parity proof and the M98p oracle. |
| `demos/zundamon-orbit/README.md` | Document the scale baseline and local workflows. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_scale_debug.py` | Generate deterministic one- or two-cycle debug scripts. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_scale_guest.py` | Independently verify all scale/page publications. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_scale_guest.py` | Cover descriptors, sequence, parity, clearing, and fail-closed behavior. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py` | Keep the historical M98o oracle test isolated from the advancing guest source. |

No emulator source changed. The first VA2 run exposed a generic SGP descriptor
construction error in the new M98p code: odd destination X coordinates had
DOT zero and were aligned one pixel left. Encoding `dst_x & 1` in the 8-bpp
DOT field and using the even word address made the exact framebuffer and trace
oracles pass. This was corrected before the evaluated implementation commit.

## Atlas and descriptor contract

The public version-1 atlas is 5,912 bytes with 4,888 payload bytes. It has one
pose, exactly 30 descriptors, required-bank count 1, first selector 1, and
SHA-256
`7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa`.
Implicit descriptor positions are scale IDs 1 through 30; IDs 0 and 31 do not
exist. Every descriptor uses logical bank slot 0 and ends below the 128 KiB
boundary.

| ID | Dimensions | Pitch | Anchor | Bank offset | Bytes | Frame CRC | Destination | SGP source |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 1x1 | 4 | 0,0 | `0000h` | 4 | `2144df1c` | 160,100 | `080000h` |
| 2 | 1x1 | 4 | 0,0 | `0010h` | 4 | `2144df1c` | 160,100 | `080010h` |
| 3 | 2x2 | 4 | 1,1 | `0020h` | 8 | `6522df69` | 159,99 | `080020h` |
| 4 | 3x2 | 4 | 1,1 | `0030h` | 8 | `771f2c43` | 159,99 | `080030h` |
| 5 | 4x3 | 4 | 2,1 | `0040h` | 12 | `f8966049` | 158,99 | `080040h` |
| 6 | 4x4 | 4 | 2,2 | `0050h` | 16 | `edecfb25` | 158,98 | `080050h` |
| 7 | 5x4 | 8 | 2,2 | `0060h` | 32 | `30598022` | 158,98 | `080060h` |
| 8 | 6x5 | 8 | 3,2 | `0080h` | 40 | `b700b50b` | 157,98 | `080080h` |
| 9 | 7x6 | 8 | 3,3 | `00b0h` | 48 | `9335343d` | 157,97 | `0800b0h` |
| 10 | 7x6 | 8 | 3,3 | `00e0h` | 48 | `9335343d` | 157,97 | `0800e0h` |
| 11 | 8x7 | 8 | 4,3 | `0110h` | 56 | `e85ba94c` | 156,97 | `080110h` |
| 12 | 9x7 | 12 | 4,3 | `0150h` | 84 | `65adc694` | 156,97 | `080150h` |
| 13 | 10x8 | 12 | 5,4 | `01b0h` | 96 | `3ffc90cb` | 155,96 | `0801b0h` |
| 14 | 10x9 | 12 | 5,4 | `0210h` | 108 | `ae6d5fd6` | 155,96 | `080210h` |
| 15 | 11x9 | 12 | 5,4 | `0280h` | 108 | `4553da3a` | 155,96 | `080280h` |
| 16 | 12x10 | 12 | 6,5 | `02f0h` | 120 | `b5b2b73a` | 154,95 | `0802f0h` |
| 17 | 13x10 | 16 | 6,5 | `0370h` | 160 | `6b721af6` | 154,95 | `080370h` |
| 18 | 13x11 | 16 | 6,5 | `0410h` | 176 | `41c460a9` | 154,95 | `080410h` |
| 19 | 14x12 | 16 | 7,6 | `04c0h` | 192 | `cb50668a` | 153,94 | `0804c0h` |
| 20 | 15x12 | 16 | 7,6 | `0580h` | 192 | `0f976ff1` | 153,94 | `080580h` |
| 21 | 16x13 | 16 | 8,6 | `0640h` | 208 | `8a214f3d` | 152,94 | `080640h` |
| 22 | 16x13 | 16 | 8,6 | `0710h` | 208 | `8a214f3d` | 152,94 | `080710h` |
| 23 | 17x14 | 20 | 8,7 | `07e0h` | 280 | `74556ddb` | 152,93 | `0807e0h` |
| 24 | 18x15 | 20 | 9,7 | `0900h` | 300 | `dc9de828` | 151,93 | `080900h` |
| 25 | 19x15 | 20 | 9,7 | `0a30h` | 300 | `257290fb` | 151,93 | `080a30h` |
| 26 | 19x16 | 20 | 9,8 | `0b60h` | 320 | `fc64f750` | 151,92 | `080b60h` |
| 27 | 20x17 | 20 | 10,8 | `0ca0h` | 340 | `ac385920` | 150,92 | `080ca0h` |
| 28 | 21x17 | 24 | 10,8 | `0e00h` | 408 | `08d1f421` | 150,92 | `080e00h` |
| 29 | 22x18 | 24 | 11,9 | `0fa0h` | 432 | `f123c54a` | 149,91 | `080fa0h` |
| 30 | 23x19 | 24 | 11,9 | `1150h` | 456 | `b88de405` | 149,91 | `081150h` |

The fixed target anchor is `(160,100)`. Thus every row above satisfies
`destination + stored anchor == target anchor`. Source row padding is excluded
from the BITBLT width. Adjacent small levels that share dimensions remain
distinct descriptors and publications.

## Renderer, BMS, and publication ordering

The accepted display geometry remains 320x200 direct-color 8-bpp over a
320x400 G1 surface with 320-byte pitch. Page A uses SGP `220000h` and DSA1
`020000h`; page B uses SGP `22fa00h` and DSA1 `02fa00h`. Each page is 64,000
bytes. DSA1 is programmed through word ports `022eh` and `0230h` in byte
address units.

BMS uses base port `01d0h`, selector 0 for ordinary RAM, selector 1 for the
public atlas, and the physical `080000h-09ffffh` aperture. All descriptors use
selector 1. For every update the guest waits for SGP idle, selects and counts
the descriptor bank, emits one full-page CLS and one transparent BITBLT in one
bounded command list, waits for completion, verifies staging poison and the
selected frame CRC, restores selector 0 and normal guards, waits for VBLANK
low then high, and only then writes DSA1. It never changes BMS while SGP is
busy and never advances the sequence before publication.

The exact sequence is:

```text
30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,
15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,
2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
17,18,19,20,21,22,23,24,25,26,27,28,29
```

Direction 0 is shrinking and direction 1 is growing. Publishing ID 1 changes
the state to grow; publishing the final ID 29 completes the cycle, changes the
state back to shrink, and selects 30 for the next publication. This convention
records two direction reversals per completed bounded cycle without
duplicating either endpoint.

## Bounded VA2 evidence

Run A started with page A visible and first rendered page B. Run B started
with page B visible and first rendered page A. Both used the same atlas,
background, positions, sequence, and SGP operations.

| Counter | Run A | Run B |
|---|---:|---:|
| pages initialized | 2 | 2 |
| render starts/completions | 58 / 58 | 58 / 58 |
| full-page clears | 58 | 58 |
| transparent BITBLTs | 58 | 58 |
| page flips | 58 | 58 |
| VBLANK edges including initial and settled | 61 | 61 |
| page A publications | 30 | 29 |
| page B publications | 29 | 30 |
| cycles / reversals | 1 / 2 | 1 / 2 |
| BMS selections / actual switches | 58 / 116 | 58 / 116 |
| source bytes | 9,068 | 9,068 |
| cleared bytes | 3,712,000 | 3,712,000 |
| shrink / grow publications | 30 / 28 | 30 / 28 |
| SGP timeout / error | 0 / 0 | 0 / 0 |
| VBLANK timeout / descriptor error | 0 / 0 | 0 / 0 |
| cleanup runs | 1 | 1 |

Both oracles reported 58 exact publication records, 58 direct-BMS SGP source
records, 58 exact hidden-page destinations, zero mismatches, stable indexed
settled frames, and stable nonblack composed screens. Interior scale IDs 2
through 29 occur in both directions; endpoints 30 and 1 occur once. Reversing
the initial visible page gives every scale/direction occurrence the opposite
physical page and proves every scale on both page parities.

A separate release-build smoke run completed two full cycles and one
additional publication before the harness-delivered Return was observed. It
reported 117 complete CLS/BITBLT/publication batches, two cycles, four
reversals, no error, one cleanup, and 117 direct-BMS trace sources. Return is
the established automated clean-exit equivalent; the human gate still
requires the real ESC path.

## Negative and deterministic tests

The public standard-library suite reports 93 tests. M98p adds 22 focused test
methods covering:

- wrong descriptor count or order and invalid scale IDs 0 and 31;
- zero/excessive dimensions, pitch underflow, payload mismatch, invalid
  anchor/destination, loaded-range overflow, BMS boundary crossing, and frame
  CRC failure;
- exact sequence length/order, endpoint duplication, scale skip, direction
  counts, and opposite-parity coverage;
- fixed-anchor placement and full-clear removal of the larger silhouette; and
- fail-closed models for BMS switch while busy, CLS timeout, BITBLT error,
  VBLANK-low timeout, VBLANK-high timeout, early DSA1 publication, visible-page
  write, and sequence advance without publication.

Each runtime fault retains the previous DSA1, does not advance the scale,
publishes no partial page, restores selector 0 and video state, and runs common
cleanup once. The atlas inspector independently covers malformed header
counts, canonical geometry, row/file padding, layout, and CRCs. The guest also
validates every live descriptor and frame CRC before graphics mode.

## Commands and results

The evaluated commands, written with neutral local placeholders, were:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98p-tests-pyc \
  python3 -m unittest discover \
    -s demos/zundamon-orbit/tools -p 'test_*.py'

NASM=/opt/local/bin/nasm demos/zundamon-orbit/256/build.sh \
  <fresh-a>/ZUNDORB.COM <fresh-a>/ZUNDORB.LST
NASM=/opt/local/bin/nasm demos/zundamon-orbit/256/build.sh \
  <fresh-b>/ZUNDORB.COM <fresh-b>/ZUNDORB.LST
cmp <fresh-a>/ZUNDORB.COM <fresh-b>/ZUNDORB.COM
cmp <fresh-a>/ZUNDORB.LST <fresh-b>/ZUNDORB.LST

VAEG_ZUNDAMON_MODEL=va2 VAEG_ZUNDAMON_INITIAL_PAGE=a \
  demos/zundamon-orbit/run-vaeg.sh \
    <local-bootable-2hd-template> build/macos-macports/sdl2/vaeg \
    <local-rom-directory> build/generated/zundamon-orbit/m98p-va2-page-a

VAEG_ZUNDAMON_MODEL=va2 VAEG_ZUNDAMON_INITIAL_PAGE=b \
  demos/zundamon-orbit/run-vaeg.sh \
    <local-bootable-2hd-template> build/macos-macports/sdl2/vaeg \
    <local-rom-directory> build/generated/zundamon-orbit/m98p-va2-page-b

build/macos-macports/sdl2/vaeg --selftest
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

The 93 tests passed. Both VA2 oracles passed. VAEG selftest ended with
`selftest: all tests passed`. Encoding, EOL, case, shell syntax, Python syntax,
and whitespace checks passed. A second attempt to build the same D88 returned
status 1 with the required no-overwrite refusal.

## Reproducibility and generated artifacts

Two fresh release builds produced byte-identical guest and listing files. Two
fresh public pipelines produced the same atlas.

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| release `ZUNDORB.COM` | 22,208 | `ec8af52ee29dda9eb28bdbd863639f33a429c5a41cb2338519519374d03a9c41` |
| release `ZUNDORB.LST` | 157,059 | `b6dedf8087bfcd16030418118fc6595be452edb9c2c3d217e3c906a93b8085aa` |
| public `ZUNDORB.BIN` | 5,912 | `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa` |
| QA-A `ZUNDORB.COM` | 22,208 | `6f819d13c6be0f3e6b412ce9eea585e74b27eb0fd7ac9df751f59fe2b7610884` |
| QA-B `ZUNDORB.COM` | 22,208 | `aa8aa6d943cbc640a514311176200dbb9297472042f3ef26dd77f5788f8076ca` |
| Run-A oracle | 61,075 | `2b0890db85dc31f0c94a10a5417d746973312f162c612611a440a8047d1507c8` |
| Run-B oracle | 61,075 | `a98bf0ad45d14bb2dff94c50236af36bc16f2f080cbf7ff51b5a8e3d488a3757` |
| Run-A event log | 4,111 | `4eb69f956a7e5ccf72e7311e50f6ce048b3e3fafa542c01a8cb4076f767ce0e5` |
| Run-B event log | 4,111 | `0c5575648d44c1a7fc0862442676223cca87ee8d11be921764acdecb64c882f6` |
| Run-A SGP trace | 8,675 | `dc0b5055b9dc29f3029c866f9a8c011baed111a180b6268a7177b6a9593a3737` |
| Run-B SGP trace | 8,675 | `ae39ead13361cf807103f55d87c3347f968e5bf4d97c6cfc8a9c89e9fba9e85a` |
| Run-A settled indexed GVRAM | 262,144 | `efbc4a786984ca24e55ebd875d32205f075aae35ab61f27b7d1c9b42f0c3c5b2` |
| Run-B settled indexed GVRAM | 262,144 | `10d425316eb161fefdedf8095e5d8d1afc4aa28466660b06b85de4d3a62dd0af` |
| settled composed BMP, both runs | 1,080,442 | `98f128cacc4d1bc2c248b88616e4ec08ad0df924920808014e14de33f7ab4402` |
| interactive candidate D88 | 1,338,960 | `76483bfeb1eed00f9d31ccd4b752109fc0695b1cfe2907ce81356da0974bc324` |

The interactive candidate is generated and untracked at:

```text
build/generated/zundamon-orbit/m98p-va2-candidate/
zundamon-orbit-m98p-pristine.d88
```

The line break above is only for readability; it is one repository-relative
path. `git status` and `git ls-files` found no generated COM, listing, atlas,
D88, capture, trace, save state, backup RAM, or private asset added by M98p.

## Human gate and limitations

Launch the candidate in VA2, run `ZUNDORB`, and verify at least two cycles:

1. the public marker shrinks from scale 30 through 1 and grows through 29;
2. neither endpoint pauses from a duplicated publication;
3. the fixed anchor does not wobble;
4. larger silhouettes do not remain after shrinking;
5. transparent holes reveal the nonzero checkerboard G0;
6. no clear-only or partially drawn page appears;
7. no flicker, tearing, or A/B parity difference is visible; and
8. ESC restores the previous display and returns normally.

The smallest center-sampled public fixture levels are transparent, so the
marker briefly reaches zero visible G1 pixels at the extreme small end; the
oracle still proves those distinct descriptors and publications. This is an
accepted property of the G98j public atlas, not a skipped scale.

SDL dummy video was used as deterministic transport for automated evidence.
Exact indexed GVRAM and SGP traces, not PNG existence, establish the automated
result. The maintainer subsequently inspected the interactive candidate,
confirmed the intended single-marker scale behavior, and explicitly passed
G98p. M98p adds no dirty-row clearing, cadence selector, orbit, depth coupling,
private image, multiple instances, or performance claim. Physical hardware
was not tested: `REAL_HW_PENDING`.

Final status: **G98p human gate passed; M98p closed**. M98q was not started.
