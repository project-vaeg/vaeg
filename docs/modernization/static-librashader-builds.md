
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


# Static librashader builds (M99z30)

M99 QA and release builds require static C API linkage under MPL-2.0. VAEG-owned
code remains BSD-2-Clause. Dynamic builds remain available only as an explicit
non-release developer/diagnostic configuration. This does not require
statically linking OS frameworks or components whose license or toolchain makes
that inappropriate.

## Build

Use Rust 1.88.0 and an unmodified checkout of librashader at
`87e8a97b50516d997defeaa168173dcd185d4022`. Keep it under an ignored build
directory, not in first-party sources. Install the Rust target before building.
The pinned upstream Cargo.lock is required.

For macOS arm64:

```sh
python3 tools/release/build-static-librashader.py \
  --source build/m99-static/librashader \
  --target aarch64-apple-darwin --output build/m99-static/macos-audit
cmake --preset macos-release \
  -DVAEG_STATIC_LIBRASHADER=ON \
  -DVAEG_LIBRASHADER_STATIC_LIBRARY="$PWD/build/m99-static/librashader/target/aarch64-apple-darwin/release/liblibrashader_capi.a" \
  -DVAEG_LIBRASHADER_STATIC_NOTICES="$PWD/build/m99-static/macos-audit/third-party-notices.txt" \
  '-DVAEG_LIBRASHADER_NATIVE_LIBS=c++;-framework Metal;-framework Foundation;-framework CoreFoundation;iconv;objc'
cmake --build --preset macos-release -j6
```

Set MACOSX_DEPLOYMENT_TARGET consistently for Rust/native dependencies and
CMake when producing builds for older macOS versions; the local QA build is
not a claim of compatibility with older releases.

For MinGW x86_64, install the Rust `x86_64-pc-windows-gnu` target and run
the wrapper. It checks out librashader at the pinned source revision, builds
the archive, and writes the dependency notices to the path used by the MinGW
CMake presets:

```sh
rustup toolchain install 1.88.0 --profile minimal \
  --target x86_64-pc-windows-gnu
rustup default 1.88.0
tools/release/build-mingw-static-librashader.sh
cmake --preset mingw-cross
cmake --build --preset mingw-cross -j6
python3 tools/release/check-static-windows-imports.py \
  --binary build/mingw-cross/sdl2/vaeg.exe
```

The native libraries are taken from rustc's `--print native-static-libs`;
the MinGW system-library equivalents satisfy Rust's winapi import archives.
All MinGW presets require static SDL2, LibArchive, zlib, lzma, librashader,
and MinGW runtimes. Their PE import audit permits only Windows system DLLs.
The D3D11 path uses the OS D3D compiler. The upstream shared D3D cache also
references DXC; the first-party optional_dxc adapter resolves DxcCreateInstance
only on demand and reports a normal HRESULT failure if absent. It does not
implement or replace the DXC compiler.

The helper also accepts native Linux and Intel macOS targets, but those static
configurations require their own build, native-link flags and runtime QA.
`--notices-only` reuses an already-built archive without recompiling it.

## Assets and configuration

Static builds embed the four-file standard CRT preset closure. On first
preset use, files are written to
`./vaeg-cache/shaders/<content-sha256>/`. Subsequent sessions verify and reuse
the files. The directory may be deleted when VAEG is closed and will be
regenerated. A mismatching cache fails closed with
`VAEG_SHADER_CACHE_MISMATCH`; it is not treated as a user shader.

The cache path follows the working directory. UTF-8 paths are passed to the
C API. A read-only working directory produces a diagnostic and the existing
presentation fallback. Extraction does not run per frame.

Standard shader assets and fonts require no separate runtime files. An
explicit custom preset remains an external file. Parameters still reside in
vaeg.cfg. ROMs and guest media remain user-supplied.

## Licenses and provenance

About > Third-party licenses embeds the generated static dependency notices.
The build helper records the source pin, Cargo.lock hash, static archive hash,
target, feature selection, package versions, license declarations and notice
hashes in static-build.json. CMake verifies the archive and notice identities
and rejects unresolved notices.

The helper includes normal and build dependencies conservatively, plus
native-vendor notices, Rust standard-library notices, and the Windows GCC
runtime terms including the GCC Runtime Library Exception. It selects MPL-2.0 for librashader; all other recorded
expressions are preserved with their available texts, including AND obligations.
MPL dependencies such as smartstring/persy retain their terms and corresponding
source links. No upstream implementation files are modified.

Some crate packages omit workspace-root license files. These are retrieved
at the Git revision recorded inside the crate, never from a moving branch.
The glslang workspace notice is UTF-16 and is decoded to UTF-8 for display.
sptr 0.3.2 and vec_extract_if_polyfill 0.1.0 declare MIT in Cargo.toml but
supply no standalone license at their recorded revisions; their declarations,
source links and standard MIT grant are included without inventing copyright
holders. The GNU winapi import archive uses the winapi-rs workspace MIT notice.

This is not a license assessment of arbitrary replacement archives. Keep the
generated manifest and notices with the build evidence. Dependencies with
unresolved licensing must remain external or block their static distribution.
