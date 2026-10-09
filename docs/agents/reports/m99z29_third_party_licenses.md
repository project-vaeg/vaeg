Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

# M99z29 - Embedded third-party license browser

Task: maintainer-requested About > Third-party licenses, within
[M99](../tasks/M99_librashader_crt_pipeline.md).

Base: `2406e50db43b869b68f4a658bfe10b88d2e86bdf`.
Branch: `topic/m99-native-crt-rebuild`. Main is not merged or changed.

## Implementation

The About dialog opens a resizable, font-scaled modal with a component
selector, wrapped scrollable notice text, and a Copy text button.
Notices are compiled into the executable: opening the dialog performs no
filesystem reads, downloads, cache writes, or shader initialization.
The existing About size follows the UI font scale so the new button fits.

[Generator](../../../cmake/third_party_licenses.cmake) reads the existing
licenses and provenance files, including the actual MIT license header from
librashader_ld.h (the include README alone is not a license text).
CMake watches every input and regenerates the table after notice changes.

Coverage: legacy VAEG notice and asset provenance; SDL2 and fetched SDL's
HIDAPI and yuv2rgb notices; Dear ImGui and its three stb components; ymfm;
suzukiplan Z80; Noto Sans JP; enabled LibArchive and the fetched zlib/liblzma
stack; optional librashader runtime and C API; bundled CRT shader notices.
The archive and CRT entries follow build configuration. Test-only corpora,
external user presets, private ROMs, and operating-system components are
not represented as bundled components.

The four source-reference notices under docs/licenses identify the pinned
upstream release URLs. Fetched dependencies additionally use their actual
build-source notices. System-provided LibArchive uses the documented license
reference; arbitrary system dependency versions and externally replaced
librashader binaries are not audited by this UI. This is not a new static-link
dependency audit or a replacement for release-package notices.

## Validation

- `cmake --preset macos-release -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON`: PASS.
- `CCACHE_DISABLE=1 cmake --build --preset macos-release -j6`: PASS.
  Existing duplicate-library and Mach-O common-section alignment linker
  warnings remain.
- `python3 tests/frontend/test_third_party_licenses.py`: 2 tests PASS.
  Runs the generator independently with CRT on/off and archive support off;
  checks exact complete notice bodies, extracted MIT texts and unique entries.
- `ctest --test-dir build/macos-release -R 'vaeg_third_party_licenses|vaeg_romless_tests|vaeg_sdl_startup_viewport|vaeg_.*(capture_boundary|frame_padding)' --output-on-failure`:
  5/5 PASS, including ROM-less selftest, dummy-driver startup and raw-capture
  isolation.
- Repository encoding, EOL, case checks: PASS.
- Manual dialog interaction and Windows/Linux execution: not performed.
  No GPU visual acceptance or new M99 gate completion is claimed.

Output: `build/macos-release/sdl2/vaeg`. Earlier QA archives are unchanged.
