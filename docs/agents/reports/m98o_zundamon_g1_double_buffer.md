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

# M98o transparent G1 double-buffer result

Status: **G98o human gate passed on 2026-08-31; M98o closed**

## Result

`M98o RESULT: PASS`

`G98o FINAL STATUS: PASS`

The public synthetic fixture completed four bounded hidden-page render and
publication batches in VA2 mode. The exact indexed-GVRAM oracle, alternating
SGP descriptor trace, page-lifecycle checkpoints, post-cleanup counters, two
settled frames, deterministic rebuild, focused fault model, and repository
checks all passed. On 2026-08-31, the maintainer reported that the marker
appeared slightly below and to the right of center, confirmed that ESC
restored the guest environment, and explicitly stated `G98o passed`. M98o is
closed. This remains VAEG evidence and is not physical PC-88VA evidence.

## Git and predecessor

- Branch: `topic/m98o-g1-double-buffer`
- Starting commit: `50201c9c22809246525e04de825399079b6c84f5`
- Accepted M98l candidate:
  `228f31eb192c2722862691067c46c4db9e4aeb95`
- Task-definition commit:
  `80b80ab2ef3d3d90174bce17fdc50a9bd2aa4762`
- Evaluated implementation candidate:
  `ddc70c692ecb65066269c9894eb4b14f702fd2d9`
- Push status: recorded in the final handoff because this report cannot contain
  its own commit SHA.

The predecessor report records `G98l-A PASS`, `G98l-B PASS`, and
`G98l-C PASS`. M98m and M98n remain reserved identifiers absorbed by M98l.
M98p and later retain their numbers and were not started.

## Preserved dirty-worktree baseline

The following unrelated entries existed before M98o and remain untouched:

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

Ignored M98o runtime output remains below
`build/generated/zundamon-orbit/` and does not appear in Git status.

## Changed files

| File | Purpose |
|---|---|
| `docs/agents/ROADMAP.md` | Assign M98o without renumbering later work. |
| `docs/agents/tasks/M98_zundamon_orbit_master_plan.md` | Record the active M98l/M98m/M98n/M98o sequence. |
| `docs/agents/tasks/M98o_zundamon_g1_double_buffer.md` | Freeze the gate and validation contract. |
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Add explicit A/B state, bounded render/flip batches, counters, and cleanup. |
| `demos/zundamon-orbit/256/build.sh` | Identify M98o guest builds. |
| `demos/zundamon-orbit/build-local-d88.sh` | Identify local M98o media and retain no-overwrite behavior. |
| `demos/zundamon-orbit/run-vaeg.sh` | Run the M98o VA2 capture and oracle. |
| `demos/zundamon-orbit/zundamon_orbit_m98o.debug` | Capture four flips, two settled frames, and three cleanup reports. |
| `demos/zundamon-orbit/tools/verify_zundamon_orbit_guest.py` | Independently verify page identities, trace order, events, and counters. |
| `demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py` | Test the oracle and ten fail-closed lifecycle faults. |
| `demos/zundamon-orbit/README.md` | Document the public M98o build and run workflow. |
| `docs/agents/reports/m98o_zundamon_g1_double_buffer.md` | Record this evidence and pending gate. |

No emulator source changed, so no bug-fix ledger entry or emulator-specific
regression fix was required.

## Confirmed display and page contract

The live guest, `io/videova.c`, `io/tsp.c`, `io/sgp.c`, and the established
256-color pseudo-sprite reference agree on this contract:

| Item | Value |
|---|---:|
| Logical display | 320x200 |
| Direct-color format | VA 8-bpp `GGGRRRBB` |
| G1 backing surface | 320x400, pitch 320 bytes |
| Page bytes / words | 64,000 (`fa00h`) / 32,000 (`7d00h`) |
| Page A SGP base | `220000h` |
| Page B SGP base | `22fa00h` |
| Page A DSA1 | `020000h` |
| Page B DSA1 | `02fa00h` |
| DSA1 low/high ports | `022eh` / `0230h` |
| DSA1 programming | two 16-bit writes, byte-address units |
| Transparent operation | BITBLT `0105h`; source zero is transparent |

NASM assertions prove that each page equals `320 * 200`, that the CLS word
count equals half the byte count, that both SGP and DSA1 bases differ by one
page, that the 320x400 backing is exactly two pages, and that P0/P1 are in
bounds and non-overlapping. Runtime validation checks both descriptor tables,
both DSA values, both positions, and the exact public 23x19 selected cell.

## BMS and SGP ordering

M98o retains M98l's default BMS port `01d0h`, selector-zero ordinary mapping,
selectors 1 through 128, 128-KiB banks, and CPU/SGP aperture
`80000h-9ffffh`. The selected public cell remains level 30: 23x19, pitch 24,
bank offset `1150h`, 456 payload bytes, and SGP source `081150h`.

Both pages are initialized by one hidden setup list containing SET_WORK,
zero-color, CLS page A, CLS page B, and END. Each of the four measured batches
uses one list in this exact order:

