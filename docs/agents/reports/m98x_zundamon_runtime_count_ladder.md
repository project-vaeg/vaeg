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

# M98x runtime ZUNDAMON count ladder

Status: **G98x PASS**

`REAL_HW_PENDING` remains in force. This report records the public runtime
count implementation and its machine/host evidence. It does not claim a
physical PC-88VA measurement. On 2026-09-01 the maintainer explicitly
confirmed `Human gate passed`, closing G98x for the generated public candidate.

## Authority and commits

- Branch: `topic/m98x-runtime-count-ladder`
- Starting/accepted M98w head: `b65d6c50af1f9bd7f574a17683c637e65212be78`
- Accepted remote: `origin/topic/m98w-multi-dirty-union` resolved to the same
  `b65d6c50af1f9bd7f574a17683c637e65212be78`
- Implementation commits: `7548f3c4f8824d0e9d5794ec805223594bd34dde`,
  followed by the input compatibility fix
  `4537a9214a58dcf23fbc196beefcb1a963551bd9`, and the local-disk packaging
  correction `e833b977f671c921d9ad247249d2217c6782cc52`
- Report commit: this report commit; the exact self-referential hash is
  supplied by the final Git handoff after commit and push
- Pushed remote head: this report commit on the M98x topic branch
- Accepted M98w report:
  `docs/agents/reports/m98w_zundamon_multi_dirty_union.md`
- Accepted M98w implementation: `2e402fa5bb69277aa7e4b60575e4ac2e8ccf9ae7`
- Accepted M98w report/approval head: `b65d6c50af1f9bd7f574a17683c637e65212be78`
- G98w approval: maintainer explicitly stated `G98w passed`.
- G98x approval: on 2026-09-01 the maintainer explicitly stated
  `Human gate passed` after the count-control candidate verification.
- Accepted M98w evidence: 212 host tests, 1,280 full/dirty frame pairs,
  40 static count/divisor cases, and the count-four human candidate passed;
  the accepted M98w guest was 34,656 bytes and hardware remained
  `REAL_HW_PENDING`.

The accepted M98w candidate remains generated and ignored. After the
PC-88 cursor-code compatibility fix, M98x generated the fresh local candidate
`build/generated/zundamon-orbit/m98x-va2-updown-fix/zundamon-orbit-m98x-pristine.d88`
with SHA-256
`23254520793fe5c53ad4366d5a264de78a7d97222c659204c420611b54a4fdd0`.
It is not tracked.

The packaging correction keeps `M98X_RUNTIME_MODE=1` when
`build-local-d88.sh` is called directly by the runtime candidate workflow.
This prevents a locally rebuilt disk from silently replacing the runtime
guest with the fixed-count M98w guest. The current pristine candidate was
rebuilt through that corrected path at
`build/generated/zundamon-orbit/m98x-updown-current/zundamon-orbit-m98x-pristine.d88`
with SHA-256
`23254520793fe5c53ad4366d5a264de78a7d97222c659204c420611b54a4fdd0`.
Its embedded `ZUNDORB.COM` is 36,320 bytes with SHA-256
`c8edcca160f6b1a8d96e6d119a54bcaa5af987224a5da252f690bb17d6d47d18` and
contains the PC-88 cursor make-code handlers (`3Ah`/`3Dh`) used by UP/DOWN.
The candidate remains generated and ignored.

## Worktree and changed files

The complete pre-existing dirty baseline was preserved exactly:

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

The final status after the report commit is the same list. No baseline path
was staged, reformatted, overwritten, stashed, or removed.

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/build.sh` | Select one runtime-count build mode by default while retaining fixed-count M98v/M98w QA mode. |
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Parse `/N`, latch bounded requested/pending/visible counts, handle UP/DOWN, select runtime count tiles, and commit count state with complete publication. |
| `demos/zundamon-orbit/256/zundamon_hud_table.inc` | Generated 16 fixed-width count tiles for counts 1 through 16. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_hud.py` | Generate and hash the complete runtime count HUD table. |
| `demos/zundamon-orbit/tools/validate_zundamon_orbit_hud.py` | Independently validate all 16 count tiles and M98x diagnostics. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_multi_debug.py` | Add bounded `/N` launch lines to generated capture scripts. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_multi_guest.py` | Resolve the expanded count-tile index while preserving M98w full/dirty oracle checks. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_depth_guest.py` | Update the generated HUD identity to the M98x table digest. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_multi_guest.py` | Preserve M98v/M98w checks and cover runtime capture-script construction. |
| `demos/zundamon-orbit/tools/zundamon_runtime_count.py` | Independent parser, request-boundary model, 1,024-state checks, and 32,768 transition serialization. |
| `demos/zundamon-orbit/tools/test_zundamon_runtime_count.py` | Runtime parser, saturation, geometry, transition, serialization, HUD, and deterministic-build tests. |
| `demos/zundamon-orbit/run-m98x-vaeg.sh` | Build a fresh ignored runtime candidate and run a bounded VAEG capture. |
| `demos/zundamon-orbit/build-local-d88.sh` | Label runtime-generated candidates as M98x without changing refusal-to-overwrite behavior. |
| `demos/zundamon-orbit/README.md` | Document runtime count syntax, boundary semantics, and local candidate use. |
| `docs/agents/tasks/M98x_zundamon_runtime_count_ladder.md` | Archive the active M98x contract and gate status. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Record M98x automated evidence and keep private IDA in M98y. |
| `docs/agents/ROADMAP.md` | Record M98x as the active human-gate-pending stage. |

