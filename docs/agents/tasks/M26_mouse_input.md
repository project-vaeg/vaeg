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
# M26 - SDL2 host mouse input

Status: complete; G26 passed

Branch: `topic/m26-mouse-input`

Depends on: M25 and G25.

## Goal

Implement host mouse input in the active SDL2/Dear ImGui frontend by porting
the behavior of the frozen Win9x frontend to SDL2. Feed relative movement and
active-low left/right button state through the existing `mousemng_getstat()`
seam. Do not replace or bypass the existing PC-88VA mouse I/O implementation.

The resulting frontend must support:

- runtime mouse capture using SDL relative mouse mode;
- left and right guest mouse buttons;
- the PC-88VA controller-port choice between joystick and mouse;
- persistent capture, controller-port, and rapid-button settings;
- the original F12 mouse-binding behavior and middle-button capture toggle;
- safe release on focus loss, GUI interaction, reset, state load, and exit.

## Non-goals

- Do not edit `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, or `hlp/`.
- Do not rewrite `io/mouseif.c` or `iova/mouseifva.c` unless a separately
  proven core defect blocks the frontend port.
- Do not inject guest cursor coordinates, BIOS state, RAM, VRAM, or INT 33h
  results directly.
- Do not add absolute/tablet/touch input, wheel emulation, a third guest
  button, host pointer acceleration controls, or network mouse support.
- Do not change joystick mapping beyond exposing the already implemented
  joystick/mouse controller-port selection.
- Do not assign a new guest key code to F12.

## Required reading

Read these files before editing:

- `docs/agents/ROADMAP.md`
- `docs/agents/CONVENTIONS.md`
- `docs/modernization/GUI-PARITY.md`
- `sdl2/mousemng.c` and `sdl2/mousemng.h`
- `sdl2/taskmng.c`
- `sdl2/gui/gui.cpp` and `sdl2/gui/gui.h`
- `sdl2/np2.c`, `sdl2/np2.h`, and `sdl2/ini.c`
- `sdl2/kbdmap.c` for current F12 behavior
- `io/mouseif.c` and `io/mouseif.h`
- `iova/mouseifva.c`, `iova/mouseifva.h`, `iova/sysportva.c`, and
  `iova/boardsb2.c`
- frozen read-only references `win9x/mousemng.cpp`, `win9x/mousemng.h`,
  `win9x/np2.cpp`, `win9x/ini.cpp`, and `win9x/menu.cpp`

The locally available technical-manual text `docs/tekumani/613MOUSE.TXT`
describes the VA INT 33h mouse BIOS and is useful for manual-test selection.
It is evidence about guest software behavior, not a replacement for the
existing hardware-level I/O path.

## Audit result

| Area | Current implementation | M26 consequence |
|---|---|---|
| SDL2 mouse manager | `sdl2/mousemng.c` always returns zero movement and released buttons (`0xa0`) | Replace the stub with a stateful relative-motion and button frontend. |
| Guest sampling | `pccore_exec()` calls `mouseif_sync()`, which calls `mousemng_getstat(..., clear=1)` | Keep this existing pull boundary; SDL events only update frontend state. |
| Generic mouse I/O | `io/mouseif.c` interpolates one host sample over the guest display period and implements the existing 8255 ports | Do not duplicate timing, latching, rapid state, or port behavior in SDL2. |
| VA controller port | `iova/mouseifva.c` selects `MOUSEIFVA_JOYPAD` or `MOUSEIFVA_MOUSE`, latches signed deltas, and returns nibbles/buttons | Persist and expose `mouseifvacfg.device`; default remains joystick. |
| VA wiring | `iova/sysportva.c` supplies the strobe; `iova/boardsb2.c` reads controller data through OPNA registers | Mouse input must work through this existing route for built-in and Sound Board II paths. |
| Win9x movement | `win9x/mousemng.cpp` captures around the window center, accumulates relative deltas, and under `VAEG_FIX` divides by two while preserving odd remainders | Use SDL relative events but preserve the divide-by-two sensitivity and remainder behavior. |
| Win9x buttons | Left/right are active-low bits `0x80`/`0x20`; disabled capture ignores guest button events | Keep exactly these values and semantics. |
| Win9x capture state | Capture is active only when system, UI, and background blocker bits are all clear | Model persistent user request separately from transient GUI/focus blockers. |
| Win9x shortcuts | F12 toggles capture when its binding is Mouse; middle-button down also toggles capture | Restore both without sending F12 or middle-button data to the guest. |
| Active F12 map | `F12_bind=0` currently resolves to no guest key; values 1-4 map to COPY, STOP, keypad `=`, and keypad `,` | Intercept F12 only when `F12KEY == 0`; preserve all other bindings. |
| Active event routing | `sdl2/taskmng.c` routes keyboard/text and asks ImGui whether each event is captured; mouse events are not sent to `mousemng` | Add mouse routing after ImGui processing while retaining mandatory release cleanup. |
| Active configuration | `MS_RAPID` is already saved; `Mouse_sw` and `Mouse_VA` are absent | Add the two original keys to `vaeg.cfg`, validate `Mouse_VA`, and keep old-config defaults capture-off/joystick. |
| Active GUI | Device menu shows `Mouse (not implemented)` | Replace it with explicit capture, controller-port, and rapid controls. |

## Implementation requirements

### 1. Frontend mouse manager

Expand `sdl2/mousemng.c` and `sdl2/mousemng.h` as C99 frontend code. Keep
`mousemng_getstat()` as the core-facing API and add only the small event and
lifecycle API needed by `taskmng`, GUI, and startup/shutdown code.

The manager must maintain separately:

- accumulated unscaled relative X/Y movement;
- the divide-by-two X/Y remainder required by the original `VAEG_FIX` path;
- active-low left/right guest button bits;
- persistent user capture request;
- current focus state;
- current GUI/modal block state;
- actual SDL relative-mode state;
- a short status/error string if SDL capture activation fails.

Initialization must report both buttons released (`0xa0`) and zero movement.
Use a wide accumulator and saturating addition so a stalled guest cannot wrap
movement before `mousemng_getstat()` consumes it. Return a representable
`SINT16` delta and retain any unconsumed amount. When `clear` is false, do not
consume movement or remainders.

Do not call guest I/O functions from the SDL event path. Motion/button events
only update frontend state; the core continues to pull it through
`mousemng_getstat()`.

### 2. SDL capture

Use `SDL_SetRelativeMouseMode(SDL_TRUE/FALSE)` instead of Win32 cursor warping.
Relative mode supplies `SDL_MOUSEMOTION.xrel/yrel`, hides the host cursor, and
avoids window-edge clipping. Do not derive guest motion from absolute window
coordinates and do not apply the display viewport transform.

Actual capture is active only when all of these are true:

- the persistent capture request is enabled;
- the emulator window has input focus;
- Dear ImGui is not using the mouse and no frontend modal/browser/binding
  capture is active;
- SDL relative mode was enabled successfully.

Focus or GUI blocking is temporary and must not overwrite `Mouse_sw`.
Capture should resume automatically when the blocker clears and the saved
request remains enabled. If SDL rejects relative mode, remain uncaptured,
release guest buttons, keep the application usable, and expose the SDL error
in the Device menu/status. Avoid retrying every frame; retry on an explicit
user action or a meaningful focus/GUI transition.

Disabling actual capture must:

- release both guest buttons;
- discard pending relative movement and scaling remainders;
- disable SDL relative mode if it is active;
- never leave the host cursor hidden or confined.

### 3. Event routing

Extend `sdl2/taskmng.c` to process:

- `SDL_MOUSEMOTION` using `xrel/yrel`;
- `SDL_MOUSEBUTTONDOWN` and `SDL_MOUSEBUTTONUP` for left/right;
- middle-button down as the original capture toggle;
- `SDL_WINDOWEVENT_FOCUS_GAINED` and `SDL_WINDOWEVENT_FOCUS_LOST`;
- `SDL_QUIT` and frontend shutdown cleanup.

Each event must still be passed to `gui_process_event()`. If ImGui captures a
mouse event, do not apply its motion or button-down to the guest. Button-up,
focus-loss, quit, and capture-disable cleanup must nevertheless release any
guest button that might otherwise remain held.

Add a GUI helper analogous to `gui_guest_keyboard_blocked()` if needed so the
mouse manager can distinguish normal guest input from open menus, modals,
file browsers, binding capture, and `io.WantCaptureMouse`.

The middle button is a host capture command only. It is never a guest button.
Ignore it when ImGui is actively using that click. When it toggles capture,
consume both down and matching up as needed so no duplicate frontend action
occurs.

### 4. F12 compatibility

When `np2oscfg.F12KEY == 0` (`F12 binding -> Mouse`):

- non-repeat F12 keydown toggles the persistent capture request;
- F12 keyup is consumed;
- neither event is sent to the guest keyboard mapper.

When `F12KEY` is 1-4, preserve the current M14 guest binding behavior exactly.
Do not change F11 fast-forward or right-Alt KANA behavior. Do not toggle mouse
capture from F12 while a modal/binding workflow would make immediate relative
capture unsafe; keep the request off or defer activation until the GUI block
ends, and document the chosen behavior.

### 5. Configuration

Add and persist these existing-format keys in `vaeg.cfg`:

```ini
Mouse_sw=0
Mouse_VA=0
MS_RAPID=0
```

Meanings:

- `Mouse_sw`: persistent host mouse capture request, boolean;
- `Mouse_VA`: `0` joystick, `1` mouse on the VA controller port;
- `MS_RAPID`: existing guest mouse rapid-button behavior.

Add `MOUSE_SW` to active `NP2OSCFG`. Reuse the existing global
`mouseifvacfg.device`; do not create a duplicate frontend copy. Validate
unknown `Mouse_VA` values to joystick with a warning. Existing configs lacking
the new keys must remain capture-off and joystick-selected.

Changing capture or rapid is immediate and does not reset the guest. The
original Win9x menu also changes the VA controller-port device immediately;
follow that behavior unless active-core testing proves reset is required.
Mark `SYS_UPDATEOSCFG` for `Mouse_sw` and `SYS_UPDATECFG` for `Mouse_VA` and
`MS_RAPID`, matching the ownership of their saved structures.

### 6. GUI

Replace `Device -> Mouse (not implemented)` with a `Mouse` submenu containing:

- `Capture mouse` with a check mark for the persistent request;
- `VA controller port` submenu with mutually exclusive `Joystick` and `Mouse`;
- `Rapid buttons` for `np2cfg.MOUSERAPID`;
- a disabled status line when capture activation failed.

Labels must distinguish host pointer capture from the guest controller-port
device. Do not imply that selecting the guest Mouse port automatically traps
the host pointer. Retain the existing `Keyboard -> F12 binding -> Mouse` item.

### 7. Lifecycle and reset safety

Initialize the mouse manager only after SDL video/window creation is usable
and before normal event processing. Shut it down before destroying the SDL
window. Integrate release/reset calls into:

- guest reset;
- state load;
- focus loss;
- quit and task-manager exit;
- GUI/file modal transitions;
- display/window teardown or reconstruction;
- startup failure cleanup after initialization.

Guest reset and state load clear deltas, remainders, and pressed buttons. They
must not erase the saved capture request or controller-port selection. A
temporary blocker may suspend actual capture and restore it afterward.

### 8. ROM-less tests

Add deterministic tests for the frontend state logic without requiring a ROM
or a real mouse. SDL relative-mode calls should be isolated from pure state
tests or made injectable so the dummy video driver does not invalidate them.

Cover at least:

- initial zero delta and released `0xa0` buttons;
- positive and negative motion accumulation;
- original divide-by-two scaling with odd remainder preservation;
- `clear=0` versus `clear=1`;
- saturation/no-wrap behavior;
- active-low left/right down and up;
- ignored motion and button-down while uncaptured or GUI-captured;
- forced button release and delta discard on focus loss/capture disable;
- saved request retained across temporary GUI/focus blocking;
- F12 Mouse interception, repeat suppression, and keyup consumption;
- unchanged F12 COPY/STOP/keypad bindings;
- middle-button capture toggle without a guest button;
- `Mouse_VA` validation and configuration round trip;
- `MS_RAPID` preservation.

Do not claim that ROM-less tests verify INT 33h, guest cursor direction, or
real SDL capture behavior.

## Documentation updates during implementation

Update:

- this task file with implementation and verification results;
- `docs/agents/ROADMAP.md`;
- `docs/modernization/GUI-PARITY.md`;
- `sdl2/README.md` with capture/release controls and `vaeg.cfg` keys;
- `sdl2/gui/README.md` with the mouse-event capture rule if present.

Record any departure from original sensitivity, F12 behavior, or immediate
controller-port switching as an explicit unresolved issue. Add an ADR only if
implementation requires a non-obvious new cross-module policy.

## Required verification

Run, or report the exact reason a command is unavailable:

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

The final frozen-tier diff must be empty.

## G26 human gate

Test with real host mice on the available Linux, Windows-MinGW, and macOS
builds. Unavailable platforms remain explicitly unverified.

- Start with capture off and confirm the host cursor and ImGui menus work.
- Select `VA controller port -> Mouse`, enable capture, and run VA mouse BIOS
  software or an OS/application that visibly uses the VA mouse.
- Confirm horizontal and vertical direction, sensitivity, slow one-dot motion,
  and larger rapid motion.
- Confirm left and right press, hold, drag, and release without swapped or
  stuck buttons.
- Select `Joystick` and confirm mouse input no longer replaces joystick input.
- Toggle capture through Device menu, middle click, and F12 when F12 binding is
  Mouse.
- Select each non-Mouse F12 binding and confirm its existing guest action is
  unchanged.
- Open menus/dialogs and confirm mouse motion/buttons do not leak to the guest.
- Alt-Tab or otherwise lose focus while moving and holding each button;
  confirm capture releases and no button remains stuck.
- Return focus and confirm configured capture resumes predictably.
- Reset and load state while captured and while holding a button.
- Check windowed, resized, High-DPI, and fullscreen modes; relative motion must
  not depend on guest viewport coordinates.
- Check VA and VA2/VA3, OPN/OPNA including VA Sound Board II where applicable,
  Mouse Rapid off/on, V3 boot, VA demo, and OS boot/simple operations.

G26 passed: the user explicitly confirmed the human checks passed.

## Implementation record

The active SDL2 frontend now implements the audited design:

- `sdl2/mousestate.c` owns deterministic relative movement, the original
  `VAEG_FIX` divide-by-two remainder, saturation, and active-low buttons.
- `sdl2/mousemng.c` owns SDL relative mode and separates the saved capture
  request from focus and ImGui blockers.
- `sdl2/taskmng.c` routes uncaptured SDL motion and left/right buttons, treats
  middle click as a host capture toggle, and intercepts F12 only when
  `F12_bind=0` selects Mouse.
- `Mouse_sw`, `Mouse_VA`, and the existing `MS_RAPID` are persisted in
  `vaeg.cfg`. Invalid `Mouse_VA` values fall back to joystick.
- Device -> Mouse exposes capture, VA controller-port Joystick/Mouse, rapid
  buttons, and SDL capture errors.
- Reset, state load, focus loss, GUI blocking, quit, and shutdown release
  frontend movement and button state without erasing saved settings.

No guest I/O implementation was changed. `pccore_exec()` continues to pull
the frontend sample through `mouseif_sync()` and `mousemng_getstat()`, while
`iova/mouseifva.c` retains VA nibble latching and joystick/mouse dispatch.

ROM-less tests cover state initialization, inactive-input rejection,
movement scaling and remainders, clamping and accumulator saturation,
active-low buttons, reset/disable release, F12 Mouse non-mapping, and all
four existing non-Mouse F12 guest mappings. Real SDL capture, guest cursor
direction/sensitivity, INT 33h software, and controller-port behavior remain
G26 human checks.

## Verification record

The following checks passed on the Linux development host:

- Linux debug configure/build, ROM-less selftest, and dummy-driver smoke;
- Linux release configure/build, ROM-less selftest, and dummy-driver smoke;
- Linux GCC CI configure/build and its CTest selftest;
- Windows-MinGW cross configure/build with the repository-pinned static SDL2;
- encoding, EOL, lowercase-path, and `git diff --check` invariants;
- an empty path-scoped diff for `win9x/`, `i286x/`,
  `cpuxva/memoryva.x86`, and `hlp/`.

The Linux release binary and Windows-MinGW `vaeg.exe` were copied to the
shared manual-test directory. The user completed the real host/guest mouse
checks; macOS remains outside the available build environment.
