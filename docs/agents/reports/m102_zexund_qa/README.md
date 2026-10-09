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
# M102 undocumented-opcode QA outputs

Outputs of the M102 CP/M programs run from a generated
`cpmva-zexall-test.d88` under CP/MVA, in vaeg and on a real NEC PC-88VA2 in V3
mode (the µPD9002 Z80 emulation mode entered with BRKEM). The layout follows
[`m101_zexall_qa`](../m101_zexall_qa/README.md): `raw/` holds byte-exact
output files (lower-cased names, binary in `.gitattributes`),
`manifest.sha256` their SHA-256, and `compare.txt` the full output of
`tools/cpmva/zex/compare.py raw --ref <ref>`; sections without a file are
reported as `SKIP`.

Files were extracted with `tools/cpmva/zex/extract_d88.py --lowercase
--skip-com`. The real-machine disks were written and read with a KryoFlux
and converted to D88 with the HxC Floppy Emulator software. The PC-Engine
boot disk is private and not identified.

## Run 1: `ZEXIY`, `ZEXUND`, `ZEXED`

- Disk before the run (installer at `5f100c92c0855c06e90771cf0cf821044e13f0d8`):
  `2a19f6f73036a81a45a746f0d305d74aec8faec6793f1f27a81725016e5b5581`.

### `run1_vaeg/`

- vaeg: fully static MinGW `vaeg.exe` from `5f100c92`, SHA-256
  `03dea3f0469cf13c43f13d3790e45ad3e9f9bf3b32f31bf260692bd21f3a07c6`.
- Disk after the run:
  `144e0a8f957359f7908a2b3638cd84cad8a08607ec8e7171eef1e1dae6e0ee62`.
- `compare.txt` uses the host reference of `5f100c92`: all three files are
  byte-identical to its uPD9002 outputs.

### `run1_pc88va2/`

- Disk after the run:
  `432d4d3b750ed31db93e0bdc46baaa73e5cd50d9e630a5efffb8b5c5776e6338`. The
  medium was faulty: cylinder 7 could not be written, so the maintainer
  filled it with `BAD2A2E.SYS` and copied `ZEXUND.COM` elsewhere (the copy
  that ran is identical to the generated program), and three sectors in
  unallocated blocks 122, 126 and 136 read with CRC errors. Extraction used
  `--allow-unused-errors`.
- Only `ZEXIY.TXT` was closed. It is byte-identical to the uPD9002 host
  output, which resolves O3 (IYH/IYL behave like IXH/IXL: Zilog plus R1).
- `ZEXUND` and `ZEXED` did not close their files. Their results, transcribed
  from a photograph of the screen:

  | Program | Group | Result |
  | --- | --- | --- |
  | `ZEXUND` | `rot (ix,iy+d)->r (d7)` | OK |
  | | `rot (ix,iy+d)->r (ff)` | expected `15ef6011`, found `c3095ab8` |
  | | `bit n,(ix,iy+d) all (d7)` and `(ff)` | expected `05dd9c19`, found `8930d9be` |
  | | `res,set (ix,iy+d)->r (d7)` and `(ff)` | expected `30bdcc4c`, found `59e0b8dc` |
  | | `neg all encodings (d7)` | no result; the program returned to the CP/M prompt by itself |
  | `ZEXED` | `ed 00-3f nop (d7)` and `(ff)` | expected `69eff883`, found `5c757184` |
  | | `ed 77,7f nop (d7)` and `(ff)` | expected `04bc3bb4`, found `1b78e549` |
  | | `ed 80-9f nop (d7)` and `(ff)` | expected `b44cca1b`, found `735d94ec` |
  | | `ed a4-bf holes nop (d7)` | the machine hung |

  With R12/R13 (DD/FD CB register forms) the vaeg uPD9002 profile gives
  exactly the transcribed `rot`, `bit` and `res,set` CRCs.

## Run 2: `CBPRB`, `EDPRB`

- Disk before the run (installer at `e1d34eeede0aba201e9a28a88e5b2d04c70f48fb`):
  `03fb57c2ac174cbd4f9b8cf11497c5b73309b5a0f6fe8975ad8c397bbfbf2f79`.

### `run2_vaeg/`

