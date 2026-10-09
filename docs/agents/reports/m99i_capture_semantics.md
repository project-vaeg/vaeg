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

# M99i raw capture semantics

Status: PASS

The raw framebuffer remains upstream of all native presentation work. The new
`vaeg_frame_input_initialize()` helper stores a borrowed pointer and scalar
metadata only; it does not copy, convert, clear, or mutate source pixels.

The focused capture-boundary test uses a two-row RGB565 buffer with padding,
checks pointer identity and pitch preservation, validates the frame, and
compares every source byte before and after the operation. The existing raw
capture path remains `scrnmng_save_guest_frame()` over the RGB565 shadow buffer;
the scaled/effected render-target path remains a separate operation.

## Test

```text
cmake -S . -B /tmp/vaeg-m99h-build.quXdyw -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_WERROR=OFF
cmake --build /tmp/vaeg-m99h-build.quXdyw --target \
  vaeg_librashader_frame_input_test \
  vaeg_librashader_capture_boundary_test \
  vaeg_librashader_presenter_state_test --parallel 4
ctest --test-dir /tmp/vaeg-m99h-build.quXdyw -R \
  'vaeg_librashader_(frame_input|capture_boundary|presenter_state)' \
  --output-on-failure
```

Result: 3/3 focused tests passed. Representative raw-vs-golden captures on
each native GPU are intentionally deferred to M99y; no filtered frame is used
as a raw or byte-exact QA artifact.
