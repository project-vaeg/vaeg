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

# M98k - Bring up the isolated 320x200 8-bpp guest

Status: **G98k human/VAEG gate passed on 2026-08-31 at candidate `6ed575dedc5da8827704c33a274ac72e480ce420`; M98k closed**

Branch: `topic/m98k-zundamon-guest-bringup`

Starting commit: `8b8c5ceeac5445ba1eb0d3aa804974db09de6809`

Implementation commit: `6ed575dedc5da8827704c33a274ac72e480ce420`

Commit prefix: `M98k:`

Gate type: **human/VAEG after fail-closed automated capture validation**

## Goal

Build the smallest bootable guest proof for the M98 display path. Enter the
proven 320x200 G0/G1 8-bpp configuration, draw a static nonzero G0 background,
and submit exactly one SGP command list containing one transparent BITBLT of
one built-in 16x16 synthetic marker to G1 at `(152, 92)`. Keep the completed
image static after the bounded SGP completion wait.

M98j remains closed. M98k inherits its public/private boundary and host-side
pipeline authority, but M98j contains no guest boot implementation. The guest
mode, composition, SGP, build, local-disk, and VAEG launch conventions come
from the existing `demos/sgp-pseudo-sprite/256/` and local demo workflows.

## Fixed guest contract

- Use a 16-bit DOS `.COM` program below 64 KiB.
- Select video mode `e00eh` with G0/G1 pixel-size word `0808h` through
  `INT 8fh`, matching the established 256-color demo.
- Select G1 over G0 through compose value `0034h`, palette-compose value zero,
  and direct-color compose value `0089h`.
- Treat every 8-bpp byte as one deterministic `GGGRRRBB` direct-color entry.
  This defines all 256 byte-to-RGB entries; do not invent a programmable
  palette-RAM path that the reference demo does not use.
- Keep G0 opaque and make only G1 value zero transparent.
- Configure FB1 as a 320-byte-pitch, 400-line backing surface with a 200-line
  display window. M98k uses only page A at SGP address `0220000h` and DSA
  `0020000h`; page exchange belongs to M98o.
- Fill G0 with a deterministic, entirely nonzero checkerboard using ordinary
  CPU GVRAM writes.
- Build one SGP list that clears the visible 64,000-byte G1 page and performs
  one 16x16 transparent BITBLT from conventional guest memory to `(152, 92)`.
- Use SGP BITBLT mode `0105h`. Wait for idle before submission and completion
  afterward with the reference demo's finite polling bounds.
- Publish page A only after the SGP list completes. Do not submit another SGP
  list in the idle loop.
- Expose stable English console messages and fixed register values at a
  once-per-VBLANK idle checkpoint so the debug harness distinguishes mode
  initialization, SGP completion, settled state, and failure.
- ESC may restore the saved video and memory-map state and exit. It is the
  only guest input; there are no animation or runtime controls.

## Synthetic marker

- Width and height are exactly 16 pixels.
- Row stride is exactly 16 bytes, with zero padding bytes per row.
- Value zero is transparent.
- The marker contains a one-pixel border, a diagonal, and an off-center filled
  block using at least three distinct nonzero `GGGRRRBB` values.
- The complete 256-byte marker is repository-owned source data embedded in the
  `.COM`; it is not derived from `orb_raytrace8_24.inc` or any local input.

## Build, disk, and capture

- Add a script-relative NASM build below `demos/zundamon-orbit/256/`.
- Add a local-only bootable-D88 builder that clones an explicitly supplied
  bootable PC-Engine 2HD template, installs `ZUNDORB.COM`, refuses overwrite,
  and leaves both template and output untracked.
- Add one VAEG runner using explicit executable, ROM-directory, template, and
  new output-directory arguments. It must use `--no-cfg`, `--no-bkupmem`,
  dummy SDL, the current debug harness, and deterministic guest-frame bounds.
- Capture two consecutive appearances of the settled idle checkpoint. Each
  capture includes registers, the complete 256-KiB GVRAM image, and the
  composed screen.

## Host oracle

Use only the Python standard library. Fail closed unless it proves:

1. both register captures report the fixed 320x200/8-bpp/completion signature;
2. both complete GVRAM captures are byte-identical and exactly 256 KiB;
3. G0 contains the expected nonzero checkerboard;
4. G1 page A is zero outside one exact 16x16 marker at `(152, 92)`;
5. marker rows, 16-byte stride, zero transparency, and all expected nonzero
   values match exactly;
6. the marker has one occurrence, the expected nonzero-pixel count and bounding
   box, and no second copy in the 320x400 G1 backing surface;
7. the debug event chronology reaches two settled captures without a timeout;
8. the composed captures are byte-identical, nonempty, and not all black; and
9. static source checks find no external asset, BMS, EMS, XMS, scaling,
   animation, or multi-instance path.

Focused negative tests must begin from a passing synthetic capture fixture,
apply one mutation, and assert one stable error code.

## Out of scope

- External image or atlas loading.
- BMS, EMS, XMS, bank selection, or extended-memory caching.
- Scaling or any scale-atlas traversal.
- Animation, rate selection, page exchange, or multiple instances.
- Private artwork, ROM extraction, disk analysis, or sprite decoding.
- Emulator changes or host-rendered substitution for guest output.
- Physical-PC-88VA conformance or timing claims.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98k-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py

NASM=/opt/local/bin/nasm \
  sh demos/zundamon-orbit/256/build.sh \
  build/generated/zundamon-orbit/m98k/ZUNDORB.COM

sh demos/zundamon-orbit/build-local-d88.sh \
  /path/to/local-bootable-2hd.d88 \
  build/generated/zundamon-orbit/m98k/zundamon-orbit-m98k.d88

VAEG_ZUNDAMON_MODEL=va2 sh demos/zundamon-orbit/run-vaeg.sh \
  /path/to/local-bootable-2hd.d88 \
  /path/to/vaeg \
  /path/to/rom-directory \
  build/generated/zundamon-orbit/m98k-run

python3 demos/zundamon-orbit/tools/verify_zundamon_orbit_guest.py \
  build/generated/zundamon-orbit/m98k-run

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

The guest build and local bootable disk are reproducible from clean generated
output. One bounded SGP list completes and leaves one exact embedded marker on
G1 at `(152, 92)` over the expected nonzero G0 background. The indexed GVRAM
oracle and two-frame stability checks pass, the rendered captures are stable
and non-black, diagnostics distinguish failure from completion, and no private
or generated payload enters Git.

Automated success produced the M98k VAEG candidate. The maintainer inspected
the human-gate result and explicitly stated that G98k passed on 2026-08-31.
M98k is closed. M98l remains unassigned.
