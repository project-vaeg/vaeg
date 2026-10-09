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
# M58 uPD9002 SST hash ratchet and scoreboard epoch

M58 adds immutable M43 references, separate architectural and fingerprint
profiles, canonical G58 scoreboards, a hash-level G57-to-G58 ratchet,
classification-transition governance, gap taxonomy, strict lettered-milestone
tooling, and fail-closed validation. It makes no uPD9002 semantic change.

M58 is complete and pushed. G58 is an unapproved candidate pending human
review. M59 has not been started.

The SHA of a Git commit cannot be embedded in content that participates in
that same SHA. Consequently, the evidence commit and final candidate are the
single commit containing this report; their exact SHA is supplied in the
maintainer handoff and is independently available as the peeled value of
`origin/topic/m58-upd9002-ssts-ratchet`. This avoids a self-referential
artifact. The implementation commit evaluated by every G58 artifact is fixed
and is recorded below.

## Identity, predecessor, and repository state

- Approved predecessor gate: `G57`
- Exact approved G57 SHA and M58 base:
  `72322d5c9b8e40e4a988312aebe163a8190e2aa5`
- Initial primary-worktree branch: `main`
- Initial primary-worktree HEAD:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree status: non-empty (23 short-status records)
- Dedicated M58 worktree: `/tmp/vaeg-m58.WJM6hJ`
- Starting SHA in the dedicated worktree:
  `72322d5c9b8e40e4a988312aebe163a8190e2aa5`
- Final branch: `topic/m58-upd9002-ssts-ratchet`
- Implementation commit and `evaluated_sha`:
  `d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53`
- Evidence commit: the commit containing this report; exact SHA in the
  maintainer handoff and remote branch ref
- Final candidate SHA: the same evidence commit; exact SHA in the maintainer
  handoff and remote branch ref

The primary worktree's unrelated tracked and untracked files were not reset,
cleaned, stashed, staged, or modified. The M58 worktree was created clean and
directly from the fixed G57 SHA, then the required topic branch was created
there.

The predecessor was not inferred from HEAD, a branch tip, a mutable tag, or
the default branch. It was established by all of the following:

1. The maintainer supplied the formal G57 approval and exact SHA.
2. The authoritative M57 report is introduced by commit
   `72322d5c9b8e40e4a988312aebe163a8190e2aa5`; this was verified with
   `git log -1 -- docs/agents/reports/m57_remove_frozen_reference_tier.md`.
3. `origin/topic/m57-remove-frozen-tier` resolves to the same SHA.
4. `git rev-parse 72322d5c9b8e40e4a988312aebe163a8190e2aa5`
   returned that exact SHA.

The preparation commands were:

```text
git status --short
git branch --show-current
git rev-parse HEAD
git rev-parse 72322d5c9b8e40e4a988312aebe163a8190e2aa5
git rev-parse origin/topic/m57-remove-frozen-tier
```

The relevant tool versions were:

| Tool | Version |
|---|---|
| Git | 2.53.0 |
| CMake | 4.2.3 |
| Ninja | 1.13.2 |
| Python | 3.14.4 |
| GCC | 15.2.0 |
| Clang | 21.1.8 |
| MinGW-w64 GCC | 13-win32 |
| Wine | 10.0 |

## Commit and file separation

The implementation chain ends at
`d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53`:

- `f37a2243fcfe53d73a812aa377d48f3d048d81da`,
  `M58: add immutable uPD9002 SST ratchet`, contains the ratchet, profiles,
  contracts, schemas, validators, taxonomy, registries, immutable G43
  references, tests, CI integration, and directly supporting documentation.
- `d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53`,
  `M58: make gzip verification portable`, fixes validation of canonical gzip
  containers across zlib implementations. It was required after the first
  evidence-only CI run exposed a Windows/Linux raw-DEFLATE difference. The
  generator now controls every gzip header and trailer field, while the
  verifier checks one member, canonical payload, CRC-32, size, and the
  committed raw digest without requiring distinct zlib versions to emit the
  same valid DEFLATE stream.

The later evidence-only commit contains:

- 3 canonical scoreboard summaries;
- 29 deterministic gzip failure shards;
- 2 canonical architectural transition artifacts; and
- this report.

