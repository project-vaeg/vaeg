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
# M36 Z80 vendoring and standalone conformance evidence

## Entry state and scope

G35 passed and the maintainer explicitly authorized M36. The branch started at
`topic/m36-z80-vendor-conformance`, commit
`6af802d45399edfb7ef161700b69512ae128cdae`. Tracked files were clean. Twelve
pre-existing untracked paths were reported before work and were not selected
by any edit, add, commit, or archive command. After an interrupted turn, the
eight `beep-*.log` paths were no longer present; M36 did not remove or recreate
them. The remaining four paths are still untouched.

M36 vendors and tests the raw core only. It does not implement `Z80C`, connect
the new core to the subsystem, change state save, replace disassembly, delete a
legacy file, or change guest-visible behavior. M37 was not started.

## Mandatory provenance reproduction

The successful clean reproduction used `/tmp/vaeg-m36-z80.7UocP2/upstream`:

| Command | Exact result |
|---|---|
| `git clone https://github.com/suzukiplan/z80 /tmp/vaeg-m36-z80.7UocP2/upstream` | PASS; origin is `https://github.com/suzukiplan/z80` |
| `git status --short` | no output |
| `git checkout --detach e3926769a790fab0af1c34a5540e317f8d4f0ddc` | PASS; `HEAD is now at e392676` |
| `git rev-parse HEAD` | `e3926769a790fab0af1c34a5540e317f8d4f0ddc` |
| `sha256sum docs/agents/reports/m35_suzukiplan_irq_extension.patch` | `d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4` |
| `sha256sum LICENSE.txt` before application | `ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1` |
| `git hash-object LICENSE.txt` before application | `a4cbbf62b0edaf761ef48556c7a2e50bb3b4817f` |
| `git am /home/maho/vaeg/docs/agents/reports/m35_suzukiplan_irq_extension.patch` | PASS; `Applying: M35: add level-sensitive IRQ acknowledge support` |
| `git rev-parse HEAD` after plain `git am` | `a93637b6360a7989dde6bfc503616d22b61c91f5` |
| `git rev-parse HEAD^{tree}` | `8a606eb39332a6e79b69bb62d9dedca042b923dc` |
| `sha256sum LICENSE.txt` after application | unchanged `ca7261ec...bddff1` |
| `git hash-object LICENSE.txt` after application | unchanged `a4cbbf...4817f` |
| `git status --short` | no output |

The plain `git am` commit differs only because its committer timestamp is new.
A second clean checkout used:

```sh
git checkout --detach e3926769a790fab0af1c34a5540e317f8d4f0ddc
git am --committer-date-is-author-date \
  /home/maho/vaeg/docs/agents/reports/m35_suzukiplan_irq_extension.patch
git rev-parse HEAD
git rev-parse HEAD^{tree}
```

The exact outputs are tested commit
`b4a0a5a238fecc280781e6fe5719faf0eafcd667` and tree
`8a606eb39332a6e79b69bb62d9dedca042b923dc`, with a clean worktree. This
resolves the commit-metadata qualification without weakening the mandatory
tree gate.

## Vendored files and integrity

The exact vendored file list is:

```text
external/suzukiplan-z80/LICENSE.txt
external/suzukiplan-z80/provenance.txt
external/suzukiplan-z80/test/test-interrupt-extension.cpp
external/suzukiplan-z80/z80.hpp
```

`provenance.txt` is vaeg-authored provenance. The other three files are
byte-for-byte copies from the reproduced tree:

| File | Git blob | SHA-256 |
|---|---|---|
| `LICENSE.txt` | `a4cbbf62b0edaf761ef48556c7a2e50bb3b4817f` | `ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1` |
| `z80.hpp` | `b4e1f9d357ff4076c9e4c2a9cb4b8ae64d273865` | `88c878f0087f114eb864dc6a9e8cb98473c022e052c62937ab7a23aecbdb7106` |
| `test/test-interrupt-extension.cpp` | `3708c77fabbfef85bd68da99fc7cef068af4ec55` | `abe7cd10642c6d13ad0636ef53091cd3c4bf643ecd6f564a22707393e06728c4` |

Three direct `cmp` commands against the deterministic checkout returned exit
0. No upstream file was hand-edited. The MIT text and copyright are unchanged.

## Standalone build and CI policy

CTest now owns four build/run header smoke targets:

