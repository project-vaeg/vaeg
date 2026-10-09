<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M97 P4 direct convex-polygon face fill

## Previous architecture

The P4 face path represented each visible quadrilateral as two calls to the
triangle filler.  That output was geometrically correct, but it exposed an
artificial two-pass construction when the drawing process was observed.
The triangle filler remains a valid independent primitive; only the production
face path changed.

The pre-change reference was commit `8cfa281b11c111a9e27d0d01d11e5464e88326f0`.
Its known-good checks were preserved before this change.

## New architecture

The face path is now:

```text
four projected face vertices
    -> glass_p4_convex_fill_polygon()
    -> zero or one exact logical span per physical row
    -> CPU or SGP span writer
    -> intended outline LINE list
```

`src/glass_convex.inc` is the single geometry implementation included by
both the CPU and SGP payloads.  The backend callback is the only difference:
the CPU writes logical pixels directly, while the SGP path emits complete
interior CLS words and replays general endpoint masks.

The source guard `tools/check-p4-convex.py` requires one convex-polygon call in
each P4 face routine and rejects `fill_triangle` calls in those routines.
The standalone triangle implementation is retained for primitive QA.

## Rasterization rule

- Physical rows are sampled at `y + 1/2`.
- Non-horizontal edges are active on the half-open interval
  `min_y <= sample_y < max_y`.
- The leftmost intersection is rounded with `ceil()`.
- The rightmost intersection is rounded with `floor()`.
- Spans are inclusive `[x0, x1]`.
- A polygon with fewer than three vertices, a zero-area polygon, or an empty
  `x0 > x1` span emits no pixels.
- Horizontal edges are not intersections, and the rule is independent of
  clockwise or counter-clockwise vertex order.

No internal diagonal or post-fill repair stage exists in the P4 face path.

## 4bpp write model

The storage contract remains four logical pixels per 16-bit word.  The
independent calibration remains:

```text
x % 4 = 0 -> 00f0h
x % 4 = 1 -> 000fh
x % 4 = 2 -> f000h
x % 4 = 3 -> 0f00h
```

The SGP writer uses exact endpoint read-modify-write masks and complete
interior words.  Word alignment never changes the logical polygon boundary.

## QA evidence

The final VAEG captures use stable local artifact labels `p4-convex-face`,
`p4-convex-final`, and `p4-convex-repeat`.

| check | result |
| --- | --- |
| face-only underfill | 0 |
| face-only overfill | 0 |
| internal holes | 0 |
| edge visible gaps | 0 |
| edge visible leaks | 0 |
| vertex pinholes/protrusions | 0 |
| direct convex oracle | PASS |
| previous triangle-union vs direct-quad oracle | 0 differing pixels |
| convex triangle/rectangle/slanted quad/diamond/trapezoid | PASS |
| narrow 1..5-pixel cases | PASS |
| pentagon case | PASS |
| winding reversal | PASS |
| duplicate/degenerate cases | PASS |
| x-mod-4 and outside-border matrix | PASS |
| CPU/SGP GVRAM comparison | PASS |
| repeated SGP GVRAM/screen comparison | PASS |
| no-repair guard | PASS |

The final raw GVRAM and composed-screen digests were identical for the CPU,
SGP, and repeated SGP captures.  The direct convex output also retains the
previous fixed-frame checksum (`7ACEh`).

The visual artifact checker was run on the fill-only and final logical
640x200 captures.  It reports no holes, no overfill, and no edge-registration
gap or leak.  The one-pass face construction is established by the shared
geometry include and source guard.  No claim is made here about the timing of
progressive drawing on real hardware; any remaining scan-time visibility is a
separate presentation concern.

## Real-hardware status

No new hardware run was performed as part of this emulator-side conversion.
The previously recorded observations remain:

```text
PC-88VA:
  graphics: PASS
  ESC return: PASS
  keyboard after return: UNRESOLVED

PC-88VA2:
  graphics: PASS
  ESC return: PASS
  keyboard after return: PASS
```

This result is an emulator-side regression result and does not establish
silicon-level SGP conformance.  `SCAN_LEFT` and `SCAN_RIGHT` remain a
separate QA item because this GLASS path does not use them.

## Files changed

- `demos/glass-orbit/src/glass_convex.inc`: shared convex scan
  converter.
- `demos/glass-orbit/src/glass_orbit_sgp_backend.asm`: one convex call per
  visible SGP face; triangle filler retained for QA.
- `demos/glass-orbit/src/glass_orbit_p4_cpu.asm`: same shared geometry for
  the CPU reference path.
- `demos/glass-orbit/tools/check-p4-convex.py`: structural one-pass face
  guard.
- `demos/glass-orbit/build.sh`: builds the final wrapper around the
  shared convex-face backend and invokes the OPNA source guard.
- `demos/glass-orbit/tools/verify-p4-visual.py`: direct convex oracle,
  triangle-union comparison, and generic convex-shape matrix.
- `docs/port/glass_orbit.md`: final direct convex-face architecture and contract.
