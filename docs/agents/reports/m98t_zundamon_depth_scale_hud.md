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

# M98t depth/scale coupling and G0 HUD result

Status: **G98t human gate passed; M98t closed on 2026-09-01**

## Result

`M98t AUTOMATED RESULT: PASS`

`G98t PASS`

M98t keeps the accepted 64-phase clockwise ellipse and couples each phase to
the required signed depth rank and one of all 30 stored public atlas scales.
Every publication uses that descriptor's own anchor, dimensions, pitch,
payload, and direct BMS source. The G0 panel displays the applied nominal FPS
selector and fixed `ZUNDAMON: 1`; G1 still contains exactly one transparent
pseudo-sprite.

The four full/dirty parity cases, eight static cadence cases, opposite-page
long runs, dynamic HUD ladder, pause/resume, consecutive misses, host tests,
and repository checks passed. This is VAEG evidence in VA2 mode. Physical
PC-88VA/VA2 evidence remains `REAL_HW_PENDING`.

## Git and predecessor

- Branch: `topic/m98t-depth-scale-hud`
- Starting and accepted M98s pushed head:
  `cf542bff4265272f2fd563b10d159b8e65c74966`
- M98s implementation: `3cce7e7b93171bb0fdaf31af9997ce9ae6ad63c4`
- M98s table-validator commit: `5e2e3bb1dc6fa6efcc0722a9244a36bffd16c1f9`
- M98s report commit: `cf78033533e330655357fd3f365379c6ff0b4681`
- M98s accepted human-gate head:
  `cf542bff4265272f2fd563b10d159b8e65c74966`
- M98t implementation:
  `9440798d13bd00229b03163f98f9fee7c4caac68`
- Report/pushed-head commit: supplied in the final handoff because this file
  cannot contain the SHA of the commit that contains itself.
- Accepted predecessor report:
  `docs/agents/reports/m98s_zundamon_64_phase_ellipse.md`

The maintainer explicitly stated `G98s passed` before assigning M98t. The
local and remote M98s branch resolved to the same accepted predecessor.

## Preserved dirty-worktree baseline

These unrelated entries existed before M98t and were not staged, reformatted,
or overwritten:

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

The existing `bug-fixes.md` worktree edit was preserved. M98t adds only one
separate final ledger entry for the demonstrated guest stack imbalance; the
report commit stages that isolated hunk, not the pre-existing edits. All
generated M98t output remains ignored below `build/generated/zundamon-orbit/`.

