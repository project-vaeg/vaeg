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
# M99b: librashader provenance and license boundary

## Pinned runtime inputs

The selected runtime input is librashader `0.12.0`, published at the
annotated tag `librashader-v0.12.0`. The tag object is
`5537e838fc4680f5a9b2c0f4b9b669edea6e981b`; it dereferences to commit
`87e8a97b50516d997defeaa168173dcd185d4022`. The upstream C API reports API
version 5 and ABI version 2. VAeg will use the unmodified official C API and
dynamic loader, with the implementation supplied as a separately distributed
MPL-2.0 runtime. It will not statically link or copy the runtime implementation.

The retained upstream input hashes are:

| File | SHA-256 |
| --- | --- |
| `include/librashader.h` | `5d478897c391af3f60015810b67785ae1a286d262a845485276e36ded9f21e62` |
| `include/librashader_ld.h` | `bcffcbc854afb287c9f935c1a0e3b569f5e6775ef85914bd7f7025ad1f6bde33` |
| `LICENSE.md` (MPL-2.0) | `69c15395f33bc9ce8e1d8b6cef42b7e49cdec4c6f5233d4b9cfc4bfa335f97f9` |

Sources:

- [librashader v0.12.0](https://github.com/SnowflakePowered/librashader/releases/tag/librashader-v0.12.0)
- [librashader C API header](https://github.com/SnowflakePowered/librashader/blob/librashader-v0.12.0/librashader/include/librashader.h)
- [librashader dynamic loader](https://github.com/SnowflakePowered/librashader/blob/librashader-v0.12.0/librashader/include/librashader_ld.h)
- [librashader MPL-2.0 license](https://github.com/SnowflakePowered/librashader/blob/librashader-v0.12.0/LICENSE.md)

The upstream C API and loader headers carry MIT notices. The runtime crate
declares `MPL-2.0 OR GPL-3.0-only`; M99 selects and documents only the
MPL-2.0 implementation path. The alternative `LICENSE-GPL.md` is not a
retained input and is not copied to VAeg.

## Bundled shader candidate

The audited candidate is `crt-lottes-fast.slang` from
`libretro/slang-shaders` commit `4812a82f6c9a11cc8b5a7447040a98c9fc80c00e`.
Only its individual preset and shader are candidates for bundling; the full
shader repository is not vendored.

| File | SHA-256 |
| --- | --- |
| Upstream `crt/crt-lottes-fast.slangp` | `0992238001c519503d9e8e750b37b32d30ff380e74a40287863605ae69f9fff0` |
| VAeg `assets/shaders/crt/vaeg_crt_lottes_fast.slangp` | `8489d84dce043c5f68b7b6c7b7e3721bcad82b9b3498a8320f00fba7d6135835` |
| `crt/shaders/crt-lottes-fast.slang` | `576eddc662ac4f77909c0c14dbd5a16ac4164e50c67527fff634316f4441c482` |

Sources:

- [slang-shaders commit](https://github.com/libretro/slang-shaders/tree/4812a82f6c9a11cc8b5a7447040a98c9fc80c00e)
- [crt-lottes-fast preset](https://github.com/libretro/slang-shaders/blob/4812a82f6c9a11cc8b5a7447040a98c9fc80c00e/crt/crt-lottes-fast.slangp)
- [crt-lottes-fast shader](https://github.com/libretro/slang-shaders/blob/4812a82f6c9a11cc8b5a7447040a98c9fc80c00e/crt/shaders/crt-lottes-fast.slang)

The preset has one pass and references only the adjacent shader. The shader
has no include directive, image dependency, LUT, texture, or generated header.
Its retained source notice identifies the CRT-styled scalar implementation as
public domain/Unlicense material. That notice will remain adjacent to any
bundled copy. This file-level audit establishes a permissive closure for the
candidate; it does not grant permission for other files in the repository.

## Explicit exclusions

The following are never bundled, staged, or placed in release archives:

- `crt-geom*`;
- `crt-royale*`;
- `Mega_Bezel*`;
- the complete `slang-shaders` tree; and
- any shader or dependency whose license is GPL, unknown, or cannot be
  traced to a pinned source notice.

User-supplied local presets remain the user's licensing responsibility and are
loaded only by explicit path selection. Online shader download is out of scope.

VAeg's existing source license remains BSD-2-Clause. The repository has no
top-level `LICENSE` file; existing license evidence is retained in
`LICENSES/legacy-vaeg.txt`, `README.md`, `docs/legal/legacy-source-provenance.md`,
and the third-party notices already present in `external/` and `assets/`.
