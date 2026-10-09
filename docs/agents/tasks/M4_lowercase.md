# M4 — Lowercase all tracked paths

## Goal
Every tracked path (files AND directories) is lowercase ASCII, and
every textual reference to a path matches the on-disk case exactly.
This is what makes the tree portable to case-sensitive filesystems
(the SDL2 target).

## Windows pitfalls — read before touching anything
- NTFS is case-insensitive: `git mv FOO.C foo.c` may be a no-op or
  fail. Always two-step: `git mv FOO.C __tmp__ && git mv __tmp__ foo.c`
  (directories likewise).
- Verify `git config core.ignorecase` is true on Windows and do NOT
  toggle it; rely on the two-step move, and confirm the rename landed
  with `git ls-files`.
- Case-only renames of directories must also be two-step.

## Commit structure (strict)
1. **Commit A — renames only.** `git mv` operations, zero content
   changes. `git show --stat` must show 100% renames.
   `M4: lowercase all tracked paths (rename-only)`
2. **Commit B — reference fixups.** Content changes only, no renames:
   - `#include "..."` (C/C++), `%include` (NASM), `#include` in `.rc`,
     `.tbl` inclusion sites;
   - source/header path entries in `.vcproj`, `.vcxproj`,
     `.vcxproj.filters`, `.sln`, `sdl/` CMake files;
   - any path strings found by `git grep` for the old names.
   `M4: fix path references after lowercase rename`

## Machine checks
- `python tools/repo/check_case.py` → clean (no uppercase outside
  ALLOW, no case-fold collisions, no include-case mismatches).
- Both builds pass. Additionally do a checkout + build on a
  case-sensitive filesystem if available (WSL / Linux, at least a
  compile smoke of the portable subset) — record result.

## GATE G4 (human)
Standard checklist on both binaries.
