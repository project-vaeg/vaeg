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

# M98 - Zundamon billboard-orbit demo master plan

Status: **G98a, G98e, G98j, G98k, G98l, G98o, G98p, G98q, G98r, G98s, G98t, G98v, G98w, G98x, G98y, G98z, and G98aa human gates and G98b-G98d, G98f-G98i, and G98u machine gates passed; M98aa closed; M98m/M98n remain absorbed reservations**

Branch family: `topic/m98-zundamon-orbit`

Commit prefix: `M98<stage>:`

## 1. Intended result

Build one isolated PC-88VA/VAEG demonstration named **Zundamon orbit** that:

1. accepts an explicitly supplied local 32-bpp BMP and matching 16-entry
   RGB888 palette through a generic, source-neutral interface;
2. converts the accepted image to VA 8-bpp `GGGRRRBB`, reserving byte `00h`
   for transparency;
3. generates exactly 30 deterministic nearest-neighbor scale levels;
4. stores one shared scale atlas in exactly one PC-88VA I/O Bank Memory (BMS)
   bank;
5. lets the SGP read the selected 128 KiB BMS window directly and
   transparently BITBLT frames into double-buffered Graphic 1;
6. displays a 320x200 scene over a static Graphic 0 background;
7. moves camera-facing images around a deterministic 64-phase ellipse;
8. reuses one atlas for 1 through 16 instances, drawn far to near; and
9. offers one animation update per 1 through 8 VBLANKs, with publication
   synchronized to VBLANK.

This is a billboard effect. It does not claim genuine model rotation or
multiple viewing angles.

## 2. Fixed architecture

| Item | M98 decision |
|---|---|
| Logical display | 320x200 |
| Pixel format | VA 8-bpp direct color, `GGGRRRBB` |
| Transparent value | `00h` only |
| Background | Static nonzero Graphic 0 |
| Billboard layer | Transparent Graphic 1 |
| G1 source surface | 320x400, 320-byte pitch |
| G1 page A | SGP `0220000h`, DSA1 `0020000h` |
| G1 page B | SGP `022fa00h`, DSA1 `002fa00h` |
| Transparent copy | SGP BITBLT mode `0105h` |
| Atlas storage | Exactly one 128 KiB I/O Bank Memory window at `80000h-9ffffh` |
| BMS default port | `01d0h`, overridable by guest option |
| Atlas source maximum | 98x128; this exact maximum frame is preserved |
| Scale levels | Exactly 30, smallest through full size |
| Runtime scaling | None |
| Orbit phases | 64 deterministic lookup-table entries |
| Active instances | 1-16; default 1 |
| Draw order | Far to near; instance ID breaks ties |
| Count controls | `/N1`-`/N16`; DOWN decreases, UP increases |
| Rate controls | `/V1`-`/V8`; LEFT faster, RIGHT slower |
| Other controls | SPACE pauses, ESC restores and exits |
| Page clearing | Full-clear baseline, then per-page dirty-row unions |
| Local payload | `ZUNDORB.BIN`, never tracked or distributed |
| Public guest | `ZUNDORB.COM`, source-built and below 64 KiB |

Every implementation stage must recheck the live BMS bank semantics and SGP
source mapping. The CPU must not change the selected BMS bank while the SGP is
busy, and the guest must restore the ordinary mapping on every exit path.

## 3. Public/private boundary

Tracked and distributable material is limited to source, build scripts,
schemas, validators, documentation, and a deterministic synthetic fixture.
The fixture is an abstract asymmetric face-like marker and is not a depiction
of the named demo subject.

Maintainer-supplied images, palettes, manifests, generated atlases, local disk
images, screenshots, traces, and save states stay outside Git. Tracked text
must not record their filenames, absolute paths, hashes, provenance, or other
identifying metadata. Inputs must be provided through explicit paths outside
tracked directories. M98 neither obtains nor identifies source artwork.

The public pipeline is source-neutral. The maintainer is responsible for
supplying inputs they may lawfully use and for deciding whether any resulting
media may be published.

## 4. Repository layout

Introduce paths only in the stage that owns them:

```text
demos/zundamon-orbit/
  README.md
  build.sh
  build-local-d88.sh
  256/
    zundamon_orbit_256.asm
  tools/
    build_zundamon_orbit_asset.py
    inspect_zundamon_orbit_asset.py
    generate_orbit_table.py
    test_zundamon_orbit_asset.py
```

Generated output belongs below `build/generated/zundamon-orbit/`. Private
inputs must not be copied into the source tree. The guest does not use
`incbin` for the local atlas.

