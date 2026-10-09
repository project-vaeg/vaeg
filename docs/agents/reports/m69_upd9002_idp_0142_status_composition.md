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
# M69 uPD9002 IDP 0142H status composition report

## Status

M69 corrects only the Boolean composition of the IDP/TSP `0142H` status byte.

G69 is not declared passed by this report.

## Fixed identities

- Branch: `topic/m69-upd9002-idp-0142-status-composition`
- Approved predecessor gate: `G68`
- Approved G68 predecessor SHA:
  `d1e0225c4edb716893fe5579283fbf0915db72b9`
- Approved G68 hosted CI:
  `https://github.com/nakatamaho/vaeg/actions/runs/30369606181`
- Integration base:
  `d1e0225c4edb716893fe5579283fbf0915db72b9`
- Task-authority SHA:
  `60db508511ac2a213b0474a456869f0642f2ed10`
- Regression-test SHA:
  `e797aa2aef8cb4263e21753b54b51c16dc562a47`
- Production-fix SHA:
  `6ef4f98ec1be20054db2aeb9c4a44c6a3d2e36bf`
- Evaluated SHA:
  `6ef4f98ec1be20054db2aeb9c4a44c6a3d2e36bf`
- Final candidate SHA: supplied by final handoff
- Target policy:
  `upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`

## Hardware authority

The supplied PC-88VA I/O map defines:

- `0142H` OUT: IDP command port.
- `0146H` IN/OUT: IDP parameter port.
- `0142H` IN: IDP status port.

The `0142H` status register contains independent LP, VB, SC, ER, EMEN, BUSY,
OBF and IBF bits:

| Bit | Mask | Name | Meaning |
| --- | --- | --- | --- |
| 7 | `80H` | LP | Light-pen signal detected |
| 6 | `40H` | VB | Vertical blanking period |
| 5 | `20H` | SC | Sprite overrun/collision |
| 4 | `10H` | ER | Error occurred |
| 3 | `08H` | EMEN | Emulation expansion executing |
| 2 | `04H` | BUSY | Command executing |
| 1 | `02H` | OBF | Output-data buffer full |
| 0 | `01H` | IBF | Input-data buffer full |

The required composition rule is:

```text
result = stored IDP status flags OR dynamic VB flag
```

VB must not replace, normalize, erase, or reinterpret the stored flags.

## Predecessor defect

Production source:

- Path: `iova/tsp.c`
- Function: `tsp_i142`

The predecessor expression was:

```c
dat = tsp.status | (tsp.vsync) ? STATUS_VB : 0;
```

C operator precedence parsed this as:

```c
dat = (tsp.status | tsp.vsync) ? STATUS_VB : 0;
```

It did not parse as:

```c
dat = tsp.status | (tsp.vsync ? STATUS_VB : 0);
```

Therefore the predecessor evaluated the combined status as a Boolean condition
and returned only `STATUS_VB` or zero. It erased stored status flags and
falsely reported VB whenever any stored flag was nonzero.

Recorded constants and state:

- `STATUS_BUSY`: `04H`
- `STATUS_VB`: `40H`
- `tsp.status`: `UINT8`
- `tsp.vsync`: `UINT8`; currently written as `0` or a VBLANK-domain value
  equivalent to `20H`, and consumed as a Boolean by the status read.
- Reset value: `tsp_reset()` clears the TSP structure, so `tsp.status` starts
  as `00H`.
- Save-state coverage: `statsave.tbl` serializes the `TSP` structure,
  including `status`, `vsync`, command, and parameter state.

Known writers of `tsp.status`:

- `tsp_reset()` clears the TSP structure.
- `tsp_o142()` sets `STATUS_BUSY` when a command is accepted.
- `exec_sync()`, `exec_dspon()`, `exec_dspdef()`, `exec_curdef()`,
  `exec_spron()`, `exec_exit()`, and `exec_unknown()` clear `STATUS_BUSY`.

Readers of port `0142H`:

- Byte `IN 0142H` dispatches through `iocoreva_inp8()` to `tsp_i142()`.
- Word `IN 0142H` dispatches through `iocoreva_inp16()`, using the `0142H`
  low byte and existing `0143H` high-byte behavior.

The predecessor could not return stored non-VB flags such as BUSY at their
correct bit positions through `0142H`; any nonzero `tsp.status` collapsed the
read result to `40H`.

## Truth-table evidence

Required truth table:

| Stored | VB | Expected | Predecessor | Final |
| --- | --- | --- | --- | --- |
| `00H` | 0 | `00H` | `00H` | `00H` |
| `00H` | 1 | `40H` | `40H` | `40H` |
| `04H` | 0 | `04H` | `40H` | `04H` |
| `04H` | 1 | `44H` | `40H` | `44H` |

The regression test failed on the predecessor for the intended defect:

```text
build/m69-predecessor-test/sdl2/vaeg --idp-m69-status-composition
exit status: 1

ctest --test-dir build/m69-predecessor-test -R vaeg_idp_m69_status_composition --output-on-failure
exit status: 8
```

Representative predecessor failures:

