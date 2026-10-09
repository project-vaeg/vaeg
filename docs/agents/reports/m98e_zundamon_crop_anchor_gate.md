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

# M98e - Zundamon crop and anchor review report

Evaluated predecessor: `086f7ffe7fba59e6efe6e94bf720cd8c22a0278b`

Status: **G98e human gate passed on 2026-08-31**

## Result

M98e adds a standard-library review-image generator for an M98d-validated
local bundle. It writes a full-source crop/anchor overlay, an unmarked crop,
and a crop-relative anchor overlay to a new explicitly supplied directory.
Integer nearest-neighbor review scaling is bounded to 1 through 8. The input
bundle is read-only, an existing output directory is rejected, and CLI output
does not disclose paths or private image metadata.

A neutral local bundle passed the M98d content inspector without modifying
its source, and the three untracked review images were generated below the
ignored output tree. This established `LOCAL_PREVIEW_READY` before human
review. The tracked tree records no private filenames, paths, hashes,
dimensions, crop or anchor values, colors, counts, or preview images.

## Machine evidence

The public standard-library suite ran four test methods and passed:

```text
OK
M98E_TEST_PASS
```

Coverage includes exact synthetic crop pixels, crop and anchor marker
positions, unchanged non-marker pixels, deterministic outputs, source-bundle
immutability, integer-scale bounds, overwrite refusal, and path-redacted CLI
success and failure.

M98b-M98d regression tests, Python compilation, repository encoding, EOL,
case, diff, JSON, and M98 privacy checks passed. M98e makes no color
conversion, atlas, guest, VAEG, or physical-machine claim.

## Human gate

The maintainer inspected all three untracked review images and stated
`G98e passed` on 2026-08-31, approving the crop, exact-background
transparency, unmarked crop content, and visual pivot. This closes M98e but
does not assign M98f.
