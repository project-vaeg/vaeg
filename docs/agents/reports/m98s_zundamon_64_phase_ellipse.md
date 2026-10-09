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

# M98s constant-scale 64-phase ellipse result

Status: **G98s human gate passed; M98s closed on 2026-09-01**

## Result

`M98s AUTOMATED RESULT: PASS`

`G98s PASS`

M98s replaces the accepted M98r release zoom sequence with one deterministic
64-phase clockwise screen ellipse. Every render selects public atlas scale ID
15. The inherited cadence scheduler, page-local dirty-row clearing, hidden-page
transaction, direct BMS source, transparent SGP BITBLT, VBLANK publication,
and cleanup contracts remain intact.

All host tests, four full/dirty page-parity comparisons, eight static cadence
runs, three opposite-page long runs, the dynamic cadence ladder, pause/resume,
and deterministic missed-slot run passed. The result is VAEG evidence in VA2
mode. Physical PC-88VA/VA2 evidence remains `REAL_HW_PENDING`.

## Git and predecessor

- Branch: `topic/m98s-64-phase-ellipse`
- Starting and accepted M98r head:
  `4c5a7724e31cc0a52c8bfe8e827198c1c30a8c37`
- M98r implementation:
  `72a493e9262955187d8f30b6b31ca9a2a1fc3b4f`
- M98r interactive Return correction:
  `3c3f233305915aa61c594886520764b578ef5025`
- M98s implementation:
  `3cce7e7b93171bb0fdaf31af9997ce9ae6ad63c4`
- M98s independent table-validator strengthening:
  `5e2e3bb1dc6fa6efcc0722a9244a36bffd16c1f9`
- Report/pushed-head commit: recorded in the final handoff because this file
  cannot contain its own commit SHA.
- Accepted predecessor report:
  `docs/agents/reports/m98r_zundamon_vblank_cadence.md`

The maintainer explicitly stated `G98r passed` before assigning M98s. The
accepted predecessor branch and remote both resolved to the starting commit.
M98t remains the separate depth and 30-level scale-coupling milestone.

## Preserved dirty-worktree baseline

The following unrelated entries existed before M98s. They were neither staged
nor modified by this milestone:

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

The final worktree has exactly this unrelated baseline. Generated M98s output
remains ignored below `build/generated/zundamon-orbit/`.

## Changed files

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Bind scale 15, validate orbit bounds, transact page-local phase state, and advance phase only after publication. |
| `demos/zundamon-orbit/256/zundamon_orbit_table.inc` | Store the deterministic 64-entry signed orbit table. |
| `demos/zundamon-orbit/256/build.sh` | Validate deterministic table regeneration and build M98s release/QA variants. |
| `demos/zundamon-orbit/build-local-d88.sh` | Build a non-overwriting interactive M98s disk. |
| `demos/zundamon-orbit/run-vaeg.sh` | Run one bounded VA2 orbit/cadence/clear-mode case. |
| `demos/zundamon-orbit/README.md` | Document M98s geometry, controls, and exclusions. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_table.py` | Generate the table from a canonical Q16 quarter wave without host `libm`. |
| `demos/zundamon-orbit/tools/validate_zundamon_orbit_table.py` | Independently validate phases, symmetry, direction, bounds, and identity. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_ellipse_debug.py` | Generate bounded capture/checkpoint scripts. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_ellipse_guest.py` | Compare guest registers, SGP trace, indexed pages, composition, and cadence with the host oracle. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_ellipse_guest.py` | Test generation, geometry, scheduler, page state, and fail-closed cases. |
| `docs/agents/ROADMAP.md` | Record assignment and automated/human gate state. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Reconcile the active M98q/M98r/M98s/M98t sequence. |
| `docs/agents/tasks/M98s_zundamon_64_phase_ellipse.md` | Record the fixed M98s task and gate. |
| `docs/agents/reports/m98s_zundamon_64_phase_ellipse.md` | Record this result. |

No emulator source or bug-fix ledger entry changed for M98s.

## Display, BMS, and fixed descriptor

- Display: 320x200 VA direct-color 8-bpp (`GGGRRRBB`).
- G1 pitch and backing: 320 bytes, 320x400, two 64,000-byte pages.
- Page A: SGP `220000h`, DSA1 `020000h`.
- Page B: SGP `22fa00h`, DSA1 `02fa00h`.
- DSA1: word ports `022eh`/`0230h`, byte-address values.
- BMS: port `01d0h`; selector 0 ordinary RAM; selector 1 atlas; aperture
  `080000h-09ffffh`; one 128 KiB bank.
