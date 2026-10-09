# CONVENTIONS — repo invariants and new-code rules (phase 2)

## Invariants (achieved in phase 1; never regress)

| Property  | Rule | Checker |
|-----------|------|---------|
| Encoding  | UTF-8 without BOM throughout the current tree | `tools/repo/check_encoding.py` |
| EOL       | LF throughout the current tree; any future tool-mandated exception requires explicit policy and `.gitattributes` coverage | `tools/repo/check_eol.py` |
| Names     | tracked paths lowercase, except tool/project-mandated names such as top-level `CHANGES*.md` and `external/` (allowlist in `check_case.py`) | `tools/repo/check_case.py` |
| Binaries  | `romimage/`, ROMs, disk images, fonts, icons, wave data are untouchable | review |

Run the checkers before every push. A regression in any invariant is a
defect regardless of what the diff was trying to do.

## New code (phase 2)

- Language: C (C99, no compiler extensions beyond what the tree already
  uses) for emulator-core code. C++17 is allowed under `sdl2/` and, when an
  approved third-party CPU core requires it, for CPU backends and thin
  compatibility adapters under `cpu/`. C++ and STL types must not cross
  existing C-facing subsystem or state-save interfaces. Other newly written
  emulator-core code remains C99 unless an ADR separately approves it.
- Every NEW file starts with this header (adapt comment style):

```
/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
```

  Files derived by porting an existing file (e.g. `memoryva.c` from
  `memoryva.x86`) keep the original header FIRST, then add a
  `ported to C by Nakata Maho, 2026` line. Never remove or reword an
  existing author's copyright.
- No new globals into the core from the frontend. Frontend talks to the
  core through the existing seams (`sysmng`/`taskmng`/`soundmng`-style
  interfaces).
- Canonical active-source formatting is defined by the root `.clang-format`
  and `tools/repo/clang_format_files.txt`. Use the exact command name
  `clang-format-mp-22`; the checker requires clang-format major version 22.
  Do not format files outside the closed manifest. In particular, never apply
  it to vendored, generated, inactive compatibility, assembly, or binary
  sources. Empty lines immediately after a function's opening brace are not
  permitted.
- CMake: explicit source lists, no `file(GLOB)`. Options are prefixed
  `VAEG_` (`VAEG_ENABLE_TESTS`, `VAEG_WERROR` default OFF).
- Third-party code is vendored under `external/<name>/` at a pinned
  release; record name+version+URL+SHA in `docs/agents/DECISIONS/`.

## Commits and PRs

- One concern per commit; rename-only commits separate from fixups.
- Subject: canonical `M<id>: <english imperative>`, LF, UTF-8. An ID is an
  integer or an integer followed by one lowercase letter and an optional
  nonzero decimal suffix, for example `M58:`, `M60a:`, or `M62b1:`.
- Mass mechanical commits get their hash appended to
  `.git-blame-ignore-revs` in the same PR.
- PR description contains: task file name, machine-check output, build
  logs, and an explicit statement whether archived reference-tier behavior
  or provenance is affected.
- Always push and report SHAs; work not on GitHub does not exist.
