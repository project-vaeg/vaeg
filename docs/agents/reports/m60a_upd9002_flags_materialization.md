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
# M60a uPD9002 FLAGS materialization

M60a corrects only the M59-proven guest-visible FLAGS materialization and
loading rules. It does not change interrupt eligibility, vectoring, frame
placement, stack addressing, IRET, BOUND range decisions, decoding,
effective-address calculation, comparison contracts, fixtures, or
classifications.

M60a is complete and pushed. G60a is an unapproved candidate pending human
review. M60b, M60c, and later milestones have not been started.

A Git commit cannot contain its own SHA. The evidence commit and final
candidate are therefore the commit containing this report; their exact SHA is
supplied in the maintainer handoff and is independently available as
`origin/topic/m60a-upd9002-flags-materialization`. The report and artifacts do
not create a self-reference. The semantic implementation actually executed is
fixed below as `evaluated_sha`.

## Identity and preparation

- Approved predecessor gate: `G59`
- Exact approved G59 SHA and M60a base:
  `e7f2325bc81310532091a8ca82914030fdb8b6ba`
- Approved G59 CI:
  [build 30077960589](https://github.com/nakatamaho/vaeg/actions/runs/30077960589)
- Approved M59 analysis SHA:
  `7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7`
- Initial primary-worktree branch: `main`
- Initial primary-worktree SHA:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty with unrelated maintainer work
- Dedicated worktree: `/tmp/vaeg-m60a.CQrxqW`
- Dedicated worktree starting SHA:
  `e7f2325bc81310532091a8ca82914030fdb8b6ba`
- M60a branch: `topic/m60a-upd9002-flags-materialization`
- Semantic implementation and `evaluated_sha`:
  `3d66d41f750048eb29d13c4b7b53ea757d1d1921`
- Evidence commit and final candidate: the commit containing this report;
  exact SHA in the maintainer handoff and remote branch ref

The primary worktree was not cleaned, reset, stashed, staged, or modified.
The dedicated worktree was created directly from the fixed predecessor. The
maintainer authorization, approved G59 branch, G59 report, and G59 evidence
all identify the same predecessor. No conflicting approved G59 SHA was found.

The canonical task discovered by the strict lettered-milestone tooling is
`docs/agents/tasks/M60a_upd9002_flags_materialization.md`. It was added with
the implementation because the maintainer explicitly authorized committing
the necessary task documents. No duplicate guessed task was created.

The preparation commands all exited zero:

```text
git status --short
git branch --show-current
git rev-parse HEAD
git rev-parse e7f2325bc81310532091a8ca82914030fdb8b6ba
git show --stat --oneline e7f2325bc81310532091a8ca82914030fdb8b6ba
git rev-parse origin/topic/m59-upd9002-evidence-pack
python3 tools/qa/milestone_ids.py --selftest --audit --discover
```

The milestone audit passed 48 strict checks and discovered 64 tasks, 34
reports, and 60 ROADMAP rows. It confirmed strict support for `M60a`, `G60a`,
`topic/m60a-*`, and `M60a:` without accepting ambiguous identifiers.

## Environment, corpus, and contracts

| Component | Recorded value |
|---|---|
| Host | `Linux-6.18.33.2-microsoft-standard-WSL2-x86_64-with-glibc2.43` |
| Distribution | Ubuntu 26.04 under WSL2 |
| Git | 2.53.0 |
| CMake | 4.2.3 |
| Ninja | 1.13.2 |
| GCC | 15.2.0 |
| Clang | 21.1.8 |
| Python | 3.14.4 |
| gzip command | 1.14 |
| Python gzip module | `/usr/lib/python3.14/gzip.py` |
| zlib compile/runtime | 1.3.1 / 1.3.1 |
| Wine | 10.0 |

The verified corpus was available for every required run:

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

## Approved G59 reproduction

Before semantic editing, the architectural CI, architectural full, and
fingerprint full profiles ran without skip at exact G59.

| Profile | Selected | Applicable/executed | Pass | Fail | Non-applicable | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 166,821 | 156,228 | 10,593 | 13,179 | 0 | 0 |
| architectural full | 1,562,502 | 1,443,876 | 1,359,547 | 84,329 | 118,626 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,443,876 | 1,257,109 | 186,767 | 118,626 | 0 | 0 |

The full architectural and fingerprint runs took approximately 496 and 501
seconds. The CI profile and its ratchet completed successfully through CTest.
Counts were not used as identity substitutes.

| Identity | CI | Full |
|---|---|---|
| selected hashes | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` |
| applicable hashes | `80069e9a95f29b38e8f268b806f3ad8c7cb973c11d23b6cb64450ff00fc497cc` | `7de13cbd54e709e0d0d0abefedac876306c8a67c7936f6a26c983362fed6d23c` |
| architectural pass hashes | `053195c1ac7001fa553da6201cd9e7a6843e8de2846796ca608cfca09ab30c78` | `64e33aabb7ad4e926c329105a13bf917559e8e6ee37ebc784def8a52256fdee1` |
| architectural failure hashes | `6129fb38d6ad32d739027819ec24e0ffe7caba26ef7e8e7e595f9cfd987cb63e` | `cd3bac7b62f0c661d1c660dfadd13cb9e8690dca6d66449c792b3bc1b06d57b4` |
| architectural signature index | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |

Fingerprint full pass and failure hash digests were respectively
`93bb15a8b318fe0aca43855c66cd8eda2acb8d8b941cc320fdb8dfc44769a4bd`
and
`deee927e54977788f0d0222ff5cdbdc0f4a0dace1e9deb8005d51b67c3b71299`.

Immutable evidence remained exact:

| Protected evidence | SHA-256 |
|---|---|
| G43 manifest | `77dd1e53f325f3910bd727d3dec4b9c1e23c005f0b306c085a34569e9cf5b23f` |
| M43 CI summary | `a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31` |
| M43 CI failure index | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` |
| M43 full summary | `dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9` |
| M43 full failure index | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |

M58 ratchet static verification, M59 evidence static verification over all
160,000 evidence rows, failure sidecars, mismatch classes, termination
classes, classifications, taxonomy, registries, and protected-artifact
digests all passed. Approved G58 and G59 artifacts were not regenerated or
modified.

## Pre-change FLAGS path audit

The pre-change paths were distinct but used two overly broad legacy macros:

1. Internal architectural FLAGS were stored in `I286_FLAG`, with overflow
   represented separately by `I286_OV`.
2. The SST worker reconstructed final FLAGS for architectural and all-16-bit
   fingerprint observations; neither comparison contract materialized a
   guest stack or AH image.
3. V30 `PUSHF` used `REAL_V30FLAG`, which selected low stored bits, reinserted
   overflow, and forced the high nibble.
4. CPU software interrupts called `i286c_intnum()`, which pushed
   `REAL_FLAGREG`; that macro masks the stored word to 12 bits.
5. V30 `POPF` popped directly into `I286_FLAG`, forced the high nibble, split
   overflow, and did not clear reserved bits 3 and 5.
6. `SAHF` assigned AH directly to `I286_FLAGL`.
7. `LAHF` copied the already materialized low FLAGS byte to AH.

The device/hardware interrupt entry point is separate from
`i286c_intnum()` and was intentionally left unchanged. No generic
normalization helper was introduced because internal state, PUSHF images,
interrupt images, POPF loads, SAHF loads, and LAHF images have different
contracts.

## Implementation and semantic-diff audit

The implementation chain is:

```text
aab78b78a2473ce35b1e28a9af7420e46e72a1c4
M60a: correct guest-visible FLAGS materialization

3d66d41f750048eb29d13c4b7b53ea757d1d1921
M60a: preserve immutable G43 gate identity
```

The first commit contains the minimal semantic changes, focused tests,
ratchet/evidence support, build integration, canonical task, and necessary
ROADMAP/schema documentation. The second commit changes only the generic
ratchet and M60a wrapper so the immutable G43 manifest continues to verify its
historical G57 identity after the candidate switches to G59/G60a identity.
No evidence artifact existed before that fix. All required profiles and
evidence were rerun against the second commit, which is `evaluated_sha`.

The permanent bug-fix ledger was then updated in the separate documentation
commit:

```text
e806791
M60a: record FLAGS materialization correction
```

The exact production semantic changes relative to G59 are:

| File and candidate line | Classification | Change |
|---|---|---|
| `cpu/upd9002/upd9002_core.c:246` | interrupt saved-FLAGS materialization | add an explicit all-16-bit image with split overflow reconstructed |
| `cpu/upd9002/upd9002_core.c:262` | interrupt saved-FLAGS materialization | use that image only for the existing CPU software/fault frame |
| `cpu/upd9002/upd9002_dispatch.c:305` | PUSHF image materialization | replace `REAL_V30FLAG` with an explicit all-16-bit image |
| `cpu/upd9002/upd9002_dispatch.c:318` | POPF loading | load only the M59-observed bits and force the observed fixed bits |
| `cpu/upd9002/i286c_mn.c:1788` | SAHF loading | load AH bits 0, 2, 4, 6, and 7; force bit 1 and clear bits 3 and 5 |

`LAHF` production code is byte-identical to G59. The focused test support
lives in `tests/upd9002/flags_materialization.c` and
`tests/upd9002/flags_materialization.h`.

The semantic audit found no changes to decoder tables, effective-address
logic, generic stack or memory wrapping, interrupt eligibility or vector
selection, saved IP or CS selection, frame order or placement, IRET, BOUND
range logic, FF `/7`, fixtures, classifications, taxonomy, registries, or
comparison contracts.

## Focused semantic results

The new deterministic focused test reports:

```text
upd9002-flags-materialization: groups=7 deterministic checks passed
```

It covers CC, CD, and interrupting CE; ordinary, physical-wrap, and 64-KiB
segment-wrap frames; preservation of saved IP, saved CS, addresses, final SP,
vector, and termination; PUSHF; POPF; SAHF; and LAHF.

The exact M59 representative boundary records used include:

- CC physical wrap:
  `0199d4e94995854d9b05d3a14ea0edd88ee43a00c9a9664a82bec8855e5e978e`
- CC segment wrap:
  `97656ccbabdcb985a47ad03aa0f348b7ba5a9228a730f3a807b2fd58fd2216c9`
- PUSHF physical wrap:
  `007e62c57c1a718b7fb18abf4309759fdd45984163c2d22893ce9bffac585421`

The resulting bit contracts are:

| Guest-visible operation | Proven M59/M60a rule |
|---|---|
| CC/CD/interrupting CE saved FLAGS | bits 0 through 15 copied, with the split overflow bit reconstructed |
| PUSHF image | bits 0 through 15 copied; stack addressing unchanged |
| POPF | bits 0, 2, 4, 6, 7, 9, 10, and 11 load; bit 1 and bits 12 through 15 are forced one; bits 3, 5, and 8 are forced zero |
| SAHF | bits 0, 2, 4, 6, and 7 load from AH; bit 1 is forced one; bits 3 and 5 are forced zero; unrelated high FLAGS bits remain untouched |
| LAHF | AH bits 0, 2, 4, 6, and 7 copy FLAGS; bit 1 is one; bits 3 and 5 are zero |

PUSHF remains 4,999 pass and one fail. The residual is the pre-existing
64-KiB segment-wrap high-byte write anomaly. Its exact hash-set digest is
`382656b685e987c1072d2a47fec5f338463f9baec6004ee2452a9c1b6ff2ee1d`.
M60a did not change generic stack or RAM-write behavior to hide it. LAHF
remains 5,000/5,000 with an empty failure-set digest
`4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`.

## Candidate profile and per-form results

Every candidate profile ran without skip against exact `evaluated_sha`.
Elapsed times measured from output-directory creation to summary completion
were approximately 228 seconds for CI, 517 seconds for architectural full,
and 529 seconds for fingerprint full.

| Profile | Selected | Applicable/executed | Pass | Fail | Non-applicable | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 166,821 | 158,562 | 8,259 | 13,179 | 0 | 0 |
| architectural full | 1,562,502 | 1,443,876 | 1,383,294 | 60,582 | 118,626 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,443,876 | 1,280,856 | 163,020 | 118,626 | 0 | 0 |

| Form | G59 pass/fail | G60a pass/fail | Newly passing |
|---|---:|---:|---:|
| 9C PUSHF | 4,999 / 1 | 4,999 / 1 | 0 |
| 9D POPF | 1,238 / 3,762 | 5,000 / 0 | 3,762 |
| 9E SAHF | 1,262 / 3,738 | 5,000 / 0 | 3,738 |
| 9F LAHF | 5,000 / 0 | 5,000 / 0 | 0 |
| CC INT3 | 0 / 5,000 | 5,000 / 0 | 5,000 |
| CD INT imm8 | 0 / 5,000 | 5,000 / 0 | 5,000 |
| CE INTO | 2,532 / 2,468 | 5,000 / 0 | 2,468 |

The five directly failing target groups contributed exactly 19,968 newly
passing hashes, digest
`68cc53ac3848c24239d4afc9c237f5a15f1ea003c1addb9cf96cd42d0307dc39`.
Every evidence-derived family-level target was reached.

The arithmetic reference of 64,361 full failures assumed that no dependent
population would change. The candidate instead reached 60,582 failures
because the same in-scope saved-FLAGS primitive corrected 3,779 additional,
fully enumerated interrupt/fault-frame records. The 3,779-set digest is
`fc2ad15ac7979ea85a987eea3b0ca086d17cc9198168051cdc8b580df832e672`.
This is an exact evidence-supported explanation, not permission for an
unrelated fix; G60a remains pending maintainer review.

## BOUND and dependent fault frames

| Population | G59 pass/fail | G60a pass/fail | Result |
|---|---:|---:|---|
| BOUND (`62`) | 191 / 4,809 | 3,756 / 1,244 | all 3,565 M59 frame-only failures passed |
| F6 `/6` | 2,391 / 2,609 | 2,439 / 2,561 | 48 fault-frame records passed |
| F6 `/7` | 1,220 / 3,780 | 1,284 / 3,716 | 64 fault-frame records passed |
| F7 `/6` | 2,475 / 2,525 | 2,514 / 2,486 | 39 fault-frame records passed |
| F7 `/7` | 1,214 / 3,786 | 1,277 / 3,723 | 63 fault-frame records passed |

The BOUND newly passing set is byte-for-byte the approved M59 frame-only set:

```text
count  = 3565
digest = 15862f179608f8745f76bb3565197106ae6f63cba6c3363dd307fb29e6bbd746
```

The 1,244 non-frame-only BOUND failures remain exact:

```text
count  = 1244
digest = 2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d
```

No BOUND range decision was changed. The 214 F6/F7 newly passing records are
also solely saved-FLAGS frame corrections. Another 12,481 still-failing F6/F7
records changed signature because their saved FLAGS bytes improved while an
independent divide result remained wrong. They are fully enumerated:

| Form | Changed failures | Hash-set SHA-256 |
|---|---:|---|
| F6 `/6` | 2,561 | `6903d1f589918b23e72337baec5cc104df5bdfd1590d1eb9e43ffbfdbd17dbd6` |
| F6 `/7` | 3,712 | `8b1f3edd6a038ffd5e1e15c1071bdd553f65982b0202a6cbd97cad9fc2c146dd` |
| F7 `/6` | 2,485 | `82a4e93c74e73b0f1e220d11c23d9d9f285cfd3886e2e7a8dbca6e702d7e2864` |
| F7 `/7` | 3,723 | `5eb4005382072fcd6eeac47fe353c57ddc9b94ddadecf66082838d0902e05cd6` |

The combined changed-failure hash-set digest is
`9ef4e914742a7223b1574896d53683358a4ab804be830b10045b60b0f7300ada`.
M60a does not implement a divide or interrupt-frame-placement fix.

## Ratchet identities and artifacts

Selected and applicable populations are unchanged:

| Identity | CI | Full |
|---|---|---|
| selected hash set | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` |
| applicable hash set | `80069e9a95f29b38e8f268b806f3ad8c7cb973c11d23b6cb64450ff00fc497cc` | `7de13cbd54e709e0d0d0abefedac876306c8a67c7936f6a26c983362fed6d23c` |

Candidate result identities are:

| Profile | Pass hash set | Failure hash set | Failure signature index |
|---|---|---|---|
| architectural CI | `7db636be439d720eba65646a00869ff1904928b106757e7039510fca0b8c96d6` | `605b2eccd70262c4f6c981e5727a6fd8aa2073c175c977ed4b198985f31e3d39` | `b7f7916153a58aa53dac61cfd7b4a055d80ec8e98c658d066dc2b8b8788c6476` |
| architectural full | `2f91c2fb41bc83afb639ad56fa50e7b462740d6982a005ecfd7aebba761f41a8` | `bd77a1635c75827d977a0a03b39da7601f047ab7c8b9c3409023ec46dc55ed4b` | `23fee219c9d0c7afc120690b6db40d38b4ee82fa5d275c3c323d1a0dc3f34d58` |
| fingerprint full | `3502e257f5330b1d8c6b808525880bf40379038e1319838e8a04ef02bcb25751` | `8db523363ac5e839cb328edfcc1009417701e41188d804af6c2dbdef4483f807` | `eaaa9b6012e8ab03b816db6240d99bc5eb522beacb76cb2cbcad44cad78f598b` |

Architectural full and fingerprint full have the same 23,747 newly passing
hashes, digest
`eb5f10783435131193165290818ec3f235fa8b161c9c47ffb306a581fb196df0`,
and zero newly failing hashes. The empty-set digest is
`4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`.
CI has 2,334 newly passing hashes, digest
`54a436fb214394ac48584c95ece96755f4fee18d8efae3c0499b0d1d6790b855`,
and zero newly failing hashes.

The G60a artifact family contains 36 files and 28,804,763 bytes. A canonical
manifest of `{path, bytes, sha256}` rows has digest
`2f03e42095da58d521ac5b491a571faf6d078db2b12794f2e0249354345c2901`.

| Artifact | File SHA-256 | Shard/raw-set SHA-256 |
|---|---|---|
| `tests/ssts/scoreboard/g60a_architectural_ci.json` | `7a5d2026ce5ef8ab19f8df7a067d334d4e1b0cf4e4b6a0ea75125c37776e7d59` | `411755a5a2eb02f54b2ae1769396c9ab31641df3f4754d19cdda83e42006e048` |
| `tests/ssts/scoreboard/g60a_architectural_full.json` | `2b59d863adf4d358f93c2cf5b97640c6443ecd370113ecb142ef816119692cfc` | `364b31fbffc4719995a362a02edeec57bf98e8825db8f76873b2d557a6f62548` |
| `tests/ssts/scoreboard/g60a_fingerprint_full.json` | `4813afad7a92e7de2518259fa37577fc0cdba64dd5ed70ac767d8425a2e8dc8f` | `b264c0f4fc392915a78081f10df1177dcca97d6da60fd9dfe48998bfa2355f69` |
| `tests/ssts/transitions/g60a_architectural_ci_from_g59.json` | `0931da68cb7b0f58823c7f2ed90130e513b06f04f04bfdfb4600203610772884` | changed shard `4cfb5d564f44dc8bb7da55b1e015ccc4ccd9ab6f7a27a13b4360b695bf9e8b39` |
| `tests/ssts/transitions/g60a_architectural_full_from_g59.json` | `86b05dba8b958eb731c89c016cd9898b18ac5ff91a53229c0ef3a3aa797e8c13` | changed shard `5d0ea028867228b4efd3429ec4800ba710c0e4819349a7098f4e3024989844c7` |
| `tests/ssts/transitions/g60a_flags_materialization_summary.json` | `2f644c1e65197fc65f9916596035b70134457d6bbf0eec89a45cad0b50af42b6` | not compressed |

The full transition has 23,747 newly passing, zero newly failing, 12,481
changed failures, and zero classification changes. The CI transition has
2,334 newly passing, zero newly failing, 1,256 changed failures, and zero
classification changes. No structural form lost pass count. Dataset,
contracts, selected sets, applicable sets, classifications, taxonomy, and
termination classes remain valid; timeout and crash counts are zero.

## Deterministic regeneration

The three scoreboards, all failure shards, both transitions, both
changed-failure shards, and the focused summary were generated twice from the
same raw candidate results. Every file compared byte-identically with `cmp`
or `diff -qr`.

The canonical writer emits sorted compact JSON plus one LF. For gzip it emits
a fixed ten-byte header, raw DEFLATE at level 9 with fixed parameters, and an
explicit CRC/size trailer. The verified bounded claim is byte identity on
this recorded Python 3.14.4 and zlib 1.3.1 environment. It is not a claim that
different zlib implementations produce identical DEFLATE streams. This
preserves the maintainer-accepted G58 gzip environment limitation without
rewriting G58 evidence.

## Builds, tests, and hosted CI

The relevant commands and results were:

| Command | Result |
|---|---|
| `cmake --preset linux-ci-gcc && cmake --build --preset linux-ci-gcc` | exit 0 |
| `ctest --test-dir build/linux-ci-gcc --output-on-failure -LE external` | 44/44 passed, 67.85 s |
| `cmake --preset linux-ci-clang && cmake --build --preset linux-ci-clang` | exit 0 |
| `ctest --test-dir build/linux-ci-clang --output-on-failure -LE external` | 44/44 passed, 68.12 s |
| `cmake --preset linux-ci-asan && cmake --build --preset linux-ci-asan` | exit 0 |
| `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan --output-on-failure -LE external` | 44/44 passed, 189.47 s |
| `cmake --preset mingw-cross && cmake --build --preset mingw-cross` | exit 0 |
| `WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m60a-wine.ttG3xW wine64 build/mingw-cross/sdl2/vaeg.exe --selftest` | all selftests passed |
| `x86_64-w64-mingw32-objdump -p build/mingw-cross/sdl2/vaeg.exe` | only Windows system DLL imports |
| `ctest --test-dir build/linux-ci-gcc --output-on-failure -L 'external|ratchet|evidence'` | 7/7 passed; verified CI profile did not skip, 578.87 s |
| `python3 tools/qa/upd9002_ssts_ratchet.py selftest` | 43/43 positive/fail-closed checks passed |
| `python3 tools/qa/upd9002_m60a_evidence.py selftest` | lettered identity, strict predecessor, changed-signature, complete-shard, and deterministic tests passed |
| `python3 tools/qa/upd9002_semantics_evidence.py selftest` | 33/33 passed |
| `python3 tools/qa/milestone_ids.py --root . --selftest --discover --audit` | 48 checks passed |
| `python3 tools/repo/check_encoding.py` | 0 findings |
| `python3 tools/repo/check_eol.py` | exit 0 |
| `python3 tools/repo/check_case.py` | exit 0 |
| `python3 tools/repo/find_unreferenced.py` | exit 0; unchanged known set of 98 |
| `git diff --check` | exit 0 |

LeakSanitizer cannot initialize under this WSL2/ptrace environment with leak
detection enabled. The repository CI preset already uses
`ASAN_OPTIONS=detect_leaks=0`; the exact CI environment passed the complete
ASan/UBSan test set. This is an environment limitation, not a skipped
required test.

The final implementation hosted matrix is
[build 30084900092](https://github.com/nakatamaho/vaeg/actions/runs/30084900092).
All eight jobs passed: architectural SST ratchet, repository invariants,
Ubuntu GCC, Ubuntu Clang, Ubuntu ASan/UBSan, macOS, Windows MSYS2 MinGW64, and
standalone Z80 conformance. The verified V20 corpus was available and no
required job skipped.

An earlier superseded run at implementation precursor `aab78b78` exposed a
fail-closed identity bug in the M60a wrapper: switching the mutable G58
predecessor constants before immutable G43 verification made the historical
G57 manifest appear invalid. The CPU result was not accepted as evidence.
Commit `3d66d41f` separated immutable and candidate identities, and every
profile and artifact was rerun. No production CPU line changed in that
correction.

## Exact SST and evidence commands

The raw profiles used the following command shape with the recorded corpus
and exact GCC worker:

```text
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --profile ci \
  --output /tmp/vaeg-m60a-3d66-arch-ci.NBCwTt/v20_native_ci.json \
  --failure-directory /tmp/vaeg-m60a-3d66-arch-ci.NBCwTt/v20_native_ci_failures

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --profile full \
  --output /tmp/vaeg-m60a-3d66-arch-full.IERa63/v20_native_full.json \
  --failure-directory /tmp/vaeg-m60a-3d66-arch-full.IERa63/v20_native_full_failures

python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --profile full --flags-comparison all16 \
  --output /tmp/vaeg-m60a-3d66-fingerprint-full.1G9l6T/v20_fingerprint_full.json \
  --failure-directory /tmp/vaeg-m60a-3d66-fingerprint-full.1G9l6T/v20_fingerprint_full_failures
```

Each raw result was converted with
`tools/qa/upd9002_m60a_evidence.py generate`, explicitly supplying:

```text
--predecessor-sha e7f2325bc81310532091a8ca82914030fdb8b6ba
--evaluated-sha 3d66d41f750048eb29d13c4b7b53ea757d1d1921
```

Architectural candidates were checked with the same tool's `ratchet`
subcommand. `verify-static` then validated all G60a artifacts and protected
G58/G59/G43 evidence.

## Protected-state proof and human review

The following commands are the concise human-review sequence:

```text
git fetch origin
git switch --detach origin/topic/m60a-upd9002-flags-materialization
git diff --stat e7f2325bc81310532091a8ca82914030fdb8b6ba...3d66d41f750048eb29d13c4b7b53ea757d1d1921
git diff --name-status e7f2325bc81310532091a8ca82914030fdb8b6ba...3d66d41f750048eb29d13c4b7b53ea757d1d1921
git diff e7f2325bc81310532091a8ca82914030fdb8b6ba...3d66d41f750048eb29d13c4b7b53ea757d1d1921 -- cpu/upd9002/
python3 tools/qa/upd9002_m60a_evidence.py selftest
python3 tools/qa/upd9002_m60a_evidence.py verify-static --root . --predecessor-sha e7f2325bc81310532091a8ca82914030fdb8b6ba
python3 tools/qa/upd9002_ssts_ratchet.py verify-static --root .
python3 tools/qa/upd9002_semantics_evidence.py verify-static --root .
python3 tools/qa/milestone_ids.py --root . --selftest --discover --audit
git diff --check
git status --short
git rev-parse HEAD
git rev-parse '@{u}'
git ls-remote origin refs/heads/topic/m60a-upd9002-flags-materialization
```

Diff checks against G59 show no change under approved G58/G59 scoreboards,
failure shards, transitions, the G59 evidence pack, immutable G43/M43
evidence, fixtures, classification inputs, taxonomy, registries, or
comparison contracts. The final evidence commit contains only generated
G60a evidence and this report.

## Known limitations

- The SST corpus is V20 compatibility evidence, not complete uPD9002 silicon
  validation.
- The one PUSHF segment-wrap RAM-write anomaly remains blocking and was not
  hidden by changing generic memory behavior.
- Fingerprint full remains diagnostic; it does not weaken the architectural
  ratchet.
- The 12,481 changed F6/F7 failure signatures document improved fault-frame
  FLAGS while independent divide mismatches remain. M60a does not implement
  their later semantic fix.
- The bounded gzip reproducibility claim is limited to the recorded
  compression environment.
- G60a requires human review. No G60a approval tag was created, and M60b,
  M60c, and later work remains untouched.
