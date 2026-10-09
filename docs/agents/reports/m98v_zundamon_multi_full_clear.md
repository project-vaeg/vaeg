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

# M98v multi-ZUNDAMON full-page-clear result

Status: **automated evidence passed; G98v human gate pending**

## Result

`M98v AUTOMATED RESULT: PASS`

`G98v: human gate pending`

The 16-bit guest now consumes the accepted M98u bounded records and explicit
far-to-near order for build-time counts 1, 2, 4, 8, and 16. Every steady
hidden-frame transaction performs one exact 64,000-byte CLS, then exactly the
selected count of transparent BITBLTs. The page becomes READY only after the
last list completes and can publish only at an eligible VBLANK edge.

All 1,280 two-revolution page/frame comparisons passed byte for byte. All 40
count/divisor runs, count-four ladder, pause, and missed-slot cases passed.
The independent host compositor, stable negative cases, 197 host tests,
VAEG selftest, deterministic builds, and repository checks passed. The
count-four interactive candidate is generated but has not received the
maintainer's visual approval.

## Git and accepted predecessor

- Branch: `topic/m98v-multi-full-clear`
- Starting commit and accepted M98u pushed head:
  `899678f28b301f62fa7096c7c5afb4d2cabf874b`
- Accepted M98u implementation:
  `61618f23b88730db157036d22fc2a3aa15986206`
- M98v implementation:
  `5c45ce84a61682b9fd9fd32f57aec43143e1c699`
- Report/pushed-head commit: supplied in the final handoff because a commit
  cannot contain its own identity.
- Accepted predecessor report:
  `docs/agents/reports/m98u_zundamon_multi_instance_state.md`

Before editing, `origin/topic/m98u-multi-instance-state` resolved exactly to
`899678f2`, and that commit is the direct ancestor of the M98v implementation.
The maintainer explicitly supplied G98u approval. The accepted M98u canonical
golden regenerated twice at
`6ed8e4e4b70ed62547d0feca9847999f730e25b6e4d19e0a16c670021c5a3e52`:
1,024 lists, 8,704 records, and 1,024 draw orders. All 187 accepted predecessor
tests passed before M98v edits. The predecessor guest also rebuilt twice as
32,656 bytes with its accepted SHA-256
`b6e1bbc2a600f22ca583e256c82cccab3c1523530a0a2a7836439d4cb74d87ec`.

## Preserved dirty-worktree baseline

The following unrelated entries existed before the milestone and remain
unstaged and unchanged:

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

The final dirty state is the same list. No pre-existing entry was staged,
reformatted, overwritten, stashed, or removed.

## Changed files

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/build.sh` | Restrict the build count, require an explicit bounded-QA count, and force the M98v full-clear path. |
| `demos/zundamon-orbit/256/zundamon_hud_table.inc` | Add deterministic public fixed-width count tiles for 1/2/4/8/16. |
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Generate/sort bounded records and run indivisible full-clear multi-draw transactions. |
| `demos/zundamon-orbit/README.md` | Document the build-time count and M98v workflow. |
| `demos/zundamon-orbit/build-local-d88.sh` | Name and describe the M98v local candidate without changing the source template. |
| `demos/zundamon-orbit/run-vaeg.sh` | Build and verify count-specific bounded M98v cases. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_hud.py` | Generate all five static count tiles. |
| `demos/zundamon-orbit/tools/validate_zundamon_orbit_hud.py` | Independently validate count tile pixels, bounds, and colors. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_depth_guest.py` | Update the accepted deterministic HUD-include identity. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_multi_debug.py` | Generate bounded checkpoint scripts for count/page/divisor/scenario cases. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_multi_guest.py` | Independently compose and byte-compare complete multi-instance pages and traces. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_multi_guest.py` | Test all counts/phases, ordering, overlap semantics, build restrictions, and stable faults. |
| `docs/agents/ROADMAP.md` | Record M98v assignment and automated/human-gate state. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Keep M98v full clear separate from M98w unions and M98x controls. |
| `docs/agents/tasks/M98v_zundamon_multi_full_clear.md` | Record the executable task contract and gate state. |
| `docs/agents/reports/m98v_zundamon_multi_full_clear.md` | Record this validation result and human handoff. |

