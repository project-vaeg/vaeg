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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98w multi-ZUNDAMON dirty-row interval unions

Status: **G98w passed**

`REAL_HW_PENDING`

The maintainer explicitly confirmed `G98w passed` after running the unbounded
count-four VA2 candidate: four instances, no trails/flicker/horizontal streaks,
normal controls and ESC restoration. Physical hardware status remains pending.

## Authority and repository state

- Branch: `topic/m98w-multi-dirty-union`
- Starting/accepted M98v head: `33d15aa090f392d3393083e0ebab99965fc06d22`
- M98v implementation: `5c45ce84a61682b9fd9fd32f57aec43143e1c699`
- M98v report: `docs/agents/reports/m98v_zundamon_multi_full_clear.md`
- M98w implementation commit: `2e402fa5bb69277aa7e4b60575e4ac2e8ccf9ae7`
- Report commit and pushed head: this report commit; the exact self-referential
  hash is supplied by the final Git handoff after commit and push.
- Remote predecessor: `origin/topic/m98v-multi-full-clear` resolved to the
  accepted M98v head before this branch was created.

The maintainer supplied the equivalent of `G98v passed` by confirming the
interactive count-four candidate: four simultaneous markers, `FPS: 60` /
`ZUNDAMON: 4`, continuous clockwise motion, no trail/missing frame/flicker/
tear, inactive UP/DOWN, working LEFT/RIGHT and SPACE, and normal ESC return.
That approval is recorded here without changing the accepted M98v report.

The pre-existing dirty-worktree baseline and the final state are identical:

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

No baseline path was staged, reformatted, overwritten, stashed, or removed.

## Changed files

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/build.sh` | Make dirty-union mode the M98w default while retaining an explicit full-clear QA build mode. |
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Store independent per-page footprints, validate/recompute row unions, batch CLS work, and retain the complete-frame draw/publication barrier. |
| `demos/zundamon-orbit/README.md` | Document the M98w clear contract and build restrictions. |
| `demos/zundamon-orbit/build-local-d88.sh` | Name the local-only M98w candidate and preserve refusal-to-overwrite behavior. |
| `demos/zundamon-orbit/run-vaeg.sh` | Add explicit full/dirty capture selection and M98w verification wiring. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_multi_debug.py` | Extend bounded checkpoint generation through the M98w counters. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_multi_guest.py` | Independently derive dirty unions and compare full/dirty pages, composites, and command traces. |
| `demos/zundamon-orbit/tools/zundamon_dirty_union.py` | Independent bounded host row-union oracle. |
| `demos/zundamon-orbit/tools/test_zundamon_dirty_union.py` | Synthetic parity, overlap, containment, adjacency, capacity, and address tests. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_multi_guest.py` | Assert the release union path and generated M98w evidence. |
| `docs/agents/tasks/M98w_zundamon_multi_dirty_union.md` | Record the reconciled task and automated/human-gate status. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Record G98v approval and M98w completion state. |
| `docs/agents/ROADMAP.md` | Record G98v approval and M98w automated evidence. |

No atlas, ROM, private asset, emulator timing, IDA, or generated binary is
tracked by this change.

## Geometry and SGP contract

G1 is the accepted 320x400, 320-byte-pitch backing surface. Each physical
page is 64,000 bytes / 32,000 words:

| Page | SGP byte base | DSA1 byte-address value | Range |
|---|---:|---:|---|
| A | `220000h` | `020000h` | `[220000h, 22FA00h)` |
| B | `22FA00h` | `02FA00h` | `[22FA00h, 239400h)` |

The existing page guards, backing-size checks, DSA1 low/high port writes, and
visible-page exclusion remain unchanged. A row address is
`page_base + y * 320 + x`; every end address is checked against the exclusive
64,000-byte page end before a command can be emitted.

