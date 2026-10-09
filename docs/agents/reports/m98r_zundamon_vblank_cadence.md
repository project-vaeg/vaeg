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

# M98r selectable VBLANK cadence result

Status: **G98r human gate passed; M98r closed on 2026-09-01**

## Result

`M98r AUTOMATED RESULT: PASS`

`G98r PASS`

M98r adds only VBLANK-divided publication scheduling to the accepted M98q
renderer. All eight static divisor runs, the opposite-page V1/V4/V8 long
runs, the dynamic V1-to-V8-to-V1 ladder, pause/resume, and a deterministic
two-miss VA2 case passed the indexed-frame and SGP-trace oracle. The public
atlas, fixed anchor, exact `30..1..29` sequence, page-local dirty clearing,
one transparent BITBLT, and transactional page publication are unchanged.

This is VAEG in VA2 mode. Physical PC-88VA/VA2 evidence remains
`REAL_HW_PENDING`.

The first interactive candidate did not pass its human gate: the maintainer
reported an immediate return to the Human prompt, including a retry that
printed no M98R failure diagnostic. The correction below has passed automated
VA2 regression. The maintainer subsequently accepted the corrected candidate
and stated `G98r passed` on 2026-09-01.

## Interactive Return correction

The original interactive input path accepted Return as an exit key and also
recognized ESC from scan code zero alone. A command-confirming Return retained
by the guest input path could therefore enter normal cleanup before the first
visible publication. Return is no longer an exit key, and both keyboard polls
now require the complete INT 82h/AH=09h ESC result: scan code `00h` and
internal code `1bh`.

A release-mode regression enters `ZUNDORB /V1`, injects one additional Return
at relocated entry `3000:012ah`, and then captures all 58 publication
checkpoints. The corrected guest produced 58 G1 captures, exactly 58 SGP
source and destination operations, and every G1 capture matched the accepted
M98r V1 golden byte-for-byte. This specifically rejects another prompt-only
normal exit while leaving the renderer and cadence output unchanged.

## Git and predecessor

- Branch: `topic/m98r-vblank-cadence`
- Starting and accepted M98q gate commit:
  `ade337c2d1f2ec0106a04361e1dd22a9995cb9b7`
- Accepted M98q implementation:
  `6a3f229c74d1ffed9888b279e80334ac76d2e461`
- M98r implementation:
  `72a493e9262955187d8f30b6b31ca9a2a1fc3b4f`
- Interactive Return correction:
  `3c3f233305915aa61c594886520764b578ef5025`
- Report/pushed-head commit: recorded in the final handoff because this file
  cannot contain its own commit SHA.
- Accepted predecessor report:
  `docs/agents/reports/m98q_zundamon_dirty_rows.md`

The maintainer explicitly passed G98q on 2026-09-01 before assigning M98r.
M98s remains the separate 64-phase ellipse milestone and M98t remains the
separate depth/scale-coupling milestone.

## Preserved dirty-worktree baseline

These unrelated entries existed before M98r and were not staged or modified
by this milestone:

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

The final status contains exactly the same unrelated entries. Generated M98r
outputs remain ignored below `build/generated/zundamon-orbit/`.