- vaeg: fully static MinGW `vaeg.exe` from `e1d34eee`, SHA-256
  `4a3f8408ed654f0208d303587198fd3405cf6ed0db169c4533a017b5d5879331`.
- Disk after the run:
  `7b8b1bc201ab4fe49b929fb0db8cb1b843064a768fd6acd94e2f62decba383d3`.
- `compare.txt` uses the host reference of `e1d34eee`: both files are
  byte-identical to its uPD9002 outputs.

### `run2_pc88va2/`

- Disk after the run:
  `4997e106f8f0aa913b7404f99b3f34a78f58b0779f46df761548afdefa6f44e5`
  (no sector errors).
- `CBPRB.TXT` is complete (1,024 lines). It is byte-identical to the uPD9002
  host output of `f3baa4dd4c2ab9a00a12861910e91b4946d5f943`, which includes
  R12/R13.
- `ED00.TXT` holds 67 records: the controls ED 44 and ED 45, ED 00–3F and
  ED 4C. The screen then showed `ED 54 Unknown error is detect`, and CP/MVA
  returned to the PC-Engine prompt (photograph); the remaining opcodes were
  not run.

## Run 3: `EDPRB` restarts

Continuation of run 2 on the same disk: after each stop the maintainer
rebooted and ran `EDPRB xx` with the next opcode. Disk after the runs
(read 2026-10-01, no sector errors):
`0997ae95bc58bf1b32cd20413081189ab352b7fd8fddee1025e5ae5b224903f1`.
Directory entry 37 was corrupted (user area 07h, unprintable name,
raw `07100074078b5f020bdb74128b0042` + zeros), presumably written while a
probe crashed; extraction used `--drop-entry 37`. `cbprb.txt` and
`ed00.txt` are byte-identical to the run-2 copies.

### `run3_pc88va2/`

- `ed5e.txt`, `ed60.txt` and `ed70.txt` overlap; their shared records are
  identical. Non-empty files: 00, 5e, 60, 70, ad, b5, b6, fd.
- An empty `edxx.txt` only shows that the run was started; the file is
  created before the first probe, so it does not by itself prove that
  opcode xx crashed.
- Terminations, transcribed from the maintainer's notes (details that the
  notes did not record are marked unknown):

  | Start/opcode | Observation |
  | --- | --- |
  | 54, 55 | left CP/MVA for the PC-Engine prompt (run 2 and notes) |
  | starts 56–5B | `EDPRB` itself skips to 5C |
  | 5C | machine hung |
  | 5D | `Bdos Err On A: R/O`, no record written |
  | AC | left CP/MVA (notes say "exit"; exact screen not recorded) |
  | AE, B4 | returned to the `a>` prompt at once |
  | AF, B5 | machine hung (B5 closed one record first) |
  | B7 | `Bdos Err On A: R/O` after the B6 record |
  | BC, C3–CB, CD, CE, D0–DF, E0–EC | printed its own and the next probe's header, then returned to `a>` |
  | EE–FC | left CP/MVA for the PC-Engine prompt |
  | C0–C2, CC, CF, BD–BF | not recorded |

- The run-3 analysis found a probe-design flaw: `EDPRB` sets BC=0B0Ch and
  DE=0D0Eh, which point into the program's own code, so records of opcodes
  that write through those registers can reflect self-modification. `EDPRB2`
  (run 4) supersedes it with sandboxed pointers.

## Run 4: `EDPRB2`, `EDPRB2S`

- Disk before the run (installer at `0dbce3c71c84c1c8ae72ce596bfc5800e8a2a544`):
  `40f8292c7e1961982a2c32addc81c0452f553502be82d3a10bf6a3945f454c94`.
  The write to the physical medium never verified completely; the last
  attempt differed on cylinders 9 and 15, side 0.

### `run4_pc88va2/`

- `EDPRB2S` completed: `s200.txt` holds all 27 records.
- `EDPRB2` stopped at `ED 5C 0`; a key press returned to the Ready prompt.
  After a reboot, CP/MVA reported `Bdos Err On A: Bad Sector` when the test
  disk was logged in, and the run was aborted — consistent with the
  unverified write, so the stop at 5C matches run 3 but the aborted rest of
  the run is not evidence about the opcodes.
