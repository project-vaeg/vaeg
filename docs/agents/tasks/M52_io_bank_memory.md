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
# M52: Restore portable I/O Bank Memory configuration

Status: **G52 passed**

Starting SHA: `97d19aa979f2ec235b2b72c6bda9bba69f94eefa`

## Goal

Restore the active SDL2 frontend path needed to configure the optional
PC-88VA I/O Bank Memory device. M52 also corrects the guest-visible
`80000H-9FFFFH` aperture after RDBMS 1.21 testing demonstrated that bank zero
must restore main RAM rather than select an expansion bank.

## Required behavior

- Keep BMS disabled by default.
- Persist the legacy-compatible `Use_BMS_`, `BMS_Port`, and `BMS_Size` keys in
  `vaeg.cfg`.
- Expose enable, I/O port, and 128KB bank count in the SDL2 Device menu.
- Support the two legacy UI port choices, `00ECH` and `01D0H`.
- Use the PC-88VA-native RAM-bank control port `01D0H` for clean configuration
  and invalid-port fallback; retain `00ECH` as an explicit compatibility
  choice.
- Accept 1 through 255 banks and display the resulting capacity.
- Interpret a port value of zero as main-RAM pass-through at
  `80000H-9FFFFH`.
- Map BMS banks 1 through N onto the configured N 128KB storage banks.
- Preserve the same mapping for CPU byte/word and SGP word accesses.
- Apply a changed configuration through the normal guest-reset path so BMS
  allocation, I/O binding, and the memory window change together.
- Preserve unchanged BMS storage across an ordinary reset.
- Reject or canonicalize invalid persisted settings without binding an
  unexpected I/O port or silently enabling a zero-sized device.

## Non-goals and invariants

- Do not reduce 640KB main memory merely because BMS is enabled.
- Do not change state-save section names or payload layout.
- Do not edit the frozen Win9x dialog or assembly memory reference.
- Do not enable BMS by default or bundle third-party BMS software.
- Do not change main RAM, system-memory banks, TVRAM, GVRAM, CPU, SGP, FDD,
  or sound behavior.

## Automated validation

- Extend the ROM-less selftest to cover configuration copy, allocation,
  disabled release, selected-bank isolation, and ordinary-reset retention.
- Run Linux GCC, Linux Clang, ASan/UBSan, and MinGW builds and tests available
  on the host.
- Run the standard smoke test and repository encoding, EOL, case, and diff
  checks.

## Human gate

From a clean checkout and clean configuration:

1. Confirm BMS starts disabled and V3 mode, the bundled VA demo, and an OS
   still boot normally.
2. Open Device / I/O Bank Memory, confirm `01D0H` is the clean-config default,
   select 16 banks, apply it, and confirm the guest resets cleanly.
3. With 640KB main memory, load RDBMS using `-P1D0 -S1`; confirm bank zero
   preserves DOS main memory, RDBMS and MSE register, and more than one BMS
   bank retains distinct contents. When using the driver's compiled `00ECH`
   default instead, select the `00ECH (PC-9801 mode)` UI choice.
4. Restart vaeg and confirm the three BMS settings persist.
5. Disable BMS, apply, and confirm the utility no longer detects the device
   while ordinary V3 operation remains normal.

G52 passed after maintainer confirmation of 640KB main-memory preservation,
RDBMS/MSE registration, RAM-disk operation, matched `01D0H`/`00ECH` port
configuration, reset, and ordinary V3/OS operation.

## Implementation record

- `7baa72a24acc0c4cbcf2fc2377a2b22e66941a98` restores the three
  legacy-compatible configuration keys and rejects unsupported ports or a
  zero bank count before machine initialization.
- `13908f45309b3e7370fb5fc7bcb2d727f0cb2532` restores the Device-menu
  dialog. Changes are transactional until OK; a changed configuration uses
  the normal reset path, while Cancel leaves both configuration and the live
  machine untouched.
- `cca7c3d38c6e126b866ec195745f87d1e7b77690` initially verified
  configuration-file round-trip, enabled allocation, two-bank isolation,
  same-size reset retention, and disable-time release.
- `e9ad63e3d720e8dad14d5a63289f3d3443b54422` makes `01D0H` the
  PC-88VA clean-config and invalid-value fallback, retains `00ECH` as the
  explicit compatibility choice, and adds exact default/fallback assertions.
- `5eb04ae91a9900833096bb43b3b599d358c099c5` corrects selector zero to
  pass through main RAM and maps selectors 1 through N onto all N configured
  banks for both CPU and SGP access paths.

The frozen `win9x/` dialog and `cpuxva/memoryva.x86` were not modified. The
existing BMS runtime/state structures and state-save section names are
unchanged.

Post-implementation RDBMS 1.21 source and guest testing corrected one M30/M52
assumption. The driver performs transfers through segment `8000H`, selects a
nonzero bank, and its `ResetBank` macro writes zero after each transfer. Its
compiled port default is `00ECH`, while `-P1D0` selects the PC-88VA-01/02
mode. VAEG therefore treats bank zero as main-RAM pass-through and numbers
allocated BMS storage from bank one. This retains all 640KB of conventional
memory outside the short interval in which a nonzero bank is selected.

## Local validation result

The following completed successfully:

```text
cmake --preset linux-debug
cmake --build --preset linux-debug --parallel 4
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --selftest

cmake --preset linux-release --fresh
cmake --build --preset linux-release --parallel 3
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --smoke

cmake --preset linux-ci-clang --fresh
cmake --build --preset linux-ci-clang --parallel 3
ctest --test-dir build/linux-ci-clang --output-on-failure
# 35/35 passed; the optional external SingleStepTests dataset test skipped.

cmake --preset linux-asan --fresh
cmake --build --preset linux-asan --parallel 3
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1 \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/asan/sdl2/vaeg --selftest

cmake --preset mingw-cross --fresh
cmake --build --preset mingw-cross --parallel 4
WINEDEBUG=-all WINEPREFIX=build/m51-wine-prefix \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --selftest
```

GCC, Clang, ASan, MinGW/Wine selftests reported
`VA BMS config/window lifecycle ok` for the original configuration work. The
updated lifecycle check additionally proves bank-zero main-RAM pass-through
and one-based BMS storage mapping. The ASan/UBSan run still reports the two
pre-existing sound diagnostics in `sound/tms3631c.c` and `sound/psggenc.c`;
both predate M52 and the selftest completes successfully. A separate ROM-less
smoke run with `BMS_Port=1234` and `BMS_Size=0` confirmed deterministic
warnings and fallback to port `01D0H`, 16 banks, before machine startup.
The final handoff binaries were rebuilt with `--clean-first`; Linux release
selftest and smoke, plus MinGW/Wine selftest, all exited 0.
