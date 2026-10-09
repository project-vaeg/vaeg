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
# M60d uPD9002 synchronous interrupt-frame closure

M60d selects Path A. The complete approved populations and the global current
failure population show that no independent synchronous interrupt-frame
residual remains after M60a. M60d adds deterministic audit and validation
infrastructure and commits the resulting evidence. It makes no production CPU
semantic change.

No independent synchronous interrupt-frame residual remains in the
G60c-approved target-correct population. M60d closes evidence-only and makes
no production CPU semantic change.

M60d is complete and pushed. G60d is an unapproved candidate pending human
review. M60e and later milestones have not been started.

A commit cannot contain its own SHA. The evidence commit and final candidate
are therefore the commit containing this report; the exact SHA is supplied in
the maintainer handoff and is independently available from
`origin/topic/m60d-upd9002-interrupt-frame`. The audit implementation actually
executed is fixed below as `analysis_evaluated_sha`.

## Identity and preparation

- Approved predecessor gate: `G60c`
- Exact approved G60c SHA and M60d base:
  `e425e55fc17117000ba5178a796de4444d897234`
- G60c audit implementation/evaluated SHA:
  `a9dd78bded5c1072f0285f00cf7759654da8b7d8`
- Approved G60c CI:
  [build 30148175007](https://github.com/nakatamaho/vaeg/actions/runs/30148175007)
- Approved target policy:
  `upd9002-g60b-eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93`
- Initial primary-worktree branch: `main`
- Initial primary-worktree SHA:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty with unrelated maintainer work
- Dedicated worktree:
  `/home/maho/vaeg/build/m60d-worktree`
- Dedicated worktree starting SHA:
  `e425e55fc17117000ba5178a796de4444d897234`
- M60d branch:
  `topic/m60d-upd9002-interrupt-frame`
- Initial audit implementation commit:
  `2a531e985600154c0b2fd28e94e8557749b51746`
- Repeat-invocation isolation commit:
  `0fdbf16cbe7bf6507119d327397fe5216165d7b7`
- Upstream-verification implementation commit:
  `f608c9ec902dda9270935977c04f86cb65b4dd74`
- Forward-compatible predecessor-validator commit:
  `49b4a00f4a88c53a61451417b4c3c025fc97fe71`
- Final audit implementation commit and `analysis_evaluated_sha`:
  `ada55de79751c04e44d02abf7ecd6851b55c9763`
- Semantic implementation SHA: none
- Evidence commit/final candidate: the commit containing this report; exact
  SHA in the maintainer handoff and remote branch ref
- Chosen path: `Path A`
- `m60d_outcome`: `evidence_only_closure`
- `semantic_change`: `false`

The primary worktree was not cleaned, reset, stashed, staged, or modified by
M60d. The dedicated worktree was created directly from the fixed predecessor.
The maintainer authorization, local predecessor object, authoritative G60c
report, and `origin/topic/m60c-upd9002-fpo2-audit` all resolve to the same
approved SHA. No conflicting approved G60c SHA was found.

The mandatory preparation commands exited zero:

```text
git status --short
git branch --show-current
git rev-parse HEAD
git rev-parse e425e55fc17117000ba5178a796de4444d897234
git show --stat --oneline e425e55fc17117000ba5178a796de4444d897234
git rev-parse origin/topic/m60c-upd9002-fpo2-audit
python3 tools/qa/milestone_ids.py --selftest --audit --discover
```

Milestone discovery passed 48 strict identifier tests and discovered 79 tasks,
37 reports, and 75 ROADMAP rows. ROADMAP, task discovery, and
`docs/agents/tasks/M60d_upd9002_interrupt_frame.md` agree on the identifier,
branch, report path, and conditional scope.

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

The G60b target policy above remains unchanged. Selected and applicable set
identities remain:

| Scope | Selected SHA-256 | Applicable SHA-256 |
|---|---|---|
| CI | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `5a00d3fa15d55b38630015e771163fe58aff544d5f13342beac01a8236a485a1` |
| full | `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` | `a29c0a52d0818c7797515d5fbcc680b1fecdf5b10b896e5e02afff498cf99d65` |

No comparison contract, top-level classification, taxonomy entry, registry,
fixture, selected set, applicable set, or target-policy identity changed.

## Approved G60c reproduction

Before implementation, a fresh tests-enabled build at the exact approved G60c
SHA ran all three profiles without skip and reproduced the approved state:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,179 | 8,121 | 0 | 0 |
| architectural full | 1,562,502 | 1,438,594 | 1,378,653 | 59,941 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,438,594 | 1,276,215 | 162,379 | 0 | 0 |

Counts were not used as identity substitutes. Dataset, contracts, target
policy, selected/applicable sets, pass/failure sets, signature indexes,
mismatch classes, termination classes, classifications, taxonomy, registries,
scoreboards, transitions, and protected evidence matched G60c exactly. The
approved G60c authority manifest, transition, and artifact tree remained:

```text
authority manifest:
7c556b1edd22637744dfeef6063ec6139bf78a6e350100292dc39cdc5361c68f

authority transition:
85b8b7466e39fbe761bc5993c570878070b053da38f2605edbb97eab05a20751

artifact tree:
51c0bda25f87f679c795df36ac9dc925176890ded55205235999d5598c24fc79
```

M58 ratchet verification, M59 evidence verification, M60a artifact
verification, M60b authority/policy verification, and M60c erratum,
authority, and transition verification all passed.

## Audit design and conditional decision

`tools/qa/upd9002_m60d_frame_audit.py` performs four independent checks:

1. It verifies each raw profile against the approved G60c scoreboard,
   deterministic failure shards, hash sets, signature indexes, termination
   classes, and per-form results.
2. It replays the complete CC, CD, CE, and BOUND populations and the exact
   214-hash M60a divide-exception dependency population.
3. It scans every current architectural failure and replays every failure
   having an expected or actual synchronous event.
4. It rejects Path A if any independently attributable frame observable
   remains different.

The audit compares event classification, termination, initial CS:IP and SS:SP,
final SP, logical and physical frame addresses, 16-bit segment wrap, 20-bit
physical wrap, saved IP, saved CS, saved FLAGS, vector number, vector-table
addresses and fetched target, final CS:IP, post-entry TF/IF, represented RAM,
and unrelated registers.

The audit explicitly distinguishes guest-visible saved FLAGS bytes from final
internal FLAGS, metadata-masked architectural FLAGS, and the full 16-bit
fingerprint. Remaining DIV/IDIV cases in which the saved word faithfully
materializes the actual pre-event FLAGS but the expected and actual pre-event
FLAGS differ are owned by later DIV/IDIV semantics, not by frame placement or
materialization. The exact 214 M60a downstream improvements are independently
required to remain green.

During deterministic-regeneration validation, the first implementation exposed
a state-leak bug in repeated in-process invocation of upstream epoch
validators. Their mutable ratchet identity remained configured after the first
generation and caused a false `wrong epoch gate` rejection on the second. The
second implementation commit isolates and restores that state. Final
evidence-only validation then exposed a separate forward-compatibility defect
in the M60c validator: it treated the entire scoreboard directory, including
new G60d scoreboards, as immutable M60c input. The final implementation commit
validates the M60c artifacts directly and explicitly protects all G60c-and-
earlier scoreboard families while allowing the new G60d family. A final
repository-policy commit records that completed, identity-bound SST results
must be preserved and that hosted CI is not an iterative debugger. No evidence
from an earlier evaluated SHA was retained. The final evidence was generated
against `ada55de79751c04e44d02abf7ecd6851b55c9763`.

## Primary CC, CD, and CE population

The primary interrupt-frame set is unchanged:

```text
count: 12,468
SHA-256:
4498c8aa838f93aba7220f0cdacff34341d704a9cbe7f6d35d79b75219b41d0b
```

| Form | Selected/executed | Taken event | Non-taken | Pass/fail | Frame residual |
|---|---:|---:|---:|---:|---:|
| CC | 5,000 | 5,000 | 0 | 5,000 / 0 | 0 |
| CD | 5,000 | 5,000 | 0 | 5,000 / 0 | 0 |
| CE | 5,000 | 2,468 | 2,532 | 5,000 / 0 | 0 |

Boundary coverage:

| Form/population | Ordinary | Physical wrap | Segment wrap |
|---|---:|---:|---:|
| CC | 4,841 | 158 | 1 |
| CD | 4,839 | 161 | 0 |
| taken CE | 2,390 | 78 | 0 |

For every taken case, final SP, all six logical and physical frame-byte
addresses, saved IP, saved CS, saved FLAGS, vector fetch, final target CS:IP,
post-entry TF/IF, termination, RAM, and unrelated-register preservation agree.
No boundary or wrap residual exists.

For all 2,532 non-taken CE cases, no event or frame is generated and ordinary
continuation remains correct.

Representative cases are recorded under
`tests/ssts/evidence/g60d/representative/`. They include:

- CC ordinary `0000c3973227bb631ff5eb928614199f783d134bdd810d01c142e6f9ce544b4b`;
- CC segment wrap `97656ccbabdcb985a47ad03aa0f348b7ba5a9228a730f3a807b2fd58fd2216c9`;
- CC physical wrap `0199d4e94995854d9b05d3a14ea0edd88ee43a00c9a9664a82bec8855e5e978e`;
- CD ordinary `000dfea7a111adba3e5a13fd92a6182223b6df015f47572636ed13b2fb84bcba`;
- CD physical wrap `0383c91c3b086a47782972cf959d9cb3ea7245a673162b4e60c57f9f690de8cf`;
- CE non-taken `00015a81f6528aa77b401454d35d1bac69d0e791770b2feb7a581f764dd1f63f`;
- CE taken `0007a4eef94283423280cdbad4e6e839560c669d9a71edec8aa905ff6b8b448a`.

## BOUND partitions

The approved BOUND partition arithmetic is exact:

```text
191 previously passing normal cases
+
3,565 former frame-only failures
=
3,756 current passing cases
```

| Partition | Count | Current result | Frame residual | SHA-256 |
|---|---:|---|---:|---|
| former frame-only | 3,565 | all pass | 0 | `15862f179608f8745f76bb3565197106ae6f63cba6c3363dd307fb29e6bbd746` |
| range/non-frame residual | 1,244 | all fail | 0 | `2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d` |
| previously passing normal | 191 | all pass | 0 | content-addressed in the case table |

The 3,565 former frame-only cases include 3,458 ordinary and 107 physical-wrap
frames. All now have correct frame observables. The remaining 1,244 failures
retain their exact G60c hashes and are owned by range/non-frame behavior. M60d
does not change or hide the BOUND range decision.

Representative hashes:

- former frame-only, now green:
  `001e7a0472dbf6f45af3dfbbae3e1dd499c33fa50f21b768decaba6833ff6743`;
- range residual:
  `001bd4ca1c82cd061dfe8ef5c5bef7e87acaf8803fa6b957db9d52becf66e03c`;
- normal completion:
  `0278f04a8fe35fdace053590f9588b78cd2b016a173fb36c301eede22219f272`.

## Divide-exception dependency

The exact M60a dependent improvement set was derived from the approved G60a
transition, not guessed:

```text
count: 214
combined SHA-256:
5cda7079da30b8266de2df3b55b90b3e5ee12a20429e670d1f128b924903c719
```

| Form | Count | SHA-256 |
|---|---:|---|
| F6.6 | 48 | `2223861f50be66681297264720400b50a760163404222b705cc01ae53aa62d5b` |
| F6.7 | 64 | `f120a2a55e5c992390762aa5303195dc198fc3e68d052845bc061536cbb74eac` |
| F7.6 | 39 | `f2e51ac87d951a68210f7917a7b4231c7dd9c31c3ace81895190e6938893eedb` |
| F7.7 | 63 | `b0da56dbe6553fa1de93418a2e4dc1165e94e6475a72eb2a224a94a163f6e6db` |

All 214 cases remain passing with zero frame residual. Coverage includes 211
ordinary and three physical-wrap frames. DIV/IDIV arithmetic, quotient,
remainder, overflow detection, and pre-event FLAGS computation are untouched.

## Global residual-signature scan

All 59,941 current architectural failures were mapped and scanned. The audit
replayed 13,730 failures with expected or actual synchronous event activity:

| Form | Event-related failures |
|---|---:|
| BOUND | 1,244 |
| F6.6 | 2,561 |
| F6.7 | 3,716 |
| F7.6 | 2,486 |
| F7.7 | 3,723 |

The event-related set SHA-256 is:

```text
7934f86c0ffe200f4c5ec363ef23574f2462922aca81490c020a8ad28f55131e
```

The independent frame-residual result is:

```text
count: 0
empty hash-set SHA-256:
4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945
```

No unexplained in-scope family or boundary regression was found. Saved FLAGS
was not reopened: the green M60a primary and 214-hash dependent populations
remain exact, while later arithmetic/FLAGS ownership remains separate. IRET,
BOUND range logic, and DIV/IDIV arithmetic are untouched.

These results satisfy all nine Path A conditions. Path B was not entered and
no cosmetic semantic commit was created.

## Final profile and ratchet results

The exact evaluated commit ran all required profiles without skip:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash | Approx. elapsed |
|---|---:|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,179 | 8,121 | 0 | 0 | 150 s |
| architectural full | 1,562,502 | 1,438,594 | 1,378,653 | 59,941 | 0 | 0 | 303 s |
| fingerprint full | 1,562,502 | 1,438,594 | 1,276,215 | 162,379 | 0 | 0 | 306 s |

Exact result identities:

| Profile | Pass SHA-256 | Failure SHA-256 | Signature-index SHA-256 |
|---|---|---|---|
| architectural CI | `ae83f89ed1bfe34088197012683c726d9dab5a76700bb247efcf9a7e6e0d8bff` | `04ffb31baf4f66e60f0e700ebe8713ad7b1d582352160dc02c61415619426603` | `a67c9135780ce1df35486b2c2a28fe3e309be97c30045b4761ed5a8c652a4132` |
| architectural full | `898b7e6e66fa9ea4e475bfc78db5a0cf8c3d6b109276fac6db0f68e05a1c27f2` | `9c69f518d168e1675e4d1fcf59db031dc1bce058db3fdfe9f6cb66b8d2bec91d` | `776c17c19e52cc03becc83464dfcdcb0b1590532812deac7e93c8955f0c1a473` |
| fingerprint full | `691242bb0a05324fe3be261653863db45f45fd495c83d6798d8491a3e8db42db` | `2d825411e958ec980317d0c29d06aee8f9e7337c036f1d5703014311e53c2cc4` | `84c40271c91cb8d01e3545ada60c50e35594e790ce29e9b4d609c75e1d59d3bb` |

Termination classes and mismatch classes are exact. The transitions record:

```text
newly_passing = empty
newly_failing = empty
changed_failure_count = 0
top_level_classification_changes = empty
gap_kind_changes = empty
hardware_pending_changes = empty
```

There is no per-form pass decrease.

## Evidence identities and deterministic regeneration

Canonical evidence locations:

```text
tests/ssts/evidence/g60d/
tests/ssts/evidence/g60d_result_manifest.json
tests/ssts/scoreboard/g60d_architectural_ci.json
tests/ssts/scoreboard/g60d_architectural_ci_failures/
tests/ssts/scoreboard/g60d_architectural_full.json
tests/ssts/scoreboard/g60d_architectural_full_failures/
tests/ssts/scoreboard/g60d_fingerprint_full.json
tests/ssts/scoreboard/g60d_fingerprint_full_failures/
tests/ssts/transitions/g60d_architectural_ci_from_g60c.json
tests/ssts/transitions/g60d_architectural_full_from_g60c.json
```

Identity digests:

| Artifact | SHA-256 |
|---|---|
| evidence manifest | `5e6e6d4a6946c19bfad59f32fce5dfded345881f17977f8f4add6b011a32c69d` |
| artifact tree | `00e51450b4e2ca9cbdf77c486b193d11448f057eb70c58b65a81eb388cf94e86` |
| architectural CI transition | `2c46c494876b4d3881ac30e0225f7856d175fa94bd3e71678b253bde50f3a4e2` |
| architectural full transition | `e58bc0d05a9af2ce537c962276c326b9ae1c9d521d38cd50bcf25d2428dfba0a` |

The complete evidence family was generated twice against
`ada55de79751c04e44d02abf7ecd6851b55c9763` in the recorded environment.
File inventory and every byte were identical. Both runs produced the same
manifest and artifact-tree digests above.

The three completed profile outputs were preserved across the final
documentation-only commit. Their SHA-256 values are:

```text
worker:
6cabdd52844daf60d76d13ee982078eaf3703a92490e317ebcb710598d589638
architectural CI raw result:
cf351bf8334963cce4ad6b3ed3d6f08971c981fbe0b8f794ff6dea6bcbb73017
architectural full raw result:
63ce779a5c176b8bc2310f4c45f3e8fde6add6a6f7ee24a3fa084c793c9cf864
fingerprint full raw result:
7943a0c256eee58995eb6a2a6a0cc5316541b8b9dc7f3b8eb2a1783580e5db7e
```

At the final evaluated commit, `cmake --build build/linux-ci-gcc -j2`
reported `ninja: no work to do`; the worker digest remained exactly the value
above. The history cleanup changed only `AGENTS.md` and the placement of
generated G60d evidence; neither is a worker build input. Consequently the
already completed, unskipped runs are executions of the exact worker built by
the final evaluated tree; they were not needlessly repeated.

As accepted at G58, the gzip reproducibility claim is bounded to the recorded
Python 3.14.4, gzip module, and zlib 1.3.1 environment. The deterministic
writer fixes gzip metadata. No broader cross-zlib byte-identity claim is made.

## Implementation and semantic-diff audit

The evaluated implementation changes only:

```text
.github/workflows/build.yml
AGENTS.md
CMakeLists.txt
tests/ssts/README.md
tests/ssts/schema/synchronous-frame-audit-v1.md
tools/qa/upd9002_m60c_audit.py
tools/qa/upd9002_m60d_frame_audit.py
```

The implementation commits are one contiguous audit and validation concern.
The second fixes repeat-invocation isolation discovered by deterministic
regeneration. The next two changes make upstream verification
forward-compatible while retaining explicit protection for every approved
predecessor scoreboard. The final documentation-only change records the
maintainer-required CI and expensive-test reuse discipline. The final evidence
was regenerated after all of these changes.

The required semantic diff is empty:

```text
git diff --exit-code \
  e425e55fc17117000ba5178a796de4444d897234...ada55de79751c04e44d02abf7ecd6851b55c9763 \
  -- cpu/upd9002/
```

No IRET, saved-FLAGS implementation, BOUND range decision, DIV/IDIV arithmetic,
fixture, comparison contract, target policy, classification, taxonomy,
registry, 6C-6F, 66/67, FPO2, or 0F extension source changed. Protected
G43/G58/G59/G60a/G60b/G60c artifacts remain byte-identical.

The final evidence commit contains only the generated G60d families listed
above and this report. It changes no audit tooling, test logic, CPU code,
fixture, comparison contract, or policy logic.

## Validation and hosted CI

The focused M60d selftest passed three positive and 25 fail-closed checks.
They cover predecessor and policy identity, selected/applicable/contract
drift, classification/taxonomy drift, primary and CE coverage, BOUND partition
coverage and overlap, exact divide dependency derivation, false ownership
between frame/range/arithmetic/fingerprint domains, prohibited saved-FLAGS,
IRET, BOUND, DIV/IDIV, policy, and production edits, residual ownership,
new/unenumerated failures, protected evidence, deterministic JSON/gzip, and
evidence-only final-commit scope.

All repository native/romless tests, upstream M58-M60c validation, milestone
validation, encoding, EOL, path-case, and documentation checks passed. No
required SST profile skipped.

The audit implementation at `f608c9ec902dda9270935977c04f86cb65b4dd74`
completed hosted [build 30153158686](https://github.com/nakatamaho/vaeg/actions/runs/30153158686)
successfully. Later hosted runs exposed only the two fail-closed validator
ordering defects described above; their unit-test logs were retrieved before
the local corrections. No CPU, build, or SST result failed. The final
candidate hosted run is started only after the evidence-only commit containing
this report is pushed, so its GitHub-assigned URL and conclusion are supplied
in the maintainer handoff rather than self-referenced here.

Principal evaluated commands, all with exit status zero:

```text
cmake --preset linux-ci-gcc -B build/linux-ci-gcc \
  -DVAEG_SSTS_V20_ROOT=<verified-corpus>
cmake --build --preset linux-ci-gcc --target vaeg_sdl2
ctest --test-dir build/linux-ci-gcc --output-on-failure

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope ci --profile architectural --policy g60b \
  --output <raw-ci> --failure-directory <raw-ci-sidecars>

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope full --profile architectural --policy g60b \
  --output <raw-full> --failure-directory <raw-full-sidecars>

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope full --profile fingerprint --policy g60b \
  --output <raw-fingerprint> \
  --failure-directory <raw-fingerprint-sidecars>

python3 tools/qa/upd9002_m60d_frame_audit.py selftest
python3 tools/qa/upd9002_m60d_frame_audit.py regenerate-twice \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --architectural-ci-raw <raw-ci> \
  --architectural-full-raw <raw-full> \
  --fingerprint-full-raw <raw-fingerprint> \
  --evaluated-sha ada55de79751c04e44d02abf7ecd6851b55c9763
python3 tools/qa/upd9002_m60d_frame_audit.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --check
```

Human verification from a clean candidate checkout:

```text
python3 tools/qa/upd9002_m60d_frame_audit.py selftest
python3 tools/qa/upd9002_m60d_frame_audit.py verify-static --root .
python3 tools/qa/upd9002_m60c_erratum.py verify-static --root .
python3 tools/qa/upd9002_m60c_audit.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
ctest --test-dir build/linux-ci-gcc --output-on-failure
git diff --exit-code \
  e425e55fc17117000ba5178a796de4444d897234...ada55de79751c04e44d02abf7ecd6851b55c9763 \
  -- cpu/upd9002/
git diff --check
```

## Known limitations

- This is evidence about the verified V20 SST population under the approved
  target policy, not complete uPD9002 silicon validation.
- Remaining BOUND range failures are not corrected or reclassified.
- Remaining DIV/IDIV arithmetic and pre-event FLAGS behavior is outside M60d.
- IRET is not audited or changed by M60d; it remains M60e work.
- Diagnostic full-FLAGS fingerprint mismatches do not weaken the architectural
  gate and are not inferred to be frame defects.
- Deterministic gzip identity is bounded to the recorded environment.
- The report's containing evidence commit SHA is necessarily supplied by the
  branch handoff rather than self-referenced inside the commit.

G60d remains unapproved pending human review. M60e is untouched.
