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
# M19 - Consolidate the portable runtime and VA sound hardware

Status: complete

Branch: `main` (the four implementation commits were integrated directly)

Gate: G19 passed

Depends on: G18 passed.

## Goal

Make the active SDL2 application and its release artifacts self-contained,
give the portable frontend a single configuration identity, align backup
memory with that configuration's portable lookup policy, and represent the
VA model's OPN/OPNA hardware explicitly.

M19 covers four already-integrated concerns:

1. embed the host GUI font and startup splash, and make release SDL2 linkage
   self-contained where supported;
2. replace the active frontend's `np2.ini`, `np2.cfg`, and `vaeg.ini` names
   with the single `vaeg.cfg` name, without compatibility fallback;
3. give `vabkupmem.dat` the same executable-local-first portable policy;
4. distinguish the VA built-in YM2203/OPN from the YM2608/OPNA hardware in
   VA Sound Board II and VA2/VA3.

## Constraints

- Do not modify `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, or `hlp/`.
- Do not restore compatibility lookup for `np2.ini`, `np2.cfg`, or
  `vaeg.ini` in the active frontend.
- Do not put writable backup memory in the ROM lookup directory.
- Keep NP2/ymfm synthesis-backend selection independent from OPN/OPNA
  hardware selection.
- Do not define `OPNGENX86`.
- Do not add ROM, disk, WAV, or other user payloads to the repository.

## Delivered runtime packaging

`assets/vaeg.bmp` and `assets/NotoSansJP-Regular.ttf` are canonical source
assets. CMake converts them into generated C byte arrays and links them into
the active executable. Normal startup displays the embedded VAEG splash for
approximately 1500 ms; smoke and selftest runs do not add that delay. Dear
ImGui loads the embedded Noto Sans JP data and no longer requires an external
font file.

`VAEG_STATIC_SDL2` controls static SDL2 linkage. Release presets which use
the pinned FetchContent SDL2 path enable it, so a Windows release does not
require `SDL2.dll` beside `vaeg.exe`. System-SDL development configurations
remain supported and do not silently claim static linkage when only a shared
package is available. License and attribution files remain in the release
documentation even when assets or SDL2 are linked into the executable.

## Configuration identity and lookup

The active SDL2 frontend uses only `vaeg.cfg`. At startup it selects the
first existing file in this order:

1. `vaeg.cfg` beside the executable;
2. `vaeg.cfg` in the platform user-state directory.

If neither exists, the frontend creates and subsequently updates the
user-state copy. Reads and writes continue using the selected path for the
process lifetime. The removed names are not migration aliases.

The user-state directory remains platform-specific:

- Linux: XDG state/config location selected by the existing SDL2 path shim;
- Windows: `%APPDATA%\vaeg`;
- macOS: `~/Library/Application Support/vaeg`.

## Backup-memory lookup

`vabkupmem.dat` follows the same portable-selection principle:

1. if an executable-local `vabkupmem.dat` already exists, load and save that
   file;
2. otherwise load and save the user-state `vabkupmem.dat`.

The executable-local file must already exist before startup; this prevents a
normal installed application from unexpectedly creating writable state in a
read-only executable directory. Backup memory no longer falls back to the ROM
path. The selected path is passed into the shared VA backup-memory module;
the frozen Win32 behavior is unchanged.

## VA OPN and OPNA hardware

M19 separates sound hardware identity from the NP2/ymfm FM synthesis choice.
The configuration values are:

| `SNDboard` | Emulated hardware | Availability |
|---:|---|---|
| `100` | VA built-in YM2203/OPN, 3 FM channels + PSG | VA only |
| `200` | YM2608/OPNA, 6 FM channels + PSG + rhythm + ADPCM | VA Sound Board II and VA2/VA3 |

Fresh VA configuration selects OPN. Fresh VA2/VA3 configuration selects
OPNA. VA may explicitly select OPNA to represent Sound Board II; VA2/VA3
cannot select the VA-only built-in OPN entry in the GUI.

At guest reset, `SNDboard` selects the board reset and bind path. OPN binds
the primary `44h/45h` ports. OPNA additionally binds `46h/47h`, the second FM
bank, rhythm, and ADPCM. With OPN selected, accesses to the unbound extension
ports are ignored on write and return `ffh` on read. Guest software can
therefore probe the installed capability through normal emulated hardware
I/O; the emulator does not infer the chip from ROM contents or register
writes.

The `opn_backend=np2|ymfm` setting from M17 chooses only the FM operator
implementation. It does not add or remove OPNA ports or peripherals.

## Machine verification

Run from a clean checkout:

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug

SDL_AUDIODRIVER=dummy ./build/linux-debug/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --smoke

cmake --preset linux-release
cmake --build --preset linux-release
cmake --preset mingw-cross
cmake --build --preset mingw-cross

git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py

git diff --stat HEAD -- win9x i286x cpuxva/memoryva.x86 hlp
rg -n "np2\\.(ini|cfg)|vaeg\\.ini|OPNGENX86" \
  sdl2 CMakeLists.txt CMakePresets.json
```

