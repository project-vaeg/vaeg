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
# M68 - Restore canonical mapped-memory dispatch for segmented word access

## Maintainer authorization

M68 is reassigned to the uPD9002 segmented-word mapped-memory dispatch fix.
Any previous unapproved M68 scope is revoked and deferred for later
reassignment under a new milestone identifier. This task is the canonical M68
authority. Do not execute any former M68 scope.

No formal G68 approval exists at reassignment time. If an approved G68 gate is
later found, stop and report the conflict.

All newly authored code, comments, tests, task text, reports, artifacts, and
commit messages must be in English.

## Fixed identities

Approved predecessor gate: `G67`

Approved predecessor SHA:
`f8f350e1aadec4b6c79c20192d14c50bd39934be`

Integration base:
`5e044f802c6cd3a1bb55f694897b0fe5561d146b`

Branch:
`topic/m68-upd9002-segmented-word-mapped-dispatch`

Commit prefix: `M68:`

Candidate gate: `G68`

Report:
`docs/agents/reports/m68_upd9002_segmented_word_mapped_dispatch.md`

Approved target policy:
`upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`

## Regression evidence

The PC-Engine/MS-DOS text-scroll regression was manually bisected to:

- M65d checkpoint `ef44acbf5183ac5a8233ac007b07de72fd61eae8`: OK.
- M65e checkpoint `8350ca5d8345f3414e1864dcb6d70e391ea60cc1`: NG.

The A5 isolation experiment established:

- M65e without only the A5/MOVSW change: OK.
- M65d with only the A5/MOVSW change: NG.

Therefore the M65e A5 segmented-word access change is necessary and
sufficient for the observed runtime regression.

The defect is not MOVSW iteration behavior. CX, SI, DI, IP, DF, REP count,
and direction are not the cause. The defect is an independent flat `mem[]`
fast path inside the segmented word helper, which bypasses canonical
mapped-memory dispatch and reads flat shadow RAM instead of active TVRAM
backing storage.

## Architectural rule

Segmented helpers own only segment-offset address formation and 16-bit
offset wrapping. The canonical generic memory API exclusively owns backing
store selection, VA mapping dispatch, callbacks, side effects, dirty tracking,
and fast-path selection.

The segmented word helper must not inspect or use:

- `mem[]`
- `I286_MEMREADMAX`
- `I286_MEMWRITEMAX`
- `memmode_va`
- TVRAM ranges
- BMS ranges
- device-specific mapping policy

Do not special-case A5, TVRAM, or `A0000h`. Fix the shared helper boundary.

For contiguous segmented word reads and writes, delegate to
`i286_memoryread_w()` and `i286_memorywrite_w()`. Split only the
noncontiguous `FFFFh -> 0000h` segment-wrap case into two canonical byte
accesses.

## Required tests

Add focused mapped-memory regression tests before the production fix. The
tests must fail on the M68 starting base for the established flat-`mem[]`
bypass reason.

Cover:

- segmented word read and write;
- non-REP MOVSW;
- REP MOVSW with count 1 and count greater than 1;
- normal RAM to TVRAM;
- TVRAM to normal RAM;
- TVRAM to TVRAM;
- mapped below-`A0000h` to normal RAM;
- normal RAM to mapped below-`A0000h`;
- ordinary aligned offset, ordinary unaligned offset, `FFFEh`, and
  `FFFFh -> 0000h`;
- `DF=0` and `DF=1`;
- deliberately distinct flat `mem[]` shadow values and mapped backing values.

The tests must prove that TVRAM reads return/copy `textmem[]` values, not
flat shadow values, and that mapped writes update the mapped backing store
with required dirty/display side effects. Use BMS for the below-`A0000h`
mapped region when deterministic probing is supported; otherwise use another
existing mapped region and document why.

## Required implementation

Correct the shared segmented word helpers only. Preserve the M65e segment-wrap
fix.

For reads:

```c
address = segment_base + LOW16(offset);
high_address = segment_base + LOW16(offset + 1);
if (high_address == address + 1) {
	return i286_memoryread_w(address);
}
return (REG16)(i286_memoryread(address) |
	((REG16)i286_memoryread(high_address) << 8));
```

For writes:

```c
if (high_address == address + 1) {
	i286_memorywrite_w(address, value);
	return;
}
i286_memorywrite(address, (REG8)value);
i286_memorywrite(high_address, (REG8)(value >> 8));
```

Use actual repository types and preserve existing physical-address mask/A20
behavior.

## Consumer audit

Inventory every active caller of the corrected helper, including at least:

- A5 MOVSW, REP and non-REP;
- 61 POPA;
- 81 word RMW;
- 83 word-immediate RMW;
- FF /3 far CALL;
- FF /5 far JMP;
- 9C PUSHF;
- D1 /6;
- C8 ENTER;
- C4 LES;
- C5 LDS;
- `meml_read16()`;
- `meml_write16()`.

For every consumer record source path, function, opcode/form, read or write,
normal or wrapping use, mapped-memory relevance, and focused test coverage.
Stop if any caller relies on the erroneous flat-memory bypass.

## Validation

Run the native non-external CTest suite, Linux GCC, Clang, ASan/UBSan,
MinGW, state save/load protected tests, M66 identity/state checks, M67
registry validators, milestone-ID validation, encoding/EOL/path-case checks,
and `git diff --check`.

Run the complete executable A5 population and preserve the owned case
`cbad10077f6e4b2dd631f45baffb3a862400450f561bedd74c9bd5be7d52b9da`.

Run the protected M65 populations:

- M65a FF /7: 5000 / 5000.
- M65b BOUND: 1244 / 1244.
- BOUND frame-only: 3565 / 3565.
- M65c F7 /2: 5000 / 5000.
- M65d FF /6: 5000 / 5000.
- M65e tail: 10 / 10.

Execute architectural CI, architectural full, and fingerprint full SST
profiles with the unchanged dataset, contracts, and target policy. The
intended new behavior is outside the flat-memory SST observation boundary, so
full SST identities must remain unchanged and the mapped-memory tests carry
the behavioral proof.

## Manual gate

Produce a maintainer test executable before terminal G68 closure. The
maintainer must cold-boot PC-Engine/MS-DOS and test `DIR A:`, `CHKDSK A:`,
multi-screen output, `CLS` after scrolling, a demo/game, save/load state, and
Sound Board II.

If maintainer manual validation is not yet available, stop at an explicitly
identified `M68 manual-test candidate`. Do not create terminal evidence, do
not request G68 approval, and do not declare G68 passed.

## TSP exclusion

Do not modify `iova/tsp.c`, `0142H` IDP status-port behavior, BUSY/VB
expression, IBF/OBF behavior, or TSP timing. That defect is separate and must
be handled in a later milestone.

## Artifacts and report

Create deterministic artifacts under `tests/ssts/campaigns/g68/` and generate
them twice to prove byte-identical output. Record artifact byte counts, row
counts where applicable, and SHA-256 values.

Write
`docs/agents/reports/m68_upd9002_segmented_word_mapped_dispatch.md`.

Stop after M68. Do not start M69 or any later milestone. Do not declare G68
passed.