## Runtime contract

The normal build has one binary and defaults to count 4. `M98V_ACTIVE_COUNT`
is recognized only when explicitly selecting legacy fixed-count M98v/M98w QA;
normal builds do not dispatch to count-specific binaries or duplicate atlas
payloads. The generated runtime guest is 36,320 bytes, below the 64-KiB COM
limit. Its resident record/index/footprint storage remains statically bounded
at the accepted 16-instance capacity; no heap, recursion, or per-count payload
copy was added. The atlas remains the existing single 5,912-byte public image,
30 descriptors, IDs 1 through 30 only, and one 128-KiB BMS bank.

The accepted default/count reconciliation is:

```text
normal default: 4
explicit initial range: /N1 through /N16
interactive range: 1 through 16
load-ladder checkpoints: 1, 2, 4, 8, 16
```

The parser scans the complete PSP token tail before graphics mode. It accepts
at most one ASCII-case-insensitive `/N1`..`/N16` and one `/V1`..`/V8` token in
either order. `/N`, zero, signed, leading-zero, trailing-junk, separated,
equals-sign, unknown, and duplicate forms fail closed; host diagnostics are
`M98X_INVALID_N`, `M98X_DUPLICATE_N`, `M98X_INVALID_V`,
`M98X_DUPLICATE_V`, and `M98X_UNKNOWN_OPTION`. No numeric prefix is accepted.

UP and DOWN consume the same make/press path as the accepted cadence controls.
The VA2/PC-88 BIOS returns cursor make codes `3Ah` (UP) and `3Dh` (DOWN);
the compatibility path also accepts `48h`/`50h` without changing debounce or
one-press handling. Each accepted press changes the requested count by one,
increments the request
generation, and coalesces while a frame is rendering or READY. At 16/1 the
corresponding key saturates, increments the no-op counter, and never wraps.
UP/DOWN are compiled out of the fixed M98v/M98w QA mode.

The guest keeps `requested_count`, `next_render_count`,
`pending_render_count`, `visible_published_count`, request/pending/published
generations, and the immutable `build_active_count` distinct. A key press
updates only requested control state. A transaction latches the latest request
before generation and keeps that count unchanged through dirty clear, draw,
READY, and publication. After a successful DSA1 publication the page footprint,
visible count, generation, and HUD identity commit, then the global phase
advances once. Misses, pause, divisor changes, and failures do not mutate the
immutable pending frame.

The existing M98w page-local old-count/rectangle lists, row-union clear, BMS
ordering, complete far-to-near draw list, VBLANK eligibility, and cleanup path
are unchanged. Count decreases clear the hidden page's committed old footprint
before drawing the smaller complete list; increases draw the complete new list.
The first publication remains same-count, and no count-specific phase reset or
automatic load reduction exists.

## Publication-synchronous HUD

The existing G0 layout is unchanged:

```text
HUD_RECT       [4,4,70,20)
FPS_VALUE_RECT [34,4,52,12)
HUD_COUNT_RECT [58,12,70,20)
```

The generated public 5x7 font writes exactly two cells for every count. Values
1 through 9 are space-padded (`ZUNDAMON: 1` through `ZUNDAMON: 9`); values 10
through 16 use the intentional no-space form (`ZUNDAMON:10` through
`ZUNDAMON:16`). The count tile digest is
`fa5552dd236cc078e94d905e35698a9887269ede13aa4db86658988b16775b8e`.

Initialization renders the selected count once. A queued request does not
rewrite visible digits. When a complete pending frame has a different count,
the fixed-width G0 field is written at the publication boundary immediately
before DSA1; the field is selected from the validated 1..16 table and never
writes G1 or outside `HUD_COUNT_RECT`. The old visible field is represented by
the committed published count. If DSA1 publication fails after staging the
new field, the old tile is redrawn through a counter-free rollback path and
`hud_count_rollbacks` is incremented. The existing FPS field continues to
update only for an applied divisor boundary. `ZUNDAMON: 1` is no longer a
fixed release value because M98x makes the currently published runtime count
visible; this is the requested M98x HUD change, not an instance-count control
in an earlier milestone.

## Host/reference evidence

The independent host model uses the accepted M98u generator and does not read
guest trace output. It validates all 1,024 `(count, global_phase)` states and
generates exactly 8,704 records. It validates parser/state saturation,
count-one equality to M98t, and the exact count/phase/page transition matrix:

