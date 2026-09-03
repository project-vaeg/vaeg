<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source and binary forms must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M99 progress log

Branch: `topic/m99999-misc-fixes` (M99 work retained on the existing user branch)
Starting commit: `1e73f2b2c20b09b2ac97c58590014cc413401d5c`
Working policy: one focused commit per sub-milestone; unrelated dirty paths
are intentionally not staged.

| Sub-milestone | Commit(s) | Result | Report |
| --- | --- | --- | --- |
| M99a | `46283303` | PASS: presentation and raw-capture boundary inventoried | `m99a_librashader_presentation_inventory.md` |
| M99b | `e11d7146` | PASS: runtime and shader provenance/license closure frozen | `m99b_librashader_provenance.md` |
| M99c | `c26325ff` | PASS: bounded OpenGL backend selected | `m99c_librashader_backend_selection.md` |
| M99d | `0b3bedac` | PASS: optional CMake switch | `m99d_librashader_build_switch.md` |
| M99e | `2c5dbb87`, `ca0b63e1` | PASS: official loader, ABI/API and symbol guards | `m99e_librashader_loader.md` |
| M99f | `cca93104` | PASS: VAeg-owned BSD pass-through | `m99f_librashader_passthrough.md` |
| M99g | `e28f7203` | PASS: persistent framebuffer/GL bridge | `m99g_librashader_source_bridge.md` |
| M99h | `752e9aa0` | PASS: audited Lottes-fast bundle | `m99h_lottes_fast_bundle.md` |
| M99i | `31350ce7` | PASS in code/static scope; GPU visual evidence deferred | `m99i_librashader_present_filtered_output.md` |
| M99j | `6e5a1a7d` | PASS in code/static scope; physical resize/fullscreen deferred | `m99j_librashader_geometry.md` |
| M99k | `3829593c` | PASS: runtime metadata enumeration | `m99k_librashader_parameters.md` |
| M99l | `1017b531` | PASS in code/static scope; physical UI deferred | `m99l_librashader_settings_dialog.md` |
| M99m | `37deb6a6` | PASS: Off/Recommended/Custom selection | `m99m_librashader_preset_selection.md` |
| M99n | `9da68906`, `47dba762` | PASS: versioned settings and missing-custom recovery | `m99n_librashader_persistence.md` |
| M99o | `a5c6c546` | PASS: raw/presented capture separation | `m99o_librashader_capture_semantics.md` |
| M99p | `58dd1fae` | PASS in code/static scope; context-loss display test deferred | `m99p_librashader_lifecycle.md` |
| M99q | `6b1e0394` | PASS: bounded presentation instrumentation | `m99q_librashader_performance.md` |
| M99r | `fb1d0bbe` | PASS: platform package license/source/shader records | `m99r` changes and release workflow |
| M99s | `0aa7278e`, `70c6a443`, `36a68848` | PASS in workflow definition; hosted feature-on build/runtime fallback checks passed, full suite had unrelated baseline failures | `m99s_librashader_platform_matrix.md` |
| M99t | `d241a497`, `10aedf7b`, `02c43122`, `7e673260`, `34295654`, `0fa37564`, `2680fa54`, `9abd6c97` | PASS: source-texture completeness correction, 9 static QA checks, Linux Mesa pass-through/Lottes smoke, resize/fullscreen/filter-toggle/context-recreation smoke, benchmark, and staged package inspection | `m99t_librashader_automated_qa.md` |
| M99u | `21f22908` | Documentation and final evidence log updated; physical display lifecycle/HiDPI/settings inspection, normal-hardware performance, and official release archive inspection remain deferred | this log and M99u docs |

## Tests and CI

Local tests completed before M99u:

