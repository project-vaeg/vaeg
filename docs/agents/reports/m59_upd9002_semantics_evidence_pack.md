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
# M59 uPD9002 semantics evidence pack

M59 adds a versioned, deterministic, content-addressed evidence pack for six
future uPD9002 semantic work areas. It replays 160,000 complete SST records
and records initial, expected, and actual state side by side. It does not
change production CPU semantics, comparison semantics, fixtures,
classifications, or any approved G58/G43 evidence.

M59 is complete and pushed. G59 is an unapproved candidate pending human
review. No production CPU semantics were changed. M60 and later milestones
have not been started.

A Git commit cannot contain its own SHA. The evidence commit and final
candidate are therefore the single commit containing this report; their exact
SHA is supplied in the maintainer handoff and is independently available as
`origin/topic/m59-upd9002-evidence-pack`. The report does not create a
self-referential artifact. The separately committed analysis implementation
that was actually executed is fixed below.

## Identity and preparation

- Approved predecessor gate: `G58`
- Exact approved G58 SHA and M59 base:
  `bc8a55c6da1082b85b794068e0d933e31fe46b13`
- Approved G58 implementation/evaluated SHA:
  `d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53`
- Approved G58 CI:
  [build 30067599396](https://github.com/nakatamaho/vaeg/actions/runs/30067599396)
- Initial primary-worktree branch: `main`
- Initial primary-worktree HEAD:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty, with unrelated tracked and untracked
  maintainer files
- Dedicated M59 worktree: `/tmp/vaeg-m59.G89RCI`
- Dedicated worktree starting SHA:
  `bc8a55c6da1082b85b794068e0d933e31fe46b13`
- M59 branch: `topic/m59-upd9002-evidence-pack`
- Analysis implementation commit and `analysis_evaluated_sha`:
  `7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7`
- Evidence commit: the commit containing this report; exact SHA in the
  maintainer handoff and remote branch ref
- Final candidate SHA: the same evidence commit; exact SHA in the maintainer
  handoff and remote branch ref

The primary worktree was not reset, cleaned, stashed, staged, or modified.
The dedicated worktree was created directly at the fixed predecessor. The
maintainer-supplied G58 SHA, the authoritative G58 report, the approved G58
branch, and the repository's G58 artifacts agree. No repository metadata
records a conflicting approved G58 SHA. The artifacts continue to record
their intentionally different `evaluated_sha`:

```text
d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53
```

The preparation commands were:

```text
git status --short
git branch --show-current
git rev-parse HEAD
git rev-parse bc8a55c6da1082b85b794068e0d933e31fe46b13
git show --stat --oneline bc8a55c6da1082b85b794068e0d933e31fe46b13
git rev-parse origin/topic/m58-upd9002-ssts-ratchet
```

All exited zero. The approved predecessor was not inferred from HEAD,
`main`, a mutable tag, or a branch tip.

## Environment

| Component | Recorded value |
|---|---|
| Host | `Linux-6.18.33.2-microsoft-standard-WSL2-x86_64-with-glibc2.43` |
| Distribution | Ubuntu 26.04 under WSL2 |
| Git | 2.53.0 |
| CMake | 4.2.3 |
| Ninja | 1.13.2 |
| Compiler | `cc (Ubuntu 15.2.0-16ubuntu1) 15.2.0` |
| Python | 3.14.4 |
| gzip command | 1.14 |
| Python gzip module | `/usr/lib/python3.14/gzip.py` |
| zlib compile/runtime | 1.3.1 / 1.3.1 |

The verified corpus was available for every required run:

```text
/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
```

Its identity is:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

The architectural contract is `upd9002-v20-architectural-v1`, SHA-256
`aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5`.
The all-16-bit diagnostic contract is `upd9002-v20-fingerprint-v1`, SHA-256
`47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e`.

## Commit and file separation

The implementation chain is:

```text
726039882b4c986b7009c0e570517770884fd876
M59: add deterministic uPD9002 semantic evidence tooling

7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7
M59: canonicalize representative evidence endings
```

The first commit contains only:

- the read-only generator/validator
  `tools/qa/upd9002_semantics_evidence.py`;
- two JSON schemas and one schema description;
- CMake and hosted-CI integration for the validator;
- the authoritative M59 task;
- ROADMAP and SST documentation needed to describe M59.

It changes nine paths, adding 3,831 lines and deleting three documentation
lines. It was pushed before evidence generation and was not amended. The
second implementation commit changes two generator/validator lines so every
generated representative Markdown file has exactly one final LF and the
validator rejects an extra blank line at EOF. Staged `git diff --check`
detected the pre-canonical output before any evidence commit existed. Those
generated files were discarded, and every profile and artifact was rerun
against the second commit. That commit is the final `analysis_evaluated_sha`.

The later evidence-only commit contains only:

- 20 generated files under `tests/ssts/evidence/g59/`; and
- this report.

It contains no tooling, test, harness, classification, fixture, or CPU source
change.

## Approved G58 reproduction before editing

A fresh tests-enabled GCC build at exact G58 was created in
`/tmp/vaeg-m59-g58-build.lQbpnQ`. Before any M59 edit, all three approved
profiles ran with the verified corpus available. No required profile was
skipped.

| Profile | Selected | Executed/applicable | Pass | Fail | Classified non-applicable | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 166,821 | 156,228 | 10,593 | 13,179 | 0 | 0 |
| architectural full | 1,562,502 | 1,443,876 | 1,359,547 | 84,329 | 118,626 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,443,876 | 1,257,109 | 186,767 | 118,626 | 0 | 0 |

The architectural summaries and failure sidecars were byte-identical to the
immutable M43 material. Rebuilding the three G58 scoreboard families
reproduced the approved hash sets, signatures, classifications, terminations,
and failure shards. The approved G58 artifact tree remained 34 files and
30,187,370 bytes with tree digest
`44776ad3a961ae564517ae0aa17e2987d3732c9d949774cbe77dce413092e1c2`.

| Immutable/profile evidence | SHA-256 |
|---|---|
| G43 manifest | `77dd1e53f325f3910bd727d3dec4b9c1e23c005f0b306c085a34569e9cf5b23f` |
| M43 CI summary | `a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31` |
| M43 CI failure index | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` |
| M43 full summary | `dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9` |
| M43 full failure index | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |
| M59 fingerprint raw summary | `02c758b3f5cdac87011737c05cb9e9cceacf3ee9fa7d396470382d2afa33228f` |

Hash-level identities were:

| Identity | CI | Full |
|---|---|---|
| selected hashes | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` |
| applicable hashes | `80069e9a95f29b38e8f268b806f3ad8c7cb973c11d23b6cb64450ff00fc497cc` | `7de13cbd54e709e0d0d0abefedac876306c8a67c7936f6a26c983362fed6d23c` |
| architectural pass hashes | `053195c1ac7001fa553da6201cd9e7a6843e8de2846796ca608cfca09ab30c78` | `64e33aabb7ad4e926c329105a13bf917559e8e6ee37ebc784def8a52256fdee1` |
| architectural failure hashes | `6129fb38d6ad32d739027819ec24e0ffe7caba26ef7e8e7e595f9cfd987cb63e` | `cd3bac7b62f0c661d1c660dfadd13cb9e8690dca6d66449c792b3bc1b06d57b4` |
| architectural signature index | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |

Fingerprint full pass hashes are
`93bb15a8b318fe0aca43855c66cd8eda2acb8d8b941cc320fdb8dfc44769a4bd`;
failure hashes are
`deee927e54977788f0d0222ff5cdbdc0f4a0dace1e9deb8005d51b67c3b71299`;
the signature index is
`b84a518fbfb034eca55f708d60a327b75ab16a872a488e63b1461fef23e408c2`.

The approved taxonomy and orthogonal registries are unchanged:

| `gap_kind` | Rules | Resolved hashes |
|---|---:|---:|
| `documented_silicon_absent` | 1 | 5,000 |
| `implementation_missing` | 39 | 63,626 |
| `target_support_unverified` | 0 | 0 |

The taxonomy SHA-256 is
`541001ae960b487ca4ee932de2cd764936e08af141dcaca1fb456a82f76eb38e`.
Hardware-pending coverage remains exactly 0 of 0; the empty registry SHA-256
is `cb1b55739278badea0dd1c5a10b0becb45ee73394715cf8b88077a513e5e72cc`.
The approved-divergence registry remains schema-valid and empty, SHA-256
`dffc832b8f163bcf1783220ca0484ad432195105722bc0a40b441f656478c6e7`.

## Exact implementation-commit profile results

The final implementation commit was configured and built from scratch in
`/tmp/vaeg-m59-impl-build.Zdv61g`. The verified architectural CI CTest ran
without profile skip. Architectural full and fingerprint full then ran
concurrently, finishing in approximately 463 and 478 seconds.

| Profile | Selected | Executed/applicable | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 166,821 | 156,228 | 10,593 | 0 | 0 |
| architectural full | 1,562,502 | 1,443,876 | 1,359,547 | 84,329 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,443,876 | 1,257,109 | 186,767 | 0 | 0 |

Architectural CI/full matched immutable M43 byte-for-byte. Regenerated full
architectural and fingerprint scoreboards differed from the approved G58
JSON only in the explicit M59 `evaluated_sha`, generated path names, and
path-derived aggregate fields. Every generated failure shard was
byte-identical to its G58 counterpart. Dataset, contracts, selected,
applicable, pass, failure and classification sets, signatures, mismatch
classes, and termination classes were exact. There was no new failure,
missing record, changed signature, timeout, crash, or classification drift.

## Evidence-pack format and manifest

The pack is:

```text
tests/ssts/evidence/g59/
```

Its manifest schema is
`vaeg-upd9002-semantics-evidence-manifest-v1`; case and summary records are
version 1. The top-level manifest SHA-256 is:

```text
acfe153ff686ed9e4a1b093c954e36bab8c306d6dbb03fc30a0a5096e981a1a0
```

The pack contains 160,000 case rows. All populations were analyzed in full;
no sampling was used.

| Item | Case artifact, rows, SHA-256 | Summary SHA-256 | Representative dump SHA-256 |
|---|---|---|---|
| FLAGS | `cases/flags.json.gz`, 35,000, `ce0db24dd325a6d530b3ccd81ac25b5e0af3f6d5c3a69b49f77facd164327052` | `78c39d469feb80459212af721608436b83061fe44884e8f27726555c4075ae4d` | `bdcabea0aa13068a4e5f7d52694a0a2d84944b590020c5fe3cb6cc2774155ef1` |
| canary | `cases/canary.json.gz`, 15,000, `d259145947fadaf514f0cee84b59971995a13ce334b60fe190ea421d0c65a1a9` | `4ba9ada4f10ca826509a6f88e7fb551806b5e5e5023ea11f043db8a3259dd0af` | `11033b30f9c2f4290a8c43b7a5ea48890b9ea5ac3789b196a7e9f5c4d0b540b6` |
| D4/D5 | `cases/d4_d5.json.gz`, 10,000, `dfb8a1ec1dcddedf8f207416bedefc82414709b929b10b787ec525a796df642f` | `4ef1d5da183a922b49c4b624d1ac470ff3f3c776a28e095c48c2281aaecedfb6` | `be2e42a6a01224bf9634e532b38fa77ee116dd1f01d724dbe09e52381ab6c360` |
| 0F28/0F2A | `cases/0f28_0f2a.json.gz`, 10,000, `df7c49c19f122785e2beac1689852db18c3660c367bdc8fa97446195036cc452` | `abab991b07d15c2eebfe97c3d6fb9e0ce480bb18395976aaae027d220d46c7a5` | `f7dec097630c91f9c4f68230433bb3fc49cb79506569625d50338a394e543666` |
| shifts | `cases/shifts.json.gz`, 80,000, `004da3afbf6cc5c4ea55d52e10fadfd0617038a1e5724f8c7d58dcdf9a060d5f` | `d9921f8a2b7acbd77076b946292a7aadf236525e529d4c4ac3793f2166a0ad34` | `d3ab9cc67f8550767ba5ac15a517102a3c216c12e1b2b61a2e29ad559b8e0bb1` |
| FF/BOUND | `cases/ff7_bound.json.gz`, 10,000, `60ae4d4ba7b53d8af03e19e8079c550d8a73b56f47c8b00934cdf9a646d9a7d2` | `ec68c1de6edab3fcf7f91bea65c067ea13b3b8b3343569e774c2990f77c58114` | `27c0bd9804fe2bbd21ffa26b4d7306b0aa44984f866ca4c0fb8b36e41c73c6f3` |

Every case row carries classification, gap taxonomy where applicable,
selection/execution state, instruction bytes, initial state, expected state,
actual state, terminations, registers, final SST RAM, mismatch classes,
logical/physical addresses where derivable, structural partitions,
conclusion status, and notes. Expected-only diagnosis is rejected.

Representative hashes include:

| Item | Representative hashes |
|---|---|
| FLAGS | `0000c3973227bb631ff5eb928614199f783d134bdd810d01c142e6f9ce544b4b`, `0011cd2cfaf2f1fcd8c58e112c8d2fa321cd57d23694c93c75dd2b6c09615874`, `00058e8d5153ad9cd3e08f5b8539ca66bfbd22cb5c50bc9eb2f1960a8118cc81` |
| canary | `0007c6816eaa00b97019651aaf5fee7f5fc7d81661413e69aaaf6952ac441325`, `00098a1727add96bce60dc61cb11cf62ee357fc68e8d4d18ac76abd7dd85e658` |
| D4/D5 | `10350314af809c7498c9fddf1b870a93d62601b6dcfa2633b1edbe725ded63e0`, `08a7e02e2b4efb78526bf33200d5ced28f44f1a715e0cb7c2451f00ae40798c2` |
| 0F28/0F2A | `0001478c5d1bde99b561bfda6c953eb38592349debdaf1d3d40e53f97187a2e6`, `000a933d0ba529d3d94900849aa6beac35d81c6c8b9b620453611c864d13dc36` |
| shifts | `00004758f4f2be410b56d026bc0e3672db3bc7b8ef0836e66fa4ff8d101f9102`, `00014ccd3883413513d9d284ef62b954788eb952de047c996a56d8fe314efe95` |
| FF/BOUND | `000584448ce54b755fc3323e057dc1fad60f8b9535133b68866f848a1cdc8071`, `001bd4ca1c82cd061dfe8ef5c5bef7e87acaf8803fa6b957db9d52becf66e03c` |

The readable files under `representative/` contain the complete selected
representative lists and side-by-side state dumps.

## Item 1: guest-visible FLAGS materialization

All seven forms are applicable, selected 5,000 times, and executed 5,000
times:

| Form | Architectural pass | Architectural fail |
|---|---:|---:|
| 9C PUSHF | 4,999 | 1 |
| 9D POPF | 1,238 | 3,762 |
| 9E SAHF | 1,262 | 3,738 |
| 9F LAHF | 5,000 | 0 |
| CC | 0 | 5,000 |
| CD | 0 | 5,000 |
| CE | 2,532 | 2,468 |

`proven`: The complete 12,468-case interrupt population has matching expected
and actual frame placement. Its hash-set digest is
`4498c8aa838f93aba7220f0cdacff34341d704a9cbe7f6d35d79b75219b41d0b`.
Observed final RAM places six frame bytes at final `SP` through `SP+5`, with
saved IP, CS, and FLAGS values decoded from those observed bytes. There are
12,070 non-boundary frames, 397 physical-boundary frames, one 64-KiB
segment-boundary frame, and 2,532 CE records for which no interrupt is
expected. No frame mapping remained underdetermined.

The independent PUSHF population has 4,853 non-boundary, 146 physical-wrap,
and one segment-wrap case.

The SST-observed guest-visible bit rules are:

| Bits | Expected interrupt image | Actual interrupt image | Expected PUSHF image | Actual PUSHF image |
|---|---|---|---|---|
| 0-11 | copied | copied | copied | copied |
| 12-15 | copied | forced-zero | copied | underdetermined |

The PUSHF high-bit result is marked `underdetermined`, not inferred from the
interrupt result: one segment-wrap record has a high-byte write anomaly, so
the full population cannot independently establish a uniform actual rule.

`proven`: LAHF expected and actual AH rules agree: AH bits 0, 2, 4, 6, and 7
copy FLAGS bits 0, 2, 4, 6, and 7; AH bit 1 is forced one; bits 3 and 5 are
forced zero.

| Instruction | Expected-only differences from actual observed bit rules |
|---|---|
| POPF | Expected bits 3 and 5 forced zero; actual bits 3 and 5 are loadable. Other bit rules agree. |
| SAHF | Expected bits 3 and 5 forced zero; actual bits 3 and 5 are loadable. Other bit rules agree. |

`underdetermined`: Similar reserved-bit outcomes do not prove that
interrupt-frame materialization, PUSHF, POPF, and SAHF use one implementation
primitive. Guest-visible stack/AH images, final metadata-masked
architectural FLAGS, and full 16-bit fingerprint FLAGS remain separate
domains in the pack.

## Item 2: F7 /2, C6, and C7 canary cluster

| Form | Selected/executed | Pass | Fail | Failure digest |
|---|---:|---:|---:|---|
| C6 | 5,000 | 3,912 | 1,088 | `2def4cc309f2a11b5950d4708ae1093e661e0d57e636c7f6600262d7efe8abe3` |
| C7 | 5,000 | 3,880 | 1,120 | `640e24a7c324690e73c72db449f3d6a750dca66b690fd35f021317c82816394a` |
| F7 /2 | 5,000 | 3,887 | 1,113 | `69bf316c8a0751f7aed67504d0ea606fd2530e8d254b2b4e73ead66ccbc30ccc` |

`proven`: Every C6/C7 memory form passes. Every C6/C7 failure is a register
form whose actual destination remains equal to its initial value. The 161 C6
and 154 C7 register passes are value coincidences, not proof of execution.

`proven`: Every F7 /2 register form passes. Every F7 /2 failure is an even
physical address below `0xa0000` where the low byte is inverted and the high
byte remains initial. The failing low byte is at the expected address, so a
general effective-address error is not sufficient to explain this failure
set.

`proven`: There are zero unexpected FLAGS changes. C6/C7 16-bit offset-wrap
and 20-bit physical-wrap memory cases pass. F7 /2 physical-wrap cases pass;
its failures remain tied to the low-memory word path.

`underdetermined`: Final-state SST evidence cannot establish transient
displacement/immediate fetch ordering.

`underdetermined`: One shared canary primitive is not supported. C6/C7 fail
only on register destinations, while F7 /2 fails only on a particular memory
word path. Future semantic work should not combine them merely because they
were previously grouped as canaries.

## Item 3: D4 and D5

Both forms are applicable, selected, executed, and terminate normally in all
5,000 records.

| Form | Architectural pass/fail | Fingerprint pass/fail |
|---|---|---|
| D4 | 197 / 4,803 | 19 / 4,981 |
| D5 | 5,000 / 0 | 649 / 4,351 |

The D4 architectural failure hash-set digest is
`e0ffd2df098de38bc99cc0fc455b351a266baff2d74bddeb3e2f1fc0e857b731`.

All required immediate strata were present and executed for both forms:
0, 1, 2, 9, 10, 11, 16, and 255. D4 immediate zero contains 27 records; all
27 have expected and actual normal termination and all 27 have register
mismatches. SST evidence therefore does not show divide-error termination
for this population.

| Immediate | D4 selected: pass/fail | D5 selected: pass/fail |
|---:|---:|---:|
| 0 | 27: 0/27 | 22: 22/0 |
| 1 | 17: 0/17 | 16: 16/0 |
| 2 | 27: 0/27 | 22: 22/0 |
| 9 | 16: 0/16 | 12: 12/0 |
| 10 | 14: 11/3 | 26: 26/0 |
| 11 | 14: 0/14 | 12: 12/0 |
| 16 | 15: 1/14 | 19: 19/0 |
| 255 | 30: 0/30 | 20: 20/0 |

`proven`: D5 is architecturally green because all 5,000 selected records were
executed and passed, not because it was absent from a failure list.

`proven`: D5 is not fingerprint-green; its 4,351 diagnostic failures show
full-FLAGS differences.

`underdetermined`: M59 does not assign an implementation cause to D4, and
does not justify a D5 architectural semantic change.

## Item 4: 0F28 and 0F2A

The approved G58 repository evidence contradicts one statement in the M59
task prompt: at exact approved SHA
`bc8a55c6da1082b85b794068e0d933e31fe46b13`, the 5,000 0F28 records are
annotated `implementation_missing`, not `documented_silicon_absent`.
`documented_silicon_absent` belongs to a different 5,000-record rule. M59
does not rewrite or reinterpret the approved epoch.

The exact 0F28 annotation is:

```text
selector_sha256 =
d4978211d0588687f1e04486b42209460c585a89126367df76742a749463ae01

resolved_count = 5000

resolved_test_hashes_sha256 =
1d01e7d8ec9cd05fa804acc5c9cb7e30cc451f8eea710847826b15b0622ef247
```

`proven`: 0F28 remains `known_target_gap`. It was selected 5,000 times and
officially executed zero times. Its separately labelled diagnostic replay is
not a pass result. The SST expected state describes ROL4. The current target
diagnostic leaves the destination and AX unchanged in all 5,000 records and
consumes prefixes plus the two opcode bytes, but not ModR/M/displacement
bytes. The diagnostic hash-set digest is
`7aa79eb2754eab51104c07689016e4782c97406afbe0618a33bf824d0b8ff83f`.

`proven`: 0F2A is applicable and executed 5,000 times, with 308 pass and 4,692
fail. SST expected state copies the full source byte to AL in all 5,000
records. Actual state exhibits a low-nibble merge in 4,832 records. The
failure digest is
`4bbe0bf9537bbae74bb0c7d9c2e94bfa82a6ac0f3283945e6841de36c48bf3a3`.

`hypothesis`: The 0F2A evidence is consistent with a local conceptual
misinterpretation rather than a shared 0F28 primitive. M59 does not prove the
implementation cause and makes no fix.

`underdetermined`: Available NEC V20/V30 primary documentation establishes
the V-series operations, but the inspected set contains no acquired
uPD9002/PC-88VA primary manual proving target support or absence for 0F28.
The M47 primary-document manifest and report remain pinned at
`2fe6d19336d091f31f98211ae9056e68a9ab453505ce72632ed40fb3fd2819cc`
and
`3a0543f960a5b79a55f48d5fee071325b0b5549c4e73c7b680a4879a928734c3`.

## Item 5: rotate and shift forms

All 80,000 records are applicable, selected, executed, and normally
terminated.

`proven`: C0/C1/D2/D3 rotate subforms `.0` through `.3` were each selected
and executed. All 40,000 are architecturally green. The complete rotate hash
set digest is
`638a5692ff2b2b98dc37ac0ee7d23458e1ca5185054b3937985144599a3b3b83`.
Some 16-bit rotate forms have fingerprint-only FLAGS differences; the pack
does not infer fingerprint-green status from architectural pass.

Shift subforms `.4` through `.7` have 19,139 architectural failures, digest
`85c431ba4d46a285aa6c352192ba1b583ac3aadd739b6154e79c8d96f0b06bce`.
The pack contains 2,953 exclusive structural strata, 384 required count
strata, and 1,152 count/FLAGS observation rows, covering width, register or
memory destination, subform, count source/value, initial sign, initial CF,
termination, destination, RAM, and seven observed FLAGS bits.

The evidence evaluates raw and `count & 0x1f` models as competing
observations:

| Observation | Expected rows | Actual rows |
|---|---:|---:|
| raw only | 6,692 | 609 |
| mask-1f only | 0 | 6,083 |
| both models agree | 33,308 | 21,169 |
| neither model agrees | 0 | 12,139 |

`proven`: The data do not support one unconditional assertion that all counts
are masked, or that none are masked.

`underdetermined`: A shared count-normalization primitive and a shared
FLAGS-materialization primitive remain separate hypotheses. Count zero,
count one, greater-than-one, at-width, beyond-width, and fingerprint results
are reported independently in the machine summary.

## Item 6: FF /7 and BOUND

Both forms are applicable, selected 5,000 times, and executed 5,000 times.

`proven`: SST expected and target actual termination are normal for every
FF /7 record; M59 does not assume a 286 `#UD`. All 5,000 fail on final state.
The exact normal-termination hash set digest is
`6028d5dcd4b6a3dcded2aaf69fb186e502f7f5a4d094180572f802c86240039a`.

For 4,848 non-register-SP forms, final SP advances by two, digest
`c0ea14bd1b4376e52052e2f1b7078218671d55232b9c25a343a5f2b0b42259d9`.
This is consistent with, but does not prove, a POP-like path. The 152
register-SP forms finish with SP zero, digest
`a1c3f8f51558c89364b50f41209c74917e2741be2469d1e65a2b4a888e1dbda0`;
transient read ordering is `underdetermined`.

BOUND has 191 pass and 4,809 fail:

| Expected/actual execution | Count |
|---|---:|
| INT 5 / INT 5 | 3,565 |
| INT 5 / normal | 611 |
| normal / INT 5 | 633 |
| normal / normal | 191 |

The 3,565 frame-only failures have digest
`15862f179608f8745f76bb3565197106ae6f63cba6c3363dd307fb29e6bbd746`.
The 1,244 range/non-frame-only failures have digest
`2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d`.
Execution telemetry resolves the range decision, so zero cases are
underdetermined solely because the final frame is corrupt.

## Cross-item dependency analysis

No implementation-level shared primitive is claimed solely from a matching
mismatch signature. In particular, M59 establishes no `proven shared
primitive`; it records probable and possible candidates for future controlled
tests, independent failures, and underdetermined relationships.

| Proposed dependency | Exact supporting population | Assessment | Counterevidence / rationale | Smallest future test |
|---|---|---|---|---|
| interrupt-frame saved-FLAGS materialization across CC/CD/CE | 12,468 hashes, `4498c8aa838f93aba7220f0cdacff34341d704a9cbe7f6d35d79b75219b41d0b` | probable shared primitive | identical high-bit result and placement, but final-state evidence cannot prove call-path identity | M60a; regenerate architectural CI/full and fingerprint full |
| BOUND frame-only failures depend on interrupt-frame work | 3,565 hashes, `15862f179608f8745f76bb3565197106ae6f63cba6c3363dd307fb29e6bbd746` | probable shared primitive | the other 1,244 BOUND failures are range-decision mismatches | M60b after M60a; regenerate BOUND and global ratchet populations |
| POPF/SAHF and interrupt delivery share FLAGS machinery | FLAGS failure set 19,969, `bc3f6e4ba3b685400bace87a4891331e69eb8de47339c79b71564e58012f4c75` | underdetermined | load and materialization directions differ; outcome overlap is insufficient | M60a with per-family assertions |
| C6/C7 and F7 /2 share one canary primitive | 3,321 hashes, `7dd060ba703398307dbb8e79a0d427bbd2e52e6dffde49f9ceac6e00df2cdf01` | independent failures | C6/C7 are register no-ops; F7 /2 is memory-only low-byte RMW | split M61; regenerate each affected form plus global ratchets |
| D4 and D5 require one semantic fix | D4 4,803 failures, `e0ffd2df098de38bc99cc0fc455b351a266baff2d74bddeb3e2f1fc0e857b731` | independent failures | D5 is architecturally 5,000/5,000 | M62a for D4 only |
| 0F28 and 0F2A share one operation primitive | 0F2A 4,692 failures, `4bbe0bf9537bbae74bb0c7d9c2e94bfa82a6ac0f3283945e6841de36c48bf3a3`; 0F28 gap digest above | underdetermined | 0F28 is nonexecuted and target support is not established | evidence-only target-authority task before conditional 0F28 work |
| C0/C1/D2/D3 shifts share count normalization | 19,139 hashes, `85c431ba4d46a285aa6c352192ba1b583ac3aadd739b6154e79c8d96f0b06bce` | possible shared primitive | raw/masked/neither observations coexist; width and destination counterexamples exist | first split of M63; regenerate all shift and global ratchet populations |
| FF /7 belongs to BOUND/interrupt work | FF /7 5,000 hashes, `6028d5dcd4b6a3dcded2aaf69fb186e502f7f5a4d094180572f802c86240039a` | independent failures | FF /7 completes normally and has state-side effects; BOUND has range/INT5 partitions | separate M65 residue tasks |

The labels above use the campaign's dependency vocabulary. Conclusions about
observed output are `proven` where stated in the item sections; implementation
causes remain `hypothesis` or `underdetermined` unless a future controlled
semantic change tests them.

## Recommended future ordering

This is a recommendation for explicit human approval. It neither renames nor
starts a later task.

1. **M60a — required by proven dependency:** preserve its FLAGS
   materialization scope and independently test interrupt frames, PUSHF,
   POPF, SAHF, and LAHF. Correct guest-visible images before treating frame
   consumers as isolated.
2. **M60b — required by proven dependency:** retain interrupt-frame work
   after M60a. Include the exact BOUND frame-only population as a dependent
   ratchet, without absorbing BOUND range semantics.
3. **M60c — recommended by evidence:** retain IRET after frame representation
   is fixed and gated.
4. **M61 — recommended by evidence:** split the provisional combined canary
   task into C6/C7 register-form execution and F7 /2 memory-word RMW work.
   Exact identifiers require maintainer approval; do not implement both under
   a false shared-EA premise.
5. **M62a — recommended by evidence:** keep D4 AAM work; make D5 semantic work
   conditional because D5 is architecturally green.
6. **M62b1 — recommended by evidence:** analyze/fix the executed 0F2A
   full-byte versus nibble-merge mismatch before any 0F28 implementation.
7. **New evidence-only target-authority task — blocked by underdetermined
   evidence:** acquire uPD9002/PC-88VA primary evidence or hardware evidence
   for 0F28.
8. **M62b2 — blocked by underdetermined evidence:** keep 0F28 conditional on
   the preceding evidence task. Do not treat the V20 SST expected state as
   proof of uPD9002 support.
9. **M62c — optional:** retain its provisional scope unless later evidence
   establishes a dependency.
10. **M63 — recommended by evidence:** split count normalization/destination
    behavior from FLAGS refinement; place FLAGS-sensitive work after M60a.
11. **M64 — optional:** retain DIV/IDIV ordering unless a later gate proves a
    dependency.
12. **M65 — recommended by evidence:** split BOUND residual range decisions
    from FF /7 state behavior; place BOUND residual work after M60b and keep
    FF /7 independent.

The canonical lettered-milestone scheme accepted by M58 remains in use. This
report proposes no integer renumbering and does not mix naming schemes.

## Determinism, gzip boundary, and negative tests

The generator uses sorted-key compact UTF-8 JSON with LF and
`raw-deflate-level9-mtime0-os255` gzip containers. Generation was run twice
in the recorded environment, once into
`/tmp/vaeg-m59-evidence-regen-canonical.hs8Zyl/g59` and once into the tracked
pack.

```text
diff -qr /tmp/vaeg-m59-evidence-regen-canonical.hs8Zyl/g59 \
  tests/ssts/evidence/g59
```

exited zero with no output. Both manifests had SHA-256
`acfe153ff686ed9e4a1b093c954e36bab8c306d6dbb03fc30a0a5096e981a1a0`.
The tracked-path artifact-list digest was
`cfc65213add59789b32198c85e3e3bdc6e1512e08d5b13299b18668d90a379ad`.

The exact bounded reproducibility claim is: repeated generation with Python
3.14.4 and zlib 1.3.1 in the recorded host environment is byte-identical,
including gzip bytes. As accepted at G58, this is not a universal claim that
all zlib implementations emit identical raw-DEFLATE bytes. M59 records the
environment and does not alter approved G58 evidence to revisit that
limitation.

```text
python3 tools/qa/upd9002_semantics_evidence.py selftest
```

exited zero: 33/33 positive and fail-closed checks passed. Rejections cover:

- unknown schema versions, malformed/missing/duplicate hashes, invalid
  conclusion status, and invalid classification;
- selected/executed inconsistency and expected-only evidence;
- row/manifest count mismatch and artifact digest mismatch;
- nondeterministic row order, JSON serialization, and gzip;
- missing or invalid representative evidence;
- overlapping or incomplete exclusive partitions;
- selected/applicable/classification/contract drift; and
- mutation of approved G58 or immutable G43/M43 evidence.

The existing M58 ratchet selftest passed 43 checks. The strict milestone
identifier test passed 48 checks and discovered 63 tasks, 34 final reports,
and 59 ROADMAP rows. Encoding, EOL, and path-case checks exited zero with zero
findings. After the tracked pack existed, CTest also passed the ratchet
static, evidence selftest, and evidence static tests 3/3 in 25.82 seconds.

## Semantic and protected-evidence no-change proof

The production tree object is identical at G58 and the M59 implementation:

```text
git rev-parse bc8a55c6da1082b85b794068e0d933e31fe46b13:cpu/upd9002
git rev-parse 7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7:cpu/upd9002

5daf52f3a7748c7b2ef2eabcfacc84ea83443e78
5daf52f3a7748c7b2ef2eabcfacc84ea83443e78
```

These commands exited zero with no diff:

```text
git diff --exit-code \
  bc8a55c6da1082b85b794068e0d933e31fe46b13...7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7 \
  -- cpu/upd9002/

git diff --exit-code \
  bc8a55c6da1082b85b794068e0d933e31fe46b13...7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7 \
  -- tests/ssts/baseline tests/ssts/scoreboard tests/ssts/transitions \
     tests/ssts/epochs/g43 tests/ssts/gap_taxonomy.json \
     tests/ssts/hardware_pending.json \
     tests/ssts/approved_target_divergences.json \
     tests/ssts/v20_dataset_manifest.json \
     tools/qa/golden/upd9002_support_map_m48.csv \
     tests/ssts/contracts
```

Thus M59 changes no active CPU semantics, SST fixture semantics, comparison
contract, top-level classification, taxonomy coverage, selected/applicable
set, approved G58 scoreboard/shard/transition, or immutable G43/M43 artifact.

## Commands and results

The approved baseline and implementation profiles used:

```text
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m59-impl-build.Zdv61g/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /tmp/vaeg-m59-final-arch-full.kAvmF9/v20_native_full.json \
  --failure-directory /tmp/vaeg-m59-final-arch-full.kAvmF9/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m59-impl-build.Zdv61g/sdl2/vaeg \
  --profile full --flags-comparison all16 --shard-timeout 300 \
  --output /tmp/vaeg-m59-final-fingerprint-full.SEPvRs/v20_fingerprint_full.json \
  --failure-directory /tmp/vaeg-m59-final-fingerprint-full.SEPvRs/v20_fingerprint_full_failures
```

Both exited zero. Architectural CI ran through:

```text
ctest --test-dir /tmp/vaeg-m59-impl-build.Zdv61g \
  --output-on-failure -j2
```

The fresh native build completed and CTest passed 41/41 in 515.72 seconds,
including the verified external architectural CI profile. No required
profile was skipped.

After fixing the final analysis implementation SHA, the build was
reconfigured so the CTest command carried
`--evaluated-sha 7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7`.
The external architectural CI test then passed 1/1 in 486.01 seconds.

The evidence generation command, executed twice, was:

```text
python3 tools/qa/upd9002_semantics_evidence.py generate \
  --root . \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --worker /tmp/vaeg-m59-impl-build.Zdv61g/sdl2/vaeg \
  --analysis-evaluated-sha 7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7 \
  --output tests/ssts/evidence/g59
```

The final validation commands all exited zero:

```text
python3 tools/qa/upd9002_semantics_evidence.py selftest
python3 tools/qa/upd9002_semantics_evidence.py verify-static --root .
python3 tools/qa/upd9002_ssts_ratchet.py selftest
python3 tools/qa/upd9002_ssts_ratchet.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Hosted CI

The first implementation commit passed the complete matrix:

- Run: [build 30075805598](https://github.com/nakatamaho/vaeg/actions/runs/30075805598)
- Head SHA: `726039882b4c986b7009c0e570517770884fd876`
- Result: success; all eight jobs passed

The final analysis implementation commit was then pushed before canonical
evidence generation:

- Run: [build 30076602593](https://github.com/nakatamaho/vaeg/actions/runs/30076602593)
- Head SHA: `7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7`
- Result: `success`; all eight jobs passed
- Required job: `uPD9002 architectural SST ratchet`
- Verified corpus: available and manifest-verified
- Required architectural CI profile: not skipped

The final evidence-only push triggers the same branch matrix at the final
candidate. Its run identifier and result are supplied in the maintainer
handoff because that run cannot exist before the commit containing this
report is created and pushed.

## Final repository checks

Immediately before the final evidence commit and again after it was pushed,
the following checks were used:

```text
git status --short
git diff --check
git rev-parse HEAD
git rev-parse '@{u}'
git ls-remote origin refs/heads/topic/m59-upd9002-evidence-pack
```

The final tracked worktree is clean. Local HEAD, upstream, and the remote
branch have the same final candidate SHA supplied in the maintainer handoff.
The final commit contains only the generated evidence pack and this report.

## Known limitations

- This is V20 SST differential evidence, not complete uPD9002 silicon
  validation.
- The architectural baseline still has 10,593 CI and 84,329 full failures.
  M59 explains selected populations but fixes none.
- Fingerprint full still has 186,767 diagnostic failures and is not an
  architectural gate.
- Final RAM proves final bytes, not transient write order, rollback, or bus
  behavior.
- 0F28 target support remains underdetermined without acquired uPD9002 or
  PC-88VA primary documentation or hardware evidence.
- Several shared-primitive statements are hypotheses or underdetermined; the
  report does not promote them to proven root causes.
- The accepted gzip limitation bounds raw-byte reproduction to the recorded
  canonical environment.

## Human verification

From a clean checkout of the analysis implementation commit, configure a
tests-enabled worker with the verified corpus, run the three exact profile
commands above, and then:

```text
python3 tools/qa/upd9002_semantics_evidence.py generate \
  --root . \
  --dataset-root /path/to/verified/singlesteptests-v20 \
  --worker /path/to/tests-enabled/vaeg \
  --analysis-evaluated-sha 7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7 \
  --output /tmp/g59-a

python3 tools/qa/upd9002_semantics_evidence.py generate \
  --root . \
  --dataset-root /path/to/verified/singlesteptests-v20 \
  --worker /path/to/tests-enabled/vaeg \
  --analysis-evaluated-sha 7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7 \
  --output /tmp/g59-b

diff -qr /tmp/g59-a /tmp/g59-b
python3 tools/qa/upd9002_semantics_evidence.py verify-static --root .
git diff bc8a55c6da1082b85b794068e0d933e31fe46b13...HEAD \
  -- cpu/upd9002/
git diff --check
git status --short
git rev-parse HEAD
git rev-parse '@{u}'
```

The two generated trees must be byte-identical in the recorded environment,
the static verifier must pass, the CPU diff must be empty, and the tracked
worktree must be clean.

M59 is complete and pushed. G59 is an unapproved candidate pending human
review. No production CPU semantics were changed. M60 and later milestones
have not been started.
