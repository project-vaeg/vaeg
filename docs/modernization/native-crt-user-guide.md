<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# Native CRT user guide

Native CRT is an optional presentation mode for the SDL2 frontend. It uses
Metal on macOS, D3D11 on Windows, and OpenGL on Linux. It does not change the
emulated machine or the deterministic guest-frame capture path.

## Build and enable

Windows menus follow the monitor DPI automatically; there is no UI-size menu.
If necessary, set `GUI_ui_scale=200` in `vaeg.cfg` before startup;
`GUI_ui_scale=0` restores automatic sizing. This scales the GUI, not the raw
guest framebuffer, and works with both SDL and native CRT rendering.

Build with the optional feature enabled (it is enabled by default):

```sh
cmake --preset linux-debug -DVAEG_ENABLE_LIBRASHADER=ON
cmake --build build/linux-debug --target vaeg_sdl2
```

On Windows, open `画面 > 描画方式` and select `標準（SDL）`,
`CRT効果（librashader）`, or `加工なし（librashader）`. This switches
the renderer immediately, including a real return to SDL. Selecting librashader
again retries initialization. The menu remains visible and is drawn after the CRT effect at the
same logical font size as the SDL version. SDL exposes `エフェクト`; librashader
exposes `CRT設定…`, a separate window for preset paths, reload,
parameter sliders and reset. Scaling, window size and aspect correction remain
shared. Switching renderers retains the SDL effect and saved shader settings.
`Window size` offers Native, x2, x3 and x4. `Custom...` accepts an integer
multiplier from 1 to 16, or explicit pixel dimensions. The multiplier applies
to the guest display area; the menu height is added separately. It is saved as
`GUI_scale` and works with both SDL and native CRT presentation. Large sizes
may be constrained by the desktop/window manager and increase GPU memory use.
The selection marks actual renderer ownership. Normal menu status text is
hidden; failures and the unfiltered librashader state remain visible. The CRT
settings window also reports the current filter status. `加工なし（librashader）`
presents the raw image through the native presenter without CRT or SCREEN_SIZE
padding, while retaining the same preset and parameter values. Displayed
screenshots still include any enabled information overlays; canonical raw QA is
unchanged.
The bundled default preset provides `SCREEN_SIZE` (Screen size (%)) in
`CRT設定…`: 80–120%, initially 98.00%, with 0.01% steps. Its default `CURVATURE`
is 0.030. Saved settings still take precedence; use Reset to apply these defaults.
The RGB mask has an automatic low-resolution profile enabled by default:
`MASK_INTENSITY` is 0.15 at x1, 0.20 at x2, and 0.30 at x3 or larger. This
reduces RGB-mask aliasing at small window sizes without blurring the framebuffer.
The CRT settings window can disable automatic adjustment for manual control;
that choice is saved as `NativeCRTAutoMask` in `vaeg.cfg`.
CRT parameter values live in `NativeCRTParameters` in the active `vaeg.cfg`
(or the file selected with `--cfg`). Changes and reset are saved on normal exit,
along with other frontend settings. The old `vaeg-crt-parameters.cfg` is neither
read nor written; no migration is performed. With no integrated values, preset
defaults apply. `--no-cfg` disables loading and disk saving,
while live adjustments remain available for the session.
Below 100%, the frontend centers the original pixels in a larger black canvas,
without resizing or blending them. The copy pass uses exact texel fetches;
the subsequent CRT shader retains its own filtering and curvature.
The default now averages only scanline brightness over each output pixel.
At four or more output pixels per source row, thin scanlines remain visible;
between four and two their contrast fades, and at two or fewer the periodic
dark gaps disappear. This suppresses low-resolution moire without adding a
whole-image blur. RGB mask stripes are anchored to actual render-target pixel
centers, independently of the inset viewport and window dimensions. This
prevents letterboxing from compressing the stripe pattern into vertical bands;
the image is not blurred. Mask/content interference can still occur with
particular guest patterns or scaling applied outside VAeg. Curvature is
included in the local sampling-width estimate.
Canvas dimensions preserve aspect ratio and symmetric integer margins, so
the effective size changes in discrete steps despite the fine slider steps.
For 640x400 input, 98.00% uses a 656x410 canvas (effective 97.56%, margins
8x5); 80% uses 800x500 (margins 80x50). At 100% the original buffer is
borrowed unchanged. Above 100%, centered integer cropping enlarges the image.
This operates only on the presentation input, before the existing CRT shader,
so curvature may reshape the borders. It does not guarantee integer-scaled
physical display pixels after CRT distortion and final output scaling.
Normal displayed screenshots include this result and enabled information
overlays. Unprocessed screenshots and canonical raw QA captures are unaffected
by this control. Custom presets without SCREEN_SIZE are unchanged; that name
now identifies this frontend padding control. Update the package's `assets/`
directory and executable together: mixing old and new versions can apply
sizing twice or omit it entirely.

Preset reload and parameter sliders
also apply to the active filter. On macOS/Linux, enablement still requires a
restart and native GUI integration remains pending.

DLL presence alone does not enable CRT. The Windows package includes
`start-native-crt.cmd` to request CRT for that run even if `vaeg.cfg` has it
disabled. The launcher starts in the package directory and writes diagnostics
to `native-crt.log`. It sets the session-only `VAEG_NATIVE_CRT=1` environment
override; `VAEG_NATIVE_CRT=0` requests SDL at startup instead. The window title
shows `Native CRT ON`, unfiltered librashader, or the fallback status. A
missing runtime/preset or failed shader preserves the native unfiltered image;
failure of the native device returns to SDL.

