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
# M60e uPD9002 IRET restoration semantics

M60e corrects the executed SST-observed real-mode `CF IRET` FLAGS restoration
rule. The complete 5,000-case population proves that stack reads, IP, CS, SP,
termination, and boundary mapping were already correct. The residual was
limited to reserved FLAGS bits 3 and 5, which the old path loaded from the
stack instead of forcing to zero.

M60e is complete and pushed. G60e is an unapproved candidate pending human
review. M61 and later milestones have not been started.

A commit cannot contain its own SHA. The evidence commit and final candidate
are therefore the commit containing this report; the exact SHA is supplied in
the maintainer handoff and is independently available from
`origin/topic/m60e-upd9002-iret`.

## Identity and preparation

- Approved predecessor gate: `G60d`
- Exact approved G60d SHA and M60e base:
  `8736f8afe6d8eeb58e58c7afdaf5951e2306cb63`
- G60d audit implementation/evaluated SHA:
  `ada55de79751c04e44d02abf7ecd6851b55c9763`
- Approved G60d CI:
  [build 30155594048](https://github.com/nakatamaho/vaeg/actions/runs/30155594048)
- G60d outcome: `evidence_only_closure`
- Initial primary-worktree branch: `main`
- Initial primary-worktree SHA:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty with unrelated maintainer work
- Dedicated worktree: `/home/maho/vaeg/build/m60e-worktree`
- Dedicated worktree starting SHA:
  `8736f8afe6d8eeb58e58c7afdaf5951e2306cb63`
- M60e branch: `topic/m60e-upd9002-iret`
- Initial audit implementation:
  `e13e115ea0d9cd04d27057af71d7cd805c2ceb5e`
- Predecessor-validation isolation:
  `6372362d3e61ed043e516fa9c188e185457f9140`
- Final pre-semantic audit contract:
  `74601393bd77f254bce8f70bad10d1fd847411c5`
- Semantic implementation and `evaluated_sha`:
  `7f815acb26f1be546bbcfd5de12972235dfd175c`
- Candidate-epoch scoreboard validation:
  `7eeee48585162bf60937d356f03c94a9b3f074b0`
- Bug-fix ledger commit:
  `e6e82e22e5938edbc90317260c8e7c3558184fa6`
- Profile-scoped transition validation:
  `6570605a935ccd192bf6e53bb0dae2b932786b4d`
- Historical-artifact/current-semantics validation isolation:
  `c591836d0abf0257c69888aa60b1ded64be3e466`
- G60e-specific hosted CI enforcement:
  `8675e768b9149f4abfb6d8b98115c8c023b41298`
- Evidence commit/final candidate: the commit containing this report

The primary worktree was not cleaned, reset, stashed, staged, or modified.
The dedicated worktree was created directly from the fixed predecessor. The
maintainer authorization, local object, authoritative G60d report, and
`origin/topic/m60d-upd9002-interrupt-frame` all resolve to the approved SHA.
No conflicting approved G60d SHA was found.

The preparation commands exited zero:

```text
git status --short
git branch --show-current
git rev-parse HEAD
git rev-parse 8736f8afe6d8eeb58e58c7afdaf5951e2306cb63
git show --stat --oneline 8736f8afe6d8eeb58e58c7afdaf5951e2306cb63
git rev-parse origin/topic/m60d-upd9002-interrupt-frame
python3 tools/qa/milestone_ids.py --selftest --audit --discover
```

ROADMAP, milestone discovery, and the canonical M60e task agree on the
identifier, branch, G60d prerequisite, report path, and CF-only scope.

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

Dataset, contracts, target policy, selected/applicable sets, top-level
classifications, gap taxonomy, registries, and fixtures did not change.

## Approved G60d reproduction

Before production editing, a fresh tests-enabled build at the exact approved
G60d SHA ran all three profiles without skip:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,179 | 8,121 | 0 | 0 |
| architectural full | 1,562,502 | 1,438,594 | 1,378,653 | 59,941 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,438,594 | 1,276,215 | 162,379 | 0 | 0 |

Counts were not identity substitutes. The approved pass, failure, signature,
termination, selected, and applicable identities all matched:

| Profile | Pass SHA-256 | Failure SHA-256 | Signature SHA-256 |
|---|---|---|---|
| architectural CI | `ae83f89ed1bfe34088197012683c726d9dab5a76700bb247efcf9a7e6e0d8bff` | `04ffb31baf4f66e60f0e700ebe8713ad7b1d582352160dc02c61415619426603` | `a67c9135780ce1df35486b2c2a28fe3e309be97c30045b4761ed5a8c652a4132` |
| architectural full | `898b7e6e66fa9ea4e475bfc78db5a0cf8c3d6b109276fac6db0f68e05a1c27f2` | `9c69f518d168e1675e4d1fcf59db031dc1bce058db3fdfe9f6cb66b8d2bec91d` | `776c17c19e52cc03becc83464dfcdcb0b1590532812deac7e93c8955f0c1a473` |
| fingerprint full | `691242bb0a05324fe3be261653863db45f45fd495c83d6798d8491a3e8db42db` | `2d825411e958ec980317d0c29d06aee8f9e7337c036f1d5703014311e53c2cc4` | `84c40271c91cb8d01e3545ada60c50e35594e790ce29e9b4d609c75e1d59d3bb` |

M58 ratchet, M59 evidence, M60a artifacts, M60b authority/policy, M60c
erratum/authority, and M60d frame-audit verification passed. Protected
G43/M43 and G58-G60d artifacts remained byte-identical.

## Pre-edit CF audit and case table

`CF` is top-level `applicable`; all 5,000 selected records were applicable and
executed. The exact pre-fix result was:

| Result | Count | Hash-set SHA-256 |
|---|---:|---|
| pass | 1,231 | `838aaf1c7de3973f705e77d5fb2ac8afa820d0a488df8cb6bc8192dd4881720e` |
| fail | 3,769 | `181b01975f5cb4ad42e6556b4473eed060e06b1afad04e9265f51cd00cbcdae5` |

The historical 3,769 cross-check was therefore exact. The complete
machine-readable side-by-side table contains 5,000 rows at
`tests/ssts/evidence/g60e/iret_cases.json.gz`; its SHA-256 is
`b7a29713caf75f8820b9a6ca4224fefed1ad83f8b6d8c46461ac4a3a07c56ba3`.
Every row records instruction and prefix bytes, initial state, all six stack
bytes, logical and physical addresses, expected/actual IP, CS, FLAGS, SP,
termination, RAM, unrelated registers, mismatch classes, partitions,
conclusion status, and notes.

## Stack, control-flow, and termination audit

The complete population independently confirms this observed word order:

```text
word 0: restored IP
word 1: restored CS
word 2: restored FLAGS
```

Each word is reconstructed little-endian from the observed stack bytes.
Logical offsets wrap at 16 bits, physical addresses wrap at 20 bits, and final
SP is `(initial SP + 6) & 0xffff`.

| Boundary partition | Count | Result after correction |
|---|---:|---|
| ordinary | 4,844 | 4,844 pass |
| 20-bit physical wrap | 156 | 156 pass |
| 16-bit offset wrap | 0 | explicit zero from corpus metadata |
| both wraps | 0 | explicit zero from corpus metadata |

The boundary summary SHA-256 is
`e6fbf0f8b0ce0cea1fcad7bf52e3c8859e625e94f16bf84a10d84d9568752c96`.
Expected and actual stack addresses, byte order, restored IP, restored CS,
final CS:IP, final SP, termination, RAM, and unrelated registers matched
before the semantic correction. No transient read ordering is inferred.

## Independently derived IRET FLAGS rules

The complete CF population was analyzed independently of POPF. The rule-table
digest is:

```text
8dcf0cdaea2e1081ca3f02b9f46af8aec33286499d2b911a40555127f1af3af5
```

| Bit | Expected rule | Pre-fix actual | Post-fix actual |
|---:|---|---|---|
| 0 | loadable | loadable | loadable |
| 1 | forced-one | forced-one | forced-one |
| 2 | loadable | loadable | loadable |
| 3 | forced-zero | loadable | forced-zero |
| 4 | loadable | loadable | loadable |
| 5 | forced-zero | loadable | forced-zero |
| 6 | loadable | loadable | loadable |
| 7 | loadable | loadable | loadable |
| 8 | undetermined | undetermined | undetermined |
| 9 | loadable | loadable | loadable |
| 10 | loadable | loadable | loadable |
| 11 | loadable | loadable | loadable |
| 12 | forced-one | forced-one | forced-one |
| 13 | forced-one | forced-one | forced-one |
| 14 | forced-one | forced-one | forced-one |
| 15 | forced-one | forced-one | forced-one |

Bits 3 and 5 are the only proven defect. Bit 8 is underdetermined because the
executed expected/stack observations do not provide both states; it is
deliberately unchanged. Guest-visible normalized FLAGS, internal FLAGS,
metadata-masked architectural comparison, and the full 16-bit fingerprint are
kept separate.

The derived observable rule matches the approved POPF rule for every
determined bit. That conclusion follows the independent IRET analysis; POPF
was not used as the IRET oracle, and the implementation retains a separate
IRET contract.

## Root cause and semantic correction

The pre-change dispatch tables route normal, REPE, and REPNE `CF` to
`v30_iret`. The handler uses `REGPOP0` for IP, CS, and FLAGS, updates
`CS_BASE`, splits overflow from the stored FLAGS word, restores trap state,
and uses the existing IRQ/trap termination path.

The old IRET-specific mask was:

```c
flag = (flag & 0x0fff) | 0xf002;
```

It made stack bits 3 and 5 loadable. The only final production change is:

```c
flag = (flag & 0x0fd7) | 0xf002;
```

This clears exactly bits 3 and 5. The following split-overflow assignment,
internal high-FLAGS representation, bit 8, stack helpers, CS/IP assignment,
SP update, and termination code are unchanged.

An earlier pre-evidence implementation commit used the POPF mask and changed
the internal high-FLAGS representation. Although it made CF green, the audit
rejected that scope expansion because bit 8 was underdetermined. No final
profile or G60e artifact was retained from that superseded implementation.
The evaluated commit is the narrower one above.

The semantic diff against G60d is exactly one production line:

```text
git diff \
  8736f8afe6d8eeb58e58c7afdaf5951e2306cb63...7f815acb26f1be546bbcfd5de12972235dfd175c \
  -- cpu/upd9002/
```

No interrupt-entry, saved-FLAGS, PUSHF, POPF, SAHF, LAHF, RETF, BOUND range,
DIV/IDIV arithmetic, decoder, fixture, comparison contract, target policy,
classification, taxonomy, or registry source changed.

## Focused and protected results

The focused test covers ordinary and physical-wrap reads, an explicit
offset-wrap setup, distinct IP/CS values, little-endian reconstruction,
final-SP wrapping, unrelated-register preservation, and explicit per-bit
FLAGS cases. It passed after the correction.

| Form/population | Result |
|---|---:|
| `9C PUSHF` | 4,999 pass / 1 fail |
| `9D POPF` | 5,000 pass / 0 fail |
| `9E SAHF` | 5,000 pass / 0 fail |
| `9F LAHF` | 5,000 pass / 0 fail |
| `CC INT3` | 5,000 pass / 0 fail |
| `CD INT imm8` | 5,000 pass / 0 fail |
| `CE INTO` | 5,000 pass / 0 fail |
| `62 BOUND` | 3,756 pass / 1,244 fail |
| `CF IRET` | 5,000 pass / 0 fail |

The 12,468 synchronous-entry hashes remain frame-correct:
`4498c8aa838f93aba7220f0cdacff34341d704a9cbe7f6d35d79b75219b41d0b`.
The G60d residual-frame set remains empty:
`4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`.
The exact 214-hash divide-exception dependency set remains green:
`5cda7079da30b8266de2df3b55b90b3e5ee12a20429e670d1f128b924903c719`.

## Candidate profiles and ratchet

All candidate profiles used the exact evaluated worker:

```text
worker SHA-256:
0718d96c616bc46406b5afea09b14b7774147bfcf635b2e8873d3865cdb7ffa6
```

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,561 | 7,739 | 0 | 0 |
| architectural full | 1,562,502 | 1,438,594 | 1,382,422 | 56,172 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,438,594 | 1,279,984 | 158,610 | 0 | 0 |

The evidence-derived reference of 56,172 architectural full failures was
reached exactly because all 3,769 pre-fix CF failures passed and no other
architectural hash changed.

| Profile | Pass SHA-256 | Failure SHA-256 | Signature SHA-256 |
|---|---|---|---|
| architectural CI | `dc6bdee9f856ca6102748ca442ac579adf8a7f05e01e02564766148a35825cdc` | `2ae38099d67c240ff5bf48a1c7643d1b6d6480e4e27d1b8967d87508d751ebd6` | `af14392ce957dfeaf770da595551fef8767bc7412eec06c10badbe9d7c8930b4` |
| architectural full | `11958b52c4fa71e1ac38c22d7e305562ab00f408c453fa423955bcc3eb6882c4` | `2c2bae091f33ebcd334767d9a9597eab5707d45a4d66b5433b8b37b10ce367f7` | `4fc2d3603ec05633f4a4b63f574d92bb5b26140519f03e5e50c848d5066dd84b` |
| fingerprint full | `17a6bc59e91efc7439621037842072c3ae0d0bf2f600307ae3ef407e1dafc542` | `795fdeb7c0469783f4863aeebf45c730118c7cccfede5b7804d5a55f7e1ae2cb` | `0c184c75164afe40cb5afddaa0aab635c24b131cf8925f5df9163c89d6e3d377` |

Architectural CI has 382 newly passing CF hashes, digest
`373793ae0b7fb4ef57a0a6644b9041c0d5b90f16cc826d8112575bae760cff0d`.
Architectural full has exactly the 3,769 pre-fix CF failures newly passing,
digest `181b01975f5cb4ad42e6556b4473eed060e06b1afad04e9265f51cd00cbcdae5`.
The newly failing set is empty, digest
`4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`.
Changed failure count is zero. There is no per-form decrease, timeout, crash,
classification, taxonomy, or registry change.

## Fresh target-correct failure ranking

The complete 333-row ranking is
`tests/ssts/rankings/g60e_architectural_full.json`, SHA-256
`662ec6a3645833dac25ba4ad6e7c3250d47c600dbaf84066971a70f18cd8484e`.
The human-readable top 30 is
`tests/ssts/rankings/g60e_architectural_full.md`, SHA-256
`188e840699ecae0f2931ee91a1e457c4ba7f3fb1f626114fa65f4bfbb8544ee8`.
It reconciles exactly to 56,172 failures. Its leading forms are:

| Rank | Form | Fail | Cumulative share |
|---:|---|---:|---:|
| 1 | `FF.7` | 5,000 | 8.90% |
| 2 | `D4` | 4,803 | 17.45% |
| 3 | `3F` | 4,716 | 25.85% |
| 4 | `0F2A` | 4,692 | 34.20% |
| 5 | `F7.7` | 3,723 | 40.83% |
| 6 | `F6.7` | 3,716 | 47.44% |
| 7 | `F6.6` | 2,561 | 52.00% |
| 8 | `F7.6` | 2,486 | 56.43% |

CF is explicitly present as 5,000 pass / 0 fail in the machine table. Omission
from the human top 30 is not treated as proof of passing.

## Evidence identities and determinism

| Artifact | Path | SHA-256 |
|---|---|---|
| evidence manifest | `tests/ssts/evidence/g60e/manifest.json` | `27909e9305d2bc49e491f5a4f81285433840a6a0d5397f82e741a6e4b10c44ae` |
| artifact tree | manifest field | `199438d8d70fccba15ed61a4478ec5a9b3a68fa77824c77346d44854a1d3bdef` |
| CI transition | `tests/ssts/transitions/g60e_architectural_ci_from_g60d.json` | `0781ed4768ea05db3af48c8d4f1ae33afb59d5855b634c3ee6c52758e8501ad7` |
| full transition | `tests/ssts/transitions/g60e_architectural_full_from_g60d.json` | `730c0788702a2d52ccaa3b9973622c92a0b63558ff52d05f8ce30b2fe0e76b8c` |

Two complete generations in the recorded environment produced identical path,
byte-count, and SHA-256 tuples. The canonical JSON writer fixes key and row
ordering; the deterministic gzip writer fixes gzip metadata.

The first generation exposed that the shared G60b scoreboard validator still
configured its own historical epoch during a forward G60e call. The generator
now scopes and restores the M60e candidate identity explicitly. This was an
artifact-validator defect, not a CPU or SST result defect. It was committed
separately after the semantic evaluated SHA, and the same preserved raw SST
results were reused under the repository-wide identity-bound reuse rule.

Final static verification then exposed a separate scope error: it required the
full-profile 3,769 CF transition count when validating the CI transition,
whose exact count is 382. Commit
`6570605a935ccd192bf6e53bb0dae2b932786b4d` makes that check use each
transition's independently resolved `cf_failure_count_before` and adds a
fail-closed regression test. This changes only post-generation validation.
The evidence remains bound to the generator SHA-256
`8b0ab6a322f2e89b4f4dddf18d911fd3e7db036f39fc0d0018a0794303f797e8`,
which had already generated the complete family twice with identical bytes;
the expensive source profiles and deterministic-generation proof were not
repeated for a verifier-only change.

The historical M60b-M60d static validators originally combined two concerns:
immutable historical evidence and the CPU tree as it existed at their own
gate. A legitimate M60e semantic change therefore caused the surrounding
historical checks to fail even though their evidence was unchanged. Commit
`c591836d0abf0257c69888aa60b1ded64be3e466` adds an explicit
`--protected-evidence-only` mode for forward-gate CTest use. The old validators
continue to check their artifacts, policies, scoreboards, and transitions;
the M60e validator separately and fail-closed verifies that the current CPU
diff is exactly the one authorized IRET line. The older protected-deletion
test similarly canonicalizes exactly that one M60e line before checking its
frozen digest. No unrelated CPU change is accepted by the current gate.

The former external CTest entry enforced the historical G60b behavior and
could not accept a later semantic improvement. Commit
`8675e768b9149f4abfb6d8b98115c8c023b41298` replaces that call with a
G60e-specific hosted check. It runs architectural CI once under the unchanged
G60b target policy, then compares counts, pass/failure sets, signatures,
termination classes, mismatch classes, per-form records, selected/applicable
identities, and canonical failure-sidecar identities with the committed G60e
scoreboard. A local reuse-mode check against the preserved CI raw result
matched exactly: 180,000 selected, 165,300 executed, 157,561 pass, and 7,739
fail, with zero timeout and crash.

The gzip reproducibility claim is bounded to Python 3.14.4, its recorded gzip
module, and zlib 1.3.1. No universal cross-zlib byte-identity claim is made.

## Validation and CI

The M60e selftest passed 34 fail-closed mutations at their intended reason
codes. Surrounding checks could not mask the targeted rejection. Coverage
includes predecessor, policy, dataset, contract, selected/applicable,
classification/taxonomy/registry, CF coverage, selector structure,
expected-only evidence, stack/byte order, SP and boundary mapping, independent
FLAGS rules, domain separation, prohibited entry/POPF/BOUND/DIV changes,
protected evidence, newly passing/failing ownership, ranking reconciliation,
deterministic JSON/gzip, and final evidence-only scope.

All native tests, focused M60e tests, M58-M60d verifiers, milestone discovery,
documentation, encoding, EOL, path-case, and diff checks pass locally. No
required SST profile was skipped.

Hosted CI is started only after the evidence-only commit containing this
report is pushed. Its GitHub-assigned run URL and conclusion are supplied in
the maintainer handoff rather than self-referenced here. Hosted CI is not used
as an iterative debugger.

Principal commands, all with exit status zero:

```text
cmake --preset linux-ci-gcc -B build/linux-ci-gcc \
  -DVAEG_SSTS_V20_ROOT=<verified-corpus>
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope ci --profile architectural --policy g60b \
  --output <raw-ci> --failure-directory <raw-ci-failures>

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope full --profile architectural --policy g60b \
  --output <raw-full> --failure-directory <raw-full-failures>

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope full --profile fingerprint --policy g60b \
  --output <raw-fingerprint> \
  --failure-directory <raw-fingerprint-failures>

python3 tools/qa/upd9002_m60e_iret.py selftest
python3 tools/qa/upd9002_m60e_iret.py regenerate-twice \
  --root . --dataset-root <verified-corpus> \
  --pre-fix-audit <pre-fix-audit> \
  --post-fix-audit <post-fix-audit> \
  --architectural-ci-raw <raw-ci> \
  --architectural-full-raw <raw-full> \
  --fingerprint-full-raw <raw-fingerprint> \
  --evaluated-sha 7f815acb26f1be546bbcfd5de12972235dfd175c \
  --output-root .
python3 tools/qa/upd9002_m60e_iret.py verify-static
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

## Reuse decisions

The fresh G60d baseline profiles were run before semantic editing. After the
evaluated SHA was fixed, all three candidate profiles ran once against worker
SHA-256 `0718d96c...ffa6`. Their raw outputs and sidecars are preserved under
`/home/maho/vaeg/build/m60e-results/final-7f815ac/`.

Subsequent changes affect only candidate-epoch validation, the bug-fix ledger,
generated evidence, and this report. They do not affect the worker, corpus,
dataset, contracts, target policy, selected/applicable sets, or evaluated
semantic commit. The completed SST outputs were therefore reused by exact
identity rather than rerun. No gate was weakened and no affected result was
reused.

## Human verification

From a clean checkout of the candidate:

```text
python3 tools/qa/upd9002_m60e_iret.py selftest
python3 tools/qa/upd9002_m60e_iret.py verify-static
python3 tools/qa/upd9002_m60d_frame_audit.py verify-static --root .
python3 tools/qa/upd9002_m60c_erratum.py verify-static --root .
python3 tools/qa/upd9002_m60c_audit.py verify-static --root .
python3 tools/qa/upd9002_m60b_authority.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
cmake --preset linux-ci-gcc
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure
git diff \
  8736f8afe6d8eeb58e58c7afdaf5951e2306cb63...7f815acb26f1be546bbcfd5de12972235dfd175c \
  -- cpu/upd9002/
git diff --check
```

## Known limitations

- This is complete evidence for the selected executed V20 SST population under
  the fixed contracts and target policy, not complete uPD9002 silicon
  validation.
- The corpus contains no 16-bit-offset-wrap CF case; that partition is an
  explicit evidence zero. The focused regression setup exercises the helper
  boundary without claiming absent SST coverage.
- IRET bit 8 is underdetermined by this population and remains unchanged.
- M60e does not implement protected-mode IRET, change interrupt entry, or
  address any remaining BOUND, DIV/IDIV, shift, BCD, FF `/7`, or 0F-family
  semantic failures.
- The one existing PUSHF boundary-write anomaly remains outside M60e.
