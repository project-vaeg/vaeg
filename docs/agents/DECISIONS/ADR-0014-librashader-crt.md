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

# ADR-0014: librashader CRT runtime and shader boundary

Date: 2026-09-04

Status: Accepted for M99

## Decision

M99 consumes the official librashader C API. By maintainer amendment, M99 QA
and release builds statically link the pinned C API archive with the required
platform feature. The official `librashader_ld.h` dynamic loader remains
available only for an explicitly non-release developer diagnostic build. Static
builds bind the C API at link time and must not require a librashader shared
library at startup. The MPL implementation stays separately licensed; notices
and corresponding source references must accompany the binary. The vendored
files are placed under `external/librashader`
because this repository's convention requires third-party material under
`external/`, rather than the task proposal's `third_party/` path.

The pin is the official `librashader-v0.12.0` release:

| Field | Value |
| --- | --- |
| Repository | https://github.com/SnowflakePowered/librashader |
| Release | `librashader-v0.12.0` |
| Commit | `87e8a97b50516d997defeaa168173dcd185d4022` |
| Source archive SHA-256 | `4bf8cf2489d00848dcabbf2163204093776082da4217d5a5db45e4cbf335cedf` |
| C API version | `5` |
| C ABI version | `2` |
| Implementation license | MPL-2.0 (`LICENSE.md`) |
| C headers/loader license | MIT, as stated by the upstream include README |
| `librashader.h` SHA-256 | `5d478897c391af3f60015810b67785ae1a286d262a845485276e36ded9f21e62` |
| `librashader_ld.h` SHA-256 | `bcffcbc854afb287c9f935c1a0e3b569f5e6775ef85914bd7f7025ad1f6bde33` |
| `LICENSE.md` SHA-256 | `69c15395f33bc9ce8e1d8b6cef42b7e49cdec4c6f5233d4b9cfc4bfa335f97f9` |

The following audit describes the original dynamic-runtime inputs. The
official release assets audited so far are:

| Artifact | SHA-256 |
| --- | --- |
| `librashader-aarch64-macos-v0.12.0-optimized.zip` | `49808004a4904f6a99e0231092dcfdfe52b7b61f68430a4c9f1e165749c4c90e` |
| `librashader-x86_64-macos-v0.12.0-optimized.zip` | `8b2a50cefacf4073e8fa4757bec30242a788068c4096a580d94430688c184767` |
| `librashader-aarch64-windows-v0.12.0-optimized.zip` | `acea3871d7c1cf345ae9da47378b399d89a1e05496160a652103ba5b7b5eb775` |
| `librashader-x86_64-win7-windows-v0.12.0-optimized.zip` | `f40941e5db58fa9123da76c74bf17e52d73ef89514d0ab8960cd77a875a27c19` |
| `librashader-x86_64-windows-v0.12.0-optimized.zip` | `521fe0f364bfa705883f9e99fb1733a3a24f0f403bcc5591b6b78f6cff289183` |

The macOS archives contain `librashader.dylib` and the Windows archives
contain `librashader.dll`; the archives also contain upstream static-library
files; MinGW static builds require a compatible GNU-target build rather than
assuming that the MSVC archive is compatible. No Linux binary is published in
the audited release assets. The Linux OpenGL runtime must therefore be built
from this exact source pin in the Linux packaging/CI stage, with its resulting
artifact hash recorded before G99-6.

The following shared-runtime files are historical developer-package inputs;
they are not required or shipped by M99 static QA/release packages:

| Platform | File | SHA-256 |
| --- | --- | --- |
| Linux | source-built `librashader.so` | `8433577cf2e6872f4e4152e42832d52b48c632fd55b0897da9aae17b0eba3574` |
| macOS x86_64 | `librashader.dylib` from the pinned optimized release | `f1699370b8a01be5df108a7dbb29a8f2c10ef6d701b14d0a830d55084e879f10` |
| Windows x86_64 | `librashader.dll` from the pinned optimized release | `1890f647c7fbe52d4cc591526db24367caca284996855c4565c6003c7e46f8cc` |

These are staged-file hashes, not source-tree payloads. The full release-asset
hashes above remain the provenance identity for the official macOS and Windows
archives.

## Shader closure

The requested `crt-lottes-fast.slang` candidate was audited at the pinned
`libretro/slang-shaders` commit recorded in
`assets/shaders/crt/licenses/crt-default-provenance.md`. Its preset references
only that shader and has no include, LUT, texture, or secondary-pass closure.
The shader contains an explicit Unlicense/public-domain dedication. No
CRT-Geom, CRT-Royale, Mega Bezel, complete `slang-shaders` tree, GPL shader,
or unknown-license shader is copied or distributed.

The current asset names are `assets/shaders/crt/vaeg_crt_default.slangp`,
`assets/shaders/crt/shaders/crt-lottes-fast.slang`, and the adjacent license
and provenance notices. The nested `shaders/` directory is intentional: it
preserves the relative path used by the audited preset, so librashader resolves
the shader from `PRESET_DIR` in both a checkout and a release package. The
preset and shader contents are byte-preserved from the audited upstream
sources; only the preset filename is local.

## Release staging

`tools/release/stage-librashader-assets.sh` copies the exact preset, shader,
license, provenance, and third-party notice closure into a platform package.
Static package creation additionally embeds the audited shader closure and
generated dependency notices in the executable. `librashader.so`,
`librashader.dylib`, and `librashader.dll` are not M99 package payloads.
`tools/release/check-librashader-package.py` validates the staged shader and
notice closure; the static-build manifest and CMake hash checks validate the
linked archive and dependency audit before linking.

## Consequences

- VAEG-owned sources remain BSD-2-Clause; the MPL-2.0 implementation retains
  its license and corresponding-source obligations with either linkage mode.
- A missing library, unsupported runtime, ABI mismatch, or shader failure can
  disable the optional filter without changing the existing SDL renderer.
- Platform runtime hashes and the Linux source-built hash remain release-gate
  evidence, not source-tree binaries.
