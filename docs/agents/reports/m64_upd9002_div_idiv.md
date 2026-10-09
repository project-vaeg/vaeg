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
# M64 uPD9002 DIV/IDIV and monitor-authorized 0F support

M64 corrects the complete executed `F6/F7 /6-/7` DIV/IDIV populations and
completes the requested SST-covered PC-88VA monitor-authorized 0F support:
`ADD4S`, `SUB4S`, `CMP4S`, and the expanded `TEST1`, `CLR1`, `SET1`, and
`NOT1` families.

The approved v20 dataset contains `0FFF BRKEM` metadata but no
`0FFF.json.gz` corpus shard. BRKEM therefore has zero selected and executed
cases in this gate. M64 records that exact absence without fabricating cases,
activating policy, implementing semantics, or claiming a pass.

M64 is complete and pushed. G64 is an unapproved candidate pending human
review. M65 and later milestones have not been started.

A commit cannot contain its own SHA. The evidence commit and final candidate
are therefore the commit containing this report; its exact SHA is supplied in
the maintainer handoff and is independently available from
`origin/topic/m64-upd9002-div-idiv`.

## Identity and preparation

- Approved predecessor gate: `G62`
- Exact approved G62 SHA and M64 base:
  `70b8e94e96aef4cb79eed72c7813c4148c5c0dd8`
- G62 final semantic/evaluated SHA:
  `2cdaed95072d74bbf7187ae854fb31d3886c995d`