On Windows, Video info and Framebuffer info overlays also work in CRT mode.
They are drawn after filtering, behind GUI menus, without changing raw captures.

### Windows DLL prerequisites

The pinned official x64 `librashader.dll` imports `D3DX9_43.dll`, `MSVCP140.dll`,
`VCRUNTIME140.dll`, and `VCRUNTIME140_1.dll`. These are not bundled in this ZIP
and DLL presence alone does not prove its dependencies can load. If the menu
reports a missing/unloadable dependency, install the appropriate official
Microsoft runtime, then select librashader again:

- [DirectX End-User Runtimes (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=8109)
  for `D3DX9_43.dll` (run the extracted `DXSETUP.exe`). Modern DirectX alone
  does not supply these legacy helper libraries.
- [Visual C++ Redistributable, x64](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
  for the `*140*.dll` imports.

Do not download individual DLLs from unofficial DLL sites. VAEG does not
automatically download or install these prerequisites. `native-crt.log` records
the Windows loader error and unavailable dependency names; the menu also shows
preset and filter-chain errors instead of only a generic unavailable label.

The rendering mode is stored in `vaeg.cfg`: `NativeCRT=0` selects standard
SDL rendering, while `NativeCRT=1` selects the native librashader presenter.
When the native presenter is selected, `NativeCRTFilter=1` enables the CRT
effect and `NativeCRTFilter=0` selects unfiltered librashader output. The
default preset is:

```text
assets/shaders/crt/vaeg_crt_default.slangp
```

The bundled default is found beside the executable, with a working-directory
fallback for development builds. Custom relative preset paths are resolved
from the working directory. A custom preset is used only when it is supplied by
the user; it is not copied into or downloaded by VAEG.

## Release package layout

Start VAEG from the package directory so the default relative asset path is
available:

```text
vaeg                         (or vaeg.exe; static librashader API embedded)
assets/shaders/crt/vaeg_crt_default.slangp
assets/shaders/crt/shaders/crt-lottes-fast.slang
licenses/...
```

M99 release binaries statically link the pinned librashader C API and do not
require `librashader.so`, `librashader.dylib`, or `librashader.dll`. The
release staging helper checks the shader, preset, license, and provenance
hashes before packaging. Shared runtimes remain supported only for explicit
non-release developer builds.

On Linux or macOS, if a development environment does not search the package
directory for a neighboring dynamic library, add that directory to the
platform loader path for the run:

```sh
LD_LIBRARY_PATH="$PWD" ./vaeg       # Linux
DYLD_LIBRARY_PATH="$PWD" ./vaeg     # macOS development run
```

Windows uses the application directory for `librashader.dll` under the normal
DLL search rules. Do not rename a runtime across platforms or architectures.

## Captures

`画面 > 全画面表示` is a single checked toggle: check it to enter Exclusive
fullscreen, uncheck it to return to the window. No return button is added to
the menu bar, and Esc remains available to the guest.
On entry, `画面上端でメニュー表示` appears for up to three seconds while the
menu is hidden. This non-interactive hint is excluded from screenshots.

In Exclusive fullscreen, the menu normally hides. Move the mouse to the top
edge (12 DPI-scaled logical pixels) to reveal it over the image. It stays visible
while the pointer is over the bar or a popup/item is being operated, and hides
after 0.5 seconds away. Windowed mode keeps the bar visible. Fullscreen never
reserves a menu strip, so revealing the bar does not resize or shift the guest
viewport. Information overlays remain beneath the menus. If guest relative
mouse capture is enabled, release it with the existing capture toggle first.
Fullscreen screenshots retain the full drawable image without a menu-strip
crop, but still omit menus and dialogs.

`画面 > スクリーンショットを保存` (the first menu entry), PrintScreen and
the configured F12 screenshot binding save the next composed display frame:
SDL effects or Windows D3D11 librashader output, scaling/aspect correction,
and Video info / Framebuffer info exactly as enabled. Menus, dialogs and the
top menu strip are excluded; the remaining drawable area, including letterbox
margins, is retained. Readback and PNG encoding occur only on request and may
briefly stall presentation. Failure is reported rather than substituting raw
pixels. Native Metal output readback is supported; native OpenGL output
readback is not yet supported by this path. Native GUI integration remains
platform-specific.

`スクリーンショットを保存（加工前）` saves the guest-resolution image without
display effects or scaling. Video info and Framebuffer info are included
independently only when their respective display toggles are on, just as for
normal screenshots. With both off, no analysis panel is added. Deterministic
CLI raw capture remains separate and does not inherit these display toggles.

Use `--screenshot FRAME:PATH` or `--screen-tvram-dump PATH` for deterministic
guest-frame evidence. `--screen-dump` captures the SDL rendered target and is
not available while Native CRT owns the output; VAEG reports that limitation
instead of labeling a filtered GPU image as a deterministic capture.

## Disable or customize

Select `標準（SDL）` in the rendering-mode menu to return to SDL immediately.
For a one-run SDL fallback diagnostic, use `VAEG_NATIVE_CRT=0`,
or set `NativeCRT=0` in the current `vaeg.cfg` before
starting. To use another preset, set `NativeCRTPreset` in the configuration or
enter its path in `CRT設定…`, then reload the preset. The Windows renderer applies
the reload between frames; macOS/Linux still require a restart.

The default preset combines a VAeg-owned BSD-2-Clause size pass with the
audited Unlicense/public-domain CRT shader. Other
shader files remain user-provided and are outside the VAEG release payload.
See the [troubleshooting guide](native-crt-troubleshooting.md) when the native
path falls back.
