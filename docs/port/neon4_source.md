<!--
Copyright (c) 2026 Nakata Maho

This document is licensed under the BSD 2-Clause License.  See the repository
license for the complete terms.  Ported By Maho Nakata.
-->

# NEON RELAY 4 P0-1 source extraction

This milestone extracts the 80286 geometry and scene progression into
`demos/neon4/src/`.  It is an assembly-only source check; it is not yet a
PC-88VA payload and it does not claim any VA hardware behavior.

## Retained source

| File | Role | Status |
| --- | --- | --- |
| `config4_256.inc` | shared scene constants and logical 640x400 coordinates | copied from `neon4_1_0` |
| `config4_286.inc` | 286-safe configuration and scene timing | copied from `neon4_1_0` |
| `geom4_low.inc` | 286-safe transforms, projection, raster geometry, and scene routines | copied unchanged for P0 extraction |
| `scene4_256.inc` | scene selection and indirect dispatch | copied from `neon4_1_0` |
| `frame_render4_low.inc` | frame-level call skeleton | copied from `neon4_1_0` |
| `data4_p0.inc` | mutable state, geometry tables, authored scene data, and score data | extracted from `DATA4.INC`; OPL3-only state/table block removed |
| `low4_data.inc` | 286 renderer scratch and source tables | copied from `neon4_1_0` |
| `music_neon4_melody.inc` | authored common score data | copied because `data4_p0.inc` includes it |

The geometry source is intentionally retained as a single source of truth for
the eight `scene4_*` routines.  Its low-colour implementation still contains
the original platform helper bodies for source archaeology; the P0 harness
does not execute them.  They are replaced by VA primitives in later P4/P6
milestones, after the hardware contract is approved.

## Removed from the P0 extraction

The P0 tree does not copy or include the following original modules:

- `OPL3.INC` and the OPL3-only data/table block;
- `AFS4_256.INC` / `AFS4_286.INC` interrupt scheduler paths;
- `BREAKGUARD.INC`, `FAULTGUARD.INC`, and DOS command-line handling;
- `VIDEO4_LOW.INC`, `VIDEO4_256.INC`, `VIDEO256_PACKED*.INC`, and the
  original GRCG/EGC device-control code as an entry path;
- `TEXT4_*` and the original DOS text/console path.

`data4_p0.inc` keeps OPNA-authored data only for the later P8 audio milestone;
no OPNA or OPL device routine is linked by this P0 harness.

## Primitive contracts

`neon4_p0.asm` supplies inert `ret` stubs for the platform entry points used by
the frame skeleton and geometry, including:

```text
clear_graphics_frame16  set_access_page  text_update
hline_set*              line_set*       fill_rect
line_batch_begin        grcg16_prepare_color
low_dirty_span_*        low_raster_track_rect16
egc16_enable_vram_copy  egc16_disable_to_grcg
```

The stubs preserve the original symbol/calling-contract surface while making
it impossible for P0 to access GVRAM, SGP, or guessed VA I/O registers.

## Build and reachability evidence

Build command:

```sh
demos/neon4/build_p0.sh /tmp/neon4-p0.com
```

Observed result on the evaluated checkout:

```text
built /tmp/neon4-p0.com (36337 bytes)
```

The NASM listing shows `render_scene` indexing the eight-entry
`scene_routines` table:

```text
scene4_solid_primitives
scene4_facet_rotation
scene4_pattern_cube
scene4_morphing_solid
scene4_shift_blitter
scene4_ribbon_wave
scene4_checker_plane
scene4_material_finale
```

`neon4_p0_render_probe` calls `select_scene` followed by `render_scene` so the
dispatch path is present in the flat binary.  The entry point itself stops
without invoking DOS, VA BIOS, SGP, or graphics I/O.

## P0-1 boundary

This milestone proves only that the selected 286 geometry/data/scene source
assembles together.  It does not prove 320x200/640x200 mode setup, RGB332
packing, page switching, SGP command semantics, BITBLT descriptors, text, or
OPNA playback.  Those remain P1 and later work.
