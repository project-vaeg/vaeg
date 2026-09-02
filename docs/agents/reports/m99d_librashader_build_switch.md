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
# M99d: optional librashader build switch

`VAEG_ENABLE_LIBRASHADER` is an opt-in CMake option and defaults to `OFF`.
The disabled configuration has no librashader include, runtime, or link
dependency and remains suitable for headless and ordinary builds. When it is
enabled, the target receives `VAEG_ENABLE_LIBRASHADER=1` and reports the
selected configuration during CMake generation; the loader and backend are
added by the following sub-milestones.

The switch is deliberately independent of the host audio/video defaults and
does not change SDL2 selection. Runtime shared libraries are not copied into
the source tree or linked statically.

## Verification

The existing `linux-debug` feature-off build and ROM-less selftest remain the
baseline. A separate configuration with `-DVAEG_ENABLE_LIBRASHADER=ON` is
expected to remain SDK-independent at this stage because no loader source has
yet been added; M99e will add the audited header and required compile-time
checks.
