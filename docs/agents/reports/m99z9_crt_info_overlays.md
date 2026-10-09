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

# M99z9 — CRT information overlays and renderer menu clarity

Starting commit: `fd25f524b9a0a8ce319c43e7a6ac6fae146915a0`.
Branch: `topic/m99-native-crt-rebuild`. Main checkout remains untouched.

## Cause and correction

The maintainer reported successful display after M99z8, but Video info and
Framebuffer info disappeared in CRT mode. Both overlays were called only in
the SDL tail of `scrnmng_present_end()`, after the native branch returned.

Native GUI rendering now submits those same overlay formatters, layout and
bitmap glyphs to ImGui's background draw list before finalizing draw data.
D3D11 composes that data after the guest filter and before menus/dialogs.
Coordinates are converted from drawable pixels to ImGui logical coordinates,
including viewport origin. The SDL renderer keeps its original overlay path.
No guest pixel, raw capture, shader, runtime, or font payload is changed;
no CPU full-frame overlay surface or GPU overlay texture is created per frame.

The extra `SDL / CRT off` row was status text, not a third renderer.
Remove that confusing row; renderer choices remain SDL and librashader.
Preset/parameter controls, title diagnostics and failure logs are preserved.

## Verification

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

The new ROM-less selftest uses the actual ImGui rectangle-emission helper,
checking vertex count, bounds, color and nonzero origin at 1x and 2x
framebuffer scale. It does not require a GPU. Existing raw-capture and
presenter/fallback tests remain in the focused suites.

Results: MinGW and macOS feature-on/off builds PASS; both focused suites
10/10 PASS. Encoding/EOL/case checks reported zero findings; diff check clean.

Physical Windows overlay visibility and GPU performance remain untested locally.
Metal/OpenGL native GUI remains outside this Windows follow-up's scope.

## Handoff

Implementation: [d8979d1d](https://github.com/nakatamaho/vaeg/commit/d8979d1d9f3ebcca6d035eeb0d5090d0379c66ed).
Reconfigured and rebuilt MinGW after committing to embed that build identity.
Local package: `build/mingw-cross/vaeg-m99z9-windows-x86_64.zip`.
Includes the runtime DLL, assets, licenses and native-start launcher.

- EXE SHA-256: `cddb414dd85174054f1f7a91c7c03f1947f0b85393d6a23b2cd29ed0bbbbe5a9`.
- ZIP SHA-256: `23fb0802d58266cbea1604086f872362f0280127e52082d33a03d9ef0ae2a520`.
- Staged executable matches the build artifact using `cmp`.
- Directory and ZIP both pass `tools/release/check-librashader-package.py`
  with `--platform windows`.

The runtime is unchanged: librashader 0.12.0, pinned revision
`87e8a97b50516d997defeaa168173dcd185d4022`, API 5 / ABI 2.
DLL SHA-256: `1890f647c7fbe52d4cc591526db24367caca284996855c4565c6003c7e46f8cc`.
