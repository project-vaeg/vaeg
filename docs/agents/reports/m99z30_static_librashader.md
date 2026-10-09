
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


# M99z30 - Static C API and embedded standard CRT assets

Starting commit: `b9f7f1bba4ddc3800743830bb06133d1d726b754`.
Branch: `topic/m99-native-crt-rebuild`. No main merge or release publication.

The maintainer permits static linkage and explicitly allows dependencies to
remain external if their licensing is problematic. Section 5 and the packaging
policy in the M99 task and ADR-0014 now permit both static and dynamic builds.

## Result and boundaries

- Windows GNU x86_64 and macOS arm64 static executables built successfully.
- librashader stays unmodified at
  `87e8a97b50516d997defeaa168173dcd185d4022`, release 0.12.0.
- Rust 1.88.0; Windows target `x86_64-pc-windows-gnu`, feature
  `runtime-d3d11`; macOS target `aarch64-apple-darwin`, feature
  `runtime-metal`. Builds use `--locked --release`.
- Static binding uses the existing C API table, with direct references to
  the functions VAEG consumes. Actual API=5 and ABI=2 are checked.
- The standard four-file CRT closure is embedded without changing any shader
  bytes. Extraction uses the working-directory content-addressed cache and
  UTF-8 filenames; configuration and explicit custom presets retain their
  existing ownership.
- The renderer, raw framebuffer, capture boundary, core and timing are unchanged.
- Dynamic builds remain the default development/CI mode. These are explicit
  static QA candidates, not a claim that all release jobs now build statically.

The upstream D3D cache enables DXIL reflection and references DxcCreateInstance
even in its D3D11 feature graph. The initial GNU executable therefore imported
dxcompiler.dll. `optional_dxc.cpp` supplies the direct and import-address
symbols and resolves the compiler through LoadLibraryEx only when called.
Absent DXC returns a normal HRESULT failure. This keeps DXC external and
optional; it does not substitute a shader compiler or modify upstream code.
The final PE imports contain no librashader.dll, SDL2.dll, dxcompiler.dll,
libstdc++ DLL, libgcc DLL or libwinpthread DLL. OS D3D11, D3DCompiler 47,
UCRT and system APIs remain external.

## License and source evidence

The build helper collects normal/build dependency identities, notices and
source references, rejects unknown declared license expressions and unresolved
notices, and emits `static-build.json`. CMake validates the pin and the archive
and notice hashes before accepting the static build.

Windows: 154 packages; macOS: 143 packages, including build dependencies
conservatively. librashader uses MPL-2.0, not the alternative GPL terms.
Other MPL components retain their source references and terms. Native-vendor
notices, Rust standard-library notices and Windows GCC runtime terms (with
the GCC Runtime Library Exception) are embedded in About. Long notices use
ImGui's unwrapped, vertically clipped text path to avoid wrapping megabytes
of text every UI frame.

Workspace-root notice retrieval, UTF-16 decoding and the two manifest-only
MIT declarations are documented in
[static build instructions](../../modernization/static-librashader-builds.md).
This records the selected sources and notices, not legal certification of
arbitrary replacement libraries. No upstream implementation or shader source
was modified. CMake license-header extraction also now starts at the first
comment delimiter, including shader files with a preceding version directive.

Static archive SHA-256:

- Windows: `960b059c0521c926fc11fec2585f4bff3b2a6f23f634b9038497ef52ded629d5`
- macOS: `14e8db70a47a55092513cb31b4481a9dd79b7b0c7286971ea003e13b14684a46`

The ignored audit directories retain the complete generated manifests,
dependency/source identities, license bodies and their hashes.

## Validation

Reproducible configure/build commands and environment settings are in the
[build instructions](../../modernization/static-librashader-builds.md).

- `cmake --preset mingw-cross`, then
  `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j6`: PASS with the
  documented static library, native library list and generated notice options.
- `cmake --preset macos-release`, then
  `CCACHE_DISABLE=1 cmake --build --preset macos-release -j6`: PASS with the
  documented static options, tests enabled and Z80 integration tracing enabled.
- `ctest --test-dir build/macos-release -R 'vaeg_static_|vaeg_third_party_licenses|vaeg_romless_tests|vaeg_sdl_startup_viewport|vaeg_.*(capture_boundary|frame_padding)' --output-on-failure`:
  7/7 PASS, final run 9.69 seconds.
- Actual static C API probe: `ABI=2 API=5; embedded preset parameters=9; SCREEN_SIZE=present`.
  No librashader dylib or assets directory is present in its test working directory.
- Cache test: fresh Japanese UTF-8 working directory; exact four-file byte
  comparison; repeat invocation preserves mtimes; one controlled corruption
  after a passing fixture rejects with `VAEG_SHADER_CACHE_MISMATCH`.
- Feature-off macOS build:
  `cmake -S . -B build/m99-static/feature-off -G Ninja -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_ARCHIVE_DROP=OFF -DCMAKE_PREFIX_PATH=/opt/local -DCMAKE_BUILD_TYPE=Release`,
  followed by `CCACHE_DISABLE=1 cmake --build build/m99-static/feature-off -j6`: PASS.
  Its dummy SDL `--smoke --no-cfg --no-bkupmem`: PASS.
- `x86_64-w64-mingw32-objdump -p build/m99-static/vaeg.exe`: PE import inspection PASS.
- `otool -L build/m99-static/vaeg-macos-arm64`: no librashader dylib dependency.
- Encoding, EOL and case validators plus `git diff --cached --check`: PASS.

Intermediate issues resolved locally: test configure initially needed the
required tracing option; overlapping initial configure attempts were stopped
and the final build was rerun; missing cache source after stale configure was
resolved by reconfiguration; the first DXC adapter needed both direct and
import-address symbols. No failed intermediate binary is the handoff artifact.

## Handoff and deferred evidence

Generated artifacts (not committed):

- `build/m99-static/vaeg.exe`, stripped PE x86_64, approximately 43 MiB.
  SHA-256 `0087333998255218aad43a0e3726c31652a42d974e064cc9b055ccff578b4cd6`.
- `build/m99-static/vaeg-macos-arm64`, approximately 25 MiB.
  SHA-256 `37793131d000d782b07801449eae0695d0a5aab8382cd001d59a2408a515edf0`.

Windows execution and CRT GPU rendering have not been tested in this
environment (no Wine/Windows host). The macOS tests exercise the static API,
preset and ROM-less paths, not a new Metal visual/performance gate.
The local macOS static archive contains native objects built for macOS 26.5
while the final link defaults to 26.0; older-macOS deployment needs a consistent
deployment-target rebuild. Existing duplicate-library/common-alignment linker
warnings remain. Native Linux and Intel macOS static builds are not tested.

No whole-M99 completion or new G99 real-GPU acceptance is claimed. The next
manual check is Windows startup and default CRT rendering with only the new
exe, existing configuration and user-supplied ROM/media, without sibling
librashader.dll or standard shader assets.
