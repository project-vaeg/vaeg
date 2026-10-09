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

# M98b - Add the public Zundamon orbit fixture

Status: **G98b machine gate passed on 2026-08-31**

Branch: `topic/m98-zundamon-orbit`

Commit prefix: `M98b:`

Gate type: **machine-verifiable**

## Goal

Add the isolated public directory and a deterministic synthetic indexed-image
fixture that exercises later host tools without consuming any
maintainer-supplied input.

## Required changes

- Create `demos/zundamon-orbit/` with lowercase tracked paths.
- Generate a 23x19 asymmetric abstract marker with transparency, crossed
  diagonals, isolated pixels, transparent holes, and opaque near-black.
- Generate a source-neutral 16-entry RGB888 palette. Index 0 is transparent
  and index 15 is reserved.
- Emit a canonical JSON manifest with dimensions, features, filenames, sizes,
  and public-fixture SHA-256 values.
- Refuse to overwrite an existing output directory.
- Add an independent inspector and focused reproducibility, corruption, and
  overwrite-refusal tests.
- Write generated output only below `build/generated/zundamon-orbit/` or an
  explicit new directory.

## Out of scope

- Maintainer-supplied images, palettes, manifests, or identifying metadata.
- Crop, anchor, color conversion, scaling, atlas, BMS, SGP, guest, and disk
  implementation.
- VAEG or physical-machine claims.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98b-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_asset.py

output_root=$(mktemp -d /tmp/vaeg-m98b.XXXXXX)
demos/zundamon-orbit/build.sh "$output_root/fixture"

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

The three focused tests pass, two fresh builds are byte-identical, deliberate
pixel corruption reaches `M98B_FIXTURE_PIXELS_SHA`, an existing output reaches
`M98B_FIXTURE_OUTPUT_EXISTS`, the standalone builder/inspector pass, and the
repository checks pass. No human, emulator, or physical-machine gate is
implied. Stop at G98b.
