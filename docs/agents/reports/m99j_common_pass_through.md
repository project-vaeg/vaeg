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

# M99j common pass-through coverage

Status: PASS

The common layer now provides a caller-owned conversion buffer for explicit
RGB565/ARGB8888-to-RGBA8888 conversion. It preserves row origin, pitch, and
frame metadata without allocating in the conversion function. This is the
format-normalization seam used by native backends; it does not alter the
emulator's raw RGB565 framebuffer or raw capture API.

The focused pass-through test covers:

- RGB565 conversion with known red and green pixels;
- short destination pitch and short destination buffer rejection;
- 4:3 aspect viewport calculation at 1920x1080 and point mapping;
- unavailable-presenter fallback results;
- repeated shutdown and recovery calls on the unavailable presenter.

The test is registered as `vaeg_librashader_pass_through` and runs together
with the M99h frame-input, raw-capture, and presenter-state tests. The final
run passed 4/4 tests.

The implementation intentionally does not perform any device calls, shader
compilation, preset parsing, or allocation. Those operations belong to the
platform presenter lifecycle milestones.
