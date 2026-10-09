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

# M98c - Freeze the generic local-input manifest

Status: **G98c machine gate passed on 2026-08-31**

Branch: `topic/m98c-zundamon-input-manifest`

Commit prefix: `M98c:`

Gate type: **machine-verifiable**

## Goal

Freeze a source-neutral version-1 manifest that identifies an already
prepared local 32-bpp BMP and 16-entry RGB888 palette and declares crop,
exact-RGB transparency, and crop-relative anchor values. Validate only this
contract; file-content validation belongs to M98d.

## Required changes

- Add a Draft 2020-12 JSON schema and a neutral synthetic example.
- Require lowercase local basenames rather than absolute or parent-relative
  paths. The manifest and its inputs form one explicitly selected local
  bundle outside tracked directories.
- Fix image encoding to `bmp32`, palette encoding to `rgb888`, palette count
  to 16, transparent index to 0, and reserved index to 15.
- Fix crop coordinates and dimensions to bounded integers.
- Fix transparency to an exact three-channel RGB value.
- Define the anchor as an integer pixel coordinate relative to the crop's
  top-left corner and require it to lie inside the crop.
- Reject unknown, missing, duplicate, ill-typed, malformed, oversized, BOM,
  and non-UTF-8 input with stable `M98C_*` error codes.
- Never print the supplied manifest path or input basenames in success or
  failure output.

## Privacy boundary

The tracked schema contains no origin, acquisition, tool, source title,
source identifier, palette identifier, hash, or free-form notes field. M98c
does not read the referenced BMP or palette. The public example uses only
neutral filenames and synthetic dimensions.

## Out of scope

- Checking whether referenced files exist or match their declared encodings.
- Reading BMP pixels or palette bytes.
- Approving a crop or anchor for a maintainer-supplied image.
- Color conversion, scaling, atlas packing, guest code, VAEG, or physical
  machine work.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98c-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_manifest.py

python3 demos/zundamon-orbit/tools/validate_zundamon_orbit_manifest.py \
  --input demos/zundamon-orbit/examples/input-manifest-v1.json

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

The schema-contract test, neutral example, focused one-mutation negative
tests, duplicate-key parser test, and CLI integration test pass. Every
negative test asserts its exact stable error code. Privacy and repository
checks pass. No human, file-content, emulator, or physical-machine gate is
implied. Stop at G98c.