The live SGP CLS contract is a byte address plus a count in 16-bit words. The
count is the number of words executed and reaches zero after the final word
(the builder therefore does not apply a scattered inclusive `-1` correction).
The command list has 64 words. A dirty list contains SET_WORK, SET_COLOR, at
most eleven five-word CLS commands, and END: `6 + 11*5 = 61` words. Every list
is submitted only while SGP is idle, and every clear batch completes before
the first BITBLT. BUSY polling continues to observe VBLANK and the existing
bounded timeout/error cleanup is retained.

## Footprint and transaction state

Each page has independent `valid`, committed global phase, active count,
publication count, 16 logical half-open rectangles `(x,y,width,height)`, and
16 instance IDs. The pending frame remains in the accepted M98u record buffer
and is never copied into committed state during rendering. On publication the
target page's valid flag is cleared, all records and IDs are copied, phase is
stored, and valid is set last; only then are page roles swapped and global
phase advanced. A failure leaves the prior visible page and committed
footprints untouched and enters common cleanup once.

The guest footprint storage is bounded: 256 bytes of rectangles, 32 bytes of
IDs, and two 16-entry row scratch arrays of 6-byte `(x0,x1,instance_id)`
records. It does not allocate a 200-row matrix, heap memory, recursion, or a
multi-instance dirty map.

## Rounding, sorting, and two-pass union

Logical rectangles are validated as `[x0,x1) = [dst_x,dst_x+width)` and
`[y0,y1) = [dst_y,dst_y+height)`, with `0 <= x0 < x1 <= 320` and
`0 <= y0 < y1 <= 200`. For every contributing row the span is rounded first:

```text
clear_x0 = x0 & ~1
clear_x1 = (x1 + 1) & ~1
```

The rounded span is checked for even endpoints, positive even byte length,
and `clear_x1 <= 320`. Candidates are visited in instance-ID order, then
insertion-sorted by `(clear_x0, clear_x1, instance_id)`. A sweep merges when
`next.clear_x0 <= current.clear_x1`; equality is adjacency and is merged.
When `next.x0 < current.x1`, a strictly shorter `next.x1` is one containment
merge; otherwise it is one overlap merge. When `next.x0 == current.x1`, it is
one adjacency merge. These three counters are mutually exclusive per merge
event. Commands are
emitted row-major, then x-major, from the canonical merged list.

Pass 1 validates the entire committed footprint, every row's candidates and
merged intervals, all word counts, all physical page end addresses, pending
records/sources, and aggregate candidate/merged totals without writing G1.
Pass 2 recomputes the immutable rows, asserts candidate and merged totals are
identical, then emits bounded clear batches and waits for each one. The draw
barrier follows all clear batches; exactly the M98v active-count transparent
BITBLTs are then submitted far-to-near. READY is asserted only after the last
draw completes, and publication remains eligible-VBLANK-only.

## Preserved atlas, compositor, HUD, and controls

The public atlas remains 5,912 bytes, SHA-256
`7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa`, with
exactly IDs 1..30, no 0/31, in one 128 KiB BMS bank. Source-zero transparency,
descriptor anchors, M98u signed-depth order, one-bank selection, BMS BUSY
ordering, DSA1 publication, and the G0 HUD are unchanged. The release
interactive build is count 4 and displays `FPS: 60` / `ZUNDAMON: 4`.

`/V1` through `/V8`, LEFT/RIGHT, SPACE, ESC, pause, missed-slot retention,
and inactive UP/DOWN remain unchanged. Full-page mode is a build/QA oracle;
the release default has no steady-state full-page CLS. M98w does not replace
the public fixture with IDA; private IDA remains a later milestone explicitly
outside this task.

## Full/dirty equivalence and work volume

The accepted M98v full-clear renderer was rerun before optimization. For both
initial page choices, all five counts, two revolutions, and 1,280 corresponding
publications, dirty physical G1 pages and composited 320x200 frames matched
the full-clear golden byte-for-byte. Source descriptors, draw order, HUD,
transparent holes, guards, page parity, and phase identities also matched.

The recorded two-revolution frame-identity digests (the same digest is
obtained for full and dirty mode) were:

