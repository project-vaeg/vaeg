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
# M30: Restore the VA BMS memory window

Status: complete; G30 architectural correction accepted

Date: 2026-07-13

## Scope

Restore the PC-88VA `80000H-9FFFFH` I-O Bank Memory window semantics in the
active portable C memory layer. This is a focused correction to the M9 port of
`cpuxva/memoryva.x86`; it does not change the frozen reference tier, add a BMS
configuration GUI, or alter main RAM, system-memory banks, FDD, or sound.

## Reported behavior

After M29 corrected the VA1 TVRAM aperture, PC-Engine 1.00 boots and enters
N88 BASIC V3.0. The maintainer then observed:

- `FILES` reports an error;
- `BEEP` leaves the VA1 guest apparently hung.

Earlier diagnostics had already shown that disabling host sound, suppressing
BEEP PCM registration, and suppressing the BEEP event did not remove the
failure. `LIST` and `FILES` could also reproduce related behavior without the
BEEP sound path. The next investigation therefore returned to VA1 memory
detection rather than changing the audio implementation.

## Portable-port defect

The frozen VA memory dispatch maps `80000H-9FFFFH` to the BMS handlers in
`i286x/memory.x86`:

```text
cpuxva/memoryva.x86: @bms_wt/@bmsw_wt/@bms_rd/@bmsw_rd
i286x/memory.x86:    selected 128KB bmsio bank, or open bus when absent
```

The M9 audit explicitly recorded that these assembly-internal handlers had no
callable equivalent in `i286c/memory.c` and required local equivalents or a C
core hook. The initial C port instead implemented all four local BMS helpers by
temporarily leaving VA memory mode and calling normal `i286_memory*()` access.
Consequently, the active build exposed ordinary writable `mem[]` storage at
`80000H-9FFFFH` even while `bmsiocfg.enabled` was false and `bmsio.nomem` was
true.

That behavior differs from both the frozen implementation and its canonical
comparison configuration (`Use_BMS_=false`). It makes a nonexistent 128KB
aperture pass guest write/read memory probes. This is a concrete M9 porting
defect irrespective of the separately observed V3 BASIC failures.

## Correction

`cpucva/memoryva.c` now implements local C equivalents of the frozen BMS
handlers:

- `bmsio.nomem != 0` or missing storage: reads return `FFH`/`FFFFH` and writes
  are ignored;
- a valid selected bank addresses
  `bmsiowork.bmsmem + bank * 0x20000 + (address - 0x80000)`;
- byte and little-endian word access are range checked against the allocated
  BMS storage.

The standard active configuration still leaves BMS disabled. Existing BMS
allocation, bank selection, reset, binding, and state-save ownership remain in
`io/bmsio.c` and `cbus/cbuscore.c`.

## Automated verification

The ROM-less selftest covers:

- disabled-window byte and word open-bus reads;
- ignored writes while the window is disabled;
- enabled bank-0 byte and word access;
- enabled bank-1 byte and word access;
- bank switching without aliasing or loss of bank-0 data.

Required checks:

```text
cmake --build --preset linux-release
SDL_AUDIODRIVER=dummy ./build/linux-release/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --smoke
cmake --build --preset mingw-cross
git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
```

## G30 result

The maintainer tested the MinGW build with the normal VA1 ROM set and BMS
disabled. PC-Engine 1.00 continued to boot, but N88 BASIC V3.0 `FILES` still
entered the previously observed apparent hang. Therefore:

- the BMS correction is not the root cause of the `FILES`/`BEEP` defect;
- no claim is made that M30 fixes that defect;
- the maintainer accepted M30 independently as restoration of frozen-reference
  BMS semantics;
- ROM-less tests verify both the absent-window behavior used by the default
  configuration and enabled two-bank selection.

The unresolved BASIC behavior is deferred to a separate investigation so that
its diagnostics and eventual fix are not mixed with this memory-map parity
correction.

## Remaining work

The active SDL2 GUI does not expose optional BMS enable, I/O port, or bank
count settings. That is existing GUI parity work and is not required to model
the default absent aperture correctly. The VA1 N88 BASIC V3.0 `FILES`, `LIST`,
and `BEEP` behavior remains unresolved. The next investigation should capture
the post-command CPU state; sound-path changes remain unsupported by the
evidence collected so far.
