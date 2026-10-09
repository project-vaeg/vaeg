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

# M89 - merge active VA source directories

Predecessor: G88 passed; M88 is merged to `main` at
`b142bc37c4fe0cc50381727eac5766a5b3843e71`.

Branch: `topic/m89-merge-va-source-directories`

Commit prefix: `M89:`

Candidate gate: `G89`

## Scope

M89 consolidates the active VA source tree without changing guest-visible
behavior or binary payloads:

- move the active files in `biosva/` into `bios/`;
- move the active files in `vramva/` into `vram/`;
- move the active files in `cpucva/` into `cpu/`;
- update CMake source/include lists, active includes, tests, and current
  documentation to use the consolidated paths;
- preserve existing file names with `va` suffixes, public symbols, save-state
  section names, and the approved C++ compatibility backend behavior.

The `cpu/` destination is the ownership boundary for the uPD9002 main-CPU
adapter and the shared uPD780/uPD70008-compatible backend. The move does not
merge those implementations with `cpu/upd9002/` or `cpu/upd780/`; those
subdirectories remain distinct implementation owners.

## Non-goals

M89 must not change instruction semantics, video behavior, BIOS behavior,
storage behavior, ROM/font/disk payloads, or public API names. Historical
reports may retain old paths when they describe the tree at their recorded
checkpoint; current build files and current operational documentation must
use the consolidated paths.

## Commit order

1. Rename-only source-directory consolidation.
2. Include, CMake, test, and current-documentation reference fixups.
3. M89 validation report and ROADMAP gate record.

## Validation

Run repository invariant checks, Linux Debug and CI builds/tests, MinGW
cross-build, active-path scans, and the standard human VA gate: clean
checkout, V3 mode, bundled VA demo, OS boot, simple FDD/SASI/SCSI/keyboard/
display/state-save operations, plus Screen font and MPU98II checks.

Do not start M90 or merge M89 to `main` before G89 approval.
