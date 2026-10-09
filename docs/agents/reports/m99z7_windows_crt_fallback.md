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
# M99z7 — Windows renderer selection and black fallback

Status: candidate; physical Windows CRT acceptance remains OPEN.
Starting commit: `77bca4d6f9ec479245358997c39b90dcb703025e`.
Branch: `topic/m99-native-crt-rebuild`; main checkout changes are untouched.

## Findings and changes

- Remove UI size from the menu, retaining automatic Windows DPI and the
  existing config override. Replace the ambiguous enable toggle with
  `CRT shader > SDL / librashader`. Explicit SDL selection destroys the
  native presenter and recreates SDL resources; selecting librashader again
  retries initialization. Filter-failure fallback remains native pass-through.
- The pass-through strip's winding is counterclockwise in render-target space.
  D3D11 default raster state culls back faces with clockwise fronts. Previously
  binding a null rasterizer could therefore remove the entire guest quad.
  Bind an explicit no-cull state, created once and released with the device.
  The production shader and rasterizer descriptor are shared with a Windows
  WARP readback regression test; it compares old default state with no-cull
  using the same 16x16 target, source texture and shaders.
- PE inspection of the pinned official DLL found external imports
  `D3DX9_43.dll`, `MSVCP140.dll`, `VCRUNTIME140.dll`, and
  `VCRUNTIME140_1.dll`. The former package validation established hashes
  and payload shape, not target-machine dependency availability. Missing
  dependencies are a supported failure case, not established as the exact
  cause on the maintainer's machine without its log.
- Wrap (do not modify) the audited upstream loader to report Win32 load errors,
  unavailable dependencies, required symbols, API/ABI and loaded DLL path.
  Preserve actual preset/filter errors through presenter and frontend.
  Upstream no-op stubs are not sufficient proof that required symbols exist.
- The package guide links official Microsoft DirectX June 2010 and VC++ x64
  installers. No installer is run automatically and no Microsoft DLL is
  redistributed without a separate license review.

## Validation

Commands (repository root):

```sh
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
ctest --test-dir build/macos-ci --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
CCACHE_DISABLE=1 cmake --build build/m99-gui-debug --target vaeg_librashader_controller_test vaeg_librashader_d3d11_lifecycle_test vaeg_librashader_presenter_state_test vaeg_librashader_pass_through_test vaeg_librashader_fallback_test -j4
ctest --test-dir build/m99-gui-debug --output-on-failure -R '^vaeg_librashader_'
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

MinGW build PASS; feature-on/off macOS builds PASS; focused suites 10/10 each;
assertion-enabled Debug suite 8/8. Added checks prove detailed errors survive
successful fallback rendering and clear on recovery. Existing Apple linker
warnings and unused Release assert-fixture warnings remain unrelated.

The Windows raster test cross-compiles with:

```sh
x86_64-w64-mingw32-g++ -std=c++17 -Isdl2 tests/frontend/librashader/test_d3d11_raster.cpp -o build/mingw-cross/vaeg-d3d11-raster-test.exe -ld3d11 -ld3dcompiler -static-libgcc -static-libstdc++
```

Runtime command on Windows:
`ctest --test-dir <windows-build> -R vaeg_librashader_d3d11_raster -V`.
Expected white-pixel counts: default=0, explicit-no-cull=256/256.
At preparation time no Windows/Wine execution environment is available locally;
compilation is not a PASS claim for the readback assertions. Hosted results
must be recorded separately. WARP is software rasterization, not GPU performance
or librashader filter-chain acceptance.

## Candidate package

Rendering/diagnostic implementation:
[ee58bda0](https://github.com/nakatamaho/vaeg/commit/ee58bda0f3652cc9936f791806d6f2f65e94b2a1).
Local ZIP: `build/mingw-cross/vaeg-m99z7-windows-x86_64.zip`.
The executable was rebuilt after configuring the committed identity. The DLL,
shader assets, updated guide, CRT launcher, licenses and a standalone WARP
diagnostic executable are included. No private input is packaged.

SHA-256:

- ZIP: `2213c3d4daf2c5c89038490b0b2d5433f89ddc59b2f84cc3e0b32ae9c0b96bd2`
- EXE: `709c7282a56f7d0ebcb003a223c74f396271a033de41b260fa6b8b48aabbae6f`
- WARP diagnostic: `e70ca766aab5b9587405ff0141459c5d2f74a2a0ee0137c3b6c15dc1062717bb`

`check-librashader-package.py --input <directory-or-zip> --platform windows`
passed on directory and ZIP. Packaged EXE matched the build with `cmp`; all
five pre-existing guest-driver `SHA256SUMS` entries passed. This is payload
validation, not proof of DLL dependencies being installed on the target.

## Dependency and evidence boundaries

librashader remains v0.12.0 / commit
`87e8a97b50516d997defeaa168173dcd185d4022`, C API 5 / ABI 2.
The official x64 DLL remains SHA-256
`1890f647c7fbe52d4cc591526db24367caca284996855c4565c6003c7e46f8cc`.
No vendor shader or runtime changes; existing MPL-2.0 / MIT / Unlicense
decisions remain in force. No emulator-core/raw-capture changes.

[D3D11 rasterizer defaults](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc)
document back-face culling and clockwise fronts.
[DirectX runtime](https://www.microsoft.com/en-us/download/details.aspx?id=8109)
supplies the legacy D3DX libraries; the
[VC++ runtime](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)
supplies the MSVC runtime imports.

Remaining evidence: the maintainer's loader log, actual CRT output, SDL/native
switching and DPI input behavior on the affected Windows machine. G99-3 and
performance gates remain open. No measured physical GPU performance is claimed.
