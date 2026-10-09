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
# ADR-0008: SDL2 Keyboard Mapping Policy

Date: 2026-07-08

Status: Accepted

## Context

G11 exposed that the old SDL2 keyboard table was a partial 101-key map:
common alphanumeric keys worked, but PC-88VA/PC-8801-specific keys such
as Kana, Henkan, NFER/Kettei, RO/underscore, Zenkaku, and keypad Return
were incomplete or unavailable on US host keyboards.

M14 needs a complete, documented keyboard inventory without changing the
guest keyboard protocol or writing directly into guest text buffers.

## Decision

- Use SDL scancodes for guest input mapping. Do not use SDL key symbols
  as guest keys.
- Store custom host bindings by SDL scancode name, not numeric scancode
  values. GUI-edited bindings are persisted in `keyboard.map` in the
  portable user-state directory, with
  `keyboard_custom_map=file:keyboard.map` as the INI pointer, so long
  binding tables do not exceed the legacy INI reader's practical line
  length. Executable-local configuration does not relocate the sidecar.
- Use QUASI88 only as semantic naming reference for PC-8801-style key
  roles. Do not copy QUASI88 numeric `KEY88_*` values.
- Prove VA guest key codes from this repository, primarily
  `machine/keystat.h`, `win9x/winkbd.cpp`, and `io/serial.c`.
- Route physical and synthetic input through `sdl2/kbdinject.c`, which
  wraps the existing `keystat_senddata()` and `keystat_forcerelease()`
  make/break path.
- Provide JIS, US, and Custom host layouts. JIS is the fidelity preset;
  US is a keytop/text-entry preset for common modern keyboards. US
  printable punctuation may translate host Shift chords into proven guest
  keys or guest Shift chords; JIS physical mode remains position-based.
- Bind the VA PC key to `ScrollLock` by default. The guest code is
  proven, and the key is required for VA2/3 setup entry on PC-held reset
  or power-on and for popup-helper chords such as PC+D.
- Provide an explicit tenkeyless game overlay, off by default, that maps
  `YUI/HJK/NM,.` to guest keypad `789/456/123/0`. It is independent of
  JIS/US/Custom host layout and still emits normal guest keypad
  make/break events.
- Roman-Kana parses A-Z and apostrophe host scancodes into internal kana
  tokens and then emits guest key sequences. It ignores SDL_TEXTINPUT for
  guest input so host IME state and UTF-8 composition cannot leak into the
  guest. It never injects Unicode, CP932, BIOS/DOS buffers, RAM, or VRAM.
  The menu selects JIS-Kana or Roman-Kana as the kana input method only.
  Guest kana mode is entered and left by the assigned KANA key: one press
  locks KANA, the next press unlocks it. Roman-Kana consumes A-Z
  scancodes only while that KANA lock mirror is active.
- Yoon Roman-Kana syllables are emitted as verified physical kana
  sequences: base kana plus small `ya/yu/yo` from `bios/keytable.res`,
  never as direct character text.
- `VAEG_KBD_TRACE=1` enables keyboard routing traces for host scancode,
  keycode, modifiers, layout, ImGui capture state, selected action, and
  consumed/sent status.

## Consequences

- ImGui input capture remains the boundary: captured key or text events
  never reach the guest, including Roman-Kana helper output.
- Missing host bindings are visible in `docs/modernization/keyboard-mapping.md`
  and in the ImGui binding table.
- The loader accepts the initial inline M14 custom-map format for
  migration, but new saves use the sidecar form.
- Manual G14 testing must confirm physical JIS behavior, US keytop
  usability, Kana lock behavior, Roman-Kana output, and rebinding.