| Count | Initial A | Initial B | Dirty bytes / words |
|---:|---|---|---:|
| 1 | `075ef2ae745e` | `94a9fd1c4beb` | 18,892 / 9,446 |
| 2 | `df7d25bfc85d` | `ff62f938addf` | 37,740 / 18,870 |
| 4 | `ef7c9f6b08e2` | `2980cd1b964a` | 75,180 / 37,590 |
| 8 | `1d4fbbcca1d6` | `ef4d0f46040c` | 150,482 / 75,241 |
| 16 | `78d4976daacc` | `683b3ac40c87` | 300,956 / 150,478 |

These compact prefixes identify the full generated digests; the gate
comparison itself was byte-for-byte, not hash-only.

For 128 publications, the equivalent M98v steady clear is 8,192,000 bytes /
4,096,000 words. The dirty totals below are identical for both page starts;
the command merge ratio is 0% on the natural public orbit because its rows are
disjoint, while synthetic tests exercise all merge classes.

| Count | Candidate intervals | Merged/CLS intervals | Dirty words | Dirty bytes | Clear batches | Byte reduction |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 1,200 | 1,200 | 9,446 | 18,892 | 1,200 | 99.7694% |
| 2 | 2,398 | 2,398 | 18,870 | 37,740 | 2,362 | 99.5393% |
| 4 | 4,796 | 4,796 | 37,590 | 75,180 | 4,444 | 99.0823% |
| 8 | 9,592 | 9,592 | 75,241 | 150,482 | 7,311 | 98.1631% |
| 16 | 19,184 | 19,184 | 150,478 | 300,956 | 10,428 | 96.3262% |
| Aggregate | 37,170 | 37,170 | 291,625 | 583,250 | 25,745 | 98.5760% |

All dirty bytes are even and equal twice the word count. Full mode retained
the exact 64,000-byte hidden-page CLS for QA only. Reduced logical bytes are
not an elapsed-speed claim; VAEG timing is diagnostic only.

## 40 static count/divisor cases

Each of the 5 counts was run for one 64-publication revolution at V1..V8,
with full/dirty frame identity checks. The VAEG display reference is 59.95 Hz.
The entries below give `(edges, misses)` for V1..V8; all runs published 64
complete frames and used the count-specific source/command totals shown.

| Count | V1 | V2 | V3 | V4 | V5 | V6 | V7 | V8 | Dirty bytes | Source bytes |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 64/0 | 128/0 | 192/0 | 256/0 | 320/0 | 384/0 | 448/0 | 512/0 | 9,338 | 9,928 |
| 2 | 64/0 | 128/0 | 192/0 | 256/0 | 320/0 | 384/0 | 448/0 | 512/0 | 18,632 | 19,856 |
| 4 | 64/0 | 128/0 | 192/0 | 256/0 | 320/0 | 384/0 | 448/0 | 512/0 | 36,964 | 39,712 |
| 8 | 94/30 | 128/0 | 192/0 | 256/0 | 320/0 | 384/0 | 448/0 | 512/0 | 74,050 | 79,424 |
| 16 | 124/60 | 128/0 | 192/0 | 256/0 | 320/0 | 384/0 | 448/0 | 512/0 | 148,092 | 158,848 |

At V1 the count-8 and count-16 cases missed eligible slots but retained the
complete prior page and later published the unskipped frame. No partial frame,
count reduction, or phase skip occurred. Requested rates are `59.95/d` Hz;
measured publication rates for the no-miss rows equal those values. VAEG
timing is not physical PC-88VA/VA2 timing.

Count 4 dynamic ladder, pause/resume, and injected-miss cases each completed
128 publications over two revolutions with 512 instance BITBLTs, 4,796
candidate/merged intervals, 75,180 dirty bytes, 4,444 clear batches, 4,957
SGP lists, and 20,181 SGP commands. The ladder and pause cases had zero misses;
the consecutive-miss case had two misses. All retained phase/page/HUD
continuity and reached cleanup once.

## Synthetic union and inherited fault evidence

