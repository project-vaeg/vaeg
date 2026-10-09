<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
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
# CP/MVA setup guide

This guide explains how to prepare and run CP/MVA on VAEG. The procedure was
validated with the M76 uPD70008-compatible Z80 emulation path and a generated
CP/M tools disk that reaches the CP/M `A>` prompt.

## Requirements

- A VAEG build with the M76 changes.
- A user-owned, FAT-formatted PC-Engine boot D88. The input image must be
  obtained lawfully; VAEG does not redistribute PC-Engine ROMs or guest disks.
- Python 3.10 or newer.
- `z80asm` 1.8. If none is supplied with `--assembler` or `VAEG_Z80ASM` and
  none is in `PATH`, the installer downloads the locked z80asm 1.8 source,
  verifies its SHA-256, and builds it once into its cache. This needs a C
  compiler (`cc`, `gcc`, `clang`, or `$CC`).
- `lha` (lha-1.14i or lhasa, e.g. the Debian/Ubuntu `lhasa` package) or
  `unar` for the CPMVA archive.

The installer downloads and verifies the locked CPMVA, CP/M, game, and BDS C
sources. It does not execute downloaded DOS or CP/M programs on the host.

## Build the disks

From the VAEG checkout, run:

```sh
python3 tools/cpmva/install_cpmva.py \
    --boot-disk /path/to/pcengine-boot.d88 \
    --output-dir /path/to/cpmva-ready \
    --accept-cpm-license \
    --accept-cpmva-license \
    --accept-games-license \
    --accept-bdsc-license
```

The original boot disk is never modified. The output directory contains:

```text
pcengine-boot-cpmva.d88  PC-Engine boot disk copy with CPMVA files
cpmva-tools.d88         CP/M tools and games disk
cpmva-source.d88        source and documentation disk
cpmva-dev.d88           BDS C development disk
cpmva-zexall-test.d88   Z80 exercisers and uPD9002 flag probes only
cpmva-zexall-test.2d.d88  the same disk with the 2D (00h) media byte
cpmva-build-manifest.json
cpmva-install-report.txt
```

The installer builds the 64K CP/M 2.2 CCP/BDOS from the locked public source,
combines it with the CPMVA `CPMBIOS.COM`, and writes the resulting `CPM.SYS`.
It does not use NEC `MOVCPM5.COM`, `DDT.COM`, `SAVE`, M80, L80, or MASM.

For an offline installation, populate the cache during an online run and add
`--offline`. Local archives can be supplied with `--cpmva-archive`,
`--cpm22-archive`, `--vt100-games-archive`, and `--bdsc-archive`; every local
archive is still checked against the lock file. See
[`tools/cpmva/README.md`](../tools/cpmva/README.md) for source-lock and cache
details.

For automated runs, `--screen-dump PATH` saves the final rendered SDL screen as a BMP.
Use `--screen-tvram-dump PATH` separately when the raw `VAEGSCN1` TVRAM diagnostic is
needed; omitting both options preserves the normal display defaults.

## Run CP/MVA in VAEG

1. Start VAEG and load `pcengine-boot-cpmva.d88` as the PC-Engine floppy
   boot disk.
2. Boot the guest and run `CPMVA.BAT`, or type `CPMVA` at the DOS prompt.
3. When CP/MVA displays `Set CP/M diskette on drive FD1: and hit any key`,
   replace FD1 with `cpmva-tools.d88`.
4. Press a key. CP/MVA loads the CCP, BDOS, and BIOS from the tools disk and
   should display the CP/M `A>` prompt.
5. Run `DIR` to verify the tools disk, then run `EXIT` to return to
   PC-Engine.
6. Swap FD1 to `cpmva-source.d88` or `cpmva-dev.d88` when source files or BDS C
   tools are needed.

The CP/MVA disk images generated together by the installer must be used as a
set. Do not manually rewrap them as another D88 geometry; the tested VAEG
layout preserves the CP/M directory and sector mapping expected by the
PC-88VA BIOS.
The CP/MVA `CPMBIOS.MAC` DPB uses `EXM=1`, so the installer stores two
16 KiB sub-extents in one directory entry. Large programs such as
`BACKGMMN.COM` and `CC2.COM` must not be manually split into one entry per
16 KiB; that legacy layout repeats the same logical extent and causes CP/MVA
to load only the first part of the program. The installer validator rejects
duplicate or gapped logical extents before emitting a disk.

## Headless runs

The SDL2 frontend accepts the existing headless input script format. A disk
can be swapped after CP/MVA has displayed its prompt with `@fdd1`:

```text
CPMVA
@wait 1200
@fdd1 /path/to/cpmva-tools.d88
@wait 1200
@enter
@wait 600
DIR
@wait 600
EXIT
```

Pass the script with `--headless-input-script /path/to/script.txt`. This is
useful for repeatable smoke tests, but it does not replace the interactive
instruction to change FD1 in a normal session.

## Z80 exercisers and uPD9002 flag probes

The installer also assembles the following CP/M programs with z80asm 1.8 and
puts them on both `cpmva-tools.d88` and `cpmva-zexall-test.d88` (M101). They exist
only on the disks you generate; VAEG does not distribute them.

