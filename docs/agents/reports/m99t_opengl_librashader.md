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

# M99t — Integrate librashader OpenGL

Status: PASS (dynamic GL C API integration and Linux build); GPU runtime deferred

## Implementation

The OpenGL bridge now loads the pinned librashader instance dynamically and
creates a GL filter chain with the repository's audited preset path. It passes
the SDL GL loader callback, GLSL 330, no-DSA mode, disabled shader cache, and
no-mipmap policy to the pinned C API. ABI and missing-runtime/symbol failures
leave the presenter on the existing fallback path.

Filtered frames use the uploaded source texture as `libra_image_gl_t` and a
reusable RGBA8 output texture as the filter-chain target. librashader's GL
chain owns its internal pass FBOs; VAeg owns the external output texture and
then draws that texture to the default framebuffer. The output texture is
reallocated only when the drawable extent changes. Frame options carry the
source aspect ratio, frame rate, frame delta, and first-frame history clear.
Toggle and teardown paths keep the GL context current and free the chain before
GL resources and context destruction.

The bridge restores the host GL state it borrows around the filtered or
pass-through draw, including framebuffer, viewport, bindings, unpack state,
clear color, and common enable flags. All chain calls occur on the same thread
as the context owner.

## Verification

The Linux Docker build compiled the OpenGL filter-chain integration and the
four focused frontend contract tests passed:

```text
cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_WERROR=OFF -DVAEG_FETCH_LIBARCHIVE=OFF
cmake --build /build --parallel 4
ctest --test-dir /build --output-on-failure \
  -R 'vaeg_librashader_(frame_input|capture_boundary|presenter_state|pass_through)$'
```

Result: PASS. The Linux PE-independent build produced the executable and the
focused tests passed 4/4. No real X11/Wayland display or OpenGL GPU was
available, so filtered pixels, context rebuild, and hardware performance remain
deferred to G99-4. Software rendering is not counted as the hardware gate.