No emulator source, atlas pixel, phase/depth/scale entry, orbit radius,
descriptor geometry, BMS capacity, SGP multiplier, cadence rule, or private
input changed.

## Build-time count and record contract

`M98V_ACTIVE_COUNT` accepts exactly `1`, `2`, `4`, `8`, or `16`; the normal
build defaults to four. Bounded QA refuses an absent explicit value. Zero,
3, 5, 15, 17, negative, signed, malformed, and other values fail before NASM.
There is no runtime requested-count state, `/N` parser, or UP/DOWN action.

For global phase `g`, active count `n`, and instance ID `i`, the guest uses the
accepted direct M98u formula:

```text
offset = floor(64*i/n)
phase  = (g + offset) & 63
```

It fills the accepted 50-byte M98u record in an 800-byte, 16-record fixed
buffer. A separate 16-byte index array is insertion-sorted by
`(signed depth_rank ascending, instance_id ascending)`. There is no heap,
recursion, pointer serialization, copied atlas payload, or 1,024-state guest
table. Startup validates all 64 states for the selected count before graphics
mode. The host test still validates every M98u count 1 through 16.

## Atlas, pages, and SGP transaction

The public atlas remains 5,912 bytes with SHA-256
`7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa`.
It has exactly 30 descriptors, IDs 1 through 30, no 0/31, and occupies one
128 KiB bank. Every record reuses selector 1 and the accepted descriptor's
pitch, anchor, payload length, bank offset, SGP source, and frame CRC. Every
source range is inside the loaded aperture at SGP base `080000h`; the observed
descriptor sources remain within `0801b0h..081150h`.

G1 remains a 320x400 surface at a 320-byte pitch:

| Page | SGP base | DSA1 byte-address value | Bytes |
|---|---:|---:|---:|
| A | `220000h` | `020000h` | 64,000 |
| B | `22fa00h` | `02fa00h` | 64,000 |

DSA1 is programmed through its accepted word-wide port pair while the value
is a byte address. Compile-time checks retain page size, non-overlap, backing
bounds, and word-count equality (`32,000 words == 64,000 bytes`).

The SGP list buffer is 64 words. M98v deliberately uses one ordered full-page
clear list followed by one ordered BITBLT list per instance. Thus count 16
uses 17 bounded lists per steady frame instead of assuming all commands fit
one list. Each clear uses SGP CLS with base equal to the hidden page and count
`7d00h` 16-bit words. Each draw uses mode `0105h`; zero source bytes remain
transparent.

The partial order is:

```text
SGP idle -> select bank 1 -> hidden CLS completes
         -> ordered BITBLT[0] completes -> ... -> BITBLT[n-1] completes
         -> restore ordinary selector 0 -> READY
         -> eligible low-to-high VBLANK -> DSA1 -> commit -> phase advance
```

The selected bank, record, source, command storage, and hidden-page role are
stable while each list is BUSY. VBLANK edges continue to be observed through
every bounded wait. The release renderer has one steady full-page clear per
frame, zero dirty-row/union clears, exactly two separately counted
initialization clears, and no hidden write after publication.

For a 128-frame count `n` run, command accounting is:

```text
SGP lists     = 1 + 128*(1+n)
SGP commands  = 5 + 128*(3+4*n)
steady CLS    = 128
CLS bytes     = 8,192,000
CLS words     = 4,096,000
BITBLTs       = 128*n
BMS selections= 128
BMS switches  = 256
```

The leading list contains the two initialization clears. Ordinary BMS mapping
is restored after every complete render and by the common cleanup path.

## HUD and controls

The accepted G0 HUD remains `[4,4,70,20)`. The existing FPS value rectangle
is unchanged. The count rectangle is `[58,12,70,20)` and contains a complete
12x8 fixed-width tile:

| Build count | Field | Complete line |
|---:|:---:|---|
| 1 | ` 1` | `ZUNDAMON: 1` |
| 2 | ` 2` | `ZUNDAMON: 2` |
| 4 | ` 4` | `ZUNDAMON: 4` |
| 8 | ` 8` | `ZUNDAMON: 8` |
| 16 | `16` | `ZUNDAMON:16` |

