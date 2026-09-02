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
# M99e: dynamic loader and ABI guard

The feature-on target now compiles the official `librashader_ld.h` loader with
the OpenGL runtime enabled. `LibrashaderLoader` checks the reported ABI before
using the API, requires API version 5 or newer, and distinguishes absent
runtime, ABI mismatch, API mismatch, and missing required symbols in its
state. The loader is the only runtime boundary; VAeg does not statically link
or copy librashader implementation code.

The required symbols are checked against the loader's no-op functions before a
filter chain can be created. `unload()` clears all function pointers and is
safe on every failed initialization path. The upstream 0.12.0 loader itself
does not expose an OS handle or an unload function, so the VAeg wrapper opens a
matching platform handle before invoking the official loader and closes both
references on teardown when the official loader opened the module. This keeps
the vendored headers unmodified while giving VAeg an explicit owner for the
loader lifetime.

## Verification

`VAEG_ENABLE_LIBRASHADER=OFF` still configures without the vendored include
path. The feature-on configuration now compiles the loader source and links
the platform dynamic-loader library where required. Runtime-present tests are
deferred until a pinned librashader shared library and a usable OpenGL context
are available; missing-runtime startup remains a required fallback test.
