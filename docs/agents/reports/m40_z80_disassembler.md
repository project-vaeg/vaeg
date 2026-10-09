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
# M40 Z80 disassembler evidence

## Starting state and scope

The required pre-read completed in the prescribed order. Before branch
creation the exact commands reported:

```text
$ git status --short
?? "PC-Engine 1.05.d88"
?? docs/modernization/PCEPAT.DOC
?? docs/tekumani/
?? his.txt
$ git rev-parse HEAD
88cda7fa17ae0ba8de42c68f137336a6c686ddd6
$ git branch --show-current
topic/m39-z80-integration
```

Work then moved to `topic/m40-z80-disassembler`. The four untracked paths were
not opened, hashed, copied, modified, staged, or used. M40 did not change CPU
execution, clocks, IRQ, WAIT, state layout, FDD behavior, SLEEP_HACK constants,
the default core, vendored source, the approved M35 patch, or the frozen tier.
It did not delete any of the seven approved legacy files or begin M41.

## Implementation and license

The selected strategy is an independently authored vaeg decoder based on the
documented Z80 `x/y/z/p/q` encoding and prefix rules. The new
`cpucva/upd780_disasm.h` and `cpucva/upd780_disasm.cpp` are BSD-2-Clause vaeg code,
Copyright (c) 2026 Nakata Maho. No M88/cisc implementation or opcode table,
GPL opcode table, or other disassembler implementation/table was copied or
adapted.

The approved MIT suzukiplan revision was inspected only for an available
public side-effect-free decoder API. It has execution-time debug strings but
no reusable API, so M40 took no disassembler code from it and changed no
vendored byte. The old `z80diag` source was inspected for consumer/build
inventory, not as the implementation or golden-output source.

The fixed-width public API is:

```text
VaegZ80Disassemble(pc, destination, capacity, reader, opaque)
    -> { next_pc, length, status }
```

It reads memory deterministically through a function pointer, wraps 16-bit
addresses, never executes or mutates a CPU, and bounds every output write. STL
is implementation-private. `subsystem_disassemble_bounded()` adapts the live
subsystem memory seam. The historical `subsystem_disassemble()` signature is
retained as the known-consumer 64-byte compatibility adapter.

Canonical text uses lower-case mnemonics/registers, fixed-width `0x` hex,
comma-space separation, absolute wrapped relative targets, and signed IX/IY
displacements. It recognizes SLL, IXH/IXL/IYH/IYL, and indexed-CB register
results. Reserved ED bytes use deterministic `db` output with the actual
length. Redundant DD/FD prefixes count toward reads/length; the last prefix
selects IX/IY. A stream with no finite boundary is rejected after 32 prefixes
with explicit prefix-limit status, length zero, unchanged next PC, and
`<invalid-prefix-sequence>`.

## Corpus and review

`tests/upd780/disasm.cpp` implements a test-side encoding classifier independent
of the decoder's returned length. The 3,844 exhaustive cases are:

- all 256 base bytes;
- all 256 CB bytes;
- all 256 ED bytes;
- all 256 DD-following bytes;
- all 256 FD-following bytes;
- all 256 DDCB final bytes at displacements `80 ff 00 01 7f`;
- all 256 FDCB final bytes at the same displacements;
- four repeated-prefix/address-wrapping cases.

Every case checks exact length, next PC, deterministic non-empty text, ordered
read addresses/count, wrap, output canaries, and a CPU-state sentinel. The 31
golden rows were generated from the new decoder and manually reviewed against
documented instruction encodings, not the legacy decoder. No installed
permissively licensed side-effect-free Z80 decoder was available for another
machine cross-check. Buffer tests cover capacity zero, one, exact, truncated,
4,096 bytes, null reader, and an unbounded prefix image.

## Consumer migration and source selection

The production seam in `iova/subsystem.cpp` now includes `z80_disasm.h` and
uses the new decoder for both core choices. The new wrapper's inert `GetDiag()`
placeholder and the M39 `z80diag_bridge.*` compatibility bridge are removed.
No active portable source parses disassembly strings, and the active SDL2
frontend has no interactive Z80 debugger. The frozen Win9x debugger caller
remains untouched.

The exact production source proof was:

```text
legacy:
cpucva/upd780_disasm.cpp
cpucva/z80c.cpp
cpucva/z80diag.cpp

suzukiplan:
cpucva/z80_compat_cpu.cpp
cpucva/upd780_disasm.cpp
cpucva/z80_compat_state.cpp
```

The legacy diagnostic source remains only because the still-selectable legacy
CPU owns it for internal dump support. A source search found no include of
`types.h`, `z80.h`, `z80if.h`, or `z80diag.h` outside the seven legacy files
after excluding frozen/vendor/build paths. All seven remain present:

