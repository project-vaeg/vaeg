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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M70 - uPD9002 REPC/REPNC prefix and string-instruction closure

## Fixed predecessor

M70 starts from the formally approved G69 candidate:

`680308a603b24341c5b9649657f01791b79002f7`

Approved predecessor gate: `G69`

Approved G68 SHA:
`d1e0225c4edb716893fe5579283fbf0915db72b9`

Approved G69 hosted CI:
`https://github.com/nakatamaho/vaeg/actions/runs/30375275853`

The approved G69 lineage contains the complete approved M68 history.

Branch:
`topic/m70-upd9002-prefix-string-closure`

Commit prefix: `M70:`

Candidate gate: `G70`

Report:
`docs/agents/reports/m70_upd9002_prefix_string_closure.md`

Do not merge M70 to `main`. Do not start M71. Do not declare G70 passed.

## Scope

M70 is one indivisible milestone with one terminal human gate. Internal
checkpoints may be committed separately, but no selector subset, decoder
phase, string operation, repeat condition, negative-protection result, or
restart behavior may be independently declared complete or approved.

M70 implements a maintainer-approved project-target closure for exactly the
canonical SST-present `64H` / `65H` prefix plus primary string-opcode
population. M70 does not claim complete physical uPD9002 silicon proof.

All newly authored source code, comments, identifiers, tests, artifacts,
reports, task text, and commit messages must be in English.

## Dataset and contracts

Dataset:

`ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4`

Architectural contract:

- ID: `upd9002-v20-architectural-v1`
- digest: `aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5`

Fingerprint contract:

- ID: `upd9002-v20-fingerprint-v1`
- digest: `47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e`

Predecessor target policy:

`upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`

M70 must create a new content-addressed target policy. Do not mutate the G64
policy artifact and do not reuse the old policy ID for the final M70 policy.

## Owned population

M70 owns exactly the canonical SST-present population established by the M65j
amendment and consolidated by G67:

- selector groups: `19`
- owned hashes: `5908`
- population digest:
  `240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5`

Owned selector groups:

- `REPC + A4`
- `REPC + A5`
- `REPC + A6`
- `REPC + A7`
- `REPC + AA`
- `REPC + AB`
- `REPC + AC`
- `REPC + AD`
- `REPC + AF`
- `REPNC + A4`
- `REPNC + A5`
- `REPNC + A6`
- `REPNC + A7`
- `REPNC + AA`
- `REPNC + AB`
- `REPNC + AC`
- `REPNC + AD`
- `REPNC + AE`
- `REPNC + AF`

`REPC + AE` is absent from the canonical population. Do not synthesize a
twentieth group, invent hashes, count zero execution as passing, or make a
conformance claim for the absent selector. Corpus absence alone is not
silicon absence.

## Policy amendment

M70 supersedes the G67 current-state disposition for exactly the 19 owned
records:

- from `known_target_gap / target_support_unverified`
- from non-applicable, unimplemented, unexecuted, no passing claim
- to maintainer-approved, milestone-owned implementation authority

At terminal closure every owned hash must be selected, applicable, executed
through production semantics, and architecturally passing. No owned hash may
remain deferred, non-applicable, unexecuted, `target_support_unverified`, or
`approved_nonblocking_defer`.

This amendment does not authorize generic V20/V30 compatibility, i286/i386
compatibility, FS/GS interpretation for `64H` or `65H`, unrelated reserved
opcodes, or fingerprint diagnostics as architectural authority.

## Required architecture

Implement one coherent uPD9002 prefix and string execution model. Do not
create 19 selector-specific implementations.

The architecture must separate:

1. prefix collection;
2. prefix normalization;
3. primary-opcode decode;
4. one-element string semantics;
5. repeat continuation;
6. per-iteration architectural commit;
7. interrupt, exception, and restart handling;
8. trace and comparison presentation.

For the approved project target:

- `64H` means `REPNC`.
- `65H` means `REPC`.
- Neither byte is an i386 FS or GS override.

The implementation must preserve raw prefix bytes in trace or diagnostics and
must derive effective behavior through general decoding rules, not hashes or
fixtures.

## String semantics

Provide shared one-element semantics for:

- `A4 MOVSB`
- `A5 MOVSW`
- `A6 CMPSB`
- `A7 CMPSW`
- `AA STOSB`
- `AB STOSW`
- `AC LODSB`
- `AD LODSW`
- `AE SCASB`
- `AF SCASW`