## 5. Milestone sequence

Execute exactly one assigned stage and stop at its gate.

| Stage | Single concern | Gate |
|---|---|---|
| M98a | Reserve the generic namespace and privacy boundary | Human |
| M98b | Add the deterministic public fixture and scaffold | Machine |
| M98c | Freeze the generic local-input manifest and schema | Machine |
| M98d | Validate indexed pixels and the 16-entry palette | Machine/local |
| M98e | Approve crop, transparency, and anchor | Human/local |
| M98f | Convert opaque colors to VA `GGGRRRBB` | Machine |
| M98g | Generate exactly 30 deterministic scale levels | Machine |
| M98h | Freeze the atlas format and fail-closed inspector | Machine |
| M98i | Pack complete frames into 128 KiB BMS banks | Machine |
| M98j | Run the complete local host-asset pipeline | Human/local |
| M98k | Bring up the isolated 320x200 8-bpp guest | Human/VAEG |
| M98l | BMS atlas streaming and direct G1 proof; absorbs former M98m/M98n scopes | Human/VAEG |
| M98m | Reserved - absorbed into M98l; no separate execution | Reserved |
| M98n | Reserved - absorbed into M98l; no separate execution | Reserved |
| M98o | Add transparent G1 double buffering - G98o passed | Human/VAEG |
| M98p | Visit all 30 scales with a full-page-CLS baseline - G98p passed | Human/VAEG |
| M98q | Add page-local dirty-row clearing - G98q passed | Human/VAEG |
| M98r | Add VBLANK cadence selection and telemetry - G98r passed | Human/VAEG |
| M98s | Add a constant-size 64-phase ellipse - G98s passed | Human/VAEG |
| M98t | Couple orbit depth to the 30-level atlas and add the FPS/ZUNDAMON G0 HUD - G98t passed | Human/VAEG |
| M98u | Generate deterministic 1-16 instance state and far-to-near order - G98u passed | Machine |
| M98v | Add the multi-instance full-clear baseline | Human/VAEG |
| M98w | Add multi-instance dirty-row interval unions | Machine + human |
| M98x | Add 1-16 runtime controls, HUD count, and load telemetry | Human/VAEG |
| M98y | Integrate and tune the approved local image - G98y passed | Human/local |
| M98z | Add bounded interactive orbit/camera controls and final QA | Machine + human |
| M98aa | Extend the private IDA profile to bounded 1-64 runtime instances with deterministic speed/distance animation | Machine + human |
| M98ab | Finish documentation and the final gate | Human |

M98k produced automated VAEG candidate
`6ed575dedc5da8827704c33a274ac72e480ce420` on 2026-08-31. Its exact indexed
GVRAM oracle passed, and the maintainer explicitly passed G98k on 2026-08-31.
M98k is closed. M98l combined the BMS mapping, bounded atlas streaming, and
direct BMS-window-to-G1 proof. Its three internal gates passed against VA2
candidate `228f31eb192c2722862691067c46c4db9e4aeb95`, and the maintainer
explicitly passed G98l on 2026-08-31. M98l is closed. M98m and M98n remain
reserved identifiers absorbed into M98l. M98o's automated transparent-G1
double-buffer candidate `ddc70c692ecb65066269c9894eb4b14f702fd2d9`
passed in VA2 mode, and the maintainer explicitly passed G98o on 2026-08-31
after confirming the displayed marker and ESC restoration. M98o is closed.
M98p implemented the 30-scale full-page-CLS zoom baseline without repeating
the atlas-streaming scope already completed by G98l-B. Its implementation
candidate is `4e9c57975a2e3705bc7cb2c29b3b94e5b88f4bea`; two opposite-parity bounded
VA2 runs passed the exact indexed oracle. After confirming the expected
single-marker shrink-and-grow behavior, the maintainer explicitly passed
G98p on 2026-08-31. M98p is closed.
M98q replaces only the steady-state full-page clear with independent
per-physical-page dirty-row clearing for the one homogeneous G1 object. M98p
remains the byte-correct golden. Its implementation candidate is
`6a3f229c74d1ffed9888b279e80334ac76d2e461`; all four 116-publication
`A/full`, `A/dirty`, `B/full`, and `B/dirty` VA2 comparisons passed with zero
framebuffer mismatches. The maintainer explicitly passed G98q on 2026-09-01
after confirming no stale image, horizontal one-pixel streak, anchor wobble,
or flicker and successful ESC restoration. M98q is closed. M98r adds only
selectable VBLANK cadence and is closed at accepted head
`4c5a7724e31cc0a52c8bfe8e827198c1c30a8c37`. The maintainer explicitly
passed G98r on 2026-09-01. M98s added only a constant-size, fixed-scale-15
64-phase screen ellipse. The maintainer explicitly passed G98s on 2026-09-01,
so M98s is closed. M98t couples the same 64 phases to all 30 stored scales and
adds the bounded FPS/ZUNDAMON information panel on G0. Its automated VA2/VAEG
evidence passed, and the maintainer explicitly passed G98t on 2026-09-01.
M98t is closed. M98u freezes only the deterministic 1-16 instance state and
far-to-near ordering contract. Multi-instance drawing, clearing, and controls
remain assigned to M98v, M98w, and M98x respectively. The exhaustive host
matrix, independent validator, count-one compatibility, deterministic rebuild,
and unchanged M98t guest checks passed on 2026-09-01. G98u passed and M98u is
closed. M98v consumes that exact state for the five build-time counts
1/2/4/8/16 and uses one complete hidden-page clear before every ordered draw
list. Its automated evidence passed on 2026-09-01, and the maintainer passed
G98v after the count-four visual and ESC-restoration check. M98w replaces only
the steady-state clear with page-local dirty-row interval unions; its
automated evidence and count-four visual/ESC-restoration gate are passed.
M98x adds one runtime count binary with `/N1` through `/N16`, bounded UP/DOWN
requests, publication-synchronous HUD count updates, and host transition/load
evidence. The normal default remains four. Its automated evidence and VA2
visual gate are passed. M98y integrates the approved private IDA profile and
its human gate is passed. M98z adds bounded A/Z speed, Q/E distance, W/S look,
and O/P radius controls while preserving the public/private renderer contract;
its automated evidence is complete and the maintainer passed G98z. M98aa
extends only the private IDA profile to 1..64 instances and a bounded
1.00X..4.00X animation. The maintainer explicitly passed G98aa on 2026-09-02,
so M98aa is closed. The later 128-instance extension remains separate.

