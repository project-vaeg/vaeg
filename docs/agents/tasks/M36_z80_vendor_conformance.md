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
# M36: Vendor the approved Z80 core and add standalone conformance

Status: implemented; G35 passed and M36 was explicitly authorized on
2026-07-15; stop at G36 for maintainer review

Branch: `topic/m36-z80-vendor-conformance`

## Goal and boundaries

Reproduce the approved MIT tree from upstream base
`e3926769a790fab0af1c34a5540e317f8d4f0ddc` and the approved downstream patch,
then vendor only the required files under `external/suzukiplan-z80/` and prove
them independently. The expected tree is
`8a606eb39332a6e79b69bb62d9dedca042b923dc`. Do not write the vaeg wrapper,
connect it to the emulator, change the selected core, migrate state, replace
disassembly, or delete legacy files.

## Vendoring and notices

Before copying, verify patch SHA-256
`d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4`, apply it
with `git am`, reproduce tested commit
`b4a0a5a238fecc280781e6fe5719faf0eafcd667` when the committer date is made
deterministic, reproduce the expected tree, and prove `LICENSE.txt` unchanged.
Import only required source/license/test files without hand edits. Add
`provenance.txt` recording the upstream repository, base, patch path/hash,
tested commit, resulting tree, MIT license, milestone/date, and extension
summary. The lowercase descriptive name avoids shadowing the C++ standard
library's `<version>` header on case-insensitive filesystems. Preserve
`LICENSE.txt` verbatim. Update the third-party notice and ADR, distinguishing
upstream/downstream MIT code, vaeg BSD-2-Clause code, and external GPL test
inputs. A later third-party source change requires a newly approved
base-and-patch reproduction in its own commit.

## Standalone validation

Add target-local compile tests for normal callbacks, `Z80_NO_FUNCTIONAL`,
debug/default, and supported release-oriented feature switches on Linux
GCC/Clang, macOS, and Windows-MinGW. Mirror the acceptance callback, level IRQ,
raw IM0, IM1/2, EI, HALT, and NMI tests required at G35 so a bad import cannot
pass. The complete existing upstream suite remains required only in its
supported default callback configuration; focused tests are required in both
callback configurations. The approved baseline limitation for legacy tests
under forced `Z80_NO_FUNCTIONAL` remains unchanged.

Add a deterministic headless CP/M-style runner under `tests/z80_compat/`: 64 KiB,
load address `0x0100`, sufficient BDOS CALL-5 output handling, bounded clock
and wall time, clear pass/fail parsing, and diagnostic state on failure. Run
ZEXDOC and ZEXALL from immutable raw URLs at the approved upstream base. The
dedicated CI job must verify both `.cim` files, both corresponding `.src`
files, and the license SHA-256 before execution, support a user-provided
offline cache, and prove no fetched artifact enters source or release
archives. Do not link or package GPL test inputs with vaeg.

## Checks and G36

Run repository invariants, all existing builds/CTest/smoke, three-platform
compile tests, focused interrupt tests in both configurations, both ZEX
programs, and source/release archive exclusion tests. Record native execution,
Wine execution, cross-compilation, remote CI, and unavailable platforms
separately. G36 is machine plus maintainer review and requires the emulator
still to use the legacy core. Push, report exact vendored hashes, commands,
results, changed files, and SHAs, then stop. Do not begin M37.
