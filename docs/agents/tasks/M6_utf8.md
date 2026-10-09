# M6 — CP932 → UTF-8 without BOM

## Goal
All tracked text files are UTF-8 without BOM; the v141 build compiles
them with explicit charset flags. This milestone permanently ends
VS2008 support (VC9 cannot be told a file is BOM-less UTF-8; it would
parse the bytes as CP932).

## Pre-gate decision (user decides BEFORE conversion starts)
Present both options with the M0/M2 evidence and record the choice in
this file:

- **Option A (conservative): `/source-charset:utf-8` only.**
  Execution charset remains CP932. Japanese literals keep their CP932
  bytes in the binary; all ANSI Win32 API behavior (menus, dialogs,
  file paths through the A-APIs) is unchanged. Zero runtime risk;
  right choice while the Win32 build is the reference.
- **Option B (end-state): `/utf-8`** (source AND execution UTF-8),
  plus `activeCodePage=UTF-8` in the application manifest
  (requires Win10 1903+). Correct for the SDL2 future, but changes the
  bytes every A-API sees — needs a full manual sweep of UI text at the
  gate.

Default recommendation: A now, B when the SDL2 backend becomes primary.

Recorded decision for this run: **Option A**. The v141 build uses
`/source-charset:utf-8 /execution-charset:.932`.

## Conversion rules
- Codepage is **CP932** in both directions, never "SHIFT_JIS"
  (wave dash: 0x8160 ↔ U+FF5E must round-trip).
- Round-trip requirement per file:
  `bytes == encode_cp932(decode_cp932(bytes))` before conversion, and
  `original_bytes == encode_cp932(decode_utf8(converted))` after.
  Any file failing round-trip goes to a manual list, untouched.
- No BOM. No other byte changes (EOL was settled in M5).
- Scope: text files per `.gitattributes`; binaries excluded
  automatically.

## .rc handling (part of the pre-gate decision)
rc.exe ignores `/utf-8`. Either:
- convert `.rc` to UTF-8 and put `#pragma code_page(65001)` at the top
  of each, then verify every menu/dialog string at the gate; or
- exempt `.rc` (keep CP932, `#pragma code_page(932)` made explicit) and
  list them in CONVENTIONS.md as a documented exception.

Recorded decision for this run: convert `win9x/np2.rc` to UTF-8 and set
`#pragma code_page(65001)`.

## Commit structure
1. `M6: convert sources CP932 -> UTF-8 (no BOM, mechanical)` — content
   conversion only, plus append hash to `.git-blame-ignore-revs`.
2. `M6: set v141 charset flags per decision (A|B)` — project-file
   change replacing the M2 `/source-charset:.932 /execution-charset:.932`
   pair.
3. If Option B: `M6: add UTF-8 activeCodePage manifest`.

## Machine checks
- `python tools/repo/check_encoding.py --expect utf8` → clean
  (valid UTF-8, zero BOMs).
- Round-trip verification log for every converted file.
- v141 build passes with the new flags; compare warning count against
  `m2_warnings.txt` — new C4566/C4819-class warnings indicate literal
  encoding problems and block the gate.

## GATE G6 (human)
v141 binary only. Standard checklist PLUS explicit attention to every
piece of Japanese UI text (menus, dialogs, window title, statusbar) and
to file-path handling with Japanese file/directory names.