The public task-authored 5x7 font, foreground `ffh`, background `01h`, and G0
write path are unchanged. The complete HUD initializes once, the count field
writes once, and only an applied cadence change updates the FPS field. No HUD
write reaches G1 or G0 outside the fixed HUD. The generated include is 54,376
bytes with SHA-256
`95887389e3da7bc0fd70e69ff92909389c2e979c9598d093c7390f8947e8acb1`.

LEFT/RIGHT, SPACE, ESC, divisor-boundary resets, READY, pause, and missed-slot
semantics remain accepted. UP/DOWN remain inactive. A miss keeps the complete
old visible page, pending phase, full instance list, and build count.

## Independent compositor and overlap evidence

The host oracle allocates a zeroed 64,000-byte G1 page, derives the exact M98u
records, traverses the sorted indices, and copies only nonzero public source
bytes. It then composes that page over the independently generated G0/HUD and
compares all 64,000 G1 bytes and all 64,000 composite bytes, both physical
pages, guards, HUD/outside-HUD bytes, phases, records, rectangles, sources,
and SGP trace order.

The accepted public geometry naturally has zero inter-instance overlap pixels
even at count 16; the orbit was not moved to manufacture overlap. A host-only
2x2 synthetic compositor fixture proves that a nearer opaque byte overwrites
a farther opaque byte, a nearer transparent byte preserves the farther byte,
and an equal-depth larger ID is submitted last. This is compositor evidence,
not a claim that the public candidate visibly overlaps.

## Both-page, two-revolution matrix

Every row published phases `0..63,0..63`, recorded two publications for every
phase, two wraps, 128 page flips, 128 steady full-page clears, two initial
clears, zero partial publications, and zero framebuffer/source/bounds/guard/
timeout errors. `CLS trace` includes the two initialization CLS commands.

| Count | Initial page | Frames | Instances/BITBLTs | CLS trace | Clear bytes | Source bytes | Frame identity | Result |
|---:|:---:|---:|---:|---:|---:|---:|---|---|
| 1 | A | 128 | 128 | 130 | 8,192,000 | 19,856 | `a2fda2e3c3c9ddd9` | PASS |
| 1 | B | 128 | 128 | 130 | 8,192,000 | 19,856 | `b1ac38bd6d11a1e1` | PASS |
| 2 | A | 128 | 256 | 130 | 8,192,000 | 39,712 | `b7e5610b9510dcf0` | PASS |
| 2 | B | 128 | 256 | 130 | 8,192,000 | 39,712 | `8225ce84eeb5bb3e` | PASS |
| 4 | A | 128 | 512 | 130 | 8,192,000 | 79,424 | `2157c4293fcb7103` | PASS |
| 4 | B | 128 | 512 | 130 | 8,192,000 | 79,424 | `dafca6a2b7bc1c68` | PASS |
| 8 | A | 128 | 1,024 | 130 | 8,192,000 | 158,848 | `b6e95959d74b2702` | PASS |
| 8 | B | 128 | 1,024 | 130 | 8,192,000 | 158,848 | `cc501f08ca83868a` | PASS |
| 16 | A | 128 | 2,048 | 130 | 8,192,000 | 317,696 | `b0b8b551c481e468` | PASS |
| 16 | B | 128 | 2,048 | 130 | 8,192,000 | 317,696 | `bdfb9542c77a9846` | PASS |

These are 1,280 byte-for-byte publication comparisons. Count one matches the
accepted M98t full-clear oracle for all 64 phases and both page parities.

## Forty count/divisor runs

The VAEG VA2 timing reference is 59.95 Hz. Requested and observed rates below
are derived from the guest-observed edge count; they are not physical-hardware
rates. Every row published exactly 64 complete frames, cleared 4,096,000
steady bytes, matched the independent compositor, and preserved its static
count HUD.

