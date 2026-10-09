<!--
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->


# M99z10 — Renderer-specific screen settings

Starting commit: `c09d135f8a2f98ee1249cb06826e8e3a92339a17`.
Branch: `topic/m99-native-crt-rebuild`; unrelated main changes preserved.

## Changes

- Replace the CRT shader parent menu with direct SDL/librashader selection.
- Show SDL Effect only for actual SDL ownership; show Shader settings only
  for native ownership. Move preset path, reload, parameters and reset into
  a separate closable window. Shared scaling/aspect/window controls remain.
- Keep existing configuration and parameter persistence across renderer changes.
- Show actual renderer/filter status, including native pass-through, and
  retain creation failure details after the failed presenter is destroyed.
- No screenshot semantics, raw captures, guest timing, GPU algorithms, vendor
  dependencies or binary assets changed.

## Verification

Commands (from the task checkout):

```sh
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

Builds PASS. Both focused suites: 10/10 PASS. Repository checks: zero findings.
The controller test now injects one initialization failure into the passing
fake presenter and checks exact retained diagnostic text, then verifies a
successful retry clears it. Existing parameter persistence, capture boundary
and SDL window-rebind tests remain enabled.

Windows menu interaction/visual verification is deferred to the maintainer;
local cross-compilation and mock tests are not physical GPU evidence.
macOS/Linux native GUI integration remains outside this follow-up.

## Handoff

Implementation: [11bafaf5](https://github.com/nakatamaho/vaeg/commit/11bafaf5ede49c4bae1df7fc8316e754eb85e571).
Reconfigured/rebuilt MinGW after committing to embed that build identity.
Package: `build/mingw-cross/vaeg-m99z10-windows-x86_64.zip`, including the
unchanged runtime DLL, shaders, licenses and updated user guide.
Directory and ZIP package validators PASS; staged EXE matches the build via `cmp`.
ZIP SHA-256: `e469c2327c281cb7bf8c9e394b62ea87eca7c1eee95a292c38642c9668594fad`.
Final feature-on/off focused tests passed 10/10 each (8.49 s / 7.70 s).
Removed an extra report EOF blank line detected by the staged diff checker.

## M99z13 — Purpose-oriented renderer menu

Starting commit: `076e9eb4aa7b3b3d73e545de447992ec121b1301`.
The maintainer approved `画面 > 描画方式 > 標準（SDL） / CRT効果（librashader）`.
SDL shows `エフェクト` directly beneath the renderer group; native CRT shows
`CRT設定…`. The settings window keeps its existing ImGui identity. Hide normal
SDL/filtered status text in the menu, but retain failures, native pass-through
and restart-required notices. Screenshots remain the first two menu entries.
No configuration, renderer lifecycle, capture, shader or guest behavior changes.

Validation: the MinGW and macOS feature-on build commands listed above PASS;
the macOS feature-on focused ctest command passes 10/10 (6.07 s). Encoding,
EOL and case checks report zero; diff check clean. Formatting was limited to
the changed renderer-selection block. Windows manual menu verification is
deferred; no new physical GPU evidence is claimed.

Implementation: [e595f00e](https://github.com/nakatamaho/vaeg/commit/e595f00e7f2db7a12d34bb69f13bd0e886f1d7c3).
MinGW rebuilt with committed identity; staged EXE matches via `cmp`.
Package: `build/mingw-cross/vaeg-m99z13-windows-x86_64.zip` (DLL/assets/notices
included); package validator PASS.
ZIP SHA-256: `027f74e5e0cb75932530967c511f3516433c9d979ca04af13b4db894658daccd`.

## M99z14 — Exclusive fullscreen auto-hide menu

Starting commit: `c84c5491d95571bbdbe45930a4bc634f25b33d10`.
Exclusive fullscreen starts with a hidden menu. A 3-DPI-scaled-logical-pixel
top-edge hover reveals it; pointer-over-bar or popup/item interaction retains
it, otherwise it hides after 500 ms. Windowed mode remains always visible.
Relative guest mouse capture must first be released with the existing toggle.
The reveal area blocks guest mouse input to avoid click-through.

Fullscreen reserves zero menu inset regardless of visibility. SDL and native
CRT share this viewport policy, so menu visibility cannot resize or shift the
guest image. SDL information overlays now draw before ImGui (at most once per
frame), with a present-end fallback for non-GUI callers. This keeps a revealed
menu above information panels, matching native CRT composition. Screenshot
capture still omits GUI and now retains the entire fullscreen drawable area.

Focused tests exercise hidden entry, reveal, 499/500 ms timeout boundary,
popup/item retention, bar hover, mode exit/reentry, tick wrap, zero-inset crop,
and idempotent SDL overlay composition. The builds and focused ctest commands
listed above are reused. No GPU code, guest timing, raw golden capture or
configuration format changes. Real Windows exclusive-mode hover/input and
SDL/CRT screenshot checks remain deferred to the maintainer.

Results: MinGW and macOS feature-on/off builds PASS; focused suites 10/10
PASS each (6.27 s / 5.69 s). Encoding/EOL/case checks zero; diff check clean.

Implementation: [8fa87aea](https://github.com/nakatamaho/vaeg/commit/8fa87aea2711f91b2889cf8e5dac1243291ae3ba).
MinGW rebuilt with committed identity; staged EXE matches via `cmp`.
Package: `build/mingw-cross/vaeg-m99z14-windows-x86_64.zip` (DLL/assets/notices
included); package validator PASS.
ZIP SHA-256: `608c9f2df23adb13e6c33916dcf3b811be0b8c10b7b902f67fa2dbe4dfb3cfa0`.

## M99z15 — Discoverable fullscreen toggle

Starting commit: `711bfc183880f246a3c7aa760a5162dec471279f`.
The maintainer confirmed Windowed restoration works; the provisional exit-path
changes were withdrawn without committing. This follow-up changes only GUI
discoverability: replace Windowed/Exclusive fullscreen with one checked
`全画面表示` item; show `画面上端でメニュー表示` for up to three seconds on
entry while the menu is hidden; widen the reveal zone from 3 to 12 DPI-scaled
logical pixels. No return button or Esc shortcut is added. The hint takes no
input and is skipped by the existing screenshot GUI-suppression path.

The existing menu-policy selftest additionally verifies the hint's 2999/3000 ms
boundary and immediate disappearance on leaving fullscreen. The previous
build/test/check commands apply. Windows physical UI verification is deferred.

Results: MinGW and macOS feature-on/off builds PASS; focused suites 10/10
PASS each (6.17 s / 5.84 s). Encoding/EOL/case checks zero; diff check clean.

Implementation: [db8c2a57](https://github.com/nakatamaho/vaeg/commit/db8c2a57b319369f8fe2a9edca7de250d2a94131).
MinGW rebuilt with committed identity; staged EXE matches via `cmp`.
Package: `build/mingw-cross/vaeg-m99z15-windows-x86_64.zip` (DLL/assets/notices
included); package validator PASS.
ZIP SHA-256: `5ea7f01ea36a1f6ae6c0e3a43c21edb4f788f6e2ac8656661ee91da77b549251`.

## M99z16 — Fit startup splash to drawable size

Starting commit: `5bcd66b9b91ca64d7e87341be556bcc29bb87c75`.
The SDL startup splash now fits the actual renderer output dimensions instead
of using the configured integer guest scale. Aspect ratio is preserved,
remaining margins are centered, and small outputs can downscale safely.
The embedded bitmap and nearest-neighbor sampling remain unchanged. This
does not add a splash to native-only startup paths that lack an SDL renderer.

Verification: `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` and
`CCACHE_DISABLE=1 cmake --build build/macos-ci -j4` PASS.
`ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'`
passes 2/2 (5.08 s), including new fit geometry checks for 640x422, 1280x844,
1920x1080, downscaling and zero output rejection. Encoding/EOL/case checks
zero; diff check clean. Physical Windows splash appearance remains untested.

