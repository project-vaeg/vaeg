# Open issue: VA2/VA3 BASIC /87 and the 8087 exception route

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Status: **OPEN**

GitHub issue: [#8 — 8087: VA2/VA3 BASIC /87 local-QA boundary and BUSY/INT route](https://github.com/nakatamaho/vaeg/issues/8)

## Newly established guest target

The VA2/VA3 main-ROM `/87` path is the production guest target for this issue.
The user supplied that selecting `/87` starts the 8087-using N88 Japanese BASIC
V3.1 path, and that this path applies to VA2/VA3 only. The ROM fixture remains
outside the VAEG repository and is not copied into it.

A self-authored direct-8087 probe has now passed through this target. See
[the VA2 ASM probe record](va2-basic87-asm-probe.md) for the reproducible
source and the observed short-real/long-real `2.0` stores. This qualifies the
local `/87` machine-language seam only; it does not qualify the physical
BUSY/INT route. The N88 BASIC FAC conversion path is now qualified for the
supplied arithmetic workload; the remaining hardware boundary is recorded
below.

## Clarification of the BASIC `/87` failure boundary

The result must not be described as “the `/87` startup is missing a required
conversion routine.” The supplied VA2 ROM contains the `/87` arithmetic
dispatch and executes real 8087 `FLD`, arithmetic, and `FSTP` instructions. The
self-authored ASM workload proves that the local 8087 instruction path can
read and write IEEE short-real/long-real memory operands. The initial numeric
mismatch was instead caused by a local dispatcher defect in the `DC`/`DE`
register-form operation/reversal argument order; that defect is corrected and
covered by focused core tests.

Before that correction, a runtime memory probe found that the VA2
`1040:A00` five-byte service-slot area is initialized with `RETF`
bytes, including the slot at `1040:D57`, in both ordinary BASIC and `/87` runs.
That observation establishes the local slot state, but not the intended owner
or ABI of each slot: `D57` may be a patchable service hook, an intentionally
empty entry, or only one part of a conversion path elsewhere in the guest.
It is therefore not evidence that a specific ROM routine is absent.

The reproducible qualification boundary is consequently:

```text
direct embedded 8087 mnemonics: PASS
N88 BASIC /87 numeric arithmetic (supplied X87TEST workload): LOCAL SOFTWARE QA PASS
FAC-to-IEEE conversion path: IDENTIFIED AND LOCALLY VERIFIED
```

No host-floating-point shortcut or address-only guest interception was used;
the closure above follows the guest buffer format and ROM calling sequence.

## BASIC /87 numeric closure after register-form repair

The ROM path is now traced from the BASIC FAC buffer to the 8087 memory
operands. At runtime the FAC packed-BCD bytes for `A=1.5` are stored at the
neutral physical address `037DE` as `00 00 00 00 00 00 00 00 15 00`. The
conversion entry at runtime `E000:B85C` executes `FBLD`; the active
`varom00` bank is bank 1. The helper derives the signed FAC exponent and uses
the extended-real, long-real, and short-real decimal constants around the
`B8DA`/`B919` helper, including `DC F9` (`FDIV ST(1),ST(0)`), to remove the
packed-BCD decimal scale. The resulting stores are:

```text
A internal long-real:  3FF8000000000000 (1.5)
A internal short-real: 3FC00000 (1.5)
B internal long-real:  4002000000000000 (2.25)
B internal short-real: 40100000 (2.25)
A+B short-real:        40700000 (3.75)
```

The full ASCII `X87TEST.BAS` program was loaded under `BASIC /87`, run, and
displayed the expected values for ADD, MUL, DIV, SQR, LOG, EXP, SIN, COS, TAN,
and ATN, ending in `Ok`. It was also saved as ASCII with
`SAVE "X87COPY.BAS",A`, reloaded, and run successfully. Separately, `NEW 87`
followed by `A=1.5`, `B=2.25`, and `PRINT A+B` displayed `3.75`. Plain BASIC
continued to display the same expected workload values with the 8087 disabled.

This is local software/guest-seam evidence for the supplied workload, not a
claim about all possible BASIC programs or the physical VA2 signal route.

## `NEW 87` runtime distinction

The VA2 BASIC path also accepts `NEW 87` as an in-session 8087 feature command.
In the same local VA2 run, `NEW /87` produced `Syntax error`, while `NEW 87`
returned `Ok`. With the optional NDP disabled, `NEW 87` returned `Feature not
available` and the subsequent `A=1.5`, `B=2.25`, `PRINT A+B` workload produced
the ordinary `3.75`. Before the register-form repair, the NDP-enabled run
selected the 8087 arithmetic path and reproduced the diagnostic `3.75E+17`
result; after the repair the same workload displayed `3.75`. This gives a
second guest entry path alongside DOS startup with `BASIC /87`; it does not by
itself prove that the two entry paths have identical initialization.

The service-slot probe performed after `NEW 87` still found `RETF` bytes at the
`1040:A00` five-byte slots, including `1040:D57`. Therefore the command changes
the BASIC arithmetic mode without, on the observed run, installing code into
those slots.

## ROM operation inventory and local BASIC results

The supplied VA2 ROM dump is sufficient to identify the arithmetic families used
by the `/87` path, provided the bytes are interpreted as code in their known
bank/entry context rather than as an undirected opcode scan. The dump shows the
following direct 8087 execution sites:

- The bank-7 BASIC arithmetic dispatch at `E000:19CB`--`E000:1AD7` uses
  `FLD`/`FADD`/`FSUB`/`FMUL`/`FDIV`/`FSTP` for long-real operands, and
  `E000:1ADA`--`E000:1BD5` contains the corresponding short-real paths.
- The same bank's unary and conversion dispatch at `E000:1BE0`--`E000:1DE0`
  loads short/long real values, invokes the operation selector, and stores the
  result with `FSTP`. The surrounding code also contains `FILD`, `FSTCW`,
  `FLDCW`, `FRNDINT`, and status-word handling.
- The long-real remainder helper at `E000:27E2`--`E000:2805` executes a
  repeated `FPREM` and tests the 8087 status word. Comparison helpers use
  `FCOMP` for both long- and short-real operands.
- The bank-1 transcendental routines contain `FSQRT` at `F000:A8EB`, range
  reduction using `FPREM` around `F000:A91F`, `FPTAN` around `F000:A9FB`,
  `FPATAN` at `F000:AD1B` and `F000:AD4C`, `FYL2X` at `F000:AD8F` and
  `F000:AF80`, `F2XM1` at `F000:AE59`/`F000:AE72`, and `FSCALE` at
  `F000:AE7B`. This is the expected 8087-style implementation of the
  square-root, logarithm, exponential, sine/cosine, tangent, and arctangent
  BASIC functions; there is no need to infer those operations from output alone.

The local BASIC workload then gives the guest-visible qualification result. The
program uses `A=1.5`, `B=2.25`, `SQR(2)`, `LOG(2)`, `EXP(1)`, `SIN(.5)`,
`COS(.5)`, `TAN(.5)`, and `ATN(1)`:

```text
operation   plain BASIC   BASIC /87
ADD         3.75          3.75E+17
MUL         3.375         3.375E+34
DIV         1.5           1.5
SQR         1.41421       1.41421
LOG         .693147       .693147
EXP         2.71828       5.43656
SIN         .479426       4.37394
COS         .877583       1
TAN         .546302       1.65955
ATN         .785398       4.14159
```

Thus the ROM evidence answers which operation classes call the 8087, while the
guest run answers whether the complete BASIC value path is correct. The current
result is mixed: several unary operations happen to produce the expected value,
but addition, multiplication, exponential, and trigonometric results do not.
This is consistent with an unresolved FAC-to-IEEE/service-boundary problem; it
is not evidence that those 8087 opcodes are absent or that the ROM uses only a
V30 fallback.

## Guest-vector evidence

The user supplied a BNN technical-manual reference (p. 15) stating that
interrupt number `16h` is reserved for the NDP on 8259-2. This narrows the
guest-side vector target for the VA2/VA3 NDP path to `INT 16h`. It does not by
itself identify the 8087 INT pin's physical 8259-2 input, signal polarity,
edge/level behavior, acknowledgement sequence, or the corresponding handler
entry evidence; those remain separate physical-route gates.

This corrects the earlier classification of the target as a generic external
`87BASIC` program. The relevant target is the VA2/VA3 firmware-selected BASIC
path.

## What can be verified

The external ROM fixture can provide a production guest workload for:

- 8087 presence/enablement and `/87` startup behavior;
- real ESC/D8--DF instruction dispatch from the BASIC path;
- arithmetic, conversion, transcendental, `WAIT/FWAIT`, and service-time
  behavior exercised by the guest;
- whether the BASIC path installs, unmasks, and reaches an 8087 exception
  handler when a deterministic unmasked exception is produced.

The guest-handler result must be recorded separately as one of:

```text
BASIC87_GUEST_HANDLER_REACHED
BASIC87_GUEST_HANDLER_NOT_EXERCISED
BASIC87_GUEST_HANDLER_NOT_REACHED
BASIC87_GUEST_HANDLER_ROUTE_UNVERIFIED
```

Normal `/87` startup or valid arithmetic alone does not prove that an INT
signal was generated. A masked exception, guest status polling, or an error
path that never raises an unmasked 8087 exception may legitimately leave the
handler unexercised.

## What remains unresolved

This guest target does not establish the VA2 PCB net from IC19.23 BUSY or
IC19.32 INT. The following remain unknown:

- the physical BUSY and INT destinations and any intervening VA2 glue;
- signal polarity, inversion, level/edge behavior, and masking;
- the actual PIC input and vector on VA2/VA3;
- source-condition clearing versus PIC acknowledgement/EOI;
- whether the production BASIC handler's behavior corresponds to the physical
  route or only to the repository's provisional virtual route.

The current VAEG synthetic route may be used to observe guest behavior, but it
must be labeled provisional and cannot close the VA-specific route issue.
Consequently:

```text
guest BASIC /87 qualification: CONDITIONAL / OPEN
VA2/VA3 PCB BUSY/INT route: UNKNOWN
P17: INTEGRATION_BLOCKED
P19: INTEGRATION_BLOCKED
```

The software-side integration is nevertheless suitable for local QA: the SDL2
boot-model menu now provides `VA`, `VA2/VA3`, and `VA2/VA3 + 8087` profiles.
The first two disable the optional NDP and the third enables the single
machine-owned 8087, with the existing persisted clock and reset-time
application path. This menu integration is tracked as the follow-up boundary
for [GitHub Issue #8](https://github.com/nakatamaho/vaeg/issues/8); it does not
close the physical-route questions above.

## Closure evidence required

Close the guest portion with a reproducible VA2/VA3 `/87` run that records
neutral run identifiers, model, 8087 configuration, observed 8087 dispatch,
exception-mask state, guest vector/handler entry if exercised, handler clear
operation, return, and final result. Close P17/P19 only after separately
approved VA evidence establishes the physical BUSY/INT route, polarity,
controller input/vector, and acknowledgement behavior.