- BITBLT: transparent source byte `00h`, accepted mode `0105h`.
- Public atlas: version 1; 5,912 bytes; 4,888 payload bytes; exactly 30 scale
  IDs 1 through 30; no IDs 0 or 31; one bank; SHA-256
  `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa`.
- Fixed scale 15: 11x9, pitch 12, anchor `(5,4)`, payload 108 bytes, bank
  offset `0280h`, SGP source `080280h`, frame CRC32 `4553da3a`.

The release loop contains one fixed-scale selection owner and no call to the
old 58-step zoom advance. `scale_changes` remains zero. BMS selector 1 is held
through every SGP read and selector 0 is restored on cleanup.

## Orbit generation and geometry

The logical anchor-space center is `(160,100)`. The live scale-15 extents
derive safe maxima greater than the capped radii, so the selected radii are
`radius_x=96` and `radius_y=48`. Destinations use:

```text
target_anchor = (160 + dx[phase], 100 + dy[phase])
destination   = target_anchor - (5,4)
```

The generator uses a frozen Q16 quarter-sine table and explicit signed
round-half-away-from-zero. It uses no host floating point or `libm`; the guest
contains no runtime trigonometry or scaling. Two independent generations were
byte-identical to the checked-in include. Table SHA-256 is
`b69763cc8c1bcef198ff35b8244bb02b59169d8d474829f0f8654570db723605`.

| Phase | Offset | Target anchor | Destination rectangle |
|---:|---:|---:|---:|
| 0 | `(96,0)` | `(256,100)` | `[251,96,262,105)` |
| 16 | `(0,48)` | `(160,148)` | `[155,144,166,153)` |
| 32 | `(-96,0)` | `(64,100)` | `[59,96,70,105)` |
| 48 | `(0,-48)` | `(160,52)` | `[155,48,166,57)` |

All 64 entries have exact opposite and quarter-wave symmetry, no consecutive
duplicate including 63-to-0, clockwise screen-coordinate order, and valid
signed components. All 64 half-open 11x9 rectangles fit 320x200 and both G1
physical address ranges without clipping or wrapping.

## Phase transaction and page-local clearing

Both physical pages retain independent logical old rectangles and saved
phases. On hidden-page reuse, the guest clears only that page's old half-open
rectangle, rounding the horizontal interval outward to complete 16-bit words.
All row CLS batches finish before the one scale-15 transparent BITBLT. A page
becomes READY only after bounded SGP completion.

At an eligible low-to-high VBLANK edge, publication commits DSA1, the pending
logical rectangle and phase, swaps page roles, and then advances phase once.
Pause, missed slots, divisor changes, rendering, and failed work do not advance
the phase. Phase 63 wraps exactly to 0. The two initialization full clears are
accounted separately; dirty release steady state has zero full-page CLS.

For each 128-publication dirty run:

```text
phase sequence             0..63,0..63
phase_publications[0..63]  2 each
phase_advances             128
revolution_wraps           2
scale_changes              0
dirty_rect_clears          126
dirty_row_cls_commands     1134
dirty_words_cleared        6804
dirty_bytes_cleared        13608
transparent_bitblts        128
page_flips                 128
```

The equivalent steady full-page clear volume is 8,192,000 bytes. The M98s
contract's steady-update comparison excludes the two common initialization
clears: 13,608 dirty bytes versus 8,192,000 full bytes. This is logical work,
not an elapsed-speed or hardware-performance claim.

## Full/dirty parity and framebuffer evidence

The `A/full`, `A/dirty`, `B/full`, and `B/dirty` cases each published 128
frames. Every full/dirty corresponding 64,000-byte physical G1 state and full
320x200 composition compared byte-for-byte, with zero mismatches. Every frame
used scale 15, contained 25 nonzero G1 pixels, preserved transparent G0
samples, and retained zeroed G1 guards.

| Case | Publications | Slots | Missed | CLS commands | BITBLTs | Result |
|---|---:|---:|---:|---:|---:|---|
| A/full | 128 | 129 | 1 | 130 | 128 | PASS |
| A/dirty | 128 | 128 | 0 | 1,136 | 128 | PASS |
| B/full | 128 | 129 | 1 | 130 | 128 | PASS |
| B/dirty | 128 | 128 | 0 | 1,136 | 128 | PASS |