```text
vaeg_z80_header_default
vaeg_z80_header_no_functional
vaeg_z80_header_debug_default
vaeg_z80_header_release
```

The release target defines `Z80_DISABLE_DEBUG`, `Z80_DISABLE_BREAKPOINT`,
`Z80_DISABLE_NESTCHECK`, and `Z80_NO_EXCEPTION`. The no-functional definition
is target-local. The two focused targets compile the exact vendored 21-case
test with normal callbacks and `Z80_NO_FUNCTIONAL`.

All existing Linux, Windows-MinGW, and macOS CI builds enable these targets.
A dedicated Linux job fetches and hash-verifies ZEX inputs, runs standalone
conformance, and checks a source archive. Release packaging checks every Linux,
Windows, and macOS runtime archive for prohibited ZEX names and content hashes.

## ZEX runner and acquisition

`tests/z80_compat/zex_runner.cpp` owns deterministic 64 KiB memory, loads at `0x0100`,
implements CP/M CALL 5 functions 2 and 9 through a small in-memory BDOS shim,
and recognizes the warm-boot halt. Default limits are 60,000,000,000 emulated
clocks and 600 wall seconds. Failure output includes register state, total
clocks, and the most recent 2 KiB of guest output.

`tests/z80_compat/fetch_zex.py` pins the approved upstream base and verifies all five
M34 hashes. Its offline reproduction command passed:

```sh
python3 tests/z80_compat/fetch_zex.py \
  --source-dir /tmp/vaeg-m36-z80.7UocP2/deterministic-am/test-ex \
  --output-dir /tmp/vaeg-m36-z80.7UocP2/zex-cache
```

All five outputs printed `verified` with the approved SHA-256. Re-running
against the populated directory printed `verified cache` for all five; the
cache is only an optimization. No `.cim`, `.src`, or fetched license is
tracked.

## Commands and results

