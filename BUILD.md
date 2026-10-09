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
# Build

The portable frontend is built with CMake and SDL2. The legacy Visual
Studio projects remain as reference artifacts and are not part of these
instructions.

The active main-CPU instruction core is under `cpu/upd9002/`; its public
entry points use `upd9002_core_*` and `upd9002_dispatch_*`. The separate
built-in register/port model is `io/upd9002_regs.*` and uses
`upd9002_regs_*`. The VA memory header is `memoryva/memoryva.h`.

## Linux

Install CMake 3.20 or newer, Ninja, a C/C++ compiler, NASM, and SDL2
development files. On Debian-like systems:

```sh
sudo apt install cmake ninja-build gcc g++ clang nasm libsdl2-dev pkg-config
cmake --preset linux-release
cmake --build --preset linux-release
```

When NASM is present, the normal build also produces the independently built
PC-Engine read-only HOSTFAT driver at
`build/linux-release/guest/hostfat.sys`. It is a generated integration
artifact and is not committed.

The Linux sanitizer smoke preset is:

```sh
cmake --preset linux-asan
cmake --build --preset linux-asan
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 \
  ./build/asan/sdl2/vaeg --smoke
```

## Windows (MSYS2 / MinGW-w64)

Use the MSYS2 MinGW64 shell. Install:

```sh
pacman -S --needed mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-pkgconf python git
```

Install Rust 1.88.0 with the GNU Windows target, then prepare the pinned
static librashader library and its third-party notices:

```sh
rustup toolchain install 1.88.0 --profile minimal \
  --target x86_64-pc-windows-gnu
rustup default 1.88.0
tools/release/build-mingw-static-librashader.sh
```

Build:

```sh
cmake --preset mingw-release
cmake --build --preset mingw-release
```

`mingw-release` builds a GUI subsystem executable and statically links SDL2,
LibArchive, zlib, lzma, librashader, and the MinGW GCC/C++/pthread runtimes.
Only Windows system DLLs remain as runtime imports. For a console debug build,
configure manually with `-DVAEG_WINDOWS_CONSOLE=ON`.

Linux cross-checks use:

```sh
cmake --preset mingw-cross
cmake --build --preset mingw-cross
```

The MinGW presets download the pinned SDL2 release recorded in ADR-0006
and build static third-party libraries. Windows release artifacts therefore
require only `vaeg.exe` at runtime. The import audit is:

```sh
python3 tools/release/check-static-windows-imports.py \
  --binary build/mingw-release/sdl2/vaeg.exe
```

MinGW ASan is not enabled in this tree;
sanitizer availability depends on the MinGW runtime/package set, so G11
keeps sanitizer acceptance on Linux and macOS.

## Maintainer Local Handoff

For the maintainer, "the usual place" means:

`/mnt/c/Users/maho/Dropbox/Documents/Emulators/PASOCON/NEC PC-88VA/vaeg_new/`

Copy `build/linux-release/sdl2/vaeg` to `vaeg` in that directory. Copy
`build/mingw-release/sdl2/vaeg.exe`, or
`build/mingw-cross/sdl2/vaeg.exe` when cross-building on Linux, to
`vaeg.exe`. Verify both copies byte-for-byte or with SHA-256. Do not copy or
modify any other files in this maintainer-local integration directory unless
the maintainer explicitly requests it.

## macOS Release

The release preset downloads pinned SDL2 and links it statically:

```sh
cmake --preset macos-release
cmake --build --preset macos-release
```

## macOS Development (MacPorts)

The M11 baseline is MacPorts, not Homebrew. Install:

```sh
sudo port install cmake ninja libsdl2 pkgconfig
```

Build:

```sh
cmake --preset macos-macports
cmake --build --preset macos-macports
```

The MacPorts presets set `CMAKE_PREFIX_PATH=/opt/local` and
`PKG_CONFIG_PATH=/opt/local/lib/pkgconfig`. The sanitizer preset is:

```sh
cmake --preset macos-asan
cmake --build --preset macos-asan
```

## Runtime State And Config

`vaeg.cfg`, `vabkupmem.dat`, and fixed GUI save-state slots are normally
stored in the portable per-user state directory:

- Linux: `$XDG_CONFIG_HOME/vaeg` or `$HOME/.config/vaeg`
- Windows: `%APPDATA%\vaeg`
- macOS: `~/Library/Application Support/vaeg`

An executable-local `vaeg.cfg` overrides the user-state configuration. An
existing executable-local `vabkupmem.dat` similarly overrides the
user-state backup memory, and saves return to the selected file. Save
states and keyboard sidecars remain in the user directory.

`vabkupmem.dat` does not fall back to the ROM path.

ROMs are read beside the active executable. VA uses unsuffixed names, while
VA2/VA3 uses MAME-compatible suffixed names:

```text
VA:       vadic.rom, vafont.rom, varom00.rom, varom08.rom, varom1.rom
VA2/VA3:  vadic_va2.rom, vafont_va2.rom, varom00_va2.rom,
          varom08_va2.rom, varom1_va2.rom
Extra:    vasubsys.rom
```

ROMs are not distributed and must be extracted from hardware you own. The
VA2 names follow MAME's `pc88va2` ROM declaration and do not fall back to VA
names. The current working directory is accepted only as a development
fallback. The frontend compares ROM size, CRC32, and SHA-1 with MAME and
warns without aborting when a file differs.

For VA booting, `vaeg.cfg` must use:

```ini
pc_model=88VA1
SNDboard=100
clk_base=3993600
clk_mult=2
sgp_mode=0
sgp_mult=1
```

For a VA with Sound Board II, use `SNDboard=200`. `88VA2` is also valid
for `pc_model` and uses `SNDboard=200` for its YM2608/OPNA. Stale configs
can cause a V2 halt, a silent FM-timer hang, or an invalid execution setting.
CPU x2 is standard; x1-x32 changes V30 execution capacity without changing
the fixed machine/peripheral clock. `sgp_mode=0|1|2` selects Model default,
Follow CPU, or Custom, and `sgp_mult=1..16` supplies the Custom scale. Startup
logs warn about invalid model/sound/clock settings but do not rewrite the
user's configuration.

Configuration files are UTF-8 only in the portable frontend. Obsolete
`np2.ini`, `np2.cfg`, and `vaeg.ini` files are not read.

## Known Issue

Short-term pacing jitter has been observed under WSLg: over a longer
sample the average tempo matches the v141 reference, but note timing may
briefly stretch and compress when WSLg presents stall. Native Windows is
the G11 measurement platform for deciding whether this is environmental.