- Disk after the run (read 2026-10-02, all 1280 sectors good):
  `c5309cbc469067fdc96c5388029e1492d390d39fd34698ba336cabed23fb9778`.
- `e200.txt` holds 201 records: ED 44, 45, 00–3F and 4C, each with input
  sets 0–2.
- First findings (see `compare.txt`; the vaeg profiles implement none of
  this yet, so real records differ from both references):
  - ED 00–3F write an input-dependent value to the pair selected by bits
    5–4 (set 0: 2800h, set 1: 232Ah, set 2: A426h) and set F to 04h/00h/80h.
    `EDPRB2S` reproduces the same values with the code moved by 100h bytes,
    so the value is not an address of the program; it also does not depend
    only on A and F, because run 3 got 2329h from the same A and F with
    other pointers and buffer contents.
  - ED 64/6C change A and (HL) exactly like RRD/RLD but also change (HL+1).
  - ED 80 copies two bytes from (HL) to (DE) with BC−1, DE+2, HL+2,
    confirming the word-transfer reading of run 3.
  - `R=` return addresses equal the first sled byte (`L=0000`): these
    opcodes consumed no operand bytes from the sled.

## Run 5: `EDPRB2 64` completion

- Disk read 2026-10-02 09:22, all 1280 sectors good:
  `2cec252e0ade8371e3d8475f77721796edf7869919970077c6ec19ca5bfe4c12`.
- Besides the new files the disk carries byte-identical copies of files from
  earlier runs (`ed00.txt` = run 2; `ed5e.txt`, `edae.txt`, `edaf.txt`,
  `edb0.txt`, `edb5.txt` = run 3; `e200.txt`, `s200.txt` = run 4; `edad.txt`
  is empty here but had records in run 3). The maintainer does not remember
  how the medium was prepared from the earlier disks, and before `EDPRB2 64`
  he mistakenly ran the old `EDPRB` again with starts around 5C–AE and
  B0–B5, which recreated those `ed??.txt` files (`edb5.txt` closed one
  record again; `edad.txt` stayed empty).
- `e25d.txt` is empty: a run started at ED 5D wrote no record, consistent
  with the run-3 `Bdos Err On A: R/O` stop at 5D.
- `e264.txt` is complete: `EDPRB2 64` ran every remaining probed opcode,
  312 records (104 opcodes x 3 input sets) through ED FF. Every record exits
  through the RST sled; none of the crashes seen with `EDPRB` recurred,
  consistent with those crashes being self-corruption through unsandboxed
  BC/DE.
- Observed behavior (all tentative until implemented and re-verified; the
  only tested operand byte for the 3-byte forms is CFh):
  - ED 74/75/77 equal the ED 30–37 family with SP as the pair; same
    input-dependent values 2800h/232Ah/A426h and F = S/Z/P of the high byte
    with H=N=C cleared — the flag shape of IN r,(C). The value source is
    still unidentified; the only inputs that changed it so far are C and the
    buffer contents (I/O-read hypothesis untested).
  - ED 64/65: RRD on A/(HL), and additionally the low nibble of (HL+1) is
    replaced by the old low nibble of (HL). F = parity of A, all other
    flags cleared, in all six observations.
  - ED 6C/6D: RLD on A/(HL), and (HL+1) is replaced by
    old A.low : old (HL).high. F as for 64/65.
  - ED 7C/7D/7F, 80–9F, A4–A7: one word copied from (HL) to (DE);
    BC−1, DE+2, HL+2; F unchanged.
  - ED AD–AF, B4–B7: repeated word copy until BC=0 (DE and HL advance
    by 2×BC); F unchanged.
  - ED BC–BF, C0–EC: consume one operand byte (L=0001) with no visible
    register, flag or sandbox-memory change.
  - ED FE, FF: two bytes, no visible change.

## Run 6: `INPRB`

- Disk before the run (installer at `6c98d615`, media byte normalized to 2D
  by hand for writing):
  `e20f00612cd2a99150d1c0e55455711b19ab25f6ff4a9709c2e7db45f85ea18e`.
- Disk after the run (all sectors good):
  `aaf96261a6e5d190d2122b2c10aca5732d54cdbfeead1569223382260762c329`.
