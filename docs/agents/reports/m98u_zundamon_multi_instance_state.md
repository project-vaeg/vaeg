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

# M98u deterministic multi-instance state result

Status: **G98u machine gate passed; M98u closed on 2026-09-01**

## Result

`M98u AUTOMATED RESULT: PASS`

`G98u PASS`

M98u freezes a host/reference-only, fixed-capacity state contract for future
counts 1 through 16. All 1,024 `(active_count, global_phase)` combinations
generated exactly 8,704 records and 1,024 draw orders. Phase assignment,
balanced gaps, accepted M98t geometry and sources, screen/G1 bounds, HUD
exclusion, far-to-near ordering, explicit equal-depth ties, count-one
compatibility, and deterministic serialization all passed independently.

The release guest was not changed. Two final `ZUNDORB.COM` builds remained
32,656 bytes and byte-identical to the accepted M98t release binary. The HUD
still displays `ZUNDAMON: 1`, UP/DOWN remain inactive, `/N` remains absent,
and the renderer still submits one transparent BITBLT for one public object.

## Git and accepted predecessor

- Branch: `topic/m98u-multi-instance-state`
- Accepted M98t implementation:
  `9440798d13bd00229b03163f98f9fee7c4caac68`
- Accepted M98t report head:
  `06d43348a35efb2b93db8272fba961631be146eb`
- M98t human-gate audit head and actual M98u starting commit:
  `ae5bfc9b3fa6284e97390b6b40fb04eea9a0a700`
- M98u implementation:
  `61618f23b88730db157036d22fc2a3aa15986206`
- Report/pushed-head commit: supplied in the final handoff because this file
  cannot contain the SHA of the commit that contains itself.
- Accepted predecessor report:
  `docs/agents/reports/m98t_zundamon_depth_scale_hud.md`

The task assignment named `06d43348` as the M98t remote head. Before M98u,
the maintainer's explicit `G98t passed` statement had already been recorded in
the documentation-only audit commit `ae5bfc9b`, and the live remote M98t branch
resolved to that audit successor. The four-file `06d43348..ae5bfc9b` diff is
documentation only. M98u therefore starts from the live audit head while
retaining `06d43348` unchanged as the accepted implementation/report baseline.
Both commits are ancestors of the M98u implementation.

The accepted generated M98t candidate identity was also rechecked as input
evidence: the relative candidate path remains
`build/generated/zundamon-orbit/m98t-va2-candidate/zundamon-orbit-m98t-pristine.d88`
with SHA-256
`a12483ef3120ac33ade6c9138a5fdc8b8bcb6a9b70b24b764cb952959d940ef5`.
It remains ignored and was not copied, modified, or staged.

## Preserved dirty-worktree baseline

These unrelated entries existed before M98u and remained unstaged and
unchanged by this milestone:

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

The final dirty state is the same list. No pre-existing path was staged,
reformatted, overwritten, stashed, or removed.

