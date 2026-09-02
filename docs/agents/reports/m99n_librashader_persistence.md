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
# M99n: persist versioned filter settings

The existing INI configuration now stores a versioned CRT-filter schema:

- enabled state;
- `off`, `recommended`, or `custom` preset identity;
- the last custom preset path; and
- bounded per-identity parameter overrides.

Overrides use the version-1 `identity|name=value;` representation. Startup
loads the selected preset first and applies only the matching identity's
metadata-backed values, so ranges remain enforced by the runtime API. Unknown
or malformed override entries are ignored. Invalid schema versions or preset
identities recover to the disabled recommended configuration. A missing
custom preset also disables filtering for startup and selects the Recommended
identity while retaining the last custom path for later recovery; the SDL
renderer remains available.

The optional build now creates the OpenGL presentation bridge even when the
filter starts disabled, allowing the settings dialog to enable it without
changing the feature-off build or requiring a restart. A runtime-absent or
non-OpenGL host still falls back to the existing SDL presentation path.

Validation:

```text
cmake --build /private/tmp/m99d-lib-on -j2 --target vaeg_sdl2  PASS
cmake --build /private/tmp/m99d-lib-off -j2 --target vaeg_sdl2 PASS
```