It contains no implementation change and no semantic source change.

## No semantic change

The implementation change list contains only CI/build integration,
documentation, SST contracts/data registries, and Python validation tooling.
No emulator source file changed. In particular:

```text
git diff --exit-code \
  72322d5c9b8e40e4a988312aebe163a8190e2aa5 \
  d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53 \
  -- cpu/upd9002 iova/upd9002_regs.c iova/upd9002_regs.h
```

exited zero. The `cpu/upd9002/` tree object is identical at both commits:

```text
5daf52f3a7748c7b2ef2eabcfacc84ea83443e78
```

The exact architectural CI/full identities and failure signatures below also
prove that the active implementation's behavior is unchanged from G57.

## Verified corpus and immutable M43 evidence

The verified 360-file corpus was available for every required run:

```text
/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
```

Its dataset ID is:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

`tests/ssts/epochs/g43/manifest.json` provides content-addressed references
for all 23 immutable M43 files. It does not modify, rename, recompress, or
re-schema any M43 file. `git diff --exit-code G57 -- tests/ssts/baseline`
exited zero, and the static verifier rejects a one-byte mutation of any
referenced file.

| Immutable evidence | SHA-256 |
|---|---|
| G43 manifest file | `77dd1e53f325f3910bd727d3dec4b9c1e23c005f0b306c085a34569e9cf5b23f` |
| M43 CI summary | `a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31` |
| M43 CI failure index | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` |
| M43 CI canonical sidecar set | `f1a940e4029fc796135f3e23a313b1feae0a0496ac14e76da4e8be271058e919` |
| M43 CI raw sidecar set | `1d5a54db35ad2a63855e7719873a24cb5aff4ab51f49f492dd4e8e00d95efb0e` |
| M43 full summary | `dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9` |
| M43 full failure index | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |
| M43 full canonical sidecar set | `1f1b61bb8fa4d952a1e5e98127e849179773b557211b1dede6aff72c289b4b59` |
| M43 full raw sidecar set | `6c318aab017b10f2ddf7090d2b5893951844ce9662eae33adf1116d9ebfea87b` |
| M43 known-gap JSON | `11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1` |
| M43 CI-to-full transition | `95559fa2a42a80710e850a9308202780a6fd4dad42ae20644c308bd0a72be092` |

## G57 baseline reproduction

A fresh tests-enabled GCC build was configured at exact G57 in
`/tmp/vaeg-m58-g57-build.Eh5Clt`. Both required architectural profiles ran
without skip before the first M58 edit:

| Scope | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| CI | 180,000 | 166,821 | 156,228 | 10,593 | 0 | 0 |
| full | 1,562,502 | 1,443,876 | 1,359,547 | 84,329 | 0 | 0 |

The generated summaries were byte-identical to the immutable M43 summaries.
The complete sidecar directories were byte-identical. Dataset, comparison
contract, selected/applicable/pass/failure hash sets, failure signatures,
mismatch and termination classes, classification sets, resolved hashes,
summary digests, failure indexes, and sidecar contents all matched.

The commands were:

```text
cmake --preset linux-ci-gcc -B /tmp/vaeg-m58-g57-build.Eh5Clt \
  -DVAEG_SSTS_V20_ROOT=/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build /tmp/vaeg-m58-g57-build.Eh5Clt --parallel

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m58-g57-build.Eh5Clt/sdl2/vaeg \
  --profile ci --shard-timeout 300 \
  --output /tmp/vaeg-m58-g57-ssts.d7peQ3/v20_native_ci.json \
  --failure-directory /tmp/vaeg-m58-g57-ssts.d7peQ3/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m58-g57-build.Eh5Clt/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /tmp/vaeg-m58-g57-ssts.d7peQ3/v20_native_full.json \
  --failure-directory /tmp/vaeg-m58-g57-ssts.d7peQ3/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
