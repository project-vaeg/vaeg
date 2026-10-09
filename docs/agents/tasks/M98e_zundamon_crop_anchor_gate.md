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

# M98e - Approve crop, transparency, and anchor

Status: **G98e human gate passed on 2026-08-31**

Branch: `topic/m98e-zundamon-crop-anchor`

Commit prefix: `M98e:`

Gate type: **human/local**

## Goal

Generate deterministic private previews from the validated M98d bundle so the
maintainer can approve the exact crop, transparency result, and visual pivot
without publishing the input or its identifying metadata.

## Required changes

- Add a standard-library preview tool that consumes the M98d bundle without
  modifying it.
- Write only to a new explicitly supplied output directory.
- Emit a full-source crop/anchor overlay, an unmodified nearest-neighbor crop
  preview, and a crop-relative anchor overlay.
- Keep preview filenames neutral and keep the generated files outside Git.
- Keep CLI success and failure output free of input/output paths, filenames,
  dimensions, crop coordinates, anchor coordinates, colors, counts, and
  hashes.
- Add synthetic tests for exact crop pixels, overlay positions, deterministic
  output, source immutability, bounded integer preview scaling, overwrite
  refusal, and path-redacted diagnostics.

## Private boundary

The local manifest, source image, palette, crop and anchor values, preview
images, and their identities remain untracked. The tracked report may state
only whether a neutral local preview is ready and whether the maintainer has
passed the gate.

## Out of scope

- Changing source pixels, palette entries, or transparency rules.
- Claiming that an unreviewed crop or anchor is approved.
- VA color conversion, scaling-atlas generation, guest code, VAEG, or physical
  machine work.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98e-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_crop_preview.py

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Human acceptance

The maintainer inspects all three untracked previews and confirms that:

1. the crop selects exactly the intended subject and excludes unrelated
   components and unnecessary border;
2. transparency removes only the intended background;
3. the unmarked crop contains no contamination or clipped intended pixels;
4. the anchor marks the approved stable visual pivot; and
5. the source bundle remains unchanged.

The maintainer stated `G98e passed` on 2026-08-31 after inspecting all three
local previews. M98e is closed. Do not begin M98f until it is explicitly
assigned.
