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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# HOSTFAT clean-room implementation attestation

I independently authored `hostfat.asm` from the clean-room factual contract.
The implementation expression—including all labels, control flow, instruction
choices, data layout beyond mandated byte contracts, comments, and
formatting—is my own and was not copied, adapted, or compared against any prior
implementation or output.

## Inputs consulted

The complete list of informational inputs consulted is:

1. The 220-line revision of
   `docs/agents/research/m54_hostfat_cleanroom_spec.md` committed at
   `53e44dac5f2046c1c2cd5090f4f7464744fecf7e`, read completely. The later
   explicit CPU-level amendment was made during integration and was not an
   authorship input.
2. My pre-existing general knowledge of NASM syntax and NEC V30/8086-family
   real-mode assembly. I did not consult an external manual, web page, example
   driver, or other source while producing this implementation.

NASM 3.01 was used only to assemble the independently authored source.
Standard local shell utilities were used only to create the isolated working
directory, read the authorized specification, and validate the newly produced
artifact. After authorship, I inspected the header, required strings, size,
and hash of my own generated `hostfat.sys`; that artifact was not an
implementation input and was not compared with any previous output. No other
externally supplied file, document, archive, binary, source, report, history,
web page, or generated output was consulted.

## Prohibited-input affirmation

I affirm that I did not view or consult:

- any `RDBMS*` file, archive, document, source, or binary;
- any existing or historical `tools/pc88va/hostfat/hostfat.asm`;
- any pre-existing or externally supplied compiled `HOSTFAT.SYS`, any
  disassembly of one, or any derived output;
- `tools/pc88va/hostfat/check_driver.py`;
- any M54 report, commit, diff, or Git history; or
- any source implementing an analogous PC-Engine driver.

I did not search the vaeg repository, inspect any other vaeg file, run a Git
command, or compare this work with a previous output.

## Build result

NASM 3.01 assembled `hostfat.asm` successfully as the 512-byte flat binary
`hostfat.sys`.

- `hostfat.asm` SHA-256:
  `0b1d7fb586fe5fdfb2b6077943dbc6eb9bcd14aa6a271d34fe30d1d8ca6d4368`
- `hostfat.sys` SHA-256:
  `665882e22cbf7f3106cfedc6fb2cdae09cf9434296a4f4a9caf2599e2382ec29`

Signed-off-by: OpenAI Codex clean-room implementation agent  
Date: 2026-07-22 (Asia/Tokyo)

## Post-authorship integration audit

The hashes above identify the isolated implementation seed exactly as
attested; they are not the final integrated source or binary identities. The
reviewing agent subsequently assembled and disassembled that seed, compared
only after independent authorship, and performed PC-Engine integration tests.

The seed allowed NASM to select its default CPU. Long conditional branches
were consequently relaxed to 80386 `0F 84H` encodings. On a V30, `0FH` is not
the 80386 conditional-jump escape, so sector-command dispatch did not reach
the intended handler. The integration correction:

- fixes the assembler CPU level at 8086;
- expresses dispatch as short `JNE` around an 8086 short or near `JMP`; and
- makes the generated-driver checker reject every `0F 80H`--`0F 8FH`
  encoding and validate each dispatch edge.

No RDBMS source, superseded HOSTFAT source, or third-party implementation was
used to express that correction. Its inputs were the NEC V30 execution
constraint, the independently authored source, its generated disassembly,
and observed PC-Engine behavior. A separate comparison tested the original
resident-end ordering with the corrected 8086 dispatch; DIR, TYPE, and COPY
still passed, so that independently authored ordering was retained.

Final integrated identities:

- `hostfat.asm` SHA-256:
  `aa91ed4768a789398a63fa26669da843f91c28947b929b1051d542abe8f04788`;
- generated `hostfat.sys`: 528 bytes, SHA-256
  `c036b88178f058295eaeedae8c9dffd0bcf13addb13449c307b2fba921a8f675`;
- integration correction commit:
  [`bdcbeae89b254dd02b8916104baac81c94f94a4d`](https://github.com/nakatamaho/vaeg/commit/bdcbeae89b254dd02b8916104baac81c94f94a4d).
