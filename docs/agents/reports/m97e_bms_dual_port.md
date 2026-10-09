<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M97e: BMS native and compatibility port aliases

## Scope

The development disk and its `BMSDRVA.SYS` are inputs to this fix and remain
unchanged. The VAEG bank-memory device is the only implementation changed.

## Observed contract

The fixed BMS driver on the development disk probes the PC-9801-compatible
`00ECH` selector. Native PC-88VA software uses `01D0H`. Before this fix VAEG
registered the device only at the configured address, whose clean default is
`01D0H`; an unhandled `00ECH` probe could therefore be interpreted by the
guest as a one-bank result.

## Implementation

When BMS is enabled, `io/bmsio.c:bmsio_bind()` registers the same input and
output callbacks at the configured primary address and at the other supported
address. The two ports are aliases for the existing single `bmsio.bank`
state and allocation. `BMS_Port=01d0` remains the default and preferred
configuration; selecting `00ec` remains valid for compatibility users.
No second bank device, disk patch, or alternate memory window is introduced.

## Verification

The extended `test_va_bms_window()` selftest exercises both aliases with a
native preference and with a compatibility preference. It verifies that a
bank selected through either address is returned by reads from both addresses,
then runs the existing main-RAM pass-through, bank isolation, ordinary-reset
retention, and disable-time release checks.

Validated locally:

```text
cmake --build build/linux-debug -j2                         PASS
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/linux-debug/sdl2/vaeg --selftest --no-cfg \
  --no-bkupmem --mute                                     PASS
ctest --test-dir build/linux-debug --output-on-failure      PASS (no tests registered)
check_encoding.py / check_eol.py / check_case.py            PASS
clang-format-mp-22 --dry-run --Werror                       PASS
git diff --check                                            PASS
```

Real VA and VA2 guest confirmation is not included in this report.