## M99z17 — Two-thirds splash and native presentation

Starting commit: `8301422e62b38733b372a1d5e0acd61d7081416d`.
Fit the splash inside a centered box occupying two thirds of each drawable
dimension, preserving aspect ratio. Native startup previously returned early
because `splash_show` required an SDL renderer. It now converts the embedded
bitmap to ARGB8888 and submits it through the active native presenter with a
temporary splash viewport and filter disabled, then restores the filter and
guest viewport. The guest shadow and raw capture paths are not used or changed.
No vendor/backend code or bitmap payload is modified.

MinGW and macOS feature-on builds PASS using the commands above. The focused
macOS ctest suite passes 10/10 (6.33 s), including updated two-thirds geometry
checks and existing controller/filter/viewport tests. Encoding/EOL/case checks
zero; diff check clean. Actual native splash visibility remains untested locally;
the geometry and controller tests are not physical GPU evidence.

Implementation: [74e32878](https://github.com/nakatamaho/vaeg/commit/74e32878f70cb8809ab6306dd676927ad6225a8a).
MinGW rebuilt with committed identity; EXE-only handoff at
`build/mingw-cross/vaeg-m99z17-windows-x86_64/vaeg.exe` matches the build via `cmp`.

## M99z18 — CRT screen size, 80–120 percent

Starting commit: `908b0a7076992767b84773f197d59f830517d6e9` on
`topic/m99-native-crt-rebuild`. Scope: bundled default preset only.
The new independently authored BSD-2-Clause shader adds `VAEG_SCREEN_SIZE`
(80–120, default 100, step 1) through the existing runtime parameter UI and
persistence. Older parameter files leave the new value at its default.
No frontend, emulator, vendor shader, or capture implementation is changed.

Data flow: unchanged guest texture -> centered size pass at source resolution
-> unchanged audited Lottes CRT pass -> existing overlays/display capture.
At 80%, the pre-CRT image occupies the central 80% of each dimension, leaving
10% black on each edge. At 120%, it is enlarged and clipped. Curvature may
reshape this border. Normal displayed screenshots include the result;
unprocessed screenshots and canonical QA captures do not acquire this effect.
Custom presets remain untouched. The additional pass requires updating assets,
not just the EXE. Package staging and validation pin the new shader and preset.
The complete new BSD notice is embedded in the shader; upstream provenance and
the original permissive CRT shader bytes remain intact.

Local commands and results:

```sh
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
python3 tests/frontend/librashader/test_screen_size.py
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

Builds PASS. macOS feature-on focused suite: 11/11 PASS (6.39 s).
macOS feature-off focused suite: 11/11 PASS (6.05 s).
Coordinate/preset tests: 3 PASS, including exact rational pixel-center checks
at 640, 400, 1920 and 1080 pixels for 80/100/120%. These are explicitly static
and reference-math evidence, not GPU readback. Repository checks: zero findings.
Existing macOS duplicate-library/alignment and `maketextva.c` warnings, SDL
CMake deprecation and MinGW libarchive cross-check warnings remain unrelated.

Both shader stages additionally compiled successfully to SPIR-V using Debian
glslang-tools 12.0.0-2 in an ephemeral Colima container:

```sh
docker run --rm --mount type=bind,source="$PWD",target=/src,readonly debian:bookworm-slim sh -c 'apt-get update -qq && apt-get install -y -qq python3 glslang-tools && python3 /src/tests/frontend/librashader/test_screen_size.py --glslang /usr/bin/glslangValidator'
```

Container image digest:
`sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171`.
This does not prove librashader reflection, D3D11 compilation, or physical GPU
appearance. Windows visual/screenshot checks at 80/100/120%, parameter reload,
and GPU performance remain deferred. The added source-sized intermediate pass
has not been GPU-benchmarked; no performance gate is claimed.

Implementation: [fc470537](https://github.com/nakatamaho/vaeg/commit/fc470537f0bb018c6d8d0d5ec5a92b17ea65dcab).
MinGW was reconfigured and rebuilt after this commit to embed its identity.
Handoff: `build/mingw-cross/vaeg-m99z18-windows-x86_64.zip`, containing the EXE,
unchanged optional DLL, updated assets and notices. Directory and ZIP validation
PASS with `python3 tools/release/check-librashader-package.py --input <path>
--platform windows`; copied EXE matches the build with `cmp`.
EXE SHA-256: `10565bd5b1ba90a87397a588071bd04e3c426dc4470789efe49cd3622619bf48`.
DLL SHA-256: `1890f647c7fbe52d4cc591526db24367caca284996855c4565c6003c7e46f8cc`.

## M99z19 — Expand the FDD browser list with its window

Starting commit: `810fc808e900072109fc86bd533c3667e0a643cd`.
`draw_fdd_browser()` already uses a resizable ImGui window with first-use-only
initial dimensions. Vendored ImGui enables edge resizing by default and the
SDL platform backend advertises mouse cursor support. The inner list, however,
was fixed at 230 pixels high. It now consumes the available vertical space,
reserving the path field, Mount/Cancel row and wrapped status text using the
current font/style metrics. Small windows retain a three-row minimum and the
existing parent scrolling. No HDD browser or disk mounting code is changed.

MinGW and macOS feature-on builds PASS with the M99z18 commands above.
The initial build caught the repository's `max` macro; using `(std::max)`
resolved that collision before the successful builds. The same focused macOS
suite passes 11/11; encoding/EOL/case and diff checks are clean. Only the edited
GUI lines were formatted with `clang-format-mp-22`. These tests do not exercise
physical mouse dragging. Manual check: drag a side/corner of "Mount FDD image
or archive", verify the list expands, and confirm Mount/Cancel remain usable
at normal and high DPI. Windows interaction evidence remains deferred.

## M99z20 — librashader version header regression

The maintainer reported "shader parameter enumeration failed" after M99z18.
Reproduced against official librashader 0.12.0 macOS arm64, without a GPU:
`PreprocessError(MissingVersionHeader)`. The new shader's BSD comment preceded
its version directive. Moving `#version 450` to the first line resolves actual
runtime enumeration: nine parameters, including size metadata 100/80/120/1.
The license notice is unchanged; its position follows the required directive.
The standalone glslang test used in M99z18 did not enforce this librashader
preprocessor requirement and was insufficient. The new optional runtime test
checks both the working preset and a passing temporary fixture mutated only
by prepending a comment, asserting the exact MissingVersionHeader category.
The GUI now retains the runtime's detailed error string for future failures.

```sh
python3 tests/frontend/librashader/test_screen_size.py --runtime build/m99z20-runtime-check/librashader.dylib
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
```

Official release archive `librashader-aarch64-macos-v0.12.0-optimized.zip`
SHA-256: `49808004a4904f6a99e0231092dcfdfe52b7b61f68430a4c9f1e165749c4c90e`.
Downloaded with `gh release download librashader-v0.12.0 --repo
SnowflakePowered/librashader --pattern librashader-aarch64-macos-v0.12.0-optimized.zip
--dir build/m99z20-runtime-check`, and verified against the release asset digest.
No dependency version, shader math, or emulation/capture boundary changes.
Windows GPU appearance and mouse dragging remain unverified locally. The
handoff also includes the separate M99z19 FDD list resize commit.
Builds PASS; focused suite 11/11 PASS (7.31 s), runtime positive/negative checks
PASS, repository encoding/EOL/case checks zero and diff check clean.
Test runtime dylib SHA-256:
`1dbecea0c165fd0fddc2407ed4b8872f9f73f4fd2c3689a80ffc24c87e3fda2a`.

Fix: [0730d507](https://github.com/nakatamaho/vaeg/commit/0730d507e44a4e5fc4eb4d827763b5b93cbdabcf).
FDD resizing: [74644d99](https://github.com/nakatamaho/vaeg/commit/74644d996045fd60192e0b6058c4fa95d3cc8835).
Reconfigured and rebuilt MinGW with the committed fix identity. Handoff:
`build/mingw-cross/vaeg-m99z20-windows-x86_64.zip`. Both directory and ZIP pass
the package validator; copied EXE matches the build artifact using `cmp`.
EXE SHA-256: `15fdbc2e1c0a1578ad3eef8fcd8c789b658d06b331e56adbac161502e380c82a`.
Replace the package assets as well as the EXE: the shader file fixes CRT,
while the executable contains the FDD layout and improved error diagnostic.

## M99z21 — Requested CRT defaults and parameter name

Starting commit: `6e89523278bf67c23972047b742ff87b9bd4478f`.
Rename the owned shader parameter to `SCREEN_SIZE`, default 96.50%, range
80–120%, step 0.01%, displayed with two decimal places. Set `CURVATURE=0.030`
in the VAeg-authored preset without modifying the audited upstream shader.
The actual pinned runtime enumerates the preset override as the parameter's
initial value, so the existing reset path uses 0.030 without special-case code.
Saved settings retain precedence; use "Reset parameters" for the new defaults.
The retired parameter name is ignored by the existing unknown-key policy.

Validation uses the same M99z20 build, ctest and runtime commands above.
MinGW and macOS feature-on builds PASS. The actual 0.12.0 runtime returns
SCREEN_SIZE 96.5/80/120/0.01 and CURVATURE initial 0.030 (float32 precision).
The runtime negative test for MissingVersionHeader also PASS. Package hashes
are updated. No guest/capture changes or physical Windows GPU claims.
Focused ctest: 11/11 PASS (7.95 s). Encoding/EOL/case: zero; diff check clean.

## M99z22 — CRT parameters in the main configuration

Starting commit: `e95fdd1681ebece3108d51e2dd53292608ef28d6`.
Per the maintainer's clarification, no backward compatibility or import.
The old `vaeg-crt-parameters.cfg` is not referenced by production startup/UI
and is neither read, overwritten nor removed.

`NativeCRTParameters` is a bounded 8192-byte string in NP2OSCFG, serialized by
the existing ini reader/writer under the main config section. The C binding
shares only a buffer pointer/capacity with the frontend C++ parameter codec.
GUI and native presenters use that session store, including recreation and
reset. Normal exit saves CRT changes even if no other settings changed;
`--cfg` selects the destination and `--no-cfg` leaves only session memory.
No per-frame work or shader/capture math changes. The prior explicit-file
helper remains for isolated presenter tests, but no production caller supplies
the old filename. Invalid/oversized state fails without partial application.

Tests cover main ini round-trip using the real `initsave`/`ini_read` path,
disabled disk saving, session rebind/reset, transactional malformed-value
rejection and capacity overflow. Parameter tests now keep assertions enabled
in Release builds. Commands: the M99z20 focused ctest command plus the three
M99z18 local build commands. MinGW, macOS feature-on and feature-off builds
PASS. Feature-on focused suite: 11/11 PASS (8.00 s). Repository checks zero.
Physical Windows GUI restart/parameter persistence remains manual evidence.
Feature-off focused suite: 11/11 PASS (6.66 s).

Implementation: [d8105e3c](https://github.com/nakatamaho/vaeg/commit/d8105e3c8da3d10f958756b9e1b35a4b2a404eb1).
MinGW rebuilt with committed identity; EXE-only handoff at
`build/mingw-cross/vaeg-m99z22-windows-x86_64/vaeg.exe`, verified by `cmp`.
SHA-256: `80ab874e0a5affbfdb74a3ae0f058187535dd032819d02d1fcc35178466e1e64`.
Existing M99z21 shader assets and DLL are unchanged and can be retained.

## M99z23 — x4 and custom integer window sizes

Starting commit: `d729113cacaac58139fe13eff704f74302d5cffd`.
Add x4 to Window size and an integer multiplier mode (1–16) to Custom.
Preserve the existing explicit-pixel mode. Both use the existing windowed
presentation sizing path; the guest area is multiplied and menu height added
separately. The shared maximum replaces the former x3 cap in the renderer and
config loader, so selected custom multipliers survive config reload. GUI_scale
remains byte-sized; zero still identifies explicit pixel sizing. No core,
raw-capture or shader changes. Oversized windows may be constrained by the OS.

MinGW and macOS feature-on builds PASS using the previous build commands.
Focused ctest suite: 11/11 PASS (6.05 s). Added ROM-less SDL window sizing
checks use a small synthetic source at x1, x4, x5, x16, plus upper/lower clamp
cases, and verify actual window dimensions including menu height. This is not
physical Windows/native GPU evidence. Encoding/EOL/case checks zero; diff clean.

## M99z24 — Pixel-preserving padding before CRT

Starting commit: `223bedb25a6c1aca62e706c55789e876036b4770`.
The maintainer's SCREEN_SIZE=100 comparison removed the observed softness.
The old owned size pass used linear texture sampling to shrink the image into
a source-sized target, blending neighboring dots before CRT processing.
Replace that with a shared frontend FramePadding stage: copy original rows
1:1 onto an opaque black canvas, then upload and run the existing CRT chain.
The owned shader keeps SCREEN_SIZE metadata but only performs texelFetch;
its preset disables linear sampling on this copy pass. The audited Lottes
shader, CRT filtering, raw framebuffer, capture boundary and core are unchanged.

Symmetric integer margins preserve exact source aspect. At 640x400, requested
96.50% becomes 672x420 (effective 95.24%); 80% becomes 800x500. Quantization is
deliberate, toward more border, so small slider changes can share a size.
100% borrows the original frame. Above 100%, integer central cropping preserves
the former enlargement intent. This does not promise integer physical pixels
after CRT distortion or final scaling. Modes with small dimension GCDs have
coarser steps. SCREEN_SIZE is now reserved for frontend sizing; presets without
that parameter retain their previous path.

Storage is reused between frames, reinitialized on source/canvas/format changes,
and capped at 128 MiB (temporarily two buffers during replacement). Allocation
failure requests the existing SDL fallback. Failed CRT retries use the original
unmodified input. Normal screenshots include padded CRT output and enabled
overlays; unprocessed screenshots and raw QA do not. Splash/pass-through is
not padded. Distribution requires updating both EXE and assets together.

Local verification (all exit 0 unless otherwise stated):

```sh
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
python3 tests/frontend/librashader/test_screen_size.py --runtime build/m99z20-runtime-check/librashader.dylib
DYLD_LIBRARY_PATH="$PWD/build/m99z20-runtime-check" build/macos-ci/vaeg_librashader_controller_test assets/shaders/crt/vaeg_crt_default.slangp
build/macos-ci/vaeg_librashader_frame_padding_test
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

Feature-on/off macOS Release suites: 12/12 PASS, respectively 6.90 and 5.90 s.
The optional controller extension was rebuilt and rerun after those suites:
real runtime metadata plus fake GPU verifies padded input, unpadded retry on
filter failure and 100% bypass. This is not GPU evidence. The copy test checks
every source byte in RGB565/ARGB8888 with padded row pitch, both row origins,
all 256000 pixels retained below 100%, black borders, cropping, unchanged
source, buffer reuse, NaN and allocation bounds. CPU-only 10000-frame copy
benchmark: 20.442 us/frame, 1128960-byte buffer (640x400 to 672x420).
GPU resource/presentation cost and physical Windows sharpness remain unmeasured.
Existing unrelated compiler/linker warnings remain; no new build errors.

Both owned shader stages compile with glslangValidator 12.0.0 in an ephemeral
Debian bookworm container (python3 + glslang-tools); command:

```sh
docker run --rm --mount type=bind,source="$PWD",target=/src,readonly debian:bookworm-slim sh -c 'apt-get update -qq && apt-get install -y -qq python3 glslang-tools && python3 /src/tests/frontend/librashader/test_screen_size.py --glslang /usr/bin/glslangValidator'
```

Actual librashader 0.12.0 C API still enumerates nine parameters, SCREEN_SIZE
96.50 and CURVATURE .030; MissingVersionHeader negative regression passes.
No dependency upgrades or license changes. Updated owned shader SHA-256:
`d60de82a497cf15be02c07b06dcfc05f539d1558035159b736d3b3319b60acf5`.
Preset: `a705decc9008b81a033e4864d3be7c16a2f06d8ec241e741a6cf56cf246a1fc9`.
Audited upstream CRT shader and runtime DLL remain unchanged. Package validators
pin the new owned assets and revised provenance. Windows GPU/manual acceptance
remains deferred to the maintainer's updated EXE + assets test.

Implementation: [6bcda1e0](https://github.com/nakatamaho/vaeg/commit/6bcda1e02de3aae94b1605a01cd1c93e69f1b70d).
MinGW rebuilt with that committed identity; full local handoff directory:
`build/mingw-cross/vaeg-m99z24-windows-x86_64` (also ZIP of the same basename).
EXE SHA-256: `139f19693448c43b554cc3d1787abd15dfb495775675e61bb163cf9e53aa3299`.
Verified executable against build artifact with `cmp`. The package is based
on the existing freely distributable M99z21 bundle, restaged with
`tools/release/stage-librashader-assets.sh` and checked with
`tools/release/check-librashader-package.py --input <directory> --platform windows`.
No private media or binaries are committed.

## M99z25 — Default SCREEN_SIZE to 98 percent

Starting commit: `51b0371549401e7f81ef581887a5a53a0b05a136`.
Change only the owned shader's SCREEN_SIZE default from 96.50 to 98.00,
matching runtime/static tests, package hash pins and the current user guide.
Saved values still take precedence; Reset applies the new default. CURVATURE,
padding math, shader code and raw QA are unchanged. The maintainer reported
the preceding padding version as "much better"; this is a preference change,
not a new correctness fix or a full platform gate claim.

Verification: `python3 tests/frontend/librashader/test_screen_size.py --runtime
build/m99z20-runtime-check/librashader.dylib` passes three tests and the actual
0.12.0 C API reports SCREEN_SIZE initial 98.0. The MissingVersionHeader negative
test passes. `ctest --test-dir build/macos-ci --output-on-failure -R
'^vaeg_librashader_(screen_size|shader_parameters|frame_padding)$'`: 3/3 PASS.
`CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4`: no work to do;
no C/C++ build inputs changed. Prior binary test evidence remains applicable.
The shader SHA-256 is
`9a521d7ecf3ead998a5039d33d144903c5764793eba5415dc98ac4ae7ec5c361`.
The Windows M99z25 handoff reuses the M99z24 EXE/DLL and restages the updated
assets; package validation passes. No new physical GPU testing claimed.

## M99z26 — Scanline brightness antialiasing

Starting commit: `9aa4035daff2e3149c2ac00ea5e5b41daad8b9bf`.
Scope: the maintainer approved averaging scanline brightness, retaining thin
lines at large sizes and fading them at small sizes, without a whole-image
blur. Private supplied screenshots were inspected locally only; no private
image, identity or image-derived payload is tracked. Synthetic uniform-color
fixtures reproduce the approximately 40px beat at 410 source rows / 400 output
pixels, including curvature=0. Point-sampled beam modulation is the demonstrated
defect; other mask/content interference is not claimed resolved.

The audited third-party `crt-lottes-fast.slang` remains byte-identical.
New BSD-2-Clause `vaeg-crt-aa.slang` expresses the public-domain Lottes
color/kernel/warp/mask/tone model with an owned AA helper. The preset selects
this pass after the unchanged size/copy pass. All nine parameter names,
defaults, persistence and raw-capture boundaries remain. No core, native
presenter, texture-upload, frame-allocation or runtime-loader changes.
Active dependency closure is two owned shaders plus one owned include and
the preset. Notices/provenance/package pins record the reference and new
code separately; no GPL or unknown-license dependencies are introduced.

`vaeg-scanline-aa.inc` analytically integrates the sum of the two periodic
cosine beam envelopes. `fwidth(position.y)` supplies a local, curvature-aware
source-row footprint. Existing normalized row-color weights and four-tap
horizontal reconstruction are retained (eight image fetches, no added taps).
Only their common brightness multiplier changes. At exact zero beam weights,
use the symmetric limiting row blend to avoid division by zero. Fade contrast
between footprints .25 and .5 (four to two output pixels/row); at .5 and above
use mean brightness. A later fade initially left gamma/tone-generated aliases
at 800px height; final raster tests motivated fading before Nyquist instead.
SCREEN_SIZE=98, black padding, curvature control and raw QA are unchanged.

Evidence and commands (exit 0):

```sh
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
build/macos-ci/vaeg_librashader_scanline_aa_test
python3 tests/frontend/librashader/test_screen_size.py --runtime build/m99z20-runtime-check/librashader.dylib
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

macOS feature-on/off Release: 13/13 PASS (7.21 / 7.40 s). Runtime 0.12.0
enumerates all nine parameters, including includes; MissingVersionHeader
negative regression passes. Existing linker/dependency warnings remain.
The C++ test executes the same float functions used in GLSL and compares to
independent double-precision numerical integration across period boundaries,
thinness endpoints and minification. Linear-light 40px band amplitude:
400px `.35442824 -> 0`; 800px `.02255468 -> 0`; 1600px `.00251115 -> .00006107`.
At 1600px the fundamental scanline contrast retains 89.38%.

Optional software-raster/compile reproduction (task-local Docker container,
read-only source mount; dependencies only inside the container):

```sh
docker run -d --name vaeg-m99z26-shader-check --mount type=bind,source="$PWD",target=/src,readonly debian:bookworm-slim sleep 3600
docker exec vaeg-m99z26-shader-check sh -c 'apt-get update -qq && apt-get install -y -qq python3 python3-moderngl python3-numpy glslang-tools libegl1-mesa libgl1-mesa-dri libgl1 libgl1-mesa-dev'
docker exec vaeg-m99z26-shader-check python3 /src/tests/frontend/librashader/test_screen_size.py --glslang /usr/bin/glslangValidator
docker exec vaeg-m99z26-shader-check python3 /src/tests/frontend/librashader/test_scanline_aa_gpu.py
```

glslang 12.0.0 compiles both stages of both active shaders. Raster tests use
ModernGL 5.7.4, NumPy 1.24.2, EGL llvmpipe LLVM 15.0.6 (128 bits), Mesa 22.3.6
GL 4.5. This is NOT librashader-chain, D3D11, Metal or physical GPU evidence.
The test translates descriptor syntax only; test-local point-brightness
substitution isolates color/kernel parity against the unchanged reference.
All masks and curvature 0/.03/.25 pass, worst float RGB difference .000546
(less than one 8bit code). Constant modulation also passes edge parity.
Uniform grey/cyan/red rendered through tone/gamma demonstrate small-output
40px bands reduced to at most 1.6e-8, versus original .073-.149 at 400px and
.013-.027 at 800px. At 1600px residual amplitude is at most .000620, below
half an 8bit code. Some already sub-LSB original bands are smaller than the
new residual; this is not an across-the-board relative improvement claim.
The raster criterion is 85% attenuation OR a half-code absolute ceiling.
Complete-period windows avoid Fourier-bin leakage from cropped scanlines.
Mean exposure shift stays below .04 on [0,1]. Extreme thinness/curvature and
small viewports produce finite output. No new image-blur kernel is used.

Performance: CPU helper benchmark, one million float evaluations, point
8.19 ns versus AA 9.38 ns (not GPU cost). Final 20-frame software-raster
median/p95 at 640x400: original 19.96/31.79 ms, AA 30.35/35.11 ms;
640x1600: original 48.76/66.26 ms, AA 51.16/65.56 ms. An earlier small-output
run had the opposite ordering (26.28 vs 21.98 ms median), indicating host
variance. Report the measured small-output regression, not a speedup or
60Hz hardware claim. Real GPU performance and Windows visual acceptance
remain deferred. No per-frame allocations, extra passes or image taps added.

Package pins cover the new shader, helper, preset and provenance. Runtime
0.12.0 DLL, loader and dependency versions are unchanged. Windows package
staging/validation uses the M99z25 bundle plus the updated assets. Update all
of `assets/shaders/crt/` and restart; do not copy only the preset or omit its
new include. Existing saved settings remain explicit user choices.

Pre-existing hosted failure inspected before push:
[33953017511](https://github.com/nakatamaho/vaeg/actions/runs/33953017511),
macOS FetchContent `vaeg_upd9002_trace_equivalence`, checkpoint counts 17 vs 18
at `tests/upd9002/run_trace_equivalence.cmake:74`. M99z26 changes no CPU/trace
code; this unrelated failure is not repaired or represented as passed here.

Implementation: [058e238c](https://github.com/nakatamaho/vaeg/commit/058e238cb113ed8da9ae57895e09eba13944ad65).
MinGW was configured/rebuilt with this committed identity. Full local handoff:
`build/mingw-cross/vaeg-m99z26-windows-x86_64.zip` and the same-named directory.
EXE SHA-256: `c0925bcfb088a7f71f50e6d8c85a9ff9fc55b8ef6c5de970e5185e473b8dab21`;
`cmp` against the build artifact passed. The asset stager and package validator
pass with the new closed dependency hashes. No generated binaries or private
inputs are committed. The test container is retained, stopped, for reuse;
base image digest is
`debian@sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171`.

## M99z27 — D3D11 mask compilation and pass-through

Starting commit: `6a6a61cd9e87260fcde2edb3ed9b7b2dd1968e6b`, clean
`topic/m99-native-crt-rebuild`. Scope: the reported default-preset X3500
regression and a native pass-through menu comparison control. The main
checkout and private integration data are untouched.

The maintainer supplied a real D3D11 compiler diagnostic: X3500, array
reference cannot be used as an l-value / not natively addressable. The AA
shader assigned `mask[channel]` using a computed vector index. Replace it
with equivalent `.r`, `.g`, `.b` writes. No scanline math, taps, curve,
SCREEN_SIZE default or core behavior changes. The new static regression
rejects the old mask write. GLSL and a glslang HLSL round-trip both accept
the old version, so neither substitutes for the actual D3DCompile test.

Dependency/provenance: librashader remains dynamic MPL-2.0 release 0.12.0,
commit `87e8a97b50516d997defeaa168173dcd185d4022`, API 5 / ABI 2.
No runtime/loader changes. The only changed shader is VAeg-owned BSD-2-Clause;
the audited Unlicense original and dependency/license closure are unchanged.
New shader SHA-256:
`9d329990e19d26722a8acfd6b5c20699564220d8bb7e7ef9fc604eb90f379a4b`.
Both staging and package-check pins are updated.

Local candidate verification (exit 0 unless stated):

```sh
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
python3 tests/frontend/librashader/test_screen_size.py --runtime build/m99z20-runtime-check/librashader.dylib
docker exec vaeg-m99z26-shader-check python3 /src/tests/frontend/librashader/test_screen_size.py --glslang /usr/bin/glslangValidator --spirv-cross /usr/bin/spirv-cross
docker exec vaeg-m99z26-shader-check python3 /src/tests/frontend/librashader/test_scanline_aa_gpu.py
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

ON/OFF: 13/13 PASS, 6.71 / 6.48 seconds. Metadata: all nine parameters,
including SCREEN_SIZE 98 and CURVATURE .030; version-header negative passes.
Static suite: 4 tests PASS. glslang 12 / SPIRV-Cross 1.3.239 compile both
stages of both owned shaders and generated SM5 HLSL. Software EGL raster
uses the retained Mesa 22.3.6 / llvmpipe LLVM 15 environment, no private
screenshots. All masks and curves preserve reference color/kernel behavior,
worst float RGB difference .000546. Small-output 40px band amplitudes still
at most 1.6e-8; 1600px residual at most .000620. The scanline AA fix remains.
Software timing median/p95 (ms): 640x400 original 14.67/28.05, AA 26.85/35.28;
640x1600 original 48.29/57.98, AA 54.53/138.44. Concurrent cross-build and
emulated Wine bootstrap make these timings noisy, not GPU performance proof.
No added frame allocations or texture taps. Real GPU timing remains deferred.

Optional `test_d3d11_filter.cpp` creates a real WARP device and filter chain
through the pinned C API with shader caching disabled. It is an explicitly
invoked test, not an unconditional CTest requiring the optional runtime.
Build/run on Windows beside the pinned DLL:

```sh
cmake --build build/mingw-release --target vaeg_librashader_d3d11_filter_test
# Run from the staged package directory:
diagnostics/vaeg-d3d11-filter-test.exe assets/shaders/crt/vaeg_crt_default.slangp
```

Local cross-build commands:

```sh
cmake --preset mingw-cross -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON
CCACHE_DISABLE=1 cmake --build build/mingw-cross --target vaeg_librashader_d3d11_filter_test -j4
cmake --preset mingw-cross -DVAEG_ENABLE_TESTS=OFF -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=OFF
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
```

An initial tests-ON configure without integration trace was correctly
rejected by the existing CMake guard. The corrected test build uses both
flags; the release executable retains tests/trace OFF. Existing upstream
CMake, linker, and unrelated text-mode precedence warnings are not changed.
The optional probe was also built with static MinGW C++17 and `-ld3d11`.
Local Wine 8 / Debian amd64 / Xvfb attempt returned exit 2:
`D3D11_RUNTIME_UNAVAILABLE`, loader error 126. Loader tracing identifies
absent `bcryptprimitives.dll` in Wine, not the maintainer's Windows machine.
No replacement DLL or unpinned runtime was used. This is NOT a successful
D3D11 filter-chain run. Windows visual/physical-GPU validation remains open.

Prior hosted run inspected before publication:
[33956386947](https://github.com/nakatamaho/vaeg/actions/runs/33956386947),
9 jobs PASS; macOS FetchContent fails the pre-existing
`vaeg_upd9002_trace_equivalence`. Its exact failed log was retrieved; CPU
and trace code are unchanged and no unrelated repair is included here.

Shader correction commit:
[c3efc946](https://github.com/nakatamaho/vaeg/commit/c3efc946799a33e00f6ac739b240711bf8b626f5).

### Native comparison menu

Add `画面 > Pass-through（加工なし）` only while the native presenter is
active. Its check reflects actual filter state, including automatic fallback.
It calls the existing presentation-thread filter toggle, without renderer
ownership changes, GUI destruction, preset parsing or guest reset. Uncheck
to restore the current filter. A failed chain cannot be re-enabled; its
failure remains visible and `CRT設定… > Reload preset` retries creation.
This is a session-only comparison control, not a new saved configuration
field: reload/restart re-enables the configured CRT. Preset parameters are
retained. SDL selection is unchanged and still lives under `描画方式`.

The existing 100-toggle controller / 50-pair D3D11 mock lifecycle tests pass.
The extended real-metadata + fake-presenter integration explicitly shows
padding bypass in pass-through, restored padding on CRT resume and unchanged
device creation count. Command (exit 0, `controller checks: PASS`):

```sh
DYLD_LIBRARY_PATH=build/m99z20-runtime-check build/macos-ci/vaeg_librashader_controller_test assets/shaders/crt/vaeg_crt_default.slangp
```

The ON/OFF 13-test results above include this menu-controller candidate.
No raw framebuffer or overlay-capture semantics are changed. Physical mouse
interaction on Windows remains a maintainer check. Code formatting was
restricted to the changed regions; pre-existing unrelated formatting was
preserved. MinGW Release and the optional CMake probe target both build.
The staged Windows directory passes the package validator with all shader
pins, notices, the unchanged 0.12.0 runtime, and the optional probe.

### Handoff identities and evidence boundary

Menu implementation / rebuilt release identity:
[601ce5f1](https://github.com/nakatamaho/vaeg/commit/601ce5f1e565b93eeafd0897c49f0cf101e457da).
The final follow-up report commit changes documentation only; it does not
justify repeating the unchanged emulator tests. Evaluated controller binary
SHA-256: `5bce43c9c6f1edc4ef542b8aa4b0c96277fac2f78da2e0a0f48981c753c045f8`.

Local deliverables (not committed):

- `build/mingw-cross/vaeg-m99z27-windows-x86_64.zip`:
  `8c21e56218512b997f3a4e78b80d3d67288b195d31e220e7426e9d70e9fc8578`.
- Same-named staging directory, EXE:
  `b4632b6616a45bc89aa87207a2bf42fb0dc634be7430078b0fc0d0c8849307e4`.
- Optional static diagnostic `diagnostics/vaeg-d3d11-filter-test.exe`:
  `e51a1daa9fee0c62526456e366cf751bb43555ddcdd91979299e928f56191dda`.
- Unchanged runtime DLL:
  `1890f647c7fbe52d4cc591526db24367caca284996855c4565c6003c7e46f8cc`.

`cmp` verifies the staged EXE against its build output. Both directory and
ZIP package checks pass. The test containers are stopped and retained for
reuse; no broad cleanup or private-data changes occurred.

```sh
cmake -E copy build/mingw-cross/sdl2/vaeg.exe build/mingw-cross/vaeg-m99z27-windows-x86_64/vaeg.exe
cmp build/mingw-cross/sdl2/vaeg.exe build/mingw-cross/vaeg-m99z27-windows-x86_64/vaeg.exe
bash tools/release/stage-librashader-assets.sh --output build/mingw-cross/vaeg-m99z27-windows-x86_64 --platform windows --runtime build/mingw-cross/vaeg-m99z26-windows-x86_64/librashader.dll
python3 tools/release/check-librashader-package.py --input build/mingw-cross/vaeg-m99z27-windows-x86_64 --platform windows
# From build/mingw-cross:
cmake -E tar cf vaeg-m99z27-windows-x86_64.zip --format=zip vaeg-m99z27-windows-x86_64
# From repository root:
python3 tools/release/check-librashader-package.py --input build/mingw-cross/vaeg-m99z27-windows-x86_64.zip --platform windows
```

Local builds/tests/package checks PASS. Required human follow-up: update
EXE and assets together, restart, confirm CRT creation and the menu's
pass-through/CRT comparison on Windows. Physical D3D11 execution and GPU
performance remain unverified locally. No overall M99 hardware gate is
advanced by these software-only results.

## M99z28 — Raster-anchored RGB mask

Start: `81dfbdd7f9027f6abdda805f37d24314aae87c7e`, clean
`topic/m99-native-crt-rebuild`. The maintainer confirmed M99z27 substantially
restored Windows CRT rendering, then reported vertical bands. No private
screenshot, path or identity is published. This follow-up only changes the
owned default shader's mask coordinates, tests, package pins and documentation.

### Cause and correction

The D3D11 bridge supplies an inset `libra_viewport_t` while the output RTV is
the whole window target. In the pinned runtime,
[`filter_pass.rs`](https://github.com/SnowflakePowered/librashader/blob/87e8a97b50516d997defeaa168173dcd185d4022/librashader-runtime-d3d11/src/filter_pass.rs)
derives output-size semantics from the output texture. The old mask's
`vTexCoord * params.OutputSize.xy` therefore advances more than one mask
pixel per raster pixel when the viewport is smaller than the target.
This is a VAeg shader assumption error, not a changed runtime ABI.

Use `gl_FragCoord.xy` for the mask only. SPIRV-Cross translates its fragment
input to `SV_Position` in SM5 HLSL; the compilation test verifies that semantic.
The RGB mask remains anchored to render-target pixels, including when the
viewport moves. Guest coordinates, curve, SCREEN_SIZE padding, image taps,
scanline AA and all parameter defaults remain unchanged. The M99z27 constant
component writes remain in place. No new per-frame allocation or filter pass.

### Reproduction and focused tests

The optional EGL test now draws real sub-viewports into larger targets, not
just full-target quads. Neutral fixtures use targets/viewports:

- 1000x650 / (180,55) 640x400;
- 1280x800 / (323,101) 640x400;
- 1001x701 / (37,49) 641x401.

All four masks pass 3px (or 6px for mask 3) period checks: maximum float RGB
residual `1.1920929e-7`, tolerance `1e-5`. The first fixture compresses the
old 3px period to 1.92px and produces a 24px alias. Its rendered mean-RGB
24px Fourier amplitude drops from `.00642442084` to `1.9690132e-9`.
The passing fixture receives exactly one old-coordinate mutation and must
fail `M99_MASK_GPU_PIXEL_PERIOD`, not an unrelated check. This establishes
the defect in a ROM-less software raster, without claiming a Windows run.

With MASK=0, random-image A/B outputs have exactly equal float RGB values in the inset
viewport: no image blur or geometry change. Full-target color/kernel parity
against the audited original still passes all masks and curvature values,
worst float RGB difference .000546. Previous scanline tests still suppress
small-output 40px bands to at most 1.6e-8, with 1600px residual below .000620.

Commands (exit 0):

```sh
docker start vaeg-m99z26-shader-check
docker exec vaeg-m99z26-shader-check python3 /src/tests/frontend/librashader/test_scanline_aa_gpu.py
docker exec vaeg-m99z26-shader-check python3 /src/tests/frontend/librashader/test_screen_size.py --glslang /usr/bin/glslangValidator --spirv-cross /usr/bin/spirv-cross
python3 tests/frontend/librashader/test_screen_size.py --runtime build/m99z20-runtime-check/librashader.dylib
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

macOS ON/OFF: 13/13 each, 6.09 / 6.13s. Static suite: 5 tests PASS;
runtime metadata: nine parameters, SCREEN_SIZE 98 and CURVATURE .030, plus
the version-header negative regression. GLSL and HLSL stages compile and
SM5 fragment position semantic is present. These are not D3DCompile or Metal
filter-chain execution claims. The already-recorded Wine 8 missing
`bcryptprimitives.dll` prevents testing the unchanged Windows runtime locally;
do not repeat that unchanged failed setup or substitute an unpinned DLL.

Software performance only (Mesa 22.3.6 / llvmpipe LLVM 15 / ModernGL 5.7.4,
20 timed frames per case, `ctx.finish()` per frame), median/p95 milliseconds:
640x400 old AA coordinates 17.28/33.80, fixed 11.47/21.51;
640x1600 old AA coordinates 38.62/50.20, fixed 37.18/88.55.
The noisy p95 is reported, not a hardware speedup or 60Hz acceptance claim.

### Build reuse, licensing and handoff

No compiled C/C++ or build configuration changes relative to the starting
commit. Reuse the M99z27 verified builds rather than rebuilding unchanged
emulator code for a runtime-loaded asset fix. Verified hashes:

- MinGW EXE, built at `601ce5f1e565b93eeafd0897c49f0cf101e457da`:
  `b4632b6616a45bc89aa87207a2bf42fb0dc634be7430078b0fc0d0c8849307e4`.
- macOS ON executable:
  `920e1fb5fda61d6f884d21b028a1850c9281631e951d27662cfc9d211723a6f9`.
- Evaluated controller binary:
  `5bce43c9c6f1edc4ef542b8aa4b0c96277fac2f78da2e0a0f48981c753c045f8`.

The sole changed production asset SHA-256 is
`7cef376e3f8d4c76112842b155cf642b8a32eb4b89921dd026ba55f07390b9ee`.
It remains VAeg-owned BSD-2-Clause. Original Lottes/Unlicense file,
dependencies, notices and closure are unchanged. librashader remains dynamic
MPL-2.0 v0.12.0 / commit `87e8a97b50516d997defeaa168173dcd185d4022`, API 5,
ABI 2; no GPL/unknown shader or vendor modification is introduced.

Stage the complete M99z28 Windows package from M99z27, retaining the exact
EXE, DLL, optional diagnostic and licenses, then replace the validated assets
and user guide. Both package pins are updated. Already-installed M99z27 only
needs the new assets and preset reload/restart; no EXE or DLL replacement is
required for this correction. Full ZIP is provided for a consistent handoff.

Prior CI [33957628012](https://github.com/nakatamaho/vaeg/actions/runs/33957628012)
has 9 passing jobs; the exact failed macOS FetchContent log was retrieved:
`vaeg_upd9002_trace_equivalence`, 17 vs 18 checkpoints at
`tests/upd9002/run_trace_equivalence.cmake:74`. This unchanged unrelated
failure is neither repaired nor counted as PASS. No physical platform gate
is advanced. Remaining manual check: Windows CRT and pass-through, resize,
letterboxing and fullscreen, with the new assets.

Package: `build/mingw-cross/vaeg-m99z28-windows-x86_64.zip`, SHA-256
`773e08e0db5afc44128b0eff7b616d9b17b60ceba14f8e910e13202164992879`.
Directory and ZIP package validation PASS; EXE `cmp` against M99z27 PASS.
Commands (exit 0):

```sh
cmake -E copy_directory build/mingw-cross/vaeg-m99z27-windows-x86_64 build/mingw-cross/vaeg-m99z28-windows-x86_64
bash tools/release/stage-librashader-assets.sh --output build/mingw-cross/vaeg-m99z28-windows-x86_64 --platform windows --runtime build/mingw-cross/vaeg-m99z27-windows-x86_64/librashader.dll
cmp build/mingw-cross/vaeg-m99z27-windows-x86_64/vaeg.exe build/mingw-cross/vaeg-m99z28-windows-x86_64/vaeg.exe
python3 tools/release/check-librashader-package.py --input build/mingw-cross/vaeg-m99z28-windows-x86_64 --platform windows
# From build/mingw-cross:
cmake -E tar cf vaeg-m99z28-windows-x86_64.zip --format=zip vaeg-m99z28-windows-x86_64
# From repository root:
python3 tools/release/check-librashader-package.py --input build/mingw-cross/vaeg-m99z28-windows-x86_64.zip --platform windows
```

Correction commit:
[dadbd38f](https://github.com/nakatamaho/vaeg/commit/dadbd38fafcac017685239513a0cfe34b0d6e4ff).
The following evidence-link commit changes only these reports/ledger, so
the evaluated shader, tests and packaged payload remain identical. The
task-local software-render container is stopped and retained for reuse.