```

Every command exited zero.

## G58 profile contracts and results

The architectural contract compares defined final FLAGS bits, final general
and segment registers, final IP, SST-represented RAM bytes, and architectural
termination class. Cycles, prefetch, and bus timing are excluded. Its ID is
`upd9002-v20-architectural-v1` and its SHA-256 is
`aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5`.

The non-blocking fingerprint contract compares all 16 final FLAGS bits. Its
ID is `upd9002-v20-fingerprint-v1` and its SHA-256 is
`47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e`.

All three profiles ran against exact evaluated implementation commit
`d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53`, with the verified corpus
available and no profile skip:

| Profile | Scope | Blocking | Selected | Executed | Pass | Fail | Timeout | Crash |
|---|---|---|---:|---:|---:|---:|---:|---:|
| architectural | CI | yes | 180,000 | 166,821 | 156,228 | 10,593 | 0 | 0 |
| architectural | full | yes | 1,562,502 | 1,443,876 | 1,359,547 | 84,329 | 0 | 0 |
| fingerprint | full | no | 1,562,502 | 1,443,876 | 1,257,109 | 186,767 | 0 | 0 |

Architectural termination classes are CI
`normal=165546, type0=1275` and full
`normal=1431180, type0=12696`. Fingerprint termination classes are identical
to architectural full. The fingerprint result is diagnostic; it neither
weakens nor replaces the architectural gate.

The first candidate CI invocation used a non-canonical failure-directory
basename and exited 1 because `--expect` intentionally compares the embedded
sidecar path. Its normalized content was otherwise identical. That discarded
invocation was corrected by using the immutable basename
`v20_native_ci_failures`; the required CI run then exited zero and was not
skipped. Architectural full exited zero with `--expect`. Fingerprint full
exited zero without `--expect`, as it has its own all-16-bit contract.

The corrected commands were:

```text
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m58-portable-build/sdl2/vaeg \
  --profile ci --shard-timeout 300 \
  --output /tmp/vaeg-m58-portable-raw.8Qgola/v20_native_ci.json \
  --failure-directory /tmp/vaeg-m58-portable-raw.8Qgola/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m58-portable-build/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /tmp/vaeg-m58-portable-raw.8Qgola/v20_native_full.json \
  --failure-directory /tmp/vaeg-m58-portable-raw.8Qgola/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker /tmp/vaeg-m58-portable-build/sdl2/vaeg \
  --profile full --flags-comparison all16 --shard-timeout 300 \
  --output /tmp/vaeg-m58-portable-raw.8Qgola/v20_fingerprint_full.json \
  --failure-directory /tmp/vaeg-m58-portable-raw.8Qgola/v20_fingerprint_full_failures
