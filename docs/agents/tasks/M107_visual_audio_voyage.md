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

# M107 — Visual Audio Voyage prototype

## Authorization and boundary

The maintainer requests an original 1988-style PC-88VA visual/audio demo with
undulating motion, and authorizes proceeding with the recommended short sea
prototype. M106's applicable documentation machine gates passed; its unresolved
ROM/runtime questions remain separate. This is one guest-demo milestone, not an
emulator, CPU, SGP, BIOS, sound-backend or private-media correctness milestone.

## First human-gate candidate

- Build an isolated NASM V3 guest payload and wrapped `VOYAGE.COM` using the
  existing NEON loader continuation contract. Sources under `demos/va-voyage/`.
- Original perspective wave mesh, slow camera bank/heave, stars and vector VA
  title; approximately 20-second cyclic choreography. A wire-mesh sea is the
  deliberately bounded first look, not the completed 90-second presentation.
- CPU fixed-point projection, SGP LINE/CLS rasterization, hidden-page composition
  and VBlank display selection. This differs from the analyzed demonstration's
  CPU pixel sampler; do not claim that the old demonstration uses our backend.
- Common low-bank YM2203/YM2608 FM and SSG, original note sequence and bounded
  status polling; absent/failing sound leaves a silent visual demonstration.
- One observed VBlank timeline for motion and musical events, polled also during
  SGP waits. Calibrated 900 observed ticks per loop, geometry target every three
  ticks. The initial 1200/four-tick candidate ran longer than 20 seconds under
  the default VA2 worker; this is prototype calibration, not a portable timebase.
  Measure emulator cadence; do not claim real-hardware or exact wall-clock timing.
- Escape exit, bounded hardware waits, audio shutdown and video/loader return.
  Audio is taken over exclusively; resuming another resident song is not promised.
- Generated trigonometric data and preview/validation evidence go out of tree.
  No private art, notes, source, filenames or hashes enter tracked files. Do not
  use INT91H selector01, undocumented BIOS probes or original demo payloads.
- Bootable validation disk remains private, generated from a read-only local
  PC-Engine template. No D88, COM, image, ROM, music sample or report capture is
  a tracked deliverable. No distribution-image or aggregate-demo changes yet.

## Local validation

Build twice out of tree and compare outputs; test generated data/geometry/music,
command capacity, wrap and coordinate bounds, missing audio and bounded polling
contracts at the smallest relevant layer. Build the existing Linux debug worker
and run its smoke check; run a bounded ROMful private demonstration capture,
verify motion and successful Escape return, and retain worker/guest/input/output
identities privately. Do not run SSTs or hosted CI for unchanged emulator code.
Run repository encoding, LF, case and staged-diff checks; review exact scope and
privacy before commit/push. Record applicable output in the PR.

## G107 — HUMAN GATE (pending)

Clean-checkout build; normal V3/VA demo/OS regression checklist, then launch
VOYAGE, watch at least two loops, listen to FM/SSG and assess smooth undulation,
colour, rhythm and visible synchronization. Escape must mute and return to the
prompt; launch again. VA/VA2 and real hardware are independent claims, not implied
by an emulator capture. Do not begin a longer sequence, filled/textured sea,
TSP-logo layer or emulator optimization before the maintainer passes this gate.
