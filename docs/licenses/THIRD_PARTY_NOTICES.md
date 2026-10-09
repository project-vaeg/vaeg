# VAEG third-party notices

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
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

## librashader

VAEG uses the official librashader v0.12.0 implementation through its C API,
either statically linked or optionally loaded at runtime. The implementation is
under the Mozilla Public License 2.0. The complete license is distributed as
`licenses/librashader-MPL-2.0.txt`; the upstream source and pin are recorded
in `docs/agents/DECISIONS/ADR-0014-librashader-crt.md` in the source tree.

The vendored C headers and loader are under the MIT license as stated by the
upstream `include/README.md`. The corresponding notice is distributed as
`licenses/librashader-headers-MIT.txt`. Static builds preserve the MPL-2.0
license of librashader and embed its license and corresponding-source reference
in About > Third-party licenses. The corresponding upstream source is
https://github.com/SnowflakePowered/librashader/tree/87e8a97b50516d997defeaa168173dcd185d4022.

## Default CRT shader

The bundled `crt-lottes-fast.slang` shader is from the audited
`libretro/slang-shaders` source and carries an Unlicense/public-domain
dedication. Its preset, license notice, dependency closure, source commit,
and hashes are recorded beside the shader under
`assets/shaders/crt/licenses/` in the source tree. The complete
`slang-shaders` repository and GPL or unidentified shader families are not
distributed.

The active default uses VAeg's BSD-2-Clause `vaeg-crt-aa.slang` and
`vaeg-scanline-aa.inc`, implementing the referenced public-domain Lottes
display equations with pixel-footprint scanline-brightness antialiasing.
The original shader is retained unchanged. VAeg's new code does not alter
the original dedication or attribution; the complete active dependency
closure and both licensing boundaries are recorded in the same provenance.

## Other bundled assets

The general VAEG asset notice, including the MIT-licensed suzukiplan Z80
component and the SIL-licensed Noto Sans JP font, is distributed as
`assets/NOTICE.md` together with the applicable license files.
