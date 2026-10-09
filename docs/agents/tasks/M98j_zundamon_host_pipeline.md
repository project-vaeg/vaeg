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

# M98j - Run the complete local host-asset pipeline

Status: **G98j human/local gate passed on 2026-08-31 at candidate `95d431b717124b69c3e490d1c53fce485e9e15d5`; M98j closed**

Branch: `topic/m98j-zundamon-host-pipeline`

Commit prefix: `M98j:`

Gate type: **machine-verifiable public pipeline plus maintainer-only local visual gate**

## Goal

Connect the frozen M98c through M98i host stages into one fail-closed command
that turns an explicitly supplied generic local manifest into the final
single-bank version-1 atlas, a deterministic 30-level contact sheet, and a
private combined report. Apply the approved 98x128 downscale-only source
normalization without shrinking a 98x128 maximum frame. Run the public
synthetic fixture through exactly the same pipeline before preparing any local
human-gate candidate.

## Required changes

- Add one standard-library pipeline entry point that performs manifest and
  source validation, exact indexed-pixel recovery, VA8 conversion,
  downscale-only source normalization, 30-level scaling, single-bank BMS
  packing, M98h format inspection, and M98i packing inspection without
  intermediate private files.
- Fit sources larger than 98x128 within that bounding box while preserving the
  aspect ratio. Use deterministic center-sampled nearest-neighbor selection,
  project the anchor with the same pixel-center rule, leave inputs that fit
  byte-for-byte unchanged, preserve an exact 98x128 maximum frame, and never
  upscale.
- Use the M98g 30-level schedule with numerator 1 through 29 followed by 31,
  over denominator 31. Require the complete atlas to fit one BMS bank without
  further source shrinking.
- Accept either an explicit local manifest and a new output directory or a
  public-fixture output directory. The public mode must create a temporary
  M98d fixture and invoke the same production pipeline function.
- Write only `zundorb.bin`, `contact-sheet.bmp`, and
  `pipeline-report.json` into the new output directory. Refuse overwrite.
- Render all 30 levels in fixed order on a bounded contact sheet, with a
  checkerboard transparency background, anchor cross, level, dimensions, and
  anchor coordinates. Preview resampling must not alter atlas pixels.
- Combine the conversion, scale-set, packing, contact-sheet, and final
  validation summaries into a deterministic private report without recording
  input paths, filenames, provenance, source hashes, or atlas hashes.
- Keep CLI success and failure output free of paths, filenames, dimensions,
  anchors, bank assignments, sizes, CRCs, pixel values, and hashes.
- Add public end-to-end tests for deterministic outputs, input immutability,
  exact atlas agreement with independently composed M98f-M98i calls, contact
  sheet structure and marker placement, report reconciliation, overwrite
  refusal, isolated failures, and CLI privacy.

## Local human gate

If an already approved local bundle is available, run the same command into
the ignored `build/generated/zundamon-orbit/` tree and report only
`LOCAL_HOST_PIPELINE_READY`. The maintainer must inspect contact-sheet levels
1, 8, 15, 23, 29, and 30, including anchors and transparency, before stating
that G98j passed. Codex must not infer this approval from successful file
generation or automated inspection.

If no approved bundle is available, report `LOCAL_HOST_PIPELINE_PENDING` and
stop without fabricating or acquiring input.

If an approved bundle reaches a frozen fail-closed stage error, record only a
neutral stable status and retain G98j as pending. Do not split, crop, or alter
the accepted local input. The fixed 98x128 downscale-only normalization is the
only M98j pixel transformation before the scaler and packer.

## Private boundary

Local inputs and every generated atlas, contact sheet, combined report,
dimension, anchor, palette value, metric, hash, and identity remain untracked.
Tracked documentation records only neutral status tokens. The public fixture
contains only the M98b abstract marker.

## Out of scope

- Changing the G98e-approved input crop, transparency, or anchor.
- Any normalization other than the fixed downscale-only 98x128 rule.
- Any M98h format or M98i packing change beyond the authorized 30-descriptor,
  single-bank revision.
- Guest assembly, BMS probing/loading, SGP transfer, VAEG, disk images,
  screenshots, animation, or physical-machine work.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98j-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_pipeline.py

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

`HOST_FIXTURE_PASS` is mandatory: the public fixture reproduces byte for byte,
leaves its input unchanged, matches an independently composed M98f-M98i
atlas, and passes both final inspectors. Contact-sheet pixels and report
metrics reconcile, negative cases reach exact codes, diagnostics are
path-redacted, and repository/privacy checks pass.

When an approved local bundle exists, successful generation establishes only
`LOCAL_HOST_PIPELINE_READY`. G98j passes only after the maintainer inspects the
six required levels and explicitly says so. Stop at G98j; M98k remains
unassigned.

The maintainer inspected the required levels and explicitly stated that G98j
passed on 2026-08-31. M98j is closed; M98k remains unassigned.
