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

# M98d - Validate local image and palette content

Status: **G98d machine gate passed on 2026-08-31; `LOCAL_INPUT_PENDING`**

Branch: `topic/m98d-zundamon-input-validation`

Commit prefix: `M98d:`

Gate type: **machine-verifiable public fixture; optional local integration status**

## Goal

Read the files named by a valid M98c manifest, validate a strict source-neutral
32-bpp BMP and 16-entry RGB888 palette, and recover the declared crop as
palette indices 0 through 14 without rescaling, interpolation, or nearest-color
matching.

## Required changes

- Define `bmp32` version 1 as a 54-byte BMP using a 40-byte
  BITMAPINFOHEADER, 32 bits per pixel, BI_RGB, BGRA storage, no color table,
  and no extra payload. Accept both positive-height bottom-up and
  negative-height top-down row order.
- Bound BMP dimensions to 1-4096 and file size to the corresponding pixel
  array plus header.
- Require exactly 48 RGB888 palette bytes.
- Require palette entries 0 and 15 to equal the declared background, entries
  1-14 to be unique, and no visible entry to equal the background.
- Resolve only the already validated sibling basenames and reject symlinks,
  non-regular files, missing files, and oversized files.
- Check the declared crop against actual BMP dimensions.
- Map exact background RGB to index 0 and exact visible palette colors to
  indices 1-14. Reject every unexplained color, an all-transparent crop, and a
  crop without transparency.
- Return recovered indices in top-to-bottom, left-to-right order without
  writing them to a tracked location.
- Keep CLI success and error output free of manifest paths, input basenames,
  dimensions, palette values, counts, and hashes.

## Public fixture

Add a standard-library generator that converts the M98b abstract indexed
fixture into a complete M98c bundle. It must generate deterministic bottom-up
and top-down BMP variants without Pillow, network access, or maintainer input.

## Out of scope

- Nearest-color recovery or tolerated color error.
- Alpha-channel semantics, image rescaling, color conversion, crop/anchor
  approval, previews, atlas work, guest code, VAEG, or physical-machine work.
- Recording any local manifest path, filename, digest, dimensions, palette,
  or validation result in tracked text.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98d-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_input.py

bundle_root=$(mktemp -d /tmp/vaeg-m98d.XXXXXX)
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_input_fixture.py \
  --output "$bundle_root/input"
python3 demos/zundamon-orbit/tools/inspect_zundamon_orbit_input.py \
  --manifest "$bundle_root/input/input.json"

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

The deterministic fixture, both row orders, exact recovered-index oracle,
focused one-mutation negative cases, and path-redacted CLI integration tests
pass. Repository and privacy checks pass. Report `LOCAL_INPUT_PENDING` unless
an explicit normalized local bundle was separately supplied and validated.
No human, image-approval, emulator, or physical-machine gate is implied. Stop
at G98d.