```

## Hash-set and classification identity

| Evidence | CI SHA-256 | Full SHA-256 |
|---|---|---|
| selected hash set | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` |
| applicable hash set | `80069e9a95f29b38e8f268b806f3ad8c7cb973c11d23b6cb64450ff00fc497cc` | `7de13cbd54e709e0d0d0abefedac876306c8a67c7936f6a26c983362fed6d23c` |
| pass hash set | `053195c1ac7001fa553da6201cd9e7a6843e8de2846796ca608cfca09ab30c78` | `64e33aabb7ad4e926c329105a13bf917559e8e6ee37ebc784def8a52256fdee1` |
| failure hash set | `6129fb38d6ad32d739027819ec24e0ffe7caba26ef7e8e7e595f9cfd987cb63e` | `cd3bac7b62f0c661d1c660dfadd13cb9e8690dca6d66449c792b3bc1b06d57b4` |
| applicable classification | `80069e9a95f29b38e8f268b806f3ad8c7cb973c11d23b6cb64450ff00fc497cc` | `7de13cbd54e709e0d0d0abefedac876306c8a67c7936f6a26c983362fed6d23c` |
| known-target-gap classification | `631126aecb68585983e70f6c1d697cc75b33cd8cd5abc285a0fe08b6fd88930b` | `ba6458f9ca10f7aed7ce1f23b8601daf0d17c31460f4e479b35b983e7c9ee7c1` |
| upstream-nonblocking classification | `bb88b130688aa92d5d31047017e9c22245bb30d6424d93177722df44a071ff65` | `d16ebae3396ee095c34902d0a99d4c59162a79ce358e03d2a19affcae24d0b69` |
| empty category | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` |

The CI classification counts are applicable 166,821, known target gap 8,179,
upstream nonblocking 5,000, and zero for the other two categories. Full
counts are applicable 1,443,876, known target gap 68,626, upstream
nonblocking 50,000, and zero for the other two categories. No top-level
classification or resolved hash set changed.

## Scoreboard and transition artifacts

| Artifact | File SHA-256 | Internal scoreboard digest |
|---|---|---|
| `g58_architectural_ci.json` | `3477bccefae9bcaec12bda348dd82fc9392085ca4b833bb65054b94b906dea5c` | `ab730c242bebbc2f60d21590051d2eafe8cc19d44922bf4c3782cf486ec7604c` |
| `g58_architectural_full.json` | `4e622a7a75e03cd7f58795f9fc74414891853eb16394123bb149cc219f06f34a` | `70218bd610a52cc28971fc630ff08574959187c177b8c597e1a31c9e4ff8bdf4` |
| `g58_fingerprint_full.json` | `377413430cb3940a0ec2ca99c8745b13d8d7ce07af3ddedf8c3a02c03539d0ff` | `673525e608551d1f35ea03242d1ed19c2a7095b6417eac0cb3e4c5371fefeefd` |

| Profile | Signature-index digest | Canonical shard-set digest | Raw gzip-set digest | Shards |
|---|---|---|---|---:|
| architectural CI | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` | `fc9f53d62af20375a1527e4e8951d355260d4067653fbcbdb4b4ef39a24483ac` | `728c4d526dcea6f4d2165612d9e4aa8cf2c2125d2bf4fcd5842cd4aafdf69219` | 9 |
| architectural full | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` | `e313286af081f1be58bbf069480b86fa740b3c091e5ba7fc523f7904fd14018e` | `fb42eaf2f150c0ba33b93f9f5c1f604d255ca4c299fa0406a5abccd65649ba43` | 10 |
| fingerprint full | `b84a518fbfb034eca55f708d60a327b75ab16a872a488e63b1461fef23e408c2` | `c3758cb20cc940234a77a5493476c8f7ee09159a86639d981854859a2352f8e3` | `4874480a0182134b8948326ac5b4be549704192b337aa046334bdcf3fcec77e2` | 10 |

Every individual shard's failure count, canonical-content digest, and
deterministic gzip-byte digest is embedded in its owning scoreboard.

| Transition | File SHA-256 | Result |
|---|---|---|
| architectural CI from G57 | `42cd9645452f135c277d08edfc079c4adfa64728362d40d3569f6ec6887dc975` | no new pass/fail/signature/classification change |
| architectural full from G57 | `f4cd0383a676712d3dad96ff8acadc57c9250b2e6f3b14a5f8c5b3f16fc1f9be` | no new pass/fail/signature/classification change |

Both transitions explicitly identify G57, the exact predecessor SHA,
`evaluated_sha`, profile, scope, dataset, comparison contract, and
selected/applicable before-and-after digests. For both scopes:

```text
newly_passing=0
newly_failing=0
changed_failure_count=0
changed_failure_shards=[]
classification_changes=[]
```

The before and after scoreboard digests are identical within each scope.

The generator was invoked separately for each unambiguous family:

```text
python3 tools/qa/upd9002_ssts_ratchet.py generate \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --raw-summary /tmp/vaeg-m58-portable-raw.8Qgola/v20_native_ci.json \
  --contract tests/ssts/contracts/upd9002_architectural_v1.json \
  --profile architectural --scope ci \
  --evaluated-sha d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53 \
  --output tests/ssts/scoreboard/g58_architectural_ci.json \
  --failure-directory tests/ssts/scoreboard/g58_architectural_ci_failures

python3 tools/qa/upd9002_ssts_ratchet.py generate \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --raw-summary /tmp/vaeg-m58-portable-raw.8Qgola/v20_native_full.json \
  --contract tests/ssts/contracts/upd9002_architectural_v1.json \
  --profile architectural --scope full \
  --evaluated-sha d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53 \
  --output tests/ssts/scoreboard/g58_architectural_full.json \
  --failure-directory tests/ssts/scoreboard/g58_architectural_full_failures

python3 tools/qa/upd9002_ssts_ratchet.py generate \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --raw-summary /tmp/vaeg-m58-portable-raw.8Qgola/v20_fingerprint_full.json \
  --contract tests/ssts/contracts/upd9002_fingerprint_v1.json \
  --profile fingerprint --scope full \
  --evaluated-sha d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53 \
  --output tests/ssts/scoreboard/g58_fingerprint_full.json \
  --failure-directory tests/ssts/scoreboard/g58_fingerprint_full_failures

