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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98a - Reserve the generic Zundamon orbit scope

Status: **G98a human gate passed on 2026-08-31**

Branch: `topic/m98-zundamon-orbit`

Commit prefix: `M98a:`

## Goal

Reserve M98 for a source-neutral billboard-orbit demonstration and freeze its
architecture, privacy boundary, stage order, and gate discipline before any
image-processing or guest-rendering implementation begins.

## Required changes

- Add the M98 roadmap entry and master plan.
- Name the public project and paths Zundamon orbit.
- Define the public interface as an explicitly supplied local BMP, palette,
  and manifest without recording source-identifying metadata.
- Keep all maintainer inputs and generated integration media outside Git.
- State explicitly that the effect is a camera-facing billboard orbit, not
  genuine multi-angle rotation.
- Leave M98c and later unassigned.

## Out of scope

- Asset acquisition, identification, or provenance investigation.
- Private input processing.
- Demo, guest, BMS, SGP, disk-image, or emulator implementation.
- Passing any machine, VAEG, physical-machine, or later human gate.

## Acceptance

Repository documentation checks pass and the maintainer explicitly assigns
the generic Zundamon orbit family. Stop after recording G98a.