| Program | Output | Purpose |
| --- | --- | --- |
| `ZEXDOC.COM`, `ZEXALL.COM` | console | Stock exercisers, rebuilt byte-identically from `external/zex` |
| `ZEXDOCF.COM`, `ZEXALLF.COM` | `ZEXDOC.TXT`, `ZEXALL.TXT` | Stock exercisers that also write the console output to a file |
| `ZEX13S.COM` | `ZEX13S.TXT` | ZEXDOC with `<daa,cpl,scf,ccf>` split into single-opcode groups |
| `FLAGPRB.COM` | `FLAGPRB.TXT` | Direct flag probes P0–P7 for rules R1–R7 |
| `DAADUMP.COM` | `DAA.BIN`, `CPL.BIN`, `SCF.BIN`, `CCF.BIN`, `DAADUMP.TXT` | Exhaustive DAA/CPL/SCF/CCF dump |
| `ZEXIY.COM` | `ZEXIY.TXT` | INC/DEC IXH/IXL/IYH/IYL, correcting the stock ZEX iyh/iyl groups (M102) |
| `ZEXUND.COM` | `ZEXUND.TXT` | Undocumented DDCB/FDCB forms, NEG duplicates and redundant DD/FD prefixes (M102) |
| `ZEXED.COM` | `ZEXED.TXT` | Undefined ED opcodes, expected to be NOPs (M102); run last |
| `EDPRB.COM` | `EDxx.TXT` | One-by-one probe of undefined ED opcodes and the NEG/RETN duplicates (M102) |
| `CBPRB.COM` | `CBPRB.TXT` | One-by-one probe of every DD/FD CB opcode with two input sets (M102) |
| `EDPRB2.COM` | `E2xx.TXT` | Second ED probe: sandboxed pointers, instruction length, three input sets (M102) |
| `EDPRB2S.COM` | `S2xx.TXT` | `EDPRB2` for nine opcodes with the code moved by 100h bytes (M102) |
| `INPRB.COM` | `INPRB.TXT` | Documented `IN` reads of the ports named by C in the ED probes (M102); real machine only, the host runner rejects I/O |
| `EDPRB3.COM` | `EDPRB3.TXT` | One-input-at-a-time isolation of the undefined ED 00–3F value (M102) |
| `EDPRB4.COM` | `EDPRB4.TXT` | Full C sweep (00h–FFh) of the undefined ED 20 value (M102) |
| `ALTPRB.COM` | `ALTPRB.TXT` | Alternate register set and flags across a `CALLN 91h` round trip (M103a); real machine only |

`EDPRB` prints `ED xx` before each opcode and closes its output file after
every record, so a run that hangs keeps all earlier results. After a reset,
continue with the next opcode, for example `EDPRB A5`; that run writes
`EDA5.TXT`, and `compare.py` merges all `ED??.TXT` files.

`EDPRB2` (generated by `tools/cpmva/zex/edprb2.py`) prints `ED xx s` before
each run, where `s` is the input set 0–2, and also closes its file after every
256-byte record. Restart it in the same way, for example `EDPRB2 C1` writes
`E2C1.TXT`. By default it skips the opcodes that left the CP/M emulation mode
on the real machine (ED 54, 55, AC and EE–FC); add `+` to run them as well,
for example `EDPRB2 EE+`. `EDPRB2S` runs nine opcodes in a shifted layout and
writes `S200.TXT`. Both need about 140 KiB of free space together, so run them
on a freshly generated `cpmva-zexall-test.d88`.

The ZEX-derived programs are GPL-2.0-or-later; see
`external/zex/provenance.txt` and ADR-0015. Output files are written to the
current CP/M drive. `DAADUMP` needs about 130 KiB of free space, which the
tools disk does not have. For real-machine runs, write `cpmva-zexall-test.2d.d88` to the medium: it
differs from `cpmva-zexall-test.d88` only in the media byte at offset 1Bh
(00h instead of the 10h that vaeg needs; see the `wrap_cpm_d88` comment),
which real-disk writing tools expect for the 2D-320 geometry. In vaeg, swap
FD1 to `cpmva-zexall-test.d88`
after CP/M has started, press Ctrl-C to log in the new disk, and run the
programs from it; it holds only these programs and has room for all of their
outputs. The ZEX runs take hours on a real PC-88VA.

To produce host references and compare real-machine outputs:

```sh
cmake --build --preset linux-ci-gcc --target vaeg_cpm_runner
python3 tools/cpmva/zex/host_reference.py \
    --runner build/linux-ci-gcc/vaeg_cpm_runner --output /tmp/m101-host
python3 tools/cpmva/zex/compare.py /path/to/real-outputs --ref /tmp/m101-host/ref
```

`host_reference.py` writes `programs/MANIFEST.TXT` with the SHA-256 of every
program and checks the host acceptance criteria for the Zilog and uPD9002
profiles; `--skip-zex` omits the long ZEXDOCF/ZEXALLF runs. It obtains
z80asm 1.8 the same way as the installer.

## Troubleshooting

- If CP/M stops before `A>`, confirm that the boot disk is the generated
  `pcengine-boot-cpmva.d88` and that FD1 was changed to the matching
  `cpmva-tools.d88` only after the CP/MVA disk prompt.
- If `DIR` reports `Bdos Err on A: Bad Sector` or shows no file names,
  regenerate the disks with the same installer version and use the generated
  pair without a manual D88 conversion.
- If `EXIT` is reported as unknown, verify that `EXIT.COM` is visible in
  `DIR` and that the tools disk is still mounted in FD1.
- Existing output files require `--force`; `--dry-run` and `--verify-only`
  do not replace output files.

CP/M, CPMVA, the included games, BDS C, and their documentation retain their
original licenses. The installer records their provenance and does not assign
VAEG's BSD-2-Clause license to those materials or to generated disk images.
