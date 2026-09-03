<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
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
# M99t: automated librashader QA

`tests/frontend/librashader/test_librashader_pipeline.py` is a GPU-independent
content and integration-boundary test. It checks the pinned file hashes,
shader denylist and dependency closure, optional dynamic-loader boundary,
non-per-frame filter-chain creation, raw-capture separation, settings
metadata/migration markers, and release-archive staging records.

The source-texture regression guard also checks that the OpenGL entry point for
`glGenerateMipmap` is loaded and that the source texture is completed before
the runtime samples it. This protects the librashader 0.12.0 source-sampler
behavior used by the pinned runtime.

The OpenGL smoke executable also exercises a resizable window, fullscreen
entry and exit, repeated filter disable/re-enable, renderer/context
recreation, and a bounded 1920x1080 benchmark. These checks run only when a
usable OpenGL context and runtime are available; they are not substituted for
the ordinary GPU-less fallback test.

It is registered as `vaeg_librashader_pipeline_static` in CTest and runs in
both feature-on and feature-off test configurations. It does not claim a GPU
rendering result.

Validation:

```text
python3 tests/frontend/librashader/test_librashader_pipeline.py --root .  PASS (9 tests)
cmake --build /private/tmp/m99-gl-test -j2 --target vaeg_gl_preset_smoke PASS
ctest --test-dir /private/tmp/m99t-lib-on-tests --output-on-failure \
  -R '^vaeg_librashader_pipeline_static$'                         PASS
ctest --test-dir /private/tmp/m99t-lib-off-tests --output-on-failure \
  -R '^vaeg_librashader_pipeline_static$'                         PASS
ctest --test-dir /private/tmp/m99t-lib-on-tests --output-on-failure \
  -R '^vaeg_romless_tests$'                                       PASS
ctest --test-dir /private/tmp/m99t-lib-on-tests --output-on-failure \
  -R '^vaeg_m75_transfer_info_compiled$'                          PASS
```

The feature-on test build also passed `vaeg_sdl2` compilation. The existing
SCSI, screenshot-scheduler, and invalid-debug-script negative-case messages
remain expected self-test output.

Hosted run [`33717000722`](https://github.com/nakatamaho/vaeg/actions/runs/33717000722)
passed the Ubuntu feature-on build, runtime-absent and runtime-present
fallback smokes, pinned runtime build, OpenGL smoke, and staged package
inspection. The Ubuntu OpenGL smoke used Xvfb with Mesa software rendering and
reported:

```text
pass-through: 60 frames at 1920x1080, elapsed 234.061 ms, average 3.901 ms/frame
lottes-fast:  60 frames at 1920x1080, elapsed 4386.994 ms, average 73.117 ms/frame
context recreation: passed
```

The same run's Windows and macOS feature-on jobs passed their build, runtime,
and fallback steps. Their complete CTest steps, and Ubuntu's complete CTest
step, failed only on the repository's pre-existing external/optional test
inputs; no M99-specific test failure was reported. The hosted software-Mesa
benchmark is evidence of bounded behavior and relative cost, not proof of
normal hardware performance.