| Count | V | Nominal | Edges/slots | Pub/miss | Requested/observed Hz | Revolution s | Lists/commands | Source bytes |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 1 | 60 | 64/64 | 64/0 | 59.950/59.950 | 1.068 | 129/453 | 9,928 |
| 1 | 2 | 30 | 128/64 | 64/0 | 29.975/29.975 | 2.135 | 129/453 | 9,928 |
| 1 | 3 | 20 | 192/64 | 64/0 | 19.983/19.983 | 3.203 | 129/453 | 9,928 |
| 1 | 4 | 15 | 256/64 | 64/0 | 14.988/14.988 | 4.270 | 129/453 | 9,928 |
| 1 | 5 | 12 | 320/64 | 64/0 | 11.990/11.990 | 5.338 | 129/453 | 9,928 |
| 1 | 6 | 10 | 384/64 | 64/0 | 9.992/9.992 | 6.405 | 129/453 | 9,928 |
| 1 | 7 | 8.6 | 448/64 | 64/0 | 8.564/8.564 | 7.473 | 129/453 | 9,928 |
| 1 | 8 | 7.5 | 512/64 | 64/0 | 7.494/7.494 | 8.540 | 129/453 | 9,928 |
| 2 | 1 | 60 | 117/117 | 64/53 | 59.950/32.793 | 1.952 | 193/709 | 19,856 |
| 2 | 2 | 30 | 128/64 | 64/0 | 29.975/29.975 | 2.135 | 193/709 | 19,856 |
| 2 | 3 | 20 | 192/64 | 64/0 | 19.983/19.983 | 3.203 | 193/709 | 19,856 |
| 2 | 4 | 15 | 256/64 | 64/0 | 14.988/14.988 | 4.270 | 193/709 | 19,856 |
| 2 | 5 | 12 | 320/64 | 64/0 | 11.990/11.990 | 5.338 | 193/709 | 19,856 |
| 2 | 6 | 10 | 384/64 | 64/0 | 9.992/9.992 | 6.405 | 193/709 | 19,856 |
| 2 | 7 | 8.6 | 448/64 | 64/0 | 8.564/8.564 | 7.473 | 193/709 | 19,856 |
| 2 | 8 | 7.5 | 512/64 | 64/0 | 7.494/7.494 | 8.540 | 193/709 | 19,856 |
| 4 | 1 | 60 | 127/127 | 64/63 | 59.950/30.211 | 2.118 | 321/1,221 | 39,712 |
| 4 | 2 | 30 | 128/64 | 64/0 | 29.975/29.975 | 2.135 | 321/1,221 | 39,712 |
| 4 | 3 | 20 | 192/64 | 64/0 | 19.983/19.983 | 3.203 | 321/1,221 | 39,712 |
| 4 | 4 | 15 | 256/64 | 64/0 | 14.988/14.988 | 4.270 | 321/1,221 | 39,712 |
| 4 | 5 | 12 | 320/64 | 64/0 | 11.990/11.990 | 5.338 | 321/1,221 | 39,712 |
| 4 | 6 | 10 | 384/64 | 64/0 | 9.992/9.992 | 6.405 | 321/1,221 | 39,712 |
| 4 | 7 | 8.6 | 448/64 | 64/0 | 8.564/8.564 | 7.473 | 321/1,221 | 39,712 |
| 4 | 8 | 7.5 | 512/64 | 64/0 | 7.494/7.494 | 8.540 | 321/1,221 | 39,712 |
| 8 | 1 | 60 | 64/64 | 64/0 | 59.950/59.950 | 1.068 | 577/2,245 | 79,424 |
| 8 | 2 | 30 | 128/64 | 64/0 | 29.975/29.975 | 2.135 | 577/2,245 | 79,424 |
| 8 | 3 | 20 | 192/64 | 64/0 | 19.983/19.983 | 3.203 | 577/2,245 | 79,424 |
| 8 | 4 | 15 | 256/64 | 64/0 | 14.988/14.988 | 4.270 | 577/2,245 | 79,424 |
| 8 | 5 | 12 | 320/64 | 64/0 | 11.990/11.990 | 5.338 | 577/2,245 | 79,424 |
| 8 | 6 | 10 | 384/64 | 64/0 | 9.992/9.992 | 6.405 | 577/2,245 | 79,424 |
| 8 | 7 | 8.6 | 448/64 | 64/0 | 8.564/8.564 | 7.473 | 577/2,245 | 79,424 |
| 8 | 8 | 7.5 | 512/64 | 64/0 | 7.494/7.494 | 8.540 | 577/2,245 | 79,424 |
| 16 | 1 | 60 | 64/64 | 64/0 | 59.950/59.950 | 1.068 | 1,089/4,293 | 158,848 |
| 16 | 2 | 30 | 128/64 | 64/0 | 29.975/29.975 | 2.135 | 1,089/4,293 | 158,848 |
| 16 | 3 | 20 | 192/64 | 64/0 | 19.983/19.983 | 3.203 | 1,089/4,293 | 158,848 |
| 16 | 4 | 15 | 256/64 | 64/0 | 14.988/14.988 | 4.270 | 1,089/4,293 | 158,848 |
| 16 | 5 | 12 | 320/64 | 64/0 | 11.990/11.990 | 5.338 | 1,089/4,293 | 158,848 |
| 16 | 6 | 10 | 384/64 | 64/0 | 9.992/9.992 | 6.405 | 1,089/4,293 | 158,848 |
| 16 | 7 | 8.6 | 448/64 | 64/0 | 8.564/8.564 | 7.473 | 1,089/4,293 | 158,848 |
| 16 | 8 | 7.5 | 512/64 | 64/0 | 7.494/7.494 | 8.540 | 1,089/4,293 | 158,848 |

