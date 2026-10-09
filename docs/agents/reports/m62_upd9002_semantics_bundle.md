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
# M62 uPD9002 consolidated semantics bundle

M62 corrects the executed SST-observed behavior of `D4 AAM`, `0F2A ROR4`,
`0F28 ROL4`, `27/2F/37/3F` decimal and ASCII adjust, and the
`C0/C1/D2/D3 /4-/7` shift forms. It activates the complete ROM-authorized
`0F28` population and no other target-policy population.

M62 consolidates the previously prospective M62a, M62b1, M62b2, M62c, and
M63 tasks under one maintainer-approved gate. Each phase remains independently
reviewable through separate semantic commits and content-addressed evidence.

M62 is complete and pushed. G62 is an unapproved candidate pending human
review. M64 and later milestones have not been started.

A commit cannot contain its own SHA. The evidence commit and final candidate
are therefore the commit containing this report; its exact SHA is supplied in
the maintainer handoff and is independently available from
`origin/topic/m62-upd9002-semantics-bundle`.

## Identity and preparation

- Approved predecessor gate: `G61`
- Exact approved G61 SHA and M62 base:
  `829f314bb0d363ec5b6e9aa738e948b1a3adb365`
- G61 semantic implementation/evaluated SHA:
  `90fa7dec5d46708a807851f61ae0792ee39e9b8f`