- Approved G62 CI:
  [build 30199906912](https://github.com/nakatamaho/vaeg/actions/runs/30199906912)
- Initial primary-worktree branch: `main`
- Initial primary-worktree SHA:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty with unrelated maintainer work
- Dedicated worktree: `/home/maho/vaeg/build/m64-worktree`
- Dedicated baseline worktree: `/home/maho/vaeg/build/m64-g62-baseline`
- Dedicated worktree starting SHA:
  `70b8e94e96aef4cb79eed72c7813c4148c5c0dd8`
- M64 branch: `topic/m64-upd9002-div-idiv`
- Prospective scope-expansion SHA:
  `b5b3d169119f3d28c03703e438b2ad0a26a3a747`
- Zero-case BRKEM documentation clarification SHA:
  `fcf07c9`
- Shared audit and target-policy tooling SHA:
  `4000199737993ebec850c3409dec55d8e8392fbb`
- DIV/IDIV semantic SHA:
  `63f12b4e2bc38999efec66a43042673111e242fe`
- ADD4S/SUB4S/CMP4S semantic SHA:
  `60385167cede30a3c06e97373a92646e19021523`
- Bit-operation semantic SHA and final `evaluated_sha`:
  `99c6388df903dfc69432730cc9fa908a83946774`
- Final evidence/hosted-CI validator SHA:
  `437a8c70ed73e8dfa58d3a68393f55685ffb49c6`
- Identity-bound policy-enumeration SHA:
  `84540e251b2e9e369de38d0b42195b26089443d8`
- Complete policy-classification preservation SHA:
  `c52534f82537caab99e1aab3b0a5ddfaf7a4975b`
- Historical-validator isolation SHA:
  `cf21705f13a1791c31f8db9676178e6b2a51cd8d`
- BRKEM semantic commit: none; the governed selected population is zero
- Evidence commit/final candidate: the commit containing this report

The primary worktree was not cleaned, reset, stashed, staged, or modified.
The dedicated worktree was created directly from the fixed predecessor. The
local G62 object and
`origin/topic/m62-upd9002-semantics-bundle` both resolved to the approved
SHA. No conflicting approved G62 SHA was found.

The documentation-only first commit expanded the existing canonical M64 task;
it did not create a competing task. ROADMAP, migration master, task discovery,
branch, report path, prerequisite, and the M65 successor remain unambiguous.

## Environment, corpus, contracts, and authority

| Component | Recorded value |
|---|---|
| Host | `Linux 6.18.33.2-microsoft-standard-WSL2 x86_64` |
| Git | 2.53.0 |
| CMake | 4.2.3 |
| Ninja | 1.13.2 |
| GCC | 15.2.0 |
| Python | 3.14.4 |
| gzip command | 1.14 |
| zlib compile/runtime | 1.3.1 / 1.3.1 |

Every required profile used:

```text
/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
```

Dataset identity:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

| Contract | ID | SHA-256 |
|---|---|---|
| architectural | `upd9002-v20-architectural-v1` | `aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5` |
| fingerprint | `upd9002-v20-fingerprint-v1` | `47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e` |

Final worker SHA-256:

```text
5611c26224fd060dfdcaaca02ed3a57ce9e30156d8617eaca2d9a6fd9f593199
```

The protected G60b ROM-authority manifest remains:

```text
f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac
```

The raw monitor records are `(mask, value, group)` entries, not instruction
byte sequences. M64 implements the expanded 0F instruction encodings and
preserves the already approved `0F28 ROL4` and `0F2A ROR4` results.

The duplicated group-00 and group-01 monitor records for ADD4S, SUB4S, and
CMP4S remain separately bound in the G60b authority. The executed SST
metadata exposes one architectural opcode population per mnemonic and no
machine-readable monitor-display state that selects group 00 versus group 01.
Consequently the two authority records are not collapsed, but their
monitor-internal selection distinction remains `underdetermined`; the
executed architectural contract is complete.

The F6-mask expansions are:

```text
TEST1: 0F10 0F11 0F18 0F19
CLR1:  0F12 0F13 0F1A 0F1B
SET1:  0F14 0F15 0F1C 0F1D
NOT1:  0F16 0F17 0F1E 0F1F
```

`0FFE BRKFEM` remains present in authority and wholly outside M64.

## Approved G62 reproduction

Before semantic editing, a fresh tests-enabled build at the exact G62 SHA ran
all required profiles without skip:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,800 | 163,567 | 2,233 | 0 | 0 |
| architectural full | 1,562,502 | 1,443,594 | 1,423,202 | 20,392 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,443,594 | 1,343,614 | 99,980 | 0 | 0 |

The selected/applicable, pass/failure, and signature digests matched the
approved G62 identities exactly. The G62 policy was:

```text
upd9002-g62-6961b0f295110d32d16799cb3799bedff7600b9b956bc4ad893eebc249140212
```

All M58 through M62 validators, protected scoreboards and transitions,
immutable G43/M43 evidence, classification/taxonomy/registry state, and the
G60b authority pack also matched. No new baseline was established.

## Requested 0F classification audit

The exact G62-to-G64 state is:

| Form | Instruction | G62 classification | G62 official result | G64 result |
|---|---|---|---:|---:|
| 0F10 | TEST1 byte, CL | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F11 | TEST1 word, CL | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F18 | TEST1 byte, immediate | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F19 | TEST1 word, immediate | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F12 | CLR1 byte, CL | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F13 | CLR1 word, CL | known gap / implementation missing | not executed | 5,000 / 0 |
| 0F1A | CLR1 byte, immediate | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F1B | CLR1 word, immediate | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F14 | SET1 byte, CL | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F15 | SET1 word, CL | known gap / implementation missing | not executed | 5,000 / 0 |
| 0F1C | SET1 byte, immediate | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F1D | SET1 word, immediate | applicable | 5,000 / 0 | 5,000 / 0 |
| 0F16 | NOT1 byte, CL | known gap / implementation missing | not executed | 5,000 / 0 |
| 0F17 | NOT1 word, CL | known gap / implementation missing | not executed | 5,000 / 0 |
| 0F1E | NOT1 byte, immediate | known gap / implementation missing | not executed | 5,000 / 0 |
| 0F1F | NOT1 word, immediate | known gap / implementation missing | not executed | 5,000 / 0 |
| 0F20 | ADD4S | applicable | 909 / 91 | 1,000 / 0 |
| 0F22 | SUB4S | applicable | 696 / 304 | 1,000 / 0 |
| 0F26 | CMP4S | known gap / implementation missing | not executed | 1,000 / 0 |
| 0FFF | BRKEM | metadata only; no dataset shard | 0 selected / 0 executed | unchanged |

The seven activated selectors are exact, structural, fixed before candidate
execution, and completely enumerated in
`tests/ssts/target_policy/g64.json`. No partial or outcome-derived activation
occurred. `0F20` and `0F22` remained applicable; their 395 exact G62 failure
hashes form set `O`. Diagnostic replay of pre-activation `0F26` was used only
to derive its implementation contract and was not counted as an official G62
pass/failure population.

## Phase A — DIV and IDIV

| Form | G62 pass/fail | G64 pass/fail |
|---|---:|---:|
| F6 /6 DIV r/m8 | 2,439 / 2,561 | 5,000 / 0 |
| F6 /7 IDIV r/m8 | 1,284 / 3,716 | 5,000 / 0 |
| F7 /6 DIV r/m16 | 2,514 / 2,486 | 5,000 / 0 |
| F7 /7 IDIV r/m16 | 1,277 / 3,723 | 5,000 / 0 |

The exact 12,486-hash pre-fix failure union has SHA-256:

```text
fa23973029d0117791c5b178c576baf6d65bfc786a1da8fb84bf9561655494d8
```

The root cause is `proven`: the former signed paths relied on host signed
division and incomplete overflow checks, while successful and pre-event FLAGS
did not match the target contract. The corrected paths use widened magnitudes,
exclude zero and overflow before host division, preserve signed truncation
toward zero and dividend-signed remainder, and place quotient/remainder in
the exact byte/word registers. The word memory divisor uses explicit wrapped
byte reads.

Divide-error decision and pre-event FLAGS are M64-owned. Event-frame
placement, saved IP/CS/FLAGS, vector fetch, final target, and TF/IF behavior
remain the approved G60d machinery. The exact 214-hash M60a divide dependency
set remains green. F7 `/2`, FF `/7`, and BOUND are unchanged.

## Phase B — ADD4S, SUB4S, and CMP4S

| Form | G62 pass/fail | G64 pass/fail |
|---|---:|---:|
| 0F20 ADD4S | 909 / 91 | 1,000 / 0 |
| 0F22 SUB4S | 696 / 304 | 1,000 / 0 |
| 0F26 CMP4S | gap / not executed | 1,000 / 0 |

The root cause is `proven`. The prior implementation handled one byte and
did not honor the complete packed-decimal string contract. The corrected
handlers process the target-observed `(CL & 0x7f) + 1` byte count, walk SI/DI
under DF, propagate decimal carry or borrow through the complete string, and
update SI/DI. ADD4S and SUB4S write only the destination stream. CMP4S
performs the same decimal subtraction for flags but has no destination write.
All source/destination, alias, DF, prefix, segment-wrap, and physical-wrap
partitions are represented in the complete case tables.

The DAA/DAS helpers are reused only after their already approved G62 contract
was independently protected. `0F28` and `0F2A` were not used as oracles and
remain unchanged.

## Phase C — TEST1, CLR1, SET1, and NOT1

All sixteen expanded opcodes finish selected/applicable/executed with
5,000 pass and zero fail each. Ten forms were already applicable and green;
six exact missing forms were implemented and activated.

The byte/word and CL/immediate mapping shown in the classification table was
derived from metadata and complete case replay, not from opcode numbering
alone. CL and immediate indexes are masked to 3 or 4 bits according to the
evidenced operand width. TEST1 preserves the operand and changes only the
target-observed zero result. CLR1, SET1, and NOT1 write exactly the selected
register or memory bit and preserve FLAGS. Register alias and wrapped memory
forms are green. The root causes for the six missing dispatch/handlers are
`proven`.

## Phase D — BRKEM coverage boundary

The approved dataset manifest contains no `0FFF.json.gz` shard. The exact
machine-readable conclusion is:

```text
compatibility_scope = no_v20_sst_cases
sst_contract_status = not_yet_present
selected = 0
executed = 0
silicon_mode_identity = underdetermined
```

This is an accepted maintainer-supplied dataset state, not a fixture defect.
No BRKEM semantic handler or target-policy activation was added, and zero
coverage is not described as passing. Executable BRKEM semantics remain
pending an approved content-addressed corpus. BRKFEM, RETEM, CALLN, and
continued 8080/Z80 mode execution are untouched.

## Target-policy transition

The content-addressed G64 policy is:

```text
upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6
```

Policy artifact SHA-256:

```text
44b977576e62a9b776115ab2cce7ab4677966c116c80d81faad51485cadea083
```

| Scope | Selected before/after | Applicable before | Applicable after | Newly applicable |
|---|---:|---:|---:|---:|
| CI | 180,000 / 180,000 | 165,800 | 169,300 | 3,500 |
| full | 1,562,502 / 1,562,502 | 1,443,594 | 1,474,594 | 31,000 |

| Scope | Selected SHA-256 | Applicable before SHA-256 | Applicable after SHA-256 |
|---|---|---|---|
| CI | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `440a621dea647cf11a4e8b834fc139c2c95f6081f294d717263ba8f42eb2a750` | `6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f` |
| full | `0aa3db24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` | `4e8cf0af125f3d8404912311fc18fc3c75952c4c27215256ae7dd983d095cdff` | `4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c` |

Only `0F13`, `0F15`, `0F16`, `0F17`, `0F1E`, `0F1F` (5,000 hashes each)
and `0F26` (1,000 hashes) transition from
`known_target_gap/implementation_missing` to `applicable`. The newly
applicable full-set SHA-256 is:

```text
90debb1ada9f7b631c92d7681dda6870dd84420f5a7d5e5a0c26f513fb5ea6c5
```

All 31,000 execute and pass. `implementation_missing` decreases from 36,908
to 5,908; `documented_silicon_absent` remains 32,000. No other
classification, gap kind, approved-divergence entry, or hardware-pending
entry changes.

## Final arithmetic and ratchet

| Governed set | Count | SHA-256 |
|---|---:|---|
| `D`: G62 DIV/IDIV failures | 12,486 | `fa23973029d0117791c5b178c576baf6d65bfc786a1da8fb84bf9561655494d8` |
| `O`: requested existing-applicable 0F failures | 395 | `3169f5f37e1a05f00cb6c0ce2e35b9ebb87aa3f20fe3c05226c77877447b38d9` |
| `L`: newly applicable requested 0F records | 31,000 | `90debb1ada9f7b631c92d7681dda6870dd84420f5a7d5e5a0c26f513fb5ea6c5` |
| existing-applicable newly passing | 12,881 | `050eb0f43e831cd2c9cba99391ff507816a673242bcd28a32a55d4c5d691ade0` |
| newly failing | 0 | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` |

The required full arithmetic is reached exactly:

```text
candidate failures = 20,392 - |D| - |O|
                   = 20,392 - 12,486 - 395
                   = 7,511

candidate pass = 1,423,202 + |D| + |O| + |L|
               = 1,423,202 + 12,486 + 395 + 31,000
               = 1,467,083
```

`L` is newly applicable passing coverage, not failure reduction. There are
zero newly failing hashes and zero changed surviving failure signatures.

## Final profiles

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash | Wall time |
|---|---:|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 169,300 | 168,531 | 769 | 0 | 0 | 180.0 s |
| architectural full | 1,562,502 | 1,474,594 | 1,467,083 | 7,511 | 0 | 0 | 424.7 s |
| fingerprint full | 1,562,502 | 1,474,594 | 1,394,692 | 79,902 | 0 | 0 | 518.6 s |

The wall times are noncanonical execution metadata reconstructed from the
preserved sequential raw-output boundaries. Profile content identity is
governed by the following canonical digests:

| Profile | Pass SHA-256 | Failure SHA-256 | Signature SHA-256 |
|---|---|---|---|
| architectural CI | `388c5d3a8d7b78baeefdfcc724bd9b63e4de0798f30d3acb278ab0fef6203bf1` | `c92cb4f0413d743371952de12ad043e5f4b8de72afcf400ab823c108097db960` | `6c3905021b9f8671aecaba41a475e1eda3a6778be531cf3d1dfbe55ccdaf3968` |
| architectural full | `a12fb6f695240db0c975179e37841aae4147f750ff982b6c1af0c4eb40a22f64` | `d504fa09678568a226a6e2214caa0783462700010a7e8d953c199d830025592b` | `593a0943a721701929b74f57ca13b47ba373f88bae81d74383d9ef5ad0657bb2` |
| fingerprint full | `9299d0d149c889649875a0e4a80146ab94de3aaefe9000b9cfc3e9fbadbfd59b` | `de838567da25c76d41bb4e9adc643e07b148176768106a19bdf6d0b6922f2cb4` | `6a74aa377448dfb7f3f7559c646572cdb8737836f0a08cb505ef80d404ebb091` |

Full termination totals are 1,461,895 normal and 12,699 type-0 executions.
Timeout and crash are zero in every profile.

## Protected behavior

The final worker preserves:

| Form/family | Final architectural result |
|---|---:|
| FF `/7` | 0 / 5,000 |
| F7 `/2` | 3,887 / 1,113 |
| BOUND | 3,756 / 1,244 |
| 0F28 ROL4 | 5,000 / 0 |
| 0F2A ROR4 | 5,000 / 0 |
| D4 AAM | 5,000 / 0 |
| D5 AAD | 5,000 / 0 |
| DAA/DAS/AAA/AAS | each 5,000 / 0 |
| C0/C1/D2/D3 shift and rotate forms | green |
| C6/C7 MOV immediate | each 5,000 / 0 |
| CF IRET | 5,000 / 0 |

The G60d residual-frame set remains empty, the exact M60a 214-case divide
dependency remains green, and interrupt entry/IRET are unchanged. BRKFEM,
RETEM, CALLN, 6C-6F, 66/67, FPO/FPU, fixtures, selected sets, and comparison
contracts are untouched.

## Semantic and changed-file audit

Relative to G62, the final evaluated SHA changes only:

- prospective M64 scope documents;
- M64 audit schemas, focused tests, and policy tooling;
- `cpu/upd9002/upd9002_dispatch.c`;
- narrow test-state initialization in `sdl2/np2.c`.

Every changed production line is classified as:

```text
DIV arithmetic
IDIV arithmetic
divide-error decision
DIV/IDIV result placement
DIV/IDIV pre-event FLAGS
ADD4S semantics
SUB4S semantics
CMP4S semantics
CLR1 semantics
SET1 semantics
NOT1 semantics
```

TEST1 was already implemented and remains protected. No production line is
owned by BRKEM or another family.

Later validator-only commits add G64 evidence generation/CI checks, preserve
complete policy enumerations, and isolate exact M64 graph/support/harness and
four trace-FLAGS transitions from immutable older baselines. The detectors
continue to reject every change outside those explicit sets. They do not
alter the final worker or any profile-governing input.

Semantic review:

```text
git diff --stat \
  70b8e94e96aef4cb79eed72c7813c4148c5c0dd8...\
99c6388df903dfc69432730cc9fa908a83946774
git diff --name-status \
  70b8e94e96aef4cb79eed72c7813c4148c5c0dd8...\
99c6388df903dfc69432730cc9fa908a83946774
git diff \
  70b8e94e96aef4cb79eed72c7813c4148c5c0dd8...\
99c6388df903dfc69432730cc9fa908a83946774 \
  -- cpu/upd9002/
```

## Evidence, checkpoints, transitions, and ranking

- Evidence manifest:
  `tests/ssts/evidence/g64/manifest.json`
- Evidence-manifest SHA-256:
  `4a381aaa56fed971320c7dec8362be90b7442bcafd1be430a81a567eda232d7c`
- Result manifest:
  `tests/ssts/evidence/g64_result_manifest.json`
- Artifact-tree SHA-256:
  `a8cebbaaa27c9288d2b0cb786bf3705ce3c4e18dc6bff64b8c6f194fe8315549`
- Architectural CI transition:
  `tests/ssts/transitions/g64_architectural_ci_from_g62.json`
- Architectural CI transition SHA-256:
  `f582edf01f309891050f7832aa654ccb250f2f5535e3979687a6572e1d611076`
- Architectural full transition:
  `tests/ssts/transitions/g64_architectural_full_from_g62.json`
- Architectural full transition SHA-256:
  `f858dce8f65f17096dce885c4103b262004a00c3f8386ade393eab46841396c0`
- Complete ranking:
  `tests/ssts/rankings/g64_architectural_full.json`
- Ranking SHA-256:
  `0a7874346f17ab84379b0507d0ff2722ab3bd52ef903d78a442f69051d096e91`
- Human top 30:
  `tests/ssts/rankings/g64_architectural_full.md`

Phase checkpoints:

| Phase | Path | SHA-256 |
|---|---|---|
| DIV/IDIV | `tests/ssts/evidence/g64/phases/phase_div_idiv.json` | `89d290072ada2e096f2d92d4997a9c1811f63aa6f6d6f04a8d39394fda2bc6c0` |
| ADD4S/SUB4S/CMP4S | `tests/ssts/evidence/g64/phases/phase_add4s_sub4s_cmp4s.json` | `56469d29f6c594f6c33fc1ae2af1bcff7381b8a97ac07fe4987dcdb00c3ec19b` |
| bit operations | `tests/ssts/evidence/g64/phases/phase_bit_operations.json` | `919cc5b45f3e992aa01f19a16a63e8a1de372f0efcc1242db83f249a1530f8e8` |
| BRKEM zero coverage | `tests/ssts/evidence/g64/phases/phase_brkem.json` | `5032f81051491d9fbe966ee31a9958f37ea2d71d7d4a8bd6392029c1e76b8fae` |

The complete evidence family was generated twice from the same preserved raw
profiles, final phase audits, exact policy, evaluated SHA, and worker. Both
trees were byte-identical. JSON is canonical, ordered, and timestamp-free;
gzip uses the deterministic repository writer.

The fresh ranking reconciles exactly to 7,511 failures. The leading residuals
are FF `/7`=5,000, BOUND=1,244, F7 `/2`=1,113, and FF `/6`=144. Every M64
form has an explicit zero-failure row; BRKEM is separately recorded as zero
coverage. Omission from the human top list is never interpreted as pass.

## Validation and execution discipline

Candidate raw profiles were preserved at:

```text
/home/maho/vaeg/build/m64-results/final-99c6388/
```

Final phase audits were preserved at:

```text
/home/maho/vaeg/build/m64-audits/final-99c6388/
```

The profiles were executed once against the exact evaluated worker and were
reused only for deterministic evidence serialization. The evaluated SHA,
worker digest, dataset, contracts, final target policy, selected sets, and
applicable sets were unchanged by later validator/report work. No expensive
profile was rerun merely for evidence placement.

The first non-external native-suite pass identified seven fixed historical
detectors that still treated M64-owned transitions as unrelated changes.
The validator-isolation commit authorizes only seven exact 0F
graph/support/harness replacements and four exact DIV/IDIV trace FLAGS lines.
Those seven focused tests passed 7/7, followed by the complete non-external
suite. This separates the intended mismatch from its surrounding protected
state; it does not weaken unrelated rejection behavior.

Principal commands, all with exit status zero:

```text
cmake -S . -B build/linux-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVA_BUILD_TESTS=ON
cmake --build build/linux-debug -j2
ctest --test-dir build/linux-debug --output-on-failure -LE external

python3 tools/qa/upd9002_m64_expanded.py selftest
python3 tools/qa/upd9002_m64_expanded.py verify-static --root .
python3 tools/qa/upd9002_m64_expanded.py verify-evidence \
  --root . --dataset-root <verified-corpus>
python3 tools/qa/milestone_ids.py --selftest --audit --discover

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root <verified-corpus> \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map <generated-g64-support-map> \
  --worker build/linux-debug/sdl2/vaeg \
  --profile ci --flags-comparison defined \
  --output <architectural-ci-raw> \
  --failure-directory <architectural-ci-failures>

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root <verified-corpus> \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map <generated-g64-support-map> \
  --worker build/linux-debug/sdl2/vaeg \
  --profile full --flags-comparison defined \
  --output <architectural-full-raw> \
  --failure-directory <architectural-full-failures>

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root <verified-corpus> \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map <generated-g64-support-map> \
  --worker build/linux-debug/sdl2/vaeg \
  --profile full --flags-comparison all16 \
  --output <fingerprint-full-raw> \
  --failure-directory <fingerprint-full-failures>

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

The final hosted CI is launched once after the evidence-only commit containing
this report is pushed. Its GitHub-assigned URL and successful conclusion are
supplied in the maintainer handoff rather than self-referenced inside the
report. Hosted CI is not used as an iterative debugger.

## Known limitations

- This is SST-observed V20 compatibility behavior, not complete uPD9002
  silicon validation.
- The monitor-internal group-00/group-01 display-selection distinction for
  ADD4S/SUB4S/CMP4S is not represented in SST architectural state.
- BRKEM has no v20 SST shard in this dataset; no executable compatibility or
  silicon mode identity is claimed.
- Fingerprint-only differences outside the architectural contract remain
  diagnostic and did not weaken or broaden M64.
- FF `/7`, F7 `/2`, BOUND, and other ranking residuals are outside M64.
- M65 has not been started; the fresh ranking does not authorize it.
- The hosted CI URL is external post-commit evidence and is reported in the
  final handoff.

## Human review

```text
git fetch origin
git switch topic/m64-upd9002-div-idiv
git status --short
git log --oneline --decorate \
  70b8e94e96aef4cb79eed72c7813c4148c5c0dd8..HEAD
git diff --check
git diff --stat \
  70b8e94e96aef4cb79eed72c7813c4148c5c0dd8...\
99c6388df903dfc69432730cc9fa908a83946774
git diff \
  70b8e94e96aef4cb79eed72c7813c4148c5c0dd8...\
99c6388df903dfc69432730cc9fa908a83946774 \
  -- cpu/upd9002/
sha256sum \
  tests/ssts/evidence/g64/manifest.json \
  tests/ssts/target_policy/g64.json \
  tests/ssts/transitions/g64_architectural_ci_from_g62.json \
  tests/ssts/transitions/g64_architectural_full_from_g62.json \
  tests/ssts/rankings/g64_architectural_full.json
python3 tools/qa/upd9002_m64_expanded.py selftest
python3 tools/qa/upd9002_m64_expanded.py verify-static --root .
python3 tools/qa/upd9002_m64_expanded.py verify-evidence \
  --root . --dataset-root <verified-corpus>
python3 tools/qa/milestone_ids.py --selftest --audit --discover
cmake -S . -B build/review -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVA_BUILD_TESTS=ON
cmake --build build/review -j2
ctest --test-dir build/review --output-on-failure -LE external
git rev-parse HEAD
git rev-parse '@{u}'
git ls-remote origin refs/heads/topic/m64-upd9002-div-idiv
```