The non-monotonic V1 misses are observed VAEG scheduler telemetry, not a
throughput claim. Every requested-slot invariant is
`requested_slots == published_frames + missed_slots`; every miss retained the
old complete page and later published the original unskipped phase/count.

## Dynamic count-four cases

| Scenario | Frames | Instances | Misses | Lists/commands | Source bytes | Result |
|---|---:|---:|---:|---:|---:|---|
| V1-to-V8-to-V1 ladder | 128 | 512 | 74 | 641/2,437 | 79,424 | PASS |
| pause/resume | 128 | 512 | 124 | 641/2,437 | 79,424 | PASS |
| consecutive missed slots | 128 | 512 | 129 | 641/2,437 | 79,424 | PASS |

The ladder applied every field at the accepted boundary, including requests
while rendering, and retained `ZUNDAMON: 4`. Pause froze the complete visible
frame; resume waited a full divisor interval. Miss injection never flipped a
partial page, changed count, or skipped phase. All three ran two revolutions,
performed 128 full clears and 512 BITBLTs, and reached cleanup once.

## Counter and transaction invariants

Every bounded passing run required:

```text
requested_slots == published_frames + missed_slots
published_frames == page_flips == complete_frames_published
instances_published == published_frames * build_active_count
complete_frames_started == complete_frames_ready == published_frames
instances_planned == instances_submitted == instances_completed
                  == instances_published
steady_full_page_clears == published_frames
transparent_bitblts == instances_published
partial_publication_attempts == 0
draw_order_failures == tie_break_failures == 0
dirty_rect_clears == dirty_row_cls_commands == 0
sgp_timeouts == sgp_errors == vblank_timeouts == 0
bounds_failures == source_failures == guard_failures == 0
framebuffer_mismatches == hud_mismatches == 0
runtime_count_changes == 0
cleanup_runs == 1
```

Every phase counter equals the revolution count. The complete scale histogram
equals the accepted M98t per-revolution histogram multiplied by the active
count and revolution count. All page/source/command guards passed.

## Negative and fault-isolation results

Every test starts from a valid transaction, applies one mutation, asserts the
listed stable code, emits no partial valid frame, retains the prior visible
page and global phase, and reaches bounded cleanup once.

