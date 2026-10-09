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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M21 - SDL2 display effects, resizable viewport, and fullscreen

Status: complete; G21 passed

Branch: `topic/m21-sdl2-display-effects`

Gate: G21 human display and guest-regression gate

Depends on: G20 passed.

## Decision

M21 keeps the existing SDL_Renderer and Dear ImGui SDLRenderer2 backend. It
does not add bgfx, bx, bimg, a shader compiler, or another graphics API. This
replaces the earlier bgfx proposal with a smaller dependency-free milestone.

The SDL_Renderer abstraction has no portable custom-shader API. M21 therefore
implements only effects that can be composed from SDL textures, blending,
color modulation, render targets where available, and destination geometry.
The implementation must remain useful on the software renderer and must not
depend on a particular SDL OpenGL, Metal, or Direct3D implementation.

## Goals

- Preserve the current SDL_Renderer backend and single-presenter main loop.
- Make the SDL window resizable by dragging its edges.
- Add numeric custom logical window sizes.
- Use one tested viewport calculator for all scaling and display modes.
- Distinguish logical window size, drawable output size, menu inset, and the
  fixed guest framebuffer size.
- Add immediate Windowed and current-desktop Exclusive fullscreen modes.
- Add Unfiltered, Linear, Scanline, and CRT Lite display effects.
- Preserve the existing 640x400 RGB565 guest shadow framebuffer and upload
  path.
- Keep default behavior compatible with the current SDL2 windowed display.

## Non-goals

- No bgfx, bx, bimg, Vulkan, custom OpenGL, Metal, or Direct3D code.
- No source or assets from MAME's GPL renderer or shader chains.
- No custom shader compilation or runtime shader assets.
- No gamma-correct beam simulation, bloom, phosphor persistence, LUTs,
  texture-based MAME masks, or multi-pass CRT emulation.
- No curved-screen mesh in M21. A later milestone may evaluate
  `SDL_RenderGeometry` after the rectangular viewport and input transforms are
  established.
- No guest framebuffer reallocation for host window or fullscreen changes.
- No changes to guest video timing, TSP, VRAM, frame pacing, or draw-skip
  behavior.
- No Borderless desktop or detailed monitor/resolution/refresh GUI in the
  simplified final M21 interface.
- No edits to `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, or `hlp/`.

## Existing presentation path

The current active path is:

```text
guest draw
  -> scrnmng_surflock()
  -> persistent 640x400 RGB565 shadow buffer
  -> scrnmng_surfunlock()
  -> SDL_UpdateTexture()

main loop, once per presented host frame
  -> scrnmng_present_begin()
  -> clear and SDL_RenderCopy() guest texture
  -> gui_render()
  -> ImGui_ImplSDLRenderer2_RenderDrawData()
  -> scrnmng_present_end()
  -> SDL_RenderPresent()
