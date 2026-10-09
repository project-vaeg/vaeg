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
# Keyboard Mapping

M14 replaces the SDL2 frontend's anonymous 101-key table with a named
PC-88VA / PC-8801-style keyboard inventory. Guest input remains scancode
based: host SDL scancodes are mapped to proven VA guest key codes, and
all physical and synthetic input goes through the normal `keystat`
make/break path via `sdl2/kbdinject.c`.

## Evidence

The `win9x/` references below resolve in the immutable
[`archive/frozen-win9x-i286x-g56`](https://github.com/nakatamaho/vaeg/tree/archive/frozen-win9x-i286x-g56)
snapshot; that tier is absent from the current tree.

- Pre-M14 SDL2 mapping: `sdl2/sdlkbd.c`.
- VA guest key names and base codes: `machine/keystat.h`.
- Win32/v141 host mapping reference: `win9x/winkbd.cpp`.
- VA model-dependent keyboard matrix positions: `io/serial.c`.
- Roman-Kana physical kana table: `bios/keytable.res`.

The archived Win32 tree is evidence only. It was not edited for M14.

## Runtime Model

- `keyboard_host_layout`: `jis`, `us`, or `custom`.
- `keyboard_kana_input`: `jis-kana` or `roman`.
- `keyboard_auto_kana_lock`: accepted from old configs, but Roman-Kana
  ignores it. The assigned KANA key controls guest kana lock directly.
- `keyboard_tenkey_overlay`: `0` or `1`; when enabled, a tenkeyless
  game overlay maps `YUI/HJK/NM,.` to guest keypad digits.
- `keyboard_custom_map`: `file:keyboard.map` for GUI-edited bindings.
  The sidecar file remains in the portable user-state directory even when
  `vaeg.cfg` is executable-local, and stores one `role=scancode-name`
  entry per line. SDL scancode names are stored, never numeric
  host-dependent values.

Early M14 builds wrote the whole custom map as a single INI value. The
loader keeps compatibility with that inline form, including scancode
names such as `;` and `=`, but new saves use the sidecar to avoid long
INI lines and delimiter ambiguity.

The same sidecar also stores frontend host actions. `fast_forward` defaults
to F11 and controls the temporary hold-to-fast-forward pacing action;
`mouse_capture` defaults to F12 and controls the host capture shortcut when
`F12_bind=0` (Mouse). These actions are shown as host rows in the Keyboard
mapping window and can be rebound without changing guest key codes.

The F12 binding menu also provides `F12_bind=6` (`Full speed (No Wait)`). While
F12 is held in this mode, it selects the same temporary No Wait and draw-skip
16 pacing path as the configurable `fast_forward` host action; the F12 event is
not sent to the guest.

ImGui input capture remains authoritative. If ImGui wants keyboard or
text input, neither raw scancodes nor Roman-Kana helper output reach the
guest. During key-binding capture, the captured keydown and matching
keyup are consumed by the GUI.

## Host Layout Modes

`jis` is the JIS physical preset. It maps host SDL scancode positions to
PC-88VA physical guest keys. This is the fidelity-first mode and is the
better default for games or software that expects the original keyboard
geometry. The preset accepts a small number of SDL backend aliases for
keys whose reported scancode varies by host stack: `International3` is
accepted as an additional Yen key, and `International2` is accepted as
an additional KANA key.

`us` is the US keytop preset. It is text-entry friendly for BASIC, DOS,
monitors, filenames, and shells on commodity US keyboards. Printable
punctuation is resolved as host scancode plus host Shift state, then
translated to a guest key or guest Shift chord that produces the intended
ASCII symbol. The original host key event is consumed when a translation
is used, and the generated make/break bytes still go through
`keystat_senddata()` via `sdl2/kbdinject.c`. No Unicode, BIOS/DOS buffer,
RAM, VRAM, or SDL_TEXTINPUT character injection is used.

If host Shift is physically held for a translated US symbol, the frontend
temporarily releases mirrored guest Shift, sends the selected guest
key/chord, and restores mirrored guest Shift. This avoids cases such as
US Shift+`2` becoming guest Shift+`2` instead of guest `@`.

Set `VAEG_KBD_TRACE=1` to log SDL scancode/keycode, modifier state,
layout, capture state, selected action type, target, consumption, and the
make/break style used for each relevant keyboard event.

### US Keytop Translations

Evidence for guest key and chord results is the active VA key inventory
below plus `bios/keytable.res`, whose normal and Shift tables prove the
guest-side printable characters.

Problem mappings fixed in US keytop mode:

| US host input | Guest action | Guest result |
|---|---|---|
| Shift+`2` | guest `@` key `0x1a` | `@` |
| Shift+`;` | guest `:` key `0x27` | `:` |
| `=` | guest Shift + `-` key `0x0b` | `=` |
| Shift+`-` | guest Shift + `_ / RO` key `0x33` | `_` |
| `'` | guest Shift + `7` key `0x07` | `'` |
| Shift+`'` | guest Shift + `2` key `0x02` | `"` |
| `\` | guest Yen/backslash key `0x0d` | `\` |
| `[` | guest `[` key `0x1b` | `[` |
| `]` | guest `]` key `0x28` | `]` |

Additional US shifted punctuation handled by the same table:

`^`, `&`, `*`, `(`, `)`, `+`, `{`, `}`, `|`, `` ` ``, `~`, `<`,
`>`, `?`

Pass-through regression cases in US keytop mode:

| US host input | Guest action |
|---|---|
| `,` | guest `,` key `0x30` |
| `.` | guest `.` key `0x31` |
| `/` | guest `/` key `0x32` |
| `-` | guest `-` key `0x0b` |
| `;` | guest `;` key `0x26` |

Unresolved mappings remain visible in the inventory and GUI binding
table. The VA `PC` key is bound to `ScrollLock` by default because VA2/3
use PC-held reset or power-on for the BIOS setup path, and popup helpers
use PC key chords such as PC+D.

## Tenkeyless Game Overlay

`keyboard_tenkey_overlay=1` is a game-oriented overlay independent of
the JIS/US/Custom host layout. It is off by default. When enabled from
Device / Keyboard / Tenkey overlay, the following host scancodes are sent
as guest keypad make/break events:

| Host scancode | Guest keypad role | Guest code |
|---|---|---:|
| Y | keypad 7 | 0x42 |
| U | keypad 8 | 0x43 |
| I | keypad 9 | 0x44 |
| H | keypad 4 | 0x46 |
| J | keypad 5 | 0x47 |
| K | keypad 6 | 0x48 |
| N | keypad 1 | 0x4a |
| M | keypad 2 | 0x4b |
| Comma | keypad 3 | 0x4c |
| Period | keypad 0 | 0x4e |

This overlay is intended for games on tenkeyless keyboards. It does not
change the normal binding table and does not inject guest text. While it
is enabled, these host keys are consumed before Roman-Kana or US keytop
translation.

## Inventory

Status values:

- `implemented`: carried over from the working SDL2 map or proven and
  exercised by the automated mapping tests.
- `mapped-but-untested`: guest code is proven, but the physical host key
  needs manual G14 confirmation.
- `unresolved`: guest code or default host scancode is incomplete and is
  visible in the GUI.

| Physical key | Semantic | Pre-M14 SDL2 | Guest | JIS preset | US preset | Status | Evidence |
|---|---|---|---:|---|---|---|---|
| STOP | KEY88_STOP | Pause | 0x60 | Pause | Pause | implemented | `machine/keystat.h`, `win9x/winkbd.cpp`, `sdl2/sdlkbd.c` |
| COPY | KEY88_COPY | PrintScreen | 0x61 | PrintScreen | PrintScreen | implemented | `machine/keystat.h`, `win9x/winkbd.cpp`, `sdl2/sdlkbd.c` |
| F1 | KEY88_F1 | F1 | 0x62 | F1 | F1 | implemented | `machine/keystat.h`, `win9x/winkbd.cpp`, `sdl2/sdlkbd.c` |
| F2 | KEY88_F2 | F2 | 0x63 | F2 | F2 | implemented | same as F1 |
| F3 | KEY88_F3 | F3 | 0x64 | F3 | F3 | implemented | same as F1 |
| F4 | KEY88_F4 | F4 | 0x65 | F4 | F4 | implemented | same as F1 |
| F5 | KEY88_F5 | F5 | 0x66 | F5 | F5 | implemented | same as F1 |
| F6 | KEY88_F6 | F6 | 0x67 | F6 | F6 | implemented | same as F1 |
| F7 | KEY88_F7 | F7 | 0x68 | F7 | F7 | implemented | same as F1 |
| F8 | KEY88_F8 | F8 | 0x69 | F8 | F8 | implemented | same as F1 |
| F9 | KEY88_F9 | F9 | 0x6a | F9 | F9 | implemented | same as F1 |
| F10 | KEY88_F10 | F10 | 0x6b | F10 | F10 | implemented | same as F1 |
| ROLL UP | KEY88_ROLLUP | PageUp | 0x36 | PageUp | PageUp | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| ROLL DOWN | KEY88_ROLLDOWN | PageDown | 0x37 | PageDown | PageDown | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| ESC | KEY88_ESC | Escape | 0x00 | Escape | Escape | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| 1 | KEY88_1 | 1 | 0x01 | 1 | 1 | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| 2 | KEY88_2 | 2 | 0x02 | 2 | 2 | implemented | same as 1 |
| 3 | KEY88_3 | 3 | 0x03 | 3 | 3 | implemented | same as 1 |
| 4 | KEY88_4 | 4 | 0x04 | 4 | 4 | implemented | same as 1 |
| 5 | KEY88_5 | 5 | 0x05 | 5 | 5 | implemented | same as 1 |
| 6 | KEY88_6 | 6 | 0x06 | 6 | 6 | implemented | same as 1 |
| 7 | KEY88_7 | 7 | 0x07 | 7 | 7 | implemented | same as 1 |
| 8 | KEY88_8 | 8 | 0x08 | 8 | 8 | implemented | same as 1 |
| 9 | KEY88_9 | 9 | 0x09 | 9 | 9 | implemented | same as 1 |
| 0 | KEY88_0 | 0 | 0x0a | 0 | 0 | implemented | same as 1 |
| - | KEY88_MINUS | Minus | 0x0b | Minus | Minus | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| ^ | KEY88_CARET | Equals | 0x0c | Equals | Equals | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| Yen | KEY88_YEN | Backslash | 0x0d | International3 | Backslash | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| Backspace | KEY88_BS | Backspace | 0x0e | Backspace | Backspace | implemented | `machine/keystat.h`, `win9x/winkbd.cpp`, `sdl2/sdlkbd.c` |
| TAB | KEY88_TAB | Tab | 0x0f | Tab | Tab | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| Q | KEY88_q | Q | 0x10 | Q | Q | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| W | KEY88_w | W | 0x11 | W | W | implemented | same as Q |
| E | KEY88_e | E | 0x12 | E | E | implemented | same as Q |
| R | KEY88_r | R | 0x13 | R | R | implemented | same as Q |
| T | KEY88_t | T | 0x14 | T | T | implemented | same as Q |
| Y | KEY88_y | Y | 0x15 | Y | Y | implemented | same as Q |
| U | KEY88_u | U | 0x16 | U | U | implemented | same as Q |
| I | KEY88_i | I | 0x17 | I | I | implemented | same as Q |
| O | KEY88_o | O | 0x18 | O | O | implemented | same as Q |
| P | KEY88_p | P | 0x19 | P | P | implemented | same as Q |
| @ | KEY88_AT | none | 0x1a | LeftBracket | LeftBracket | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| [ | KEY88_BRACKETLEFT | none | 0x1b | RightBracket | RightBracket | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| Return left | KEY88_RETURNL | Return | 0x1c | Return | Return | implemented | `machine/keystat.h`, `win9x/winkbd.cpp`, `sdl2/sdlkbd.c` |
| CTRL/Roman | KEY88_CTRL | LeftCtrl/RightCtrl | 0x74 | LeftCtrl | LeftCtrl | implemented | `machine/keystat.h`, `io/serial.c`, `sdl2/sdlkbd.c` |
| CAPS | KEY88_CAPS | CapsLock | 0x71 | CapsLock | CapsLock | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| A | KEY88_a | A | 0x1d | A | A | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| S | KEY88_s | S | 0x1e | S | S | implemented | same as A |
| D | KEY88_d | D | 0x1f | D | D | implemented | same as A |
| F | KEY88_f | F | 0x20 | F | F | implemented | same as A |
| G | KEY88_g | G | 0x21 | G | G | implemented | same as A |
| H | KEY88_h | H | 0x22 | H | H | implemented | same as A |
| J | KEY88_j | J | 0x23 | J | J | implemented | same as A |
| K | KEY88_k | K | 0x24 | K | K | implemented | same as A |
| L | KEY88_l | L | 0x25 | L | L | implemented | same as A |
| ; | KEY88_SEMICOLON | none | 0x26 | Semicolon | Semicolon | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| : | KEY88_COLON | none | 0x27 | Apostrophe | Apostrophe | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| ] | KEY88_BRACKETRIGHT | none | 0x28 | Backslash | NonUSHash | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| Shift left | KEY88_SHIFTL | LeftShift | 0x70 | LeftShift | LeftShift | implemented | `machine/keystat.h`, `io/serial.c`, `sdl2/sdlkbd.c` |
| Z | KEY88_z | Z | 0x29 | Z | Z | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| X | KEY88_x | X | 0x2a | X | X | implemented | same as Z |
| C | KEY88_c | C | 0x2b | C | C | implemented | same as Z |
| V | KEY88_v | V | 0x2c | V | V | implemented | same as Z |
| B | KEY88_b | B | 0x2d | B | B | implemented | same as Z |
| N | KEY88_n | N | 0x2e | N | N | implemented | same as Z |
| M | KEY88_m | M | 0x2f | M | M | implemented | same as Z |
| , | KEY88_COMMA | Comma | 0x30 | Comma | Comma | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| . | KEY88_PERIOD | Period | 0x31 | Period | Period | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| / | KEY88_SLASH | Slash | 0x32 | Slash | Slash | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| _ / RO | KEY88_UNDERSCORE | none | 0x33 | International1, alias NonUSHash | Grave | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| Shift right | KEY88_SHIFTR | RightShift as 0x70 | 0x58 | RightShift | RightShift | mapped-but-untested | `io/serial.c` |
| KANA | KEY88_KANA | none | 0x72 | RightAlt, alias International2 | RightAlt | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| GRPH | KEY88_GRAPH | LeftAlt/RightAlt | 0x73 | LeftAlt | LeftAlt | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| NFER/KETTEI | KEY88_KETTEI | none | 0x51 | International5 | F11 (when not used by Fast forward) | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp`, `io/serial.c` |
| SPACE | KEY88_SPACE | Space | 0x34 | Space | Space | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| XFER/HENKAN | KEY88_HENKAN | none | 0x35 | International4 | Application | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| PC | KEY88_PC | none; F12 option | 0x5a | ScrollLock | ScrollLock | mapped-but-untested | `win9x/winkbd.cpp`, `io/serial.c`, `sdl2/kbdmap.c` |
| ZENKAKU | KEY88_ZENKAKU | none | 0x5b | Lang5 | unassigned | mapped-but-untested | `win9x/winkbd.cpp`, `io/serial.c` |
| INSERT | KEY88_INS | Insert | 0x38 | Insert | Insert | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| DELETE | KEY88_DEL | Delete | 0x39 | Delete | Delete | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| UP | KEY88_UP | Up | 0x3a | Up | Up | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| DOWN | KEY88_DOWN | Down | 0x3d | Down | Down | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| LEFT | KEY88_LEFT | Left | 0x3b | Left | Left | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| RIGHT | KEY88_RIGHT | Right | 0x3c | Right | Right | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| HOME CLR | KEY88_HOME | Home | 0x3e | Home | Home | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| HELP | KEY88_HELP | End | 0x3f | Help | End | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad - | KEY88_KP_SUB | KP Minus | 0x40 | KP Minus | KP Minus | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad / | KEY88_KP_DIVIDE | KP Divide | 0x41 | KP Divide | KP Divide | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad 7 | KEY88_KP_7 | KP 7 | 0x42 | KP 7 | KP 7 | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad 8 | KEY88_KP_8 | KP 8 | 0x43 | KP 8 | KP 8 | implemented | same as keypad 7 |
| keypad 9 | KEY88_KP_9 | KP 9 | 0x44 | KP 9 | KP 9 | implemented | same as keypad 7 |
| keypad * | KEY88_KP_MULTIPLY | KP Multiply | 0x45 | KP Multiply | KP Multiply | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad 4 | KEY88_KP_4 | KP 4 | 0x46 | KP 4 | KP 4 | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad 5 | KEY88_KP_5 | KP 5 | 0x47 | KP 5 | KP 5 | implemented | same as keypad 4 |
| keypad 6 | KEY88_KP_6 | KP 6 | 0x48 | KP 6 | KP 6 | implemented | same as keypad 4 |
| keypad + | KEY88_KP_ADD | KP Plus | 0x49 | KP Plus | KP Plus | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad 1 | KEY88_KP_1 | KP 1 | 0x4a | KP 1 | KP 1 | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad 2 | KEY88_KP_2 | KP 2 | 0x4b | KP 2 | KP 2 | implemented | same as keypad 1 |
| keypad 3 | KEY88_KP_3 | KP 3 | 0x4c | KP 3 | KP 3 | implemented | same as keypad 1 |
| keypad = | KEY88_KP_EQUAL | F12 option only | 0x4d | KP Equals | KP Equals | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| keypad 0 | KEY88_KP_0 | KP 0 | 0x4e | KP 0 | KP 0 | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad , | KEY88_KP_COMMA | F12 option only | 0x4f | KP Comma | KP Comma | mapped-but-untested | `machine/keystat.h`, `win9x/winkbd.cpp` |
| keypad . | KEY88_KP_PERIOD | KP Period | 0x50 | KP Period | KP Period | implemented | `machine/keystat.h`, `sdl2/sdlkbd.c` |
| keypad Enter | KEY88_RETURNR | none | 0x59 | KP Enter | KP Enter | mapped-but-untested | `win9x/winkbd.cpp`, `io/serial.c` |

## Roman-Kana Helper

Roman-Kana accepts only A-Z and apostrophe host scancodes. It converts
Roman syllables into internal tokens, then emits guest key sequences. It
never injects Unicode or Shift-JIS bytes into the guest. SDL_TEXTINPUT is
ignored for guest Roman-Kana so host IME state and UTF-8 composition do
not affect guest input. The menu chooses only the kana input method.
Guest kana mode is controlled by the assigned KANA key: one press locks
KANA, and the next press unlocks it. Roman-Kana consumes A-Z host
scancodes only when that KANA lock mirror is active; with KANA unlocked,
A-Z remains direct alphabetic guest input.

The physical kana mapping is derived from `bios/keytable.res`: for
example `ka` uses the VA `T` key in Kana mode, `shi` uses `D`, `nn` uses
`Y`, voiced marks use `@`, semi-voiced marks use `[`, and small-tsu uses
Shift+`Z`. Yoon syllables use the base kana plus small `ya/yu/yo`,
which are Shift+`7`, Shift+`8`, and Shift+`9` in the same table.
`vu` uses `u` plus the voiced mark, and `va/vi/ve/vo` append small
`a/i/e/o`.
Unsupported Roman input is flushed with a log message and no guest memory
or text buffer writes.

Implemented parser inputs:

`a i u e o`, `ka ki ku ke ko`, `sa shi si su se so`,
`ta chi tsu tu te to`,
`na ni nu ne no`, `ha hi fu he ho`, `ma mi mu me mo`, `ya yu yo`,
`ra ri ru re ro`, `wa wo`, `nn`, `n'`, `ga gi gu ge go`,
`za ji zu ze zo`, `da de do`, `ba bi bu be bo`, and
`pa pi pu pe po`.

Yoon inputs:

`kya kyu kyo`, `sha shu sho` (`sya syu syo` aliases),
`cha chu cho` (`cya cyu cyo` and `tya tyu tyo` aliases),
`nya nyu nyo`, `hya hyu hyo`, `mya myu myo`, `rya ryu ryo`,
`gya gyu gyo`, `ja ju jo` (`jya jyu jyo` and `zya zyu zyo` aliases),
`bya byu byo`, and `pya pyu pyo`.

Additional inputs: `va vi vu ve vo`.

Small-kana aliases: `xya xyu xyo`, `lya lyu lyo`, `xa xi xu xe xo`,
`la li lu le lo`, `xtsu ltsu`, and `xtu ltu`.

Uppercase ASCII is accepted as lowercase. Doubled non-`n` consonants
emit small-tsu when followed by a supported syllable.
