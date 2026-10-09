# M11 — Windows (MinGW) + macOS builds

Status: not-started
Branch: topic/m11-mingw-macos
Gate: G11 (human, per OS, full VA checklist)
Depends on: G9 AND G10 passed (all OSes ship the VA machine + GUI).

## Goal

The identical CMake tree builds and passes the VA gate on
Windows (MSYS2/MinGW-w64) and macOS (clang, Homebrew SDL2), in
addition to Linux.

## Windows/MinGW specifics

- Presets: `mingw-release` (native MSYS2) and, if feasible without
  contortions, `mingw-cross` from Linux for agent-side link checks.
- Paths and filenames: the portable frontend speaks UTF-8 internally.
  On Windows, every filesystem touch in sdl2/dosio must convert
  UTF-8 → UTF-16 and use the wide CRT/Win32 calls (_wfopen etc.).
  Centralize the conversion in ONE place (sdl2/ platform shim); no
  scattered MultiByteToWideChar calls. The ANSI/CP932 API family is
  forbidden in sdl2/.
- Legacy config compatibility: reading an old CP932 np2.ini is a
  non-goal; document that phase-2 config is UTF-8-only, fresh location.
- Console/subsystem: -mwindows for release, keep a console build option
  for debugging.

## macOS specifics

- clang + Homebrew SDL2 via find_package; no Xcode project.
- App bundle is OPTIONAL (plain binary acceptable at this gate); if
  trivial via CMake MACOSX_BUNDLE, do it, else note as later.
- Retina/HiDPI: verify scrnmng texture scaling with
  SDL_WINDOW_ALLOW_HIGHDPI; document behavior.
- Keyboard: verify JIS/US layout handling through SDL_Scancode; note
  gaps rather than hacking around them.

## Constraints

- No #ifdef spraying through shared sdl2/ code: platform divergence is
  confined to the platform shim and compiler.h.
- Endianness: both targets are little-endian; do not remove the
  big-endian-safe macro usage introduced in M9.
- LEGACY v141 stays untouched and green.

## Machine checks

    Linux build still green (gcc+clang) + --smoke
    mingw cross or native build log
    tools/repo checkers

## Gate G11 (human)

Full VA checklist (V3 boot, demo, OS ops) executed by the user on
real Windows and real macOS. GUI walk of the must-have set on both.

## Risks to report

- SDL2 audio backend differences (WASAPI vs CoreAudio buffer sizing).
- Filesystem case-sensitivity assumptions surfacing on macOS
  (case-insensitive default) vs Linux.
- MSYS2 packaging drift for SDL2.
