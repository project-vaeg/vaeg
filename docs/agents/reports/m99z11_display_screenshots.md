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

# M99z11 — Display screenshots

Starting commit: `524138beed26ea351e62e20ab4603e0ff15f0f99`.
Branch: `topic/m99-native-crt-rebuild`. Main's unrelated changes remain untouched.

## Implementation

The first Screen menu entry now saves a composed display screenshot, not the
raw guest shadow used by the previous normal-save handler. PrintScreen and the
configured F12 binding share the same handler. The raw graphics-analysis item
is explicitly labeled separately; CLI raw captures and goldens are unchanged.

A bounded one-request queue waits until a fresh GUI frame can omit menus and
dialogs. Normal rendering still emits enabled Video/Framebuffer overlays. SDL
reads the renderer after effects and overlays and before Present. Windows D3D11
reads the swap-chain buffer after librashader and ImGui overlay composition,
before Present, using a temporary CPU-readable staging texture. BGRA is
converted to top-down RGBA with opaque alpha and source row pitch respected.
Only the top menu strip is cropped; drawable size, aspect, scaling and margins
are otherwise preserved. PNG encoding uses the existing writer.

No staging/readback/encoding allocation occurs without an explicit screenshot
request. One-shot readback may block briefly for GPU completion and PNG writing.
The borrowed target is detached immediately after presentation; no stale
readback is reused. Failed/unsupported readback reports an error and never
silently saves raw input. Minimized output waits until presentation resumes.

Native Metal/OpenGL readback and native GUI remain outside this Windows
follow-up; unsupported readback fails explicitly. Physical Windows shader/PNG
comparison remains deferred; cross-compilation is not GPU proof.

## Verification

Commands:

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

The new SDL software-renderer selftest uses production effect, overlay,
readback and crop functions. It verifies white unfiltered pixels, scanline
darkening, info-on pixel differences, exact restoration with info off, menu
strip cropping (640x422 to 640x400), and bounded/deferred request handling.
Controller tests verify target forwarding, completion during presentation and
immediate detachment. Existing raw-capture and lifecycle tests remain enabled.

Results: MinGW and macOS feature-on/off builds PASS; both focused suites
10/10 PASS (7.40 s / 6.71 s). Encoding/EOL/case checks: zero findings; diff
check clean. Existing logical-not, unused-variable and Apple linker warnings
remain unchanged. No Windows execution or native GPU readback was possible
locally, so output equivalence on that backend is not claimed as tested.

## Handoff

Implementation: [5699d96e](https://github.com/nakatamaho/vaeg/commit/5699d96e471ac72b07203e26182ec62a5a210a06).
MinGW was reconfigured/rebuilt after committing to embed that build identity.
Package: `build/mingw-cross/vaeg-m99z11-windows-x86_64.zip`, including the
unchanged runtime DLL, shader assets, notices and updated user guide.
Staged directory and ZIP validators PASS; staged EXE matches its build via `cmp`.
ZIP SHA-256: `ae643f6938e905e1884349a7d0f4c16c55b814c4ed70daadb22894fa06cb5516`.

## M99z12 — Unprocessed screenshot overlay selection

Starting commit: `bf008214f283583edf0c0d986fdc7fa22bb12417`.
Rename the second menu entry to `スクリーンショットを保存（加工前）`.
The former raw-analysis formatter always generated both panels. It now
independently honors `VAEG_DISPINFO_VIDEO` and `VAEG_DISPINFO_FRAMEBUFFER`,
returning without any panel allocation or pixel change when both are off.
Normal composed screenshots already honor those toggles and are unchanged.
Deterministic CLI raw captures remain independent of GUI display flags.

The production surface formatter is exercised with all four toggle combinations;
all produce distinct images and both-off is byte-identical to the input surface.
Validation uses the same build/test/check commands listed above. Windows visual
verification remains deferred; this follow-up changes no native GPU code.

Results: MinGW and macOS feature-on/off builds PASS. Both focused suites
10/10 PASS (6.06 s / 5.90 s); encoding/EOL/case checks zero, diff check clean.

Implementation: [e25b14c9](https://github.com/nakatamaho/vaeg/commit/e25b14c907d18e8bbdb73c1f364a3edc40a28bf5).
Rebuilt MinGW after committing; staged EXE matches via `cmp`.
Updated handoff: `build/mingw-cross/vaeg-m99z12-windows-x86_64.zip`.
ZIP package validator PASS; DLL/assets/notices included.
ZIP SHA-256: `7f2c77bb83c675d2a9ce0c39bd0b8b5f53b729d567c900e5eb2a830d9e677cc8`.
