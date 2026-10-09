# M99x — Shader and release packaging

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

Status: PASS for the machine-verifiable M99x packaging scope.

## Changes

The audited single-pass CRT closure is now staged with the same relative path
used by its preset:

```text
assets/shaders/crt/vaeg_crt_default.slangp
assets/shaders/crt/shaders/crt-lottes-fast.slang
assets/shaders/crt/licenses/crt-default-license.txt
assets/shaders/crt/licenses/crt-default-provenance.md
licenses/librashader-MPL-2.0.txt
licenses/librashader-headers-MIT.txt
licenses/THIRD_PARTY_NOTICES.md
```

The release workflow invokes `tools/release/stage-librashader-assets.sh` and
`tools/release/check-librashader-package.py` for Linux tarballs, Windows zips,
and macOS tarballs. The staging script accepts an optional exact-platform
runtime and records its SHA-256 in `licenses/librashader-runtime.sha256`.
Without that file, the package remains valid and the native CRT feature fails
closed to the existing renderer. No runtime binary is committed to the source
tree.

The dependency and license decisions remain in
`docs/agents/DECISIONS/ADR-0014-librashader-crt.md`. The default shader is
the audited Unlicense/public-domain file; no GPL, unknown-license, or complete
`slang-shaders` payload is tracked or staged.

## Verification

The following package checks passed:

```text
PATH=/usr/bin:/bin:/sbin:/opt/local/bin bash -n \
  tools/release/stage-librashader-assets.sh
PYTHONPYCACHEPREFIX=<writable-temp> python3 -m py_compile \
  tools/release/check-librashader-package.py
python3 tools/release/check-librashader-package.py \
  --input <linux-package.tar.gz> --platform linux
```

Fresh staged package directories passed for all three platforms both without
the optional runtime and with the tested runtime filename. Fresh Linux and
macOS tar archives and a Windows zip passed archive inspection in both the
no-runtime and optional-runtime shapes. The optional-runtime staging also
passed using the locally audited platform artifacts:

| Platform | Runtime | SHA-256 |
| --- | --- | --- |
| Linux | `librashader.so` (source-built) | `8433577cf2e6872f4e4152e42832d52b48c632fd55b0897da9aae17b0eba3574` |
| macOS x86_64 | `librashader.dylib` | `f1699370b8a01be5df108a7dbb29a8f2c10ef6d701b14d0a830d55084e879f10` |
| Windows x86_64 | `librashader.dll` | `1890f647c7fbe52d4cc591526db24367caca284996855c4565c6003c7e46f8cc` |

The Windows archive checker rejected a deliberately wrong-platform Linux
runtime fixture, confirming that platform mismatch is fail-closed. Source
encoding, EOL, staged-package hashes, and `git diff --check` also passed.

Actual GPU startup and frame rendering remain M99y evidence; archive shape
validation does not claim the Windows, Linux, or macOS hardware gates.
