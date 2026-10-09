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

# M98f - Convert opaque colors to VA 8-bpp direct color

Status: **G98f machine gate passed on 2026-08-31; `LOCAL_VA8_PASS`**

Branch: `topic/m98f-zundamon-va8-conversion`

Commit prefix: `M98f:`

Gate type: **machine-verifiable public fixture; optional local integration status**

## Goal

Convert the M98d-recovered crop to deterministic VA 8-bpp `GGGRRRBB`, keep
byte `00h` exclusive to transparency, and record private collision and
quantization diagnostics without changing the approved input.

## Conversion contract

For every opaque RGB888 palette entry:

```text
red3   = (red8   * 7 + 127) // 255
green3 = (green8 * 7 + 127) // 255
blue2  = (blue8  * 3 + 127) // 255
va8    = (green3 << 5) | (red3 << 2) | blue2
```

Source index 0 produces `00h`. If an opaque entry produces `00h`, evaluate
every VA byte 1 through 255 after expanding its channels with nearest-integer
`level * 255 / maximum`. Select the byte with the smallest RGB squared error;
select the lowest byte when errors tie. Do not dither or modify geometry.

## Required changes

- Add a standard-library converter that consumes the complete M98c/M98d
  bundle through the existing validators.
- Emit a top-to-bottom, left-to-right raw `pixels.va8` file and deterministic
  private `report.json` in a new explicitly supplied output directory.
- Record per-entry conversion, usage, squared error, opaque-zero repair,
  used-entry collisions, and aggregate error in the private report.
- Reject invalid geometry, pixel length, palette length, RGB/VA8 range,
  reserved pixel indices, and an existing output directory with stable codes.
- Keep CLI success and failure output free of paths, filenames, dimensions,
  palette values, output bytes, counts, errors, collisions, and hashes.
- Add independent-oracle tests for channel order, rounding boundaries,
  transparent-zero preservation, opaque-zero repair, and collision reporting.

## Private boundary

The local manifest, source image, palette, converted pixels, report, and all
of their identifying metadata remain untracked. Tracked text may record only
the neutral `LOCAL_VA8_PASS` integration status.

## Out of scope

- Scaling, dithering, atlas metadata or packing, guest code, VAEG, and
  physical-machine work.
- Changing the G98e-approved crop, anchor, palette, or source pixels.
- Publishing local conversion values or interpreting a collision as a visual
  failure without maintainer review.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98f-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_va8.py

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

The public fixture matches an independent conversion oracle, every opaque
fixture pixel is nonzero, transparent pixels stay zero, focused failures
reach exact codes, deterministic outputs match byte for byte, source inputs
remain unchanged, and CLI diagnostics remain path-redacted. Repository and
privacy checks pass. A separately approved local bundle may report only
`LOCAL_VA8_PASS`. This is a machine gate. Stop at G98f; M98g remains
unassigned.
