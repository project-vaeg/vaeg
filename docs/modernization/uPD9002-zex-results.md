<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# ZEXDOC/ZEXALL on the µPD9002 Z80 emulation mode (PC-88VA2, V2 mode)

## 1. Summary

- Both suites were run on a real PC-88VA2 in V2 mode.
- ZEXDOC: 55 OK, 12 ERROR. ZEXALL: 35 OK, 32 ERROR.
- A reference Z80 core modified by eleven flag rules (R1–R11, §4)
  reproduces every reported found CRC exactly: ZEXDOC 12/12 and ZEXALL
  32/32 failing groups, and all passing groups.
- ZEXDOC failures are documented-flag deviations (H, N, P/V), not only
  undocumented-bit differences.
- M101 re-ran both suites from recorded exerciser sources, together with
  direct flag probes and exhaustive DAA/CPL/SCF/CCF dumps (§2, §9). Every
  rule is now directly observed, and the vaeg uPD9002 profile reproduces all
  of these outputs byte for byte.

| Pattern | Groups | Explained by |
| --- | ---: | --- |
| ZEXDOC ERROR, ZEXALL ERROR | 12 | 11 by documented-flag rules R2–R5; #13 by R8–R11 |
| ZEXDOC OK, ZEXALL ERROR | 20 | 16 by R1 alone; #1–#4 need R1 plus H deviations R6/R7 |
| ZEXDOC OK, ZEXALL OK | 35 | agreement within the exercised space only; #29–#31 are degenerate (§6 C2) |

The ZEXALL-only failures are therefore not all undocumented-bit (Y/X)
differences. For #1–#4 the documented H flag also differs; ZEXDOC does not
see it because its mask for these groups (C7h) excludes H (§6 C1).

## 2. Provenance

| Item | ZEXDOC | ZEXALL |
| --- | --- | --- |
| Machine / mode | NEC PC-88VA2, V2 mode | NEC PC-88VA2, V2 mode |
| CPU | NEC µPD9002, Z80 emulation mode (project identifier `upd70008`) | same |
| Source of record | `ZEXDOC.TXT`, written by the exerciser | `ZEXALL.TXT`, written by the exerciser |
| Executable | Modified build: output also written to a file; source and hash not recorded | same |
| Result | 55 OK / 12 ERROR | 35 OK / 32 ERROR |

The table describes the first run. M101 repeated both suites from recorded
exerciser sources, with identical results in every group (§9).

Both exercisers were modified so that the console output is also written to
a file, and the two `.TXT` files are that output. The results are therefore
machine-written, with no transcription step. The earlier photograph-based
reading is superseded.