- `inprb.txt` holds the documented I/O reads: ports 0Ch/0Dh/50h/51h/81h read
  FFh, 30h reads DBh, 31h reads F9h, 80h reads 01h, with `IN A,(n)` and with
  `IN A,(C)` at the exact BC values of the ED probes.
- None of these matches the values the undefined ED 00–3F wrote (2329h,
  2800h, 232Ah, A426h), so the I/O-read hypothesis is refuted, with 8-bit
  and with 16-bit port addressing. Together with the code bytes at 0B0Ch
  (2FCDh, not 2329h) and the zero scratch page at 0130h/0150h/0180h, a
  memory read at (BC) is refuted as well. The remaining candidate inputs
  are A, F, B, C and the sandbox buffer contents; `EDPRB3` isolates them.

## Run 7: `EDPRB3`

- Disk before the run (installer at `f75965fa21819151b50a98afc77c3e45f0be1f29`,
  written from `cpmva-zexall-test.2d.d88`):
  `1b6c5376b46c8e4c22e6e80505510244dec895814fcce18f3946168256c77338`
  (the emulator container `cpmva-zexall-test.d88` of the same build differs
  only in the media byte).
- Disk after the run (all sectors good):
  `11214725708af1eed9868bb8353929a216d27e0b8fb49f95664397f1a3d7e450`.
- `edprb3.txt` holds all 28 records. Varying A, F, B, DE, HL, SP or any of
  the three sandbox memory words leaves the value at 2800h; only C changes
  it, and the ED 10/ED 00 controls return the same value into DE/BC.
  The value therefore depends on C alone:

  | C | value | C | value |
  | --- | --- | --- | --- |
  | 00h | 2329h | 31h | 0028h |
  | 0Ch | 2329h | 50h | 232Ah |
  | 30h | 2800h | 80h | A426h |
  | FFh | 29F0h | | |

- Reading the pairs as little-endian words of one hidden byte-addressed
  space S gives a consistent assignment (S[30]=00, S[31]=28, S[32]=00
  satisfies both C=30 and C=31; C=FF and C=00 share S[00]=29, so the
  address wraps modulo 256). The observed bytes equal an x86-style
  interrupt vector table: vectors 0 and 3 share offset 2329h, C=50h names
  the adjacent stub 232Ah, and S[FF] = F0h fits a BIOS ROM segment high
  byte. The working hypothesis — untested beyond these seven points — is
  that the undefined ED 00–3F/74/75/77 read the word at native physical
  address C of the uPD9002's x86 side (the emulation-mode IVT page);
  `EDPRB4` sweeps all 256 C values to test it.

## Run 8: `EDPRB4` C sweep

- Disk before the run (installer at `54d51aaa53aa56e8433e1ee4bdb369317c95d1c5`,
  written from `cpmva-zexall-test.2d.d88`):
  `79c568c18dedabe69a3967debce2e8490eaa7aeae00c96d027f44575f9057a9f`.
- The maintainer first ran `EDPRB3` again by mistake; its output is
  byte-identical to run 7, confirming determinism. A first disk read
  (`018326b6...`) predates the `EDPRB4` run; the evidence read is
  `7fdaa683557be8a30dc21fd0cbf0d52726ec3e9de4a5b4086f85d6e8eb7b862d`
  (all sectors good).
- `edprb4.txt` holds all 256 records. Reconstructing one byte-addressed
  page from the 256 overlapping little-endian word reads is consistent in
  all 512 byte observations, confirms the modulo-256 wrap, and yields a
  well-formed x86 interrupt vector table: every vector has segment F000h,
  unused vectors share the handler F000h:2329h, INT 08h–0Fh and
  INT 10h–17h have individual handlers (INT 0Ch = F000h:2800h is the
  value EDPRB2 set 0 saw), and INT 20h–27h are service entries
  (INT 20h = F000h:A426h).
- Together with runs 4, 5 and 7 this establishes the rule for the
  undefined ED 00–3F, 74, 75 and 77: the pair selected by bits 5–4
  (74/75/77: SP) is loaded with the little-endian word at bytes C and
  (C+1) mod 256 of the uPD9002's native physical page 0 — the x86-side
  interrupt vector table — and F becomes S/Z/P of the high byte with
  H=N=C cleared. A, F, B, the pointers and Z80-visible memory do not
  affect the value.

