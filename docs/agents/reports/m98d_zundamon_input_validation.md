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

# M98d - Zundamon orbit input-content validation report

Evaluated predecessor: `fdedf8048ff95910d6f4dc1c3e533b10536d9bbb`

Status: **G98d machine gate passed on 2026-08-31; `LOCAL_INPUT_PENDING`**

## Result

M98d adds a strict standard-library parser for the M98c local bundle. The
version-1 `bmp32` contract is a 54-byte BMP, 40-byte BITMAPINFOHEADER, 32-bpp
BI_RGB image with BGRA storage, no color table, and no extra payload. Positive
and negative stored heights support bottom-up and top-down row order.

The palette is exactly 48 RGB888 bytes. Entries 0 and 15 must equal the
declared background, entries 1-14 must be unique, and no visible entry may
equal the background. Each crop pixel must be either the exact background or
an exact visible palette color. Recovery produces top-to-bottom,
left-to-right indices in memory only.

The inspector rejects non-regular or symlink inputs, oversized files, malformed
headers, out-of-bounds crops, unexplained colors, empty crops, and crops without
transparency. Its CLI emits only `M98D_INPUT_PASS` on success or a stable error
code and neutral detail on failure.

## Machine evidence

The standard-library suite ran nine test methods and passed:

```text
OK
M98D_TEST_PASS
```

Coverage includes byte-reproducible bundles, both BMP row orders, exact
recovery of every public-fixture index, 28 focused negative scenarios,
zero-image-size BI_RGB handling, isolated header failures, missing and invalid
file cases, palette ambiguity, crop bounds, unexplained pixels,
all-transparent/no-transparency crops, overwrite refusal, and path-redacted
CLI success/failure.

The standalone workflow reported:

```text
M98D_FIXTURE_BUILD_PASS
M98D_INPUT_PASS
```

M98b-M98c regression tests, Python compilation, JSON, repository encoding,
EOL, case, diff, and M98 privacy scans passed. No normalized local bundle was
supplied to this stage, so no maintainer input was read and
`LOCAL_INPUT_PENDING` remains explicit. M98d makes no crop-approval, color
conversion, scaling, guest, VAEG, or physical-machine claim. M98e remains
unassigned.
