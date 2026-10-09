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
# M40: Replace Z80 disassembly and detach active legacy-header consumers

Status: complete; G40 passed on 2026-07-15

Branch: `topic/m40-z80-disassembler`

## Goal and implementation

Independently author the ADR-0011 BSD-2-Clause vaeg disassembler from
documented Z80 encodings. Do not copy M88/cisc or GPL tables. Keep both CPU
selections for final comparison, with the previously approved default.

Cover base, CB, ED, DD, FD, DDCB, and FDCB pages; use bounded output; return
next PC or explicit length; wrap reads at `0xffff`; handle reserved/truncated
input deterministically; and include only reviewed canonical undocumented
forms required by the consumer. Preserve the active bridge's `Init(memory)` /
`Disassemble(pc,destination)` behavior or migrate all consumers atomically.
Do not retain unused diagnostic APIs or expose STL/third-party types.

## Tests and detachment

Generate a legal exhaustive opcode-page corpus covering length, next PC,
bounds, state non-mutation, wrap, and stable formatting. Create a manually
reviewed golden output from documented encodings, not from the old
disassembler. A permissively licensed independent length cross-check may flag
review items but is not a source to copy. Perform a legal ROM-less or private
debugger UI spot check without committing proprietary bytes.

Migrate active subsystem/debugger consumers so none includes `z80diag.h`,
`z80.h`, `z80if.h`, or `types.h`. The old CPU target may still use the seven
files internally until M41. Prove this with source search, compiler dependency
output, and link/source-list evidence.

## Gate G40

Run invariants, all builds/CTest/smoke, exhaustive/golden tests on every
platform, debugger spot check, and new-core system smoke. Both CPU selections
must build, and the include graph must confine legacy files to the legacy CPU
target. Push exact evidence and SHAs, then stop; do not cut over or delete.

## Implementation result

M40 independently authored the BSD-2-Clause decoder in `z80_disasm.*`, moved
the production subsystem seam to its bounded API for both CPU selections, and
removed the temporary M39 bridge and the new wrapper's inert `GetDiag()`
placeholder. The portable active tree has no disassembly-string parser or
interactive Z80 debugger; the frozen Win9x caller remains unchanged. Legacy
diagnostic headers and implementation are confined to the still-selectable
legacy CPU until M41, and all seven approved files remain present.

The deterministic suite covers 3,844 exhaustive page/displacement cases, 31
manually reviewed golden rows, buffers, wrapping, reserved encodings, repeated
prefixes, and non-mutation. GCC, Clang, sanitizers, MinGW/Wine, ZEX, both
production selections, and a sanitized dual-core private boot/live-memory
spot check passed locally. Hosted Linux, native Windows, and native macOS
passed all 12 configured jobs. The final G40 disposition is recorded in the
M40 evidence report.