```text
SET_WORK
SET_COLOR zero
CLS complete hidden page (7d00h words)
SET_SOURCE 081150h, 23x19, pitch 24, 8-bpp
SET_DEST hidden page plus P0 or P1, 23x19, pitch 320, 8-bpp
BITBLT 0105h
END
```

The guest waits for SGP idle before selecting bank 1, does not change the bank,
command storage, page descriptor, or role state while BUSY, and marks the page
complete only after the bounded completion wait. A completion timeout issues
the generic SGP abort request before common cleanup, so selector zero is not
restored while an SGP read remains active. The successful path validates the
poisoned 4,096-byte staging buffer and the BMS frame CRC after every batch,
then changes from selector 1 to selector zero. Eight observed selector changes
therefore represent exactly two value changes per batch.

The generic SGP trace contains four and only four matching source rows. Its
destination sequence is:

```text
232c30h  page B + (48,40)
22aff8h  page A + (248,140)
232c30h  page B + (48,40)
22aff8h  page A + (248,140)
```

No CPU or host copy writes the cell into G1.

## VBLANK and page lifecycle

Port `0142h`, bit `40h`, is the live VBLANK source. Every publication first
waits for the bit to become low and then waits for it to become high. Each half
has four outer passes of at most 65,535 polls. DSA1 changes only after the SGP
page is complete and the fresh edge succeeds. Software roles change only
after the DSA1 write.

The page states are explicit values for UNINITIALIZED, HIDDEN_CLEAN,
HIDDEN_RENDERING, HIDDEN_COMPLETE, VISIBLE, and HIDDEN_STALE. The four
publication checkpoints report:

| Flip | Frame | Visible/hidden | A/B states | Position | DSA1 low |
|---:|---:|---|---|---|---:|
| 1 | 2315 | B/A | stale/visible | P0 | `fa00h` |
| 2 | 2318 | A/B | visible/stale | P1 | `0000h` |
| 3 | 2321 | B/A | stale/visible | P0 | `fa00h` |
| 4 | 2324 | A/B | visible/stale | P1 | `0000h` |

Frames 2325 and 2326 are consecutive settled captures. Their complete indexed
GVRAM and composed BMP files are byte-identical. The final page remains on
screen for interactive inspection until ESC; the deterministic harness sends
Return after its second settled capture to exercise the same common cleanup.

## Independent page oracle

The oracle reconstructs the expected pages directly from the validated public
atlas, source-zero transparency, G0 checkerboard, pitch, and P0/P1. It does not
use a screenshot as its pixel authority.

| Page | Declared rectangle | Nonzero bounding box | Nonzero pixels | SHA-256 |
|---|---|---|---:|---|
| A / P1 | `(248,140)` through `(270,158)` | `(249,141)` through `(269,157)` | 73 | `c62d1e43d3d861171b294518302aa25e4b53d4ced6979644d223101eee88c3f5` |
| B / P0 | `(48,40)` through `(70,58)` | `(49,41)` through `(69,57)` | 73 | `77ea6d5db8ba361d9b1b7f71464081aac0f709d271402fd80fb32bbb2e16cd51` |

After flip 1, page A is still entirely zero. On every later capture, page A
contains only P1 and page B contains only P0. Cross-capture comparisons prove
that the currently visible page was not changed while the other page was
prepared. Exact zero source pixels remain zero on G1 while the independently
verified nonzero G0 checkerboard supplies the composed background. This also
rejects stale data, an opaque zero copy, a visible-page clear, a wrong pitch,
or a destination on the wrong parity.

## Counter evidence

The three post-video-restoration checkpoints report:

| Counter | Value |
|---|---:|
| pages initialized | 2 |
| render batches started/completed | 4 / 4 |
| measured full-page clears / transparent BITBLTs | 4 / 4 |
| VBLANK publication/settled edges | 7 |
| measured page flips | 4 |
| page A / page B publications | 3 / 2 |
| SGP timeouts / errors | 0 / 0 |
| VBLANK timeouts | 0 |
| BMS selector value changes | 8 |
| cleanup runs | 1 |

The initial page-A publication is included in the A count and VBLANK edge
count but not in the four measured flips or clear/BITBLT counts. The two
settled-frame edges are included. Later idle-display edges are excluded.

## Negative lifecycle evidence

The test-only state model starts from valid A-visible/B-hidden state, changes
one condition, asserts one stable code, and verifies the prior DSA1 value,
selector-zero restoration, one cleanup run, restored video, and no partial
publication. The release guest contains no fault-injection switch.

