<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# M18 - Use model-specific PC-88VA ROM names

Status: complete

Branch: `topic/m18-rom-layout`

Gate: G18 passed

Depends on: G17 passed.

## Goal

Remove the active SDL2 runtime dependency on `romimage/`. Resolve ROMs
beside the executable using unsuffixed names for the original VA and MAME's
`pc88va2` `*_va2.rom` names for VA2/VA3:

```text
vaeg distribution root/
  vaeg[.exe]
  vadic.rom
  vafont.rom
  varom00.rom
  varom08.rom
  varom1.rom
  vadic_va2.rom
  vafont_va2.rom
  varom00_va2.rom
  varom08_va2.rom
  varom1_va2.rom
  vasubsys.rom
```

The GUI boot-model selector keeps the core model and selected ROM filename
set synchronized. VA2 must not fall back to unsuffixed VA names.

## Model mapping

| GUI label | Core configuration | Model ROM names |
|---|---|---|
| VA | `pc_model=88VA1` | `vadic.rom`, `vafont.rom`, `varom00.rom`, `varom08.rom`, `varom1.rom` |
| VA2/VA3 | `pc_model=88VA2` | `vadic_va2.rom`, `vafont_va2.rom`, `varom00_va2.rom`, `varom08_va2.rom`, `varom1_va2.rom` |

VA3 remains represented by the existing `PCMODEL_VA2` behavior.
The VA2 names follow MAME's `ROM_START(pc88va2)` declaration in
[`src/mame/nec/pc88va.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp).
`vasubsys.rom` is a separate vaeg extra: vaeg executes the Z80 FDD
subsystem, while MAME currently leaves that ROM declaration unconnected.

## MAME checksum reference

These values come from MAME's `pc88va` and `pc88va2` ROM declarations. MAME
marks both font dumps `BAD_DUMP`; vaeg still compares against the documented
bytes and reports differences as warnings.

| Set | File | Size | CRC32 | SHA-1 |
|---|---|---:|---|---|
| VA | `vafont.rom` | 0x50000 | `faf7c466` | `196b3d5b7407cb4f286ffe5c1e34ebb1f6905a8c` |
| VA | `vadic.rom` | 0x80000 | `f913c605` | `5ba1f3578d0aaacdaf7194a80e6d520c81ae55fb` |
| VA | `varom00.rom` | 0x80000 | `8a853b00` | `1266ba969959ff25433ecc900a2caced26ef1a9e` |
| VA | `varom08.rom` | 0x20000 | `154803cc` | `7e6591cd465cbb35d6d3446c5a83b46d30fafe95` |
| VA | `varom1.rom` | 0x20000 | `0783b16a` | `54536dc03238b4668c8bb76337efade001ec7826` |
| VA2 | `vafont_va2.rom` | 0x50000 | `b40d34e4` | `a0227d1fbc2da5db4b46d8d2c7e7a9ac2d91379f` |
| VA2 | `vadic_va2.rom` | 0x80000 | `a6108f4d` | `3665db538598abb45d9dfe636423e6728a812b12` |
| VA2 | `varom00_va2.rom` | 0x80000 | `98c9959a` | `bcaea28c58816602ca1e8290f534360f1ca03fe8` |
| VA2 | `varom08_va2.rom` | 0x20000 | `eef6d4a0` | `47e5f89f8b0ce18ff8d5d7b7aef8ca0a2a8e3345` |
| VA2 | `varom1_va2.rom` | 0x20000 | `7e767f00` | `dd4f4521bfbb068f15ab3bcdb8d47c7d82b9d1d4` |
| extra | `vasubsys.rom` | 0x2000 | `08962850` | `a9375aa480f85e1422a0e1385acb0ea170c5c2e0` |

## Constraints

- Do not modify `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, or `hlp/`.
- Do not add or modify ROM payloads, disk images, fonts, or WAV data.
- Do not add empty-directory placeholders to distribution artifacts.
- Keep the change limited to active ROM path resolution, GUI model
  selection, tests, and distribution documentation.
- Preserve the existing case-insensitive final-component lookup for
  `.rom` and `.wav` assets.

## Required work

1. Audit active `romimage`, `biospath`, BIOS loader, asset helper,
   smoke, packaging, and documentation references.
2. Resolve the selected filename set in this order:
   - executable directory;
   - development fallback at cwd.
3. If neither candidate is complete, keep the executable directory as the
   expected root and report the model and first missing required ROM.
4. Remove `biospath` from the active SDL2 INI table. The shared core
   field remains the loader entry point, but SDL2 derives it from the
   selected model rather than trusting a persisted absolute path.
5. Add `Emulate -> Boot model -> VA / VA2/VA3`. Selection updates
   `np2cfg.model`, resolves the corresponding ROM filename set, persists
   the model, and invokes the existing reset flow that preserves FDD and
   SASI selections.
6. Update README and build/frontend documentation. State that ROMs are
   not included and must be extracted from hardware the user owns. Cite the
   MAME source for the VA2 filename contract.
7. Compare file size, CRC32, and SHA-1 with MAME's `pc88va`/`pc88va2`
   declarations at startup. Include `vasubsys.rom` using MAME's disabled FDD
   subsystem declaration. A mismatch is warning-only and reports expected
   and actual values.

## Verification

```sh
rg -n "romimage|ROMIMAGE|_va2\\.rom" . \
  --glob '!win9x/**' --glob '!i286x/**' \
  --glob '!cpuxva/memoryva.x86' --glob '!hlp/**' \
  --glob '!docs/agents/reports/raw/**'

cmake --build --preset macos-release
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/macos-release/sdl2/vaeg --smoke
SDL_AUDIODRIVER=dummy ./build/macos-release/sdl2/vaeg --selftest

git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
```

## Gate G18

G18 is a human model/ROM gate:

- install the unsuffixed VA ROMs beside the executable;
- install the five MAME-compatible VA2 `*_va2.rom` files beside the
  executable;
- install the extra `vasubsys.rom` beside the executable;
- select VA and confirm the GUI checks `VA`, resets, and boots from the
  VA ROM set;
- select VA2/VA3 and confirm the GUI checks `VA2/VA3`, resets, and boots
  from the VA2 ROM set;
- confirm FDD and SASI selections survive both model changes;
- restart and confirm the selected `pc_model` persists and resolves the
  matching executable-relative filename set;
- temporarily remove a model ROM and confirm the error identifies the
  selected model, expected root, and missing ROM;
- confirm VA2 does not fall back to unsuffixed VA ROMs;
- deliberately change a copy of one ROM and confirm size/CRC32/SHA-1 warning
  output without aborting startup;
- confirm a stale `biospath=romimage` setting cannot select the old flat
  layout.

## Completion record

The active SDL2 frontend resolves model-specific ROM names beside the
executable, with cwd as the development fallback. VA uses unsuffixed ROM
names; VA2/VA3 uses MAME-compatible `*_va2.rom` names without falling back
to the VA set. Startup checks size, CRC32, and SHA-1 and reports mismatches
as warnings. `vasubsys.rom` remains a documented vaeg extra.

G18 passed after the maintainer verified VA and VA2/VA3 model selection,
boot and reset behavior, missing-ROM and checksum warnings, and persistence
of configured FDD and SASI media across reset and restart.