```text
16 old counts * 16 new counts * 64 phases * 2 pages = 32,768 cases
canonical transition digest = 6dcb6103f6db88cdaa81568d05617bce263d7b6ee509c25d585da8941f5cdb68
```

The serialization uses fixed insertion order, decimal JSON values, stable
count/phase/page order, LF endings, and no absolute path, timestamp, hostname,
pointer, private value, or guest trace. It records old/new logical rectangles,
descriptor/source identity, and draw order. The existing independent M98u,
M98v, and M98w compositors continue to validate geometry, transparency,
page-local clearing, and G0 invariance.

The host static load matrix covers all 5 checkpoint counts × 8 divisors (40
bounded model rows), with continuous phase and no automatic count/divisor
change. The same runtime binary completed all 40 bounded VAEG captures
(counts 1, 2, 4, 8, and 16 crossed with V1 through V8), plus opposite-page
count-four V1/V8 captures and representative ladder, pause, and missed-slot
scripts. These are emulator smoke/capture results, not physical timing
evidence. The VA2 human control pass remains part of the pending gate.

## State, counters, and failure handling

The request, next-render, pending-render, and visible-published counts are
separate bounded bytes. A request changes only the requested count and its
generation; the latched pending count remains immutable through dirty clear,
draw, READY, and publication. Successful publication updates the visible
count/HUD, commits the page footprint, and advances the phase once. Endpoint
UP/DOWN presses saturate at 16/1. Normal bounded captures therefore have
`hud_count_rollbacks=0`, `hud_mismatches=0`, and
`partial_publication_attempts=0`, with no automatic count or divisor
reduction. Count-one is the first same-count publication, so the transition
matrix has no out-of-range sentinel.

The host checks assert the stable equations
`published_frames=page_flips=complete_frames_published`,
`instances_published=published_frames*visible_published_count`, and
`requested_slots=published_frames+missed_slots` for drained segments. The
guest retains M98w's independent page footprints, zero release full-page
fallbacks, one shared atlas bank, complete draw lists, and common cleanup.
The added HUD rollback redraws the committed visible count if a staged count
field cannot be paired with DSA1; it does not increment publication counters.

Parser negatives and state negatives use stable host codes, including
`M98X_INVALID_N`, `M98X_DUPLICATE_N`, `M98X_INVALID_V`,
`M98X_DUPLICATE_V`, `M98X_UNKNOWN_OPTION`, `M98X_COUNT_STATE`,
`M98X_FRAME_ALREADY_PENDING`, and `M98X_NO_PENDING_FRAME`. The bounded host
suite also rejects malformed state cardinality, instance permutations,
depth-order violations, transition-count mismatches, nondeterministic
serialization, and private absolute-path material. Guest failures retain the
last complete page, avoid pending-state commit, and converge on the inherited
single cleanup path.

## Build and verification results

The following completed successfully:

```text
PYTHONPYCACHEPREFIX=/tmp/m98x-pyc python3 -m unittest discover \
  -s demos/zundamon-orbit/tools -p 'test_*.py'
224 tests PASS

python3 tools/repo/check_case.py
0 finding(s)
python3 tools/repo/check_encoding.py --expect utf8
0 violation(s)
python3 tools/repo/check_eol.py --enforce
0 violation(s)

cmake --build build/macos-macports -j2
ninja: no work to do

build/macos-macports/sdl2/vaeg --selftest
selftest PASS (all reported checks)
```

The HUD generator was run twice, validated independently, and compared
byte-for-byte. Two clean runtime guest builds were byte-identical:

```text
size: 36,320 bytes
SHA-256: c8edcca160f6b1a8d96e6d119a54bcaa5af987224a5da252f690bb17d6d47d18
```

The generated atlas still validates as one bank with exactly 30 descriptors.
The M98w count-four dirty-union regression completed 64 publications with
`errors=[]`, zero full-page steady clears, and the accepted page/union/source
invariants. The runtime `/N16 /V1` and `/N1 /V8` captures completed with the
same guest and returned normally; their first publication register signatures
reported the requested runtime count and one complete list, not a prefix.

Generated D88s, COM/LST files, VAEG traces, framebuffer captures, and temporary
serialization output are all under ignored `build/generated/` or temporary
directories. `git check-ignore` confirms the candidate and guest artifacts are
ignored. No private IDA/ROM-derived bytes were added to tracked files.

## Closure and limitations

The candidate is the public synthetic ZUNDAMON fixture only. M98x has one
runtime binary, no private IDA, no gameplay/projectiles, no multi-instance
sorting change, and no hardware timing claim. VAEG rates are diagnostic; the
nominal FPS HUD is a selector label. The maintainer's explicit human-gate
approval closes the M98x scope. Private IDA integration remains the separate
M98y milestone.

**G98x PASS**
