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
# M41: Cut over to the replacement Z80 and remove the legacy implementation

Status: complete; G41 passed

Branch: `topic/m41-z80-cutover`

## Required cutover

Make the suzukiplan-backed wrapper the only production core. Remove the legacy
selection, sources, definitions, and CI jobs; keep permanent wrapper,
interrupt, state-fixture, ZEX, disassembler, deterministic self-consistency,
smoke, and archive-audit tests. There is no one-release fallback.

Delete exactly, and only with the standing approval, these seven files:

```text
cpucva/types.h
cpucva/z80.h
cpucva/z80if.h
cpucva/z80c.h
cpucva/z80c.cpp
cpucva/z80diag.h
cpucva/z80diag.cpp
```

Use deletion commits consistent with the repository's one-concern rules. Do
not delete anything else without explicit approval. Remove active includes,
CMake entries, scripts, tests, and package references; frozen reference files
remain untouched and may retain documented historical names.

Update README/build/release documentation, roadmap/task/ADR status, notices,
and packaging records with exact MIT vendored SHA and BSD-2-Clause vaeg
components. Update the bug ledger only for demonstrated corrected defects.
History is not rewritten.

## Release and permanent QA

Build the real distribution archive and machine-readable manifest. Fail if it
contains any approved deleted path, renamed/stale copy, ZEX artifact,
unrecorded third-party source, patch work file, or private ROM/disk data. Run
case-insensitive cisc/M88 and exact legacy-path searches; historical migration
documentation hits may remain, but active source/build/package hits may not.
Verify all seven paths are absent from HEAD and archive.

Run complete three-platform CI, invariants, CTest, wrapper/interrupt/state,
ZEX, exhaustive disassembly, bounded deterministic tests, smoke, and release
audit. Preserve final differential/cycle reports without compiling deleted
code.

## Human gate G41

From a clean checkout, the maintainer verifies VA/V3 boot, bundled demo, OS
file operations, FDD read/write, timing-sensitive loader, VA and Sorcerian
idle, WAIT wake, old-save load, new save/load during FDD activity, reset/
reboot, and archive contents. G41 passes only when tests and human regression
pass, all seven files/fallbacks are absent, the archive is clean, and exact
SHAs are reported. Tag only after explicit approval. Stop.

## Execution record

M41 removed the production selector, made the suzukiplan-backed wrapper and
BSD-2-Clause decoder unconditional, deleted exactly the seven approved files,
collapsed duplicate CI jobs, retained standalone conformance/regression
coverage, and strengthened the archive audit against copied legacy bytes and
private/test artifacts. Revision-1 remains 68 bytes and all 15 retained
fixtures pass.

Fresh private regression on 2026-07-15 passed VA2/VA3 boot, FDD read,
expendable-copy write/readback, timing-sensitive loader, VA and Sorcerian
SLEEP_HACK with ATN WAIT wake, legacy-to-final and final-to-final state, an
FDD-active frame-boundary state, and a 16-instruction live disassembly spot
check. Only neutral identifiers and sanitized summaries are tracked.

Exact implementation, validation, archive, licensing, and private evidence is
in the [M41 evidence report](../reports/m41_z80_cutover.md).