## Run 9 (vaeg): live-IVT wiring check

- `EDPRB3` and `EDPRB4` run inside vaeg emulating a PC-88VA (not VA2) under
  CP/MVA, as the end-to-end check of the G102 candidate
  (`vaeg.exe` from `077718baab2b0e73c0db9a01f982e053bb7cb10c`, SHA-256
  `ca3e3b98c1fb04598de98269446bd3f90859faa6df755a221aaab28826c24e12`; the
  maintainer reported the environment as "vaeg/VA"). The mounted disk was
  the `cpmva-zexall-test.2d.d88` variant (media byte 00h), which vaeg read
  without problems in this run.
- Disk after the run:
  `b8a3fa3064680b367144fada32ef193d31d4cc12b3f2dca6bf82faf1606bb1c4`.
- All 28 + 256 records are present. The 256-point sweep reconstructs a
  byte-consistent page (512 overlapping observations, modulo-256 wrap) that
  is again a well-formed IVT — default handler F000h:19A5h, and INT 09h,
  0Ah and 0Ch hooked to segment 19E3h — and the R14 flag rule reproduces
  on the new values (for example C=30h: C050h, F=84h = sign of C0h plus
  parity of 50h).
- The values differ from runs 7–8 because vaeg reads its guest's live
  native page 0, and the VA BIOS/OS environment differs from the real
  PC-88VA2; this is the intended behaviour of the wiring, not an emulation
  mismatch. `compare.txt` therefore shows these records as matching neither
  fixed reference. Byte-identical equality with the real VA2 records is
  checked by the CP/M runner's run-8 fixture instead.

## Run 10 (vaeg): PC-88VA2 guest against the real VA2

- The run-9 check repeated with vaeg emulating a PC-88VA2, the same machine
  type as the real runs. Disk after the run:
  `66adfed83bd23257795ba70d412a7930b6660aa2599530ff26aab267580570fe`.
- `edprb3.txt` is byte-identical to the real VA2 output of run 7.
- `edprb4.txt` matches the real run 8 in 241 of 256 records. The 15
  differing records decode to exactly three IVT entries: INT 09h, 0Bh and
  0Dh point to ROM handlers (F000h:738Ch/038Ch/039Ah) on the real machine
  but are hooked to RAM segment 1AFBh in vaeg's guest. This is operating
  system state at the time of the run, not CPU behaviour; every other byte
  of the live page, including the modulo-256 wrap, matches the real
  machine.

## Host references

| Used for | Commit | `vaeg_cpm_runner` SHA-256 |
| --- | --- | --- |
| `run1_vaeg` | `5f100c92c0855c06e90771cf0cf821044e13f0d8` | not recorded (superseded build) |
| `run2_vaeg` | `e1d34eeede0aba201e9a28a88e5b2d04c70f48fb` | not recorded (superseded build) |
| `run1_pc88va2`, `run2_pc88va2` | `f3baa4dd4c2ab9a00a12861910e91b4946d5f943` | `47e31ccea1c9cd47bb266be51428af594f39f1e5b091fff214377eb78b79c7e9` |
| `run3_pc88va2`–`run5_pc88va2` | `0dbce3c71c84c1c8ae72ce596bfc5800e8a2a544` | `4134146c4ee4be72db56411609f90a0bc5457869f1737868cdf1c69e9d8fe1c0` |
| `run6_pc88va2`, `run7_pc88va2` | `f75965fa21819151b50a98afc77c3e45f0be1f29` | `4134146c4ee4be72db56411609f90a0bc5457869f1737868cdf1c69e9d8fe1c0` |
| `run8_pc88va2` | `54d51aaa53aa56e8433e1ee4bdb369317c95d1c5` | `4134146c4ee4be72db56411609f90a0bc5457869f1737868cdf1c69e9d8fe1c0` |
| `run9_vaeg`, `run10_vaeg` | `97f04efe53ed070834aed42f14142fb47712591f` | `810675f728b69384a7982ee3ae8964daf39a6c1272a6ffed982f29cbd6b8bf55` |

All were produced by `tools/cpmva/zex/host_reference.py` (Linux GCC
`linux-ci-gcc` build, exit status 0) and are regenerated by the same command
at the listed commit.
