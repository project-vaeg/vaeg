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
# ADR-0009: Selectable OPN/OPNA FM Backend

Date: 2026-07-10

Status: Accepted

## Context

The active tree uses the NP2 OPN generator in `sound/opngenc.c` and
`sound/opngeng.c`. Its public interface is `opngen_initialize()`,
`opngen_setvol()`, `opngen_setVR()`, `opngen_reset()`,
`opngen_setcfg()`, `opngen_setextch()`, `opngen_setreg()`,
`opngen_keyon()`, `opngen_getpcm()`, and `opngen_getpcmvr()`. The backend
layer adds `opngen_setcontrol()` for FM control registers that previously
only reached the separate NP2 timer path.

The sound board implementation already separates the Yamaha chip roles:

| Responsibility | Existing owner |
|---|---|
| Board I/O, register address/data ports, and register shadows | `cbus/board*.c`, `iova/boardsb2.c`, and `sound/fmboard.c` |
| FM operator synthesis | `sound/opngenc.c` and `sound/opngeng.c` |
| Timer A/B and IRQ delivery | `sound/fmtimer.c` |
| SSG | `sound/psggenc.c` and `sound/psggeng.c` |
| ADPCM-B | `sound/adpcmc.c` and `sound/adpcmg.c` |
| OPNA rhythm | `sound/rhythmc.c` and its existing sample mixer |
| Final stream mixing and SDL output | `sound/sound.c` and `sdl2/soundmng.c` |

Importing MAME device classes would also import GPL-covered framework code
and duplicate these responsibilities. The reusable `3rdparty/ymfm` subtree is
independent and BSD-3-Clause licensed.

## Decision

Keep the NP2 implementation as a selectable compatibility backend and add
`ymfm` as the default FM operator backend for YM2203 (OPN) and YM2608 (OPNA).

| Field | Value |
|---|---|
| Name | ymfm |
| MAME release | `0.288` (`mame0288`) |
| Upstream URL | `https://github.com/mamedev/mame/tree/mame0288/3rdparty/ymfm` |
| Standalone upstream | `https://github.com/aaronsgiles/ymfm` |
| MAME tag object | `2c38dc6e555e17560bbf6f5531c3e86cf8570f54` |
| MAME release commit | `27a8d9e85b58058965907d1d8a7a92f8ed039348` |
| ymfm subtree SHA | `764185eef1ee5c30b1bdc2ef86ec90adb7c00948` |
| License | BSD-3-Clause, copyright Aaron Giles |
| Vendored path | `external/ymfm` |

The vendored scope is limited to `LICENSE`, `README.md`, and the OPN
dependencies: `ymfm.h`, `ymfm_fm.h`, `ymfm_fm.ipp`, `ymfm_opn.*`,
`ymfm_ssg.*`, and `ymfm_adpcm.*`. MAME device code, examples, and unrelated
chip families are not included. Vendored files are exact upstream copies and
must not be hand-edited.

The handwritten C++ bridge is confined to `sdl2/ymfmbridge.cpp`, where C++17
is already permitted. `sound/ymfmbridge.h` is the C ABI consumed by the C99
sound core. The existing `opngen_*` API remains the only API used by sound
boards.

Both FM engines receive the same FM register, key-on, and channel-3 mode
writes. The selected backend only controls which engine contributes PCM.
Changing the backend performs the existing GUI guest-reset flow because an
inactive synthesizer does not advance its envelope and phase state. Selection
is stored as `opn_backend=np2|ymfm` in the SDL2 `vaeg.cfg`; missing or invalid
values select `ymfm`. An existing explicit `opn_backend=np2` remains effective.

This is a staged integration. ymfm supplies FM operator output only. The NP2
timer remains authoritative and forwards expiry cadence to ymfm for FM CSM;
timer status and IRQ delivery remain NP2-owned. SSG, ADPCM-B, rhythm, board
I/O, register shadows, and the final mixer also remain NP2-owned. The wrapper
uses selectable ymfm minimum, medium, or maximum OPN fidelity and box
downsampling from the native ymfm output rate to the configured host rate.
Minimum remains the compatibility default; the SDL2 frontend persists the
selection as `ymfm_fidelity`.

## Consequences

- Fresh configurations and configurations without a valid backend value use
  `ymfm`; an existing explicit `opn_backend=np2` preserves NP2 synthesis.
- The GUI Sound menu can switch FM synthesis between `NP2` and `ymfm`; the
  automatic guest reset rebuilds the board and keeps configured FDD/SASI
  mounts.
- The GUI exposes ymfm OPN fidelity only while `ymfm` is selected. Higher
  fidelity increases the native generation rate and CPU cost but does not
  change NP2-owned timer/IRQ, SSG, ADPCM, rhythm, or mixer behavior.
- Save states continue to use the existing NP2 register shadows. Loading a
  state restores both FM backends through the normal `fmboard` replay path.
- A later decision is required before moving SSG, ADPCM, rhythm, timer, status,
  or IRQ ownership to ymfm.
- `OPNGENX86` remains forbidden and undefined.
