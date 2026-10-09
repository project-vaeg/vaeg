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
# M31: Command-line boot model selection

Status: implemented; automated checks passed; focused model regression checks passed; G31 persistence checks pending

Date: 2026-07-14

## Scope

Add a startup-only boot-model override to the active SDL2 frontend:

```text
vaeg --model va
vaeg --model va2
```

`va` selects `88VA1` and the unsuffixed VA ROM set. `va2` selects the `88VA2`
compatibility model used for VA2/VA3 and the `*_va2.rom` set. As in the GUI,
changing models selects the destination model's default sound hardware, while
selecting the already configured model preserves its configured hardware.

## Persistence

The command line is a session-only override. Startup first loads `vaeg.cfg`,
saves its model and sound values, then applies the selected model through the
same `np2_select_boot_model()` policy used by the GUI. Before a configuration
save at shutdown, the original model and sound values are restored if the CLI
model still owns the active selection. Other configuration changes made during
the session can still be saved.

The option does not introduce a separate VA3 machine model. `va2` continues to
mean the existing VA2/VA3-compatible `PCMODEL_VA2` path.

## Errors

Missing and unsupported values fail before SDL machine initialization with a
clear message. Values are ASCII case-insensitive, but only `va` and `va2` are
accepted; aliases are intentionally not added.

## Verification

ROM-less selftest covers lowercase and uppercase accepted values plus rejected
`va1`, `va3`, empty, and null values. Required checks are:

```text
cmake --build --preset linux-release
SDL_AUDIODRIVER=dummy ./build/linux-release/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --model va --smoke
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --model va2 --smoke
cmake --build --preset mingw-cross
git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
```

Automated results on the implementation branch:

- Linux release build passed. The pre-existing `kbdmap_selftest` possible-
  uninitialized warnings remain unchanged.
- ROM-less selftest passed, including CLI model-name parsing.
- `--model va --smoke` selected the unsuffixed VA ROM set and passed.
- `--model va2 --smoke` selected the `*_va2.rom` set and passed.
- missing and `va3` model values failed before SDL initialization with the
  documented messages.
- MinGW cross build passed and produced `vaeg.exe`.

## M29 regression found during G31

Initial G31 testing exposed a pre-existing M29 regression rather than a CLI
parser or ROM-selection failure. M28 commit `455c7d5` could enter and use VA2
V3 BASIC, while its direct child M29 commit `c17d64a` froze after entering
BASIC. M29 had applied its VA1 64KB TVRAM aperture correction to the shared
VA2/VA3 memory path.

Commit `c580222` scopes the open-bus aperture to `PCMODEL_VA1` and restores the
full 256KB TVRAM aperture for `PCMODEL_VA2`, matching the model-specific
capacities in NEC's VA/VA2/VA3 product specifications. ROM-less selftest now
covers both mappings. Maintainer verification confirmed:

- `--model va2`: VA2 V3 BASIC command entry works;
- `--model va`: PC-Engine 1.00 boots;
- VA1 V3 BASIC still reproduces the separately tracked inherited command
  failure.

## G31 Gate

1. Run `vaeg --model va --debug`; confirm the unsuffixed ROM set, `88VA1`,
   and VA default sound hardware are reported and VA boots.
2. Run `vaeg --model va2 --debug`; confirm the `*_va2.rom` set, `88VA2`, and
   VA2/VA3 default sound hardware are reported and VA2/VA3 boots.
3. Exit each run and confirm `pc_model` and `SNDboard` in `vaeg.cfg` retain
   their pre-run values.
4. Confirm positional FDD image arguments still mount with either model.

The maintainer confirmed command-line VA and VA2 boot selection and approved
integration on 2026-07-14. G31 passed.
