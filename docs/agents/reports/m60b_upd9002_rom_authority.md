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
# M60b uPD9002 ROM authority and target-policy epoch

M60b formalizes content-addressed monitor-ROM authority and makes the one
v5-authorized target-policy correction for primary opcodes 6C through 6F. It
does not change production CPU instruction semantics. Denominator retirement
is recorded separately from semantic progress: no retired failure is counted
as newly passing and no retired pass remains a candidate pass.

M60b is complete and pushed. G60b is an unapproved candidate pending human
review. No production CPU semantics were changed. M60c and later milestones
have not been started.

A Git commit cannot contain its own SHA. The evidence commit and final
candidate are therefore the commit containing this report; their exact SHA is
supplied in the maintainer handoff and is independently available from
`origin/topic/m60b-upd9002-rom-authority`. The implementation and policy
actually evaluated are fixed below as `evaluated_sha`.

## Identity and preparation

- Approved predecessor gate: `G60a`
- Exact approved G60a SHA and M60b base:
  `ba2b7d3f5c76646b30d63fd8951f4a1964817b15`
- Approved G60a semantic/evaluated SHA:
  `3d66d41f750048eb29d13c4b7b53ea757d1d1921`
- Approved G60a CI:
  [build 30135508228](https://github.com/nakatamaho/vaeg/actions/runs/30135508228)
- Initial primary-worktree branch: `main`
- Initial primary-worktree SHA:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty with unrelated maintainer work
- Dedicated worktree:
  `/tmp/vaeg-m60b.Fcqput/worktree`
- Dedicated worktree starting SHA:
  `ba2b7d3f5c76646b30d63fd8951f4a1964817b15`
- M60b branch: `topic/m60b-upd9002-rom-authority`
- Implementation/policy commit and `evaluated_sha`:
  `23c5de2a7d28b35dd184201dee8d101607178510`
- Evidence commit and final candidate: the commit containing this report;
  exact SHA in the maintainer handoff and remote branch ref

The primary worktree was not cleaned, reset, stashed, staged, or modified.
The dedicated worktree was created directly from the fixed predecessor.
`origin/topic/m60a-upd9002-flags-materialization`, the approved report, and
the maintainer authorization all resolve G60a to the exact approved SHA. No
conflicting approved G60a SHA was found.

The mandatory preparation commands exited zero:

```text
git status --short
git branch --show-current
git rev-parse HEAD
git rev-parse ba2b7d3f5c76646b30d63fd8951f4a1964817b15
git show --stat --oneline ba2b7d3f5c76646b30d63fd8951f4a1964817b15
git rev-parse origin/topic/m60a-upd9002-flags-materialization
```

## Authoritative v5 specification

The supplied `upd9002_semantics_migration_v5.zip` had the required SHA-256:

```text
559554222439deb1b8081ff1929af1a74ac5f3ffd7051df3f1b4b562a1c77127
```

Every entry covered by the package manifest matched its declared digest.
The approved G60a tree did not yet contain the prospective v5 master/task
set, so the exact required master, ROADMAP, and canonical task documents were
added in the documentation-only commit:

```text
18e09771a241ba2f5fae50ca23d2ff3eb8a276e1
M60b: import v5 prospective campaign specification
```

The package README, concatenated task export, obsolete integer-numbered task
copies, and report template were not imported. The repository root
`README.md` was not changed, as explicitly requested by the maintainer.
Historical G58, G59, and G60a reports and artifacts were not rewritten.

`python3 tools/qa/milestone_ids.py --selftest --audit --discover` passed all
48 strict identifier checks and confirmed that ROADMAP, task discovery, the
v5 master, and
`docs/agents/tasks/M60b_upd9002_rom_authority_epoch.md` agree.

## Environment, corpus, and contracts

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

The verified corpus was available for every required SST run:

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

The dataset, both comparison contracts, and both selected hash sets remain
unchanged. Only the content-addressed target policy and consequently the
applicable hash set change.

## Approved G60a reproduction

Before policy implementation, a fresh tests-enabled build at the approved
G60a SHA reproduced all three profiles without skip:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 166,821 | 158,562 | 8,259 | 0 | 0 |
| architectural full | 1,562,502 | 1,443,876 | 1,383,294 | 60,582 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,443,876 | 1,280,856 | 163,020 | 0 | 0 |

Counts were not used as identity substitutes. Dataset, contracts, selected
and applicable sets, architectural and fingerprint pass/failure sets,
failure signatures, mismatch classes, termination classes, classifications,
taxonomy, registries, scoreboards, shards, and transitions matched approved
G60a exactly. In particular:

```text
G60a full transition SHA-256:
86b05dba8b958eb731c89c016cd9898b18ac5ff91a53229c0ef3a3aa797e8c13

G60a artifact-tree SHA-256:
2f03e42095da58d521ac5b491a571faf6d078db2b12794f2e0249354345c2901
```

M58 ratchet selftests/static verification, M59 evidence
selftests/static verification, and G60a scoreboard/transition verification
all passed. Immutable G43/M43 and approved G58, G59, and G60a evidence
remained byte-identical.

The candidate worker was then run under the old G60a target policy. The
complete pass/failure/signature/termination state reproduced the same three
approved rows exactly, proving that M60b tooling did not alter execution or
comparison behavior.

## Implementation and no-semantic-change audit

The implementation chain before evidence is:

```text
18e09771a241ba2f5fae50ca23d2ff3eb8a276e1
M60b: import v5 prospective campaign specification

8adc689325869a54528c39bb1f22934e6fb52db5
M60b: add ROM authority and target-policy tooling

c0afe65ab6ab8610ce16150fea897f95d1e6dff4
M60b: make protected-history checks available in CI

23c5de2a7d28b35dd184201dee8d101607178510
M60b: scope target-policy transition summaries
```

The final implementation commit adds:

- deterministic ROM-authority extraction and verification;
- target-policy identity and target-authority transition validation;
- structural prefix-aware 6C-6F selector resolution;
- retired-applicable pass/failure accounting;
- G43 reconciliation;
- scoreboard-v2 and transition generation;
- nine positive selftests and 46 fail-closed negative tests;
- CTest and hosted-CI integration.

The initial implementation CI correctly failed closed because the checkout
was shallow and could not resolve the protected predecessor for a semantic
diff. The follow-up CI-history commit changes only the five checkout steps
whose CTest execution requires protected history to use full history.

The first complete evidence-generation attempt then rejected its own CI
transition because a full-profile selector summary was compared with the CI
retirement count. The final implementation derives selector summaries from
the exact scope-local classification-change shards and verifies their
content-addressed ownership. A new positive test and fail-closed negative test
cover this distinction. The real CI transition then generated successfully.
Every artifact and all six profiles from the prior implementation SHA were
discarded and regenerated against the final `evaluated_sha`.

The required semantic audit is empty:

```text
git diff --exit-code \
  ba2b7d3f5c76646b30d63fd8951f4a1964817b15...23c5de2a7d28b35dd184201dee8d101607178510 \
  -- cpu/upd9002/
```

There are also no changes to SST fixtures, comparison contracts, immutable
G43/M43 artifacts, approved G58/G59/G60a artifacts, existing scoreboards, or
existing transitions. Production handlers for 6C-6F remain present and
unchanged. M60b does not implement or change 0F28, 0F2A, BRKFEM, BRKEM,
FPO1/FPO2, 66, or 67.

## ROM source authority

The out-of-tree source analyzed was:

```text
local path: /tmp/vaeg-m60b-authority/monitor.rom
role:       PC-88VA2 main varom00 monitor ROM
size:       524,288 bytes
SHA-256:    0460b58d4c5fa19cc8f9a2120bb7e65bbe7c78613552c3057a718444b76e8fee
CRC32:      98c9959a
SHA-1:      bcaea28c58816602ca1e8290f534360f1ca03fe8
mapping:    canonical address = ROM file offset
bank base:  0x60000
```

The complete copyrighted ROM and archived HTML snapshots remain out of tree.
The committed pack contains only content hashes, minimal deterministic
decoded table evidence, neutral provenance, and public evidence references.
That use was authorized by the maintainer's M60b task.

The debugger/runtime corroboration manifest has SHA-256
`dce0a562f2e46309a03157a37d18017233efed4efeefc6b1120b5a1755cf9599`.
Its analysis-only source was
`/tmp/vaeg-m60b-authority/debugger-evidence.json`.
It records three public archival sources with local snapshot digests:

| Post | SHA-256 | Evidentiary role |
|---:|---|---|
| 557 | `c01b045bbd8773076e0d258a6c292123ed407b2afb23a9dc4e399ab627972254` | identifies monitor-based investigation |
| 563 | `d8f62bb6c7bd7dc3ddbca2d88d282efdf065e44dc7cd976a0209f6f48d6fd51c` | independently reports BRKFEM/BRKEM encodings |
| 566 | `ae522c162dde4374d44a849d184bdcb525961378796c19bed386415262c95c7b` | corroborating VA2 runtime experiment |

Runtime/debugger material is corroboration. The ROM dispatch and mnemonic
tables are the primary target authority.

The authority pack is:

```text
tests/ssts/authority/g60b/
```

Authority-manifest SHA-256:

```text
f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac
```

## Complete V30-side 0F dispatch

The specification's nominal `0x66a8a` byte is the high-bit terminator of the
preceding `DS0` string. The first three-byte `(mask, value, group)` record
starts at file offset `0x66a8b`; the exact table is
`[0x66a8b, 0x66ab5)`. Pointer evidence at `0x66f80` proves the record/mnemonic
boundary. The adjacent high-bit-terminated mnemonic sequence is
`[0x66ab5, 0x66af7)`.

Exactly fourteen raw records were decoded:

```text
ff2000 ff2200 ff2600 ff2001 ff2201 ff2601 ff2802
ff2a02 f61003 f61603 f61203 f61403 ffff04 fffe04
```

The raw table slice digest is
`4348052cfe2891e0585b2cb3f5ccfd0f335efc89b1888a229ef3c0318735f453`.
Deterministic mask expansion and the parallel mnemonic table prove the
complete 23-opcode inventory:

| Second opcode(s) | Mnemonic |
|---|---|
| 10, 11, 18, 19 | TEST1 |
| 12, 13, 1A, 1B | CLR1 |
| 14, 15, 1C, 1D | SET1 |
| 16, 17, 1E, 1F | NOT1 |
| 20 | ADD4S |
| 22 | SUB4S |
| 26 | CMP4S |
| 28 | ROL4 |
| 2A | ROR4 |
| FE imm8 | BRKFEM |
| FF imm8 | BRKEM |

The complete table contains no entry for 0F31, 0F33, 0F39, or 0F3B. This
conclusion comes from complete table-boundary and expansion proof, not from
string absence. BRKFEM `0F FE imm8` and BRKEM `0F FF imm8` are independently
corroborated by the public debugger/runtime evidence. Their destination,
vector, return, and mode semantics remain unresolved.

## Complete primary dispatch and 6C-6F absence

The complete primary dispatch table is `[0x66350, 0x664f4)` and contains 140
three-byte records. Its parallel 140-entry mnemonic table is
`[0x66515, 0x666fa)`. Decoder code at `[0x6592c, 0x6598d)` hardcodes internal
record start `0x6350` and reads the mnemonic/end pointers at `0x0847` and
`0x0849`; pointer bytes at file offset `0x66f76` prove mnemonic start-minus-one
`0x6514` and record end `0x64f4`.

The complete record table has no entries for primary opcodes 6C, 6D, 6E, or
6F. Adjacent control entries 64 REPNC, 65 REPC, 68/6A PUSH, and 69/6B IMUL
are present, confirming the decoded table alignment. A separate 12-record
group-subdispatch at `[0x668fd, 0x66921)` was decoded only as corroborating
structure; it is not substituted for the complete primary table.

The string-pool audit searched exactly `[0x66600, 0x66f7a)` using
`exact-high-bit-terminated-ascii-v1`; the range digest is
`5d9b4d3898cfb5e867f556cb0cda80d837fd76f48e9601b736b81d80ad9a4e7d`.
It found no exact INM, OUTM, INS, or OUTS encoding, while ordinary IN and OUT
occur independently. This is corroborating evidence only. It also records
independent monitor evidence for REPC, REPNC, PREPARE, and DISPOSE.

## FPO/FPU non-evidence and deferred 66/67 audit

Generic FPO1/FPO2/ESC string presence or absence is non-probative. The same
monitor stores individual 8087 mnemonics including FADD and FMUL and contains
D8-DF-related records. The audit happens to find no FPO1/FPO2 exact string
and does find ESC, but neither result establishes an opcode class.

Primary 66 and 67 have no ordinary main-table entry; M60b deliberately draws
no support conclusion from that fact because an alternate dispatch path is
not excluded. Their classifications, gap kinds, support-map entries,
implementation, and hardware-pending coverage are byte-identical to G60a.
M60c owns the complete 66/67/FPO2 main-dispatch audit.

## Content-addressed target-policy epoch

The predecessor policy is derived without retroactively changing G60a:

```text
target_policy_before_id:
upd9002-g60a-derived-01b5d2f43b83cd7a43de2b162dba260ec5f6d3e681cf9a7ec9d114ee07c8a96c

target_policy_before_sha256:
01b5d2f43b83cd7a43de2b162dba260ec5f6d3e681cf9a7ec9d114ee07c8a96c
```

The new policy is:

```text
target_policy_after_id:
upd9002-g60b-eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93

target_policy_after_sha256:
eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93
```

The selected hash sets are unchanged:

| Scope | SHA-256 |
|---|---|
| CI | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` |
| full | `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` |

Applicable identities change only by exact retired 6C-6F hashes:

| Scope | G60a applicable SHA-256 | G60b applicable SHA-256 |
|---|---|---|
| CI | `80069e9a95f29b38e8f268b806f3ad8c7cb973c11d23b6cb64450ff00fc497cc` | `5a00d3fa15d55b38630015e771163fe58aff544d5f13342beac01a8236a485a1` |
| full | `7de13cbd54e709e0d0d0abefedac876306c8a67c7936f6a26c983362fed6d23c` | `a29c0a52d0818c7797515d5fbcc680b1fecdf5b10b896e5e02afff498cf99d65` |

The transition kind is exactly `target_authority_correction`. Structural
selectors decode the primary opcode after prefixes; they never inspect
pass/fail outcome. Segment and lock prefixes are dispatch-neutral constraints.
The five repeat classes per opcode are `none`, `repe`, `repne`, `repc`, and
`repnc`. Exact full-population selector ownership is:

| Form | Repeat class | Before | Count | Resolved-hash SHA-256 |
|---|---|---|---:|---|
| 6C | none | applicable | 501 | `2b90114fc72879ba1603b7f12547ee13600240e62e4fea875e024eef002e01c4` |
| 6C | repe | applicable | 127 | `95bdd83b4a94c2ce7b6bdf628a0f08c817318faebad7f1798411dfecca4a4ba7` |
| 6C | repne | applicable | 140 | `30c2c65155907f452c8a2cd6e8d554178e97ccf65501cddab33c8999ead46752` |
| 6C | repc | known_target_gap | 110 | `3485fcb103a145743898c5f7020734718498e6a45671759883a579d4352e0a10` |
| 6C | repnc | known_target_gap | 122 | `2a604cd0458d78af0fe4100a7175ee9742508be0240628ad03734c26ead141ff` |
| 6D | none | applicable | 495 | `b98ab4b0581ca2189a4e2219afa1bf09db943aa9c2eb230cf25ecec02fcd0729` |
| 6D | repe | applicable | 120 | `eee05c3cf928bab95414ed2662a20587180607e279e6e5ed24b6e1f011a2d6bb` |
| 6D | repne | applicable | 131 | `25480bade3c34146f29e044c260279b59db5fe8e1c9421311e5b0fe7f1175182` |
| 6D | repc | known_target_gap | 129 | `f89edba8709848087b884509e755ba8cdbc202e4bb6875d0f5a867e31a3be662` |
| 6D | repnc | known_target_gap | 125 | `825377ade51085376aa4b2a52cce66ae3d77990ab552cb92b4226dffb505976b` |
| 6E | none | applicable | 1,247 | `cfc3890a53aefd6ed3a9107d374c3dd61c6482f6231711ba886257704e5cef66` |
| 6E | repe | applicable | 304 | `34bd0106dc9b4b6490d0b85b76f4f0899edc3555a680c8b5b0852a618a13cefa` |
| 6E | repne | applicable | 326 | `1b2a71092bb82aac996d7be9df241ef87308bee8253d88308add19c0bd0d727b` |
| 6E | repc | known_target_gap | 317 | `aad69d15393b5b91ff3e6acee0fe33b5b08cf4c44b08df178a33a659d089260a` |
| 6E | repnc | known_target_gap | 306 | `40685bf4773fae71005f4591139351024a2545617e5a7109abdc87e005987d6b` |
| 6F | none | applicable | 1,246 | `deb16662b97922df0c761f77e9851626dc9a38a115f053af13fb7015cc2a7747` |
| 6F | repe | applicable | 323 | `ce2d12a1cb469b4ce746a7d43177854df91b469b545803f06798689ed2f2d5ad` |
| 6F | repne | applicable | 322 | `c313f18ca78c35b9d5c8465b5b47eaef896ab28921705054a91eea25529eb105` |
| 6F | repc | known_target_gap | 290 | `9f094a071893be388a0d2ef37976aeb054acadd3d53c75443c1b7424e412cc8c` |
| 6F | repnc | known_target_gap | 319 | `bf441d0fdf4fa09aa34ee41b2578b2f1c5aff9c305d911a9c75ed949c382d414` |

These 20 disjoint selectors cover all 7,000 selected full-profile 6C-6F
records and all 2,000 selected CI records, including segment overrides,
F2/F3, REPC/REPNC, and multiple-prefix sequences. Previously applicable
selector rows total 5,282 full / 1,521 CI. No selected 6C-6F record remains
applicable and no hash outside 6C-6F changes top-level classification.

The exact top-level changes are:

```text
5,282 full-profile hashes:
applicable -> known_target_gap/documented_silicon_absent

1,521 of those hashes are in the CI selection.
```

Existing 6C-6F known-gap hashes keep their top-level ownership and receive
`documented_silicon_absent` as necessary. Exact existing 0F31, 0F33, 0F39,
and 0F3B known-gap populations keep their selectors, hashes, counts, and
top-level ownership while their `gap_kind` changes from
`implementation_missing` to `documented_silicon_absent`.

| Form | Resolved count | Resolved-hash SHA-256 |
|---|---:|---|
| 0F31 | 5,000 | `2b1b75db2c86511f4d867541f69492e3f560befc2db583af2da2bf60402f77c7` |
| 0F33 | 5,000 | `c1cd39416c328c50e8d2e465dfefac70301d2fab12b8027abcb7e360882faab4` |
| 0F39 | 5,000 | `01a99962cf28a23976e65eb51e3f3ee3cf2924f42a687eadec6597171ff64351` |
| 0F3B | 5,000 | `f4f8ef9e179fc9a014303027c8eb1abb58d3268ef96ccf872c1426858c29662d` |

No other top-level classification or gap kind changes. In particular, 0F28
remains `known_target_gap/implementation_missing`, and 66/67 remain unchanged.

Taxonomy arithmetic is:

| Gap kind | G60a | G60b |
|---|---:|---:|
| documented_silicon_absent | 5,000 | 32,000 |
| implementation_missing | 63,626 | 41,908 |
| target_support_unverified | 0 | 0 |

## Retired-applicable accounting

The retired sets are exact, disjoint, complete, sorted, and fully enumerated
in deterministic content-addressed shards:

| Scope | Retired pass | Pass-set SHA-256 | Retired failure | Failure-set SHA-256 |
|---|---:|---|---:|---|
| CI | 1,383 | `a4f1558d4ca6b13a19a9fa56f77e247a32e8ed88c9ebfde4e2914ec36db378b0` | 138 | `0ac22864ffb11a0f6ff5dee2bf4c3b478b3af81905f7670db342686b41a94e2d` |
| full | 4,641 | `3890f8ed00f8a957ba0e343c934b6add9cdeec92728a9a8f07e613832adf0609` | 641 | `03f8ea83c510e67e27cc60a9455322f0cd899eb88287835080d2f9e98a0fa1f2` |

The required equations hold:

```text
full: 4,641 + 641 = 5,282
CI:   1,383 + 138 = 1,521
```

Retired passes are not G60b passes. Retired failures are not newly passing.
The lower blocking denominator and failure count are a target-authority
correction, not a semantic fix.

## Historical G43 OUTS reconciliation

G43's 1,204-hash OUTS fixture correction remains immutable historical V20
differential evidence. Its exact pass-population digest is:

```text
c8de1415733c5bad2ba85d667d56f5d04631d19379ce16f85e641792e7644322
```

All 1,204 hashes intersect the full retired-pass set. That intersection does
not become uPD9002 target progress and the fixture correction was not
reverted.

The exact G60a retired failure population is:

| Form | Count | SHA-256 |
|---|---:|---|
| 6E | 0 | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` |
| 6F | 641 | `03f8ea83c510e67e27cc60a9455322f0cd899eb88287835080d2f9e98a0fa1f2` |

The historical `6E = 417` and `6F = 224` labels are not the exact G60a
opcode-form split. The immutable G43 transition shows that the 641 remaining
OUTS failures consist of 417 unchanged-signature hashes and 224
changed-signature hashes:

| Historical subset | Count | SHA-256 |
|---|---:|---|
| unchanged signature | 417 | `7240eff77e38a2ca67cf94d6cec13c4ddec1f2e122cf62cbb7318ee39c82be2e` |
| changed signature | 224 | `f70b2e4e614cc677a883bc8d9ceb349f7a9bff32f185b253d893e6aea904a814` |

After target-policy correction these are no longer uPD9002 target failures.
The exact G60a hash resolution, rather than historical labels, governs the
retired accounting.

Explicitly, the historical `6E = 417` and `6F = 224` failure labels are not
uPD9002 target failures after this target-policy correction.

## Old-policy and target-correct profile results

The same worker from `evaluated_sha` ran each profile under both policies.
Every profile ran without skip and had zero timeout and zero crash:

| Profile/policy | Selected | Applicable/executed | Pass | Fail | Timeout | Crash | Elapsed |
|---|---:|---:|---:|---:|---:|---:|---:|
| architectural CI / G60a | 180,000 | 166,821 | 158,562 | 8,259 | 0 | 0 | 183.80 s |
| architectural CI / G60b | 180,000 | 165,300 | 157,179 | 8,121 | 0 | 0 | 183.97 s |
| architectural full / G60a | 1,562,502 | 1,443,876 | 1,383,294 | 60,582 | 0 | 0 | 672.39 s |
| architectural full / G60b | 1,562,502 | 1,438,594 | 1,378,653 | 59,941 | 0 | 0 | 666.76 s |
| fingerprint full / G60a | 1,562,502 | 1,443,876 | 1,280,856 | 163,020 | 0 | 0 | 677.53 s |
| fingerprint full / G60b | 1,562,502 | 1,438,594 | 1,276,215 | 162,379 | 0 | 0 | 668.07 s |

The exact arithmetic holds:

```text
candidate applicable = predecessor applicable - 5,282
candidate pass       = predecessor pass       - 4,641
candidate fail       = predecessor fail       -   641
```

The CI equations analogously subtract 1,521 applicable, 1,383 pass, and 138
failure hashes. The unchanged applicable intersection has no newly failing
hash, changed signature, termination regression, mismatch-class regression,
or per-form pass-count decrease. `newly_passing` and `newly_failing` are both
empty.

The exact result identities are:

| Profile/policy | Pass-set SHA-256 | Failure-set SHA-256 | Signature-index SHA-256 |
|---|---|---|---|
| architectural CI / G60a | `7db636be439d720eba65646a00869ff1904928b106757e7039510fca0b8c96d6` | `605b2eccd70262c4f6c981e5727a6fd8aa2073c175c977ed4b198985f31e3d39` | `b7f7916153a58aa53dac61cfd7b4a055d80ec8e98c658d066dc2b8b8788c6476` |
| architectural CI / G60b | `ae83f89ed1bfe34088197012683c726d9dab5a76700bb247efcf9a7e6e0d8bff` | `04ffb31baf4f66e60f0e700ebe8713ad7b1d582352160dc02c61415619426603` | `a67c9135780ce1df35486b2c2a28fe3e309be97c30045b4761ed5a8c652a4132` |
| architectural full / G60a | `2f91c2fb41bc83afb639ad56fa50e7b462740d6982a005ecfd7aebba761f41a8` | `bd77a1635c75827d977a0a03b39da7601f047ab7c8b9c3409023ec46dc55ed4b` | `23fee219c9d0c7afc120690b6db40d38b4ee82fa5d275c3c323d1a0dc3f34d58` |
| architectural full / G60b | `898b7e6e66fa9ea4e475bfc78db5a0cf8c3d6b109276fac6db0f68e05a1c27f2` | `9c69f518d168e1675e4d1fcf59db031dc1bce058db3fdfe9f6cb66b8d2bec91d` | `776c17c19e52cc03becc83464dfcdcb0b1590532812deac7e93c8955f0c1a473` |
| fingerprint full / G60a | `3502e257f5330b1d8c6b808525880bf40379038e1319838e8a04ef02bcb25751` | `8db523363ac5e839cb328edfcc1009417701e41188d804af6c2dbdef4483f807` | `eaaa9b6012e8ab03b816db6240d99bc5eb522beacb76cb2cbcad44cad78f598b` |
| fingerprint full / G60b | `691242bb0a05324fe3be261653863db45f45fd495c83d6798d8491a3e8db42db` | `2d825411e958ec980317d0c29d06aee8f9e7337c036f1d5703014311e53c2cc4` | `84c40271c91cb8d01e3545ada60c50e35594e790ce29e9b4d609c75e1d59d3bb` |

The content digests of the unchanged applicable result maps are:

```text
architectural CI:   278ac7e33980e2e4379b2051b30beb4d5f69977ed23b263d713b294f13484051
architectural full: 22a6c3c412dbdbec36a4de78a896b2edc96b148d82f3b7443303440baf801159
fingerprint full:   45b167ff126f2bde05be0f1766187990be197c2ad467c7802068b3620a217d91
```

## Generated G60b evidence

The evidence families are:

```text
tests/ssts/authority/g60b/
tests/ssts/target_policy/g60b.json
tests/ssts/target_policy/g60b_*/
tests/ssts/scoreboard/g60b_architectural_ci.json
tests/ssts/scoreboard/g60b_architectural_ci_failures/
tests/ssts/scoreboard/g60b_architectural_full.json
tests/ssts/scoreboard/g60b_architectural_full_failures/
tests/ssts/scoreboard/g60b_fingerprint_full.json
tests/ssts/scoreboard/g60b_fingerprint_full_failures/
tests/ssts/transitions/g60b_architectural_ci_from_g60a.json
tests/ssts/transitions/g60b_architectural_full_from_g60a.json
tests/ssts/target_policy/g60b_result_manifest.json
```

| Artifact | SHA-256 |
|---|---|
| authority manifest | `f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac` |
| target policy | `b8d43fd743f205149a54a280c8350bb88e129e4b3d648385f81a203d2cef0814` |
| architectural CI scoreboard | `fb4f98187ca33d9a7856378836ed897386b62e1e71d12ca7b209d9364e85c8ca` |
| architectural full scoreboard | `7fe94153c1976ab2aafef97c092425136b2c5969ca9d9fc073f2291b124afb98` |
| fingerprint full scoreboard | `b1bdb2472254dae48d275e7d6546b5e45dad326e748dcf01c0af08dadcd8d485` |
| architectural CI transition | `d59385813b9b37c0cf207d93a99d3b0afbd6cbc0fe0dc0dbe8453e1392da835d` |
| architectural full transition | `2396a03ec5b64033406f200458ef9d3d1e8bf805a551385c8bfbb07ed9493cdd` |
| result manifest | `c21ed7ff3538768cf569e07151e510e9c8df6ca651d8ced14c799eaf6e3d5415` |
| complete G60b artifact tree | `af1d979faa3d75019e3df6d419f6caf870c33dbb19b18a5c0d8010f33bd695c5` |

The full architectural transition is:

```text
tests/ssts/transitions/g60b_architectural_full_from_g60a.json
```

Its transition digest is
`2396a03ec5b64033406f200458ef9d3d1e8bf805a551385c8bfbb07ed9493cdd`.
All changed classifications and retired results are enumerated in the
referenced shards. No failure disappearance is represented as a pass.

Authority and target-policy generation ran twice in the same pinned
environment. Complete generated outputs were byte-identical. Final
scoreboard, shard, transition, and result-manifest generation also ran twice
from the same six raw results and produced a byte-identical artifact tree.

The bounded reproducibility claim is byte identity within the recorded
Python 3.14.4 / gzip 1.14 / zlib 1.3.1 environment. As accepted at G58,
compressed-byte identity is not claimed across arbitrary zlib
implementations. Canonical uncompressed content digests remain recorded
beside compressed-byte digests.

The final evaluated-SHA generation and verification commands all exited zero.
Measured wall-clock times were:

| Operation | First run | Second run |
|---|---:|---:|
| target-policy generation | 839.01 s | 847.89 s |
| architectural CI scoreboard | 229.40 s | 202.35 s |
| architectural full scoreboard | 336.98 s | 348.08 s |
| fingerprint full scoreboard | 339.41 s | 358.41 s |
| architectural CI transition | 211.35 s | 190.32 s |
| architectural full transition | 356.92 s | 292.05 s |
| result manifest | 751.30 s | 737.68 s |

Old-policy verification took 152.94 s, 314.90 s, and 327.77 s for
architectural CI, architectural full, and fingerprint full respectively.
The corresponding new-policy intersection checks took 213.56 s, 336.21 s,
and 344.36 s. Final static verification took 40.02 s. The repeated old/new
CI executions took 183.80 s and 183.97 s and reproduced the original raw
summaries and sidecars byte-for-byte.

## Validation and negative tests

`upd9002_m60b_authority.py selftest` passed nine positive checks and 46
fail-closed negative checks. The rejected cases include:

- wrong ROM SHA, truncation, missing boundary proof, malformed records,
  ambiguous/duplicate expansion, wrong mnemonic mapping, unexpected record
  count, absent-opcode contradiction, missing BRKFEM corroboration, incomplete
  string search, and nondeterministic authority output;
- incorrect prefix decoding, selector overlap/gap, outcome-derived selection,
  non-6C-6F selection, and hash-digest mismatch;
- unauthorized classification retirement, absent authority digest, wrong
  transition kind/gap kind, 0F28 or 66/67 change, and dataset/contract/selected
  set drift;
- retired-set overlap, omission, excess, wrong result, arithmetic/digest
  mismatch, and counting retired results as candidate progress;
- mutation of G43/M43, G58, G59, G60a, fixtures, comparison contracts, or
  `cpu/upd9002/`.

The standard native build completed successfully. A 47-test CTest invocation
started while the result manifest was still being generated: 46 tests
passed, including the no-skip external SST test, and the M60b static test
correctly failed closed on the incomplete evidence family. After the
manifest was complete, that exact static test was rerun and passed. Thus all
47 tests passed against the finalized tree. Repository encoding, EOL,
path-case, milestone-ID, and documentation validation passed.
`git diff --check` produced no output.

The exact principal commands were:

```text
cmake -S . -B build/linux-ci-gcc -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVAEG_BUILD_SDL2=OFF \
  -DVAEG_BUILD_TESTS=ON \
  -DVAEG_UPD9002_SSTS_DATASET_ROOT=<verified-corpus>
cmake --build build/linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure
ctest --test-dir build/linux-ci-gcc \
  -R '^vaeg_upd9002_m60b_authority_static$' --output-on-failure

python3 tools/qa/upd9002_m60b_authority.py selftest
python3 tools/qa/upd9002_m60b_authority.py verify-static
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

The six no-skip SST executions used
`upd9002_m60b_authority.py run-profile`, the same
`build/linux-ci-gcc/sdl2/vaeg` worker, explicit `g60a` or `g60b` policy,
architectural CI/full or fingerprint full scope, and the verified corpus.
The corresponding `verify-old-policy` and `verify-new-policy` commands
proved exact G60a identity and unaffected-intersection identity.

## Hosted CI

The evaluated implementation CI is:

[build 30142376883](https://github.com/nakatamaho/vaeg/actions/runs/30142376883)

All eight jobs succeeded, including repository invariants, Linux GCC/Clang,
ASan, Windows MinGW, macOS, standalone Z80 conformance, and the verified
uPD9002 architectural SST target-policy gate. The verified V20 corpus was
available and the required architectural CI profile was not skipped.

The final evidence commit triggers the same workflow. Its successful run and
URL are supplied with the maintainer handoff because a commit cannot contain
the URL of a workflow that starts only after that commit exists.

## Complete changed-file and protected-tree summary

Implementation/policy changes relative to G60a consist only of:

- the exact v5 master, ROADMAP, convention-neutral task set requested by the
  maintainer;
- `tests/ssts/schema/target-authority-v1.md`;
- `tools/qa/upd9002_m60b_authority.py`;
- narrow CMake and hosted-CI integration.

The final evidence commit contains only the generated G60b authority, target
policy, scoreboards, shards, transitions, result manifest, and this report.
It does not change tooling, tests, policy logic, CPU code, fixtures,
classifications outside the generated G60b epoch, or comparison contracts.

The following commands prove the semantic and protected-artifact boundary:

```text
git diff --stat \
  ba2b7d3f5c76646b30d63fd8951f4a1964817b15...23c5de2a7d28b35dd184201dee8d101607178510
git diff --name-status \
  ba2b7d3f5c76646b30d63fd8951f4a1964817b15...23c5de2a7d28b35dd184201dee8d101607178510
git diff --exit-code \
  ba2b7d3f5c76646b30d63fd8951f4a1964817b15...23c5de2a7d28b35dd184201dee8d101607178510 \
  -- cpu/upd9002/
```

The final command is empty and exits zero.

## Human review commands

From a clean checkout of the final candidate with the verified corpus and
the authorized out-of-tree evidence:

```text
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/qa/upd9002_m60b_authority.py selftest
python3 tools/qa/upd9002_m60b_authority.py verify-authority \
  --pack-root tests/ssts/authority/g60b \
  --rom /tmp/vaeg-m60b-authority/monitor.rom \
  --debugger-evidence /tmp/vaeg-m60b-authority/debugger-evidence.json
python3 tools/qa/upd9002_m60b_authority.py verify-static
ctest --test-dir build/linux-ci-gcc --output-on-failure
git diff --exit-code \
  ba2b7d3f5c76646b30d63fd8951f4a1964817b15...23c5de2a7d28b35dd184201dee8d101607178510 \
  -- cpu/upd9002/
git diff --check
```

`verify-static` checks the policy, all scoreboards/shards/transitions, result
manifest, protected artifact digests, fixture/contract identity, no
production semantic diff, exact 6C-6F ownership, 0F28 and 66/67 preservation,
and the one-shot nature of `target_authority_correction`.

## Known limitations

- The monitor ROM proves debugger/monitor target authority, not complete
  uPD9002 silicon validation.
- BRKFEM and BRKEM encodings are proven; their execution, vector, destination,
  return, and mode semantics are underdetermined.
- The absence of 66/67 from the ordinary main table is not enough to resolve
  FPO2. M60c owns the alternate/main-dispatch audit.
- 0F28 remains an implementation gap despite being present in ROM authority.
- The G43 OUTS correction remains useful V20 differential evidence but is not
  uPD9002 target progress.
- Deterministic gzip byte identity is bounded to the recorded environment,
  matching the limitation already reviewed and accepted at G58.

G60b remains unapproved pending human review. M60c is untouched.
