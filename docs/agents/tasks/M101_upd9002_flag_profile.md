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

# M101 - µPD9002 Z80-emulation-mode flag profile and real-machine probes

Status: **G101 human gate passed on 2026-09-30; M101 complete**

Predecessor: `main` at `6fcb2992593ae8da287fa92fd77f62dec4d67d1b`
(M100 ZEX report).

Branch: `topic/m101-upd9002-flag-profile`

Commit prefix: `M101:`

Evidence document: `docs/modernization/uPD9002-zex-results.md`

## Background

The NEC µPD9002 (PC-88VA) has a Z80 emulation mode used in V1/V2 mode. In
vaeg it is the uPD70008-compatible main-CPU mode (`cpu/upd9002_upd70008.*`).
ZEXDOC and ZEXALL were run on a real PC-88VA2 in V2 mode, using builds of the
exercisers modified to write their console output to a file (`ZEXDOC.TXT`,
`ZEXALL.TXT`). The found CRCs were reproduced exactly (32 bits) by a Z80 core
with seven flag rules:

- R1: F bits 5/3 (Y/X) are always 0.
- R2: AND sets H = 0.
- R3: BIT n,r / (HL) / (IX+d) / (IY+d) sets H = 0.
- R4: RLCA/RRCA/RLA/RRA preserve H and N.
- R5: LDI/LDD/LDIR/LDDR preserve H and N.
- R6: ADD HL/IX/IY,rr preserves H.
- R7: ADC/SBC HL,rr take H from the carry/borrow out of bit 3.

Unresolved (U1): the `<daa,cpl,scf,ccf>` group (found CRC `6096b6aa` in both
suites) is not reproduced by any model tried.

Open question for R1: whether F can store bits 5/3 at all (e.g. via POP AF)
is not exercised by ZEX.

The modified exerciser source and executables used for the first real-machine
run were not preserved (report O5). M101 therefore rebuilds file-output
exercisers from recorded sources, and the real-machine ZEX runs are repeated
with them.

## Decisions

- **Profile names.** The suzukiplan core gains a per-instance
  `Z80::FlagProfile` with exactly two values:
  - `Zilog`: the default. Documented and undocumented Zilog Z80 behaviour.
    This is the existing behaviour and is used by the FDC CPU (`UPD780C`).
  - `Upd9002`: the R1–R7 profile of the µPD9002 Z80 emulation mode. It uses
    the storage variant of R1 and keeps Zilog behaviour for DAA/CPL/SCF/CCF
    until U1 is resolved. It is used by the uPD70008-compatible main-CPU
    adapter.

  The default was first named `Upd780` and renamed on 2026-09-29 after
  upstream review: NEC documentation calls the µPD9002 Z80 mode itself
  "µPD780 mode", and nothing shows that the FDC's µPD780C matches the Zilog
  behaviour. MAME 0.250 (`src/devices/cpu/z80/z80.cpp`) records SCF/CCF X/Y
  as `(F|A) & 28h` on SGS/Sharp/Zilog NMOS parts and `F & A & 28h` on NEC
  NMOS parts, and calls the exact NEC behaviour unknown. Running the FDC
  µPD780C with the Zilog profile is therefore an unverified assumption; the
  impact is probably small.
