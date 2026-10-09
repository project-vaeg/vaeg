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
# M27: Frame display

## Goal

Restore the original VAEG `Frame Disp` function in the active SDL2 frontend.
The Screen menu exposes a persistent `Frame display` toggle. When enabled,
the native window title reports the measured draw rate as `N.NFPS`.

## Audited behavior

The frozen Win9x implementation is read-only evidence:

- `win9x/np2.cpp` toggles bit 1 of `np2oscfg.DISPCLK` for `IDM_DISPFRAME`.
- `win9x/ini.cpp` persists the low two bits as `DspClock`.
- `win9x/sysmng.cpp` samples the global `drawcount` over at least 2000 ms,
  computes tenths of a frame per second, and appends the result to the native
  window title.
- `drawcount` counts guest framebuffer draws. It is not the SDL present rate,
  configured frame-skip value, CPU speed, or guest VBlank frequency.

The active core retains the same `drawcount` increment points in `pccore.c`.
M27 therefore reuses that counter and does not alter core timing or drawing.

## Implementation

- `sdl2/framedisp.c` contains the C99, wrap-safe two-second measurement state,
  driven by SDL's monotonic millisecond counter.
- `sdl2/scrnmng.c` owns the title suffix and removes it immediately when the
  option is disabled.
- `sdl2/np2.c` samples after each executed guest frame, including frames whose
  host presentation is skipped.
- `DspClock` is restored in `vaeg.cfg`; bit 1 controls Frame display and the
  low two bits are retained for original configuration compatibility.
- Frame display defaults to enabled when `DspClock` is absent. Existing
  configurations that explicitly save bit 1 clear remain disabled.
- Screen -> Frame display toggles the option immediately and marks frontend
  configuration dirty.

The original CPU clock title display represented by bit 0 is not restored in
this milestone. Its configuration bit is preserved but has no active UI or
title effect.

## Non-goals

- Do not modify frame skip, No Wait, F11 fast-forward, guest VBlank timing, or
  CPU/SGP execution speed.
- Do not count Dear ImGui-only presents as guest frames.
- Do not edit the frozen reference tier.
- Do not add an on-screen overlay; the original feature used the window title.

## Automated verification

ROM-less tests cover the two-second renewal threshold, 60.0 and 30.0 FPS
calculations, zero draws, null input, and 32-bit tick/draw counter wrapping.

Run:

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --smoke
cmake --preset linux-release
cmake --build --preset linux-release
cmake --preset mingw-cross
cmake --build --preset mingw-cross
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
git diff -- win9x i286x cpuxva/memoryva.x86 hlp
```

## G27 human gate

- Enable Screen -> Frame display and confirm the title changes first to
  `0FPS`, then to a stable `N.NFPS` after about two seconds.
- Disable it and confirm the FPS suffix disappears immediately.
- Restart and confirm the setting persists through `DspClock` in `vaeg.cfg`.
- Check Auto, Full frame, 1/2, 1/3, and 1/4 frame skip. The displayed value
  must follow actual guest framebuffer draws without changing guest timing.
- Check No Wait and hold-F11 fast-forward; release F11 and confirm normal
  pacing returns.
- Check Windowed and Exclusive fullscreen behavior. The OS may hide the
  native title in fullscreen; the setting and measurement must remain safe.
- Complete the standard VA gate: V3 boot, VA demo, and OS boot/simple use.

G27 passed: the user accepted the implementation and requested integration
into `main`.

## Verification record

The following checks passed on the Linux development host:

- Linux debug configure/build, direct ROM-less selftest, and dummy-driver
  smoke;
- Linux release configure/build, direct ROM-less selftest, and dummy-driver
  smoke;
- Windows-MinGW cross configure/build using the pinned static dependency
  path;
- encoding, EOL, lowercase-path, and `git diff --check` invariants;
- an empty path-scoped diff for `win9x/`, `i286x/`,
  `cpuxva/memoryva.x86`, and `hlp/`.

The configured Linux debug tree does not enable CMake test registration, so
`ctest --test-dir build/linux-debug` reported `No tests were found`. The same
binary's explicit `--selftest` completed successfully, including the new
Frame display cases.

Linux release `vaeg` and Windows-MinGW `vaeg.exe` were copied to the shared
manual-test directory and verified byte-for-byte by SHA-256. Native macOS
build and visible window-title behavior remain outside automated verification.
