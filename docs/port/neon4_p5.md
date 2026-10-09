# NEON4 P5 full-scene status

The stage-8 payload now runs the complete NEON4 timeline: all eight original
scene routines, 384 logical frames each, for `TOTAL_FRAMES = 3072`.  There is
no `NEON4_P5_SCENE` build-time selector.  The original geometry remains in
logical 640x400 coordinates; the backend halves X and Y at primitive entry for
the 320x200 target.

## Scope

The stage-8 payload advances one absolute frame counter from 0 through 3071.
`select_scene` maps that counter to the original eight chapter-local
`scene_frame` values, then `render_scene` dispatches through the original
eight-entry `scene_routines` table.  Each iteration waits for a VBLANK edge,
clears the hidden draw page, submits SGP batches, waits for SGP idle, and flips
the draw page at the next VBLANK edge.  ESC remains an early exit; after the
3072nd frame both published profiles reset the logical counter to zero and
continue from scene 0.

The RASTER TRANSFER scene and recurring raster panels use the existing generic
span fallback while `low_egc_available` is disabled.  This is a functional
SGP/CLS fallback; SGP BITBLT conformance remains a separate P6 item.

The same source now builds three profile values:

| `NEON4_P5_PROFILE` | Physical mode | Storage |
| --- | --- | --- |
| `16` | 640x400 | packed 4bpp G0 with sixteen VA palette entries |
| `256` | 320x200 | packed RGB332 bytes |
| `65536` | 320x200 | direct 16bpp |

The `16` profile is the PC-88VA 16-colour mode.  It is not RGB332: every
pixel is a four-bit index, while each of the sixteen entries selects a colour
from the VA's 4096-colour palette.  The backend converts the source G/R/B
nibbles to the documented `$SetPal` word layout (`G<<12 | R<<6 | B<<1`) and
uploads the entries once at startup.  See `demos/neon4/README.md` for the
mode and distribution details.

The P5 presentation now follows the validated 8bpp two-page sequence from
`demos/sgp-pseudo-sprite`: the displayed page is never cleared or rebuilt
while visible.  This removes the clear/rebuild flicker; any remaining motion
is scene animation rather than an incomplete-frame exposure.

## Text overlay and console ownership

Both published profiles keep the VA TEXT plane enabled and draw the live
NEON4 status overlay through the same VA BIOS text path used by NEON3.  At
stage-8 entry, `INT 83h` with `AH=2Fh, AL=00h` removes the inherited soft-key
guide and `INT 94h` with `AH=01h, AL=FFh` hides the reserved system line.  The
overlay then selects text-only composition, clears all TVRAM rows while the
display is hidden, writes fixed labels once, and restores text-above-G0
composition.  Each VBLANK edge updates only fixed-width frame/local values;
the scene title row is cleared and rewritten only when the scene changes.  The
cursor is hidden with Text BIOS `AH=25h, AL=00h`.  Exit restores the caller's
ten-entry guide with `AL=0Ah` for both services and the normal cursor.  No DOS
console API, direct TVRAM address, or TEXT-OFF workaround is used.

The VA colour path keeps the original 0..255 PEGC index through the low
geometry helpers.  `neon4_va_palette.inc` contains two independent conversion
tables from the original `pegc_palette_grb` source triplets (G/R/B, 8 bits
each): the 8bpp profile quantises to direct RGB332 bytes (`gggrrrbb`), while
the 65536 profile converts directly to the VA direct-colour word
`G6[15:10] | R5[9:5] | B5[4:0]`.  The 16bpp path therefore never discards the
source blue component through an intermediate 2-bit RGB332 value.

The first VAEG run exposed an address-calculation regression: the imported
`config4_256.inc` planar constants redefine `BYTES_PER_LINE` as 80.  The VA
packed backend restores the physical 320-byte pitch immediately after the
scene includes.  Without that override, every SGP row address was compressed
and the scene appeared as a band near the top of the display.

## Direct-profile scene colour selection

The low-colour geometry helpers retain their graph-coloured 16-entry tables
only for the 4bpp profile.  The 8bpp and 65536-colour profiles follow the
original NEON4 256-colour scene calculation: for a front-facing face,
`shade = clamp((projected_cross >> 11) + 8, 8, 31)`, then
`colour_a = n4_geo_colour_base + shade` and `colour_b = colour_a - 3`.
The direct profiles do not dither those face shades.  Surface-wave strips and
finale blades likewise use their original 256-entry ramp indices.  These
indices are passed unchanged to `p5_emit_color_if_needed`, where the selected
profile converts them from `pegc_palette_grb` to RGB332 or directly to
`G6/R5/B5`; no 16-colour slot is used as an intermediate colour.

## Backend invariant

The original NEON4 low-colour helpers use `DI` as a private flag while the
scene is building raster assets.  The P5 SGP command cursor is stored in
`p5_list_offset` instead.  This prevents `n4_story_raster_panel` from writing
command words to offset 1 of the payload and was required for the scene to
reach the normal ESC wait path.

The same register ownership rule also applies to the RASTER TRANSFER CAT
span tables.  `low_draw_cat48` and
`low_draw_cat48_200_overlay` keep their source-table cursor in `DI` while
walking each row.  The SGP span and line emitters use `DI` internally as their
command-list cursor, so they now save and restore the caller's `DI` on every
entry.  Before this correction the first CAT span was valid, but the next
source pair was read from the SGP command list; the resulting direct-colour
image corruption was therefore data-cursor clobbering, not a bad CAT table or
the RGB conversion.  The CPU 4bpp path did not expose the fault because its
endpoint writer already preserved `DI`.

## Verification

The following local checks were completed:

* `demos/neon4/build_p4.sh sgp ...` — PASS (P4 regression payload builds).
* `NEON4_P5_BPP=8 demos/neon4/build_p5.sh ...` — PASS (full-scene payload).
* `NEON4_P5_BPP=16 demos/neon4/build_p5.sh ...` — PASS (full-scene payload).
* Long VAEG headless run with the 8bpp payload — PASS; the script completed
  without a guest hang and the screen dump contained non-black scene output.
* Long VAEG headless run with the 16bpp payload — PASS; the script completed
  without a guest hang and the screen dump contained non-black scene output.
* Static dispatch audit — PASS: `scene_routines` contains all eight original
  entries, each source scene length is 384, and `TOTAL_FRAMES` is 3072.
* Loop audit — PASS: both profile listings reset `frame_counter` and branch to
  the frame label after the `TOTAL_FRAMES` comparison; ESC remains checked on
  every iteration.
* A temporary one-span check — PASS: a logical span x=100..500, y=100 is
  centered at the expected physical row after the 320-byte pitch correction.
* CAT cursor audit — PASS: both `p5_emit_span_physical` and
  `p5_emit_line_physical` preserve `DI`; the 65536 profile builds with the
  CAT table cursor intact across every emitted span.

The current payload SHA-256 is recorded outside the repository with the D88
capture because generated disk images are distribution artifacts, not source
inputs.

The captures are kept outside the repository under `/private/tmp` because
generated disk images and screenshots are not repository artifacts.

## Not yet verified

* exact 1-pixel endpoint writes for packed 8bpp spans;
* CPU-reference/SGP pixel equality for the complete scene;
* 640x200 variant;
* OPNA integration;
* real PC-88VA hardware.
