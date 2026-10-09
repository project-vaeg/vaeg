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
# M24 - Host clipboard paste

## Scope

M24 adds ASCII-only paste from the host clipboard to the SDL2 guest. The
frontend reads UTF-8 with `SDL_GetClipboardText()`, converts supported text to
proven VA guest key roles, and emits paced make/break events through the same
`kbdinject` and `keystat_senddata()` path as physical and Roman-Kana input.
It never writes guest RAM, BIOS/DOS buffers, VRAM, or Unicode text.

The user entry points are `Edit -> Paste`, Command+V on macOS, and Control+V
on Linux/Windows. A shortcut is ignored while Dear ImGui captures keyboard or
text input. Copy from guest to host is not part of M24.
An empty clipboard is a no-op and does not replace an active queue.

## Character mapping

All printable ASCII characters U+0020 through U+007E are mapped to a VA guest
key or a left-Shift guest chord using the key codes already recorded by the
M14 inventory in `sdl2/kbdmap.c`. Uppercase letters and punctuation requiring
Shift explicitly emit Shift make and break. CR, LF, and CRLF map to the left
Return key; CRLF produces one Return. Other ASCII control characters and each
non-ASCII UTF-8 code point are skipped and counted in the Edit-menu status.

Paste assumes the guest is in an alphabet input state compatible with its
physical keyboard. It does not toggle CAPS, KANA, ZENKAKU, or an IME.
Japanese clipboard text, host IME conversion, JIS-specific text conversion,
and guest-to-host copy remain future work.

## Queue and input safety

The queue sends one guest-visible transition every 20 ms. An unshifted
character therefore takes 40 ms, while a shifted character takes 80 ms. This
is intentionally conservative for guest BIOS and OS keyboard polling.

Starting a paste releases mirrored physical guest keys before the first
synthetic event. Opening a modal dialog, keyboard binding capture, or an ImGui
text field finishes only an in-flight release, then pauses the queue without
new makes or catch-up bursts. Reset, state load, focus
loss, quit, and frontend shutdown cancel it and release any synthetic key or
Shift still held. `Edit -> Cancel Paste` provides an explicit cancellation
path. A new paste replaces an active queue safely.

## Automated checks

ROM-less selftests cover all 95 printable ASCII characters, representative
direct and shifted guest actions, CRLF normalization, UTF-8/control-character
skip counting, and the 20 ms transition interval. Normal smoke and repository
invariant checks remain required.

## G24 human gate

- Paste lowercase and uppercase letters, digits, spaces, and punctuation into
  V3 BASIC and an OS text field or command prompt.
- Paste LF, CR, and CRLF multiline text and confirm CRLF submits one Return.
- Paste text containing Japanese or another non-ASCII character and confirm
  supported surrounding ASCII remains intact and the skipped count is shown.
- Confirm Command+V on macOS and Control+V on Linux/Windows.
- Type or paste into an ImGui text field and confirm no guest paste starts.
- Open a modal during a long paste and confirm the queue pauses and resumes.
- Reset and load state during a paste; confirm cancellation and no stuck key.
- Confirm `Edit -> Cancel Paste` and focus loss release all synthetic keys.
- Confirm normal physical keyboard, right-Alt KANA, and Roman-Kana behavior.
- Complete the standard V3 boot, VA demo, and OS boot/simple-operation gate.

## Gate result

G24 passed by user report on 2026-07-13. The implemented scope remains
ASCII host-to-guest paste; Japanese/IME paste and guest-to-host copy remain
future work.
