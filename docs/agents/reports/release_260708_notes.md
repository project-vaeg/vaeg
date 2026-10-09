<!--
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
-->

# 88VA Eternal Grafx Rel.260708

This is the first post-phase-2 source and binary release of 88VA Eternal
Grafx, the maintained portable fork of the PC-88VA emulator derived from
Neko Project II.

GitHub automatically provides source archives for the release tag. The
release workflow also attaches platform binary packages:

- `vaeg-rel260708-win-x86_64.zip`
- `vaeg-rel260708-linux-x86_64.tar.gz`
- `vaeg-rel260708-macos-arm64.tar.gz`

## Phase 2 Summary

- The active tree is now CMake-based and builds the portable C core with
  the SDL2 frontend.
- Dear ImGui provides the portable host GUI for media mounting, reset,
  save/load state slots, display scale/aspect controls, sound toggles,
  volume, and exit.
- The VA machine runs on `i286c/`, with the VA memory layer ported to
  `cpucva/memoryva.c` and the Z80 side in `cpucva/z80c.cpp`.
- CI covers Linux, Windows MinGW, and macOS builds plus ROM-less smoke
  and unit tests.
- VA behavior was verified through the phase-2 human gates: V3 boot, the
  bundled VA demo, and OS boot/simple operations on Linux, Windows, and
  macOS.

The frozen reference tier remains in source history and in the tree for
behavior archaeology: `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, and
`hlp/`. It is not the active release target.

## Runtime Package Contents

The binary packages include only runtime files that the portable frontend
loads from its own distribution:

- `vaeg` or `vaeg.exe`
- `assets/NotoSansJP-Regular.ttf`
- `assets/OFL.txt`
- `assets/NOTICE.md`
- `README-dist.txt`
- Windows only: `SDL2.dll`
- macOS package: a bundled SDL2 dylib layout when required by the
  FetchContent build

ROMs, disk images, and guest WAV payloads are not included. ROMs must be
extracted from hardware you own.

## Known Issues

- US physical host keyboards do not yet have a dedicated mapping mode for
  the JIS PC-88VA guest layout. Known symptoms include swapped `@` / `"`
  positions, no direct `:` key, and unreachable `-`, `^`, and yen-key
  positions. The numeric keypad path is not affected.
- State-save (`statsave`) files are not portable across architectures,
  compilers, build families, or operating systems. Treat a state file as
  tied to the exact build family that wrote it.
- The SDL2 `fontmng` implementation is still a ROM-less link stub used
  to satisfy shared core references. Host GUI text uses the bundled
  Dear ImGui font asset instead of the guest font ROM.

## Upgrade Notes

Rel.260708 stores writable files in the portable per-user state
directory:

- Linux: `$XDG_CONFIG_HOME/vaeg` or `$HOME/.config/vaeg`
- Windows: `%APPDATA%\vaeg`
- macOS: `~/Library/Application Support/vaeg`

Old exe-relative files from legacy builds are not migrated wholesale. If
you reuse an old `np2.cfg`, verify that it selects the VA machine, VA
Sound Board II, and the VA clock domain:

```ini
pc_model=88VA1
SNDboard=200
clk_base=3993600
clk_mult=2
```

`pc_model=88VA2` is also valid. Fresh first-run defaults are already set
for the portable VA target, but stale PC-98-era configs can halt at a V2
screen, leave the VA sound board unbound, or run in the wrong clock
domain.

Do not copy save-state files from old Win32 builds or from another
platform into this release. Reboot the guest and create new state slots
with the Rel.260708 binary instead.
