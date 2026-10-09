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

# M9 V30/uPD9002 handler map

## Normative source order

For the M9 V30/uPD9002 work, the normative behavior source is
`i286x/v30patch.cpp` as exercised by the LEGACY v141 build. The C port
must preserve that behavior, including reserved-opcode behavior and
quirks, unless a later ADR explicitly changes compatibility.

## Decode and handoff evidence

The VA main CPU is forced into V30/uPD9002 execution during reset:
`pccore.c:400-407` sets `CPU_TYPE = CPUTYPE_V30` whenever
`pccore.model_va != PCMODEL_NOTVA`. The execution loop then selects
`CPU_EXECV30()` or `v30*_step()` for V30 CPUs (`pccore.c:1147-1152`,
`pccore.c:1222-1235`). This is the legacy VA native execution path.

The Z80 FD subsystem is separate and is advanced after each main-CPU
slice via `subsystemmx_exec()` (`pccore.c:1236-1240`). The bridge
between the main CPU and Z80 is the I8255-backed subsystem interface:
main-side ports `0x0fc`, `0x0fd`, `0x0fe`, `0x0ff` are attached in
`iova/subsystemif.c:64-70`; Z80-side ports `0xf0`, `0xf4`, `0xf8`,
`0xfa`, `0xfb`, `0xfc`, `0xfd`, `0xfe`, `0xff` are handled in
`iova/subsystem.cpp:228-305`. That code is not the V2S-to-V3 native CPU
mode transition.

No BRKEM/BRKEM2/RETEM-named handler exists in the tracked source. The
only legacy source mention of a BREAKM-class V-series opcode is
`i286x/v30patch.cpp:1989-2003`: the `0F` dispatcher accepts only second
bytes below `0x40` through `v30ope0x0f_xtable`; for second bytes
`>= 0x40`, the comment says `FF(BREAKM)` is the only known opcode, and
the handler deliberately falls into `v30_reserved_0x0f`.

Maintainer correction for uPD9002/PC-88VA: VA does not use the
V30-compatible `BRKEM` encoding `0F FF nn`. The VA-specific transition
from V30 mode to uPD780/Z80 emulation mode is `BRKEM2`, encoded as
`0F FE nn`, where `nn` is the interrupt vector number. The tracked
legacy source does not implement this mode transition; it treats both
`0F FE nn` and `0F FF nn` as reserved through the high-subopcode branch
above. A faithful `BRKEM2` implementation is not just another V30
arithmetic/string handler: it requires a main-CPU uPD780/Z80 emulation
mode and a matching return path. It must not be wired to the separate
FDD subsystem Z80.

The current portable halt/fault site is therefore the missing V30 `0F`
hook in `i286c/v30patch.c`. `v30cinit()` copies the base i286 table
(`i286c/v30patch.c:793-807`), and `v30patch_op` currently lacks the
legacy `{0x0f, v30_ope0x0f}` patch (`i286c/v30patch.c:557-582`).
Thus opcode `0F` is still `i286c_cts` from the base i286 tables
(`i286c/i286c_mn.c:3082`, `i286c/i286c_mn.c:3427`), and the base i286
upper-opcode handler faults any non-`00`/`01`/`05` subopcode
(`i286c/i286c_0f.c:259-287`). The legacy V30/uPD9002 path instead
dispatches `0F 10`..`0F 2A` through `v30_ope0x0f`.

The `0xfff0` uPD9002 I/O port is not the native-mode handoff. It stores
the timing-control byte `upd9002.tcks` and calls `pit_ontckschanged()`
(`iova/upd9002.c:18-21`); the port is bound at `iova/upd9002.c:37-40`.

## Missing handler table