The shared implementation must correctly handle byte and word widths, AL/AX,
SI/DI/CX, DF, flags, zero count, read-before-write ordering, segment
selection, segment overrides supported by the existing architecture, normal
RAM, TVRAM, BMS and mapped-memory dispatch, segment-offset word wrapping, and
partial state after each completed iteration.

Preserve the M68 abstraction boundary:

- segmented helpers own address formation and segment-offset wrapping only;
- the canonical memory API owns RAM, TVRAM, BMS, device routing, callbacks,
  side effects, dirty tracking, and fast-path selection.

Do not reintroduce direct flat `mem[]` routing into string execution or
segmented helpers.

## REPC and REPNC repeat semantics

Implement `REPC` and `REPNC` as explicit target repeat modes. Determine and
document the general rule from canonical evidence and executable results,
including first-element execution, carry-condition timing, CX decrement,
non-compare flag preservation, early termination, counter exhaustion, zero
count, final IP, FLAGS, SI/DI, and memory state.

Do not infer behavior from instruction names alone when executable evidence
provides stronger authority. Do not gate production behavior on a case hash or
expected final state.

## Interrupt, exception, and restart

Repeated string execution must be a sequence of architecturally observable
iterations. Integrate with the existing uPD9002 interrupt and exception
architecture. Do not create an independent M70-only restart model.

If a test-only seam is required, it must be disabled in production,
deterministic, fail closed when misconfigured, and avoid exposing expected SST
results or a second execution path.

## Negative protection

M70 owns a blocking negative-protection contract for:

- `64 6C`
- `64 6D`
- `64 6E`
- `64 6F`
- `65 6C`
- `65 6D`
- `65 6E`
- `65 6F`

These pairs remain outside executable M70 semantics. They must not enter the
string engine, INM/OUTM semantics, I/O cycles, repeat-loop mutation, or the
5,908-hash denominator. Their reserved behavior remains evidence-pending and
requires a later maintainer-approved policy amendment before implementation.

## Required evidence

M70 must add deterministic directed tests before production closure. They must
cover the owned string operations, both prefix bytes, zero and nonzero CX,
DF=0 and DF=1, SI/DI updates and wrapping, aligned and unaligned words,
`FFFFH -> 0000H` segment wrapping, segment selection, normal and mapped
memory, compare and non-compare flags, early termination, full exhaustion,
restart or interrupt boundaries, repeated or mixed prefixes present in the
corpus, and all eight negative-protection pairs.

M70 must reconstruct the 19-group, 5,908-hash population from authoritative
repository inputs before production implementation. Reconstruction must fail
closed on any group, hash, digest, classification, or membership mismatch.

Run the complete owned architectural and fingerprint campaigns, the global
architectural CI/full and fingerprint full profiles, M65a-M65e protection,
M68 protection, M69 protection, native CTest, platform builds, sanitizer
builds where supported, repository invariant checks, deterministic double
generation, anti-cheating audit, and hosted CI against the exact candidate.

## Artifacts and report

Create deterministic G70 campaign artifacts under:

`tests/ssts/campaigns/g70/`

Create the G70 divergence registry generation under:

`tests/ssts/divergence/g70/`

Create a new content-addressed target policy under:

`tests/ssts/policies/`

Write:

`docs/agents/reports/m70_upd9002_prefix_string_closure.md`

The terminal report must distinguish maintainer-approved project target,
physical-silicon evidence, architectural contract, and fingerprint diagnostic
results.

## Manual gate

Produce a manual-test candidate from the exact evaluated production SHA. The
maintainer must cold-boot PC-Engine/MS-DOS and test `DIR A:`, `CHKDSK A:`,
multi-screen output, `CLS`, a demo/game, save state, load state, Sound Board
II, and any available guest exercise for `REPC`, `REPNC`, `MOVS`, `CMPS`,
`STOS`, `LODS`, `SCAS`, early carry-condition termination, and full CX
exhaustion.

If maintainer validation is not yet available, stop at `M70 manual-test
candidate`. Do not create terminal evidence, run terminal hosted CI, request
G70 review, or declare G70 passed before manual acceptance is bound to the
exact evaluated SHA and executable digest.

## Closure language

At handoff write either:

```text
G70 candidate ready for human review; not self-approved.
```

or:

```text
G70 blocked.
```

Do not write `G70 passed`.
