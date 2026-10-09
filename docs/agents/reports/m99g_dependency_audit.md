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
# M99g — librashader dependency audit

Status: PASS for the pinned headers, runtime boundary, and shader provenance.

The implementation consumes the official librashader C API through its dynamic
loader. The VAEG binary does not statically link librashader. The complete pin,
API/ABI identity, header hashes, official runtime archive hashes, and license
decisions are recorded in
[`ADR-0014`](../DECISIONS/ADR-0014-librashader-crt.md).

The selected release is `librashader-v0.12.0`, peeled commit
`87e8a97b50516d997defeaa168173dcd185d4022`, C API 5 and C ABI 2. The
implementation is MPL-2.0; the audited loader/header material is MIT. Runtime
archives are treated as optional platform inputs and are never committed to
the source tree.

The default CRT preset uses one `crt-lottes-fast.slang` pass. Its dependency
closure has no include, LUT, or secondary shader pass. The shader is recorded
as Unlicense/public-domain provenance from the pinned libretro
`slang-shaders` source; the complete shader suite is neither tracked nor
shipped. The closure and notices are under `assets/shaders/crt/` and
`docs/licenses/THIRD_PARTY_NOTICES.md`.

The release staging helper checks the fixed preset, shader, provenance,
licenses, notices, and platform runtime basename/hash. The package checker
rejects prohibited shader-suite names and wrong-platform runtimes. Both
runtime-free and optional-runtime archive shapes were inspected for Linux,
macOS, and Windows in M99x.