The one full-mode V1 miss is valid telemetry caused by the deliberately large
clear baseline. It retained the prior complete page and the original pending
phase. Dirty/full output remained exact.

Representative physical G1 SHA-256 identities for phases 0, 16, 32, and 48
are respectively:

```text
95d360fb30953156481733029aab3d95203820642b80b5cd82fd52f3369fa868
61886f4a1d05145ec801118fc026094543b0da3f4901c736627270ec23130f83
69f9d92c1a72d4c889878c4476f6d6bbf85e863b0684f2fd952794179ad79a3d
c5595de85a3e3ab4e8824253fb29b9c7f45ed0ab116525178f3867eb9b4c231b
```

## Cadence evidence

The inherited VA2 profile uses the established 59.95 Hz VBLANK reference.
Nominal labels, requested actual rates, and observed publication rates remain
separate. Every static run published exactly phases 0 through 63 with no miss.

| Option | Nominal | Edges | Published | Missed | Requested/observed Hz | Revolution seconds |
|---|---:|---:|---:|---:|---:|---:|
| `/V1` | 60 | 64 | 64 | 0 | 59.950 | 1.068 |
| `/V2` | 30 | 128 | 64 | 0 | 29.975 | 2.135 |
| `/V3` | 20 | 192 | 64 | 0 | 19.983 | 3.203 |
| `/V4` | 15 | 256 | 64 | 0 | 14.988 | 4.270 |
| `/V5` | 12 | 320 | 64 | 0 | 11.990 | 5.338 |
| `/V6` | 10 | 384 | 64 | 0 | 9.992 | 6.405 |
| `/V7` | 8.6 | 448 | 64 | 0 | 8.564 | 7.473 |
| `/V8` | 7.5 | 512 | 64 | 0 | 7.494 | 8.540 |

Opposite-initial-page two-revolution runs passed at V1 (128 edges), V4 (512
edges), and V8 (1,024 edges). The V1-to-V8-to-V1 ladder published 128 phases
over 338 edges and applied 14 changes at 14 reset boundaries. The pause case
published 128 phases over 146 edges, including 15 paused edges and six applied
pause transitions. The forced-miss case observed 130 slots, published 128
phases, counted two misses, and later published the unchanged pending phase.

LEFT/RIGHT, SPACE, ESC, debounce, READY-only publication, and the boundary
reset order match M98r. UP/DOWN remain inactive. Every drained run satisfied:

```text
requested_slots == published_updates + missed_slots
published_updates == page_flips == phase_advances
scale_changes == partial_publication_attempts == 0
steady_full_page_clears == 0 in dirty mode
sgp_timeouts == sgp_errors == vblank_timeouts == 0
bounds_failures == guard_failures == dirty_full_mismatches == 0
cleanup_runs == 1
```

## Host model and negative coverage

The independent host model derives table positions, scale-15 composition,
page A/B state, dirty-row words, publication-only phase advancement, cadence,
pause, misses, and counters from public inputs rather than guest output
constants. Thirty-seven stable negative classifications cover malformed
phase tables and components; bad cardinals/symmetry/direction; invalid or
changed scale 15; destination overflow, bounds, clipping, and page errors;
global/wrong-page rectangles; early rectangle/phase commit; missed-phase skip
and wrap errors; rounded-bound storage and CLS off-by-one; early BITBLT;
visible-page writes; partial, ineligible, or paused publication; shortened
resume/change intervals; busy edge loss and BMS mutation; SGP/VBLANK failures;
steady full-clear regression; golden mismatch; guard corruption; and ESC with
queued work.

Each modeled failure retains the prior complete visible page, performs no
rectangle/phase/page commit, restores ordinary BMS selector 0 and video state,
and runs cleanup exactly once. Mutated orbit inputs exercise stable validator
error paths. Test-only ladder, pause, and miss injections do not exist as a
release runtime switch.

## Build and verification

Principal commands:

```sh
cmake --build build/macos-macports --target vaeg -j4
build/macos-macports/sdl2/vaeg --selftest
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98s-pyc \
  python3 -m unittest discover -s demos/zundamon-orbit/tools -p 'test_*.py'
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
sh -n demos/zundamon-orbit/256/build.sh \
  demos/zundamon-orbit/build-local-d88.sh demos/zundamon-orbit/run-vaeg.sh
python3 -m py_compile demos/zundamon-orbit/tools/*m98s*.py
git diff --check
```

