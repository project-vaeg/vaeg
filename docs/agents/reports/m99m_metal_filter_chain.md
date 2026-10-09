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

# M99m — librashader Metal filter chain

Status: PASS (integration build and loader/preset evidence); GPU filter-chain
execution deferred to G99-5

## Implementation

The Metal bridge now loads the pinned librashader implementation through the
official dynamic loader when filtering is requested. It creates a preset and a
Metal filter chain from the existing `MTLCommandQueue`, and keeps the chain
owned by the same presentation-thread bridge as the native pass-through
resources. The preset is consumed by chain creation as required by the pinned
C API.

Filtered frames use the bridge's reusable RGBA8 source texture and call
`mtl_filter_chain_frame` on the current command buffer. The bridge supplies a
drawable-pixel viewport, source aspect ratio, source FPS, frame delta, frame
direction, and first-frame history-clear option. The chain is submitted and
presented on the same command buffer. Disabling filtering keeps the chain
allocated and returns to the native pass-through path; enabling it again
clears filter history without switching window ownership.

Shutdown frees the filter chain before Metal queue/device resources. Missing
runtime, incompatible ABI, absent preset, missing symbols, or chain creation
failure causes initialization to fail closed so the caller can retain the
existing renderer. No librashader static library is linked.

## Verification

Feature-on macOS build:

```text
cmake -S . -B /tmp/vaeg-m99l-build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=OFF
cmake --build /tmp/vaeg-m99l-build --target vaeg_sdl2 --parallel 4
```

Result: PASS. The vendored C API and Objective-C++ Metal bridge compiled and
linked without a librashader static library.

Pinned runtime loader/preset smoke test:

```text
/usr/bin/c++ -std=c++17 -x objective-c++ -Iexternal/librashader/include -Isdl2/librashader .tmp-m99-report-staging/m99m-loader-smoke.mm -o /tmp/vaeg-m99m-loader-smoke -framework Foundation -framework Metal
(cd /private/tmp/m99m-metal-runtime && DYLD_FALLBACK_LIBRARY_PATH=/private/tmp/m99m-metal-runtime /tmp/vaeg-m99m-loader-smoke /private/tmp/m99m-metal-runtime/null.slangp)
```

Result: PASS, reporting `api=5 abi=2` and successfully creating and freeing
the test preset. The temporary runtime was the arm64 `librashader-v0.12.0`
build with SHA-256
`1dbecea0c165fd0fddc2407ed4b8872f9f73f4fd2c3689a80ffc24c87e3fda2a`.

An end-to-end drawable/filter-chain frame was not run because this execution
environment has no usable Cocoa display (`The video driver did not add any
displays`). The Metal device, drawable, filtered pixels, and GPU performance
remain deferred evidence; no physical-GPU result is claimed here.
