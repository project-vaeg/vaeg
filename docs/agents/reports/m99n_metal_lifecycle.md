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

# M99n — Metal lifecycle hardening

Status: PASS (lifecycle contract and build evidence); physical GPU lifecycle
deferred to G99-5

## Lifecycle behavior

The backend-neutral `NativePresenter` contract now includes a drawable-size
update operation. The Metal presenter applies non-zero sizes to the
`CAMetalLayer` in drawable pixels and treats zero dimensions as a temporary
disabled/minimized state. The layer's current drawable size is read again at
presentation time, so resize, fullscreen, and display-scale changes do not
reuse logical window dimensions.

`nextDrawable` returning nil is reported as a non-fatal disabled result; it
does not tear down the emulator or fabricate a frame. Input extent changes
recreate the reusable source texture and upload buffer. A resource failure
moves the presenter to `Unavailable`; `recover()` repeats initialization using
the retained window, drawable size, filter choice, and bounded preset path,
recreating the Metal view, device, queue, pipeline, source texture, and
optional librashader chain. Filter toggles keep the same native resources and
clear filter history when re-enabled.

No OpenGL context is created by the macOS path. SDL2 continues to own event
and window operations, while the native presenter owns Metal resources.

## Verification

```text
cmake --build /tmp/vaeg-m99l-build --target vaeg_sdl2 vaeg_librashader_presenter_state_test --parallel 4
ctest --test-dir /tmp/vaeg-m99l-build --output-on-failure -R 'vaeg_librashader_presenter_state$'
cmake --build /tmp/vaeg-m99l-off --target vaeg_sdl2 --parallel 4
```

Result: PASS. The feature-on macOS application and lifecycle contract test
built; the focused test passed 1/1. The feature-off macOS application also
built and linked with the legacy fallback presenter.

The focused test exercises the unavailable/fallback resize contract. A real
Metal window was not available in this environment (`The video driver did not
add any displays`), so resize/fullscreen/minimize/nil-drawable/device-recovery
pixel and timing evidence remains deferred to G99-5.