python3 tools/qa/upd9002_ssts_ratchet.py ratchet \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --candidate tests/ssts/scoreboard/g58_architectural_ci.json \
  --predecessor-sha 72322d5c9b8e40e4a988312aebe163a8190e2aa5 \
  --output tests/ssts/transitions/g58_architectural_ci_from_g57.json

python3 tools/qa/upd9002_ssts_ratchet.py ratchet \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --candidate tests/ssts/scoreboard/g58_architectural_full.json \
  --predecessor-sha 72322d5c9b8e40e4a988312aebe163a8190e2aa5 \
  --output tests/ssts/transitions/g58_architectural_full_from_g57.json
```

All five commands exited zero.

## Taxonomy and registries

`tests/ssts/gap_taxonomy.json` is a separate content-addressed annotation of
the immutable known-gap rules; it does not edit the M43 classification file.

| `gap_kind` | Rules | Resolved full hashes |
|---|---:|---:|
| `documented_silicon_absent` | 1 | 5,000 |
| `implementation_missing` | 39 | 63,626 |
| `target_support_unverified` | 0 | 0 |
| total | 40 | 68,626 |

The taxonomy file SHA-256 is
`541001ae960b487ca4ee932de2cd764936e08af141dcaca1fb456a82f76eb38e`.
Each annotation records the exact selector digest, resolved count, and sorted
resolved-hash digest.

`hardware_pending.json` is schema-valid and empty because there are zero
`target_support_unverified` entries; coverage is therefore exactly 0 of 0.
Its SHA-256 is
`cb1b55739278badea0dd1c5a10b0becb45ee73394715cf8b88077a513e5e72cc`.
The registry is orthogonal and cannot alter classification, denominator, or
outcome.

`approved_target_divergences.json` is schema-valid and empty. Its SHA-256 is
`dffc832b8f163bcf1783220ca0484ad432195105722bc0a40b441f656478c6e7`.
No divergence transition was approved or used in M58.

## Lettered-milestone tooling audit

The audit covered ROADMAP parsing, task/report discovery, documentation
validation, gates, branch names, commit prefixes, artifact path generation,
CI workflow filters, and repository scripts that derive identifiers.
Canonical `M60a`, `m60a`, `G60a`, `topic/m60a-*`, and `M60a:` forms are
supported with strict full-match parsing. Malformed suffixes, ambiguous
forms, mixed case, missing separators, and numeric-only-prefix partial
matches fail closed.

```text
python3 tools/qa/milestone_ids.py --selftest --audit --discover
```

exited zero with:

```text
48 strict acceptance/rejection checks passed
implementation tree: tasks=62 reports=32 roadmap_rows=58
final evidence tree: tasks=62 reports=33 roadmap_rows=58
audit passed
```

The initial exploratory spelling
`python3 tools/qa/milestone_ids.py selftest` exited 2 because the CLI uses
flags; it made no change. The corrected command above is the validated
interface. The audit found no need for an integer-renumbering proposal.

## Schema, negative tests, and reproducibility

The versioned scoreboard schema defines structural forms independently of
outcome and validates canonical ordering, opcode/subform syntax, counts,
classification, digest generation, and schema version. The transition schema
validates complete deterministic changed-failure enumeration.

```text
python3 tools/qa/upd9002_ssts_ratchet.py selftest
```

exited zero: 43 positive and fail-closed checks passed. Rejections cover:

- mutable-golden self-approval and current-worktree self-comparison;
- omitted and wrong predecessor SHA;
- dataset, contract, and selected-set identity mismatch;
- new failure, per-form pass decrease, timeout, crash, and signature change;
- every prohibited classification transition;
- unknown/missing `gap_kind` and missing or mismatched hardware coverage;
- open selectors, overlapping ownership, and outcome-based splitting;
- malformed/nondeterministic transitions and gzip shards; and
- modification of immutable M43 evidence.

```text
python3 tools/qa/upd9002_ssts_ratchet.py verify-static --root .
```

exited zero and verified immutable M43 evidence, both contracts, taxonomy,
registries, 3 scoreboards, 29 shards, and 2 transitions.

All scoreboards and shards were generated again into
`/tmp/vaeg-m58-portable-regen.BA9uui` from the same raw results and evaluated
SHA.
Both of the following exited zero with no output:

```text
diff -qr tests/ssts/scoreboard /tmp/vaeg-m58-portable-regen.BA9uui/scoreboard
diff -qr tests/ssts/transitions /tmp/vaeg-m58-portable-regen.BA9uui/transitions
```

This proves byte-identical canonical JSON and deterministic gzip output,
including controlled gzip metadata. The verifier additionally accepts a
second standards-valid raw-DEFLATE encoding of the same canonical payload,
which prevents zlib-version differences from being misclassified as evidence
drift; the committed raw SHA-256 still pins each exact artifact.

## Builds, repository checks, and tests

| Configuration | Result |
|---|---|
| Linux GCC CI, final evaluated SHA | configure/build pass; CTest 39/39 non-external; architectural CI run separately without skip |
| Linux Clang CI | configure/build pass; CTest 40/40, external SST 507.26 seconds, no skip |
| Linux ASan/UBSan CI | configure/build pass; CTest 40/40, external SST 512.75 seconds, no skip |
| Linux release | configure/build pass; selftest and ROM-less smoke pass |
| MinGW-w64 cross release | configure/build pass; PE32+ x86-64; Wine selftest and ROM-less smoke pass |
| Encoding | pass, exit 0 |
| EOL | pass, exit 0 |
| Path case | pass, 0 findings |
| Static M58 verifier | pass |
| M58 ratchet selftest | 43/43 |
| Milestone identifier selftest | 48/48 |

The MinGW binary imports only Windows system DLLs; it does not import
`SDL2.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, or
`libwinpthread-1.dll`. The first Wine launch was blocked before program
execution by the sandbox's read-only `/run/user/1000` and exited 1. Repeating
the identical command with Wine runtime access exited zero; both selftest and
smoke passed.