```text
stored=04 vb=0 expected=04 actual=40
stored=04 vb=1 expected=44 actual=40
word-in stored=04 vb=0 expected=ff04 actual=ff40
word-in stored=04 vb=1 expected=ff44 actual=ff40
busy-vb0 expected=04 actual=40
busy-vb1 expected=44 actual=40
exhaustive failures=508 over 512 rows
```

After the production fix:

```text
ctest --test-dir build/m69-predecessor-test -R vaeg_idp_m69_status_composition --output-on-failure
exit status: 0
1 / 1 test passed
```

The final exhaustive test covers all `256 x 2` stored-status and VB
combinations with:

```c
expected = stored | (vb ? STATUS_VB : 0);
```

Result:

```text
512 / 512 rows passed
```

The single-bit preservation, representative combination, and VB idempotence
cases all passed:

- `01H`, `02H`, `04H`, `08H`, `10H`, `20H`, and `80H`.
- `3FH` and `BFH`.
- `40H` and `44H`.

## Production correction

The corrected expression is intentionally explicit:

```c
dat = tsp.status;
if (tsp.vsync) {
	dat |= STATUS_VB;
}
return dat;
```

M69 did not change:

- status-bit numerical assignments;
- command write behavior;
- parameter-port behavior;
- command decoding;
- reset behavior;
- timing;
- save-state format;
- rendering;
- interrupt behavior.

## BUSY lifecycle audit

The current command model exposes BUSY after `OUT 0142H` accepts `CMD_SYNC`
and before the required fourteen parameter writes to `0146H` complete the
command.

Tested through the registered I/O path:

| Case | Expected | Actual |
| --- | --- | --- |
| BUSY active, VB=0 | `04H` | `04H` |
| BUSY cleared, VB=0 | `00H` | `00H` |
| BUSY active, VB=1 | `44H` | `44H` |
| BUSY cleared, VB=1 | `40H` | `40H` |

M69 does not redesign command duration or asynchronous command timing.

## Port-access audit

Byte `IN 0142H` is corrected to return stored status flags OR dynamic VB.

Word `IN 0142H` is verified unchanged:

```text
low byte:  0142H status
high byte: existing 0143H input behavior, returning FFH
```

Tested rows:

| Stored | VB | Expected word | Actual word |
| --- | --- | --- | --- |
| `04H` | 0 | `FF04H` | `FF04H` |
| `04H` | 1 | `FF44H` | `FF44H` |

No high-byte word-access behavior was invented or changed in M69.

## Save-state and reset audit

State serialization is unchanged:

- Save-state schema: unchanged.
- Save-state version: unchanged.
- State files changed: none.
- `statsave.tbl` continues to serialize the `TSP` structure.
- `tsp.status`, `tsp.vsync`, command state, and parameter state remain covered
  by the existing TSP section.
- Reset status remains unchanged because `tsp_reset()` still clears the
  structure.
- No new compatibility path was added.
- The approved G66b one-generation migration bridge was not broadened.

The native non-external CTest suite passed after the fix, including the
existing state protected tests.

## M68 protection

M69 did not modify:

- `cpu/upd9002/`
- `cpucva/memoryva.c`
- M68 mapped-memory tests
- SST corpus, fixtures, contracts, classifiers, selected sets, or target
  policy

M68 protected result:

```text
ctest --test-dir build/m69-predecessor-test -R vaeg_upd9002_m68_segmented_memory --output-on-failure
exit status: 0
```

The maintainer manual result also recorded that the M68 text-scroll correction
remains working.

## CPU/SST no-change proof

M69 changes IDP/TSP I/O status behavior only. No CPU/SST-governing input
changed, so the approved G68 identities are reused.

Architectural CI:

```text
selected:              180000
applicable/executed:   169300
pass:                  169300
fail/timeout/crash:    0 / 0 / 0
selected digest:       d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6
applicable/pass digest:6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f
failure/signature:     4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945
```

Architectural full:

```text
selected:              1562502
applicable/executed:   1474594
pass:                  1474594
fail/timeout/crash:    0 / 0 / 0
selected digest:       0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7
applicable/pass digest:4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c
failure/signature:     4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945
```

Fingerprint full:

```text
selected:              1562502
applicable/executed:   1474594
pass:                  1402202
fail:                  72392
timeout/crash:         0 / 0
pass digest:           ea521512c9f49b3a73558db6ccf0a01c6b889d1df8a82fb897a9d9d1af8316f4
failure digest:        0692676136061b956d0b7f1c06a35cfc4c5ffff7b925ba83f2d07d37310f22c5
signature digest:      79913b4f99c54d263315235829f6f937c5956268d9239a4b371301e8acbcdee8
```

Transition deltas:

```text
newly passing:     0
newly applicable:  0
newly failing:     0
changed failures:  0
```

## Validation

Completed local validation:

