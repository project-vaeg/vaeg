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
# ADR-0003: Dear ImGui Japanese Font

Date: 2026-07-04

Amended: 2026-07-11

Status: Accepted

Decision maker: maintainer pre-decision for M10.

## Context

The SDL2 ImGui GUI must render Japanese-capable UTF-8 labels. Reading
the guest font ROM for host UI text is forbidden: the guest ROM is
emulated machine payload, not a host GUI asset, and it may be absent in
ROM-less tests.

Platform font lookup would make rendering and packaging differ across
Linux, MinGW, and macOS. M10 needs a deterministic redistributable font.

## Decision

Bundle Noto Sans JP Regular as:

- `assets/NotoSansJP-Regular.ttf`
- `assets/OFL.txt`
- `assets/NOTICE.md`

The source asset remains under `assets/`, but CMake converts the TTF to a
generated C byte array and links it into `vaeg` on every platform. Dear
ImGui loads the static data through `AddFontFromMemoryTTF()` with
`FontDataOwnedByAtlas=false`. No runtime font lookup or external TTF is
required.

The font is licensed under the SIL Open Font License, Version 1.1. The GUI
uses Dear ImGui's `GetGlyphRangesJapanese()` range. Any additional glyph
ranges or fallback-font strategy require updating this ADR.

Guest font ROM access is forbidden for the host GUI.

## Asset Record

| Item | Value |
|---|---|
| Upstream source | `https://github.com/google/fonts/tree/main/ofl/notosansjp` |
| Retrieved source file | `NotoSansJP[wght].ttf` |
| Static instance command | `python3 -m fontTools.varLib.instancer /tmp/m10-font/notosansjp-wght.ttf wght=400 --static --update-name-table -o assets/NotoSansJP-Regular.ttf` |
| fontTools version | 4.61.1 |
| Source SHA-256 | `c2f3b4d463500a2ddcd3849cded1fceeb9fd6d1c32e6cbecd568453ba50fc68f` |
| Bundled TTF SHA-256 | `c2b5cef5710fdf6d1bd96b7e51725c9c24945cc2db59f4d31947e44b48cebde1` |
| License SHA-256 | `babcfe66c8a098b2fa279bc724a3a342f8124f77ce18941fbcc1bbb39823cded` |
| Font family | Noto Sans JP |
| Font subfamily | Regular |
| PostScript name | NotoSansJP-Regular |
| Glyph count | 17936 |

## Consequences

- Release packages do not need a separate runtime TTF, but must include
  `assets/OFL.txt` and `assets/NOTICE.md` for license and provenance.
- The bundled font is binary payload. After introduction it should not be
  rewritten except by an explicit font asset update.
- Any future switch to platform fonts, guest ROM glyphs, FreeType
  rasterization, or extended glyph coverage requires an ADR update.