| # | Mutation | Stable code/result |
|---:|---|---|
| 1 | missing/unapproved build count | `M98V_ACTIVE_COUNT`, PASS |
| 2 | runtime count mutation | `M98V_RUNTIME_COUNT_MUTATION`, PASS |
| 3 | malformed/incomplete record list | `M98V_RECORD_LIST`, PASS |
| 4 | wrong/duplicate phase assignment | `M98V_PHASE_ASSIGNMENT`, PASS |
| 5 | missing/duplicate/out-of-range order index | `M98V_DRAW_PERMUTATION`, PASS |
| 6 | near-to-far order | `M98V_NEAR_TO_FAR`, PASS |
| 7 | wrong equal-depth ID order | `M98V_TIE_ORDER`, PASS |
| 8 | descriptor/anchor/scale/payload/source mismatch | `M98V_DESCRIPTOR`, PASS |
| 9 | screen/page/source destination violation | `M98V_DESTINATION`, PASS |
| 10 | atlas payload duplicated per record | `M98V_ATLAS_DUPLICATE`, PASS |
| 11 | missing frame CLS | `M98V_CLS_MISSING`, PASS |
| 12 | CLS not exactly 64,000 bytes | `M98V_CLS_SIZE`, PASS |
| 13 | CLS targeting visible page | `M98V_CLS_VISIBLE`, PASS |
| 14 | BITBLT before clear completion | `M98V_EARLY_BITBLT`, PASS |
| 15 | submission in unsorted record order | `M98V_UNSORTED_SUBMISSION`, PASS |
| 16 | omitted/duplicated instance | `M98V_DRAW_CARDINALITY`, PASS |
| 17 | source-zero transparency disabled | `M98V_TRANSPARENCY`, PASS |
| 18 | nearer opaque byte fails to overwrite | `M98V_NEAR_OVERWRITE`, PASS |
| 19 | nearer transparent byte erases far byte | `M98V_TRANSPARENT_ERASE`, PASS |
| 20 | equal-depth visual tie differs from ID order | `M98V_VISUAL_TIE`, PASS |
| 21 | command capacity without bounded split | `M98V_BATCH_CAPACITY`, PASS |
| 22 | publication between lists | `M98V_BATCH_PUBLICATION`, PASS |
| 23 | READY before last completion | `M98V_EARLY_READY`, PASS |
| 24 | partial/non-READY DSA1 flip | `M98V_PARTIAL_DSA1`, PASS |
| 25 | phase commit before publication | `M98V_EARLY_PHASE`, PASS |
| 26 | miss skips phase or reduces count | `M98V_MISS_MUTATION`, PASS |
| 27 | BMS/command mutation while BUSY | `M98V_BUSY_MUTATION`, PASS |
| 28 | CLS timeout/error | `M98V_SGP_CLS_TIMEOUT`, PASS |
| 29 | middle BITBLT timeout/error | `M98V_SGP_MIDDLE_TIMEOUT`, PASS |
| 30 | final BITBLT timeout/error | `M98V_SGP_FINAL_TIMEOUT`, PASS |
| 31 | VBLANK-low timeout | `M98V_VBLANK_LOW_TIMEOUT`, PASS |
| 32 | VBLANK-high timeout | `M98V_VBLANK_HIGH_TIMEOUT`, PASS |
| 33 | static HUD count mismatch | `M98V_HUD_COUNT`, PASS |
| 34 | stale one/two-digit count cell | `M98V_HUD_STALE_DIGIT`, PASS |
| 35 | G0-outside-HUD or G1 HUD write | `M98V_HUD_RANGE`, PASS |
| 36 | UP/DOWN or `/N` activation | `M98V_RUNTIME_CONTROL`, PASS |
| 37 | dirty/union clear in release path | `M98V_DIRTY_CLEAR`, PASS |
| 38 | page/source/command guard corruption | `M98V_GUARD`, PASS |
| 39 | host/guest framebuffer mismatch | `M98V_FRAMEBUFFER`, PASS |
| 40 | private/ROM-derived output | `M98V_PRIVATE_DATA`, PASS |
| 41 | ESC with partial batch/pending action | `M98V_ESC_PENDING`, PASS |

An unknown mutation is separately rejected as `M98V_FAULT_UNKNOWN`, proving
that one expected case cannot pass through another validator branch.

## Commands and checks

Principal commands and results:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98v-final-pyc \
  python3 -m unittest discover \
    -s demos/zundamon-orbit/tools -p 'test_*.py'
# 197 tests, PASS

python3 demos/zundamon-orbit/tools/generate_zundamon_multi_instance_state.py \
  --atlas <generated-public-atlas>/zundorb.bin \
  --depth-table demos/zundamon-orbit/256/zundamon_depth_table.inc \
  --golden-output <generated>/m98u-golden.json \
  --summary-output <generated>/m98u-summary.json \
  --contract-output <generated>/zundamon_multi_instance_contract.inc
python3 demos/zundamon-orbit/tools/validate_zundamon_multi_instance_state.py \
  --golden <generated>/m98u-golden.json \
  --atlas <generated-public-atlas>/zundorb.bin \
  --depth-table demos/zundamon-orbit/256/zundamon_depth_table.inc \
  --contract <generated>/zundamon_multi_instance_contract.inc
