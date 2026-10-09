<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M100g: main RAM and backup-memory synchronization

Owner task: expose installed RAM capacity and optional BIOS backup-memory
synchronization under Menu -> Devices -> Main memory capacity.
Implementation: [fccc71f63310755b89f367cd4119cc8b1d8aba22](https://github.com/nakatamaho/vaeg/commit/fccc71f63310755b89f367cd4119cc8b1d8aba22).

## Behavior

- Main_RAM selects 256, 384, 512 or 640 KiB; default 640.
- Main_RAM_Auto selects reset-time capacity/checksum synchronization; default ON.
- Valid existing Main_RAM settings survive migration. Unsupported values warn
  and fall back to 640. A missing automatic-sync key retains the ON default.
- Startup and reset apply the configured RAM limit and backup-memory correction
  before CPU execution. Menu changes are persisted but leave the running RAM
  limit untouched until reset.
- Only the capacity bits and dependent checksum change in existing backup RAM.
  With automatic synchronization OFF there is no emulator capacity correction,
  including when a backup file is missing. Guest firmware may still write it.
- Configuration saving uses a checked temporary write and atomic replacement.
  Short writes, close errors and replacement failures report failure; the menu
  rolls back the setting and displays an error. --no-cfg also rejects persistence.
- The local vaeg.cfg was created with Main_RAM=640 and Main_RAM_Auto=true. It is
  runtime state, not a tracked configuration or private artifact.

## Validation and limits

- Linux Debug executable built successfully with
  `cmake --preset linux-debug -DVAEG_ENABLE_TESTS=OFF` and
  `cmake --build build/linux-debug -j 4`.
- Encoding and EOL repository checks completed without findings.
- Case check reports the pre-existing tracked INSTALL.md and MANIFEST.json;
  this change does not rename unrelated files or alter their allowlist.
- Existing main-RAM selftest setup now explicitly applies the reset boundary.
  No new tests were added; selftests, guest boot, GUI interaction and failure
  injection were NOT RUN. No runtime PASS is claimed.
- macOS build is NOT RUN. MinGW build completed as recorded below.
- Standard V3/demo/OS human gate remains NOT RUN. No later milestone was started.

See [memory policy](../../modernization/pc88va-main-ram-options.md) and
[frontend configuration](../../../sdl2/README.md#configuration).

## MinGW handoff

- Source: `7e18026ac4d8648debe78a44c99030e8780e9fe0`.
- Ubuntu 24.04 container, MinGW-w64 x86-64 POSIX compiler, Release build via
  `mingw-cross`, tests disabled. SDL2, archive support and the audited pinned
  librashader C API archive were linked statically.
- Artifact: `build/mingw-cross/sdl2/vaeg.exe`.
- Executable SHA-256: `b271bd49b8635db134304d0b4ee59c3275f14cb90b0bda8b2f0c3ce31e8a755a`.
- PE import inspection found Windows system DLLs only; no separate SDL2,
  librashader, libgcc, libstdc++ or winpthread runtime DLL is imported.
- Bundle: `build/vaeg-mainram-windows-x64.zip`, with notices and no ROM/media.
- Windows execution, GUI interaction and tests are NOT RUN.
