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
# librashader CRT presentation pipeline

The M99 presentation boundary is:

```text
VAeg shadow framebuffer
    -> persistent SDL/GL source texture upload
    -> librashader OpenGL filter chain
    -> persistent output target and final presentation quad
```

`VAEG_ENABLE_LIBRASHADER` defaults to `OFF`. When enabled, the first vertical
slice uses the official librashader C API and dynamic-loader header with
OpenGL 3.3 as the bounded backend. The pinned C API is version 5, ABI 2, from
librashader v0.12.0 at commit
`87e8a97b50516d997defeaa168173dcd185d4022`. VAeg does not statically link or
copy the MPL-covered implementation.

The loader owns the dynamic-library reference and reports absent runtime,
ABI/API mismatch, and missing required symbols separately. The bridge owns
the persistent source texture, output texture, filter chain, and final quad
objects. Preset parsing, runtime parameter enumeration, shader compilation,
and filter-chain creation occur on preset load, not in the frame path.
Parameter values are metadata-driven and sent through the runtime API. On
context or renderer recreation the bridge destroys GL objects, rebuilds the
chain when a current context is available, and disables filtering on recovery
failure. The existing SDL path remains the fallback.

Raw captures call the shadow-framebuffer path before presentation. Presented
captures call the rendered target after the optional filter. No shader output
is used for ROM-less golden comparisons.

## Updating pinned inputs

Update the librashader tag, dereferenced commit, C API/ABI, retained headers,
MPL text, and source URL together in `third_party/librashader/` and the M99b
provenance report. Re-audit the runtime license choice and build with
`--no-default-features --features runtime-opengl`; do not add GPL runtime
features or static linking. Update the release helper and package notice when
the platform library name changes.

Update `crt-lottes-fast` only from a pinned upstream file. Recheck its full
include/image/LUT/generated-header closure, retained notice, SHA-256, and
denylist scan. The VAeg-owned preset is separate from the upstream preset.
Do not add the complete `slang-shaders` tree or CRT-Geom, CRT-Royale, Mega
Bezel, GPL, or unknown-license files.

## QA and packaging

The source-boundary QA is:

```text
python3 tests/frontend/librashader/test_librashader_pipeline.py --root .
cmake --preset linux-ci-gcc -DVAEG_ENABLE_LIBRASHADER=ON
cmake --build --preset linux-ci-gcc --target vaeg_sdl2
ctest --test-dir build/linux-ci-gcc --output-on-failure
```

Release jobs configure the feature-on target, build the pinned OpenGL-only
runtime with `tools/release/build-librashader-runtime.sh`, and inspect the
archive's runtime, shader, MPL, source, notice, and provenance paths. GPU
rendering, resize, fullscreen, HiDPI, and context-loss evidence must be
reported separately from dummy-video or static tests.
