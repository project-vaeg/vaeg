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
# uPD780C disassembly

The production subsystem uses the independently authored vaeg decoder in
`cpu/upd780/upd780_disasm.cpp`. It reads instruction memory through a function
pointer, writes within an explicit destination capacity, and returns the
16-bit next PC, exact byte length, and a status. It does not execute the CPU
or mutate CPU state.

## Source and license

The decoder is vaeg-owned BSD-2-Clause code, Copyright (c) 2026 Nakata Maho.
It was independently implemented from the documented Z80 `x/y/z/p/q`
encoding structure and prefix rules. No M88/cisc implementation, M88/cisc
opcode table, GPL opcode table, or other disassembler source/table was copied
or adapted. The approved MIT-licensed suzukiplan core was inspected for a
reusable public decoding API; its vendored revision has only execution-time
debug strings, so no disassembler code was taken from it and no vendored byte
was changed.

The reviewed golden file was produced from the new decoder and manually
checked against documented instruction encodings. It was not generated from
the legacy disassembler. The exhaustive test has a separate length classifier
implemented from the encoding rules; an installed permissively licensed
side-effect-free reference decoder was not available for an additional
machine cross-check.

## Public contract

`VaegUpd780Disassemble()` accepts a 16-bit PC, a destination and 32-bit capacity,
a fixed-width memory-reader callback, and opaque context. The result contains:

- `next_pc`, with 16-bit wrapping;
- `length`, the exact number of instruction bytes read;
- `status`, distinguishing success, an invalid reader, and the bounded-prefix
  guard.

The C-facing subsystem adds `subsystem_disassemble_bounded()`. The historical
`subsystem_disassemble()` signature remains as a 64-byte compatibility adapter
for the existing seam. No STL or third-party type crosses either interface.
The active SDL2 frontend has no interactive Z80 debugger UI and no active code
parses disassembly strings. The frozen Win9x reference caller remains
untouched.

## Canonical text

Output uses lower-case mnemonics and registers, `0x`-prefixed fixed-width
hexadecimal values, and a comma followed by one space. Relative operands are
rendered as absolute wrapped 16-bit targets. Indexed displacements use
`(ix+0x00)` or `(iy-0x02)` form. Recognized undocumented forms include SLL,
IXH/IXL/IYH/IYL, and indexed-CB register results.

All base, CB, ED, DD, FD, DDCB, and FDCB pages are decoded. A redundant DD/FD
prefix contributes to length and memory reads but is omitted from the
mnemonic; the final DD/FD selects IX or IY. Reserved ED encodings use a
deterministic `db 0xed, ...` representation and retain the exact consumed
length. A memory image containing only DD/FD prefixes has no finite Z80
instruction boundary, so the decoder stops after 32 index prefixes, returns
`VAEG_UPD780_DISASM_PREFIX_LIMIT`, length zero, the original PC, and
`<invalid-prefix-sequence>` rather than inventing a boundary.

## Tests

With `VAEG_ENABLE_TESTS=ON`, build and run:

```sh
cmake --build build/linux-ci-gcc --target vaeg_upd780_disasm
ctest --test-dir build/linux-ci-gcc --output-on-failure \
  -R '^vaeg_upd780_disasm$'
```

The deterministic corpus covers all 256 bytes of each base/CB/ED/DD/FD page,
all 256 DDCB and FDCB final opcodes over five representative displacements,
repeated prefixes, and wrap boundaries: 3,844 exhaustive cases. It checks
length, next PC, stable output, exact ordered reads, buffer guards, and state
non-mutation. The same target also checks the reviewed golden rows and zero,
one-byte, exact, truncated, large, null-reader, and unbounded-prefix outputs.

The M40 private spot check used a neutral authorized VA2/VA3 ROM/media set
under both production selections. Each reached the same OS date-entry prompt.
At a live subsystem execution boundary, 16 consecutive instructions were
read through `subsystem_disassemble_bounded()`; every returned next PC
advanced, output was readable, and live/public CPU PCs were unchanged by the
reads. Private filenames, paths, hashes, ROM bytes, disassembly text,
screenshots, and raw GDB logs remain outside Git. The active SDL2 frontend has
no interactive Z80 debugger UI, so this direct production-seam inspection is
the available debugger-equivalent check.

## M41 boundary

Production subsystem disassembly does not call `Z80C::GetDiag()` or include
`z80diag.h`, `z80if.h`, `z80.h`, or `types.h`. The temporary M39 bridge and
the legacy diagnostic implementation are removed. The seven approved
M88/cisc-derived Z80 files are absent from current HEAD. The bounded decoder
and compatibility adapter tests remain permanent.
