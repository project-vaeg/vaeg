# Changes for 2026-07-13

This release contains the changes made after `rel-260708`.

## Important: ROM layout changed

Previous releases could use the same unsuffixed ROM filenames for VA and
VA2/VA3. This release separates the model ROM sets. Users upgrading an
existing installation must review the ROM files beside `vaeg` or `vaeg.exe`.

- VA continues to use `vadic.rom`, `vafont.rom`, `varom00.rom`,
  `varom08.rom`, and `varom1.rom`.
- VA2/VA3 now requires the distinct MAME-compatible files
  `vadic_va2.rom`, `vafont_va2.rom`, `varom00_va2.rom`,
  `varom08_va2.rom`, and `varom1_va2.rom`.
- VA2/VA3 does not fall back to the unsuffixed VA filenames.
- The `_va2.rom` files are model-specific ROM contents with different
  checksums. Do not create them by merely renaming or copying the VA ROMs.
- `vasubsys.rom` remains a shared extra FDD-subsystem ROM.

Missing `_va2.rom` files are a likely cause when VA works but VA2/VA3 no
longer boots after upgrading. ROMs are not included and must be extracted
from hardware you own.

## PC-88VA keyboard input

- Added SDL2 keyboard mapping for the PC-88VA and PC-8801-style keyboard.
- Added separate JIS physical and US keytop presets.
- Added US keyboard text-friendly punctuation handling, including shifted
  symbols such as `@`, `:`, `_`, and `"`, while preserving pass-through comma,
  period, slash, hyphen, and semicolon input.
- JIS physical mode remains available for PC-88 keyboard-position fidelity;
  US keytop mode is intended for normal US keyboard BASIC, DOS, and filename
  entry.
- Added configurable bindings for special keys, including PC, KANA, GRPH,
  HENKAN, KETTEI, ZENKAKU, STOP, COPY, HELP, and keypad keys.
- Added Roman-Kana input, including yoon syllables and common extended
  combinations.
- Added a tenkeyless overlay for game use.
- Preserved F12 bindings and added PC-key support through Scroll Lock.

## VA platform and storage

- Made PC-88VA support unconditional in the active CMake/SDL2 tree while
  retaining runtime model checks.
- Reactivated SASI HDD support in the active build.
- Added GUI controls for SASI open/remove and persistent HDD mounting through
  the existing `HDD1FILE` and `HDD2FILE` configuration path.
- Added MAME-compatible VA and VA2/VA3 ROM lookup and CRC32/SHA-1 warnings.
- Added persistent floppy and SASI mount display and restored mounts across
  reset and restart where supported.
- Added creation of formatted blank D88 floppy images for 2HD and 2DD media.
  2D image creation is intentionally not exposed because its geometry is not
  currently reliable.
- Added creation of raw IMG floppy images suitable for mtools and normal
  mounting.

## Sound

- Added selectable NP2 and BSD-clean ymfm OPN/OPNA operator backends.
- Added model-aware FM hardware selection:
  OPN for VA and OPNA for VA2/VA3 or VA Sound Board II.
- Kept NP2 as a compatibility backend while making ymfm the default backend.
- Added model-specific SGP clock handling: 4 MHz for VA and 8 MHz for VA2/VA3.

## CPU, SGP, and pacing

- Replaced the current-tree M88/cisc-derived subsystem Z80 and disassembler
  with the pinned MIT-licensed suzukiplan core plus independently authored
  BSD-2-Clause vaeg wrapper, revision-1 codec, and disassembler. Historical
  Git commits were not rewritten.
- Separated CPU execution speed from machine/peripheral timing.
- Added independent CPU multiplier configuration and SGP speed modes:
  Model default, Follow CPU, and Custom.
- Added numeric CPU and SGP multiplier input with reset-safe configuration
  transactions.
- Added No Wait, frame skip, and hold-F11 fast-forward controls.
- Kept display scan, sound, FDD, RTC, and other peripheral timing in the
  machine-time domain.

## Display and frontend

- Added resizable SDL2 display support with shared viewport calculation,
  aspect correction, scaling modes, and screen effects.
- Simplified display mode selection to Windowed and Exclusive fullscreen.
- Added startup diagnostics behind `--debug` and restored the About runtime
  diagnostics panel.
- Restored the portable application icon and embedded frontend assets.
- Preserved the full VA raster after the left-side guard.
- Restored V30 LOOP timing used by VA firmware timing routines.

## Disk workflow and clipboard

- Added drag-and-drop mounting of disk images and supported archives, with
  persistent extracted archive mounts and FDD1/FDD2 assignment by filename.
- Added host-to-guest ASCII clipboard paste through the normal keyboard make /
  break path. Unsupported characters are skipped and Japanese IME paste is
  not included.
- Added Edit menu organization and mounted-media status display.

## Configuration and distribution changes

- Consolidated the active frontend configuration into `vaeg.cfg`.
- Executable-local configuration and backup memory are preferred for portable
  distributions, with user-state lookup as fallback.
- ROMs are resolved relative to the executable using the model-specific MAME
  filenames; ROM payloads are not distributed with the emulator.
- Windows-MinGW release builds are now static single-file executables. SDL2,
  LibArchive and its compression libraries, the GUI font, startup image, and
  application icon are linked or embedded into `vaeg.exe`; no companion SDL2
  or archive DLLs and no frontend asset files are required.
- macOS release builds statically include SDL2 and archive libraries and embed
  the frontend assets; normal macOS system frameworks remain external OS
  dependencies.
- Linux executables embed the frontend assets, but the standard Linux presets
  continue to use the distribution-provided SDL2 and LibArchive libraries.
- ROM and disk images remain external user-supplied files and are not embedded
  in the executable.

## Mouse input

- Added SDL2 relative mouse input through the existing PC-88VA guest mouse
  path.
- Added left/right guest buttons, host capture controls, and safe release on
  focus loss, reset, state load, GUI interaction, and shutdown.
- Added Device -> Mouse controls for capture, VA joystick/mouse port choice,
  and rapid buttons.
- Restored F12 and middle-click capture behavior without changing other F12
  guest bindings.

## Known limitations

- Japanese clipboard paste and guest-to-host copy are not implemented.
- GUI SASI mount/remove is available, but real HDD image compatibility still
  depends on the image and hardware behavior.
- 2D blank-image creation remains disabled pending reliable geometry support.
- The optional bgfx renderer remains outside this release; SDL2 is the active
  renderer backend.
