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
# M69 - Correct IDP/TSP port 0142H status-bit composition

## Fixed predecessor

M69 starts only from the formally approved G68 candidate:

`d1e0225c4edb716893fe5579283fbf0915db72b9`

Approved predecessor gate: `G68`

Approved predecessor branch:
`topic/m68-upd9002-segmented-word-mapped-dispatch`

Approved hosted CI:
`https://github.com/nakatamaho/vaeg/actions/runs/30369606181`

Approved M68 report:
`docs/agents/reports/m68_upd9002_segmented_word_mapped_dispatch.md`

Approved G68 production-fix/evaluated SHA:
`90258f26207b7ce7dc3473a5df2811da4bb0c19c`

Approved G68 worker SHA-256:
`125e39d6e1e1da35bac017e133f03ba66195001ae13bfaab1b05a213d6c47f7c`

Approved target policy:
`upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`

Do not use G67 or any earlier SHA as the M69 predecessor.

## Canonical identities

Branch:
`topic/m69-upd9002-idp-0142-status-composition`

Commit prefix: `M69:`

Candidate gate: `G69`

Report:
`docs/agents/reports/m69_upd9002_idp_0142_status_composition.md`

All newly written code, comments, tests, reports, task text, and commit
messages must be in English.

Do not rename `tsp`, `IDP`, `iova/tsp.c`, or existing public interfaces.

## Hardware authority

The supplied PC-88VA I/O map defines:

- `0142H` OUT: IDP command port.
- `0146H` IN/OUT: IDP parameter port.
- `0142H` IN: IDP status port.

The `0142H` status byte contains independent bits:

- bit 7: LP, light-pen signal detected.
- bit 6: VB, vertical blanking period.
- bit 5: SC, sprite overrun/collision.
- bit 4: ER, error occurred.
- bit 3: EMEN, emulation expansion executing.
- bit 2: BUSY, command executing.
- bit 1: OBF, output-data buffer full.
- bit 0: IBF, input-data buffer full.

The required composition rule is:

```text
result = stored IDP status flags OR dynamic VB flag
```

VB must not replace, normalize, erase, or reinterpret the stored flags.

## Defect model

The predecessor expression in `iova/tsp.c` is expected to be equivalent to:

```c
dat = tsp.status | (tsp.vsync) ? STATUS_VB : 0;
```

C operator precedence parses this as:

```c
dat = (tsp.status | tsp.vsync) ? STATUS_VB : 0;
```

not as:

```c
dat = tsp.status | (tsp.vsync ? STATUS_VB : 0);
```

The minimum required truth table is:

```text
stored status  VB  expected
-------------  --  --------
00H             0   00H
00H             1   40H
04H             0   04H
04H             1   44H
```

The defective predecessor is expected to produce:

```text
stored status  VB  predecessor
-------------  --  -----------
00H             0   00H
00H             1   40H
04H             0   40H
04H             1   40H
```

The general invariant is:

```text
read_0142(status, VB=0) = status
read_0142(status, VB=1) = status | 40H
```

## Scope boundary

M69 fixes only `0142H` status-bit Boolean composition.

M69 must not implement or redesign:

- IBF timing.
- OBF timing.
- ER generation.
- SC generation.
- EMEN behavior.
- Light-pen behavior.
- Command latency.
- Parameter latency.
- FIFO behavior.
- Asynchronous execution.
- IDP rendering.
- TVRAM mapping.
- Segmented memory.
- MOVSW.
- CPU instruction semantics.

Unimplemented status semantics may be inventoried, but not speculatively
implemented. Do not modify M68 production code, `cpu/upd9002/`,
`cpucva/memoryva.c`, or M68 mapped-memory tests.

## Required tests

Add focused tests before the production fix. Prefer the registered `0142H`
input path. Use an internal state setup seam only to establish deterministic
stored status and VB state; do not expose a new production public API solely
for testing.

Required cases:

- `00H`, VB off and on.
- `04H`, VB off and on.
- Every non-VB single bit with VB off and on.
- Representative combinations `3FH` and `BFH`.
- VB idempotence for `40H` and `44H`.
- Exhaustive composition over stored status `00H` through `FFH` with VB off
  and on.

Expected value:

```c
expected = stored | (vb ? STATUS_VB : 0);
```

Also audit command lifecycle. If the current implementation exposes an
externally observable BUSY interval, test BUSY with VB off and on through the
I/O interface. Do not add asynchronous timing or artificial delays.

## Required implementation

Use an unambiguous implementation. Preferred form:

```c
static REG8 IOINPCALL tsp_i142(UINT port) {
	REG8 dat;

	dat = tsp.status;
	if (tsp.vsync) {
		dat |= STATUS_VB;
	}
	return dat;
}
```

Do not change status-bit values, command write behavior, parameter-port
behavior, command decoding, reset behavior, save-state format, rendering,
timing, or interrupt behavior.

## Validation

Run:

- M69 truth-table and exhaustive composition tests.
- Existing TSP/IDP-relevant tests.
- Display/text BIOS and TVRAM tests.
- M68 mapped-memory regression tests.
- Save/load tests.
- Native non-external CTest suite.
- Linux GCC build, Clang build, required sanitizer build, and MinGW build
  where available.
- Repository invariants, milestone-ID validation, encoding checks, EOL checks,
  path-case checks, and `git diff --check`.

M69 changes IDP/TSP I/O status behavior only. It must not change
`cpu/upd9002/`, SST fixtures, SST corpus, comparison contracts, target policy,
classifications, selected sets, or applicable sets. Use identity-bound G68
CPU/SST results when repository policy permits; run full profiles if any
CPU/SST-governing input changes unexpectedly.

## Manual gate

Produce a test executable for maintainer validation. The maintainer should
test PC-Engine/MS-DOS cold boot, `DIR A:`, `CHKDSK A:`, multi-screen text
output, `CLS`, a demo/game, save state, load state, and Sound Board II.

If maintainer manual validation is not yet available, stop at a clearly
identified `M69 manual-test candidate`. Do not create terminal evidence and do
not declare G69 passed.

## Explicit nonclaims

M69 proves and corrects only the Boolean composition of the `0142H` status
byte. It does not prove complete IDP timing, FIFO, IBF, OBF, ER, SC, EMEN, or
light-pen behavior. It does not claim that the `0142H` defect caused the M65e
text-scroll regression; that regression was independently attributed to the
segmented-word mapped-memory bypass and corrected by M68.