The independent host suite has 13 M98w union tests. It covers empty rows,
one-rectangle M98q equivalence, x/width parity, x=0/x1=320, one-pixel and
one-row rectangles, disjoint spans, overlap, containment, duplicate/equal
ends, rounding-created overlap/adjacency, transitive chains, reverse input
order, all 16 candidates, row-major output, capacity, row bounds, page bounds,
and even CLS counts. All return stable `M98W_*` validation codes on invalid
inputs. The accepted M98v suite had 197 tests; the two new release-source
assertions plus these 13 union tests produce 212 total tests, all PASS. The
inherited M98q geometry/transaction fault suite remains PASS;
the accepted fail-closed behavior is retained for timeout, BUSY mutation,
early BITBLT/READY/publication, visible-page access, guard, HUD, and ESC cases.

Natural public frames have no inter-instance opaque overlap; the independent
host compositor still verifies source-zero transparency and the inherited
synthetic opaque/transparent/tie fixtures. No stale old-only pixels or
over-clear sentinel changes were observed in the 1,280 comparisons.

## Commands and results

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98w-pyc \
  python3 -m unittest discover -s demos/zundamon-orbit/tools \
  -p 'test_zundamon*.py'
# 212 tests, PASS

cmake --build build/macos-macports -j2
# PASS (ninja: no work to do)

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/macos-macports/sdl2/vaeg --selftest --model va2 \
  --roms <public-rom-directory> --no-cfg --no-bkupmem --nowait --mute
# PASS

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
# 0 finding(s)

sh -n demos/zundamon-orbit/256/build.sh \
  demos/zundamon-orbit/build-local-d88.sh demos/zundamon-orbit/run-vaeg.sh
git diff --check
# PASS
```

The bounded capture used for automated evidence was intentionally limited to
64 publications.  It therefore exits after one revolution and is not the
interactive human-gate image.  A separate unbounded candidate was rebuilt
through the same local-template workflow:

```text
build/generated/zundamon-orbit/m98w-va2-interactive-candidate/
  zundamon-orbit-m98w-interactive-pristine.d88
SHA-256: 0a7fbdaa1c96e2679d6c1bcb9ad539bb149c1c4f72fbe4b1b4678f802c99c57b
```

This image is unbounded and remains in `ZUNDORB` until ESC. It is local-only
and ignored. Place it in VA2 FDD1, boot the public system disk, run `ZUNDORB`,
verify at least three revolutions, the dirty-row visual checks and controls,
then press ESC. The candidate is not tracked.

## Reproducible guest builds and exclusions

The current dirty-mode guest is 34,656 bytes. Two clean QA builds for each
count were byte-identical and below 64 KiB:

| Count | SHA-256 |
|---:|---|
| 1 | `21999e345afa2981de67f902d350dad069501836155eac87fe4f729519f64ada` |
| 2 | `a04e1edca746fe81b5dcdccec4a8f36867251c30ed6c91fef4d84bb381adc1eb` |
| 4 | `0ec2046060dfdf490a2dbbbc44241286e570b4fbb4d7bec86c0bd5616c65a2c4` |
| 8 | `0d0e85af8fe9c762987c9b086c2e7078607be4ec56b8174cc114c7265389fed4` |
| 16 | `f9ce7e07087c73d4c04563738e0854559fb28077fb0f3329c87203e2f066c21f` |

The one-bank atlas and public HUD are regenerated and independently checked.
All COM/BIN/LST/D88 files, traces, captures, save states, backup RAM, and
oracle JSON remain generated or ignored. No private/ROM-derived bytes or
absolute/private paths are in the tracked M98w scope.

## Limitations and gate

M98w is rectangle-based row-union clearing for the public build-time counts
1/2/4/8/16, with interactive count 4. It has no runtime count controls, `/N`,
UP/DOWN behavior, interval masks, private IDA, multi-instance dirty-union
generalization beyond the bounded 16-record footprint, or physical timing
claim. Full clear remains a QA oracle; dirty mode is the release path.

`G98w passed`
