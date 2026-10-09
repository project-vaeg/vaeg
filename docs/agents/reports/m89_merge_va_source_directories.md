<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M89: merge VA source directories

## Status

M89 source candidate
[665877a](https://github.com/nakatamaho/vaeg/commit/665877ab7e0961907a255796b30e7438115c6e51)
completed the source-layout consolidation on `topic/m89-merge-va-source-directories`.
The maintainer passed G89 human validation against this exact candidate.
The approved topic history was fast-forwarded to `main` at
[5b4a22b](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97).
The predecessor M88 was merged to `main` at
[b142bc3](https://github.com/nakatamaho/vaeg/commit/b142bc37c4fe0cc50381727eac5766a5b3843e71).

This is a source-layout consolidation. No guest-visible behavior, public
symbol, save-state section name, or binary payload was intentionally changed.

## Directory ownership

| Former directory | Consolidated directory | Scope |
|---|---|---|
| `biosva/` | `bios/` | VA BIOS implementation, retaining `biosva.c` and `biosva.h` |
| `vramva/` | `vram/` | VA text, sprite, graphics, palette, and drawing implementation, retaining the `va` suffixes |
| `cpucva/` | `cpu/` | uPD9002 main-CPU adapter and shared uPD780/uPD70008-compatible backend |

The `cpu/` destination is an ownership boundary only. The moved compatibility
files remain separate from `cpu/upd9002/` and `cpu/upd780/`; no instruction
implementation or FDC-CPU ownership was merged. The public symbols and the
stable `UPD9Z80` save-state section remain unchanged.

## Commit chain

1. [9fc4436](https://github.com/nakatamaho/vaeg/commit/9fc44364f70af7107ee6471a13b9d096502c9e2f)
   defines the M89 task and ROADMAP entry.
2. [71a4a3b](https://github.com/nakatamaho/vaeg/commit/71a4a3bd37ab9abc2b32aa8713dbdb51cf15c10c)
   performs the 22-path rename-only consolidation; the rename commit has no
   content changes.
3. [f93bb9a](https://github.com/nakatamaho/vaeg/commit/f93bb9a95b1236f977c671bd994e8de2a6e67ff5)
   updates CMake, include paths, tests, current operational documentation,
   and the source-layout validator.

Historical reports and evidence retain old paths where those paths describe
their recorded checkpoint. Current build files, active source, tests, and
current operational documentation use the consolidated paths.

## Validation

| Check | Result |
|---|---|
| `tools/repo/check_encoding.py --expect utf8` | PASS; 0 violations |
| `tools/repo/check_eol.py --enforce` | PASS; 0 violations |
| `tools/repo/check_case.py` | PASS; 0 findings |
| `git diff --check` | PASS |
| `tools/qa/upd9002_rename.py --root .` | PASS; retired active paths absent; approved historical exceptions 0 |
| Linux Debug configure/build | PASS; 188/188 build steps |
| Linux CI Clang configure/build | PASS; 237/237 build steps |
| Linux CI Clang CTest | PASS; 83/83 tests, one external corpus test skipped |
| MinGW cross configure/build | PASS; 517/517 build steps; PE32+ x86-64 |
| MinGW artifact | `build/mingw-cross/sdl2/vaeg.exe`; SHA-256 `c6e09122e5aa64b183c49a01988ffaea4a1fe27193bc19f74b8c4c0e27243c8a` |
| Binary payload audit | PASS; no ROM, disk, font, icon, wave, or other binary payload was changed |
| Current active-path audit | PASS; no retired `biosva/`, `vramva/`, or `cpucva/` path remains in active source, CMake, tests, or current operational documentation |
| Hosted GitHub Actions | PASS; run [31577266904](https://github.com/nakatamaho/vaeg/actions/runs/31577266904), all 9 jobs successful against `665877ab7e0961907a255796b30e7438115c6e51` |

The repository and Git helper checks were run with
`GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null`, which is required
in the restricted checkout environment. Historical evidence is intentionally
not mass-rewritten by this layout move.

Hosted GitHub Actions run
[31577266904](https://github.com/nakatamaho/vaeg/actions/runs/31577266904)
completed successfully: repository invariants, Ubuntu GCC/Clang/ASAN, macOS,
both Windows MinGW jobs, standalone conformance, and the uPD9002 architectural
SST ratchet all passed against the human-gate candidate.

## G89 human gate

The maintainer reported that the clean-checkout V3 boot, bundled VA demo, OS
boot, simple FDD/SASI/SCSI/keyboard/display/state-save operations, Screen font
loading, MPU98II path, and normal VA/VA2 operation all passed against
candidate `665877ab7e0961907a255796b30e7438115c6e51`.

G89 passed on 2026-08-12. M89 was fast-forwarded to `main` at
[5b4a22b](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97)
and is closed.
