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
# M28: Sound output settings

## Goal

Expose output sampling rate, requested SDL audio buffer length, and ymfm OPN
fidelity in the active SDL2 Sound menu. Preserve the existing NP2 and ymfm
backend responsibilities and rebuild sound through the established
media-preserving guest reset path.

## Audit

The active configuration already stores the common output controls as:

- `SampleHz` in `np2cfg.samplingrate`;
- `Latencys` in `np2cfg.delayms`.

`pccore.c:sound_init()` passes the selected rate to every generator and the
SDL sound manager. This includes FM, PSG, ADPCM, rhythm, PCM, beep, and FDD
motor sound, so sampling rate and buffer length are not backend-specific.
The NP2 FM generator supports 11025, 22050, and 44100 Hz. The ymfm bridge
generates at its selected native fidelity and box-downsamples to the same
configured output rate.

The frozen Win9x Configure dialog accepted the same three rates and a 40 to
1000 ms buffer request. Changes set `soundrenewal` and rebuilt sound during
reset. M28 retains that behavior instead of attempting to replace a live SDL
audio device while the guest is running.

## Sound menu

The Sound menu adds:

- Sampling rate: 11.025, 22.05, or 44.1 kHz;
- Sound buffer: 40, 100, 200, 500, or 1000 ms, plus a validated Custom entry;
- ymfm fidelity: Minimum, Medium, or Maximum.

The existing 22.05 kHz default remains unchanged for compatibility. The GUI
labels 44.1 kHz as recommended for new configurations. Custom sound-buffer
input is transactional and accepts 40 through 1000 ms without silently
clamping the pending value.

The SDL callback buffer preserves the original power-of-two sizing rule. The
configured millisecond value is therefore a requested aggregate buffer length,
not a promise that one callback duration exactly equals that value.

## ymfm fidelity

`ymfm_fidelity=minimum|medium|maximum` is stored in `vaeg.cfg`. Missing or
unknown values select `minimum`, preserving the M17 behavior.

The options map to upstream ymfm `OPN_FIDELITY_MIN`, `OPN_FIDELITY_MED`, and
`OPN_FIDELITY_MAX`. They change the native YM2203/YM2608 generation rate before
the existing box downsampler. At the bridge's OPN/OPNA clocks these rates are
approximately 166, 333, and 998 kHz. Maximum fidelity has the highest CPU
cost. This setting affects only the ymfm FM-operator path; the menu is disabled
while the NP2 backend is selected. Timer/IRQ, SSG, ADPCM, rhythm, board I/O, and final
mixing remain NP2-owned as established by ADR-0009.

## Apply and persistence

Sampling rate and buffer changes update `SampleHz` / `Latencys`, set
`SYS_UPDATECFG` with the matching sound update flag, and set `soundrenewal`.
ymfm fidelity updates SDL2 frontend configuration and also sets
`soundrenewal`. All three use the existing GUI reset path, which preserves
mounted FDD images and reconstructs the sound device and generators from a
clean state.

Invalid values read from configuration are handled safely:

- unsupported `SampleHz` falls back to 22050;
- `Latencys` outside 40 through 1000 is clamped to that range;
- unknown `ymfm_fidelity` falls back to `minimum`.

## Non-goals

- Do not add independent rate or buffer controls for NP2 and ymfm.
- Do not change FM timers, IRQ, SSG, ADPCM, rhythm, or mixer ownership.
- Do not add live, reset-free SDL audio-device replacement.
- Do not edit the frozen reference tier or vendored ymfm source.

## Automated verification

ROM-less selftests cover accepted and rejected rates, buffer bounds and
power-of-two sample sizing, fidelity name parsing, invalid-value fallback, and
runtime fidelity selection.

Run:

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --smoke
cmake --preset linux-release
cmake --build --preset linux-release
cmake --preset mingw-cross
cmake --build --preset mingw-cross
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
git diff -- win9x i286x cpuxva/memoryva.x86 hlp
```

## G28 human gate

- Select 11.025, 22.05, and 44.1 kHz and confirm VA sound continues after the
  automatic reset.
- Restart and confirm `SampleHz` persistence.
- Select each buffer preset and a custom value within 40 to 1000 ms; restart
  and confirm `Latencys` persistence.
- Confirm custom values below 40 and above 1000 cannot be applied.
- Test NP2 and ymfm at 22.05 and 44.1 kHz with OPN on VA and OPNA on VA2/VA3.
- With ymfm selected, compare Minimum, Medium, and Maximum and confirm the
  choice persists as `ymfm_fidelity`.
- With NP2 selected, confirm the ymfm fidelity menu is disabled.
- Confirm Sound on/off, master volume, and seek/motor sound still work.
- Confirm reset preserves mounted FDD and SASI images.
- Complete the standard VA gate: V3 boot, VA demo, and OS boot/simple use.

G28 remains pending until the user reports that this checklist passed.

## Verification record

The following checks passed on the Linux development host:

- Linux debug configure/build, direct ROM-less selftest, and dummy-driver
  smoke;
- Linux release configure/build, direct ROM-less selftest, and dummy-driver
  smoke;
- Windows-MinGW cross configure/build using the pinned static SDL2 and archive
  dependency paths;
- encoding, EOL, lowercase-path, and `git diff --check` invariants;
- an empty path-scoped diff for `win9x/`, `i286x/`,
  `cpuxva/memoryva.x86`, and `hlp/`.

The configured Linux debug tree does not enable CMake test registration, so
`ctest --test-dir build/linux-debug` reported `No tests were found`. The same
binary's explicit `--selftest` completed successfully, including sound-option
validation and ymfm Minimum/Medium/Maximum selection and audio generation.

Linux release `vaeg` and Windows-MinGW `vaeg.exe` were copied to the shared
manual-test directory and verified byte-for-byte by SHA-256. Native macOS and
audible quality/latency comparison remain outside automated verification and
are part of G28.