```

`scrnmng_surfunlock()` must not present. The main loop remains the only
presenter. M21 may move calculations and draw helpers but must preserve this
ownership rule.

## Common viewport

Add a frontend-only C99 viewport helper under `sdl2/`. It must not include SDL
or C++ types so ROM-less selftests can exercise it directly.

Inputs:

- guest width and height;
- drawable/output width and height;
- menu bar inset in drawable pixels;
- logical-to-drawable scale;
- scaling mode;
- aspect-correction setting.

Outputs:

- destination x, y, width, and height in drawable pixels;
- horizontal and vertical guest-to-drawable scale;
- inverse drawable-to-guest transform;
- whether a point lies inside the guest viewport.

Scaling modes:

| Mode | Behavior |
|---|---|
| Native | Center the guest at native size without scaling. |
| Fit | Preserve the selected aspect and fill the content area. |
| Fit 8-dot | Fit while rounding destination width down to an 8-pixel boundary. |
| Integer | Use the largest whole-number scale that fits and center it. |
| Stretch | Fill the content area without preserving aspect. |

The menu bar is outside the guest viewport. Letterbox and pillarbox regions
remain black. A point outside the viewport must not be clamped into guest
coordinates.

The calculator must handle zero drawable dimensions, minimized windows,
minimum sizes, and High-DPI scaling without division by zero.

## Window resize

- Create the SDL window with `SDL_WINDOW_RESIZABLE` and
  `SDL_WINDOW_ALLOW_HIGHDPI` where supported.
- Keep the current Native/x2/x3 actions as logical window-size presets.
- Add `Custom size...` with pending integer width and height and Apply/Cancel.
- Apply custom size with `SDL_SetWindowSize()` and process the resulting SDL
  window event through the normal resize path.
- Set a minimum logical window size that leaves a usable menu and positive
  content area.
- Do not recreate the guest texture solely because the host window changed.
- Coalesce duplicate size events and skip rendering while drawable size is
  zero.
- Store only the windowed logical size, never a High-DPI drawable size or a
  temporary fullscreen size.

## SDL2 effects

The Screen menu exposes:

| Effect | SDL2 implementation |
|---|---|
| Unfiltered | Guest texture uses `SDL_ScaleModeNearest`; no overlay. |
| Linear | Guest texture uses `SDL_ScaleModeLinear`; no overlay. |
| Scanline | Linear guest sampling plus a procedural transparent black line overlay aligned to guest scanlines. |
| CRT Lite | Linear guest sampling plus procedural scanline, restrained RGB mask, and edge-darkening overlays. |

All overlay data is generated by first-party code at runtime. No MAME code,
mask texture, LUT, artwork, or shader is copied. Overlay textures are limited
to the guest viewport and must not affect letterbox or pillarbox regions.

Scanline period is based on the 400-line guest raster and current destination
height. Resize must not change which guest rows receive scanline attenuation.
The RGB mask is intentionally light and may be disabled automatically when
the output is too small for a stable pattern. CRT Lite parameters remain
fixed in M21; no advanced tuning panel is added.

If the SDL renderer cannot create a required target or overlay texture, log
the reason and fall back to Linear without terminating emulation.

## Fullscreen

The public GUI has two immediate display modes: Windowed and Exclusive
fullscreen. Exclusive uses the current desktop resolution on the saved
monitor, with no pending selection or separate Apply step.

Before entering fullscreen, retain the window position, logical size, and
maximized state. Restore them when returning to Windowed. Re-query drawable
size after every transition before recalculating the viewport.

Invalid display indices, disconnected displays, missing modes, dummy video
drivers, and failed exclusive transitions must not crash. Fall back to
Windowed and log the reason.

Alt+Enter is not required for M21. Complete the GUI-driven lifecycle first.

## Legacy fullscreen settings

Add the following SDL2 frontend keys to `vaeg.cfg`:

- `fscrn_cx`: requested exclusive output width;
- `fscrn_cy`: requested exclusive output height;
- `fscrnmod`: hexadecimal drawing-mode bits.

`fscrnmod & 0x03`:

- `0x0`: Native;
- `0x1`: Fit 8-dot;
- `0x2`: Fit;
- `0x3`: Stretch.

Bit `0x04` means use the current display size instead of changing display
mode. Values `0x00` through `0x07` are valid. Ignore unsupported upper bits,
emit a warning, and store the masked value.

When current-display bit `0x04` is set, zero dimensions use the selected
display's current mode. Otherwise `fscrn_cx=0` uses 640 and `fscrn_cy=0` uses
400.

The frozen Win32 frontend offered a `force400` choice between 400 and 480.
The active frontend is VA-specific with a canonical 640x400 canvas, so M21
does not add `force400`; its zero-height compatibility default is 400.

The old 1280-wide RPI filter condition is documentation history only. No RPI
filter implementation exists in the active tree, and M21 does not expose one.

## Configuration

Add validated frontend settings using existing `vaeg.cfg` conventions:

- display effect;
- scaling mode;
- aspect correction;
- windowed logical width and height;
- display mode;
- monitor index;
- exclusive width, height, and refresh rate;
- `fscrn_cx`, `fscrn_cy`, and `fscrnmod`.

Existing configurations default to SDL2 windowed Fit behavior and the current
aspect setting. Unknown strings or out-of-range numeric values fall back to
safe defaults. Fullscreen drawable dimensions must never overwrite stored
windowed dimensions.

## GUI

Screen menu additions:

- Effect: Unfiltered / Linear / Scanline / CRT Lite;
- Scaling: Native / Fit / Fit 8-dot / Integer / Stretch;
- Window size: Native / x2 / x3 / Custom...;
- Windowed and Exclusive fullscreen as immediate commands;
- Aspect correction.

Custom window size uses pending values. Exclusive fullscreen does not require
a setup dialog; failed switching restores Windowed and logs the reason.

## Automated tests

Extend ROM-less selftests for:

- 640x400 source into 640x400, 800x600, 1024x768, 1280x720, and 1920x1080;
- Native centering;
- Fit aspect preservation;
- Fit 8-dot destination width divisibility;
- Integer scale and centered margins;
- Stretch filling the content area;
- menu inset;
- zero/minimized drawable dimensions;
- High-DPI 2x conversion;
- inverse mouse transform and outside detection;
- `fscrn_cx/fscrn_cy` zero fallback;
- `fscrnmod` values `0x00` through `0x07` and upper-bit masking;
- config validation and persistence;
- effect selection and unsupported-effect fallback.

Run:

```text
cmake --preset linux-debug
cmake --build --preset linux-debug --target vaeg_sdl2
SDL_AUDIODRIVER=dummy ./build/linux-debug/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --smoke

