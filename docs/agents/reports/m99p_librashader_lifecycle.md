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
# M99p: harden librashader lifecycle

Bridge creation and teardown are now centralized in the screen manager. The
optional build creates the bridge after the SDL renderer exists, destroys it
before renderer teardown, and can rebuild it after `SDL_RENDER_TARGETS_RESET`
or `SDL_RENDER_DEVICE_RESET`. Rebuild resets the frame counter and reloads the
selected preset only when filtering was requested.

The bridge now separates OpenGL-object cleanup from runtime unload. If the
current context has already disappeared, it avoids issuing GL deletion calls
or librashader filter-chain destruction through a dead context, while still
dropping the owning C++ state. Repeated preset loads clear the old chain and
parameter metadata before creating the new chain.

Validation:

```text
cmake --build /private/tmp/m99d-lib-on -j2 --target vaeg_sdl2  PASS
cmake --build /private/tmp/m99d-lib-off -j2 --target vaeg_sdl2 PASS
```

Physical context-loss, fullscreen, and device-reset execution remains deferred
because this environment has no usable SDL display or OpenGL context.
