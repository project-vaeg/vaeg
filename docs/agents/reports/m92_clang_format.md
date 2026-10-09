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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M92: canonical clang-format validation

## Status

M92 is complete on `topic/m92-clang-format`. Automated validation passed, and
the maintainer passed G92 using the corrected normal Release handoff from
candidate `caa1f403cd0c1f6ce7673d6f839de7d3932c5316`.

The first G92 attempt did not pass because the maintainer observed an overall
slowdown.

The initial handoff incorrectly identified test-enabled integration-trace
executables as the artifacts for the normal-maintainer-configuration gate.
Those executables are valid validation workers, but they are not production
release executables and must not be used for runtime-performance acceptance.

## Scope and tool authority

The canonical executable name is `clang-format-mp-22`; the validated tool is
MacPorts clang-format 22.1.8. `tools/repo/clang_format.py` rejects a different
major version and applies only the closed path list in
`tools/repo/clang_format_files.txt`.

The manifest starts from the union of tracked first-party C/C++ dependencies in
test-enabled macOS and MinGW CMake/Ninja builds, plus the conditional
uPD9002 performance diagnostic translation unit. Three byte-immutable
uPD9002 files are excluded, producing 381 canonical paths:

- `cpu/upd9002/upd9002_ea.c`
- `cpu/upd9002/upd9002_state.c`
- `cpu/upd9002/upd9002_state.h`

Their approved hashes remain enforced by
`tools/qa/upd9002_protected_deletion.py`; M92 does not rewrite the protected
history to make formatting pass. Vendored, generated, inactive compatibility,
NASM, media, and guest-demo files are also outside the manifest.

## Style result

The root `.clang-format` preserves include order, disables include sorting and
comment reflow, and does not split string literals. It sets
`KeepEmptyLines.AtStartOfBlock` to `false`. In particular, a function's opening
brace is followed immediately by its first declaration or statement. The final
explicit function-start scan found zero blank-line matches in the manifest.

Of the 381 manifest entries, 339 required rewriting:

| Mechanical group | Files | Insertions | Deletions | Commit |
| --- | ---: | ---: | ---: | --- |
| Core, VA devices, CPU adapters, sound, storage, and shared code | 236 | 18,995 | 22,739 | `e5941339675fe6d2d47a1c6b8771c14fb13a09f2` |
| SDL2 frontend and tests | 103 | 9,226 | 11,904 | `88dda51ddb2ce0763407c8feef06738d6b48a968` |
| Total mechanical normalization | 339 | 28,221 | 34,643 | two commits above |

Both mechanical commit IDs are recorded in `.git-blame-ignore-revs`.

## Validator adaptation

Three source-contract checks encoded the legacy tab or comma spacing literally.
M92 changes only those formatting assumptions:

- `upd9002_native_invariant.py` recognizes the canonical `UINT8 cpu_type;`
  declarations;
- `upd9002_rename.py` recognizes the canonical register-model declaration; and
- `m75_scsi_controller.py` accepts whitespace between the protected INQUIRY
  qualifier bytes and checks canonical structure-member/return spelling.

The checks retain their semantic counts, symbols, and control-flow assertions.
No guest-visible behavior or emulator timing was changed, so the permanent
bug-fix ledger is unaffected.

## Commands and results

Baseline configuration before normalization:

```sh
cmake --preset macos-macports \
  -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build --preset macos-macports -j4

CCACHE_DISABLE=1 cmake --preset mingw-cross \
  -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4
```

Both baseline builds passed. Final validation used:

