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
# M61 uPD9002 C6/C7 register-form MOV-immediate semantics

M61 corrects the executed SST-observed register-destination behavior of
`C6 /0 MOV r/m8, imm8` and `C7 /0 MOV r/m16, imm16`. The implementation used
the ModR/M extension field at bits 5:3 as the destination register. The
executed population proves that the destination is selected by r/m bits 2:0.
The correction changes only those two register-form selectors.

M61 is complete and pushed. G61 is an unapproved candidate pending human
review. M62a and later milestones have not been started.

A commit cannot contain its own SHA. The evidence commit and final candidate
are therefore the commit containing this report; the exact SHA is supplied in
the maintainer handoff and is independently available from
`origin/topic/m61-upd9002-mov-imm-register`.

## Identity and preparation

- Approved predecessor gate: `G60e`
- Exact approved G60e SHA and M61 base:
  `a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b`
- G60e semantic implementation/evaluated SHA:
  `7f815acb26f1be546bbcfd5de12972235dfd175c`
- Approved G60e CI:
  [build 30184747721](https://github.com/nakatamaho/vaeg/actions/runs/30184747721)
- Initial primary-worktree branch: `main`
- Initial primary-worktree SHA:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty with unrelated maintainer work
- Dedicated worktree: `/home/maho/vaeg/build/m61-worktree`
- Dedicated worktree starting SHA:
  `a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b`
- Exact evaluated-SHA worktree:
  `/home/maho/vaeg/build/m61-evaluated`
- M61 branch: `topic/m61-upd9002-mov-imm-register`
- Audit implementation SHA:
  `d16dcb7556b866a94407599ed2a5f0677dd195c6`
- Semantic implementation and `evaluated_sha`:
  `90fa7dec5d46708a807851f61ae0792ee39e9b8f`
- Permanent bug-fix ledger commit:
  `51f3faff81b469e763e1a815218c9883a6eb3249`
- Evidence commit/final candidate: the commit containing this report

The primary worktree was not cleaned, reset, stashed, staged, or modified.
The dedicated worktree was created directly from the fixed predecessor. The
maintainer authorization, local object, authoritative G60e report, and
`origin/topic/m60e-upd9002-iret` all resolve to the approved SHA. No
conflicting approved G60e SHA was found.

The preparation and discovery commands exited zero:

```text
git status --short
git branch --show-current
git rev-parse HEAD
git rev-parse a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b
git show --stat --oneline a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b
git rev-parse origin/topic/m60e-upd9002-iret
python3 tools/qa/milestone_ids.py --selftest --audit --discover
```

Milestone discovery reported 48 passing selftests, 79 tasks, 39 reports, and
75 ROADMAP rows. ROADMAP, discovery, and the canonical task agree on the M61
identifier, branch, G60e prerequisite, report path, and C6/C7-only scope.

## Environment, corpus, contracts, and target policy

| Component | Recorded value |
|---|---|
| Host | `Linux-6.18.33.2-microsoft-standard-WSL2-x86_64-with-glibc2.43` |
| Distribution | Ubuntu 26.04 under WSL2 |
| Git | 2.53.0 |
| CMake | 4.2.3 |
| Ninja | 1.13.2 |
| GCC | 15.2.0 |
| Python | 3.14.4 |
| gzip command | 1.14 |
| Python gzip module | `/usr/lib/python3.14/gzip.py` |
| zlib compile/runtime | 1.3.1 / 1.3.1 |

The verified corpus was available for every required execution:

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

The target policy remains exactly:

```text
upd9002-g60b-eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93
```

| Scope | Selected SHA-256 | Applicable SHA-256 |
|---|---|---|
| CI | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `5a00d3fa15d55b38630015e771163fe58aff544d5f13342beac01a8236a485a1` |
| full | `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` | `a29c0a52d0818c7797515d5fbcc680b1fecdf5b10b896e5e02afff498cf99d65` |

Dataset, comparison contracts, target policy, selected/applicable sets,
top-level classifications, gap taxonomy, registries, and fixtures did not
change.

## Approved G60e reproduction

Before production editing, a fresh tests-enabled build at the exact approved
G60e SHA ran all three profiles without skip:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,561 | 7,739 | 0 | 0 |
| architectural full | 1,562,502 | 1,438,594 | 1,382,422 | 56,172 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,438,594 | 1,279,984 | 158,610 | 0 | 0 |

Counts were not identity substitutes:

| Profile | Pass SHA-256 | Failure SHA-256 | Signature SHA-256 |
|---|---|---|---|
| architectural CI | `dc6bdee9f856ca6102748ca442ac579adf8a7f05e01e02564766148a35825cdc` | `2ae38099d67c240ff5bf48a1c7643d1b6d6480e4e27d1b8967d87508d751ebd6` | `af14392ce957dfeaf770da595551fef8767bc7412eec06c10badbe9d7c8930b4` |
| architectural full | `11958b52c4fa71e1ac38c22d7e305562ab00f408c453fa423955bcc3eb6882c4` | `2c2bae091f33ebcd334767d9a9597eab5707d45a4d66b5433b8b37b10ce367f7` | `4fc2d3603ec05633f4a4b63f574d92bb5b26140519f03e5e50c848d5066dd84b` |
| fingerprint full | `17a6bc59e91efc7439621037842072c3ae0d0bf2f600307ae3ef407e1dafc542` | `795fdeb7c0469783f4863aeebf45c730118c7cccfede5b7804d5a55f7e1ae2cb` | `0c184c75164afe40cb5afddaa0aab635c24b131cf8925f5df9163c89d6e3d377` |

M58 ratchet, M59 evidence, M60a artifacts, M60b authority/policy, M60c
erratum/authority, M60d frame evidence, and M60e IRET verification passed.
Protected G43/M43 and G58-G60e artifacts remained byte-identical.

## Pre-edit C6/C7 audit

Both opcode populations are top-level `applicable`; every selected record was
applicable and executed:

| Form | Selected/executed | Pass | Fail | Failure SHA-256 |
|---|---:|---:|---:|---|
| C6 | 5,000 | 3,912 | 1,088 | `2def4cc309f2a11b5950d4708ae1093e661e0d57e636c7f6600262d7efe8abe3` |
| C7 | 5,000 | 3,880 | 1,120 | `640e24a7c324690e73c72db449f3d6a750dca66b690fd35f021317c82816394a` |

These exact G60e values match M59. Termination remains normal throughout.
Every failure is a register-form register mismatch. No C6/C7 failure is a
FLAGS, IP, termination, RAM, effective-address, segment, displacement, or
wrapping mismatch.

The complete deterministic side-by-side case tables are:

| Artifact | Rows | SHA-256 |
|---|---:|---|
| `tests/ssts/evidence/g61/c6_cases.json.gz` | 5,000 | `b8483b7eab519f0e8af1017e88cac7a87c342cbe50dd38265a15b9a1365e8dab` |
| `tests/ssts/evidence/g61/c7_cases.json.gz` | 5,000 | `e813297404df2721eaa93b6a184fab8ac880be7ef30ac50090f6a85d2fc553ac` |

Every row records structure and ModR/M fields, full bytes and prefixes,
initial/expected/actual IP and destination, immediate, width, memory mapping
where applicable, FLAGS, termination, represented RAM, unrelated registers,
mismatches, structural partitions, conclusion, and evidence notes.

## Structural and destination partition

| Form | Register forms | Memory forms | Pre-fix register pass | Pre-fix register fail |
|---|---:|---:|---:|---:|
| C6 | 1,249 | 3,751 | 161 | 1,088 |
| C7 | 1,274 | 3,726 | 154 | 1,120 |

All 7,477 memory-form records passed before and after M61. They cover the
corpus-represented ModR/M modes, displacement widths, segments and overrides,
16-bit effective-offset wraps, 20-bit physical wraps, C6 byte stores, and C7
little-endian word stores.

The exact machine audit corrects one assumption in the prospective task. The
161 C6 and 154 C7 pre-fix register-form passes are not passes because the
initial destination equals the immediate. They are exactly the cases where
ModR/M fields 5:3 and 2:0 select the same register, so the buggy selector
accidentally targets the correct register. The audit found seven C6
destination-value coincidences and no C7 coincidences; all seven C6 cases
still failed because the wrong register was modified. Exact evidence therefore
takes precedence over the cross-check wording.

All eight C6 byte destinations (`AL`, `CL`, `DL`, `BL`, `AH`, `CH`, `DH`,
`BH`) and all eight C7 word destinations (`AX`, `CX`, `DX`, `BX`, `SP`, `BP`,
`SI`, `DI`) are represented. The partition is structural and independent of
outcome.

## Proven root cause and correction

The direct implementation cause is **proven**. Both register-form paths
already dispatch and consume the correct immediate width, but selected the
destination using ModR/M bits 5:3:

```text
C6: REG8_B53(op)
C7: REG16_B53(op)
```

For the complete executed population, r/m bits 2:0 own the destination. The
minimal correction is:

```text
C6: REG8_B20(op)
C7: REG16_B20(op)
```

The only production file changed is `cpu/upd9002/i286c_mn.c`, with those two
selector substitutions. C6 writes exactly one selected byte register and
preserves its paired byte. C7 writes the full little-endian immediate to the
selected 16-bit register. Immediate fetch, IP advancement, FLAGS, termination,
memory forms, effective-address helpers, decoder tables, and every unrelated
instruction remain unchanged.

The focused native test covers all eight byte and word register codes,
`0x00`/`0xff`, `0x0000`/`0xffff`, distinct immediate bytes, equal and unequal
initial values, paired-byte and unrelated-register preservation, FLAGS, IP,
termination, and representative ordinary/displaced/segment-overridden/wrapped
memory cases. Explicit values are used as the oracle.

## Candidate results and ratchet

The evaluated worker is bound to:

```text
evaluated_sha:
90fa7dec5d46708a807851f61ae0792ee39e9b8f

worker SHA-256:
2dc2452f84d566acabdfb9c93bbf7999e954c8c1d5985bc06443190d24e959ff
```

All candidate profiles used that exact worker:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,794 | 7,506 | 0 | 0 |
| architectural full | 1,562,502 | 1,438,594 | 1,384,630 | 53,964 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,438,594 | 1,282,192 | 156,402 | 0 | 0 |

| Profile | Pass SHA-256 | Failure SHA-256 | Signature SHA-256 |
|---|---|---|---|
| architectural CI | `ba52df6696a0179cba856e314fb2be5b7768bc33137fee92a24ce37d51de15b1` | `de59a4d8a6a36da692ba4c09909083c5da3ab10947fed7a61248292906d7f075` | `b37009c10a41335e5b837159b36901d84a6f366613832e032568eb7498beb56c` |
| architectural full | `50120c210b49d53bb686301935115507f86a98862776c787365b936895b809b3` | `841bfe445df094d2052b2d33417c7428660f17f82eedb5d1cf2ef80cfd869a5d` | `ff0e1dd067cfc522ca01527bc1100638dd37ae7155b45687e851674dc8c8de0f` |
| fingerprint full | `6e27baa3836869781205d78c93f042772d760e7a822124baaa96b2f41f5d27ba` | `d25e2d791a027b474d71787c70dcfd3766f19b4b32a567fac33ae37039560f06` | `65346416297b405a6bd6b6821cd2e63edabdece4db8b18ba7d72e8a1c2d0fcd5` |

The evidence-derived reference result of 53,964 architectural full failures
was reached exactly:

```text
56,172 - 1,088 - 1,120 = 53,964
```

| Form | Before | After |
|---|---|---|
| C6 | 3,912 pass / 1,088 fail | 5,000 pass / 0 fail |
| C7 | 3,880 pass / 1,120 fail | 5,000 pass / 0 fail |

The full newly-passing set contains exactly 2,208 hashes and equals the union
of the G60e C6 and C7 failure sets:

```text
0d7b5a60ebef2f1364d791e5e905555b7b3d1853957ba83780d39bb624eb8903
```

The CI newly-passing count is 233. The newly-failing set is empty in both
scopes; its canonical empty-set digest is:

```text
4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945
```

There are zero changed surviving failure signatures, no per-form pass
decrease, no timeout, no crash, and no classification, taxonomy, registry,
policy, selected-set, applicable-set, dataset, or comparison-contract change.
Existing accidental register-form passes remained passes and were not counted
as newly passing.

## Memory and prior-gate protection

| Form | Memory count | Structural/result SHA-256 | Result |
|---|---:|---|---|
| C6 | 3,751 | `6e61ad67127baaee4101330c5c4c4a7a604739fc22ba0325fdea116c6b61f084` | all pass before and after |
| C7 | 3,726 | `10fc0b646c593cf7172bd4e16f6c241c21ff50fbdaa9c094e56d08dd952e9515` | all pass before and after |

All memory-form expected/actual RAM, FLAGS, IP, termination, and unrelated
register fields remain equal. The production semantic diff does not touch the
memory branches or effective-address helpers. F7 `/2`, B0-BF, and the decoder
tables have no source diff and no candidate result change.

Protected results remain:

| Form | Candidate result |
|---|---|
| 9C PUSHF | 4,999 pass / 1 fail |
| 9D POPF | 5,000 pass / 0 fail |
| 9E SAHF | 5,000 pass / 0 fail |
| 9F LAHF | 5,000 pass / 0 fail |
| CC INT3 | 5,000 pass / 0 fail |
| CD INT | 5,000 pass / 0 fail |
| CE INTO | 5,000 pass / 0 fail |
| CF IRET | 5,000 pass / 0 fail |
| 62 BOUND | 3,756 pass / 1,244 fail |

The G60d residual-frame set remains empty with digest
`4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`.
The exact 214-hash M60a divide-exception dependency set remains green with
digest
`5cda7079da30b8266de2df3b55b90b3e5ee12a20429e670d1f128b924903c719`.

## Fresh target-correct failure ranking

The complete 333-row ranking is
`tests/ssts/rankings/g61_architectural_full.json`; its SHA-256 is
`d5348cc89793286c48a784af5e2b86bfb4710c3eedf4967f6f85347f51105376`.
The human-readable top 30 is
`tests/ssts/rankings/g61_architectural_full.md`; its SHA-256 is
`126f095aaa3e4dc1bf9a4ff772b10046119d6461e5e4d7f46356ed1ee6138cb2`.
The complete rows reconcile exactly to 53,964 failures. Family aggregation is
obtained by summing the complete rows by primary opcode; no missing row is
interpreted as passing.

The largest remaining forms are:

| Rank | Form | Fail | Cumulative share |
|---:|---|---:|---:|
| 1 | FF.7 | 5,000 | 9.27% |
| 2 | D4 | 4,803 | 18.17% |
| 3 | 3F | 4,716 | 26.90% |
| 4 | 0F2A | 4,692 | 35.60% |
| 5 | F7.7 | 3,723 | 42.50% |
| 6 | F6.7 | 3,716 | 49.38% |
| 7 | F6.6 | 2,561 | 54.13% |
| 8 | F7.6 | 2,486 | 58.74% |
| 9 | C1.4 | 1,802 | 62.08% |
| 10 | C1.6 | 1,747 | 65.31% |

The family-level aggregation reconciles to the same total:

| Family | Selected | Pass | Fail |
|---|---:|---:|---:|
| F7 | 40,000 | 32,678 | 7,322 |
| C1 | 20,000 | 13,115 | 6,885 |
| D3 | 20,000 | 13,561 | 6,439 |
| F6 | 40,000 | 33,723 | 6,277 |
| FF | 40,000 | 34,855 | 5,145 |
| 0F | 57,000 | 51,913 | 5,087 |
| D4 | 5,000 | 197 | 4,803 |
| 3F | 5,000 | 284 | 4,716 |
| C0 | 20,000 | 16,916 | 3,084 |
| D2 | 20,000 | 17,269 | 2,731 |
| 62 | 5,000 | 3,756 | 1,244 |
| all smaller families | 1,166,594 | 1,166,363 | 231 |
| **total** | **1,438,594 applicable** | **1,384,630** | **53,964** |

The machine artifact explicitly records `C6: 5,000/0` and
`C7: 5,000/0`; their green status is not inferred from absence in the top
list. The ranking is prospective evidence only and does not authorize M62a.

## Evidence, transitions, and reproducibility

The evidence pack is `tests/ssts/evidence/g61/`. Its manifest digest is:

```text
2b786278ada65613eb35dd165ae51870518f582d374fdd02f670541537fba502
```

The artifact-tree digest is:

```text
a58818b3e62fe3f3fcd7f1dd684a9fd8e9034ec3f8786dd711698db48a2af6b7
```

| Transition | SHA-256 |
|---|---|
| `tests/ssts/transitions/g61_architectural_ci_from_g60e.json` | `2bef2f438937bc18fa1742fe52dd1d410f7ec6ce44a5d280c3753bd968b7233f` |
| `tests/ssts/transitions/g61_architectural_full_from_g60e.json` | `9f6d1e163c9453a5f674ea568b8e689b9bd93513767f836d7c2cec4835a49181` |

The deterministic generator ran twice in the recorded Python/gzip/zlib
environment and compared every relative path and byte. Both complete 44-file
trees were byte-identical. Canonical JSON uses sorted keys and terminal LF;
canonical gzip uses a fixed timestamp and empty original filename. The bounded
reproducibility claim is byte identity in the recorded environment. A
universal cross-zlib byte-identity claim is not made.

Raw completed profiles and sidecars were preserved outside Git at:

```text
/home/maho/vaeg/build/m61-results/final-90fa7de/
```

They were reused only by the evidence serializer, with the exact evaluated
SHA, worker digest, dataset, contracts, policy, and selected/applicable
identities bound and unchanged. No expensive profile was rerun for report or
artifact placement.

## Validation

The exact evaluated commit was configured and built from a fresh tests-enabled
worktree. The following required executions completed without skip:

```text
cmake -S . -B build/linux-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVA_BUILD_TESTS=ON
cmake --build build/linux-debug -j2
ctest --test-dir build/linux-debug --output-on-failure \
  -E 'upd9002-ssts-m61-hosted-ci'
python3 tools/qa/upd9002_m61_mov_imm.py selftest
python3 tools/qa/upd9002_m61_mov_imm.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/qa/validate_docs.py
git diff --check
```

The non-external native suite passed 58/58 tests in 198.70 seconds. This
includes the focused M61 tests, positive and fail-closed M61 selftests,
M58-M60e validators, protected-evidence checks, and repository checks. The
M61 validator rejects predecessor, identity, population, partition, register
mapping, paired-byte, FLAGS/IP/termination, memory, F7/B0-BF, protected
artifact, ratchet, serialization, compression, ranking, and evidence-only
commit violations specified by the task.

Candidate profile commands used the exact evaluated worker and completed with
exit code zero. Architectural CI completed in under one minute;
architectural full and fingerprint full each completed in approximately seven
minutes. No required profile was skipped.

Hosted CI is intentionally run only after the local validation and final
evidence-only commit are fixed. Its exact successful URL is supplied in the
maintainer handoff; the report-containing commit does not rewrite itself after
that run solely to embed its own CI result.

## Semantic-diff and changed-file audit

Relative to G60e, the evaluated commit contains nine files: audit/validator
tooling and schema, focused native-test support and CLI integration, CMake/CI
integration, one historical-verifier isolation update, and the two-line
semantic correction in `cpu/upd9002/i286c_mn.c`.

The production diff is exactly:

```diff
- GET_PCBYTE(*(REG8_B53(op)))
+ GET_PCBYTE(*(REG8_B20(op)))

- GET_PCWORD(*(REG16_B53(op)))
+ GET_PCWORD(*(REG16_B20(op)))
```

The first line is classified as C6 byte-register destination selection; the
second is C7 word-register destination selection. No other production
semantic line changed. `git diff` confirms no changes to C6/C7 memory
behavior, effective-address helpers, F7 `/2`, FLAGS, interrupt/IRET, BOUND,
DIV/IDIV, fixtures, contracts, policy, classification, or protected evidence.

The final evidence commit contains only generated G61 artifacts and this
report. The bug-fix ledger is isolated in its own documentation commit.

## Known limitations and review

- M61 establishes behavior against the complete executed SST population; it
  is not complete uPD9002 silicon validation.
- The seven C6 value-coincidence records do not isolate a successful
  destination write because the pre-fix path corrupts a different register.
  They are retained explicitly rather than treated as pre-fix successes.
- Fingerprint full remains diagnostic; its 156,402 residual failures are not
  weakened or reclassified.
- M61 does not interpret or fix F7 `/2`, effective-address behavior, or any
  later-milestone family.

Human review commands:

```text
git fetch origin
git checkout topic/m61-upd9002-mov-imm-register
git rev-parse HEAD
git diff --stat a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b...HEAD
git diff a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b...90fa7dec5d46708a807851f61ae0792ee39e9b8f -- cpu/upd9002/
python3 tools/qa/upd9002_m61_mov_imm.py verify-static --root .
cmake -S . -B build/review-m61 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVA_BUILD_TESTS=ON
cmake --build build/review-m61 -j2
ctest --test-dir build/review-m61 --output-on-failure
git diff --check
git status --short
git rev-parse '@{u}'
git ls-remote origin refs/heads/topic/m61-upd9002-mov-imm-register
```
