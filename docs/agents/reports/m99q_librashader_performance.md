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
# M99q: measure the presentation path

The screen manager now collects optional debug metrics without logging each
buffer or frame: source-upload count and CPU time, filtered-frame count and
CPU time, rebuild count, output dimensions, and the fixed 640x400 source size.
With `--debug`, it emits one aggregate line per 60 filtered frames. The
diagnostic also records the established invariants of zero per-frame CPU
frame copies and zero per-frame allocations in the VAeg bridge. No preset
parsing, shader compilation, or filter-chain construction occurs in the frame
path.

The required 640x400-to-1080p, windowed/fullscreen, runtime-present, and
GPU-presentation measurements are deferred: this managed macOS environment has
no usable SDL display/OpenGL context and no pinned librashader shared runtime
has been staged. The source-side counters and reproducible debug command are
ready for the supported-hardware run:

```text
VAEG_CRT_FILTER=1 ./build/linux-debug/sdl2/vaeg --debug
```

This is recorded as deferred evidence, not as a performance pass.
