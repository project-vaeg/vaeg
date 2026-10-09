# Intel 8087 v7 opcode inventory

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This inventory is hand-authored from the supplied Intel I01/I04 tables and
the NEC N01 FPO1 encoding table. It is deliberately independent of the VAEG
decoder implementation. The machine-readable rows are in
[`opcode-inventory.tsv`](opcode-inventory.tsv).

For a memory row, `00-7F` means all 24 memory ModR/M bytes (`mod=00`, `01`,
or `10`, each with `rm=0..7`). For a register row, only `mod=11` is used and
the listed ModR/M byte or range is exact. The inventory therefore expands to
1,597 documented FPO1 slots out of the 2,048 D8-DF/ModR/M combinations:

| partition | slots |
| --- | ---: |
| documented memory forms | 1,368 |
| documented register forms | 229 |
| total `DOCUMENTED_8087` | 1,597 |
| reserved/undefined FPO1 slots | 451 |

FPO2 (`66`/`67`) is outside this inventory by contract. It remains a native
CPU-side reserved/no-device path, and no 8087 state is attached to it.

The inventory intentionally records documented forms rather than later-x87
aliases. In particular, it excludes `FNSTSW AX`, `FUCOM*`, `FCOMI*`,
`FCMOV*`, `FISTTP`, `FSIN`, `FCOS`, `FSINCOS`, and `FPREM1`.
