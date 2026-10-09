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


# M103g - V1/V2 semigraphics, 40 columns and sound timing

Status: **complete; G103g passed on 2026-10-04; merged to `main` at [375d96b](https://github.com/nakatamaho/vaeg/commit/375d96ba41819e13c504f429e9d575f6f16c1ba8)**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103g-v1v2-semigraphics-sound`
off `topic/m103f-v1v2-text-media` at `2853f652537f3ec82b179552301f34f9f8b93fc8`
(G103d–G103f pending). Commit prefix `M103g:`.

## Goal

Close the display and sound gaps the maintainer found at the G103d–G103f
check, which N-BASIC (M103h) also needs: µPD3301 semigraphics, 40-column
text, and the SR-DEMO music timing.

## Scope

1. **Semigraphics.** Colour attribute bit 4 (with bit 3) draws each
   character code as 2×4 blocks.
2. **40-column text** under 3301 emulation.
3. **Sound timing.** In SR-DEMO's winter scene the music starts late and
   only briefly (maintainer report); find the cause.

## Gate G103g (human)

Standard V3 gate unchanged, plus: SR-DEMO's four scene titles (the
seasons title, the CG theater title, the caricature title and the band
title) appear as large block letters; `WIDTH 40` in V2 BASIC shows
40-column text; the winter music starts with the scene.

## Implementation progress

### Semigraphics (scope item 1)

- Maintainer report: SR-DEMO's scene titles are semigraphics on the real
  machine. `[ROM]` In the TVRAM of such a scene the attribute pairs are
  `(0F,08) (41,B8) (50,08) (80,B8)…`: B8h is colour cyan with bit 4 set,
  over the columns that hold the title's codes.
- `[X88000]` builds the semigraphics font with bit n of the code lighting
  block row n mod 4 of the left (bits 0–3) or right (bits 4–7) half; the
  flag carries with the colour attribute. The BNN manual (8.2.1) does not
  list semigraphics among the 3301 functions the TSP leaves out. vaeg now
  draws these blocks, one quarter of the character height each, in the
  attribute colour.
- Result: the caricature title, which vaeg drew as stray kana and
  symbols, now assembles from blocks into large letters. Romless test:
  a semigraphics cell draws its set blocks in the attribute colour and
  leaves clear blocks; it fails without the change.

### 40-column text (scope item 2)

- With `WIDTH 40` N-88 BASIC writes characters to the even byte columns
  and clears port 30h 80CM (bit 0); vaeg drew them at normal width with
  gaps. Under 3301 emulation each even cell is now stretched over the next
  one (X88000 likewise skips the odd columns). The native V3 path is
  unchanged; its existing 40-column condition tests `!txtmode8 & 0x01`,
  an operator-precedence slip left alone here because V3 behaviour is out
  of scope. Romless test: the reverse cell of column 1 disappears under
  the widened column 0.

### Sound timing (scope item 3)

- Temporary counters (not committed) per second: sound timer requests,
  8214 acceptances, SINTM and FDC reads. In the winter scene about 75
  requests per second continued while none was accepted for about 14 s;
  the 8214 had status 0Fh (all levels allowed) and held levels 1 and 4,
  and the CPU was always sampled at the scene's `LDDR`.
- `[ROM]` The scene scrolls GVRAM plane 2 with `DI / OUT 5Eh / LDDR (756
  bytes) / OUT 5Fh / EI` twenty times, leaving three instructions between
  `EI` and the next `DI`. vaeg delivers interrupts only between CPU time
  slices; the native `STI` shortens the slice when a request waits, the
  compatible `EI` did not (fixed, ledger entry).
- After the fix every request in the scene is accepted; the 3D scene's
  losses also fell. Requests that coincide are merged by the 8214 latch,
  as on the hardware.
- Observation: the demo's scene timing differs by a few hundred frames
  between runs; not investigated.