## Changed files

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/zundamon_multi_instance_contract.inc` | Freeze the compact future 16-bit record and index capacities without embedding the exhaustive matrix. |
| `demos/zundamon-orbit/tools/generate_zundamon_multi_instance_state.py` | Generate all bounded instance records, balanced phase assignments, explicit insertion-sort orders, and canonical host evidence. |
| `demos/zundamon-orbit/tools/validate_zundamon_multi_instance_state.py` | Independently derive and validate the canonical matrix and compact contract. |
| `demos/zundamon-orbit/tools/test_zundamon_multi_instance_state.py` | Exercise exhaustive positive, covariance, deterministic, and fail-closed negative cases. |
| `demos/zundamon-orbit/README.md` | Document the M98u reference contract and reproducible commands. |
| `docs/agents/ROADMAP.md` | Record the assigned and completed G98u machine gate. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Reconcile M98u through M98y without renumbering M98z or later tasks. |
| `docs/agents/tasks/M98u_zundamon_multi_instance_state.md` | Record the bounded task, exclusions, evidence, and gate result. |
| `docs/agents/reports/m98u_zundamon_multi_instance_state.md` | Record this machine-verifiable result. |

No guest assembly, guest build script, emulator source, atlas input, depth
table, HUD table, renderer, page-state logic, scheduler, or cleanup path
changed.

## Phase assignment and bounded representation

The authoritative formulas are:

```text
MAX_ZUNDAMON_INSTANCES = 16
phase_offset(i,n) = floor(64*i/n)
phase_id(i,n,g) = (g + phase_offset(i,n)) & 63
draw_key = (signed depth_rank ascending, instance_id ascending)
```

All multiplication is checked before division or narrowing. Records are built
in ascending instance-ID order. A fixed 16-byte index scratch array is sorted
by insertion sort; complete records are not copied. There is no heap,
recursion, pointer serialization, platform sort, input-stability tie rule, or
pre-expanded 1,024-state guest table.

The frozen future guest ABI is a 50-byte record with an 800-byte capacity and
a separate 16-byte draw-order capacity:

| Offset | Fields | Type |
|---:|---|---|
| 0..7 | instance ID, phase offset/ID, scale ID, depth, BMS selector, descriptor index, reserved | seven unsigned bytes plus signed 8-bit depth |
| 8..21 | dx/dy, width/height/pitch, descriptor anchor | signed dx/dy; unsigned geometry words |
| 22..33 | target anchor and half-open destination x0/y0/x1/y1 | signed 16-bit words |
| 34..49 | bank offset, SGP source, payload bytes, source identity | unsigned 32-bit values |

The active prefix is exactly `n`. Every draw-order byte indexes that prefix in
`0..n-1`. Instance records contain only references and identities; no atlas
payload or pixel copy is present.

## Accepted M98t derivation path

For each assigned phase, M98u consumes the accepted 64-entry M98t
phase/depth/scale table and the actual public atlas descriptor:

```text
orbit = accepted_m98t_orbit[phase_id]
descriptor = accepted_atlas[orbit.scale_id - 1]
target_anchor = (160 + orbit.dx, 100 + orbit.dy)
dst_x = target_anchor_x - descriptor.anchor_x
dst_y = target_anchor_y - descriptor.anchor_y
rect = [dst_x, dst_y, dst_x + width, dst_y + height)
source = 080000h + descriptor.bank_offset
```

The accepted radii remain `(96,48)`, `HUD_RECT` remains `[4,4,70,20)`, G1
page bases remain `0220000h` and `022fa00h`, and each page is 64,000 bytes at
320-byte pitch. All 8,704 derived records fit 320x200, avoid the HUD, and fit
either physical G1 page.

The public atlas is 5,912 bytes, contains exactly 30 ordered descriptors,
uses selector 1 / logical slot 0 only, and occupies bytes `[0,00001318h)` of
the 128 KiB bank. Its SHA-256 is
`7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa`.
The accepted depth-table SHA-256 is
`645414752dd68898fb382d70d49dcfc4975b722f2927670d45fd8496a036b09c`.

| Scale | Geometry/pitch | Anchor | Bank offset | SGP source | Payload | Frame CRC32 |
|---:|---|---|---:|---:|---:|---:|
| 1 | 1x1/4 | 0,0 | `0000h` | `080000h` | 4 | `2144df1c` |
| 2 | 1x1/4 | 0,0 | `0010h` | `080010h` | 4 | `2144df1c` |
| 3 | 2x2/4 | 1,1 | `0020h` | `080020h` | 8 | `6522df69` |
| 4 | 3x2/4 | 1,1 | `0030h` | `080030h` | 8 | `771f2c43` |
| 5 | 4x3/4 | 2,1 | `0040h` | `080040h` | 12 | `f8966049` |
| 6 | 4x4/4 | 2,2 | `0050h` | `080050h` | 16 | `edecfb25` |
| 7 | 5x4/8 | 2,2 | `0060h` | `080060h` | 32 | `30598022` |
| 8 | 6x5/8 | 3,2 | `0080h` | `080080h` | 40 | `b700b50b` |
| 9 | 7x6/8 | 3,3 | `00b0h` | `0800b0h` | 48 | `9335343d` |
| 10 | 7x6/8 | 3,3 | `00e0h` | `0800e0h` | 48 | `9335343d` |
| 11 | 8x7/8 | 4,3 | `0110h` | `080110h` | 56 | `e85ba94c` |
| 12 | 9x7/12 | 4,3 | `0150h` | `080150h` | 84 | `65adc694` |
| 13 | 10x8/12 | 5,4 | `01b0h` | `0801b0h` | 96 | `3ffc90cb` |
| 14 | 10x9/12 | 5,4 | `0210h` | `080210h` | 108 | `ae6d5fd6` |
| 15 | 11x9/12 | 5,4 | `0280h` | `080280h` | 108 | `4553da3a` |
| 16 | 12x10/12 | 6,5 | `02f0h` | `0802f0h` | 120 | `b5b2b73a` |
| 17 | 13x10/16 | 6,5 | `0370h` | `080370h` | 160 | `6b721af6` |
| 18 | 13x11/16 | 6,5 | `0410h` | `080410h` | 176 | `41c460a9` |
| 19 | 14x12/16 | 7,6 | `04c0h` | `0804c0h` | 192 | `cb50668a` |
| 20 | 15x12/16 | 7,6 | `0580h` | `080580h` | 192 | `0f976ff1` |
| 21 | 16x13/16 | 8,6 | `0640h` | `080640h` | 208 | `8a214f3d` |
| 22 | 16x13/16 | 8,6 | `0710h` | `080710h` | 208 | `8a214f3d` |
| 23 | 17x14/20 | 8,7 | `07e0h` | `0807e0h` | 280 | `74556ddb` |
| 24 | 18x15/20 | 9,7 | `0900h` | `080900h` | 300 | `dc9de828` |
| 25 | 19x15/20 | 9,7 | `0a30h` | `080a30h` | 300 | `257290fb` |
| 26 | 19x16/20 | 9,8 | `0b60h` | `080b60h` | 320 | `fc64f750` |
| 27 | 20x17/20 | 10,8 | `0ca0h` | `080ca0h` | 340 | `ac385920` |
| 28 | 21x17/24 | 10,8 | `0e00h` | `080e00h` | 408 | `08d1f421` |
| 29 | 22x18/24 | 11,9 | `0fa0h` | `080fa0h` | 432 | `f123c54a` |
| 30 | 23x19/24 | 11,9 | `1150h` | `081150h` | 456 | `b88de405` |

## Exhaustive phase and gap evidence

Every count appears for all 64 global phases. Offsets and circular gaps in
instance-ID order are:

| n | Offsets | Circular gaps |
|---:|---|---|
| 1 | 0 | 64 |
| 2 | 0,32 | 32,32 |
| 3 | 0,21,42 | 21,21,22 |
| 4 | 0,16,32,48 | 16,16,16,16 |
| 5 | 0,12,25,38,51 | 12,13,13,13,13 |
| 6 | 0,10,21,32,42,53 | 10,11,11,10,11,11 |
| 7 | 0,9,18,27,36,45,54 | 9,9,9,9,9,9,10 |
| 8 | 0,8,16,24,32,40,48,56 | 8,8,8,8,8,8,8,8 |
| 9 | 0,7,14,21,28,35,42,49,56 | 7,7,7,7,7,7,7,7,8 |
| 10 | 0,6,12,19,25,32,38,44,51,57 | 6,6,7,6,7,6,6,7,6,7 |
| 11 | 0,5,11,17,23,29,34,40,46,52,58 | 5,6,6,6,6,5,6,6,6,6,6 |
| 12 | 0,5,10,16,21,26,32,37,42,48,53,58 | 5,5,6,5,5,6,5,5,6,5,5,6 |
| 13 | 0,4,9,14,19,24,29,34,39,44,49,54,59 | 4,5,5,5,5,5,5,5,5,5,5,5,5 |
| 14 | 0,4,9,13,18,22,27,32,36,41,45,50,54,59 | 4,5,4,5,4,5,5,4,5,4,5,4,5,5 |
| 15 | 0,4,8,12,17,21,25,29,34,38,42,46,51,55,59 | 4,4,4,5,4,4,4,5,4,4,4,5,4,4,5 |
| 16 | 0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60 | sixteen gaps of 4 |

Every list has unique phases, every gap is `floor(64/n)` or `ceil(64/n)`, and
every gap list sums to 64. Advancing `g` advances each fixed instance phase by
exactly one modulo 64, and each instance returns to its starting phase after
64 steps.

Representative global-phase-zero draw orders are:

```text
n=1:  0
n=2:  1,0
n=4:  3,2,0,1
n=8:  6,5,7,4,0,1,3,2
n=16: 12,11,13,10,14,9,15,8,0,1,7,2,6,3,5,4
```

The first recorded equal-depth case is `n=3`, `g=27`, depth rank `+9`, with
instance IDs `0,2` in that order. Smaller ID is drawn first and larger ID
later within the equal-depth group. Every one of the 1,024 order arrays is a
complete permutation with nondecreasing signed depth and strictly increasing
IDs inside a tie.

For `n=1`, offset is zero, phase equals global phase, and draw order is `[0]`.
All 64 records match the accepted M98t phase, depth, scale, descriptor,
anchor, rectangle, BMS source, payload length, and frame identity.

## Canonical serialization and counters

The generated-only golden is canonical UTF-8 JSON with fixed field order,
decimal integers, LF endings, count order 1..16, phase order 0..63, records in
instance-ID order, and draw-order indices in far-to-near order. It contains no
timestamp, hostname, absolute path, pointer, private value, or pixel payload.

Two complete generations and two independent validations produced:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| canonical exhaustive golden | 7,288,291 | `6ed8e4e4b70ed62547d0feca9847999f730e25b6e4d19e0a16c670021c5a3e52` |
| generated QA summary | 773 | `49ee85ba720ea6b1b139c00810ad66591e98e079fddc36fac4cf94c8a6f945ae` |
| compact contract include | 3,011 | `f75c7eb8b22efa6f6eeef6ba512d92c6c6e22b4b223669643c3b82588096a7a2` |

Passing counter summary:

```text
max_instances=16
counts_tested=16
global_phases_tested=64
count_phase_combinations=1024
instance_records_generated=8704
draw_orders_generated=1024
unique_phase_failures=0
gap_balance_failures=0
descriptor_failures=0
bounds_failures=0
hud_overlap_failures=0
source_range_failures=0
permutation_failures=0
depth_order_failures=0
tie_break_failures=0
count_one_mismatches=0
determinism_mismatches=0
private_data_findings=0
```

## Negative and fail-closed evidence

Twenty-eight M98u test methods cover the exhaustive positive model and more
than the required 36 controlled negative classifications. Each mutation starts
from accepted M98t inputs or a passing canonical reference and asserts its
specific stable `M98U_*` code.

| Area | Stable failures exercised |
|---|---|
| Count, phase, ID, arithmetic | `M98U_ACTIVE_COUNT_RANGE`, `M98U_GLOBAL_PHASE_RANGE`, `M98U_INSTANCE_ID_RANGE`, `M98U_U16_MULTIPLY_INPUT`, `M98U_U16_MULTIPLY_OVERFLOW` |
| Formula, uniqueness, gaps | `M98U_OFFSET_FORMULA` for nearest/incremental variants, `M98U_PHASE_UNIQUE`, `M98U_GAP_SUM`, `M98U_GAP_BALANCE` |
| Accepted phase/descriptor inputs | `M98U_PHASE_TABLE`, `M98U_DEPTH_SCALE_MISMATCH`, `M98U_SCALE_RANGE`, `M98U_DESCRIPTOR_PAYLOAD`, `M98U_DESCRIPTOR_GEOMETRY`, `M98U_DESCRIPTOR_ANCHOR`, `M98U_DESCRIPTOR_IDENTITY` |
| Shared BMS source | `M98U_ATLAS_BANK_CONTRACT`, `M98U_SOURCE_RANGE`; record schema rejects copied `payload` or `pixels` fields |
| Screen, HUD, and G1 | `M98U_DESTINATION_BOUNDS`, `M98U_HUD_INTERSECTION`, `M98U_G1_PAGE_BOUNDS`, `M98U_G1_ADDRESS_OVERFLOW` |
| Fixed capacity and ordering | `M98U_SORT_CAPACITY`, `M98U_RECORD_IDS`, `M98U_DRAW_ORDER_PERMUTATION`, `M98U_DRAW_ORDER_KEY`, `M98U_DRAW_ORDER_TIE` |
| Canonical evidence | `M98U_RECORD_COUNT`, `M98U_RECORD_FIELDS`, `M98U_COUNT_ONE_MISMATCH`, `M98U_GAP_VALUES`, `M98U_SUMMARY`, `M98U_GOLDEN_CANONICAL`, `M98U_SERIALIZATION_PRIVATE`, `M98U_SERIALIZATION_NUMERIC` |
| Compact guest contract | `M98U_CONTRACT_LAYOUT`, `M98U_CONTRACT_DUPLICATE`, `M98U_CONTRACT_EXPANDED`; no 1,024-state expansion is permitted |

Code and binary scope checks additionally prove that no pointer/address/input
stability tie-break, platform sort, guest multi-instance list, `/N`, UP/DOWN
action, HUD count update, extra BITBLT, dirty-union code, private material, or
unrelated dirty path entered the release guest or tracked output. The two
byte-identical generation passes cover nondeterministic ordering and evidence.
Invalid modeled input produces no partial valid reference list.

## M98t baseline and non-change proof

Before editing, all accepted 159 demo host tests passed. After M98u, the same
159 tests plus 28 M98u tests passed, for 187 total. VAEG selftest passed with
the accepted executable:

```text
build/macos-macports/sdl2/vaeg
bytes=8155976
sha256=13109ea163c6c708e0e79df0b149b1ff317c04381f9ad119e90ac35bc8e73d46
```

Two clean final release builds and both pre-edit baseline builds were
byte-identical:

```text
ZUNDORB.COM bytes=32656
sha256=b6e1bbc2a600f22ca583e256c82cccab3c1523530a0a2a7836439d4cb74d87ec
```

This is the exact accepted M98t identity. The new compact include is
reference-only and is not included by `build.sh` or guest assembly. A Git
scope comparison confirms no guest assembly, build script, atlas/depth/HUD
input, emulator, BMS, SGP, page-state, scheduler, G0/G1, or cleanup source
changed.

The accepted generated M98t oracle corpus was revalidated rather than rerun
because the evaluated guest, VAEG executable, atlas, depth/HUD inputs, and
oracle implementation identities are unchanged. This follows the repository's
expensive-test reuse policy. All 17 selected accepted cases remained `PASS`
with empty error lists:

- A/full, A/dirty, B/full, and B/dirty: 128 publications each;
- 256 corresponding full/dirty publication records matched exactly;
- static V1 through V8: 64 publications and one revolution each;
- opposite-page V1 through the B cases, plus long V4 and V8: 128
  publications and two revolutions;
- dynamic HUD ladder and pause: 128 publications each; and
- missed-slot scenario: 128 publications, two misses, no skipped state.

Thus the accepted framebuffer, cadence, HUD, pause, missed-slot, BMS/SGP,
page-state, and cleanup evidence remains applicable without a new D88 or
visual gate.

## Commands and repository checks

Principal commands and results:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98u-baseline-pyc \
  python3 -m unittest discover -s demos/zundamon-orbit/tools -p 'test_*.py'
# 159 tests, PASS before M98u

python3 demos/zundamon-orbit/tools/build_zundamon_orbit_pipeline.py \
  --fixture-output build/generated/zundamon-orbit/m98u-reference-c/public-atlas
python3 demos/zundamon-orbit/tools/generate_zundamon_multi_instance_state.py \
  --atlas build/generated/zundamon-orbit/m98u-reference-c/public-atlas/zundorb.bin \
  --depth-table demos/zundamon-orbit/256/zundamon_depth_table.inc \
  --golden-output build/generated/zundamon-orbit/m98u-reference-c/m98u-golden.json \
  --summary-output build/generated/zundamon-orbit/m98u-reference-c/m98u-summary.json \
  --contract-output build/generated/zundamon-orbit/m98u-reference-c/zundamon_multi_instance_contract.inc
python3 demos/zundamon-orbit/tools/validate_zundamon_multi_instance_state.py \
  --golden build/generated/zundamon-orbit/m98u-reference-c/m98u-golden.json \
  --atlas build/generated/zundamon-orbit/m98u-reference-c/public-atlas/zundorb.bin \
  --depth-table demos/zundamon-orbit/256/zundamon_depth_table.inc \
  --contract build/generated/zundamon-orbit/m98u-reference-c/zundamon_multi_instance_contract.inc
# repeated independently as m98u-reference-d; byte-for-byte identical

PYTHONPYCACHEPREFIX=/tmp/vaeg-m98u-final-pyc \
  python3 -m unittest discover -s demos/zundamon-orbit/tools -p 'test_*.py'
# 187 tests, PASS

demos/zundamon-orbit/256/build.sh \
  build/generated/zundamon-orbit/m98u-final-build-a/ZUNDORB.COM \
  build/generated/zundamon-orbit/m98u-final-build-a/ZUNDORB.LST
# repeated as final-build-b; both exact M98t identity

build/macos-macports/sdl2/vaeg --selftest
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
sh -n demos/zundamon-orbit/256/build.sh \
  demos/zundamon-orbit/build-local-d88.sh demos/zundamon-orbit/run-vaeg.sh
python3 -m py_compile demos/zundamon-orbit/tools/*multi_instance*.py
git diff --check
```

VAEG selftest, encoding, EOL, case, shell syntax, Python compilation,
repository scope, privacy, prohibited-artifact, and whitespace checks passed.
No hosted CI run was required for this host/reference-only change.

## Generated and private artifact exclusion

The complete 7.3 MB golden, QA summaries, generated public atlas, release COM
and listings, candidate D88, captures, traces, save states, and backup RAM all
remain ignored or pre-existing and untracked. Only the compact public contract
include is tracked. Staged-path checks found no COM, BIN, D88, ROM, image,
trace, capture, or save-state artifact. The tracked M98u scope contains no
absolute host path, timestamp, hostname, pointer, private filename, private
identity, private hash, ROM identity, or ROM-derived byte.

## Limitations

M98u is host/reference state preparation only. The release guest count remains
one and does not render multiple instances. It has no count control, HUD count
change, multi-instance full clear, rectangle list, dirty-row interval union,
private IDA, multi-instance timing result, or physical-hardware evidence.
M98v, M98w, and M98x retain those later public scopes. No later milestone was
started.

Physical PC-88VA/VA2 testing remains `REAL_HW_PENDING`; this is not a G98u
failure. G98u is machine-verifiable and requires no new human visual gate.

`REAL_HW_PENDING`

`G98u PASS`