The applicable build/test commands were:

```text
cmake --preset linux-ci-gcc -B /tmp/vaeg-m58-portable-build \
  -DVAEG_SSTS_V20_ROOT=/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build /tmp/vaeg-m58-portable-build --parallel
ctest --test-dir /tmp/vaeg-m58-portable-build --output-on-failure \
  -E '^vaeg_upd9002_ssts_ci_external$'

cmake --preset linux-ci-clang -B /tmp/vaeg-m58-linux-clang.4Shqmx \
  -DVAEG_SSTS_V20_ROOT=/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build /tmp/vaeg-m58-linux-clang.4Shqmx --parallel
ctest --test-dir /tmp/vaeg-m58-linux-clang.4Shqmx --output-on-failure

cmake --preset linux-ci-asan -B /tmp/vaeg-m58-linux-asan.nl0lKK \
  -DVAEG_SSTS_V20_ROOT=/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build /tmp/vaeg-m58-linux-asan.nl0lKK --parallel
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 \
  ctest --test-dir /tmp/vaeg-m58-linux-asan.nl0lKK --output-on-failure

cmake --preset linux-release -B /tmp/vaeg-m58-linux-release.dqkRPy
cmake --build /tmp/vaeg-m58-linux-release.dqkRPy --parallel
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /tmp/vaeg-m58-linux-release.dqkRPy/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /tmp/vaeg-m58-linux-release.dqkRPy/sdl2/vaeg --smoke

cmake --preset mingw-cross -B /tmp/vaeg-m58-mingw-cross.AiCQxt
cmake --build /tmp/vaeg-m58-mingw-cross.AiCQxt --parallel
file /tmp/vaeg-m58-mingw-cross.AiCQxt/sdl2/vaeg.exe
x86_64-w64-mingw32-objdump -p \
  /tmp/vaeg-m58-mingw-cross.AiCQxt/sdl2/vaeg.exe
WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m58-wine.vkQPVS \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /tmp/vaeg-m58-mingw-cross.AiCQxt/sdl2/vaeg.exe --selftest
WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m58-wine.vkQPVS \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /tmp/vaeg-m58-mingw-cross.AiCQxt/sdl2/vaeg.exe --smoke

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/qa/upd9002_ssts_ratchet.py selftest
python3 tools/qa/upd9002_ssts_ratchet.py verify-static --root .
git diff --check
```