```text
cpucva/types.h
cpucva/z80.h
cpucva/z80if.h
cpucva/z80c.h
cpucva/z80c.cpp
cpucva/z80diag.h
cpucva/z80diag.cpp
```

An unqualified configure printed `Production Z80 core: legacy`. M40 therefore
does not change the default or remove either production choice.

## Local validation

The host tools were GCC 15.2.0, Clang 21.1.8, MinGW GCC 13-win32, CMake 4.2.3,
and Wine 10.0.

### GCC and Clang

The exact GCC commands were:

```sh
cmake -S . -B build/m40-gcc-legacy -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=legacy
cmake --build build/m40-gcc-legacy --parallel 2
ctest --test-dir build/m40-gcc-legacy --output-on-failure
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m40-gcc-legacy/sdl2/vaeg --smoke

cmake -S . -B build/m40-gcc-suzukiplan -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_CORE=suzukiplan
cmake --build build/m40-gcc-suzukiplan --parallel 2
ctest --test-dir build/m40-gcc-suzukiplan --output-on-failure
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m40-gcc-suzukiplan/sdl2/vaeg --smoke
```

Both builds and smokes passed; each CTest run passed 15/15. The disassembler
reported 3,844 exhaustive cases, 31 reviewed golden rows, all buffer/malformed
cases, and overall pass.

The exact Clang configuration substituted:

```sh
CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -S . \
  -B build/m40-clang-legacy -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_CORE=legacy
cmake --build build/m40-clang-legacy --parallel 2
ctest --test-dir build/m40-clang-legacy --output-on-failure

CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake -S . \
  -B build/m40-clang-suzukiplan -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_CORE=suzukiplan
cmake --build build/m40-clang-suzukiplan --parallel 2
ctest --test-dir build/m40-clang-suzukiplan --output-on-failure
```

Both passed 15/15. Existing warnings in unrelated legacy C sources were
retained. With `VAEG_WERROR=ON`, `vaeg_z80_disasm` itself built and passed;
the full repository target remains blocked by the pre-existing
`i286c/i286c_0f.c` unused-but-set variable warnings and was not reformatted or
changed in M40.

### ASan/UBSan

Both core choices used Clang Debug builds with:

```sh
-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer
-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer
-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
```

The build directories were `build/m40-asan-legacy` and
`build/m40-asan-suzukiplan`; CTest ran with
`ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1`. Both passed
15/15, including exhaustive, golden, and buffer tests, with no sanitizer
finding.

### MinGW and Wine

Both selections were configured from `mingw-cross` with
`VAEG_ENABLE_TESTS=ON`; the existing verified FetchContent source directories
under `build/mingw-cross/_deps` supplied SDL2, zlib, xz, and LibArchive after
the sandbox rejected DNS access. The exact core build directories were
`build/m40-mingw-legacy` and `build/m40-mingw-suzukiplan`. Both completed
250/250 test-enabled targets after the initial 776/776 production builds.

Wine used:

```sh
env WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m39-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/m40-mingw-<core>/vaeg_z80_disasm.exe
env WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m39-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/m40-mingw-<core>/sdl2/vaeg.exe --selftest
env WINEDEBUG=-all WINEPREFIX=/home/maho/vaeg/build/m39-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/m40-mingw-<core>/sdl2/vaeg.exe --smoke
```

The six executions passed. Both disassembler binaries reported all 3,844/31
and buffer cases; both production binaries passed subsystem integration and
ROM-less smoke. Wine is execution evidence, not native hosted Windows.

### ZEX

The acquisition command was:

```sh
python3 tests/z80_compat/fetch_zex.py --output-dir /tmp/vaeg-m40-zex
```

It verified all five approved SHA-256 values. The exact test commands were:

```sh
cmake -S . -B build/m40-zex -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_CORE=suzukiplan \
  -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m40-zex
cmake --build build/m40-zex --target \
  vaeg_z80_zex_runner vaeg_z80_wrapper_zex_runner --parallel 2
ctest --test-dir build/m40-zex --output-on-failure \
  -R '^vaeg_z80_(zexdoc|zexall|wrapper_zexdoc|wrapper_zexall)$' -j4
```

Raw ZEXDOC/ZEXALL and wrapper ZEXDOC/ZEXALL passed 4/4. No fetched artifact is
under the repository or in a release input.

### Private spot check

Stable ID `m40-private-disasm-a` used the maintainer-authorized VA2/VA3
ROM/media set and expendable media copies under each core. Both current M40
production binaries reached the same OS date-entry prompt. The initial
non-interactive `--smoke` detector timed out at its uniform-screen threshold
under both cores because that boot requires normal interaction; it was not
treated as a core divergence. The headless interactive runs then reached the
same prompt.

