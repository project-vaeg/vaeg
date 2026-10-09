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

# M92 - Canonical clang-format normalization

Status: **G92 human gate passed; M92 closed at `caa1f403cd0c1f6ce7673d6f839de7d3932c5316`**

Predecessor: G91 passed and M91 merged to `main` at
`a7aaeba81b3828927019b9567c3c8d6ae087a708`.

Branch: topic/m92-clang-format

Commit prefix: M92:

Report: docs/agents/reports/m92_clang_format.md

## Goal

Establish one reproducible clang-format policy for the active first-party C and
C++ implementation, then mechanically normalize exactly that declared source
set without changing emulator behavior.

## Tool and style authority

- The canonical command name is `clang-format-mp-22`.
- The checker requires clang-format major version 22. The M92 candidate was
  prepared with MacPorts clang-format 22.1.8.
- The repository root `.clang-format` is the style authority.
- Empty lines immediately after a function's opening brace are forbidden. The
  brace is followed immediately by the first declaration or statement.
- Include ordering is preserved and comments are not reflowed, keeping this
  milestone mechanical and limiting avoidable source-history damage.

## Formatting scope

`tools/repo/clang_format_files.txt` is the closed formatting manifest. It is
the union of tracked first-party C/C++ sources and headers reached by the
macOS MacPorts and MinGW test-enabled CMake/Ninja dependency graphs, plus the
conditionally compiled `cpu/upd9002/upd9002_perf.c` diagnostic translation
unit. Three byte-immutable uPD9002 evidence artifacts are then removed from
that union, leaving 381 paths at M92 start.

The manifest excludes:

- vendored `external/` code;
- build outputs and generated sources;
- NASM, resources, media, and documentation;
- retained inactive PC-98 compatibility sources outside the active CMake
  graph;
- unrelated guest demo sources; and
- `cpu/upd9002/upd9002_ea.c`, `upd9002_state.c`, and `upd9002_state.h`, whose
  exact approved hashes remain protected by `upd9002_protected_deletion.py`.

A future source becoming active must be deliberately added to the manifest.
Formatting scope must not silently expand through filesystem globbing.

## Work

1. Add the pinned style, closed manifest, and a check/apply helper.
2. Format active core C/C++ sources in a mechanical commit.
3. Format active SDL2 and test C/C++ sources in a separate mechanical commit.
4. Add both mechanical commit IDs to `.git-blame-ignore-revs`.
5. Run formatting, repository, build, test, and emulator selftest checks.
6. Record the exact scope and results in the M92 report.

## Non-goals

- Do not change behavior, APIs, comments, symbols, or diagnostics while
  formatting.
- Do not format vendored, generated, inactive, assembly, or binary files.
- Do not fix pre-existing compiler warnings as part of the mechanical pass.
- Do not change compiler or language standards.

## Automated validation

- `python3 tools/repo/clang_format.py`
- `python3 tools/repo/check_encoding.py --expect utf8`
- `python3 tools/repo/check_eol.py --enforce`
- `python3 tools/repo/check_case.py`
- macOS MacPorts configure, build, CTest, and ROM-less selftest
- diff scope checks proving the mechanical commits touch only manifest paths,
  plus compiler and test validation of the mechanical normalization.

## Human gate G92

From a clean checkout of the final candidate:

1. build with the normal maintainer configuration;
2. boot in native V3 mode;
3. run the bundled VA demo;
4. boot an OS and perform simple keyboard, disk, display, sound, and state-save
   operations; and
5. confirm normal behavior is unchanged by the formatting-only milestone.

M93 must not begin until the maintainer explicitly reports that G92 passed.
