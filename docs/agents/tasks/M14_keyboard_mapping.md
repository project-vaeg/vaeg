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
# M14 - SDL2 Keyboard Mapping

Status: complete

Branch: `topic/m14-keyboard-mapping`

Gate: G14 passed

Depends on: G13 passed / `phase2-complete` tagged

Decision record: `docs/agents/DECISIONS/ADR-0008-keyboard-mapping.md`

## Goal

Replace the SDL2 frontend's partial anonymous 101-key table with a
documented PC-88VA / PC-8801-style keyboard layer suitable for both JIS
and US host keyboards. Preserve the guest keyboard protocol: all physical
and synthetic input must use guest make/break events through
`keystat_senddata()`; no text, BIOS-buffer, RAM, or VRAM injection is
permitted.

QUASI88 is a semantic reference for PC-8801-family key names and SDL
scancode ideas only. Its numeric `KEY88_*` values are not VA guest key
codes. VA codes are proven from this tree, chiefly `keystat.h`,
`win9x/winkbd.cpp`, `io/serial.c`, and `bios/keytable.res`.

## Delivered Architecture

The implemented input path is:

```text
SDL keyboard event
  -> sdl2/sdlkbd.c input/capture boundary
  -> sdl2/kbdmap.c host layout and action selection
  -> sdl2/kbdinject.c guest make/break sender
  -> keystat_senddata()
```

The mapping layer supports four explicit action classes:

- one guest key;
- a guest modifier plus guest-key chord;
- a pass-through physical key;
- an unresolved action that remains visible instead of being guessed.

`sdl2/romankana.c` parses Roman input into internal kana tokens. The
tokens are translated to verified physical VA kana key sequences and use
the same injection path. SDL text input and Unicode are never sent to the
guest.

ImGui capture is authoritative. Raw keyboard events, US keytop actions,
Roman-Kana output, and capture-next-key events do not leak into the guest
while ImGui owns keyboard/text input.

## Host Layouts

### JIS physical

The JIS preset maps SDL scancode positions to the physical PC-88VA key
roles. It is fidelity-first and is appropriate for games and software
whose controls assume the original keyboard geometry. The implementation
retains backend aliases needed by real JIS host stacks, including the Yen
and Kana scancode variants established during G14 testing.

### US keytop

The US preset is text-entry friendly. It maps a US keytop or shifted host
chord to the proven guest key or guest Shift chord that produces the
intended ASCII symbol. It is deliberately not a JIS-position map with a
few fallback keys.

Required translations include Shift+`2` to `@`, Shift+`;` to `:`, `=` to
guest Shift+minus, Shift+minus to underscore, apostrophe/double quote,
backslash, and both brackets. Comma, period, slash, minus, and semicolon
remain pass-through regression cases. Host Shift is temporarily reconciled
around translated chords so guest Shift cannot remain stuck.

### Custom

The GUI exposes the complete physical-key inventory and a capture-next-key
workflow. Bindings are stored by SDL scancode name, never by numeric host
value. New maps are written to `keyboard.map` beside `np2.cfg`; the INI
value points to it as `file:keyboard.map`. The loader retains compatibility
with the early inline M14 format.

The complete one-row-per-key inventory, VA guest codes, presets, status,
and evidence are maintained in
`docs/modernization/keyboard-mapping.md`.

## Special-Key Decisions

- The assigned Kana key sends the real guest Kana make/break sequence.
  One press locks Kana and the next press unlocks it. The frontend mirror
  exists only to decide whether the Roman-Kana helper should intercept
  alphabetic scancodes.
- The menu selects `JIS Kana` or `Roman Kana`; a separate `Off` mode was
  removed as redundant. Kana mode itself is controlled by the guest Kana
  key.
- The VA `PC` key defaults to host `ScrollLock`. PC-held reset/power-on is
  required for the VA2/3 setup path, and VA software uses PC chords such
  as PC+D.
- Existing F12 behavior remains available and was not reassigned by the
  preset.
- `VAEG_KBD_TRACE=1` logs scancode, keycode, modifiers, layout, capture
  state, selected action, consumption, and guest send behavior.

