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

# M98o - Add transparent G1 double buffering

Status: **G98o human gate passed on 2026-08-31; M98o closed**

Branch: `topic/m98o-g1-double-buffer`

Starting commit: `50201c9c22809246525e04de825399079b6c84f5`

Accepted M98l candidate: `228f31eb192c2722862691067c46c4db9e4aeb95`

Evaluated M98o candidate: `ddc70c692ecb65066269c9894eb4b14f702fd2d9`

Result: [`../reports/m98o_zundamon_g1_double_buffer.md`](../reports/m98o_zundamon_g1_double_buffer.md)

Commit prefix: `M98o:`

Gate type: **automated VA2/VAEG evidence plus maintainer human gate**

## Goal

Extend M98l into a bounded renderer that clears and draws the accepted public
synthetic 8-bpp cell only on the hidden Graphic 1 page, waits for SGP
completion, and publishes the complete page on a fresh low-to-high VBLANK
edge. M98m and M98n remain absorbed reservations; M98p is not started.

## Accepted predecessor

M98l closed these prerequisites in one VA2 guest artifact:

- `G98l-A PASS`: the `01d0h` selector-zero/one-through-N mapping, capacity,
  invalid-selection behavior, guards, and ordinary mapping restoration;
- `G98l-B PASS`: bounded streaming of the public atlas into selector 1 through
  one 4,096-byte conventional staging buffer; and
- `G98l-C PASS`: an exact transparent SGP transfer directly from `081150h`
  into G1.

## Fixed contract

| Item | Value |
|---|---:|
| Logical mode | 320x200 VA direct-color 8-bpp |
| G1 backing geometry | 320x400, 320-byte pitch |
| Page size | 64,000 bytes (`fa00h`) |
| Page A SGP / DSA1 | `220000h` / `020000h` |
| Page B SGP / DSA1 | `22fa00h` / `02fa00h` |
| Source | public level-30 cell, 23x19, pitch 24, selector 1 |
| Transparent operation | SGP BITBLT `0105h`, source zero transparent |
| Publication | DSA1 at `022eh`/`0230h` after low-to-high VBLANK |
| Frames in flight | at most one SGP batch |

Compile-time and host checks must prove page size, non-overlap, bounds,
alignment, DSA relationships, and fixed P0/P1 bounds and non-overlap.

## Page lifecycle and invariants

Use explicit page descriptors and states:

```text
UNINITIALIZED -> HIDDEN_CLEAN -> HIDDEN_RENDERING -> HIDDEN_COMPLETE
              -> VISIBLE -> HIDDEN_STALE -> HIDDEN_CLEAN
```

Only the hidden page may be an SGP destination. DSA1 always identifies the
visible page. Completion follows a successful bounded SGP wait; publication
follows a fresh bounded VBLANK edge. Failure leaves the prior DSA1 and page
roles unchanged, and every exit restores selector zero and predecessor state.

## Deterministic positive sequence

Validate and load the public atlas, enter the accepted mode, fill nonzero G0,
clear both G1 pages, and publish initialized page A. Execute exactly four
measured render/flip batches:

```text
B:P0, A:P1, B:P0, A:P1
```

P0 and P1 are fixed, non-overlapping, and in bounds. Each batch confirms SGP
idle, selects bank 1 while idle, marks the hidden page rendering, performs a
full hidden-page clear and one transparent BMS-source BITBLT, holds bank and
descriptors stable until completion, restores selector zero, waits VBLANK low
then high, updates DSA1, and only then swaps page roles. Capture every
publication and two consecutive settled final frames. ESC requests normal
cleanup; Return provides the equivalent clean-exit request for the
deterministic debug harness.

## Counters and evidence

Expose `pages_initialized`, render starts/completions, full clears,
transparent BITBLTs, VBLANK edges, flips, A/B publications, SGP
timeouts/errors, VBLANK timeouts, BMS switches, and cleanup runs. The initial
page-A publication counts as a publication and VBLANK edge but not a flip.
Success requires starts = completions = flips = clears = BITBLTs = 4, both
page publication counts nonzero, timeout/error counters zero, and cleanup = 1.

The standard-library oracle independently derives both indexed page images
from the public atlas, zero transparency, G0 checkerboard, geometry, and P0/P1.
It checks event order, registers, exact GVRAM, stale-pixel absence, SGP traces,
counters, nonblack composition, and two-frame stability.

## Required negative model

Focused fail-closed tests start from a passing fixture, mutate one condition,
and require one stable code for:

1. SGP timeout during clear;
2. SGP timeout/error during BITBLT;
3. VBLANK-low timeout;
4. VBLANK-high timeout;
5. early publication;
6. rendering into the visible page;
7. BMS switching while SGP is busy;
8. invalid or overlapping page descriptors;
9. an out-of-bounds destination; and
10. atlas/source rejection before graphics mode.

The first four retain the prior visible page and all cases reach common
cleanup without a guest runtime bypass or unbounded wait.

## Non-goals

- Private assets, ROM-derived data, scale traversal, orbit motion, cadence
  controls, multiple instances, dirty clearing, performance, or gameplay.
- Emulator refactoring, SGP multiplier changes, or physical-hardware claims.

## Required checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98o-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py

NASM=/opt/local/bin/nasm sh demos/zundamon-orbit/256/build.sh \
  <output>/ZUNDORB.COM <output>/ZUNDORB.LST

VAEG_ZUNDAMON_MODEL=va2 sh demos/zundamon-orbit/run-vaeg.sh \
  <local-bootable-2hd-template> <vaeg> <local-rom-directory> <new-output>

<vaeg> --selftest
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

Build the guest twice from fresh outputs and compare hashes. Build D88 media
only through the explicit no-overwrite local-template path. All generated
binaries, media, captures, traces, and reports remain ignored.

## Acceptance and stop rule

Automated acceptance requires hidden-only drawing, completion-before-flip,
fresh VBLANK publication, exact A/B alternation and identities, transparency,
no partial/stale page, counter invariants, all negative cases, deterministic
rebuilds, repository checks, and no generated/private material in Git.

The automated VA2/VAEG candidate passed at
`ddc70c692ecb65066269c9894eb4b14f702fd2d9`. On 2026-08-31, the maintainer
reported that the marker appeared slightly below and to the right of center,
that ESC restored the guest environment, and explicitly stated `G98o passed`.
M98o is closed. Physical validation remains `REAL_HW_PENDING`; M98p was not
started.