cmake --preset linux-ci-gcc
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure

cmake --preset linux-ci-clang
cmake --build --preset linux-ci-clang
ctest --test-dir build/linux-ci-clang --output-on-failure

cmake --preset linux-ci-asan
cmake --build --preset linux-ci-asan
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan \
  --output-on-failure

cmake --preset mingw-cross
cmake --build --preset mingw-cross

python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
git diff -- win9x i286x cpuxva/memoryva.x86 hlp
```

Native macOS build and display verification remain maintainer checks when the
agent host has no Darwin SDK.

### Agent results

The Linux agent completed the following checks on 2026-07-11:

- Linux Debug configure/build, ROM-less `--selftest`, and ROM-less `--smoke`:
  passed;
- Linux GCC CI configure/build/CTest: passed, 1/1 test;
- Linux Clang CI configure/build/CTest: passed, 1/1 test;
- Linux ASan/UBSan configure/build/CTest: passed, 1/1 test;
- MinGW cross configure/link: passed and produced `sdl2/vaeg.exe`;
- isolated temporary-config smoke checks: Windowed custom size with CRT Lite
  and Fit 8-dot passed; unavailable Exclusive mode returned safely to
  Windowed. The earlier Borderless UI was subsequently removed when the
  fullscreen controls were simplified.

Native macOS, native Windows, real multi-monitor/fullscreen operation, visual
effect quality, and guest regressions remain part of G21.

## Application icon follow-up

After G21 passed, the portable frontend restored the historical VAEG
application icon as an M21 packaging follow-up. The source asset is
`assets/vaeg.ico`, copied byte-for-byte from the frozen reference
`win9x/icons/np2.ico`; the frozen file itself was not edited. Both files have
SHA-256
`a27533f679a31fdb8e2812c1d4906e705e544ba49b976154dde6794ce31a32f4`.

CMake embeds the ICO as generated C data in every SDL2 executable.
`sdl2/appicon.c` decodes the embedded 32-bit ICO into an SDL surface and calls
`SDL_SetWindowIcon()` immediately after window creation. This supplies the
runtime window/task-switcher icon on Windows, Linux, and macOS without an
adjacent icon file.

Windows builds additionally compile `sdl2/vaeg.rc`, embedding the unchanged
ICO as native `RT_ICON` and `RT_GROUP_ICON` resources so Explorer can display
the icon before vaeg runs. macOS remains a plain executable rather than an app
bundle, so the running SDL application has the icon but Finder has no bundle
document icon. Linux similarly has a runtime window icon but no installed
desktop entry or icon-theme payload.

The icon follow-up was verified on 2026-07-13:

- `mingw-cross` configured and linked successfully;
- PE inspection found a `.rsrc` section, three `RT_ICON` entries, and one
  `RT_GROUP_ICON` entry;
- the MinGW executable retained only Windows system DLL imports;
- Linux Debug build, ROM-less selftest, and ROM-less smoke passed;
- encoding, EOL, case, diff, and frozen-tier checks passed.

The icon disappeared from one distributed M22 binary because the completed
icon topic commit had not been integrated into `main`. Main commit `2f14f5c`
restored the resource and runtime icon while retaining M22 archive-drop code.

## Gate G21

G21 is a human display and guest-regression gate. Verify:

- Windowed 640x400, 800x600, 1024x768, 1280x720, and 1920x1080;
- continuous edge-drag resize;
- Native/x2/x3/Custom window sizes;
- Native/Fit/Fit 8-dot/Integer/Stretch;
- aspect correction;
- Retina/High-DPI clarity and geometry;
- minimize, restore, maximize, and window-position restoration;
- Windowed -> Exclusive -> Windowed;
- Unfiltered/Linear/Scanline/CRT Lite appearance;
- stable scanline period while resizing;
- no effect leakage into black margins;
- correct GUI layout and input after resizing/fullscreen transitions;
- clean failure/fallback on an invalid display mode;
- V3 boot, VA demo, OS boot/simple operation, audio, keyboard, FDD/SASI
  retention, reset, state save/load, and clean exit.

G21 passes only when the maintainer explicitly reports it passed.

### Inherited VA1 defect found during G21

The Windows human gate found a pre-existing VA1-mode failure in PC-Engine
1.05 V3 BASIC. `BEEP`, `LIST`, and `FILES` can make the guest appear
frozen; `BEEP` may also produce text-screen garbage when sound is enabled.
The same workload succeeds in VA2/VA3 mode.

This is not presently attributed to M21:

- the failure reproduces in the original VAEG and in older comparison
  builds;
- both `clk_mult=1` and the correct VA setting `clk_mult=2` reproduced it;
- the expected VA1 ROM set was loaded and its logged CRC32/SHA-1 values
  matched the known images;
- Sound Off still froze, without the text garbage;
- removing BEEP PCM stream registration did not prevent the freeze;
- suppressing the BEEP event scheduled by system-port 01CFh did not prevent
  it;
- `LIST` and `FILES` reproduce without invoking BEEP.

Instruction tracing showed that the CPU was still executing rather than
hard-stopped. One captured interval repeatedly traversed RAM at
`19E3:B7E2-B846` and ROM0 bank 1 at `E800:0C62-0CC0`. The ROM routine uses
INT 83h services and reads text VRAM through `ES:DI`; this trace was armed
on Return and may represent output before the final stuck state, so it does
not yet identify the root cause. ROM-bank tracing found only valid ROM0
banks and was dominated by the normal VA1 timer interrupt.

Follow-up work should use a host-only trigger after the visible freeze,
capture full registers plus the RAM bytes containing the `19E3` routine,
and then identify the hardware status or return condition that never
changes. The temporary trace experiments are not part of the M21 product
changes. This defect remains open for a later focused fix.

## Later work

A future optional renderer milestone may reconsider bgfx or another explicit
graphics API for gamma-aware shaders, beam profiles, curvature, bloom,
phosphor persistence, and advanced masks. That work requires a separate ADR,
dependency/license review, shader toolchain, packaging plan, and gate.