At a live subsystem execution boundary, GDB invoked
`subsystem_disassemble_bounded()` for 16 consecutive instructions under each
core. Every next PC advanced, text was readable, navigation remained aligned,
and live/public PCs were unchanged after the reads. Raw disassembly, addresses,
private filenames/paths/hashes, ROM bytes, screenshots, traces, and working
media remain only outside Git. There is no active SDL2 Z80 debugger UI, so the
live production seam was the available debugger-equivalent interaction.

## Repository, archive, and hosted validation

The repository checks after staging all new and changed documentation were:

```sh
python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
# 0 violation(s)
python3 tools/repo/check_eol.py --enforce
# 0 violation(s)
python3 tools/repo/check_case.py
# 0 finding(s)
python3 tools/repo/find_unreferenced.py --report
# roots: 9, sources: 702, reached: 633, unreferenced: 69
git diff --check
# passed with no output
git diff --cached --check
# passed with no output
```

The 69 unreferenced paths are the established repository baseline; neither
new decoder nor its tests appear in that list. The archive commands and
results on commit `5286c5cccb69c863ebbe728e584290b6ce715b38` were:

```sh
git archive --format=tar.gz \
  --output=/tmp/vaeg-m40-source-5286c5c.tar.gz HEAD
tar -czf /tmp/vaeg-m40-runtime-5286c5c.tar.gz \
  build/m40-gcc-legacy/sdl2/vaeg assets/OFL.txt assets/NOTICE.md \
  CHANGES.20260713.md dist/readme-dist.txt
python3 tests/z80_compat/check_zex_archive.py \
  /tmp/vaeg-m40-source-5286c5c.tar.gz \
  /tmp/vaeg-m40-runtime-5286c5c.tar.gz
```

The source archive passed with 998 files and the runtime archive passed with
five files; neither contained a ZEX artifact. The vendor/M35 immutability
checks passed:

```text
M35 patch SHA-256:
d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4
vendored LICENSE.txt SHA-256:
ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1
```

`git diff --exit-code -- external/suzukiplan-z80` and the same check for the
approved patch produced no difference.

Hosted run
[29411805128](https://github.com/nakatamaho/vaeg/actions/runs/29411805128)
tested commit `c469c233da7de1640ff499ea5347353f63a8aa96` from
2026-07-15 11:29:28 UTC through 11:38:06 UTC. It completed successfully with
12/12 jobs:

```text
repo invariants: success
ubuntu gcc legacy z80: success
ubuntu gcc suzukiplan z80: success
ubuntu clang legacy z80: success
ubuntu clang suzukiplan z80: success
ubuntu asan legacy z80: success
ubuntu asan suzukiplan z80: success
standalone z80 conformance: success
windows msys2 mingw64 legacy z80: success
windows msys2 mingw64 suzukiplan z80: success
macos fetch-sdl2 legacy z80: success
macos fetch-sdl2 suzukiplan z80: success
```

The Windows jobs are native hosted Windows evidence and the macOS jobs are
native hosted macOS evidence; the local MinGW/Wine result is reported
separately and is not mislabeled as native Windows.

## Risks and gate disposition

### Verified facts

- Both production choices use the new disassembler and retain their existing
  CPU implementations.
- Exact instruction boundaries, deterministic reads, bounded output, and
  state non-mutation pass the exhaustive/golden/sanitizer matrix.
- No active consumer parses legacy-formatted text.
- The default remains legacy, vendor/patch bytes are unchanged, and all seven
  approved files remain.
- Private boot and live-seam checks converged under both core selections.

### Accepted formatting changes

Legacy character-for-character formatting is not retained. Lower-case text,
`0x` hexadecimal, comma-space, canonical undocumented names, and deterministic
`db` for reserved ED encodings are accepted because no parser dependency
exists. Instruction boundaries and operands, not typography, are the contract.

### Known limitations and M41 dependencies

- Local MinGW plus Wine is not native Windows; hosted Windows is recorded
  separately.
- Linux cannot execute native macOS; hosted macOS is recorded separately.
- The active SDL2 frontend has no interactive Z80 debugger; private evidence
  used the live production C seam under GDB.
- The selectable legacy CPU still owns the old diagnostic implementation for
  internal dumps. M41 must remove the legacy selection, prove no remaining
  references, delete exactly the approved seven files, and audit archives.

### Unresolved issues and hypotheses

No unresolved disassembler, consumer, buffer, boot, or CPU-behavior finding
remains locally. No hypothesis is used as a pass condition. M41 has not
started.

G40 PASSED