## 6. Deterministic host contracts

Palette conversion uses a declared 16-entry RGB888 input. Source value zero
is transparent. Opaque colors must match a visible entry exactly unless
an explicit diagnostic-only nearest-color option is requested. Conversion to
VA direct color is:

```text
red3   = (red8   * 7 + 127) // 255
green3 = (green8 * 7 + 127) // 255
blue2  = (blue8  * 3 + 127) // 255
va8    = (green3 << 5) | (red3 << 2) | blue2
```

An opaque result that quantizes to zero must be repaired to the nearest
nonzero value with deterministic tie breaking. Before scale generation, a
source larger than 98x128 is fitted within that bounding box with deterministic
center-sampled nearest-neighbor selection. The anchor uses the same
pixel-center projection, an input that already fits is unchanged, and
upscaling is forbidden. A 98x128 source remains the exact maximum frame. Scale
generation then uses:

```text
numerator(i) = i for i=1..29, and 31 for i=30
width(i)     = max(1, (source_width  * numerator(i) + 15) // 31)
height(i)    = max(1, (source_height * numerator(i) + 15) // 31)
pitch(i)     = (width(i) + 3) & ~3
```

for `i=1..30`, deterministic center-sampled nearest-neighbor selection,
zeroed row padding, 16-byte frame alignment, an exact full-size level 30, and
a complete atlas contained in one 128 KiB bank. Omitting the `30/31` scale
slot retains 30 descriptors while preserving the exact 98x128 maximum. The
maximum-bound atlas occupies 127456 of 131072 bytes after row and frame
alignment.

## 7. Guest correctness contracts

- Load only a small header and descriptor table into conventional memory;
  stream payload through a bounded staging buffer.
- Validate lengths, geometry, bank ranges, padding, and CRCs before graphics
  mode.
- Keep the selected BMS bank stable until every dependent SGP list completes.
- Render a complete hidden page before publishing it on a low-to-high VBLANK
  edge.
- Maintain independent dirty state for both Graphic 1 pages.
- At multiple-instance stages, clear the complete union of the hidden page's
  old row intervals before drawing all new instances far to near.
- A missed animation slot retains the old published page and increments
  telemetry; it never exposes a partial frame.
- Restore video, keyboard, files, staging memory, and ordinary BMS mapping on
  every success and failure exit.

## 8. Final evidence boundary

Public-fixture, local-private, VAEG, and physical-machine claims are reported
separately. Automated fixture success cannot pass a human visual gate. VAEG
timing is not physical-machine throughput evidence. Unless physical testing
is actually performed, the final status remains `REAL_HW_PENDING`.
