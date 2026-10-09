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
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98z orbit and camera controls

Status: **G98z PASS**

`REAL_HW_PENDING` remains in force. This report records the bounded public
implementation and automatic evidence; it does not claim physical PC-88VA
throughput. The maintainer explicitly stated `good human gate passed` on
2026-09-02 after verifying the interactive candidate, closing G98z.

## Authority and preserved state

- Branch: `topic/m98z-orbit-camera-controls-final-qa`
- Exact accepted G98y starting head: `8e402c39836b290d222e997c53c184d212ec7233`
- Accepted G98y report: `docs/agents/reports/m98y_private_ida_integration_visual_gate.md`
- Accepted M98y implementation: `de0af9d73b429d1087751d9344d87396fe6c57a7`
- Accepted M98y report commit: `e7266d33dfb6fd86f060162a82c44e6f71b73139`;
  approval was subsequently recorded by the starting head above.
- G98y approval: maintainer explicitly stated `G98y human gate passed`.
- G98z approval: maintainer explicitly stated `good human gate passed` on
  2026-09-02 for the public/private interactive candidate.
- M98z implementation commits: `ada043feee7491ba980c00e43603214294036f82`,
  `d3c969758362c38d189015579202b18a240c774c`,
  `cbd88fb1c754592d495a7ca16071c25a24250e23` (corrective HUD/input fix),
  `4d94eb6f334add0c2e694b08bdc61c8400d0edc1` (cadence-boundary and
  fixed-width SPD correction), and
  `64f3ea74e750028aca4c41b0f8ee515e75959e84` (extended speed ladder and
  trailing SPD cell).
- Report commit and pushed head: supplied by the final Git handoff after this
  report commit; remote equality is checked at push time.
- M98x implementation: `e833b977f671c921d9ad247249d2217c6782cc52`.
- M98x report/head resolved from its supplied prefix:
  `78c166f2c0282d39b03d51104aeac7e38f4de202`.

The unrelated pre-existing worktree state was preserved exactly:

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

No baseline path was staged, reformatted, removed, or generated over.

## Changed scope

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Add bounded requested/active speed, distance, look, and radius state; key events; Q8.8 phase/radius projection; clipping/counters; G0 status publication, and signed status-index/input-code corrections. |
| `demos/zundamon-orbit/256/zundamon_status_table.inc` | Generated fixed-width G0 status tiles for all speed, distance, look, and radius levels. |
| `demos/zundamon-orbit/256/build.sh` | Validate the separate status include during deterministic guest builds. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_hud.py` | Add deterministic status-field definitions and optional status-include output while retaining the accepted legacy HUD bytes. |
| `demos/zundamon-orbit/tools/validate_zundamon_orbit_hud.py` | Extend the task-authored glyph vocabulary used by generic status fixtures. |
| `demos/zundamon-orbit/tools/zundamon_orbit_controls.py` | Independent integer host model for state bounds, snapshots, accumulation, projection, and status formatting. |
| `demos/zundamon-orbit/tools/test_m98z_orbit_controls.py` | M98z key, saturation, snapshot, accumulator, projection, status, contract, and bounded exhaustive tests. |
| `demos/zundamon-orbit/README.md` | Document the complete control map, defaults, bounds, billboard limitation, and emulator timing caveat. |
| `docs/agents/ROADMAP.md` | Record M98z implementation and G98z human approval. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Reconcile the active M98z scope and reserve the 128-instance extension for M98aa. |
| `demos/zundamon-orbit/tools/test_m98y_private_profile.py` | Update the deterministic public guest identity assertion after the intentional G0 status extension and corrective HUD layout. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_depth_guest.py` | Keep the accepted legacy HUD include identity assertion byte-exact. |

## Control and renderer contract

The renderer remains 320x200 VA 8bpp (`GGGRRRBB`), G0 background/HUD plus G1
transparent billboards, two physical 320x200 pages, one shared 30-scale atlas
in one BMS bank, complete dirty-union clear/draw transactions, and signed-depth
ascending order with instance-ID tie breaking. The public profile remains
ZUNDAMON; the accepted private IDA profile remains local and untracked. The
effect is a camera-facing 2D billboard orbit, not true yaw/pitch rotation or a
3D model. The 128-instance extension is explicitly deferred to M98aa.

Requested and active projection fields are separate:

```text
requested/active speed index: 0..12, default 3, Q8.8 increments
requested/active distance bias: -4..+4, default 0
requested/active look level: -4..+4, default 0, four pixels per level
requested/active radius index: 0..8, default 4, Q8.8 factors
phase accumulator: 16-bit modulo 64*256
```