- **Core patches.** The two reviewed patches are applied to the vendored
  suzukiplan tree through the M36 provenance procedure, never by hand:
  1. `0001-fix-adc-sbc-hl-carry.patch`: ADC/SBC HL,rr folded the carry into
     the 16-bit operand before computing flags. H is wrong whenever
     `rr & 0FFFh = 0FFFh` and C = 1, P/V whenever `rr & 7FFFh = 7FFFh` and
     C = 1, and C as well when rr = FFFFh. The carry is now passed
     separately.
  2. `0002-add-upd9002-flag-profile.patch`.

  Upstream pull requests: suzukiplan/z80
  [#60](https://github.com/suzukiplan/z80/pull/60) (carry fix and its test)
  and [#61](https://github.com/suzukiplan/z80/pull/61) (flag profile).
- **ZEX sources are imported** under `external/zex/` (GPL-2.0-or-later
  sources with the upstream bundled license). They are never compiled into,
  linked into, or packaged with a vaeg executable or release archive. They
  are used only by the user-run CP/MVA disk installer and by host tests. The
  release-archive audit keeps rejecting every ZEX name and hash; the source
  archive audit accepts exactly the recorded `external/zex/` files. This
  amends the ADR-0011 acquisition policy and is recorded in a new ADR.
- **Assembler.** All CP/M programs are built with the lock-pinned z80asm 1.8
  used by `tools/cpmva`. The ZMAC-dialect ZEX sources are converted by a
  deterministic vaeg translator. Acceptance: the translated stock sources
  assemble byte-identically to the hash-pinned stock `zexdoc.cim` and
  `zexall.cim`.
- **GPL boundary for derived programs.** File-output exercisers and `ZEX13S`
  are derived from ZEX and are GPL-2.0-or-later. Their modifications are kept
  as patches next to the translator and are marked as such. `FLAGPRB` and
  `DAADUMP` are independent vaeg programs under BSD-2-Clause.

## General constraints for the CP/M programs

- Code, comments and commit messages in English.
- Target is CP/M 2.2 BDOS as seen in PC-88VA2 V2 mode. Use only BDOS
  functions 2, 9, 13–16, 19, 21, 22 and 26. File names are 8.3 upper case.
- **No self-modifying code in the new probe programs.** The µPD9002 is
  V30-based and has an instruction prefetch queue. Use separate code paths
  per instruction instead. The ZEX-derived programs keep ZEX's own iut
  mechanism, which is known to work on this machine.
- Set F only through `PUSH rr / POP AF`. Every input F value must have
  bits 5/3 clear, except in P7, which tests storage itself.
- Capture results with `PUSH AF`, and store any other registers, immediately
  after the instruction under test, before any other code runs.
- Control flow in the new programs may branch only on Z, C or S produced by
  INC/DEC/CP/OR/XOR. These are verified to match the Z80 on this machine.
  Never branch on H/N/P, or on flags produced by the instructions under test.
- Disable interrupts around each instruction under test and its capture:
  `DI ... EI` per probe, not for the whole run.
- Output files must be byte-identical in format between the host run and the
  real-machine run.
- Do not change the Zilog profile behaviour of the core.
- Do not guess hardware behaviour. When something is ambiguous, stop and
  report.

## Stages

Each stage is one or more single-concern commits. Stages A–D are completed
before the first real-machine hand-over; stage E waits for real-machine
outputs.

### Stage A: import ZEX and rebuild it with z80asm

1. Import `zexdoc.src`, `zexall.src` and the bundled `LICENSE.txt` from
   suzukiplan/z80 `test-ex` at the ADR-0011 base commit into `external/zex/`
   byte-for-byte, with a `provenance.txt` recording URLs and SHA-256.
2. Add the ADR amending the ADR-0011 acquisition policy. Update
   `tests/z80_compat/check_zex_archive.py` with separate source-archive and
   release-archive modes, and use the source mode only for the `git archive`
   check in CI. The release mode stays the default and remains strict.
3. Add the translator and a verifier that rebuilds both stock exercisers
   with z80asm 1.8 and compares them with the hash-pinned `.cim` files.

### Stage B: file-output exercisers and the CP/MVA tools disk

1. `ZEXDOCF.COM` and `ZEXALLF.COM`: stock exercisers that also write every
   console byte to `ZEXDOC.TXT` / `ZEXALL.TXT` (BDOS 19, 22, 26, 21, 16;
   128-byte records, final record padded with 1Ah). `msbt` must not move.
   The test table, descriptors, and CRC code are unchanged.
2. A host CP/M runner built on the vendored core: BDOS 2, 9, 13–16, 19, 21,
   22 and 26 mapped to a host directory, plus `--profile zilog|upd9002`
   once stage C is in place.
3. Host acceptance: the file content equals the console output, and the
   stock expected CRCs pass 67/67 under Zilog.
4. `tools/cpmva/install_cpmva.py` puts the stock `ZEXDOC.COM` and
   `ZEXALL.COM`, the file-output variants, and the stage D programs on
   `cpmva-tools.d88`, building them from `external/zex/` and in-tree probe
   sources with the pinned z80asm. The build manifest records their SHA-256.
   Because the tools disk has too little free space for the DAADUMP output,
   the installer also writes `cpmva-zexall-test.d88`, which holds only
   these programs (maintainer decision, 2026-09-29; first named
   `cpmva-test.d88` and renamed the same day for clarity).

### Stage C: core profile and emulator integration

1. Record the two core patches under `docs/agents/reports/`, reproduce the
   vendored tree from the approved base plus the M35 patch plus these
   patches, and update `external/suzukiplan-z80/provenance.txt` and
   ADR-0011.
2. Add the profile to `Z80CompatCpu`. The uPD70008 adapter selects
   `Upd9002`; `UPD780C` keeps `Zilog`. Adapter paths that write F outside
   the core (LD A,I / LD A,R materialization, register import, `SetReg`,
   `SetMainReg`) must honour R1.
3. Regression: the stock suites pass 67/67 under `Zilog`, and under
   `Upd9002` every group except #13 matches the real-machine outcome and
   found CRC in the evidence document.
4. Record the ADC/SBC HL defect and the µPD9002 flag correction in
   `docs/modernization/bug-fixes.md`.

### Stage D: real-machine probe programs

#### `FLAGPRB.COM` → `FLAGPRB.TXT`

Run each probe below, and write one line per probe in this format:

```
<id> IN A=xx F=xx BC=xxxx DE=xxxx HL=xxxx OUT A=xx F=xx BC=xxxx DE=xxxx HL=xxxx [MEM=xx]
```

For LDI, `MEM` is the destination byte. After the probe lines, write the
machine-independent expected values as comment lines starting with `#`.
Echo everything to the console as well.

| id | Setup | Instruction(s) under test | Zilog | µPD9002 |
| --- | --- | --- | --- | --- |
| P0 | A=5Ah, F=00h | `XOR A` (control) | A 00h F 44h | A 00h F 44h |
| P1 | A=FFh, F=00h | `AND 0Fh` | A 0Fh F 1Ch | A 0Fh F 04h |
| P2 | A=01h, F=00h | `BIT 0,A` | F 10h | F 00h |
| P3 | A=80h, F=D7h | `RLCA` | A 01h F C5h | A 01h F D7h |
| P4a | A=00h, (HL)=00h, HL=src, DE=dst, BC=0001h, F=D7h | `LDI` | F C1h, BC 0000h | F D3h, BC 0000h |
| P4b | as P4a, BC=0002h | `LDI` | F C5h, BC 0001h | F D7h, BC 0001h |
| P5a | HL=0FFFh, BC=0001h, F=00h | `ADD HL,BC` | HL 1000h F 10h | HL 1000h F 00h |
| P5b | HL=0001h, BC=0001h, F=10h | `ADD HL,BC` | HL 0002h F 00h | HL 0002h F 10h |
| P6a | HL=000Fh, BC=0001h, F=00h | `ADC HL,BC` | HL 0010h F 00h | HL 0010h F 10h |
| P6b | HL=0010h, BC=0001h, F=00h | `SBC HL,BC` | HL 000Fh F 02h | HL 000Fh F 12h |
| P7a | BC=FFFFh | `PUSH BC / POP AF / PUSH AF / POP BC` | C FFh | C D7h |
| P7b | BC=FFFFh | `PUSH BC / POP AF / EX AF,AF' / EX AF,AF' / PUSH AF / POP BC` | C FFh | C D7h |
| P7c | BC=0028h | as P7a | C 28h | C 00h |
| P7d | BC=FFFFh | `PUSH BC / POP AF / INC DE / PUSH AF / POP BC` | C FFh | C D7h |

- The µPD9002 values for P7 are the prediction of the R1 storage variant.
  The alternative, the computation variant, predicts the Zilog values. P7
  decides which variant is correct.
- The table values were derived by hand. The host runs of both profiles must
  reproduce them exactly. If they do not, stop and report the discrepancy;
  do not edit the table to match.

#### `ZEX13S.COM` → `ZEX13S.TXT`

Derived from the file-output ZEXDOC. Test table, in this order:

1. `aluop a,nn` — unchanged. Build control; the real machine is known to give
   `12967d59`.
2. `<daa,cpl,scf,ccf>` — unchanged. Build control; the real machine is known
   to give `6096b6aa`.
3. Eight new groups: DAA, CPL, SCF and CCF, each once with flag mask D7h and
   once with FFh.
   - Copy the original `<daa,cpl,scf,ccf>` descriptor.
   - Set the opcode byte to 27h, 2Fh, 37h or 3Fh.
   - Set the instruction-byte counter mask to 00h.
   - Keep every other base, counter and shifter field unchanged.
   - Messages, for example: `daa (d7)`, `daa (ff)`, `cpl (d7)`, …
   - Expected CRCs are those of the Zilog profile, computed on the host.

Layout requirements:

- Do not move `msbt`. Keep the file-output routine unchanged.
- Verify on the host that the two control groups give their stock ZEXDOC
  expected CRCs under Zilog: `aluop a,nn` → `48799360`,
  `<daa,cpl,scf,ccf>` → `9b4ba675`.
- Verify under µPD9002 that `aluop a,nn` gives `12967d59`.

#### `DAADUMP.COM` → `DAA.BIN`, `CPL.BIN`, `SCF.BIN`, `CCF.BIN`, `DAADUMP.TXT`

An exhaustive dump. It fully determines U1 and supersedes the CRC split for
diagnosis.

- For each instruction in {DAA, CPL, SCF, CCF}, use a separate unrolled code
  path per instruction:
  - Loop `fi` from 0 to 63. Map `fi` to `F_in` by spreading its 6 bits over
    F bits 7, 6, 4, 2, 1, 0 (S, Z, H, P/V, N, C), so bits 5/3 stay clear.
  - Loop `a` from 00h to FFh.
  - Set A=`a`, F=`F_in`, execute the instruction, capture AF, and append the
    two bytes `A_out, F_out`.
- Record order is `offset = (fi*256 + a)*2`. Each file is exactly 32768 bytes.
- Stream records through 128-byte sequential writes rather than buffering the
  whole file.
- `DAADUMP.TXT` contains one line per file: name, size, and a CRC-32 (the
  standard reflected polynomial EDB88320h, init FFFFFFFFh, final XOR
  FFFFFFFFh). Print the same lines on the console so they can be checked
  against a photograph.

#### Host side

1. Build all CP/M programs and record their SHA-256 in `MANIFEST.TXT`.
2. Run each program under both profiles. Store the outputs as
   `ref/zilog/*` and `ref/upd9002/*`.
3. Write `compare.py <real_dir>`. It compares the real-machine outputs with
   both references and prints the following.
   - `FLAGPRB`: per probe, the verdict ZILOG, UPD9002, BOTH or NEITHER, plus
     the differing bits.
   - `ZEX13S`: per group, the found CRC and whether it equals the Zilog
     expected value, the µPD9002 host value, or neither. For the two control
     groups, whether the real machine reproduced `12967d59` and `6096b6aa`.
   - `ZEXDOC.TXT` / `ZEXALL.TXT`: per group, whether the real result equals
     the µPD9002 host result and the first-run evidence.
   - Dumps, per instruction:
     - the number of mismatching records against Zilog;
     - a breakdown by output field (A, and each F bit S, Z, Y, H, X, P/V, N,
       C);
     - a breakdown by input class (H_in, N_in, C_in) and by A nibble range;
     - the first 20 mismatching records in full.
   - A check that the real `DAADUMP.TXT` CRC-32 values match the real `.BIN`
     files, to detect disk or transfer corruption.
4. Acceptance before handing the programs over for the real-machine run:
   - The host Zilog run of `FLAGPRB` matches the Zilog column exactly, and
     the host µPD9002 run matches the µPD9002 column exactly.
   - `ZEX13S` under Zilog reports OK for all ten groups.
   - All ZEXDOC and ZEXALL regressions that already pass still pass.

### Stage E: real-machine results (only after the maintainer supplies them)

1. Run `compare.py` and report the results verbatim.
2. R1 variant.
   - If P7 matches µPD9002, keep the storage variant.
   - If it matches Zilog, switch R1 to the computation variant: remove the
     masking at POP AF, `setAF`/`setAF2` and reset, and keep `setFlagX`/
     `setFlagY` disabled.
   - Then re-run everything.
3. U1.
   - Derive rules for DAA/CPL/SCF/CCF only from the dumps.
   - Implement them in the µPD9002 profile, each with a rule id and comment.
   - Acceptance:
     - All four host dumps are byte-identical to the real dumps.
     - The found CRC of `<daa,cpl,scf,ccf>` is `6096b6aa` in both stock
       ZEXDOC and stock ZEXALL.
     - Full stock ZEXDOC and ZEXALL under µPD9002 match the real machine in
       all 67 groups.
     - The Zilog profile still passes 67/67 in both suites.
   - If the dumps do not determine a rule unambiguously, stop and report
     rather than choosing one.
4. Update `docs/modernization/uPD9002-zex-results.md`.
   - Record the evidence status of each rule as directly observed or
     CRC-reproduced.
   - Resolve U1 and O6/P7 there, and close O5 with the recorded exerciser
     sources and hashes.

## Machine checks

- `python3 tools/repo/*.py` checks listed in `CONVENTIONS.md`.
- `cmake --preset linux-debug` build and `ctest`, including the standalone
  `vaeg_z80_compat_*` conformance targets with `VAEG_ZEX_ARTIFACT_DIR`.
- ZEX byte-identity verification and host CP/M runs from stages A–D.
- `check_zex_archive.py` in source mode on `git archive HEAD` and in release
  mode on a locally staged package.

## Gate

G101 is a human gate:

1. The standard gate: build from a clean checkout, boot in V3 mode, run the
   bundled VA demo, boot an OS and perform simple operations.
2. Generate the CP/MVA disks with the installer, boot CP/MVA, and confirm
   that the new programs are listed by `DIR` on `cpmva-tools.d88` and
   `cpmva-zexall-test.d88`.
3. Real-machine runs of `ZEXDOCF`, `ZEXALLF`, `FLAGPRB`, `ZEX13S` and
   `DAADUMP` on a PC-88VA2 in V2 mode, followed by stage E.

### G101 progress

- 2026-09-29, items 1 and 2 passed (maintainer).
  - Build: fully static MinGW `vaeg.exe` from a clean clone of
    `c470a80e8431982b35d94af67d33de14fa96ec76` via
    `tools/release/build-mingw-static-container.sh`; SHA-256
    `9f306e2c5396b4e7d4328b9326c25971595331a5249ae00b40a393f4e43f77ba`.
  - V3 boot, the bundled VA demo, PC-Engine and FreeDOS boot with simple
    operations: OK.
  - CP/MVA disks generated by the installer from a user-owned PC-Engine boot
    disk (unchanged by the installer). CP/MVA reached the CP/M prompt and
    `DIR` on the test disk (then named `cpmva-test.d88`) listed all seven
    M101 programs. Stray
    characters from the PC-Engine function-key line afterwards were judged
    harmless by the maintainer.
- 2026-09-29, emulator run (supplementary to item 2): the maintainer ran
  `FLAGPRB`, `ZEX13S`, `DAADUMP`, `ZEXDOCF` and `ZEXALLF` from
  the test disk in the same `vaeg.exe`. All nine output files
  (`FLAGPRB.TXT`, `ZEX13S.TXT`, `DAA.BIN`, `CPL.BIN`, `SCF.BIN`, `CCF.BIN`,
  `DAADUMP.TXT`, `ZEXDOC.TXT`, `ZEXALL.TXT`) are byte-identical to the host
  `host_reference.py` uPD9002 outputs of the same programs, so the main CPU
  runs the uPD9002 profile end to end.
- 2026-09-30, item 3: the maintainer ran the five programs from
  `cpmva-zexall-test.d88` on a real PC-88VA2 in V3 mode under CP/MVA
  (µPD9002 Z80 emulation mode entered with BRKEM) and returned the
  disk read back with a KryoFlux (D88 converted with the HxC Floppy
  Emulator software). Outputs, hashes and comparisons are
  in `docs/agents/reports/m101_zexall_qa/`.

- 2026-09-30, gate decision: G101 passed (maintainer), confirmed with the
  fully static MinGW `vaeg.exe` built from a clean clone of
  `e3f63d7d4c4a86558f9292a938a7a62fbb85853b` (SHA-256
  `e0159b28ed9b207105f702fc1ca24f13e1816fb6e3d114a01a7950bc4fd96b7f`).
  Later branch commits change documentation only.

### Stage E results

1. `compare.py` (report in `docs/agents/reports/m101_zexall_qa/pc88va2/compare.txt`):
   `FLAGPRB` matches uPD9002 in every probe; `ZEXDOC.TXT`/`ZEXALL.TXT` match
   the uPD9002 expectation in every group except #13; both `ZEX13S` controls
   reproduced; the DAA/CPL/SCF/CCF dumps matched neither profile.
2. R1: P7a–P7d match uPD9002, so the storage variant stays.
3. U1: the dumps determine R8 (CPL), R9 (SCF), R10 (CCF) and R11 (DAA)
   unambiguously; each rule reproduces all 16,384 records of its dump.
   Implemented in the vendored core (`m101_suzukiplan_daa_cpl_scf_ccf.patch`,
   also added to upstream PR #61). Acceptance:
   - the four host dumps are byte-identical to the real dumps;
   - `<daa,cpl,scf,ccf>` gives `6096b6aa` in stock ZEXDOC and ZEXALL;
   - full ZEXDOC and ZEXALL under uPD9002 are byte-identical to the real
     outputs (67/67 groups each);
   - the Zilog profile still passes 67/67 in both suites.
4. `docs/modernization/uPD9002-zex-results.md` records R1–R11 as directly
   observed, resolves U1 and P7, and closes O1, O4, O5, O6 and O7; O3
   (INC/DEC IYH/IYL) stays open.
