Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# M99z32 - Static librashader policy and macOS QA build

Starting commit: `1d764c6049ba9d831ed942c9d1f482b8ec2b52bd`.
Branch: `topic/m99-native-crt-rebuild`.

## Policy

M99 QA and release builds now require static linkage to the official
librashader C API. The dynamic loader remains available only for an explicit
non-release developer diagnostic build. The release package must not require
or contain `librashader.dylib`, `librashader.dll`, or `librashader.so`.

## Static dependency audit

- Upstream release: librashader 0.12.0.
- Source revision: `87e8a97b50516d997defeaa168173dcd185d4022`.
- macOS feature: `runtime-metal`.
- Target: `aarch64-apple-darwin`.
- Rust used for this local QA build: `rustc 1.97.1` from MacPorts.
- Static archive SHA-256:
  `90cb4cc047aa7183b2e73849db1997b77dc6957060ac8f7f341399313a66c913`.
- Generated dependency-notice SHA-256:
  `83ff3f66c7ac4e3a2df93a997958587e455b819c1d8a54acc35e0baa54df9f22`.
- `static-build.json` reports zero unresolved notices.

The builder now recognizes both rustup's `share/doc/rust/` and MacPorts'
`share/doc/rustc/` Rust standard-library notice locations. No upstream source
was modified.

## QA result

The static arm64 executable is:

`build/macos-static-qa/sdl2/vaeg`

SHA-256:
`8eedcaa1721f4000c80bd113c38b2e21bf5b35b6f17b388174349e7126b77c75`.

`otool -L` shows only macOS system libraries and frameworks. It shows no
SDL2 or librashader shared-library dependency. The executable contains
librashader symbols and the embedded CRT preset test passes.

The focused test set passed 15/15:

```text
ctest --test-dir build/macos-static-qa --output-on-failure \
  -R '^(vaeg_static_preset|vaeg_static_cache|vaeg_librashader_.*|vaeg_romless_tests|vaeg_sdl_startup_viewport)$'
```

The dummy-backend smoke test also passed:

```text
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/macos-static-qa/sdl2/vaeg --smoke --no-bkupmem
```

The package checker passed for both the staged directory and the final
archive. The QA archive is:

`build/macos-static-qa/release/vaeg-qa-macos-arm64-static.tar.xz`

Archive SHA-256:
`179ba4d6bdf204c7dc013932712ab04908e2c1dd30c7d169a07a1d354458139e`.

The archive contains the audited CRT closure and generated static dependency
notices, but no librashader shared runtime. This is a local macOS arm64 QA
artifact, not a claim of Windows, Linux, Intel macOS, or physical GPU
coverage. Rust 1.88.0 remains the pinned reproducibility target in the build
instructions; the local MacPorts Rust 1.97.1 result is a compatibility QA
build and should not replace the pinned release-toolchain evidence.