| Command | Result |
| --- | --- |
| `python3 tools/repo/clang_format.py` | PASS, 381 files canonical with `clang-format-mp-22` |
| `python3 tools/repo/check_encoding.py --expect utf8` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `git diff --check github/main...HEAD` | PASS |
| `cmake --build --preset macos-macports -j4` | PASS, 230 compile/link steps |
| `ctest --test-dir build/macos-macports --output-on-failure` | PASS, 83/83; external SST test skipped by its existing policy |
| `build/macos-macports/sdl2/vaeg --selftest` | PASS, all selftests |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` | PASS, 230 compile/link steps |

Git-backed checks were run with `GIT_CONFIG_GLOBAL=/dev/null` and
`GIT_CONFIG_SYSTEM=/dev/null` so sandbox denial of the maintainer's global Git
configuration could not be confused with a repository failure. Existing
compiler warnings were not changed in this formatting milestone.

Validation-worker identities after the test-enabled builds:

- macOS arm64 `build/macos-macports/sdl2/vaeg`:
  `d1b8b1bd648d8ebd2b7a69e45cd6bac9d931337fe504c899c02984d8c09efab6`
- MinGW x86-64 `build/mingw-cross/sdl2/vaeg.exe`:
  `e1003bf06fa4ccdf0c7c6c59c53b8abd2a98dce35641079370809e658e5653f3`

Both workers were configured with `VAEG_ENABLE_TESTS=ON` and
`VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON`. The trace option retains per-instruction
trace calls even while no trace stream is active, so these workers can be
slower than production releases. This is a build-configuration effect, not an
accepted M92 runtime result.

## Failed-gate diagnosis and corrected artifacts

The failed report was investigated without changing emulator source. Fresh
normal Release builds explicitly set `VAEG_ENABLE_TESTS`,
`VAEG_Z80_COMPAT_INTEGRATION_TRACE`, and `VAEG_UPD9002_PERF_DIAGNOSTIC` to
`OFF`. They produced:

- macOS arm64 `build/g92-release/sdl2/vaeg`:
  `6dbaba4be0e7018ccabb43440db289349f4f5616226ff005df3bde09a67cbbe4`
- MinGW x86-64 `build/g92-mingw-release/sdl2/vaeg.exe`:
  `65dc984f77d698741f4b9484adc574330271f3a84f064557b618fa412ef50e5f`

The two corrected executables were built from evaluated implementation commit
`03b6ad139ea4a33d0691be8ff313c99ba9296afc`. The macOS executable passed the
ROM-less selftest.

The same compiler, target, Release flags, and disabled diagnostic options were
then used to rebuild predecessor `a7aaeba81b3828927019b9567c3c8d6ae087a708`.
For both macOS and MinGW, 172 of 173 first-party object files were byte-for-byte
identical between the predecessor and M92. The only differing object was
`generic/np2info.c`, whose generated build-commit string necessarily differs;
its disassembly was identical. The corresponding linked executable sizes were
also identical on each platform. Under these controlled builds, M92 formatting
therefore produces identical production instructions and no source-induced
performance difference was found.

The exact executable path used in the first failed human attempt was not
recorded, so the failed observation is not overwritten or reclassified as a
pass. G92 requires a fresh maintainer retest with one of the corrected normal
Release executables.

No hosted CI result is claimed in this report. No archived reference-tier path,
vendored dependency, ROM, disk image, font, icon, wave data, or private
integration asset was modified.

## Commit sequence

1. `59359ddc` - define the pinned style, manifest, task, and checker.
2. `e5941339` - normalize active emulator core sources.
3. `88dda51d` - normalize active SDL2 and test sources.
4. `5a805dec` - align source-contract validators with canonical whitespace.
5. `0ae6edd9` - register the two mechanical commits for blame.
6. `caa1f403` - correct the human-gate artifact handoff after the failed attempt.

## G92 human gate

From a clean checkout of the reported final candidate:

1. build with the normal maintainer configuration;
2. boot in native V3 mode;
3. run the bundled VA demo;
4. boot an OS and perform simple keyboard, disk, display, sound, and state-save
   operations; and
5. confirm behavior is unchanged.

On 2026-08-15, the maintainer reported that the human gate passed after the
corrected normal Release handoff. G92 is passed, M92 is closed, and integration
into `main` is authorized.
