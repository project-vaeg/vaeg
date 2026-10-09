<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 -->

# M99z6 — Windows CRT activation and native menu composition

Status: implementation and local checks PASS; physical Windows CRT retest pending.

Starting commit: `53678d960cc9b8963bdc3dfdff92f8bad92c1db2`.
Branch: `topic/m99-native-crt-rebuild`.
The separate main checkout and its pre-existing changes were preserved.

## Findings and correction

The maintainer confirmed that the M99z1 Windows build now displays the guest
but clarified that menus returned to a tiny, unreadable size, with no visible
CRT effect. This is not confirmation of restored menu sizing.
The previous package did not enable `NativeCRT` merely by including the DLL.
Furthermore, `np2.c:main` skipped GUI initialization whenever a native
presenter was active. Its menu was therefore unavailable to disable CRT or
adjust parameters. The actual DLL load state on the maintainer's machine was
not established by that report.

Windows now uses the pinned ImGui D3D11 backend on the presenter's own device.
GUI draw data is composed after the guest shader and before DXGI Present.
The base font is 16 pixels, scaled with Windows effective DPI (without
double-applying framebuffer scaling). Windows requests per-monitor-v2 DPI
awareness before video initialization. Screen > UI size offers persistent
100–300% overrides; `GUI_ui_scale=0` selects automatic DPI. Style dimensions
are recomputed from a saved baseline rather than cumulatively multiplied.
The common SDL viewport calculator
supplies both the guest rectangle and scaled menu inset. About image pixels
are registered once as ImGui-managed texture data. No full-frame UI surface is
allocated per frame, and the emulation/raw-capture path is unchanged.

The Screen menu enables CRT immediately, retains D3D11 ownership for on/off
comparisons, applies live parameters, and queues reload between GUI frames.
The title/menu distinguish filtered, pass-through, and failure states. The
bundled preset is resolved relative to the executable, with a development
working-directory fallback. The optional Windows launcher
`start-native-crt.cmd` requests CRT using a session environment override and
writes `native-crt.log`; existing configuration is not overwritten by the
launcher.

Filter initialization failure retains a D3D11 pass-through image. The controller
now honors a recovered filter by drawing pass-through immediately, instead of
unnecessarily dropping to SDL. Total native failure releases GUI objects before
creating SDL resources and uploads the preserved shadow framebuffer. D3D11
resize unbinds the old render target, teardown flushes deferred resources, and
device recovery copies remembered paths before initialization and retains the
guest/menu viewport.

## Dependencies and licensing

No SDL, ImGui core, librashader, shader or runtime version was changed.
The added official ImGui D3D11 files are byte-identical to commit
`8936b58fe26e8c3da834b8f60b06511d537b4c63` (v1.92.8), under MIT:

| File | SHA-256 |
| --- | --- |
| `imgui_impl_dx11.cpp` | `1c0c3af25b45dceb4c45de7da591244f4935718154e7149d0e90858958c6f19d` |
| `imgui_impl_dx11.h` | `fb8a314a6a2904dcc5bfeb938a169389e170f233ad367d33f1ce5c7bde586a7c` |

ADR-0002 and ADR-0004 document the native GUI extension. The Windows stage
includes the ImGui MIT license. librashader v0.12.0 remains dynamically loaded
under MPL-2.0, API 5 / ABI 2. The previously audited Unlicense shader closure
is unchanged; VAeg remains BSD-2-Clause.

## Local validation

Commands executed from the topic worktree:

```sh
cmake --preset mingw-cross
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
CCACHE_DISABLE=1 cmake --build build/macos-ci -j4
CCACHE_DISABLE=1 cmake --build build/macos-macports -j4
ctest --test-dir build/macos-ci --output-on-failure \
  -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
ctest --test-dir build/macos-macports --output-on-failure \
  -R '^(vaeg_librashader_|vaeg_romless_tests$|vaeg_sdl_startup_viewport$)'
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

MinGW feature-on, macOS FetchContent feature-on, and MacPorts feature-off
builds passed. Both macOS focused suites passed 10/10, including ROM-less
selftest and startup viewport smoke. Repo checks reported zero violations.

The focused tests were also built separately with assertions enabled:

```sh
cmake -S . -B build/m99-gui-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_ARCHIVE_DROP=OFF
cmake --build build/m99-gui-debug --parallel 4 --target \
  vaeg_librashader_controller_test vaeg_librashader_d3d11_lifecycle_test \
  vaeg_librashader_frame_input_test vaeg_librashader_capture_boundary_test \
  vaeg_librashader_presenter_state_test vaeg_librashader_pass_through_test \
  vaeg_librashader_shader_parameters_test vaeg_librashader_fallback_test
ctest --test-dir build/m99-gui-debug --output-on-failure -R '^vaeg_librashader_'
```

Result: 8/8 PASS. The first configure omitted the mandatory trace option and
was correctly rejected; the command above supplies it. Existing Release
assert-based tests alone are not treated as assertion coverage.

The two new tests use explicit checks in Release and Debug. They test 100
controller toggles and 50 D3D11 presenter on/off pairs without device
reinitialization, recovered-filter retry, minimize, device failure/recovery,
GUI lifecycle forwarding, and remembered viewport/preset preservation. Their
backend doubles are deterministic unit evidence, not real GPU evidence.

Existing build warnings remain: a logical-not expression in
`vram/maketextva.c`, Apple duplicate-library/alignment notices, and dependency
CMake deprecation/cross-compilation notices. No unrelated warning was changed.
Rebuilding after DPI support also exposed existing `kbdmap.c` selftest
uninitialized-local warnings. An initial DPI build failed due to the legacy
`min`/`max` macros; parenthesized standard-library calls corrected that error.
All three builds and both 10-test suites subsequently passed again.

## Windows package evidence

Implementation: [271cb211](https://github.com/nakatamaho/vaeg/commit/271cb2117cc369694437668a16a2d9500409acdd).
After committing, reconfigured and rebuilt `mingw-cross` to embed the committed
identity. Package: `build/mingw-cross/vaeg-m99z6-windows-x86_64.zip` (local,
not tracked). It includes `vaeg.exe`, `librashader.dll`, CRT assets, licenses,
the native CRT guide and `start-native-crt.cmd`.

SHA-256:

- ZIP: `7906d779cc21d8e53ba71839c76341bede6a22e3c3fdbc27697974ed2c484c10`
- EXE: `5aae4867b8297d0315fcc453d668a8be765f43f39dbcc10e324dec6e7e6a6919`
- DLL: `1890f647c7fbe52d4cc591526db24367caca284996855c4565c6003c7e46f8cc`

`check-librashader-package.py --input <directory-or-zip> --platform windows`
passed for both forms. Packaged EXE matched the build with `cmp`; the existing
guest-driver `SHA256SUMS` verified all five entries. PE imports include D3D11
and the Windows `D3DCOMPILER_47.dll`; SDL2 and the MinGW C++ runtime are static.
No private ROM or boot media is included. Windows execution remains untested.

## Evidence still required

Physical D3D11 shader output, GUI mouse hit testing, monitor DPI changes,
fullscreen/minimize/recovery, and GPU performance need a Windows retest.
No physical Windows execution or performance measurement was possible here.
G99-3 and G99-7 remain open. The native Metal/OpenGL GUI limitation also
remains; this Windows fix does not establish G99-4/G99-5 completion.

Unpack the whole new Windows package and run `start-native-crt.cmd`.
Confirm `Native CRT ON` in the title, compare Enable on/off in the Screen
menu, then resize/fullscreen/minimize. If it reports pass-through/fallback,
`native-crt.log` and the menu's preset error provide the next diagnostic.