## Changed files

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Add `/V`, edge observer, divider, controls, pause, READY/miss scheduling, counters, and bounded QA scenarios. |
| `demos/zundamon-orbit/256/build.sh` | Build release and bounded cadence variants. |
| `demos/zundamon-orbit/build-local-d88.sh` | Build a non-overwriting interactive M98r disk and print controls. |
| `demos/zundamon-orbit/run-vaeg.sh` | Run one static or dynamic bounded VA2 cadence case. |
| `demos/zundamon-orbit/README.md` | Document selectors, controls, and rate semantics. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_cadence_debug.py` | Generate bounded checkpoint/capture scripts. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_cadence_guest.py` | Verify guest edges, frames, traces, counters, and artifacts. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_cadence_guest.py` | Independently model parser, scheduler, and fail-closed cases. |
| `docs/modernization/bug-fixes.md` | Record the interactive Return/ESC discrimination defect and correction. |
| `docs/agents/ROADMAP.md` | Record M98r assignment and automated result. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Keep M98r/M98s/M98t separate and record gate state. |
| `docs/agents/tasks/M98r_zundamon_vblank_cadence.md` | Record the fixed M98r task and human gate. |
| `docs/agents/reports/m98r_zundamon_vblank_cadence.md` | Record this result. |

No emulator source changed in M98r.

## Preserved display, atlas, and renderer contract

- Display: 320x200 VA direct-color 8-bpp (`GGGRRRBB`).
- G1: 320-byte pitch, 320x400 backing surface, two 64,000-byte pages.
- Page A: SGP `220000h`, DSA1 `020000h`.
- Page B: SGP `22fa00h`, DSA1 `02fa00h`.
- DSA1: word ports `022eh`/`0230h`, byte-address values.
- BMS: port `01d0h`, selector 0 ordinary RAM, selector 1 atlas, aperture
  `080000h-09ffffh`, one 128 KiB bank.
- Atlas: version 1, 5,912 bytes, 4,888 payload bytes, exactly 30 descriptors,
  SHA-256
  `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa`.
- Sequence: `30,29,...,1,2,...,29`, 58 successful publications per cycle.
- Anchor: fixed at `(160,100)` through every divisor and dynamic transition.

M98q's half-open page-local rectangles and outward 16-bit-word rounding are
unchanged. Both G1 pages receive one initialization full clear. Release steady
state has no full-page clear. All old-row CLS work completes before one
transparent BMS-to-G1 BITBLT. Only a hidden page is written, and its rectangle,
page role, and scale commit only after complete publication.

For one 58-publication run the guest reported 56 dirty rectangles, 517 row CLS
commands, 4,002 words, and 8,004 cleared bytes versus a 3,712,000-byte
full-clear baseline. For a two-cycle run it reported 114 rectangles, 1,069
rows, 8,407 words, and 16,814 bytes versus 7,424,000 baseline bytes. These are
logical work counts, not elapsed-speed or hardware-performance claims.

## VBLANK observer and scheduler

The live VBLANK source is TSP status port `0142h`, bit `40h`. One observer
records a transition only after low was observed and the bit then becomes
high. The observer runs while polling SGP BUSY as well as while waiting for a
publication edge, preventing edge loss during dirty-row batches and BITBLT.
The accepted SGP and scheduler waits retain the bounded four-by-65,535 polling
limit and common cleanup path.

At one polling iteration, SGP completion is sampled first. A low-to-high edge
is then observed, queued ESC/pause/divisor actions are applied, and only then
is the divider advanced and an eligible READY page published. Divisor and
pause/resume boundaries reset `divider_count` to zero; the boundary edge is
not the first edge of the new interval. ESC has priority.

The renderer prepares the next hidden scale eagerly. A READY page waits for a
divisor-qualified edge. An eligible edge with a non-READY page increments
`missed_slots`, retains DSA1 and the complete visible page, and does not advance
the scale. The enforced invariant is:

```text
requested_slots == published_updates + missed_slots
published_updates == page_flips == scale_advances
```

## Options and controls

The PSP parser accepts zero or one exact, case-insensitive `/V1` through `/V8`
token and defaults to V1. It rejects missing values, 0/9, signed and multi-digit
forms, trailing junk, duplicates, and unknown options before graphics mode.

LEFT requests one faster divisor, RIGHT one slower divisor, SPACE queues a
pause toggle, and ESC enters cleanup. Endpoint LEFT/RIGHT presses are counted
without resetting the divider. Keyboard auto-repeat is disabled through the
accepted BIOS path while the demo runs and restored on exit, so one make event
causes at most one action. UP and DOWN have no M98r action.

While paused, VBLANK edges are counted separately but produce no slots,
publication, or scale advance. In-flight SGP work may finish and remain READY.
Resume waits a complete new divisor interval before that page can publish.

## Static divisor evidence

The VA2 profile used by these runs has the established 59.95 Hz VBLANK
reference. The guest-measured edge count is authoritative for cadence. The
requested and observed successful-publication rates below are derived
separately from `Fv/divisor` and `Fv*published/observed_edges`; every static run
had zero missed slots, so the two values agree. Nominal labels are not exact
rate promises.

| Option | Nominal | Edges | Slots | Published | Missed | Requested Hz | Observed Hz |
|---|---:|---:|---:|---:|---:|---:|---:|
| `/V1` | 60 | 58 | 58 | 58 | 0 | 59.950 | 59.950 |
| `/V2` | 30 | 116 | 58 | 58 | 0 | 29.975 | 29.975 |
| `/V3` | 20 | 174 | 58 | 58 | 0 | 19.983 | 19.983 |
| `/V4` | 15 | 232 | 58 | 58 | 0 | 14.988 | 14.988 |
| `/V5` | 12 | 290 | 58 | 58 | 0 | 11.990 | 11.990 |
| `/V6` | 10 | 348 | 58 | 58 | 0 | 9.992 | 9.992 |
| `/V7` | 8.6 | 406 | 58 | 58 | 0 | 8.564 | 8.564 |
| `/V8` | 7.5 | 464 | 58 | 58 | 0 | 7.494 | 7.494 |

The debug harness records emulator clock/frame diagnostics, but each capture
introduces a checkpoint boundary. Those frame spans are retained as chronology
evidence and are not misreported as an independent display-frequency meter.
No physical-hardware rate was measured.

## Long, dynamic, pause, and missed-slot evidence

| Case | Total edges | Unpaused | Paused | Slots | Published | Missed | Result |
|---|---:|---:|---:|---:|---:|---:|---|
| V1, initial B, two cycles | 116 | 116 | 0 | 116 | 116 | 0 | PASS |
| V4, initial B, two cycles | 464 | 464 | 0 | 116 | 116 | 0 | PASS |
| V8, initial B, two cycles | 928 | 928 | 0 | 116 | 116 | 0 | PASS |
| V1-to-V8-to-V1 ladder | 326 | 326 | 0 | 116 | 116 | 0 | PASS |
| Three pause/resume episodes | 134 | 119 | 15 | 116 | 116 | 0 | PASS |
| Forced non-READY interval | 118 | 118 | 0 | 118 | 116 | 2 | PASS |

The ladder applied 14 requests at 14 VBLANK boundaries and ended at V1. One
request was injected while SGP was busy. The pause case applied six toggles
at six boundaries and retained three completed hidden pages while paused. The
miss case published its first scale at edge 3 after two retained-page misses;
all later scales remained consecutive. All dynamic framebuffer identities,
page parities, and SGP command sequences matched the M98q golden.

Every static and dynamic case reported two initialization full clears, zero
steady full clears, zero SGP/VBLANK timeout or error, zero guard failure, zero
partial publication attempt, and one cleanup. The trace contains one BITBLT
per rendered update and no visible-page target.

## Host model and negative coverage

The independent host scheduler covers all divisors, completion before/on/after
an eligible edge, consecutive misses, queued and clamped selector changes,
pause in IDLE/RENDERING/READY states, resume with READY, page alternation,
scale reversals, and ESC priority. Twenty-five stable fault results cover:

- persistent-high VBLANK and busy-wait edge loss;
- early divisor application, counted reset boundary, clamp reset, and
  typematic duplication;
- ineligible, non-READY, missed, catch-up, paused, and early-resume publication;
- DSA1 flip/scale advance on a miss;
- BMS/command mutation while busy;
- dirty-row, BITBLT, SGP, VBLANK-low, and VBLANK-high failures;
- early rectangle/page/scale commit, visible-page writes, steady full clears,
  framebuffer mismatch, and ESC with queued work.

Each modeled failure retains the prior complete visible page, performs no
scale/page commit, restores selector 0 and video state, and runs cleanup once.
The release guest has no runtime fault switch; ladder, pause, and missed-slot
injection exist only in compile-time bounded QA builds.

## Build and verification

Principal commands:

```sh
cmake --build build/macos-macports --target vaeg -j4
build/macos-macports/sdl2/vaeg --selftest
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98r-pyc \
  python3 -m unittest discover -s demos/zundamon-orbit/tools -p 'test_*.py'
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
sh -n demos/zundamon-orbit/256/build.sh \
  demos/zundamon-orbit/build-local-d88.sh demos/zundamon-orbit/run-vaeg.sh
