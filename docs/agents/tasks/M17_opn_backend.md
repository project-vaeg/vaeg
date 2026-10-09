<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# M17 - Add a selectable NP2/ymfm OPN backend

Status: complete

Branch: `topic/m17-ymfm-sound-backend`

Gate: G17 passed

Depends on: G16 passed.

## Goal

Keep the existing NP2 OPN/OPNA implementation as a selectable compatibility
backend and add a default FM operator backend derived from MAME's
BSD-3-Clause `3rdparty/ymfm` implementation. The SDL2 Dear ImGui Sound
menu selects `NP2` or `ymfm`, and the selection persists as
`opn_backend=np2|ymfm`.

## Non-goals

- Do not import MAME GPL device code.
- Do not define `OPNGENX86`.
- Do not replace NP2 board I/O, register shadows, timer status/IRQ,
  SSG, ADPCM-B, rhythm, or final stream mixing in this milestone.
- Do not combine uPD9002/uPD70002, V30, Z80, `SUPPORT_PC88VA`,
  `VAEG_FIX`, or `VAEG_EXT` work with this milestone.
- Do not modify the frozen reference tier.

## Constraints

- Vendor only the required files from `mamedev/mame/3rdparty/ymfm`
  under `external/ymfm/`, as exact upstream copies.
- Record the release, upstream URL, commit SHA, subtree SHA, license,
  and imported file scope in `docs/agents/DECISIONS/`.
- Keep core-facing code C99. Confine the handwritten C++17 bridge to
  `sdl2/` and expose a small C ABI to the sound core.
- Preserve the existing `opngen_*` API used by sound boards.
- Missing or invalid configuration values must select `ymfm`; an explicit
  `np2` value must continue to select the compatibility backend.
- Preserve UTF-8 without BOM, LF line endings, and lowercase path
  rules.

## Required work

1. Audit the current OPN/OPNA path and document ownership of:
   - board I/O and register shadows;
   - FM register writes and key-on;
   - timer A/B, status, and IRQ delivery;
   - SSG;
   - ADPCM-B;
   - rhythm;
   - final mixing and SDL audio output.
2. Add an internal backend selector while preserving the public
   `opngen_*` call surface and NP2 behavior.
3. Vendor the pinned BSD-3-Clause ymfm OPN dependencies only.
4. Add a C ABI bridge for YM2203 and YM2608 FM operator generation.
5. Mirror the original, unmodified FM register values, key-on values,
   channel-3 mode writes, and required timer cadence to ymfm.
6. Keep NP2 authoritative for timer status/IRQ, SSG, ADPCM-B, rhythm,
   register shadows, and final mixing.
7. Add `opn_backend=np2|ymfm` to the SDL2 configuration. Default and
   invalid values are `ymfm`; explicit `np2` remains supported.
8. Add `Device -> Sound -> OPN backend -> NP2 / ymfm` to Dear ImGui.
   Use the existing guest reset flow when changing backend so stale
   synthesizer phase/envelope state is not resumed. Preserve configured
   FDD and SASI mounts across this reset.
9. Add ROM-less tests for configuration parsing and non-silent YM2203
   and YM2608 generation through both backend paths.
10. Document the staged ownership split and any expected audible
    differences in the SDL2 and GUI documentation.

## Vendor decision

ADR-0009 records the selected source and integration boundary:

- MAME release `0.288` (`mame0288`);
- MAME release commit
  `27a8d9e85b58058965907d1d8a7a92f8ed039348`;
- ymfm subtree SHA `764185eef1ee5c30b1bdc2ef86ec90adb7c00948`;
- BSD-3-Clause license;
- FM operator synthesis only for this milestone.

## Verification

Run, or explain why unavailable:

```sh
cmake --preset linux-debug
cmake --build build/linux-debug --target vaeg_sdl2
cmake --preset linux-release
cmake --build --preset linux-release

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --smoke
SDL_AUDIODRIVER=dummy ./build/linux-release/sdl2/vaeg --selftest

git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py

rg -n "opn_backend|OPN_BACKEND|ymfm|OPNGENX86" \
  CMakeLists.txt README.md sound sdl2 cbus iova external/ymfm \
  docs/agents/DECISIONS
```

The same build and selftest should also be run with clang when available.

## Gate G17

G17 is a human FM/audio comparison gate:

- clean-build the active SDL2 tree;
- boot V3 mode, the bundled VA demo, and an OS with the NP2 backend;
- select ymfm and confirm the automatic reset retains configured FDD
  and SASI media;
- repeat the V3/demo/OS checks with ymfm;
- compare pitch, tempo, envelope shape, algorithm/feedback timbre,
  channel pan, and relative FM volume on known material;
- exercise channel-3 special mode/CSM when suitable software is
  available;
- confirm SSG, ADPCM-B, and rhythm remain audible and do not change
  when only the FM backend is switched;
- restart vaeg and confirm backend persistence;
- switch back to NP2 and confirm the compatibility path still behaves
  as before.

Expected residual differences must be documented rather than hidden.
The gate does not pass merely because both backends produce nonzero PCM.

## Completion record

The active SDL2 build vendors the pinned BSD-3-Clause ymfm OPN source,
uses ymfm as the default FM-operator backend, and retains NP2 as a
selectable compatibility backend. The ImGui Sound menu and
`opn_backend=np2|ymfm` configuration select the backend; switching uses
the normal reset path and preserves configured FDD and SASI media.

G17 passed after the maintainer compared both backends and confirmed
boot, demo, OS, audio, backend switching, media retention, and setting
persistence. Timer/IRQ, SSG, ADPCM-B, rhythm, board I/O, and final mixing
remain owned by the existing NP2 implementation as documented in
ADR-0009.