`A`/`Z` changes only the speed ladder (0.25X, 0.50X, 0.75X, 1.00X, 1.25X,
1.50X, 2.00X, 3.00X, 4.00X, 5.00X, 6.00X, 7.00X, 8.00X). The SPD tile has
one trailing blank cell before the next status field. `Q`/`E` changes distance and clamps effective scale IDs
to 1..30. `W`/`S` changes look level; W is upward-looking and adds positive
screen-Y bias. `O`/`P` selects radius factors 0.50X through 1.50X from the
immutable base radii with symmetric integer rounding. LEFT/RIGHT retain the
V1..V8 nominal FPS divisor ladder, UP/DOWN retain the 1..16 count ladder,
SPACE pauses, and ESC uses common cleanup. Physical make events are consumed
through the existing one-press BIOS policy, so letter case does not create a
second path and typematic is bounded.

Input updates requested state only. At the next hidden transaction the latest
request is copied into active state, all records/order/rectangles are rebuilt,
the old page-local union is cleared, and the complete frame is drawn. Status
tiles are written to G0 only after the corresponding page/count is published;
G1 and the accepted FPS/count rectangles are untouched. A missed slot retains
the pending frame, visible state, and phase. The Q8.8 accumulator advances only
after complete publication; paused publication does not consume a phase step.

The corrective pass uses the PC-88 keymap values for A (`1Dh`), S (`1Eh`), and
Z (`29h`), and sign-extends the bounded DIST/LOOK values before selecting
their generated tiles. The four status fields now share one row below the
legacy HUD at fixed x positions 4, 64, 106, and 154, with no overlap; each
published active snapshot redraws all four fields, including negative values.
The SPD field is ten six-pixel cells (`60` bytes), including one trailing
blank cell after every `SPD:<level>` label; this keeps the complete field
separated from `DIST` and prevents tile rows from straddling adjacent status
fields. The ladder has thirteen entries through `8.00X`. The FPS field update
also treats a falling VBLANK edge after the complete CPU tile write as a
diagnostic overrun, not a runtime failure, so a valid RIGHT/LEFT transition
at the 15-FPS entry cannot exit the guest after the field is already complete.

## Host and guest evidence

The independent control model covers all thirteen speed entries, all divisor
values, saturation/no-wrap, immutable snapshots, signed Q8.8 rounding, scale
clamps, W/S sign, O/P identity at 1.00X, status formatting, and the complete
projection boundary space:

```text
64 phases * 16 counts * 9 distances * 9 look levels * 9 radius levels
= 746,496 bounded projection cases
```

The repository-wide orbit/atlas/dirty-union/runtime suite completed **243
tests PASS**, including inherited public M98t/M98u/M98v/M98w/M98x coverage and
the M98z control tests. The inherited host transition and private-profile
tokens remain accepted: `HOST_PUBLIC_PROFILE_PASS`,
`PRIVATE_IDA_ASSET_VALIDATED`, `HOST_PRIVATE_IDA_PASS`,
`PRIVATE_IDA_ONE_BANK_PASS`, `PRIVATE_IDA_30_SCALE_PASS`,
`PRIVATE_IDA_32768_TRANSITIONS_PASS`, and `VAEG_PRIVATE_IDA_MULTI_PASS`.

The normal public guest is one deterministic binary, 52,656 bytes, and two
clean builds produced SHA-256
`e0f2111e4da5d0723633f6ac11658fad49f825c2bc6e6346578ac72ff67aa93f` after
the fixed-width SPD, extended speed ladder, and nonfatal completed-write
correction.
The legacy FPS/count HUD include remains byte-identical to its accepted
`fa5552dd236cc078e94d905e35698a9887269ede13aa4db86658988b16775b8e` identity;
the new status tiles are a separate generated include. Fixed-count QA builds
for 1, 2, 4, 8, and 16 all assemble below 64 KiB. A private-profile build
also remains 49,856 bytes; private atlas identity, paths, hashes, and D88
details are intentionally excluded from this report.

The existing VAEG executable `build/macos-macports/sdl2/vaeg --selftest`
reported all tests passed. A fresh public count-four VA2-model candidate ran
one complete orbit and returned `M98X_VAEG_CAPTURE_PASS`; its generated D88,
trace, and framebuffer files remain ignored. VAEG timing is diagnostic only.

Repository checks passed:

```text
python3 tools/repo/check_case.py              0 finding(s)
python3 tools/repo/check_encoding.py --expect utf8  0 violation(s)
python3 tools/repo/check_eol.py --enforce     0 violation(s)
```

## QA limits and private-data proof

The public static/load and inherited full/dirty compositor matrices remain
unchanged; this milestone adds the control-state proof and bounded VA2 smoke
candidate without changing the atlas, SGP multiplier, cadence definitions, or
private profile bytes. Full physical hardware timing is not claimed.

Staged-change scans contain no ROM, viewer export, private manifest, palette,
private atlas, private binary/D88, screenshot, trace, private path, or private
hash. Generated guests, D88s, traces, and temporary host matrices remain
outside Git under ignored/generated or temporary paths. The final dirty state
matches the preflight baseline above.

The maintainer completed the visual gate, including public/private labels,
each new key direction and saturation bound, FPS/speed independence,
distance/look/radius behavior, pause/resume, count transitions, overlap,
phase wrap, both page parities, and ESC restoration. The milestone is closed:

**G98z PASS**