The modification did not disturb the tested state. The test vectors place
memory operands and several register values at the fixed `msbt` area near the
start of the program. The found CRCs of groups that use `msbt` (for example
#6, #9, #10, #53–#56) are reproduced exactly with the stock binaries (§8),
so the modified builds kept both the test descriptors and the `msbt` address.

The two runs are mutually consistent:

- **Identical found CRCs.** Nine groups (#5–#8, #13, #54–#56, #59) have
  identical found CRCs in the two output files, although their expected CRCs
  differ (flag mask D7h vs FFh). This is what R1 predicts: masking bits 5/3
  has no effect when they are always 0.
- **#53 excluded.** Its expected CRCs are already identical in the two suites
  (`94f42769`), so identity there carries no information (§6 C7).
- **Test-vector set.** The expected CRCs in both output files equal those of
  the stock binaries (§8). This fixes the test vectors, not the executable
  build (O5).

## 3. Results

"Rule" refers to §4. In ZEXALL, every failing group additionally requires R1.

| # | Test group | ZEXDOC | ZEXALL | Rule |
| ---: | --- | --- | --- | --- |
| 1 | `<adc,sbc> hl,<bc,de,hl,sp>` | OK | ERROR | R1, R7 |
| 2 | `add hl,<bc,de,hl,sp>` | OK | ERROR | R1, R6 |
| 3 | `add ix,<bc,de,ix,sp>` | OK | ERROR | R1, R6 |
| 4 | `add iy,<bc,de,iy,sp>` | OK | ERROR | R1, R6 |
| 5 | `aluop a,nn` | ERROR | ERROR | R2 |
| 6 | `aluop a,<b,c,d,e,h,l,(hl),a>` | ERROR | ERROR | R2 |
| 7 | `aluop a,<ixh,ixl,iyh,iyl>` | ERROR | ERROR | R2 |
| 8 | `aluop a,(<ix,iy>+1)` | ERROR | ERROR | R2 |
| 9 | `bit n,(<ix,iy>+1)` | ERROR | ERROR | R3 |
| 10 | `bit n,<b,c,d,e,h,l,(hl),a>` | ERROR | ERROR | R3 |
| 11 | `cpd<r>` | OK | ERROR | R1 |
| 12 | `cpi<r>` | OK | ERROR | R1 |
| 13 | `<daa,cpl,scf,ccf>` | ERROR | ERROR | R8–R11 |
| 14 | `<inc,dec> a` | OK | ERROR | R1 |
| 15 | `<inc,dec> b` | OK | ERROR | R1 |
| 16 | `<inc,dec> bc` | OK | OK | — |
| 17 | `<inc,dec> c` | OK | ERROR | R1 |
| 18 | `<inc,dec> d` | OK | ERROR | R1 |
| 19 | `<inc,dec> de` | OK | OK | — |
| 20 | `<inc,dec> e` | OK | ERROR | R1 |
| 21 | `<inc,dec> h` | OK | ERROR | R1 |
| 22 | `<inc,dec> hl` | OK | OK | — |
| 23 | `<inc,dec> ix` | OK | OK | — |
| 24 | `<inc,dec> iy` | OK | OK | — |
| 25 | `<inc,dec> l` | OK | ERROR | R1 |
| 26 | `<inc,dec> (hl)` | OK | ERROR | R1 |
| 27 | `<inc,dec> sp` | OK | OK | — |
| 28 | `<inc,dec> (<ix,iy>+1)` | OK | ERROR | R1 |
| 29 | `<inc,dec> ixh` | OK | OK | — (degenerate, §6 C2) |
| 30 | `<inc,dec> ixl` | OK | OK | — (degenerate, §6 C2) |
| 31 | `<inc,dec> iyh` | OK | OK | — (degenerate, §6 C2, C3) |
| 32 | `<inc,dec> iyl` | OK | ERROR | R1 (§6 C3) |
| 33 | `ld <bc,de>,(nnnn)` | OK | OK | — |
| 34 | `ld hl,(nnnn)` | OK | OK | — |
| 35 | `ld sp,(nnnn)` | OK | OK | — |
| 36 | `ld <ix,iy>,(nnnn)` | OK | OK | — |
| 37 | `ld (nnnn),<bc,de>` | OK | OK | — |
| 38 | `ld (nnnn),hl` | OK | OK | — |
| 39 | `ld (nnnn),sp` | OK | OK | — |
| 40 | `ld (nnnn),<ix,iy>` | OK | OK | — |
| 41 | `ld <bc,de,hl,sp>,nnnn` | OK | OK | — |
| 42 | `ld <ix,iy>,nnnn` | OK | OK | — |
| 43 | `ld a,<(bc),(de)>` | OK | OK | — |
| 44 | `ld <b,c,d,e,h,l,(hl),a>,nn` | OK | OK | — |
| 45 | `ld (<ix,iy>+1),nn` | OK | OK | — |
| 46 | `ld <b,c,d,e>,(<ix,iy>+1)` | OK | OK | — |
| 47 | `ld <h,l>,(<ix,iy>+1)` | OK | OK | — |
| 48 | `ld a,(<ix,iy>+1)` | OK | OK | — |
| 49 | `ld <ixh,ixl,iyh,iyl>,nn` | OK | OK | — |
| 50 | `ld <bcdehla>,<bcdehla>` | OK | OK | — |
| 51 | `ld <bcdexya>,<bcdexya>` | OK | OK | — |
| 52 | `ld a,(nnnn) / ld (nnnn),a` | OK | OK | — |
| 53 | `ldd<r> (1)` | ERROR | ERROR | R5 |
| 54 | `ldd<r> (2)` | ERROR | ERROR | R5 |
| 55 | `ldi<r> (1)` | ERROR | ERROR | R5 |
| 56 | `ldi<r> (2)` | ERROR | ERROR | R5 |
| 57 | `neg` | OK | ERROR | R1 |
| 58 | `<rrd,rld>` | OK | ERROR | R1 |
| 59 | `<rlca,rrca,rla,rra>` | ERROR | ERROR | R4 |
| 60 | `shf/rot (<ix,iy>+1)` | OK | ERROR | R1 |
| 61 | `shf/rot <b,c,d,e,h,l,(hl),a>` | OK | ERROR | R1 |
| 62 | `<set,res> n,<bcdehl(hl)a>` | OK | OK | — |
| 63 | `<set,res> n,(<ix,iy>+1)` | OK | OK | — |
| 64 | `ld (<ix,iy>+1),<b,c,d,e>` | OK | OK | — |
| 65 | `ld (<ix,iy>+1),<h,l>` | OK | OK | — |
| 66 | `ld (<ix,iy>+1),a` | OK | OK | — |
| 67 | `ld (<bc,de>),a` | OK | OK | — |

## 4. Behavior model

Evidence status of every rule: **directly observed** (M101, §9).

- R1–R7 were first accepted because each reproduced the reported found CRC
  exactly (32 bits) and was the unique match among the candidates tried
  (§8). The M101 probes (`FLAGPRB`) then observed each of them as individual
  A/F values.
- R8–R11 were derived from exhaustive dumps: every combination of S, Z, H,
  P/V, N, C (bits 5/3 clear) times every value of A, for each of DAA, CPL,
  SCF and CCF (16,384 records each). The rules reproduce all records.
- A group CRC covers A, F, BC, DE, HL, IX, IY, SP and the memory operand. A
  match therefore also shows that all non-flag results (transferred data,
  pointer and counter updates, repeat termination) agree with the Z80 over the
  exercised state space.

The rules:

- **R1.** F bits 5 and 3 (Y/X) read as 0 after every exercised instruction.
  - On a Z80 these bits are copies of result bits or of internal values.
  - R1 alone reproduces 16 ZEXALL groups: #11, #12, #14, #15, #17, #18,
    #20, #21, #25, #26, #28, #32, #57, #58, #60, #61.
  - F cannot store bits 5/3: they read as 0 after `POP AF`, after
    `EX AF,AF'` twice and after a flag-neutral `INC DE` (probes P7a–P7d).
- **R2.** AND (all operand forms, including IXH/IXL and (IX+d)): H = 0.
  - Z80: H = 1.
  - ADD/ADC/SUB/SBC/CP/OR/XOR set their documented flags as on the Z80:
    P/V is overflow for arithmetic and parity for logic.
- **R3.** BIT n,r and BIT n,(IX+d): H = 0.
  - Z80: H = 1.
  - Z, S (for n = 7), P/V (= Z), N = 0 and C (preserved) behave as on the Z80.
- **R4.** RLCA/RRCA/RLA/RRA: only A and C change; H and N are preserved.
  - Z80: H and N are cleared.
- **R5.** LDI/LDD/LDIR/LDDR: P/V = (BC ≠ 0), as on the Z80; H and N are
  preserved.
  - Z80: H and N are cleared.
  - Transferred data, HL/DE/BC updates and repeat termination match the Z80.
- **R6.** ADD HL,rr / ADD IX,rr / ADD IY,rr: H is preserved.
  - Z80: H = carry out of bit 11.
  - N = 0 and C behave as on the Z80.
- **R7.** ADC HL,rr / SBC HL,rr: H = carry/borrow out of bit 3 of the low byte.
  - Z80: out of bit 11.
  - S, Z, P/V, N and C behave as on the Z80.
- **R8.** CPL: A = NOT A; no flag changes.
  - Z80: H = N = 1, Y/X from A.
- **R9.** SCF: C = 1; no other flag changes.
  - Z80: H = N = 0, Y/X from A.
- **R10.** CCF: C = NOT C; no other flag changes.
  - Z80: H = old C, N = 0, Y/X from A.
- **R11.** DAA, with the original A:
  - low step (±06h) when H = 1 or the low nibble is above 9;
  - high step (±60h) when C = 1, or A > 99h with H = 0, or A > 9Fh with
    H = 1;
  - N selects add (N = 0) or subtract (N = 1);
  - S and Z from the result; H = 1 when the low step ran; P/V = signed
    overflow of A ± the adjustment; C = 1 when the high step ran; N
    unchanged.
  - Z80: the high threshold is 99h regardless of H, and P/V is parity. For
    A = 9Ah–9Fh with H = 1 and C = 0 the result A differs (for example,
    A = 9Ah, F = 10h gives A0h here, 00h on a Z80).
- **U1** (the former unresolved `<daa,cpl,scf,ccf>` group) is resolved by
  R8–R11; the combined rules reproduce its found CRC `6096b6aa` in both
  suites.

The M102 campaign (§10) added rules for the remaining undocumented and
undefined opcodes, each directly observed on the real machine:

- **R12.** DD/FD CB BIT with a register operand (bits 2–0 ≠ 6) tests the
  register, not (IX/IY+d). **R13.** DD/FD CB RES/SET register forms modify
  only the register. (Zilog: both act on (IX/IY+d), and the rotate/shift
  register forms also copy the result to the register, which the µPD9002
  does as well.)
- **R14.** Undefined ED 00–3Fh, 74h, 75h, 77h: the register pair selected by
  bits 5–4 (74h/75h/77h: SP) is loaded with the little-endian word at bytes
  C and (C+1) mod 256 of the CPU's native-side physical page 0 — the
  x86-style interrupt vector table of the µPD9002's host side. F = sign of
  the high byte, zero of the word, parity of the low byte; H = N = C = 0.
  A, F, B, the pointers and Z80-visible memory do not affect the value.
- **R15.** ED 4Ch duplicates RETN (not NEG as on a Zilog Z80).
- **R16.** ED 64h/65h: RRD on A/(HL), and the low nibble of (HL+1) is also
  replaced by the old low nibble of (HL). ED 6Ch/6Dh: RLD on A/(HL), and
  (HL+1) is replaced by old-A-low:old-(HL)-high. F = parity of the new A,
  all other flags cleared.
- **R17.** ED 7Ch/7Dh/7Fh, 80h–9Fh, A4h–A7h: one word is copied from (HL)
  to (DE); BC−1, DE+2, HL+2; F unchanged.
- **R18.** ED ADh–AFh, B4h–B7h: R17 repeated until BC = 0 (the BC = 0000h
  entry case is unmeasured and mirrors LDIR).
- **R19.** ED BCh–ECh consume one operand byte with no visible effect
  (only operand CFh was exercised); ED FEh/FFh are two-byte NOPs.
- Not emulated (observed effect on the real machine only): ED 54h, 55h,
  ACh and EEh–FCh leave the Z80 emulation mode for the native OS with an
  error message, ED 5Ch hangs the machine, and ED 5Dh corrupted the file
  system once. vaeg keeps the Zilog `Z80_NO_EXCEPTION` fallback for these.

R14 fits the µPD9002's architecture (a V50-family core whose Z80 emulation
mode shares the machine with a native x86 side): the undefined encodings
evidently reach IVT-access microcode. This is an observation about the
read value only; the mechanism is not otherwise verified.

Correspondence with other flag models (observation, not an explanation):

| Rule | Consistent with | Not consistent with |
| --- | --- | --- |
| R1 | V20/V30 PSW, where bits 5 and 3 are fixed 0 | |
| R2 | x86 AND as observed on x86 parts (AF = 0) | 8080 ANA (AC = OR of bit 3 of both operands) |
| R3 | x86 TEST as observed (AF = 0) | |
| R4 | 8080 RLC/RRC/RAL/RAR (only CY written) | |
| R6 | 8080 DAD (only CY written) | x86 16-bit ADD (AF from bit 3) |
| R7 | x86 16-bit ADC/SBB (AF from bit 3) | |
| R8–R10 | 8080 CMA/STC/CMC (only CY written) | |
| R11 | x86 DAA/DAS selected by N, with P/V as x86 OF | Zilog Z80 |

Intel documents AF as undefined after AND/TEST; the R2/R3 entries refer to
observed x86 behavior only. N handling is per instruction rather than
uniform: ADD HL clears N, whereas the accumulator rotates, block loads,
SCF/CCF and DAA leave N unchanged.

## 5. Error CRCs

| # | ZEXDOC expected | ZEXDOC found | ZEXALL expected | ZEXALL found | Rule | Reproduced |
| ---: | --- | --- | --- | --- | --- | --- |
| 1 | OK | OK | `d48ad519` | `38bd187c` | R1, R7 | yes |
| 2 | OK | OK | `d9a4ca05` | `e5cb0e49` | R1, R6 | yes |
| 3 | OK | OK | `b1df8ec0` | `433479fc` | R1, R6 | yes |
| 4 | OK | OK | `39c8589b` | `6a867b69` | R1, R6 | yes |
| 5 | `48799360` | `12967d59` | `51c19c2e` | `12967d59` | R2 | yes / yes |
| 6 | `fe43b016` | `4f797917` | `06c7aa8e` | `4f797917` | R2 | yes / yes |
| 7 | `a4026d5a` | `6d7397f5` | `a886cc44` | `6d7397f5` | R2 | yes / yes |
| 8 | `e849676e` | `4142bbf8` | `d3f2d74a` | `4142bbf8` | R2 | yes / yes |
| 9 | `a8ee0867` | `5a40ec5a` | `83534ee1` | `71fdaadc` | R3 | yes / yes |
| 10 | `7b55e6c8` | `9ae7950f` | `5e020e98` | `30717c8b` | R3 | yes / yes |
| 11 | OK | OK | `134b622d` | `a87e6cfa` | R1 | yes |
| 12 | OK | OK | `2da42d19` | `06deb356` | R1 | yes |
| 13 | `9b4ba675` | `6096b6aa` | `6d2dd213` | `6096b6aa` | R8–R11 | yes / yes |
| 14 | OK | OK | `81fa8100` | `d18815a4` | R1 | yes |
| 15 | OK | OK | `77f35a73` | `5f682264` | R1 | yes |
| 17 | OK | OK | `1af612a7` | `c284554c` | R1 | yes |
| 18 | OK | OK | `d146bf51` | `4523de10` | R1 | yes |
| 20 | OK | OK | `ca8c6ac2` | `e175afcc` | R1 | yes |
| 21 | OK | OK | `560f955e` | `1ced847d` | R1 | yes |
| 25 | OK | OK | `a0a1b49f` | `56cd06f3` | R1 | yes |
| 26 | OK | OK | `28295ece` | `b83adcef` | R1 | yes |
| 28 | OK | OK | `0b95a8ea` | `20581470` | R1 | yes |
| 32 | OK | OK | `36c11e75` | `fbcbba95` | R1 | yes |
| 53 | `94f42769` | `ec734af4` | `94f42769` | `ec734af4` | R5 | yes / yes |
| 54 | `5a907ed4` | `3958c2d0` | `39dd3de1` | `3958c2d0` | R5 | yes / yes |
| 55 | `9abdf6b5` | `e23a9b28` | `f782b0d1` | `e23a9b28` | R5 | yes / yes |
| 56 | `eb59891b` | `49a0684e` | `e9ead0ae` | `49a0684e` | R5 | yes / yes |
| 57 | OK | OK | `d638dd6a` | `6a3c3bbd` | R1 | yes |
| 58 | OK | OK | `ff823e77` | `955ba326` | R1 | yes |
| 59 | `251330ae` | `9ac609b5` | `9ba3807c` | `9ac609b5` | R4 | yes / yes |
| 60 | OK | OK | `710034cb` | `713acd81` | R1 | yes |
| 61 | OK | OK | `a4255833` | `eb604d58` | R1 | yes |

## 6. Coverage limits and interpretation notes

- **C1. ZEXDOC masks hide some flags.**
  - Groups #1–#4 use mask C7h, which hides H. ZEXDOC OK for these groups
    therefore does not certify H; R6 and R7 are visible only in ZEXALL.
  - BIT groups (#9, #10) use mask 53h, which hides S and P/V.
- **C2. #29–#31 are degenerate and do not show correct Y/X.**
  - The operand byte is fixed by the test vector: IXH = C6h, IXL = D4h,
    IXH = 91h respectively.
  - The results (C5h/C7h, D3h/D5h, 90h/92h) have bits 5/3 = 0, so the
    expected CRCs are identical in ZEXDOC and ZEXALL.
  - #32 (operand 9Eh → 9Dh/9Fh, bit 3 = 1) fails, as R1 predicts.
- **C3. IY-half INC/DEC is not exercised** (resolved in M102: `ZEXIY`
  exercises the real FD-prefixed forms; the real output is byte-identical
  to the uPD9002 host reference, §10).
  - The groups labelled `iyh`/`iyl` execute DD-prefixed opcodes
    (DD 24/25, DD 2C/2D), i.e. IXH/IXL.
  - INC/DEC IYH/IYL (FD 24/25/2C/2D) is exercised by neither suite.
- **C4. Storage of F bits 5/3 is not exercised by ZEX** (resolved by the
  M101 probes P7a–P7d, §4 R1).
  - In the test vectors checked (#1–#57), the base F value has bits 5/3
    clear and the F counter/shifter masks are D7h (53h for BIT).
  - F therefore never enters these groups with Y/X set.
  - The OK results of the flag-preserving groups (#16, #19, #22–#24, #27,
    #33–#52) say nothing about whether POP AF can store bits 5/3.
- **C5. A CRC match is evidence only over the exercised state space.**
- **C6. The BIT found CRCs differ between suites, but not because of Y/X.**
  - The ZEXALL mask FFh adds S and P/V, which the ZEXDOC mask 53h excludes,
    so the found CRCs of #9/#10 change even with Y/X = 0.
  - R1 + R3 reproduces both values. The difference therefore does not
    indicate non-zero Y/X after BIT.
- **C7. Identity of found CRCs across suites is informative only where the
  expected CRCs differ.** #53 has identical expected CRCs in both suites (§2).

## 7. Open items and minimal real-machine tests

Status after M102: every item is closed (O3 by `ZEXIY`, §10).

- **O1. Resolve U1.** Closed: `ZEX13S` split the group into single-opcode
  groups, and `DAADUMP` dumped DAA/CPL/SCF/CCF exhaustively; R8–R11.
- **O3. INC/DEC IYH/IYL.** Closed: `ZEXIY` (M102) runs corrected
  single-opcode groups for INC/DEC IXH/IXL/IYH/IYL; the real-machine output
  is byte-identical to the uPD9002 host reference. IYH/IYL behave like
  IXH/IXL (Zilog results plus R1).
- **O4. Full-suite regression.** Closed: the full stock ZEXDOC and ZEXALL
  under the vaeg uPD9002 profile are byte-identical to the M101 real-machine
  outputs (67/67 groups each).
- **O5. Executable identity.** Closed for the M101 re-run: the file-output
  exercisers are built from `external/zex/` plus recorded patches, with the
  hashes in §9. The executables of the first run remain unrecorded, but the
  re-run reproduced every group of that run.
- **O6. Direct confirmation probes.** Closed: `FLAGPRB` ran P0–P7d; every
  probe matched the model prediction below, including P7 (bits 5/3 are not
  storable).
  - Purpose: promote R1–R7 from CRC-reproduced to directly observed.
  - Set F through `PUSH`/`POP AF`, always with bits 5/3 clear, so that the
    inputs do not depend on C4.
  - Save the result with `PUSH AF` (and the relevant register pair)
    immediately after the instruction under test, before any output code
    runs.
  - "Z80" values include bits 5/3.

  | Probe | Setup | Instruction | Z80 result | Model prediction | Tests |
  | --- | --- | --- | --- | --- | --- |
  | P1 | A = FFh | `AND 0Fh` | A 0Fh, F 1Ch | A 0Fh, F 04h | R2, R1 |
  | P2 | A = 01h, F = 00h | `BIT 0,A` | F 10h | F 00h | R3 |
  | P3 | A = 80h, F = D7h | `RLCA` | A 01h, F C5h | A 01h, F D7h | R4 |
  | P4a | A = 00h, (HL) = 00h, BC = 0001h, F = D7h | `LDI` | F C1h | F D3h | R5 |
  | P4b | as P4a, BC = 0002h | `LDI` | F C5h | F D7h | R5 |
  | P5a | HL = 0FFFh, BC = 0001h, F = 00h | `ADD HL,BC` | HL 1000h, F 10h | HL 1000h, F 00h | R6 |
  | P5b | HL = 0001h, BC = 0001h, F = 10h | `ADD HL,BC` | HL 0002h, F 00h | HL 0002h, F 10h | R6 |
  | P6a | HL = 000Fh, BC = 0001h, F = 00h | `ADC HL,BC` | HL 0010h, F 00h | HL 0010h, F 10h | R7 |
  | P6b | HL = 0010h, BC = 0001h, F = 00h | `SBC HL,BC` | HL 000Fh, F 02h | HL 000Fh, F 12h | R7 |
  | P7 | BC = FFFFh | `PUSH BC / POP AF / PUSH AF / POP BC` | C FFh | C D7h if bits 5/3 are not storable (hypothesis) | C4 |

  - P5a and P5b together separate R6 from both the Z80 rule and a bit-3-carry
    rule.
  - P7 should be repeated through `EX AF,AF'`.
- **O7. Emulator policy.** Closed: vaeg implements R1–R11 as the
  `Upd9002` flag profile of the vendored core, used by the uPD70008-
  compatible main-CPU mode; the Zilog behavior remains the default profile.
  The regression gate is the full set of real-machine outputs (§9).

## 8. Reproduction method

- **Binaries.** The reproduction uses the stock `zexdoc.com` / `zexall.com`
  from github.com/agn453/ZEXALL (extracted from YAZE-AG 2.51.3), not the
  modified builds that ran on the machine. Their expected CRCs equal the
  "expected" values in both output files.
- **Reference core and harness.**
  - Reference core: [suzukiplan/z80](https://github.com/suzukiplan/z80) (MIT),
    which passes both suites unmodified.
  - Each rule is implemented as a switch in the core.
  - CP/M BDOS functions 2 and 9 are trapped.
  - Single groups are selected by patching the exerciser's test table.
- **Acceptance criterion.** Exact equality with the reported found CRC.
  Candidates searched and hits per group:

  | Group | Candidates | Hits |
  | --- | ---: | ---: |
  | ALU | 64 | 1 |
  | BIT | 12 | 1 |
  | Accumulator rotates | 4 | 1 |
  | Block loads | 16 | 1 |
  | 16-bit ADD H | 4 | 1 |
  | 16-bit ADC/SBC H | 4 | 1 |

- **DAA group search.**
  - The exerciser CRC is affine over GF(2), so a group CRC decomposes into
    independent per-opcode contributions. The decomposition was validated
    against the emulator, then used for a meet-in-the-middle search over
    about 2.6×10⁸ DAA × CPL × SCF × CCF combinations.
  - DAA variants: add-only, N-selected, or always-subtract adjustment; low
    condition from nibble and/or H; high threshold on the original value
    (>99h, >9Fh) or the adjusted value (>9Fh), with or without C;
    low-step carry, and high check on the original or the updated C;
    H out as x86 AF, Z80, keep, 0, 1 or bit-3 carry; N out as keep, 0 or 1;
    P/V as parity, keep, 0, 1 or several overflow variants.
  - CPL variants: H/N set, keep or clear; S/Z/P kept or updated (parity or 0);
    C kept or cleared.
  - SCF variants: H/N as Z80, keep or set.
  - CCF variants: H as old C, keep, 0, 1 or new C; N as Z80, keep or set.
  - No combination matched. The measured DAA (R11) uses a high threshold
    that depends on H (99h with H = 0, 9Fh with H = 1), which was not among
    the candidates; §9 resolved the group from direct dumps instead.

## 9. M101 real-machine re-run

- Programs: `ZEXDOCF`/`ZEXALLF` (stock exercisers plus file output),
  `ZEX13S`, `FLAGPRB` and `DAADUMP`, built from `external/zex/` and
  `tools/cpmva/zex/` with z80asm 1.8 and run from one generated CP/M disk
  under CP/MVA on a real PC-88VA2 in V3 mode. CP/MVA enters the µPD9002 Z80
  emulation mode with BRKEM, so the measured mode is the same Z80 emulation
  mode that V1/V2 mode uses.
- Evidence: byte-exact outputs, manifests, program and disk hashes, and the
  comparison with the host references are in
  [`docs/agents/reports/m101_zexall_qa/`](../agents/reports/m101_zexall_qa/README.md).
- Results:
  - `ZEXDOC.TXT` and `ZEXALL.TXT` equal the first run in every group,
    including every found CRC. The first run was made in V2 mode and this
    one in V3 mode through BRKEM, so both ways of entering the µPD9002 Z80
    emulation mode give the same flag behavior over the exercised space.
  - `FLAGPRB`: every probe matches the uPD9002 prediction (R1–R7, P7).
  - `ZEX13S`: controls `aluop a,nn` = `12967d59` and `<daa,cpl,scf,ccf>` =
    `6096b6aa`; the single-opcode groups gave `c5f0d7a8` (DAA), `a34147ce`
    (CPL), `b9e9525a` (SCF) and `a5000c57` (CCF), identical with flag masks
    D7h and FFh.
  - `DAADUMP`: the four dumps determine R8–R11.
- With R1–R11 the vaeg uPD9002 profile reproduces all nine output files byte
  for byte, on the host (`tools/cpmva/zex/host_reference.py`) and in the
  test `vaeg_z80_compat_flag_profile`, which checks every dump record.

## 10. M102: undocumented opcodes and undefined ED semantics

- Programs: `ZEXIY`, `ZEXUND`, `ZEXED` (derived exercisers) and the probes
  `EDPRB`, `CBPRB`, `EDPRB2`/`EDPRB2S`, `EDPRB3`, `EDPRB4` and `INPRB`,
  run on a real PC-88VA2 in V3 mode under CP/MVA (µPD9002 Z80 emulation
  mode through BRKEM) over eight disk runs.
- Evidence: byte-exact outputs, disk and program hashes, crash notes and
  comparisons are in
  [`docs/agents/reports/m102_zexund_qa/`](../agents/reports/m102_zexund_qa/README.md).
- Method: `CBPRB` measured every DD/FD CB opcode with two input sets
  (R12/R13). The undefined ED opcodes resisted the exerciser approach —
  `ZEXED` hung and `EDPRB`'s unsandboxed pointers let the unknown word
  transfers corrupt the probe itself — so `EDPRB2` re-measured every
  opcode with sandboxed pointers, three input sets and an RST-sled length
  detector; `EDPRB3` varied one input at a time, which reduced the
  ED 00–3F value to a function of C alone; `INPRB` refuted an I/O-read
  explanation with documented IN instructions; and `EDPRB4` swept all 256
  C values, reconstructing the hidden page consistently from 512
  overlapping byte observations as the live x86 interrupt vector table
  (R14). The RRD/RLD variants, the word transfers and the three-byte
  no-operation forms (R15–R19) come from the same records.
- Emulation: the vendored core implements R12–R19 under
  `FlagProfile::Upd9002`; the undefined-ED vector read reaches the guest's
  physical page 0 through `Z80CompatCpu::SetNativeVectorRead`, wired by
  the uPD70008-compatible main-CPU adapter. The CP/M runner carries the
  run-8 IVT page as a fixture, and the uPD9002 host references reproduce
  all 911 real-machine probe records byte for byte (EDPRB2: 513,
  EDPRB2S: 27, EDPRB3: 28, EDPRB4: 256, EDPRB run 2: 67).
- Acceptance notes: under the uPD9002 profile `ZEXED`, `ZEXUND` and
  `EDPRB` are partial by design, in the same way and for the same reasons
  as on the real machine (word transfers make `ZEXED` effectively
  unfinishable, R15 makes `ZEXUND`'s NEG-duplicate group return to CP/M,
  and `EDPRB` corrupts itself); `ZEXIY` and `CBPRB` complete and match
  byte for byte.
