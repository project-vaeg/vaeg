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
# M32: Command-line startup overrides

Status: complete; G32 passed

Date: 2026-07-14

## Scope

Extend the active SDL2 command line from M31's boot-model selector to the
existing model, sound, media, execution, display, and input settings:

| Area | Options |
|------|---------|
| Machine | `--model va|va2` |
| Sound | `--fmbackend np2|ymfm`, `--fmsound opn|opna`, `--ymfm-fidelity minimum|medium|maximum`, `--samplerate 11025|22050|44100`, `--soundbuffer 40..1000`, `--mute` |
| Media | `--fdd1 path|none`, `--fdd2 path|none`, `--sasi1 path|none`, `--sasi2 path|none` |
| Execution | `--cpumult 1..32`, `--sgp model|follow-cpu|1..16`, `--nowait`, `--frameskip auto|full|2|3|4` |
| Display/input | `--fullscreen`, `--windowed`, `--effect unfiltered|linear|scanline|crt-lite`, `--scaling native|fit|fit-8dot|integer|stretch`, `--controller joystick|mouse`, `--keyboard-layout jis|us|custom` |
| Information | `--version`, `--help`, `-h` |

Existing diagnostic options `--smoke`, `--selftest`, `--debug`, `--fdctrace`,
and `--pacelog` remain available. Enum values are ASCII case-insensitive.
Repeated options use the last value, matching the M31 behavior.

Positional FDD arguments are removed. `vaeg disk1.d88 disk2.d88` now fails
with a message directing the caller to `--fdd1` and `--fdd2`. Named media
options accept `none` to make that drive empty for the session.

Configuration-file selection, ROM-directory overrides, SCSI/IDE media,
FDD3/FDD4, and startup save-state loading remain outside this milestone.

## Application order and compatibility

Startup parses the complete command line before SDL initialization, loads
`vaeg.cfg`, and validates the effective combination before changing machine
state. Model selection is applied first through the same canonical transition
used by the GUI. Remaining overrides are then applied in sound, execution,
display/input, and media order.

The sound-hardware override must be valid for the effective model. In
particular, `--model va2 --fmsound opn` is an explicit startup error because
VA2/VA3 requires OPNA. The same check also applies when `--model` is omitted
and the saved model supplies the effective model.

FDD paths must name existing regular files. SASI paths receive the same basic
check and are additionally opened through the SxSI image parser. Startup
accepts a SASI path only when the recognized image geometry is usable through
the SASI interface and the declared sector data is present; an existing but
malformed, truncated, or non-SASI image is rejected before machine
initialization.

## Persistence

All new setting and media options are session-only startup overrides. Before
applying them, the frontend snapshots only the settings named on the command
line. Before normal configuration save, it restores each snapshot if the
active value still equals the CLI-applied value. A GUI change made during the
session therefore releases CLI ownership and can be saved normally.

Model and sound hardware are restored as one compatibility unit. FDD media
saved before a CLI override remains a protected reference during the session,
so managed archive extraction is not pruned merely because `--fdd1`,
`--fdd2`, or `none` temporarily replaces it.

## Verification

ROM-less selftest covers parsing, invalid values and removed positional
arguments, complete apply/restore ownership, and positive/negative SASI image
classification. Required checks are:

```text
cmake --build --preset macos-macports --target vaeg_sdl2
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/macos-macports/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/macos-macports/sdl2/vaeg --model va [representative overrides] \
  --debug --smoke
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/macos-macports/sdl2/vaeg --model va2 [representative overrides] \
  --debug --smoke
cmake --preset mingw-cross
cmake --build --preset mingw-cross
git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
```

Automated results on the implementation branch:

- The macOS MacPorts target configured and built successfully.
- The ROM-less selftest passed, including CLI parsing and ownership restore,
  valid SASI-image acceptance, and truncated SASI-image rejection.
- Representative VA/NP2 and VA2/YMFM smoke runs exited successfully and their
  debug output reported the requested startup settings.
- `--model va2 --fmsound opn` and a positional FDD argument both exited with
  status 1 and the expected explicit errors.
- A fresh MinGW cross configure and the `mingw-cross` preset build completed;
  `build/mingw-cross/sdl2/vaeg.exe` is a PE32+ GUI x86-64 executable.
- The encoding, EOL, and case checks passed with zero findings;
  `git diff --check` passed and the frozen reference tier remained unchanged.

### G32 archive-browser follow-up

G32 preparation exposed an archive-media browser defect inherited from M22.
After a ZIP, 7z, or LZH drag-and-drop mount, FDD Open used the live extracted
image path and therefore opened the managed user-state directory. The archive
loader now records the source archive's parent directory for each mounted
image. FDD Open uses that source directory only while the corresponding
extracted image remains mounted. Per-image metadata restores the association
after application restart and is removed by the existing managed-storage
pruning path.

The dropmedia selftest covers ZIP and 7z source-directory capture, metadata
reload, correct drive/path association, and rejection of an unrelated mounted
path. The macOS MacPorts target and complete ROM-less selftest pass with the
follow-up applied.

Two apparent macOS arm64 failures were invalid tests of a stale executable:
the rebuilt binary had been copied to `/Users/maho/vaeg_new/vaeg`, while the
application under test was `/Users/maho/88VA/vaeg_new/vaeg`. After the
`ce26003` build was copied to the actual test location, the maintainer
confirmed that FDD Open started in the source ZIP directory and declared the
defect fixed. The archive-browser item of G32 therefore passed before the full
gate was completed.

## G32 Gate

1. From a clean checkout, boot with representative VA and VA2 combinations
   covering both FM backends, OPNA, CPU/SGP speed, display, and input options.
2. Confirm `--model va2 --fmsound opn` fails before video, audio, and machine
   initialization with a clear incompatibility error.
3. Mount direct FDD images with `--fdd1` and `--fdd2`; confirm positional FDD
   image arguments are rejected.
   Also drag and drop a ZIP or 7z archive, then confirm FDD Open starts in the
   source archive directory rather than managed extraction storage.
4. Mount valid SASI images with `--sasi1` and `--sasi2`; confirm an existing
   but invalid or non-SASI image is rejected before machine initialization.
5. Confirm `none` temporarily empties each requested media slot.
6. Exit each run and confirm CLI-owned settings and media did not replace the
   pre-run values in `vaeg.cfg`; also confirm a GUI change made during a CLI
   run can still persist.
7. Complete the standard clean-checkout V3/demo/OS gate.

The maintainer tested the deployed MinGW build and reported G32 passed on
2026-07-14. This accepts the command-line startup overrides, validation and
session-only persistence behavior, media handling, and standard guest
regression gate.