## Changed files

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Select phase-specific scale/depth, validate destinations, update G0 HUD, preserve transactional page state, and balance the atlas-CRC ES save. |
| `demos/zundamon-orbit/256/zundamon_depth_table.inc` | Store the deterministic 64-entry offset/depth/scale table. |
| `demos/zundamon-orbit/256/zundamon_hud_table.inc` | Store the eight public full-HUD and fixed-width FPS tiles. |
| `demos/zundamon-orbit/256/build.sh` | Regenerate/validate both includes and build release/QA variants. |
| `demos/zundamon-orbit/build-local-d88.sh` | Build a non-overwriting interactive M98t D88. |
| `demos/zundamon-orbit/run-vaeg.sh` | Run one bounded VA2 M98t case and independent oracle. |
| `demos/zundamon-orbit/README.md` | Document depth coupling, HUD semantics, controls, and regeneration. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_depth_table.py` | Generate the exact depth/scale formula and deterministic safe radii. |
| `demos/zundamon-orbit/tools/validate_zundamon_orbit_depth_table.py` | Independently validate phase, depth, scale, atlas, bounds, and HUD exclusion. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_hud.py` | Generate the task-authored public 5x7 G0 HUD tiles. |
| `demos/zundamon-orbit/tools/validate_zundamon_orbit_hud.py` | Independently reconstruct and validate every HUD tile. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_depth_debug.py` | Generate bounded capture/checkpoint scripts. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_depth_guest.py` | Compare registers, trace, indexed G0/G1, composition, pages, HUD, and scheduler. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_depth_guest.py` | Test formulas, generated identities, descriptors, HUD, scheduler, page state, and fail-closed cases. |
| `docs/agents/ROADMAP.md` | Record automated PASS and pending human gate. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Reconcile M98t depth plus HUD without changing later numbering. |
| `docs/agents/tasks/M98t_zundamon_depth_scale_hud.md` | Record the assigned task and gate. |
| `docs/modernization/bug-fixes.md` | Record the demonstrated bounded-loader ES stack restoration in an isolated staged hunk. |
| `docs/agents/reports/m98t_zundamon_depth_scale_hud.md` | Record this result. |

No emulator source changed.

## Phase, depth, scale, and atlas contract

The generator applies exactly:

```text
round29(q) = floor((29*q + 16) / 32)
phase 16..48: scale = 30 - round29(phase - 16)
otherwise:    scale = 1 + round29((phase - 48) modulo 64)
depth_rank = 2*scale - 31
```

The exact sequence is:

```text
16,16,17,18,19,20,21,22,23,24,25,25,26,27,28,29,
30,29,28,27,26,25,25,24,23,22,21,20,19,18,17,16,
15,15,14,13,12,11,10,9,8,7,6,6,5,4,3,2,
1,2,3,4,5,6,6,7,8,9,10,11,12,13,14,15
```

There are 58 cyclic scale-change edges. Per revolution, scales 1 and 30 occur
once; 6 and 25 occur four times; 15 and 16 occur three times; all others
occur twice. All 30 IDs occur; 0 and 31 do not.

The public atlas is version 1, 5,912 bytes total and 4,888 payload bytes,
requires one 128 KiB bank, and has SHA-256
`7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa`.
Descriptor dimensions are nondecreasing. The complete descriptor table is:

| ID | WxH | Pitch | Anchor | Payload | Bank/offset | SGP source | CRC32 |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 1x1 | 4 | (0,0) | 4 | 0/0000h | 080000h | `2144df1c` |
| 2 | 1x1 | 4 | (0,0) | 4 | 0/0010h | 080010h | `2144df1c` |
| 3 | 2x2 | 4 | (1,1) | 8 | 0/0020h | 080020h | `6522df69` |
| 4 | 3x2 | 4 | (1,1) | 8 | 0/0030h | 080030h | `771f2c43` |
| 5 | 4x3 | 4 | (2,1) | 12 | 0/0040h | 080040h | `f8966049` |
| 6 | 4x4 | 4 | (2,2) | 16 | 0/0050h | 080050h | `edecfb25` |
| 7 | 5x4 | 8 | (2,2) | 32 | 0/0060h | 080060h | `30598022` |
| 8 | 6x5 | 8 | (3,2) | 40 | 0/0080h | 080080h | `b700b50b` |
| 9 | 7x6 | 8 | (3,3) | 48 | 0/00b0h | 0800b0h | `9335343d` |
| 10 | 7x6 | 8 | (3,3) | 48 | 0/00e0h | 0800e0h | `9335343d` |
| 11 | 8x7 | 8 | (4,3) | 56 | 0/0110h | 080110h | `e85ba94c` |
| 12 | 9x7 | 12 | (4,3) | 84 | 0/0150h | 080150h | `65adc694` |
| 13 | 10x8 | 12 | (5,4) | 96 | 0/01b0h | 0801b0h | `3ffc90cb` |
| 14 | 10x9 | 12 | (5,4) | 108 | 0/0210h | 080210h | `ae6d5fd6` |
| 15 | 11x9 | 12 | (5,4) | 108 | 0/0280h | 080280h | `4553da3a` |
| 16 | 12x10 | 12 | (6,5) | 120 | 0/02f0h | 0802f0h | `b5b2b73a` |
| 17 | 13x10 | 16 | (6,5) | 160 | 0/0370h | 080370h | `6b721af6` |
| 18 | 13x11 | 16 | (6,5) | 176 | 0/0410h | 080410h | `41c460a9` |
| 19 | 14x12 | 16 | (7,6) | 192 | 0/04c0h | 0804c0h | `cb50668a` |
| 20 | 15x12 | 16 | (7,6) | 192 | 0/0580h | 080580h | `0f976ff1` |
| 21 | 16x13 | 16 | (8,6) | 208 | 0/0640h | 080640h | `8a214f3d` |
| 22 | 16x13 | 16 | (8,6) | 208 | 0/0710h | 080710h | `8a214f3d` |
| 23 | 17x14 | 20 | (8,7) | 280 | 0/07e0h | 0807e0h | `74556ddb` |
| 24 | 18x15 | 20 | (9,7) | 300 | 0/0900h | 080900h | `dc9de828` |
| 25 | 19x15 | 20 | (9,7) | 300 | 0/0a30h | 080a30h | `257290fb` |
| 26 | 19x16 | 20 | (9,8) | 320 | 0/0b60h | 080b60h | `fc64f750` |
| 27 | 20x17 | 20 | (10,8) | 340 | 0/0ca0h | 080ca0h | `ac385920` |
| 28 | 21x17 | 24 | (10,8) | 408 | 0/0e00h | 080e00h | `08d1f421` |
| 29 | 22x18 | 24 | (11,9) | 432 | 0/0fa0h | 080fa0h | `f123c54a` |
| 30 | 23x19 | 24 | (11,9) | 456 | 0/1150h | 081150h | `b88de405` |

The descriptor bank field is retained and validated; all frames use logical
slot 0 in selected BMS bank value 1. No source range crosses `020000h`.

## Ellipse geometry and publication state

The accepted M98s radii `(96,48)` pass every variable-descriptor bound and HUD
exclusion check, so M98t makes zero radius adjustments. The fixed-point orbit
generator remains deterministic. The depth-table include is 6,333 bytes with
SHA-256 `645414752dd68898fb382d70d49dcfc4975b722f2927670d45fd8496a036b09c`.

| Phase | Offset | Scale/depth | Destination | Meaning |
|---:|---:|---:|---:|---|
| 0 | `(96,0)` | `16/+1` | `[250,95,262,105)` | right/near midpoint |
| 16 | `(0,48)` | `30/+29` | `[149,139,172,158)` | bottom/nearest |
| 32 | `(-96,0)` | `15/-1` | `[59,96,70,105)` | left/far midpoint |
| 48 | `(0,-48)` | `1/-29` | `[160,52,161,53)` | top/farthest |

All 64 descriptor-specific half-open rectangles fit 320x200, fit both hidden
G1 page ranges, and do not intersect `[4,4,70,20)`. Every destination aligns
the descriptor's own anchor to `(160+dx,100+dy)`. Phase 63 publishes scale 15
and then wraps exactly to phase 0/scale 16.

Each physical page independently saves its last published logical rectangle,
phase, depth, and scale. The hidden page clears its saved old rectangle with
outward 16-bit-word rounding, completes all row CLS batches, then issues one
transparent BITBLT from the phase-selected BMS source. Only a READY page is
published on an eligible fresh low-to-high VBLANK edge. Publication atomically
commits page state and advances phase once; pause, miss, divisor change, and
failure commit nothing.

The inherited display/BMS contract remains:

- G1 pitch 320 bytes; two 64,000-byte pages at SGP `220000h` and `22fa00h`.
- DSA1 byte-address values `020000h` and `02fa00h` through word ports
  `022eh`/`0230h`.
- BMS selector port `01d0h`; selector 0 ordinary RAM; selector 1 atlas;
  aperture `080000h-09ffffh`.
- Transparent source byte `00h`; BITBLT mode `0105h`.
- The BMS selection and command storage remain stable while SGP is busy;
  cleanup restores selector 0.

## G0 HUD

The font is a task-authored public 5x7 bitmap set, not ROM- or firmware-
derived. A 6x8 cell is used for space, period, colon, digits, and the required
uppercase letters. The checked include is 51,424 bytes with SHA-256
`3b9f41f2425c5fa35320fc48ba804fae6af60f8619f85dd0d1d5aa8adc0ebf93`.

- HUD rectangle: `[4,4,70,20)`, 66x16.
- FPS value rectangle: `[34,4,52,12)`, 18x8.
- Foreground/background: `ffh`/`01h`, both nonzero VA direct-color bytes.
- Full initialization: 1,056 bytes; exactly once.
- Applied divisor update: 144 bytes; exactly the complete FPS value field.
- Fields: `60 `, `30 `, `20 `, `15 `, `12 `, `10 `, `8.6`, `7.5`.
- `ZUNDAMON: 1` is initialized once and never changes; UP/DOWN are inactive.
- HUD writes use the bounded direct-G0 path during VBLANK. They issue no SGP
  command and write no G1 byte.

The dynamic V1-to-V8-to-V1 ladder applied 14 changes, performed 15 logical
FPS writes total, and wrote `1056 + 14*144 = 3072` HUD bytes. Clamped controls,
pause, and misses caused no HUD update. Every captured G0 byte outside the HUD
matched the accepted background; `hud_vblank_overruns`, `hud_g1_writes`, and
`hud_mismatches` were zero.

## Full/dirty and page-parity evidence

Each case below published phases `0..63,0..63`, produced the exact twice-
histogram, 64 near and 64 far publications, two initialization full clears,
128 transparent BITBLTs, and 128 page flips.

| Case | Publications | Missed | CLS commands | Dirty rows/words/bytes | Result |
|---|---:|---:|---:|---:|---|
| A/full | 128 | 0 | 130 | 0/0/0 | PASS |
| A/dirty | 128 | 0 | 1,202 | 1,200/9,446/18,892 | PASS |
| B/full | 128 | 0 | 130 | 0/0/0 | PASS |
| B/dirty | 128 | 0 | 1,202 | 1,200/9,446/18,892 | PASS |

Every corresponding A/full-A/dirty and B/full-B/dirty captured 262,144-byte
GVRAM state matched byte-for-byte for all 128 publications. The independent
oracle also matched each complete physical G1 page, G0 HUD, G0 outside HUD,
composite framebuffer, descriptor/source, transparent samples, guards, and
publication order. There were zero dirty/full mismatches and guard failures.

Dirty mode cleared 18,892 steady-state bytes versus the equivalent
`128*64,000 = 8,192,000` full-clear bytes, a 99.769% logical-byte reduction.
This is work-volume accounting only, not an elapsed-time or physical-hardware
performance claim.

## Cadence and dynamic evidence

The VAEG VA2 reference used by the inherited oracle is 59.95 Hz. Nominal HUD
labels, requested rates, and observed VAEG publication rates remain distinct:

| Option | HUD | Publications | Missed | Requested/observed Hz | Revolution seconds |
|---|---:|---:|---:|---:|---:|
| `/V1` | 60 | 64 | 0 | 59.950 | 1.068 |
| `/V2` | 30 | 64 | 0 | 29.975 | 2.135 |
| `/V3` | 20 | 64 | 0 | 19.983 | 3.203 |
| `/V4` | 15 | 64 | 0 | 14.988 | 4.270 |
| `/V5` | 12 | 64 | 0 | 11.990 | 5.338 |
| `/V6` | 10 | 64 | 0 | 9.992 | 6.405 |
| `/V7` | 8.6 | 64 | 0 | 8.564 | 7.473 |
| `/V8` | 7.5 | 64 | 0 | 7.494 | 8.540 |

Opposite-page V1, V4, and V8 each published 128 frames and two exact
revolutions. The ladder applied changes only at VBLANK boundaries, replaced
the complete HUD field, reset the divider, and preserved phase/scale
continuity. Pause scenarios at IDLE/RENDERING/READY retained the complete
visible phase, position, scale, and HUD, then waited a full divisor interval.
The missed-slot scenario recorded exactly two misses, retained DSA1 and the
pending transaction, and later published the same unskipped phase/scale.

For every drained segment:

```text
requested_slots == published_updates + missed_slots
published_updates == page_flips == phase_advances
table_scale_change_edges == 58
steady_full_page_clears == 0
partial_publication_attempts == 0
all SGP/VBLANK/bounds/guard/HUD error counters == 0
cleanup_runs == 1
```

## Fail-closed and regression evidence

Sixteen M98t-focused tests supplement the inherited 143 demo tests. Mutated
depth tables and HUD tiles assert stable, specific validator codes. Fifty
distinct modeled failure classifications cover all required phase/depth/scale
formula, landmark, histogram, descriptor, BMS range, anchor, radius, bounds,
HUD intersection/font/tile/update, page ownership, old-rectangle, CLS,
BITBLT, BUSY, READY, VBLANK, pause/miss, full-clear, oracle, guard, and ESC
failure cases.

Each failure retains the prior complete visible page, commits no rectangle,
phase, depth, scale, or DSA1 change, restores ordinary selector 0 and video
state, and runs cleanup exactly once. Test-only ladder, pause, and miss
injection does not exist as a release runtime option.

The larger M98t guest exposed an inherited unmatched `ES` save in the bounded
atlas header-CRC path. The correction restores `ES` immediately after the CRC
call. The first corrected smoke run passed atlas CRC and all 64 publications;
the final matrix passed. This guest-visible correction is recorded in the
permanent bug-fix ledger. No emulator change or title-specific host workaround
was required.

## Build and verification

Principal commands:

```sh
build/macos-macports/sdl2/vaeg --selftest
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98t-pyc \
  python3 -m unittest discover -s demos/zundamon-orbit/tools -p 'test_*.py'