| Command | Exit | Result |
| --- | --- | --- |
| `build/m69-predecessor-test/sdl2/vaeg --idp-m69-status-composition` on predecessor test commit | 1 | intended predecessor failure |
| `ctest --test-dir build/m69-predecessor-test -R vaeg_idp_m69_status_composition --output-on-failure` on predecessor test commit | 8 | intended predecessor failure |
| `ctest --test-dir build/m69-predecessor-test -R vaeg_idp_m69_status_composition --output-on-failure` after fix | 0 | pass |
| `ctest --test-dir build/m69-predecessor-test -R 'vaeg_idp_m69_status_composition\|vaeg_upd9002_m68_segmented_memory\|vaeg_romless_tests' --output-on-failure` | 0 | 3 / 3 passed |
| `ctest --test-dir build/m69-predecessor-test -LE external --output-on-failure` | 0 | 70 / 70 passed |
| `ctest --test-dir build/m69-macos-asan -R 'vaeg_idp_m69_status_composition\|vaeg_upd9002_m68_segmented_memory\|vaeg_romless_tests' --output-on-failure` | 0 | 3 / 3 passed |
| `cmake --build build/m69-mingw-cross --target vaeg_sdl2 -j 4` | 0 | MinGW executable built |
| `build/m69-prod-off/sdl2/vaeg --idp-m69-status-composition` | 1 | expected production-isolation rejection |
| `tools/qa/milestone_ids.py --selftest --audit --discover` | 0 | pass |
| `tools/repo/check_encoding.py` | 0 | pass |
| `tools/repo/check_eol.py` | 0 | pass |
| `tools/repo/check_case.py` | 0 | pass |
| `git diff --check` | 0 | pass |

Compiler and platform notes:

- AppleClang build passed.
- ASan/UBSan configured with the `macos-asan` preset and passed the focused
  M69/M68/romless set.
- MinGW cross build passed.
- Wine was not available locally.
- MacPorts GCC 15 failed before M69 code evaluation in macOS SDK/libstdc++
  declarations for `at_quick_exit` and `quick_exit`; this is the same local
  toolchain class observed in the predecessor work and is not M69-specific.
- The normal predecessor build did not report a compiler warning for the
  original `tsp_i142()` conditional-expression precedence defect.

## Manual runtime acceptance

Maintainer manual runtime validation passed for the M69 manual-test candidate
on 2026-07-29.

Bound identities:

- Tested production-fix SHA:
  `6ef4f98ec1be20054db2aeb9c4a44c6a3d2e36bf`
- Tested MinGW executable SHA-256:
  `dc80b7f4ca96d6bda612c15e9b0659a6273c4a50238bcc5d4ce884fb75c41eaa`
- Local macOS executable SHA-256:
  `360d9ac746b5ba81831a2ef9b378ed722c14732a781df22fd53673e30fb8519a`

Results:

| Manual check | Result |
| --- | --- |
| PC-Engine/MS-DOS cold boot | pass |
| `DIR A:` | pass |
| `CHKDSK A:` | pass |
| multi-screen text output | pass |
| `CLS` | pass |
| demo/game | pass |
| save state | pass |
| load state | pass |
| Sound Board II | pass |

The M68 text-scroll correction remains working. No new display, timing,
state-save, sound, or application regression was observed.

## Artifact family

The deterministic artifact family is stored under:

```text
tests/idp/campaigns/g69/
```

Generation was run twice into independent temporary directories and compared
with `diff -r`; the result was byte-identical.

Key artifact digests:

- Artifact tree:
  `ca9cf6ff19af59fdd14de29beeba9226747a76a30a09787c9e9359e66b3b5767`
- Truth table:
  `92cd3035711e4127faeee98d213f4111d1e585883911710b847d61111bebfd74`
- Exhaustive composition:
  `2c133075c9aad5f987f8b84e40c5393a342dc29d7711a6ce2c496aaa05132967`
- Closure audit:
  `6bdac07f9fc4bc781dff68426b1782cf6d70eeca76a179b11c5c1d0e222ad567`

Manifest:

```text
tests/idp/campaigns/g69/manifest.json
```

## TSP exclusion and explicit nonclaims

M69 proves and corrects only the Boolean composition of the `0142H` status
byte.

M69 does not prove complete IDP timing, FIFO, IBF, OBF, ER, SC, EMEN or
light-pen behavior.

M69 does not claim that the `0142H` defect caused the M65e text-scroll
regression.

The M65e text-scroll regression was independently attributed to the segmented
word mapped-memory bypass and corrected by M68.

M69 did not modify:

- M68 segmented-word mapped-memory production code.
- IDP/TSP command timing.
- IBF/OBF behavior.
- Rendering or TVRAM mapping.
- CPU instruction semantics.
- State-save format.

## Known limitations

- Existing IDP status bits beyond BUSY remain incomplete or unimplemented in
  the current model. M69 preserves any stored value rather than inventing
  missing semantics.
- M69 verifies `0142H` byte composition and records existing word-read
  behavior, but does not define new semantics for the `0143H` high byte.
- Hosted CI evidence is supplied by the final handoff after this evidence
  commit is pushed.

## Recommended predecessor wording

Subsequent work that depends on the IDP/TSP status composition fix should use
the final G69 candidate SHA supplied by the handoff as its predecessor after
G69 human approval. Broader IDP timing, FIFO, IBF, OBF, ER, SC, EMEN and
light-pen behavior remain deferred.