# repeated twice, M98u SHA unchanged, PASS

VAEG_ZUNDAMON_ACTIVE_COUNT=<1|2|4|8|16> \
VAEG_ZUNDAMON_DIVISOR=<1..8> \
VAEG_ZUNDAMON_INITIAL_PAGE=<a|b> \
VAEG_ZUNDAMON_REVOLUTIONS=<1|2> \
  demos/zundamon-orbit/run-vaeg.sh \
    <local-template> build/macos-macports/sdl2/vaeg \
    <local-rom-directory> <fresh-generated-output>
# 40 static runs plus ten two-revolution parity runs, PASS

build/macos-macports/sdl2/vaeg --selftest
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
sh -n demos/zundamon-orbit/256/build.sh \
  demos/zundamon-orbit/build-local-d88.sh \
  demos/zundamon-orbit/run-vaeg.sh
python3 -m py_compile demos/zundamon-orbit/tools/*multi*guest.py \
  demos/zundamon-orbit/tools/*hud*.py
git diff --check
```

VAEG selftest and encoding, EOL, case, shell, Python, whitespace, scope,
privacy, and prohibited-artifact checks passed. No emulator source changed, so
the selftest plus the complete VA2 guest matrix is the applicable emulator
regression. Hosted CI was not used as an iterative debugger.

## Reproducibility and generated artifacts

Every release count was built twice from fresh output and compared byte for
byte. All remain below 64 KiB:

| Count | Bytes | SHA-256 |
|---:|---:|---|
| 1 | 34,016 | `9ac804cc9d473e4556ae9e49eb5187c51ab384f13f29cde41f689402b9c83a3b` |
| 2 | 34,016 | `245846f73e7d642eaebca291c3b540accb1e1613c1fa844388605505ac5c7dfa` |
| 4 | 34,016 | `e99648d89101fff8bc3927beeb1079aaebdcd2f1cf3d8a3b30b867fa49f79bb6` |
| 8 | 34,016 | `cadab5266eb3bcbba3ca85e8014e910865e079e57127176f238189bfc0ab8ee1` |
| 16 | 34,016 | `a305996d0abf6da90ace14c57a0e6d8e3a926f808fb35b5992b2c5210457867a` |

The atlas and HUD were each regenerated twice and matched byte for byte. The
actual VAEG binary was `build/macos-macports/sdl2/vaeg`, 8,155,976 bytes,
SHA-256 `13109ea163c6c708e0e79df0b149b1ff317c04381f9ad119e90ac35bc8e73d46`.

Generated COM/BIN files, D88 images, listings, complete oracle JSON, debug
scripts, traces, captures, save states, and backup RAM remain ignored or
pre-existing and untracked. The staged implementation/report scope contains
no COM, BIN, D88, ROM, private path, private identity, ROM identity, capture,
trace, or save-state artifact.

## Interactive candidate and limitations

The pristine generated count-four candidate is:

```text
build/generated/zundamon-orbit/m98v-va2-candidate/
zundamon-orbit-m98v-pristine.d88
```

It is 1,338,960 bytes with SHA-256
`430441977ebf5cfa4023e947f7caee019bc0fbbf1bf231928400d77a489e590f`.
It is local-only and untracked. Launch VAEG in VA2 mode with this image in
FDD1, then run `ZUNDORB` at the prompt. Verify four clockwise instances for at
least two revolutions, HUD `FPS: 60` / `ZUNDAMON: 4`, cadence controls,
SPACE pause/resume, inactive UP/DOWN, identical page parities, no missing
instance/trail/clipping/flicker/tear, and ESC restoration.

The public orbit produces no natural sprite overlap, so the visual gate can
check only non-overlapping four-instance composition; transparent/opaque and
tie overlap semantics are independently proven by the synthetic host fixture.
M98v retains a fixed build-time count and expensive full-page clear. It has no
dirty unions, runtime count controls, private IDA, image rotation, physical
timing evidence, bullets, sound, gameplay, or general sprite engine. M98w and
M98x remain untouched.

No physical PC-88VA/VA2 was tested.

`REAL_HW_PENDING`

`G98v: human gate pending`
