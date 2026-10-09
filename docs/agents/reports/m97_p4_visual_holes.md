# M97 P4 visual artifact investigation

## Scope

This report separates the regular 200-line presentation raster from localized
background holes inside active GLASS faces.  The former is a VAEG presentation
policy and is not changed here.  The latter is a guest-visible raster defect.

## Evidence and root cause

The raw G0 capture is decoded as a 640x200 packed-4bpp page (320 bytes per
row), before SDL scaling and before the blank-raster presentation path.  The
pre-fix capture contained 155 interior background runs, including repeated
4-pixel runs whose X coordinate moved diagonally with Y.  Thus the defect was
already present in raw GVRAM and was not caused by `makegrphva_blankraster()`.

The first P4 implementation rounded each triangle span inward to complete
packed words.  The shared diagonal consequently lost pixels.  Outward
rounding removed the seam but expanded polygon edges.  A temporary bridge
stage then hid two one-pixel line/fill gaps.  CPU and SGP equality during those
iterations was not an independent correctness oracle because they shared the
same span convention.

The demonstrated general cause was inconsistent integer-X span ownership at
the triangle's top/vertex rows.  A computed `x_left > x_right` span was being
swapped by the generic emitter, turning an empty row into an out-of-polygon
pixel.  The fix is to discard that empty span and to use one deterministic
`y+1/2`, `ceil(left)`, `floor(right)` rule in both verification paths.

## Compensation inventory

| location | operation | category | reason | action |
| --- | --- | --- | --- | --- |
| `glass_orbit_sgp_backend.asm:glass_p4_sgp_apply_endpoint_spans` | masked first/last-word RMW | A/B | exact logical span access for packed 4bpp | keep |
| `glass_orbit_sgp_backend.asm:glass_p4_sgp_build_edge_list` | redraw the twelve intended LINE edges | A | normal wireframe rendering stage | keep |
| previous `glass_p4_sgp_bridge_outline_gaps` stage | one-pixel outline-colour writes at known cube gaps | C | geometry-specific visual repair | removed |
| `glass_orbit_sgp_backend.asm:glass_p4_sgp_emit_span` | swap when `x0 > x1` | C-equivalent bug | converted empty vertex spans into coverage | removed; empty spans are discarded |

There are no geometry-specific post-fill rectangles, erases, per-face offsets,
or cube-coordinate repair writes in the final P4 path.  The source-level
`check-p4-no-repair.py` guard fails closed on the removed repair-stage symbols
and requires exactly one general endpoint stage.

## General algorithm

The final pipeline is:

```text
face geometry
    -> sorted triangle edges sampled at y+1/2
    -> exact inclusive [ceil(left), floor(right)] span
    -> complete interior CLS words plus masked endpoint RMW
    -> intended outline LINE list
```

The logical geometry is never expanded for word alignment.  One 16-bit
packed-4bpp word represents four logical pixels.  Independently calibrated
masks are `x%4 = 0,1,2,3 -> 00f0h,000fh,f000h,0f00h`.  Both same-word and
multi-word spans preserve pixels outside the exact interval.  The host oracle
uses rational scanline intersections and does not reuse production masks or
interpolation code.

## Independent checker

`demos/glass-orbit/tools/verify-p4-visual.py` decodes raw logical pixels,
reports every interior background run with width and modulo-8/modulo-16
coordinates, and runs an independent pixel-array rectangle alignment matrix
for all eight low-bit start positions, including exhaustive endpoint and
outside-border cases.  The payload exports the computed vertices into an
unused GVRAM tail; the checker uses those vertices only as polygon input and
performs its own host-side scan conversion.  It reports independent
`underfill` and `overfill` counts.  Face-only stage 2 is checked before
outlines; final stage 3 ignores outline colours when counting underfill.

## Loader issue (orthogonal)

The SGP COM loader saved the pre-continuation stack pointer and therefore its
payload `retf` returned through a flags word.  It now follows the known-good
CPU loader convention: preserve flags, push `cs:loader_return`, save the real
`ss:sp`, then far-call the payload.  ESC detection uses VA Keyboard BIOS
`$GetChar`'s documented `BH=0, BL=1Bh` representation rather than treating any
zero AH as ESC.

## Results

| check | before | after |
|---|---:|---:|
| raw logical interior runs | 155 | 0 |
| face underfill | 220 (shared CPU/SGP convention) | 0 |
| face overfill | 168 (outward-rounding regression) | 0 |
| edge-registration gaps | 2 one-pixel gaps before cleanup | 0 |
| edge-registration leaks | 0 | 0 |
| rectangle/alignment/outside-border failures | 0 | 0 |
| regular blank-raster presentation | retained | retained |
| ESC loader return | corrupt continuation | clean return |

The final face-only capture reports `underfill=0, overfill=0`; the outlined
capture reports the same with a separate edge-registration check.  The final
no-bridge run reports zero visible gaps, zero visible leaks, zero missing
outline pixels, and zero internal holes.  The four-pixel word calibration
independently reports `00f0h,000fh,f000h,0f00h` for `x%4=0,1,2,3`.  The final
logical PNG and normal presentation PNG were inspected: the diagonal holes
and word-aligned protrusions are absent; the regular blank-raster rows remain
the expected presentation behavior.  `SCAN_LEFT`/`SCAN_RIGHT` is not used by
this GLASS path and remains a separate QA item.

Generic host QA also passes both diagonals of a two-triangle rectangle, the
positive/negative shallow/steep slope matrix, vertical/horizontal cases, and
all rectangle endpoint/residue checks.  The CPU direct-pixel path was updated
to the same exact logical-span convention and produces the same 256 KiB GVRAM
snapshot and composed screen as the SGP path; its current checkpoint checksum
is `7ACEh`.

## Real-hardware status

These are maintainer observations, not evidence produced by the VAEG run:

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

The remaining hardware-conformance question is separate: this is an emulator
side regression result and does not establish silicon-level PC-88VA behavior.