| Command | Result |
|---|---|
| `make -C test` in the deterministic upstream result | PASS; complete supported default suite, including 21/21 focused tests in both callback configurations |
| `cmake --preset linux-ci-gcc -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m36-z80.7UocP2/zex-cache` | PASS |
| `cmake --build --preset linux-ci-gcc` | PASS |
| `ctest --test-dir build/linux-ci-gcc --output-on-failure` | PASS, 10/10; ZEXDOC 76.74 s, ZEXALL 70.75 s, ROM-less selftest 1.24 s |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-gcc/sdl2/vaeg --smoke` | PASS, exit 0, ROM-less software renderer |
| `cmake --preset linux-ci-clang` and `cmake --build --preset linux-ci-clang` | PASS |
| `ctest --test-dir build/linux-ci-clang --output-on-failure` | PASS, 8/8 |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-clang/sdl2/vaeg --smoke` | PASS, exit 0 |
| `cmake --preset linux-ci-asan` and `cmake --build --preset linux-ci-asan` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan --output-on-failure` | PASS, 8/8 |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 ./build/linux-ci-asan/sdl2/vaeg --smoke` | PASS, exit 0; existing documented UBSan backlog messages only |
| `cmake --preset mingw-cross -DVAEG_ENABLE_TESTS=ON` | PASS |
| `cmake --build --preset mingw-cross` | PASS; emulator and all standalone PE32+ executables linked |
| four MinGW header executables via Wine 10.0 with explicit `WINEPATH` | PASS, exit 0 each |
| both MinGW focused interrupt executables via Wine 10.0 | PASS, 21/21 each |
| MinGW legacy contract executable via Wine 10.0 | PASS, 15 fixtures |
| MinGW vaeg `--smoke` with dummy SDL drivers via Wine 10.0 | PASS, exit 0, ROM-less software renderer |
| `python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `python3 tools/repo/find_unreferenced.py` | PASS, 69 known unreferenced paths; no M36 path |
| `git diff --check` | PASS |
| Python YAML parse of both workflow files | PASS |
| `git archive --format=tar.gz --output=/tmp/vaeg-m36-source-final.tar.gz HEAD` then archive checker | PASS; 969 files, no ZEX artifact |
| Linux release-layout tar then archive checker | PASS; 5 files, no ZEX artifact |

The archive checker was also run against a deliberate archive containing the
five cache files. It exited 1 and rejected every file by both expected name and
content hash, proving the negative path.

The exact successful Wine commands were:

```sh
WINEPREFIX=/tmp/vaeg-m36-wine.gLhGu7 \
WINEDEBUG=-all \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
wine64 build/mingw-cross/vaeg_z80_header_default.exe
WINEPREFIX=/tmp/vaeg-m36-wine.gLhGu7 \
WINEDEBUG=-all \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
wine64 build/mingw-cross/vaeg_z80_header_no_functional.exe
WINEPREFIX=/tmp/vaeg-m36-wine.gLhGu7 \
WINEDEBUG=-all \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
wine64 build/mingw-cross/vaeg_z80_header_debug_default.exe
WINEPREFIX=/tmp/vaeg-m36-wine.gLhGu7 \
WINEDEBUG=-all \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
wine64 build/mingw-cross/vaeg_z80_header_release.exe
WINEPREFIX=/tmp/vaeg-m36-wine.gLhGu7 \
WINEDEBUG=-all \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
wine64 build/mingw-cross/vaeg_z80_interrupt_default.exe
WINEPREFIX=/tmp/vaeg-m36-wine.gLhGu7 \
WINEDEBUG=-all \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
wine64 build/mingw-cross/vaeg_z80_interrupt_no_functional.exe
WINEPREFIX=/tmp/vaeg-m36-wine.gLhGu7 \
WINEDEBUG=-all \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
wine64 build/mingw-cross/vaeg_z80_legacy_contract.exe
WINEPREFIX=/tmp/vaeg-m36-wine.gLhGu7 \
WINEDEBUG=-all \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
wine64 build/mingw-cross/sdl2/vaeg.exe --smoke
```

The first four returned exit 0, the interrupt programs each printed 21 passing
tests and returned exit 0, the contract program printed 15 passing fixtures and
returned exit 0, and the smoke command returned exit 0.

One diagnostic smoke invocation omitted the required dummy SDL environment and
was interrupted with exit 130 after host audio initialization. The exact CI
command with both dummy variables then passed and is the claimed smoke result.
Wine initially exited 53 until the dynamically linked standalone tests were
given the MinGW runtime directory through `WINEPATH`; every required Wine test
then passed. These diagnostic attempts are not hidden as successful tests.

The first hosted macOS build failed after configure because the initially named
`external/suzukiplan-z80/VERSION` shadowed libc++'s `<version>` header on the
runner's case-insensitive filesystem. Public Apple Clang annotations identify
the former file's lines 3, 4, and 5 as invalid preprocessing directives. Commit
`560fab64c8556771e5d1316337df1934a3402c51` renamed only the vaeg-authored
metadata to `provenance.txt`; the copied upstream files and their hashes did not
change. The hosted rerun is reported separately for the final pushed SHA.

## Platform classification

- Linux x86-64 under WSL2: native GCC 15.2, Clang 21.1.8, and sanitizer
  compile/link/execution passed.
- Windows x86-64: MinGW-w64 GCC 13 cross-compilation and Wine 10.0 execution
  passed. This is not a native MSYS2 result.
- Native Windows/MSYS2 and native macOS: not available locally; the committed
  CI matrix compiles and runs the standalone targets on both.
- Remote CI: reported separately for the final pushed SHA.

## Verified facts, limitations, and hypotheses

Verified facts:

- The approved patch hash, deterministic tested commit, required tree, and
  unchanged MIT license all reproduce.
- Every copied upstream file matches the reproduced tree byte-for-byte.
- The complete upstream suite passes in its supported default configuration;
  focused tests pass in both callback configurations.
- ZEXDOC and ZEXALL pass through the standalone vaeg runner.
- The emulator still selects the legacy `cpucva/z80c.cpp` production core.
- No frozen reference, private ROM/disk data, state-save production path,
  disassembler, or approved deletion-list file changed.

Known limitations:

- Legacy upstream tests forced wholesale into `Z80_NO_FUNCTIONAL` retain the
  accepted baseline capturing-lambda incompatibility; they were not rewritten.
- Native Windows and macOS execution require hosted or maintainer CI evidence.
- The raw core is not yet exercised through a vaeg wrapper; that is later work.

Hypotheses:

- None is used to pass G36. Upstream acceptance remains unknown and is not an
  approved provenance requirement.

## Gate

The local machine evidence is green. Review the pushed final SHA and hosted
platform jobs, then decide G36. M37 is technically feasible from the vendored
and conformance-tested base but is not authorized and has not been started.