```text
cmake --build /private/tmp/m99d-lib-on -j2 --target vaeg_sdl2       PASS
cmake --build /private/tmp/m99d-lib-off -j2 --target vaeg_sdl2      PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy .../vaeg --selftest    PASS (on/off)
python3 tests/frontend/librashader/test_librashader_pipeline.py \
  --root .                                                          PASS (9 tests)
cmake --build /private/tmp/m99-gl-test -j2 --target vaeg_gl_preset_smoke PASS
CTest static, ROM-less, and M75 compiled tests in m99t-lib-on-tests PASS
CTest static test in m99t-lib-off-tests                             PASS
```

Repository checks pass: UTF-8, LF EOL, path case, and scoped `git diff
--check`. Hosted CI run [`33717000722`](https://github.com/nakatamaho/vaeg/actions/runs/33717000722)
completed with overall failure from existing baseline tests. The M99-specific
checks passed on Ubuntu, Windows, and macOS: feature-on builds, runtime-absent
and runtime-present SDL dummy-device smokes, pinned runtime builds, and static
pipeline QA. Ubuntu additionally ran the Mesa software-OpenGL smoke: both
presets produced non-black RGB output, resize/fullscreen/filter-toggle and
context-recreation checks passed, and the 1920x1080 benchmark measured
3.901 ms/frame for pass-through and 73.117 ms/frame for Lottes-fast over 60
frames each. Ubuntu's staged release-shaped package inspection also passed.
The full CTest failures were unrelated missing/unavailable external regression
inputs: Ubuntu had 17 failures out of 84, Windows had 31 out of 97, and macOS
had 31 out of 102; these included existing Z80 compatibility/ABI and optional
external test inputs. The Windows and macOS jobs did not provide a GPU visual
run. Other jobs in the same run passed, including repository invariants, ASAN,
MinGW release, macOS SDL2 compatibility, and GCC/Clang compatibility.

## Decisions and deferred evidence

- librashader v0.12.0 at commit `87e8a97b50516d997defeaa168173dcd185d4022`,
  C API 5 / ABI 2, is loaded dynamically under MPL-2.0. No implementation is
  statically linked or copied into VAeg.
- The only bundled CRT source is the pinned `crt-lottes-fast.slang` with its
  audited no-dependency closure. CRT-Geom, CRT-Royale, Mega Bezel, the full
  `slang-shaders` tree, GPL, and unknown-license shaders are excluded.
- Raw deterministic captures stay before presentation filtering. Presented
  captures are separately named and are not goldens.
- Local macOS has no usable SDL display/OpenGL context (`glxinfo -B` reports
  `unable to open display`); Xvfb and Weston are unavailable. Local Rust/Cargo
  is also absent. The hosted Ubuntu Mesa smoke proves the corrected shader
  source-sampling path for the two bundled presets. The same virtual-display
  run covered resize/fullscreen/filter toggling/context recreation and
  measured the bounded software-Mesa cost above, but it does not prove
  physical-display HiDPI behavior, normal-hardware performance, or
  cross-platform GPU rendering. Windows and macOS supplied build and fallback
  evidence but no GPU rendering smoke. The official release workflow was not
  invoked because it would publish a release; the staged Linux release-shaped
  package was inspected successfully, while final archive inspection remains
  deferred.

Current gate status: G99-1 PASS by reports M99a-M99c. G99-2 PASS for the
optional fallback contract and software-OpenGL preset rendering: runtime
absence remains non-fatal, runtime-present operation is covered on all three
platforms, and both bundled presets render non-black RGB under Ubuntu Mesa.
G99-3 has automated virtual-display evidence for resize/fullscreen and the
geometry path, but remains DEFERRED on physical UI/HiDPI behavior, settings
dialog/persistence inspection, and a real context-loss event. G99-4 has
automated lifecycle and software-Mesa performance evidence, but remains
DEFERRED on normal-hardware performance, physical lifecycle/recovery,
Windows/macOS GPU visual evidence, and final release-archive inspection. The
exact remaining action is to run the feature-on binary on supported physical
displays, exercise settings/lifecycle paths, measure normal hardware, and
inspect a final staged archive without publishing it. Known risks are
platform-specific OpenGL context and runtime packaging behavior; the existing
renderer remains the required fallback.