The final search may find historical prose only if explicitly labeled as
removed behavior. It must not find active compatibility lookup or a forbidden
`OPNGENX86` definition.

## Gate G19

G19 is a human portable-runtime and VA-sound gate:

### Distribution and startup

- launch the Windows-MinGW executable from Explorer, outside an MSYS2 shell;
- confirm it starts without external SDL2 or font assets;
- confirm normal startup displays the embedded splash for about 1500 ms;
- confirm `--smoke` and `--selftest` remain unattended and successful;
- repeat a normal launch on Linux and macOS where available.

### Configuration and backup memory

- with no executable-local file, confirm the application creates and updates
  only the platform user-state `vaeg.cfg`;
- place a valid `vaeg.cfg` beside the executable and confirm it overrides the
  user-state file and receives subsequent changes;
- confirm `np2.ini`, `np2.cfg`, and `vaeg.ini` are neither read nor created;
- with no executable-local `vabkupmem.dat`, change persistent VA settings and
  confirm the user-state backup file is updated and restored after restart;
- place an existing `vabkupmem.dat` beside the executable and confirm it is
  loaded, updated in place, and takes precedence over the user-state copy;
- confirm no `vabkupmem.dat` is written into the ROM path.

### VA sound hardware

- boot VA with built-in OPN, run the VA demo and an OS, and confirm FM and PSG;
- select OPNA on VA, accept the reset, and confirm Sound Board II material can
  use the added FM channels, rhythm, and ADPCM;
- boot VA2/VA3 and confirm OPNA is selected and the VA-only OPN GUI entry is
  disabled;
- verify sound-hardware selection persists across restart;
- repeat the checks with both NP2 and ymfm synthesis backends and confirm that
  changing backend does not change which hardware ports are exposed;
- verify configured FDD and SASI media survive sound-hardware resets.

### Standard VA regression

- boot V3 mode;
- run the bundled VA demo;
- boot an OS and perform simple disk and keyboard operations;
- exit cleanly.

G19 passed after the maintainer verified the portable distribution and
startup behavior, configuration and backup-memory precedence, VA and VA2/VA3
sound-hardware selection, media retention, and the standard VA regression
checklist.

## Commit record

```text
08e783c M19: embed frontend assets and static-link release SDL2
bf60ccb M19: rename portable configuration to vaeg.cfg
4d4f8a0 M19: align backup memory with portable config lookup
e50fbbd M19: add model-aware VA OPN and OPNA selection
```

## Residual risks

- OPN/OPNA presence is represented by port binding and chip behavior, not a
  separate board-identification register. Software using an undocumented
  detection sequence may require targeted comparison with real hardware.
- Static SDL2 depends on the selected CMake package exposing
  `SDL2::SDL2-static`; system development packages may remain shared.
- An executable-local backup file is intentionally writable portable state;
  using it under a protected installation directory can make saves fail.