| Case | Stable result |
|---|---|
| SGP timeout during clear | `M98O_FAULT_SGP_CLEAR_TIMEOUT` |
| SGP error during BITBLT | `M98O_FAULT_SGP_BITBLT_ERROR` |
| VBLANK-low timeout | `M98O_FAULT_VBLANK_LOW_TIMEOUT` |
| VBLANK-high timeout | `M98O_FAULT_VBLANK_HIGH_TIMEOUT` |
| publication before SGP completion | `M98O_FAULT_EARLY_PUBLICATION` |
| render into visible page | `M98O_FAULT_VISIBLE_RENDER` |
| BMS switch while SGP busy | `M98O_FAULT_BMS_SWITCH_BUSY` |
| invalid/overlapping descriptors | `M98O_FAULT_PAGE_DESCRIPTOR` |
| out-of-bounds destination | `M98O_FAULT_DESTINATION_BOUNDS` |
| atlas rejection before video mode | `M98O_FAULT_ATLAS_BEFORE_VIDEO` |

The production oracle also has focused one-mutation failures for register
state, exact G1 bytes, BMS source order, hidden destination order, settled
stability, black output, and frame-limit termination.

## Commands and results

The selected existing build tree and executable were discovered rather than
assumed:

```text
build/macos-macports/CMakeCache.txt
build/macos-macports/sdl2/vaeg
```

The final checks were:

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98o-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py

for test_file in demos/zundamon-orbit/tools/test_*.py; do
  PYTHONPYCACHEPREFIX=/tmp/vaeg-m98o-all-pyc python3 "$test_file"
done

NASM=/opt/local/bin/nasm sh demos/zundamon-orbit/256/build.sh \
  <fresh-a>/ZUNDORB.COM <fresh-a>/ZUNDORB.LST
NASM=/opt/local/bin/nasm sh demos/zundamon-orbit/256/build.sh \
  <fresh-b>/ZUNDORB.COM <fresh-b>/ZUNDORB.LST
cmp <fresh-a>/ZUNDORB.COM <fresh-b>/ZUNDORB.COM
cmp <fresh-a>/ZUNDORB.LST <fresh-b>/ZUNDORB.LST

VAEG_ZUNDAMON_MODEL=va2 NASM=/opt/local/bin/nasm \
  sh demos/zundamon-orbit/run-vaeg.sh \
  <local-bootable-2hd-template> \
  build/macos-macports/sdl2/vaeg \
  <local-rom-directory> \
  build/generated/zundamon-orbit/m98o-va2-candidate

build/macos-macports/sdl2/vaeg --selftest
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

All 71 Zundamon tests passed. The focused M98o file reports ten unittest
methods, including all ten required lifecycle-fault subcases. The VAEG
selftest ended with `selftest: all tests passed`. Encoding, EOL, case, and
whitespace checks passed. Reusing an existing D88 output path returned status
1 with the expected no-overwrite refusal.

## Reproducibility artifacts

All files in this table are generated, ignored, and untracked:

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `ZUNDORB.COM` | 17,744 | `2c3698b30002f82917b2f82d6262c7b16a7309612e71cbc64c5fab1ff23a4194` |
| `ZUNDORB.LST` | 133,803 | `582c29a252f3fc97df776e2b597b964a2ea45925ee9a20c67f1fe6d0f1808a87` |
| public `ZUNDORB.BIN` | 5,912 | `7d635be2c77680ad8d452d1cf23ee5401a61042ff317870a73f5587e5bd3b9aa` |
| local bootable validation D88 | 1,338,960 | `50fbb22f27bedc446d22ddc4bd9fcc07c0c7de37bb2d21bfdd4758e38afd23a8` |
| event log | 796 | `446eb48d769f202224d7c98b647ab03f1bf3a9c99781ed0c6de583c116d16e6d` |
| SGP trace | 1,077 | `72421ce7b7fe57f4c5d6e54a8274de3733e246dd32bf1f1a93726447fd7af868` |
| oracle report | 7,231 | `c77e1829a1e7c2c29f3256ca836f16f4a5a2597895c41948e85f2c71a951d271` |
| each settled indexed GVRAM | 262,144 | `971ada2882d08f6afc74c3a3b22e1dee5f710f184d772a2498a882bcc3494a59` |
| each settled composed BMP | 1,080,442 | `fcf762eb32676019c8a847084ddd86037019081702bf27c602c1991d919a97b3` |

Two fresh guest builds produced byte-identical COM and listing files with the
hashes above. The local disk builder listed only the PC-Engine system files and
the source-built public `ZUNDORB.COM`/`ZUNDORB.BIN` payloads. `git ls-files`
found no generated COM, listing, atlas, capture, trace, or D88 artifact.

## Limitations and gate

SDL dummy video was used only as deterministic transport for the automated
run. The exact indexed GVRAM, descriptor trace, and nonblack composed BMP
checks established the VA2/VAEG candidate independently of the later human
inspection. The maintainer then observed the displayed marker, confirmed ESC
restoration, and explicitly passed G98o on 2026-08-31.

`REAL_HW_PENDING`

Scale traversal, dirty-row clearing, cadence controls, orbit motion, private
image integration, multiple objects, performance measurement, and all M98p+
work remain deferred.

The final `git status --short` is identical to the pre-existing dirty baseline
listed above; all M98o tracked files are committed. Generated M98o output is
ignored, and no unrelated path was staged.

Final status: **G98o human gate passed; M98o closed**.
