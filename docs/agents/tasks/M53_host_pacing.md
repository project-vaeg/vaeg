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
# M53: Configurable non-blocking host pacing

Status: **G53 passed**

Starting SHA: `a80a6f9303402945942f12ec72912c49046994e2`

## Goal

Make short guest boot messages, including RAM-disk and MSE registration,
readable without changing the emulated CPU clock or making the SDL2/ImGui UI
unresponsive.

## Required behavior

- Add `PacingMs` to the portable configuration with a default of zero.
- Expose a 0-1000ms value in Configure.
- Apply the delay between guest frames, not inside CPU execution or timing
  accounting.
- Continue polling input and rendering ImGui while a guest frame is deferred.
- Keep zero equivalent to the previous unpaced behavior.
- Preserve No Wait, frame skip, and hold-F11 fast-forward behavior apart from
  the explicitly configured host delay.

## Non-goals

- Do not add fractional CPU multipliers.
- Do not change V30, SGP, DMA, timer, sound, or serialized-state semantics.
- Do not make PacingMs part of guest state.
- Do not slow UI input or rendering.

## Automated validation

- Linux release and MinGW builds.
- Existing pacing and SDL2 selftests.
- Configuration persistence for the new `PacingMs` key.
- Zero-delay regression and a nonzero scheduling check.
- Repository diff, encoding, EOL, and case checks.

## Human gate

1. Set PacingMs to 0 and confirm normal V3 boot speed and responsive UI.
2. Set PacingMs to 32 or 64 and confirm guest boot messages remain visible.
3. While the guest is slowed, open Configure and other menus and confirm that
   mouse, keyboard, and UI rendering remain responsive.
4. Restart VAEG and confirm the selected value persists.
5. Return PacingMs to 0 and confirm ordinary OS operation.

G53 passed after maintainer confirmation of zero-delay compatibility,
readable 32/64ms-paced boot messages, responsive UI/input while paced,
configuration persistence, and ordinary device operation.

## Implementation record

- `90ea0d6d0f9ece86f1ebe1f8202e5b46d295757a` adds the bounded persisted
  `PacingMs` setting, Configure control, and deadline-based guest scheduling.
  While a guest frame is deferred, the frontend continues polling events and
  rendering ImGui rather than blocking in `SDL_Delay()`.

Maintainer testing confirmed that RAM-disk and MSE registration messages are
readable at nonzero pacing values and that the corrected scheduling keeps the
UI responsive at 64ms. The maintainer subsequently accepted the complete G53
checklist.

## Local validation result

The following completed successfully:

```text
cmake --build build/linux-release -j2
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-release/sdl2/vaeg --selftest
cmake --build build/mingw-cross -j2
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-release/sdl2/vaeg --smoke
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

The selftest reported `VA BMS config/window lifecycle ok`, `ini ok`,
`pacing ok`, and `all tests passed`. The release preset does not define CTest
tests, so its direct selftest and smoke executables are the applicable checks.