```

The CMake build was current, VAEG selftest passed, all 131 demo tests passed,
and the encoding, EOL, case, shell, Python-compile, and whitespace checks
passed. No emulator source changed, so no additional emulator regression was
required beyond the full selftest and VA2 end-to-end matrix.

Each static run used `VAEG_ZUNDAMON_DIVISOR=1..8`, initial page A, one cycle,
and scenario `static`. Opposite-page runs used initial page B and two cycles at
V1/V4/V8. The dynamic cases used two cycles and scenarios `ladder`, `pause`,
and `missed`. All invoked `demos/zundamon-orbit/run-vaeg.sh` with the discovered
`build/macos-macports/sdl2/vaeg` executable and VA2 model.

## Reproducibility and generated artifacts

Two clean release guest builds were byte-identical:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `ZUNDORB.COM` | 22,464 | `be1968ba36b5c06977e6c2202b0f671d761fccf59603ea49f1f77e6261133fc2` |
| `ZUNDORB.LST` | 218,118 | `0b050889a60f401b10cfa128b0ea007d59616469a6529de047baa63834fbc8f5` |
| `ZUNDORB.BIN` | 5,912 | `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa` |
| pristine candidate D88 | 1,338,960 | `9e3c36e13c81fcaf9cbfd9feef6906750099f8de1389c7a4a0b99c844b3ac2b8` |
| VAEG executable | 8,155,976 | `13109ea163c6c708e0e79df0b149b1ff317c04381f9ad119e90ac35bc8e73d46` |

The corrected release candidate was also built twice with byte-identical
outputs:

| Corrected artifact | Bytes | SHA-256 |
|---|---:|---|
| `ZUNDORB.COM` | 22,464 | `2d1b7ead1b98ffa83a56d8a13af6c9a5b2250664728dd34b3e01027b38b040be` |
| `ZUNDORB.LST` | 217,981 | `f355d54949074772a6b0ca16ea0a8435a39fcc16bf4efc66a15f38f6bf96466c` |
| `ZUNDORB.BIN` | 5,912 | `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa` |
| corrected pristine D88 | 1,338,960 | `7079317889b8baccafd09ace259664fb0c9af62c9beaf4243a040839c94250eb` |

The eight static oracle-report SHA-256 values, V1 through V8, are:

```text
d202489bcd6186639fa69c24e5490c2284caaf6ffd4fdb372c7526a9ab64d08e
9f028f2f0d48e52d138c347f2a5a7b5f73449171950cc011869f5cfc9e841a36
290f82d9f1ffeb293a5fa094e7d4d837c8e96d3e6e41349a336e388624036c7a
b604e3a32c6664b8fe32e9858f046720510e165f5d0279f82b211d31d40a14f8
9126f0c8530a24bc82a0be96806c528866e200c1faebc307e946412a2886a564
2f1edcb9d58e6d08deb585c2d5a11b471dfbc4952de6cb9adfbbb8578e69823c
86b7047ceffbed03ace211175d2ac9f4207a05f6ad7b2c558243e4a5a309c17b
0ed45bf52a7bf77b3d83b4a05cb816882e1b31d594967dd8dca14fcce7dc4642
```

The corrected pristine human-gate candidate is generated and untracked at:

```text
build/generated/zundamon-orbit/m98r-va2-candidate-3/
zundamon-orbit-m98r-return-fix-pristine.d88
```

The disk lists only the accepted boot files plus the public `ZUNDORB.COM` and
`ZUNDORB.BIN`. Git status for every generated M98r output path is empty. A
commit-diff scan found no machine-specific absolute path or sensitive local
asset identity. No `.COM`, `.BIN`, D88, capture, trace, save state, backup RAM,
private input, or ROM-derived byte was added to Git.

## Human gate and limitations

Launch the pristine candidate in VA2 and run:

```text
ZUNDORB
```

It defaults to V1. Press RIGHT once per step through V8, verify RIGHT clamps at
V8, then press LEFT once per step through V1 and verify LEFT clamps there.
SPACE must freeze a complete frame and resume only after a full new interval.
Confirm the unchanged fixed anchor, transparency, shrink/grow sequence, lack
of stale silhouettes, partial pages, flicker, or tearing, then press ESC and
confirm normal display restoration.

Automated runs used SDL dummy output, so they prove indexed buffers, commands,
state, and stable captures but not interactive visual quality. The maintainer
completed the visual/control check against the corrected candidate and passed
G98r on 2026-09-01.

M98r still has one fixed-position public synthetic object. It adds no ellipse,
depth coupling, private image, multiple instances, UP/DOWN count control,
sound, gameplay, or general sprite engine. The 59.95 Hz rate is VAEG VA2
evidence, not physical-hardware timing.

`REAL_HW_PENDING`

`G98r PASS`