## Tenkeyless Overlay

The optional game overlay maps `YUI/HJK/NM,.` to guest keypad
`789/456/123/0`. It is independent of JIS/US/Custom layout, defaults off,
is persisted as `keyboard_tenkey_overlay`, and still sends ordinary guest
keypad make/break events.

## Roman-Kana Scope

The parser accepts the required vowels and consonant rows, voiced and
semi-voiced rows, `nn`/`n'`, delayed `n`, uppercase input, and doubled
consonants. G14 follow-ups added:

- `shi`/`si`, `tsu`/`tu`;
- yoon forms such as `kya`, `sha`, `cha`, `nya`, `gya`, `ja`, `bya`, and
  `pya`, with documented aliases;
- small-kana aliases including `xya`, `xa`, and `xtsu` families;
- `va`, `vi`, `vu`, `ve`, and `vo`.

Unsupported input is flushed visibly without corrupting the following
input. The authoritative accepted-sequence list and physical kana evidence
are in `docs/modernization/keyboard-mapping.md`.

## Configuration

The SDL2 INI layer persists:

```text
keyboard_host_layout=jis|us|custom
keyboard_kana_input=jis-kana|roman
keyboard_auto_kana_lock=0|1
keyboard_tenkey_overlay=0|1
keyboard_custom_map=file:keyboard.map
```

`keyboard_auto_kana_lock` remains readable for old configurations, but the
implemented design uses the assigned Kana key as the sole Kana-lock
control.

## Files and Documentation

Core frontend modules added by M14:

- `sdl2/kbdmap.c`, `sdl2/kbdmap.h`;
- `sdl2/kbdinject.c`, `sdl2/kbdinject.h`;
- `sdl2/romankana.c`, `sdl2/romankana.h`.

Integration and UI changes landed in `sdl2/sdlkbd.c`, `sdl2/taskmng.c`,
`sdl2/gui/gui.cpp`, `sdl2/ini.c`, `sdl2/np2.[ch]`, and
`sdl2/selftest.c`. Policy and user documentation are in ADR-0008,
`docs/modernization/keyboard-mapping.md`, `sdl2/README.md`, and
`sdl2/gui/README.md`.

The frozen reference tier was inspected for evidence and was not modified.

## Machine Gate

The M14 completion gate requires:

```sh
cmake --preset linux-debug
cmake --build build/linux-debug --target vaeg_sdl2
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --smoke
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
```

ROM-less selftests cover mapping lookup, scancode-name persistence,
unresolved-name handling, US keytop actions, pass-through punctuation,
tenkey overlay entries, and Roman-Kana parsing. ImGui capture isolation
is covered by the human gate below.

## Human Gate Result

G14 was accepted after interactive testing of:

- ordinary guest keyboard input and ImGui capture isolation;
- US keytop punctuation and JIS physical punctuation/Yen behavior;
- Kana lock and JIS/Roman input selection;
- Roman-Kana base, yoon, small-kana, and `v` combinations;
- PC key assignment and tenkeyless keypad overlay;
- custom binding capture without guest leakage.

The inventory continues to show mappings that have proven guest codes but
were not individually exercised on every host keyboard. A missing host
default remains visible in the GUI; it is not silently approximated.

## Commit Record

```text
5097033 M14: document SDL2 keyboard mapping policy
a6216b7 M14: add SDL2 keyboard mapping layer
b140cac M14: add ImGui keyboard configuration UI
aa1321c M14: store custom keyboard map in sidecar file
7532bdf M14: make Roman-Kana input scancode based
ebe9970 M14: make Roman-Kana mode own KANA lock
4f07aa4 M14: make KANA key control kana mode
b8f0b64 M14: add Roman-Kana yoon syllables
4086d08 M14: add US keytop punctuation actions
b936e37 M14: bind PC key to Scroll Lock
136c4b5 M14: add tenkeyless keypad overlay
57be6f1 M14: fix JIS kana and punctuation mappings
8bb09b4 M14: remap JIS yen scancode
```
