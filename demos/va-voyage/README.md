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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# VA — Visual Audio Voyage / first prototype

[M107 task and human gate](../../docs/agents/tasks/M107_visual_audio_voyage.md).
An independently authored 1988-style visual/audio study: neon wave mesh,
forward travel, slow banking/heave, stars, vector VA title and an original
three-voice FM / SSG-noise score. No artwork, music, code or data from the
historical demonstration disk is included. This is the short first look, not
the planned 90-second production. **G107 remains a human gate.**

## Build and launch

Requires Python 3 and NASM. All generated files must stay outside the repository:

```sh
out=$(mktemp -d)
demos/va-voyage/build.sh "$out"
PYTHONDONTWRITEBYTECODE=1 python3 demos/va-voyage/test_voyage.py
```

The build produces `VOYAGE.raw.bin`, `VOYAGE.COM`, the assembly listing,
source-generated Q7 trigonometry and score tables, and their JSON source data.
Install **the wrapped `VOYAGE.COM`, not the raw payload**, on a private writable
copy of a PC-Engine 1.1 boot disk. The installer expects a directory containing
`root/VOYAGE.COM`, not a COM filename:

```sh
mkdir -p "$out/payload/root"
cp "$out/VOYAGE.COM" "$out/payload/root/"
python3 tools/pc88va/pcengine_disk.py install \
  --image /private/generated-validation.d88 --payload "$out/payload"
```

Boot V3, type `VOYAGE`, and press Escape to return. Never install on an original
or commit a bootable disk, executable or private capture. There is no
prebuilt/distribution disk in this increment.

`NASM` can select another assembler executable. `VOYAGE_AUDIO=0` builds a
forced-silent diagnostic variant without initializing the chip. Only exact
values `0` and `1` are accepted. This is not a validator bypass or a claim of
successful absent-hardware testing.

## Rendering and synchronization

- 320x200 packed 8bpp G1, two 64,000-byte pages within its 320x400 backing;
  G0 supplies a black background. The existing NEON4 default BIOS/FB1 setup
  and NEON3 loader continuation are reused, not changed.
- CPU signed fixed-point projection of a 12-by-9 travelling wave lattice;
  independent periodic forward, bank, heave and wave phases. SGP LINE/CLS
  rasterize it. No polygon mesh fill, texture sampler, TSP sprite, per-line
  scroll trick or claim about the historical demo's SGP usage.
- Native 8-bit direct distance colours (G3:R3:B2) and beat brightness, not programmable-palette
  animation. Border vertices are clamped; accurate line clipping and a
  filled/textured sea are later aesthetic work, not promised here.
- One **observed VBlank-edge** clock, also sampled during projection, SGP
  waits and OPN BUSY waits. Music follows 128 events across 900 observed ticks;
  geometry targets an update every three ticks, hidden-page presentation at
  VBlank. The initial 1200/four-tick version ran longer than 20 seconds in the
  default worker; this short prototype is calibrated toward 20 seconds there,
  not to a universal 60Hz timebase. A missed entire blank pulse can slow this
  polling clock: real-hardware timing and overruns require separate observation;
  no exact wall-clock or portable BPM claim.
- Three low-bank YM2203/YM2608 voices (ports 44h/45h) and SSG noise percussion;
  normal BIOS FM prescaling assumed. No OPNA-only upper-bank/ADPCM dependency.
  FFh status or a bounded BUSY timeout disables further sound accesses;
  geometry/rhythm continue silently. All hardware waits are bounded.

The demo owns the sound chip exclusively: stop any resident music first.
It retains SSG I/O direction bits, but does **not** snapshot unreadable FM
registers or resume a previous song. Exit keys off the three voices, mutes
SSG, stops/acknowledges sound timers and aborts our SGP submission if necessary.
An unresponsive chip cannot be guaranteed to mute. Saved video mode/pixel sizes
and the normal console guide/composition are restored before loader return;
prior graphics contents and arbitrary custom window definitions are not saved.

The loader uses the established fixed `3000h` payload segment and caller stack,
with its `E000h` continuation reserve. This is the repository's local PC-Engine
validation contract, not a general DOS allocator or portability guarantee.

## Verification boundary

The generator validates data before writing. Focused tests use passing content
fixtures and one mutation with exact error codes; arithmetic, wrap, command
capacity and ready/absent/stuck sound **models** are checked independently of
Git. The sound models are not execution of the guest polling code. NASM builds
and bounded private ROMful captures supplement these tests; they do not prove
real hardware, subjective audio quality or the full clean-checkout boot/OS gate.

Review at least two loops, sound, motion and colour, then Escape and relaunch.
No longer presentation, textured sea, TSP logo or emulator changes should begin
until the maintainer passes G107.