Except for the two explicitly documented corrected invocations, all listed
commands exited zero.

## Hosted CI

The implementation chain was pushed before the final evidence commit to run
the full hosted matrix. The verified corpus was available to every dedicated
uPD9002 job; it was acquired, manifest-verified, and not skipped.

- Run: [build 30064747623](https://github.com/nakatamaho/vaeg/actions/runs/30064747623)
- Head SHA: `f37a2243fcfe53d73a812aa377d48f3d048d81da`
- Result: success; all 8 jobs passed
- Required hosted profile: deterministic architectural CI, verified corpus
  available, no skip; job completed in 10 minutes 40 seconds.

The first evidence-only candidate exposed the cross-zlib verification defect:

- Run: [build 30065294952](https://github.com/nakatamaho/vaeg/actions/runs/30065294952)
- Head SHA: `249ede71e3740e73387b3c7121dd486dc2d7343a`
- Result: failed only in Windows `vaeg_upd9002_ssts_ratchet_static`;
  Windows and Linux produced different valid raw-DEFLATE streams for the same
  canonical payload. The dedicated architectural SST job still passed without
  skip. This candidate and its evidence were discarded.

The corrected evaluated implementation then passed the full matrix:

- Run: [build 30066054868](https://github.com/nakatamaho/vaeg/actions/runs/30066054868)
- Head SHA: `d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53`
- Result: success; all 8 jobs passed, including the Windows static verifier
- Dedicated job: `uPD9002 architectural SST ratchet`
- Required hosted profile: deterministic architectural CI, verified corpus
  available, no skip; job completed in 10 minutes 56 seconds

The final evidence-only push triggers the same matrix at the branch tip. Its
run identifier and result are supplied in the maintainer handoff because the
run cannot exist until after the commit containing this report has been
created and pushed.

## Known limitations and remaining risk

- The architectural populations still contain 10,593 CI and 84,329 full
  failures. M58 freezes and governs those identities; it does not fix them.
- The all-16-bit fingerprint profile has 186,767 failures and is diagnostic,
  not an architectural pass criterion.
- This evidence is V20 SST differential evidence. It is not complete uPD9002
  silicon validation and makes no such claim.
- Full and fingerprint profiles remain gate-time local runs; hosted CI
  enforces the deterministic architectural CI profile.
- The corpus is external data and must be acquired and verified before a
  non-skipped run. Corpus-unavailable CI skips do not satisfy G58.
- `target_support_unverified` currently has no entries, so hardware-pending
  coverage is 0 of 0. Future entries must carry exact matching content
  addresses.

## Human verification

From a clean checkout of the implementation commit, verify the corpus, build
the worker, run all three profile commands above, regenerate the three
scoreboard families, and enforce both architectural transitions with the
explicit predecessor:

```text
python3 tools/qa/upd9002_ssts_ratchet.py ratchet \
  --dataset-root /path/to/verified/singlesteptests-v20 \
  --candidate tests/ssts/scoreboard/g58_architectural_ci.json \
  --predecessor-sha 72322d5c9b8e40e4a988312aebe163a8190e2aa5 \
  --output /tmp/g58_architectural_ci_from_g57.json

python3 tools/qa/upd9002_ssts_ratchet.py ratchet \
  --dataset-root /path/to/verified/singlesteptests-v20 \
  --candidate tests/ssts/scoreboard/g58_architectural_full.json \
  --predecessor-sha 72322d5c9b8e40e4a988312aebe163a8190e2aa5 \
  --output /tmp/g58_architectural_full_from_g57.json

python3 tools/qa/upd9002_ssts_ratchet.py verify-static --root .
python3 tools/qa/upd9002_ssts_ratchet.py selftest
python3 tools/qa/milestone_ids.py --selftest --audit --discover
git diff --check
git status --short
```

Before handoff, `git status --short` was empty after the evidence commit,
`git rev-parse HEAD` and `git rev-parse @{u}` were identical, and
`git diff --check` exited zero. G58 remains unapproved pending maintainer
review. M59 is untouched.