sh -n demos/zundamon-orbit/256/build.sh \
  demos/zundamon-orbit/build-local-d88.sh demos/zundamon-orbit/run-vaeg.sh
python3 -m py_compile demos/zundamon-orbit/tools/*depth*.py \
  demos/zundamon-orbit/tools/*hud*.py
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

VAEG selftest passed. All 159 demo tests passed. Encoding, EOL, case, shell,
Python-compile, and whitespace checks passed. The actual executable was
`build/macos-macports/sdl2/vaeg`; no build directory was assumed. No emulator
source changed, so selftest plus the end-to-end VA2 matrix is the applicable
emulator regression set.

The four page/clear runs used two revolutions. Static runs used each divisor
and one revolution. Opposite-page V1/V4/V8 and ladder/pause/missed scenarios
used two revolutions. All invoked `run-vaeg.sh` with explicit VA2 mode and
fresh generated output directories. One experimental concurrent invocation
was discarded because simultaneous debug-harness processes interfered with
checkpoint capture; every accepted run was executed singly and passed.

## Reproducibility and artifacts

Two clean atlas generations, two depth-table generations, two HUD generations,
and two release guest builds were byte-identical:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| release `ZUNDORB.COM` | 32,656 | `b6e1bbc2a600f22ca583e256c82cccab3c1523530a0a2a7836439d4cb74d87ec` |
| depth-table include | 6,333 | `645414752dd68898fb382d70d49dcfc4975b722f2927670d45fd8496a036b09c` |
| HUD include | 51,424 | `3b9f41f2425c5fa35320fc48ba804fae6af60f8619f85dd0d1d5aa8adc0ebf93` |
| public `ZUNDORB.BIN` | 5,912 | `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa` |
| pristine candidate D88 | 1,338,960 | `a12483ef3120ac33ade6c9138a5fdc8b8bcb6a9b70b24b764cb952959d940ef5` |
| VAEG executable | 8,155,976 | `13109ea163c6c708e0e79df0b149b1ff317c04381f9ad119e90ac35bc8e73d46` |

The pristine generated and untracked human-gate candidate is:

```text
build/generated/zundamon-orbit/m98t-va2-candidate/
zundamon-orbit-m98t-pristine.d88
```

No COM, BIN, raw D88, capture, trace, generated QA report, save state, backup
RAM, ROM, or private material is tracked. The tracked includes contain only
deterministically generated public source data. Private integration paths,
identities, and hashes are absent from tracked documentation.

## Limitations and human gate

M98t remains one public marker and one G1 instance. It has no private IDA,
second instance, UP/DOWN count control, depth sorting, yaw/image rotation,
runtime scaling, measured-FPS HUD, gameplay, or physical timing evidence.
The smallest public stored levels contain very few visible indexed pixels;
this is the accepted public atlas, not runtime clipping or a skipped scale.

Launch the pristine D88 in VA2, run `ZUNDORB`, and verify two clockwise
revolutions: medium at right, largest at bottom, smallest at top, stable anchor,
no stale silhouette/trail/clipping/flicker/tear, correct G0 transparency, HUD
`FPS: 60` and `ZUNDAMON: 1`, all LEFT/RIGHT fields with no stale decimal,
SPACE pause/resume, inactive UP/DOWN, and ESC restoration. Exact measured FPS
and physical-hardware performance are not part of the human gate.

The maintainer explicitly stated `G98t passed` on 2026-09-01 after the
interactive VA2 check. M98t is therefore closed. Physical hardware remains
`REAL_HW_PENDING`.

`REAL_HW_PENDING`

`G98t PASS`
