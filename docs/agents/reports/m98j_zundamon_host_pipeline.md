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

# M98j - Zundamon host-pipeline report

Evaluated predecessor: `3160756daeab2e6a51775a5aee595fc124f7ce02`

Accepted candidate: `95d431b717124b69c3e490d1c53fce485e9e15d5`

Status: **G98j human/local gate passed on 2026-08-31; M98j closed**

## Result

M98j adds one source-neutral host pipeline that validates the manifest and
input, recovers exact indices, converts VA8 pixels, generates all 30 scales,
packs the final atlas, and applies both the M98h format inspector and M98i
single-bank packing validator. Before scale generation, it deterministically
fits oversized sources within 98x128 while preserving aspect ratio. It keeps
an exact 98x128 maximum frame, projects the anchor with the same
center-sampled nearest-neighbor rule, never upscales, and writes no
intermediate pixel or scale stream. The 30-level scale schedule itself keeps
the complete atlas within one 128-KiB bank.

Successful output contains only the final atlas, a deterministic 30-level
contact sheet, and a combined private report. The contact sheet uses a fixed
bounded grid, checkerboard transparency, projected anchor crosses, and labels
for level, dimensions, and anchor. Its preview resampling never feeds back
into atlas data.

## Public machine evidence

The M98j standard-library suite ran eight test methods and passed:

```text
OK
M98J_TEST_PASS
```

Coverage includes byte reproducibility, input immutability, exact agreement
with independently composed M98f-M98i output, deterministic downscale-only
normalization and anchor projection, one-bank enforcement, both final
inspectors, contact-sheet geometry and marker pixels, report reconciliation,
the exact three-file output set, overwrite refusal, isolated failures, and
path-redacted public/local CLI behavior. The public maximum-bound oracle
confirms that a 98x128 source occupies 127456 of 131072 bank bytes.

M98b-M98i regressions also passed, for 61 passing test methods across M98b
through M98j. Python compilation, the public pipeline workflow, JSON,
repository encoding, EOL, case, diff, and M98 privacy checks passed. Generated
public and local data remains untracked.

## Local gate evidence

One existing approved local bundle passed the M98d input preflight. The
maintainer then approved 30 scale levels and exactly one atlas bank, retaining
98x128 as the exact maximum and using downscale-only normalization. The pipeline
generated exactly the final atlas, contact sheet, and private report. Both
final inspectors passed, the atlas requires one bank, and the contact sheet
contains all 30 ordered levels with projected anchors and transparency
previews.

The maintainer inspected the required levels and explicitly stated that G98j
passed on 2026-08-31. This converts `LOCAL_HOST_PIPELINE_READY` into accepted
human/local gate evidence.

## Boundary

G98j passed and M98j is closed. M98j makes no guest loading, BMS runtime, SGP,
VAEG, disk-image, screenshot, or physical-machine claim. M98k is unassigned.