The CMake build was current. VAEG selftest passed. All 143 demo tests passed,
as did encoding, EOL, case, shell, Python-compile, and whitespace checks. No
emulator source changed, so the full selftest plus end-to-end VA2 matrix is the
applicable emulator regression set.

The four parity/clear runs used `VAEG_ZUNDAMON_REVOLUTIONS=2` and full/dirty
clear mode. Static runs used divisors 1 through 8 and one revolution. Long
opposite-page runs used V1/V4/V8 and two revolutions. Dynamic cases used two
revolutions and the `ladder`, `pause`, or `missed` compile-time scenario. All
invoked `demos/zundamon-orbit/run-vaeg.sh` with the discovered
`build/macos-macports/sdl2/vaeg` and VA2 mode.

## Reproducibility and generated artifacts

Two clean release guest builds and two table generations were byte-identical:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `ZUNDORB.COM` | 22,544 | `a1d8d978344a9c43b1f8e1aa65566b895b3b17403c81b1a68340fa4bfac03dc4` |
| `ZUNDORB.LST` | 231,825 | `abad458b4778eb29a191de18c620ebd38a1ab7d02cacbb2fb97d43372ded2eb0` |
| orbit include | 3,302 | `b69763cc8c1bcef198ff35b8244bb02b59169d8d474829f0f8654570db723605` |
| table metadata | 4,271 | `d17dd368cc60533f7e55d308e0f06839c5dc4a47778e3b8ca4295b77d3438583` |
| `ZUNDORB.BIN` | 5,912 | `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa` |
| pristine candidate D88 | 1,338,960 | `57bfefecce0f21e625768d00fa6e62423d304f491e25598ea3e41923136ca859` |
| VAEG executable | 8,155,976 | `13109ea163c6c708e0e79df0b149b1ff317c04381f9ad119e90ac35bc8e73d46` |

The eight static oracle-report SHA-256 values, V1 through V8, are:

```text
b6527c8d34eeb0488bef7f216912bfef99896da27b4d20f5f6a87b4ec4b51b96
13d45def593f7ed0b7a0111fde69fd2c96627fa04e7975a634890e405a237974
734b06efd94942a07b888758bd5cbc81d2ce87390f7c445b3b4033709e0e1705
996121f9b6697d31b22797b7854ab6bc04c8ba95b998b3facea3484c082776c5
c641c921a23e0c5a21393be0eb0c3a952d26b521040049d6214caeed8ecb4e11
b8076357c3f9893f9b9c099337e3c9dde4f8c56f4a63f837ed8143c3caeca0eb
454f04238c739c83f3ac2784be9fdef203c7cd3fcf44695a4edadbc3b6c2308c
107a5283d48bba595eec560e94ead108c85ba832acdc55d96a255e7d2eee323b
```

The pristine human-gate candidate is generated and untracked at:

```text
build/generated/zundamon-orbit/m98s-va2-candidate/
zundamon-orbit-m98s-pristine.d88
```

Its disk lists the accepted boot files plus only the public `ZUNDORB.COM` and
`ZUNDORB.BIN`. No generated `.COM`, `.BIN`, D88, capture, trace, report, save
state, backup RAM, private input, or ROM-derived byte entered Git. A commit
scope scan contains no machine-specific absolute path or sensitive asset
identity.

## Human gate and limitations

Launch the pristine candidate in VA2 and run:

```text
ZUNDORB
```

Confirm one constant-size marker begins at the right and moves clockwise
toward the bottom, then reaches left and top and wraps continuously for at
least two revolutions. There must be no trail, clipping, one-pixel clear line,
flicker, tear, partial page, size change, or page-parity difference.
Transparent holes must reveal G0. RIGHT/LEFT must step cadence once, SPACE must
freeze and resume after a full interval without a phase jump, UP/DOWN must do
nothing, and ESC must restore the prior display and prompt.

The human gate checks direction, constant size, visual clearing, controls, and
restoration. It does not establish exact fps or physical-hardware performance.
The maintainer explicitly stated `G98s passed` on 2026-09-01 after the
interactive VA2 check. M98s is therefore closed. Physical hardware remains
`REAL_HW_PENDING`.

M98s intentionally retains a public marker, fixed scale 15, and one billboard.
It adds no depth, phase-to-scale coupling, private IDA, image rotation,
multiple instances, gameplay, audio, or real-hardware timing evidence.
