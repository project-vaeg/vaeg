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

# M98g - Generate exactly 30 deterministic scale levels

Status: **G98g machine gate passed on 2026-08-31; 30-level contract revalidated by M98j; `LOCAL_SCALE_SET_PASS`**

Branch: `topic/m98g-zundamon-scale-levels`

Commit prefix: `M98g:`

Gate type: **machine-verifiable public fixture; optional local integration status**

## Goal

Generate exactly 30 deterministic center-sampled nearest-neighbor scale
levels from the M98f VA8 crop, preserve transparency and every sampled byte,
and project the approved anchor without introducing the final atlas or BMS
packing contract.

## Geometry contract

For levels `i=1..30`:

```text
numerator(i) = i for i=1..29, and 31 for i=30
width(i)     = max(1, (source_width  * numerator(i) + 15) // 31)
height(i)    = max(1, (source_height * numerator(i) + 15) // 31)
pitch(i)     = (width(i) + 3) & ~3
```

For target coordinate `t`, source size `s`, and target size `d`, sample:

```text
source(t) = min(s - 1, ((2 * t + 1) * s) // (2 * d))
```

Project a source anchor coordinate `a` with:

```text
anchor(a) = min(d - 1, ((2 * a + 1) * d) // (2 * s))
```

Level 30 must reproduce every source pixel exactly. The `30/31` scale slot is
omitted so a 98x128 full-size source and all 30 frames fit one later BMS bank.
Retain all 30 descriptors when adjacent small levels have duplicate
dimensions.

## Required changes

- Add a standard-library scale generator that invokes the validated M98f
  conversion path directly from the generic local bundle.
- Store rows top to bottom with zero bytes from width through four-byte pitch.
- Concatenate frames in increasing level order, inserting only zero bytes so
  every frame begins at a 16-byte-aligned stream offset.
- Emit an intermediate `scales.va8` stream and deterministic private
  `report.json` only in a new explicitly supplied output directory.
- Record exactly 30 descriptors with level, dimensions, pitch, projected
  anchor, stream offset, and payload length.
- Keep CLI success and failure output free of paths, filenames, source or
  target geometry, anchors, pixels, byte counts, descriptors, and hashes.
- Add independent-oracle tests covering all pixels of all 30 public-fixture
  frames, including duplicate dimensions, row padding, frame alignment,
  level-30 identity, anchor bounds, and exact stable failure codes.

## Private boundary

The local input, VA8 pixels, scale stream, report, dimensions, anchors,
descriptors, sizes, and identities remain untracked. Tracked text may record
only the neutral `LOCAL_SCALE_SET_PASS` status.

## Out of scope

- Freezing an atlas header or descriptor binary format.
- CRCs, BMS bank assignment, bank-boundary padding, guest code, VAEG, or
  physical-machine work.
- Runtime interpolation, dithering, rotation, or changes to the G98e-approved
  source and anchor.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98g-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_scales.py

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

The public fixture produces exactly 30 ordered descriptors, monotonic
dimensions, preserved duplicate levels, an exact full-size level, and payloads
matching an independent center-sampling oracle. Every row and frame-alignment
padding byte is zero, anchors remain in bounds, focused failures reach exact
codes, outputs reproduce byte for byte, inputs remain unchanged, and CLI
diagnostics remain path-redacted. Repository and privacy checks pass. A
separately approved local bundle may report only `LOCAL_SCALE_SET_PASS`. This
is a machine gate. Stop at G98g; M98h remains unassigned.
