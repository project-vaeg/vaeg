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
# M103a alternate-register QA outputs

Outputs of `ALTPRB` (`tools/cpmva/zex/altprb.py`, `ALTPRB.COM` SHA-256
`f26f8aa7c3c7bc78ec65edc78fae7945e58d6355bc0f8a399493707cce210651`) run under
CP/MVA on a real NEC PC-88VA2 in V3 mode and in vaeg, from the test disks
generated at `c0d3a0d4259c02334764dc7671f086044b48c4b5` (real-machine
`cpmva-zexall-test.2d.d88`, SHA-256
`80b684392a8c1d7e18b25294b89766b4b0d87fabc71f1475cb8d0da447dcd17d`; vaeg
`cpmva-zexall-test.d88`, SHA-256
`a8080e6141865696f96991ef1ae5cdf6f10e9414aceae74d4be35f0039728b8b`).
The layout follows [`m102_zexund_qa`](../m102_zexund_qa/README.md): `raw/`
holds the byte-exact output (lower-cased names, binary in
`.gitattributes`) and `manifest.sha256` its SHA-256. Files were extracted
with `tools/cpmva/zex/extract_d88.py --lowercase --skip-com`. The PC-Engine
boot disk is private and not identified.

## Run 1 (2026-10-03)

| Directory | Machine | Result disk SHA-256 |
|---|---|---|
| `run1_pc88va2` | real PC-88VA2 | `727acb6f073757c727cc2a073eb046484908a95fd734038be501e631e7abd190` |
| `run1_vaeg` | vaeg, static Windows build of `33f268cc5487e4feffdde75bcb495c4657cdcf94` (`vaeg.exe` `999b702aa4095d517531bda007b23a9438884e6b17c4359c48986e87532c0b8e`), VA2 | `6df9bbaf58f16051920719a7fd9d3a74025d869986975e105dd62ab2231716d2` |

`compare-run1.txt` compares the two record by record using
`probe_results.parse_altprb`; `compare.py` is not used because its ALTPRB
section expects a host reference, and the host runner cannot execute
`CALLN` (see the generator's notes).

Case X0 runs no `CALLN`; X1–X3 call `CALLN 91h` (memory byte read) with
HL=F000h, DE=13EDh and input AF = 00D7h, 0000h and 00C5h; the alternate set
is AF'=AA45h, BC'=A1A2h, DE'=B1B2h, HL'=C1C2h. Records are captured
immediately before (`I`) and after (`O`) the call.

`[MEAS]` on the real PC-88VA2:

- The alternate set is unchanged by the `CALLN 91h` round trip in all
  three cases, and IX, IY, SP, DE and HL are unchanged.
- BC returns 0033h, the byte the VA2 ROM holds at F000:13EDh.
- F is returned unchanged, including bit 1 (N) = 0 in X2 (00h) and the
  value C5h in X3. Bits 3 and 5 were zero in every input, so their
  behaviour is not measured.

vaeg at `33f268cc` agreed on everything except F in X2/X3, where it
returned 02h and C7h: its native `IRET` forced bit 1 of the restored
flags to one even when the frame returns to compatible mode. The
correction and its test are recorded in the M103a task file and the
bug-fix ledger.

Scope: one firmware service, which does not use the alternate set; the
run does not establish alternate-set retention across `RETEM`/`BRKEM`
or reset (implementation policy in the CPU document §3.1).

## Run 2 (2026-10-03, vaeg after the correction)

| Directory | Machine | Result disk SHA-256 |
|---|---|---|
| `run2_vaeg` | vaeg, static Windows build of `bb217138bb039fdec3f57d5bd48af69f0cd58f07` (`vaeg.exe` `9292aceaa71ed0eae7f397b132bf4e80ab2aa4e07257c5f9a5588d8f7a817aa2`), VA2 | `a05b78551e435766646d021c1dc088f7f57c5ce54d07f87af5760cb56a07191a` |

`run2_vaeg/raw/altprb.txt` is byte-identical to `run1_pc88va2/raw/altprb.txt`
(both SHA-256 `8a559139d5eaeedee8082a0855e4af5c35acb96c4cd6c5db717d80f9899de30a`):
with the F correction vaeg reproduces all eight real-machine records.