| Family | Opcode(s) | Legacy handler and semantics | i286x source | i286c hook point |
|--------|-----------|------------------------------|--------------|------------------|
| 0F dispatch | `0F xx` | V30/uPD9002 second-byte dispatcher. Subopcodes `00`..`3F` go through `v30ope0x0f_xtable`; `>= 0x40` goes to reserved behavior. The tracked legacy comment mentions `FF(BREAKM)`, but the PC-88VA/uPD9002 transition opcode is `FE(BRKEM2)`. | `i286x/v30patch.cpp:1989-2003`, patched at `2011-2014`; maintainer correction for uPD9002/VA BRKEM2 | Add `{0x0f, v30_ope0x0f}` to `v30patch_op` before `v30cinit()` copies tables. Preserve reserved behavior unless main-CPU uPD780/Z80 emulation is implemented. |
| 0F reserved | `0F` table reserved slots, including current `0F FE` and `0F FF` paths | Clock 2 and do not advance beyond the bytes already consumed by the dispatcher. This preserves the legacy debug-stop/reserved behavior for unimplemented high subopcodes. | `i286x/v30patch.cpp:1067-1074`, table at `1918-1985` | Add `v30_reserved_0x0f` and use it in a C `v30ope0x0f_table`. |
| uPD9002 BRKEM2 | `0F FE nn` | VA-specific break from V30 mode to uPD780/Z80 emulation mode; `nn` is the interrupt vector number. VA does not use V30-compatible `BRKEM` (`0F FF nn`) for this transition. Not implemented in the tracked legacy source. | No handler exists; `i286x/v30patch.cpp:1989-1997` currently sends all `0F >= 40` subopcodes to `v30_reserved_0x0f`. | Future implementation requires a main-CPU uPD780/Z80 mode plus the matching return-from-emulation path. Do not route this to the FDD subsystem Z80 (`iova/subsystem.cpp`). |
| Bit test | `0F 10 /r`, `0F 11 /r` | `TEST1 r/m8,CL` and `TEST1 r/m16,CL`; bit index is `CL & 7` or `CL & 15`; update flags as the x86 `test` does, without modifying the operand. | `i286x/v30patch.cpp:1407-1510` | C handlers called by `v30ope0x0f_table[0x10]` and `[0x11]`; use `REG8_B20`, `REG16_B20`, `CALC_EA`, memory read helpers, and flag macros. |
| Bit clear/set by CL | `0F 12 /r`, `0F 14 /r` | `CLR1 r/m8,CL` and `SET1 r/m8,CL`; bit index is `CL & 7`; no flags are changed in the legacy handler. | `i286x/v30patch.cpp:1513-1608` | C handlers called by `v30ope0x0f_table[0x12]` and `[0x14]`. |
| Bit test by immediate | `0F 18 /r ib`, `0F 19 /r ib` | `TEST1 r/m8,imm3` and `TEST1 r/m16,imm4`; immediate low bits select the tested bit; update flags as `test`. | `i286x/v30patch.cpp:1610-1719` | C handlers called by `v30ope0x0f_table[0x18]` and `[0x19]`; consume the immediate byte after EA decode. |
| Bit clear/set by immediate | `0F 1A /r ib`, `0F 1B /r ib`, `0F 1C /r ib`, `0F 1D /r ib` | `CLR1 r/m8,imm3`, `CLR1 r/m16,imm4`, `SET1 r/m8,imm3`, `SET1 r/m16,imm4`; no flags are changed in the legacy handler. | `i286x/v30patch.cpp:1722-1916` | C handlers called by `v30ope0x0f_table[0x1a]`..`[0x1d]`; consume the immediate byte after EA decode. |
| BCD string | `0F 20`, `0F 22` | `ADD4S` and `SUB4S`. Operate on packed BCD bytes at `DS:SI` and `ES:DI`, loop count `(CL + 1) >> 1`, store each result to `ES:DI+n`, and reduce final flags to the legacy `CF/ZF` cases with `OF` cleared. | `i286x/v30patch.cpp:1076-1335` | C handlers called by `v30ope0x0f_table[0x20]` and `[0x22]`; use byte memory reads/writes and local DAA/DAS equivalents. |
| Rotate nibble | `0F 2A /r` | `ROR4 r/m8`. Low nibble of operand is merged into low nibble of `AL`; previous high nibble of `AL` is merged into high nibble of operand after a 4-bit rotate. Flags are not changed by the legacy handler. | `i286x/v30patch.cpp:1337-1404` | C handler called by `v30ope0x0f_table[0x2a]`. |
| REPC prefix | `65` | V30 `REPC` prefix. Dispatches through a dedicated `v30op_repc` table. Unsupported following opcodes use `v30_reserved_repc`, which restores the saved repeat position/prefetch in x86 asm; C equivalent must not consume an unsupported opcode permanently. | `i286x/v30patch.cpp:181-188`, `2230-2361`, patched at `2027-2030` | Add `v30op_repc`, `v30_repc`, `v30_reserved_repc`, `v30patch_repc`, and initialize it in `v30cinit()`. |
| REPC SCASB | `65 AE` | `REPC SCASB`: while `CX != 0`, compare `AL` with `ES:DI`, advance `DI` by direction, decrement `CX`, and continue while carry is set. | `i286x/v30patch.cpp:2292-2315`, table at `2317-2323` | Add `v30repc_xscasb` as `v30op_repc[0xae]`; use `SUBBYTE`/carry-compatible flag calculation and `STRING_DIR`. |
| REPC segment overrides | `65 26`, `65 2E`, `65 36`, `65 3E` | Segment override inside `REPC`; changes `DS_FIX` and `SS_FIX`, then dispatches the next byte through `v30op_repc`. | `i286x/v30patch.cpp:2243-2289`, table at `2317-2323` | Add `v30repc_segprefix_es/cs/ss/ds` and patch them into `v30op_repc`. |
| V30 IRET | `CF` | V30/uPD9002 `IRET` under `VAEG_FIX`: pop IP, CS, FLAGS; set `CS_BASE`; force V30 flag bits with `0xf002`; set trap from bit 8 only; terminate early for trap or pending IRQ. | `i286x/v30patch.cpp:702-752`, patched at `2041-2043`, `2131-2133`, `2217-2219` | Add `v30_iret` and patch opcode `0xcf` in normal, `REPE`, and `REPNE` V30 tables. |
| POP SP semantic deviation | `5C` | uPD9002/V30 `POP SP` assigns the stack word to `SP` and does not add 2 afterwards. Legacy `VAEG_FIX` achieves this by not patching V30 opcode `5C`, falling back to the base `pop_sp` implementation. | Comment at `i286x/v30patch.cpp:2021-2025`, `2116-2120`, `2202-2206`; base behavior at `i286x/i286x.cpp:1604-1614` | `i286c/v30patch.c:166` currently patches V30 `5C` to `REGPOP`, which increments after assignment. Change the V30 patch to use the C base behavior (`SP_POP`) or stop overriding `5C`. |

## Legacy table notes

- `NOT1` is not a handler in the tracked legacy source. The V30 `0F`
  table reserves `0F 16` and `0F 17` (`i286x/v30patch.cpp:1942-1943`),
  and there is no `not1` symbol in `i286x/v30patch.cpp`. M9 therefore
  ports the actual legacy behavior: those opcodes stay reserved.
- `ROL4` is likewise not implemented in the tracked legacy table;
  `0F 28` and `0F 29` are reserved (`i286x/v30patch.cpp:1961-1962`).
  `ROR4` at `0F 2A` is the only rotate-nibble handler present.
- `0F FE nn`/BRKEM2 is the PC-88VA/uPD9002 mode-transition opcode, but
  it has no explicit C++ handler in legacy. The only source-level
  behavior today is the `v30_ope0x0f` high-subopcode branch to
  `v30_reserved_0x0f` (`i286x/v30patch.cpp:1991-1997`).
- `0F FF nn`/BRKEM is the V30-compatible 8080-emulation break encoding,
  but the VA/uPD9002 path should not use it for the V30-to-uPD780/Z80
  transition.