- Approved G61 CI:
  [build 30188522466](https://github.com/nakatamaho/vaeg/actions/runs/30188522466)
- Initial primary-worktree branch: `main`
- Initial primary-worktree SHA:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty with unrelated maintainer work
- Dedicated worktree: `/home/maho/vaeg/build/m62-worktree`
- Dedicated baseline worktree: `/home/maho/vaeg/build/m62-g61-baseline`
- Dedicated worktree starting SHA:
  `829f314bb0d363ec5b6e9aa738e948b1a3adb365`
- M62 branch: `topic/m62-upd9002-semantics-bundle`
- Prospective consolidation SHA:
  `c87d91f1016f574b3e40f541bd5b7e7fb25dfdd9`
- Shared audit/tooling SHA:
  `f98961c929080265ddbceee2f4619520fb131957`
- D4 AAM semantic SHA:
  `c55e57305052b2670f0edf4f1e9bda6041cb0c80`
- 0F2A ROR4 semantic SHA:
  `e74d814f4397a5d832e7fbef675a93df4160bb2f`
- 0F28 ROL4 semantic/policy SHA:
  `f77197c1f11cd2c28e7c7df8f37df3ab9ec472ba`
- DAA/DAS semantic SHA:
  `33bec0078328fdaf6612188b6341c6e938f6dcb6`
- AAA/AAS semantic SHA:
  `bfd9710bdac52ec5092871a2f5595a34212df1f2`
- Shift semantic SHA and final `evaluated_sha`:
  `2cdaed95072d74bbf7187ae854fb31d3886c995d`
- Historical-validator isolation SHA:
  `40a6b3ae62cc219410ac1f2aeb2d084fe0e57cd4`
- Permanent bug-fix ledger SHA:
  `c302550fed36d2644516551d799d4b43bbe2573b`
- Forward target-policy validator isolation SHA:
  `e78127f44e41d51f12803726977d64c2eb643131`
- G62 hosted SST ratchet SHA:
  `a0347dbef8808288e45870116c75f953f77cfd7b`
- Evidence commit/final candidate: the commit containing this report

The primary worktree was not cleaned, reset, stashed, staged, or modified.
The dedicated worktree was created directly from the fixed predecessor.
The local G61 object and
`origin/topic/m61-upd9002-mov-imm-register` both resolve to the approved SHA.
No conflicting approved G61 SHA was found.

The documentation-only first commit introduced exactly one active canonical
M62 task, removed the five unexecuted prospective task files from active
discovery, and retained M64 as the next milestone. Git history preserves the
removed prospective documents. Milestone selftest reported 48 passing checks;
discovery and audit reported 75 tasks, 40 reports before this report, and 71
ROADMAP rows.

## Environment, corpus, contracts, and authority

| Component | Recorded value |
|---|---|
| Host | `Linux 6.18.33.2-microsoft-standard-WSL2 x86_64` |
| Distribution | Ubuntu 26.04 under WSL2 |
| Git | 2.53.0 |
| CMake | 4.2.3 |
| Ninja | 1.13.2 |
| GCC | 15.2.0 |
| Python | 3.14.4 |
| gzip command | 1.14 |
| zlib compile/runtime | 1.3.1 / 1.3.1 |

The verified corpus used by every required profile was:

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

The final worker SHA-256 is:

```text
2023cf3830bd9135c4ac50b424db7d3063a149a7c676c22e9e41f9ed44605a8a
```

The protected G60b ROM-authority manifest remains:

```text
f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac
```

That authority positively owns `0F28 = ROL4` and `0F2A = ROR4`. M62 does not
reopen 6C-6F, 66/67, FPO/FPU, BRKFEM/BRKEM, interrupt entry, IRET, or C6/C7.
The corrected G60c interpretation also remains protected: G60a retired
failures were 6E=0 and 6F=641; 417 and 224 are G43 signature subsets, not
per-opcode counts.

## Approved G61 reproduction

Before semantic editing, a fresh tests-enabled build at the exact G61 SHA ran
all profiles without skip:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,794 | 7,506 | 0 | 0 |
| architectural full | 1,562,502 | 1,438,594 | 1,384,630 | 53,964 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,438,594 | 1,282,192 | 156,402 | 0 | 0 |

The exact approved pass, failure, and signature identities all matched G61:

| Profile | Pass SHA-256 | Failure SHA-256 | Signature SHA-256 |
|---|---|---|---|
| architectural CI | `ba52df6696a0179cba856e314fb2be5b7768bc33137fee92a24ce37d51de15b1` | `de59a4d8a6a36da692ba4c09909083c5da3ab10947fed7a61248292906d7f075` | `b37009c10a41335e5b837159b36901d84a6f366613832e032568eb7498beb56c` |
| architectural full | `50120c210b49d53bb686301935115507f86a98862776c787365b936895b809b3` | `841bfe445df094d2052b2d33417c7428660f17f82eedb5d1cf2ef80cfd869a5d` | `ff0e1dd067cfc522ca01527bc1100638dd37ae7155b45687e851674dc8c8de0f` |
| fingerprint full | `6e27baa3836869781205d78c93f042772d760e7a822124baaa96b2f41f5d27ba` | `d25e2d791a027b474d71787c70dcfd3766f19b4b32a567fac33ae37039560f06` | `65346416297b405a6bd6b6821cd2e63edabdece4db8b18ba7d72e8a1c2d0fcd5` |

M58 through M61 validators, protected scoreboards and transitions, immutable
G43/M43 evidence, comparison contracts, policy, taxonomy, and registries also
matched. No new baseline was established.

## Target-policy transition

The G61 policy was:

```text
upd9002-g60b-eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93
```

The content-addressed G62 policy is:

```text
upd9002-g62-6961b0f295110d32d16799cb3799bedff7600b9b956bc4ad893eebc249140212
```

The policy artifact SHA-256 is:

```text
f145732f2774a53acde03e333156285b6ca806d3e5c73098e9b8ecfa37d5ea86
```

| Scope | Selected before/after | Applicable before | Applicable after | Newly applicable |
|---|---:|---:|---:|---:|
| CI | 180,000 / 180,000 | 165,300 | 165,800 | 500 |
| full | 1,562,502 / 1,562,502 | 1,438,594 | 1,443,594 | 5,000 |

| Scope | Selected SHA-256 | Applicable before SHA-256 | Applicable after SHA-256 |
|---|---|---|---|
| CI | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `5a00d3fa15d55b38630015e771163fe58aff544d5f13342beac01a8236a485a1` | `440a621dea647cf11a4e8b834fc139c2c95f6081f294d717263ba8f42eb2a750` |
| full | `0aa3db24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` | `a29c0a52d0818c7797515d5fbcc680b1fecdf5b10b896e5e02afff498cf99d65` | `4e8cf0af125f3d8404912311fc18fc3c75952c4c27215256ae7dd983d095cdff` |

The exact full `0F28` upstream selector remains
`1d01e7d8ec9cd05fa804acc5c9cb7e30cc451f8eea710847826b15b0622ef247`.
The newly applicable record-hash digest is
`7aa79eb2754eab51104c07689016e4782c97406afbe0618a33bf824d0b8ff83f`.
All 5,000 hashes transitioned together from
`known_target_gap/implementation_missing` to `applicable`; all executed and
passed. There was no outcome-derived split.

Taxonomy changed only by removal of those 5,000 implementation-missing
records:

| Gap kind | Before | After |
|---|---:|---:|
| `documented_silicon_absent` | 32,000 | 32,000 |
| `implementation_missing` | 41,908 | 36,908 |
| `target_support_unverified` | 0 | 0 |

There is no other classification, gap-kind, approved-divergence, or
hardware-pending change.

## Phase A — D4 AAM

| Form | Pre pass/fail | Post pass/fail | Pre-failure SHA-256 |
|---|---:|---:|---|
| D4 | 197 / 4,803 | 5,000 / 0 | `e0ffd2df098de38bc99cc0fc455b351a266baff2d74bddeb3e2f1fc0e857b731` |

The complete case table independently proves that the immediate is the radix,
not a byte to skip while dividing by a fixed 10. The implementation consumes
the radix, places quotient in AH and remainder in AL, and materializes SZP
from AL while clearing the observed CF/AF/OF state. Immediate zero terminates
normally with AH=`ff` and AL preserved, as required by every represented
target case; no Intel-tradition divide fault was introduced.

The audit explicitly contains radix 0, 1, 2, 9, 10, 11, 16, and 255 strata.
The implementation cause is `proven`. D5 remains exactly 5,000 pass / 0 fail.

## Phase B — 0F2A ROR4

| Form | Pre pass/fail | Post pass/fail | Pre-failure SHA-256 |
|---|---:|---:|---|
| 0F2A | 308 / 4,692 | 5,000 / 0 | `4bbe0bf9537bbae74bb0c7d9c2e94bfa82a6ac0f3283945e6841de36c48bf3a3` |

The prior handler merged the source low nibble into the old AL high nibble.
Complete register/memory evidence instead requires AL to receive the complete
original source byte while the destination receives source high nibble in its
low half and old AL low nibble in its high half. Destination is written before
AL so the register-alias case remains correct. This cause is `proven`.

Register, memory, ModR/M, segment, displacement, prefix, offset-wrap, and
physical-wrap partitions are all green. Phase B did not use unimplemented
ROL4 as an oracle and did not change 0F28 ownership.

## Phase C — 0F28 ROL4

Pre-transition 0F28 was exactly 5,000 selected,
`known_target_gap/implementation_missing`, and zero officially executed.
Post-transition it is 5,000 selected/applicable/executed/pass and zero fail.

Independent case evidence requires the destination to receive source low
nibble in its high half and old AL low nibble in its low half, while AL
receives old AL low nibble in its high half and source high nibble in its low
half. Register alias and memory boundary forms are covered. The implementation
cause and result contract are `proven`.

Only the exact G60b-authorized 0F28 selector was activated. No BRKFEM, BRKEM,
TEST1/CLR1/SET1/NOT1, mode transition, or other 0F family changed.

## Phase D — BCD and ASCII adjust

The independently reviewable DAA/DAS and AAA/AAS causes were committed
separately:

| Form | Pre pass/fail | Post pass/fail |
|---|---:|---:|
| 27 DAA | 4,966 / 34 | 5,000 / 0 |
| 2F DAS | 4,936 / 64 | 5,000 / 0 |
| 37 AAA | 4,876 / 124 | 5,000 / 0 |
| 3F AAS | 284 / 4,716 | 5,000 / 0 |

The exact pre-fix failure union `B` has:

```text
count: 4,938
SHA-256: 905811142e78d182e37c98fb7be16b392b1d847bf5dd8692a3bdf2b38953ad36
```

DAA and DAS now use the independently derived low/high adjustment decisions
from original AL and original AF/CF and materialize result SZP, AF, CF, and
arithmetic OF. AAA/AAS adjust AL and AH as separate bytes, always mask final
AL to its low nibble, and derive SZP from the pre-mask adjusted byte with the
observed AF/CF/OF rules. The causes are `proven`; no D4 or D5 behavior was
changed by analogy.

## Phase E — SHL/SAL/SHR/SAR

The exact pre-fix shift failure union has:

```text
count: 19,139
SHA-256: 85c431ba4d46a285aa6c352192ba1b583ac3aadd739b6154e79c8d96f0b06bce
```

| Form | Pre pass/fail | Post pass/fail |
|---|---:|---:|
| C0.4 | 1,707 / 793 | 2,500 / 0 |
| C0.5 | 1,698 / 802 | 2,500 / 0 |
| C0.6 | 1,701 / 799 | 2,500 / 0 |
| C0.7 | 1,810 / 690 | 2,500 / 0 |
| C1.4 | 698 / 1,802 | 2,500 / 0 |
| C1.5 | 819 / 1,681 | 2,500 / 0 |
| C1.6 | 753 / 1,747 | 2,500 / 0 |
| C1.7 | 845 / 1,655 | 2,500 / 0 |
| D2.4 | 1,769 / 731 | 2,500 / 0 |
| D2.5 | 1,802 / 698 | 2,500 / 0 |
| D2.6 | 1,803 / 697 | 2,500 / 0 |
| D2.7 | 1,895 / 605 | 2,500 / 0 |
| D3.4 | 864 / 1,636 | 2,500 / 0 |
| D3.5 | 908 / 1,592 | 2,500 / 0 |
| D3.6 | 808 / 1,692 | 2,500 / 0 |
| D3.7 | 981 / 1,519 | 2,500 / 0 |

The audit stratifies immediate and CL sources, 8/16-bit width, register and
memory forms, subforms `/4-/7`, count zero, count one, width boundaries, 31,
32, 33, and 255, destination state, FLAGS, termination, RAM, prefixes,
displacement, and both wrap classes.

The complete population rejects one unconditional `count & 0x1f` rule.
Only the authorized shift subforms use the evidence-derived raw count:
count zero preserves destination and FLAGS; nonzero destination and carry
rules are width/subform-specific; at/beyond-width results saturate according
to SHL/SHR/SAR ownership; `/6` is the evidence-proven SHL form. Result SZP,
CF, OF, and cleared AF are materialized exactly. The implementation cause is
`proven`.

All 40,000 protected rotate `/0-/3` hashes remain architectural pass with no
changed signature. Architectural and all-16-bit fingerprint domains remain
separate; the diagnostic fingerprint differences in unowned rotate/shift
families were not promoted into this architectural milestone.

## Phase checkpoints

The final worker validates all earlier phases through:

```text
tests/ssts/evidence/g62/phases/phase_aam.json
tests/ssts/evidence/g62/phases/phase_ror4.json
tests/ssts/evidence/g62/phases/phase_rol4_activation.json
tests/ssts/evidence/g62/phases/phase_bcd_adjust.json
tests/ssts/evidence/g62/phases/phase_shifts.json
```

Each checkpoint records all semantic commits for the phase, the final worker,
owned hash sets, exact pre/post results, protected forms, and focused-test
status. The BCD checkpoint records both independent semantic commits. No later
phase changed an earlier phase result.

## Final arithmetic and ratchet

The existing-applicable newly passing sets are:

| Set | Count | SHA-256 |
|---|---:|---|
| AAM `A` | 4,803 | `e0ffd2df098de38bc99cc0fc455b351a266baff2d74bddeb3e2f1fc0e857b731` |
| ROR4 `R` | 4,692 | `4bbe0bf9537bbae74bb0c7d9c2e94bfa82a6ac0f3283945e6841de36c48bf3a3` |
| adjust `B` | 4,938 | `905811142e78d182e37c98fb7be16b392b1d847bf5dd8692a3bdf2b38953ad36` |
| shifts `S` | 19,139 | `85c431ba4d46a285aa6c352192ba1b583ac3aadd739b6154e79c8d96f0b06bce` |
| exact union | 33,572 | `40f610caf661aa5884296e5f732637cd4ec48a7cb40ac7544264f1b4b9bc176a` |

The exact full newly applicable set `L` has 5,000 hashes and digest
`7aa79eb2754eab51104c07689016e4782c97406afbe0618a33bf824d0b8ff83f`.
It is not counted as semantic failure reduction.

The required formula is reached exactly:

```text
candidate failures = 25,330 - |B|
                   = 25,330 - 4,938
                   = 20,392

candidate pass = 1,384,630 + 33,572 + 5,000
               = 1,423,202
```

There are zero newly failing hashes and zero changed surviving failure
signatures. All changed architectural hashes are in the four governed
pre-fix failure sets; every newly applicable hash is in the exact 0F28 set.
Dataset, contracts, selected sets, and all unrelated classification ownership
are unchanged.

## Final profiles

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash | Elapsed |
|---|---:|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,800 | 163,567 | 2,233 | 0 | 0 | 156.23 s |
| architectural full | 1,562,502 | 1,443,594 | 1,423,202 | 20,392 | 0 | 0 | 336.69 s |
| fingerprint full | 1,562,502 | 1,443,594 | 1,343,614 | 99,980 | 0 | 0 | 349.47 s |

| Profile | Pass SHA-256 | Failure SHA-256 | Signature SHA-256 |
|---|---|---|---|
| architectural CI | `66563665485c202e553abc71c13ea3a5e4dfa9f8b459d72883e550f3f6495bf1` | `dc7d17b30274043062a80a0017ba0d2da35538bbd47b0ade5f56a563c5a93290` | `8ce62f40ff7a41ec7a409b459ae0088877b4061d84da85c7e7ad36ea0ba7178e` |
| architectural full | `a8631f202dc49944836f299f49268ece64c9b55d5bdbea1203df7dbf8e40139a` | `1a313e7389b3a0788200d0929d698f463e8221d91858b408155b00b7e05e1cf5` | `2286b2ea79eeb76b203d17b230a6e31bc9d050f5db617f1630c10f46706fe6c5` |
| fingerprint full | `bccc05024127640f2a59f51a46fb7f73fdd21a918f705c77ef7140d52373ad02` | `96afc7aae305d50839612b834adc493c796f86cda8cb94036c26b818a2bdcd78` | `63afa59d41501b473c32a212b5becd3ed967b642aa13f400830648049ca0a99a` |

Termination totals are unchanged: full profiles contain 1,430,898 normal and
12,696 type0 executions. Timeout and crash are zero in every profile.

## Protected behavior

| Form/family | Final architectural result |
|---|---:|
| 9C PUSHF | 4,999 / 1 |
| 9D POPF | 5,000 / 0 |
| 9E SAHF | 5,000 / 0 |
| 9F LAHF | 5,000 / 0 |
| CC INT3 | 5,000 / 0 |
| CD INT | 5,000 / 0 |
| CE INTO | 5,000 / 0 |
| CF IRET | 5,000 / 0 |
| C6 MOV | 5,000 / 0 |
| C7 MOV | 5,000 / 0 |
| 62 BOUND | 3,756 / 1,244 |
| D5 AAD | 5,000 / 0 |
| C0/C1/D2/D3 rotate `/0-/3` | 40,000 / 0 |

The G60d empty residual-frame set, M60a 214-case divide dependency set, and
G61 2,208-case C6/C7 improvement remain protected by their exact approved
digests. FF `/7`, F7 `/2`, DIV/IDIV, BOUND range logic, event entry, IRET,
C6/C7, 6C-6F, 66/67/FPU, BRKFEM/BRKEM, D5, fixtures, and comparison contracts
are untouched.

## Semantic and changed-file audit

Relative to G61, the final evaluated SHA changes only:

- prospective milestone consolidation documents;
- M62 audit, schema, focused-test, and build integration;
- `cpu/upd9002/upd9002_dispatch.c`;
- the exact G60c prospective erratum validator needed by task consolidation.

Later commits isolate historical validators from exact M62-governed graph,
support-map, provenance, and harness changes and update the permanent bug-fix
ledger. They do not alter the worker or any governing SST input.

Every changed production line is classified as one of:

```text
D4 AAM semantics
0F2A ROR4 semantics
0F28 ROL4 dispatch or semantics
BCD/ASCII adjust semantics
shift count semantics
shift destination semantics
shift FLAGS semantics
```

No production line belongs to another family. The semantic review commands
are:

```text
git diff --stat \
  829f314bb0d363ec5b6e9aa738e948b1a3adb365...\
2cdaed95072d74bbf7187ae854fb31d3886c995d
git diff --name-status \
  829f314bb0d363ec5b6e9aa738e948b1a3adb365...\
2cdaed95072d74bbf7187ae854fb31d3886c995d
git diff \
  829f314bb0d363ec5b6e9aa738e948b1a3adb365...\
2cdaed95072d74bbf7187ae854fb31d3886c995d \
  -- cpu/upd9002/
```

## Evidence, transitions, and ranking

- Evidence manifest:
  `tests/ssts/evidence/g62/manifest.json`
- Evidence-manifest file SHA-256:
  `b15fef00aa66a342734781d5b6e1b9c183de2fbda8fb8bf5ccf0ee2f5753d847`
- Result manifest:
  `tests/ssts/evidence/g62_result_manifest.json`
- Artifact-tree SHA-256:
  `bd6dda7941f7cb1efd31deb639e884099612ee27ff6987e7789614b3b581e44f`
- Architectural CI transition:
  `tests/ssts/transitions/g62_architectural_ci_from_g61.json`
- Architectural CI transition SHA-256:
  `2d6191363da948ff20d887cf000e018d303dc6098a7384f284404335c043b36b`
- Architectural full transition:
  `tests/ssts/transitions/g62_architectural_full_from_g61.json`
- Architectural full transition SHA-256:
  `4cfa8949985eeadb02efd013216c78eda4b0222fcd4b89450f2396d04f8d4db3`
- Complete ranking:
  `tests/ssts/rankings/g62_architectural_full.json`
- Ranking SHA-256:
  `d90c58348085a971806ac89ee5fe38f6ae43014cc1f7a88012cd9a914f7a966a`
- Human top 30:
  `tests/ssts/rankings/g62_architectural_full.md`

The top remaining architectural failures are FF.7=5,000, F7.7=3,723,
F6.7=3,716, F6.6=2,561, F7.6=2,486, BOUND=1,244, and F7.2=1,113.
The complete ranking reconciles exactly to 20,392 and contains explicit green
rows for every M62 form, including selected/applicable/executed/pass 0F28.
Omission from the human top list is never interpreted as pass.

The complete evidence family was generated twice in the same environment
from the same raw profiles, phase audits, policy input, evaluated SHA, and
worker. Both output trees were byte-identical. JSON is canonical,
deterministically ordered, and timestamp-free; gzip uses the repository's
deterministic writer in the pinned gzip/zlib environment.

## Validation and execution discipline

The non-external native suite passed 61/61 tests in 241.35 seconds. This
includes focused M62 tests, all positive and fail-closed M62 selftests, M58
through M61 protection, ROM authority, target-policy, historical erratum,
frame, IRET, C6/C7, encoding, state, and repository checks.

The historical M48/M50/M61 validators initially reported M62-authorized
surrounding changes rather than an owned regression. The validator-only
commit separates their ownership: immutable historical artifacts and
M48/M50-owned identifiers remain exact, while the five M62 graph/support/
provenance/harness replacements are accepted only as complete exact sets.
The seven formerly conflated checks then passed 7/7, followed by the complete
61/61 suite.

The first hosted run,
[build 30193902710](https://github.com/nakatamaho/vaeg/actions/runs/30193902710),
exposed the same boundary issue in the M60c and M60d static validators only
after the generated G62 target-policy artifact became tracked. Both validators
still treated the whole target-policy directory as an old-gate-owned path.
The forward-policy validator commit narrows their historical ownership to the
exact protected G60b policy artifacts and delegates the exact G62 policy and
support-map transition to the M62 validator. Their ordinary pre-M62
fail-closed behavior is unchanged. The two previously failing tests then
passed 2/2 locally; their selftests and both M62 tests passed 4/4. All other
tests in the failed GCC job had passed. The failed run was canceled once the
common cause was identified rather than consuming the remaining CI matrix.

The second hosted run,
[build 30194283261](https://github.com/nakatamaho/vaeg/actions/runs/30194283261),
confirmed that GCC, Clang, AddressSanitizer, macOS, repository invariants,
and the standalone Z80 suite were green. Its uPD9002 SST job executed the
correct final M62 worker but still invoked the historical M61 CI wrapper.
That wrapper reproduced the old G60b-policy CI profile and then rejected the
valid M62 result because it compared it with committed G61 evidence. The run
was canceled after this exact CI-wiring cause was established.

The G62 hosted SST ratchet commit adds a milestone-specific wrapper and wires
the external CTest gate to it. The wrapper rederives the content-addressed G62
target policy, executes architectural CI with that policy, regenerates the
candidate scoreboard, and compares its stable identity with committed G62
evidence. The exact external test passed locally:

```text
vaeg_upd9002_ssts_ci_external:
  passed in 572.99 seconds
  selected: 180,000
  applicable/executed: 165,800
  pass: 163,567
  fail: 2,233
  timeout/crash: 0/0
```

Its checkout-identity negative test rejected a mismatched configured SHA
before profile execution. This validator-only correction does not change the
evaluated worker, corpus, contracts, target policy, selected/applicable sets,
or any generated profile result.

Candidate raw profiles were preserved at:

```text
/home/maho/vaeg/build/m62-results/final-2cdaed9/
```

They were reused only for deterministic evidence serialization because the
evaluated SHA, worker digest, dataset, contracts, final target policy, and
selected/applicable identities were unchanged. No expensive profile was rerun
for report or artifact placement.

Principal commands, all with exit status zero:

```text
cmake -S . -B build/linux-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVA_BUILD_TESTS=ON
cmake --build build/linux-debug -j2
ctest --test-dir build/linux-debug --output-on-failure -LE external

python3 tools/qa/upd9002_m62_bundle.py selftest
python3 tools/qa/upd9002_m62_bundle.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root <verified-corpus> \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map <generated-g62-support-map> \
  --worker build/linux-debug/sdl2/vaeg \
  --profile ci --flags-comparison defined \
  --output <architectural-ci-raw> \
  --failure-directory <architectural-ci-failures>

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root <verified-corpus> \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map <generated-g62-support-map> \
  --worker build/linux-debug/sdl2/vaeg \
  --profile full --flags-comparison defined \
  --output <architectural-full-raw> \
  --failure-directory <architectural-full-failures>

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root <verified-corpus> \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map <generated-g62-support-map> \
  --worker build/linux-debug/sdl2/vaeg \
  --profile full --flags-comparison all16 \
  --output <fingerprint-full-raw> \
  --failure-directory <fingerprint-full-failures>

python3 tools/qa/upd9002_m62_bundle.py generate \
  --root . --dataset-root <verified-corpus> \
  --audit-root <final-phase-audits> \
  --worker build/linux-debug/sdl2/vaeg \
  --evaluated-sha 2cdaed95072d74bbf7187ae854fb31d3886c995d \
  --phase-commits <phase-commit-manifest> \
  --architectural-ci-raw <architectural-ci-raw> \
  --architectural-full-raw <architectural-full-raw> \
  --fingerprint-full-raw <fingerprint-full-raw> \
  --output-root <generated-output> --regenerate-twice

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/qa/milestone_ids.py --selftest --audit --discover
git diff --check
```

The final hosted CI is launched only after the G62-specific ratchet correction
and the evidence-only follow-up containing this report are pushed. Its
GitHub-assigned URL and successful conclusion are supplied in the maintainer
handoff rather than self-referenced inside this report. Hosted CI is not used
as an iterative debugger.

## Known limitations

- This is SST-observed real-mode behavior, not complete uPD9002 silicon
  validation.
- Fingerprint-only failures outside the architectural contracts remain
  diagnostic and were not used to weaken or broaden the milestone.
- `0F20`, `0F22`, FF `/7`, F7 `/2`, DIV/IDIV, and BOUND range residuals
  remain for separately governed work.
- M64 has not been started, and the fresh ranking does not authorize it.
- The hosted CI URL is external post-commit evidence and is reported in the
  final handoff.

## Human review

```text
git fetch origin
git switch topic/m62-upd9002-semantics-bundle
git status --short
git log --oneline --decorate \
  829f314bb0d363ec5b6e9aa738e948b1a3adb365..HEAD
git diff --check
git diff --stat \
  829f314bb0d363ec5b6e9aa738e948b1a3adb365...\
2cdaed95072d74bbf7187ae854fb31d3886c995d
git diff \
  829f314bb0d363ec5b6e9aa738e948b1a3adb365...\
2cdaed95072d74bbf7187ae854fb31d3886c995d \
  -- cpu/upd9002/
sha256sum \
  tests/ssts/evidence/g62/manifest.json \
  tests/ssts/target_policy/g62.json \
  tests/ssts/transitions/g62_architectural_ci_from_g61.json \
  tests/ssts/transitions/g62_architectural_full_from_g61.json \
  tests/ssts/rankings/g62_architectural_full.json
python3 tools/qa/upd9002_m62_bundle.py selftest
python3 tools/qa/upd9002_m62_bundle.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
cmake -S . -B build/review -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVA_BUILD_TESTS=ON
cmake --build build/review -j2
ctest --test-dir build/review --output-on-failure -LE external
git rev-parse HEAD
git rev-parse '@{u}'
git ls-remote origin refs/heads/topic/m62-upd9002-semantics-bundle
```
