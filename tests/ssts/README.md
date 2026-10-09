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
# SingleStepTests V20 comparison data

The uPD9002 comparison uses the MIT-licensed `v1_native` corpus from
<https://github.com/SingleStepTests/v20> at the exact commit recorded in
`v20_dataset_manifest.json`. The upstream corpus is external test data and is
not included in vaeg source or release archives.

Acquire the approved checkout into an external cache:

```sh
python3 tools/qa/upd9002_ssts.py acquire --cache-root /path/to/cache
```

Verify every compressed corpus shard, the metadata, and the license before use:

```sh
python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /path/to/cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json
```

The manifest records SHA-256 identities at three levels: canonical test
records, ordered opcode/form test sets, and the complete dataset. README and
metadata version strings are informational and are deliberately excluded from
the dataset identity. Any missing file, changed digest, unknown metadata status,
or unknown schema field fails closed.

The adapter follows the pinned OUTS fixture convention: a non-DS segment
override may leave the source byte at its DS-relative `initial.ram` address
while the expected MEMR cycle identifies the effective overridden address.
The byte is mirrored only when the expected cycle and default source agree;
missing or conflicting evidence fails closed. Synchronous interrupt results
come from a test-only event seam, rather than inference from final CS:IP, so
software `INT 00`, DIV/IDIV type 0, and ordinary arrival at the IVT0 target
remain distinct.

Configure a test build with the verified checkout to enable the external CI
comparison in CTest:

```sh
cmake -S . -B build/m43-ssts -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_ENABLE_ARCHIVE_DROP=OFF \
  -DVAEG_SSTS_V20_ROOT=/path/to/cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build build/m43-ssts
ctest --test-dir build/m43-ssts --output-on-failure \
  -R vaeg_upd9002_ssts
```

Without `VAEG_SSTS_V20_ROOT`, ordinary hosted builds run the synthetic
fail-closed selftest and verify the committed summaries, while the external
comparison appears as an explicit CTest skip. Such a skip does not satisfy a
human M43 or later uPD9002-series gate.

The CI profile takes the first 500 empty-prefetch-queue records per resolved
opcode/form after sorting by the stable upstream test hash. The full profile
uses every empty-queue record. Run the full gate profile with:

```sh
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /path/to/cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m42.csv \
  --worker build/m43-ssts/sdl2/vaeg \
  --profile full \
  --shard-timeout 300 \
  --output /tmp/v20_native_full.json \
  --failure-directory /tmp/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
```

The result summaries and gzip-compressed failure sidecars under
`tests/ssts/baseline/` are machine-readable. Each sidecar is verified through
the SHA-256 of its uncompressed canonical JSON, so gzip implementation details
are not part of the semantic baseline. Print a compact verified summary with:

```sh
python3 tools/qa/upd9002_ssts.py report \
  --summary tests/ssts/baseline/v20_native_full.json
```

`v20_native_g43_transition.json` records every semantic-failure removal and
signature change made while resolving the G43 adapter audit. It also proves
that the known-gap selector file and all 68,626 resolved gap hashes remained
unchanged. The `transition` subcommand regenerates this artifact when supplied
the preserved pre-correction and corrected summaries and sidecars.

## G58 immutable ratchet and profiles

M58 does not alter the M43 summaries or sidecars. Their raw-byte digests,
canonical sidecar-content digests, failure-index digests, and selected,
applicable, pass, failure, and classification hash-set digests are fixed in
`epochs/g43/manifest.json`. Verify the immutable references, contracts, gap
taxonomy, registries, and any committed G58 artifacts with:

```sh
python3 tools/qa/upd9002_ssts_ratchet.py verify-static --root .
python3 tools/qa/upd9002_ssts_ratchet.py selftest
python3 tools/qa/milestone_ids.py --root . --selftest --discover --audit
```

The blocking architectural comparison preserves the M43 metadata-defined
FLAGS mask and compares final registers, SST-represented RAM, I/O events, and
architectural termination. The diagnostic fingerprint comparison changes only
the FLAGS comparison to all 16 bits. Cycles, prefetch, and bus timing are
excluded from both contracts. The exact contracts are under `contracts/`, and
the versioned structural schema is in `schema/scoreboard-v1.md`.

Generate transient raw results with the current M48 support map. Use
`--flags-comparison all16` only for the fingerprint full profile:

```sh
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /path/to/pinned-v20 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m48.csv \
  --worker build/ssts/sdl2/vaeg \
  --profile full \
  --flags-comparison all16 \
  --output /tmp/v20_fingerprint_full.json \
  --failure-directory /tmp/v20_fingerprint_full_failures
```

`upd9002_ssts_ratchet.py generate` converts one raw result into exactly one
canonical artifact family. `ratchet` requires
`--predecessor-sha 72322d5c9b8e40e4a988312aebe163a8190e2aa5`; omission,
substitution, self-comparison, a new failing hash, a changed signature,
per-form pass regression, timeout/crash, identity drift, or unapproved
classification transition fails closed.

The taxonomy file annotates each immutable M43 known-gap rule by its selector
and resolved-set content digests. Opcode 63 is
`documented_silicon_absent`; the other 39 rules are
`implementation_missing`. There are no `target_support_unverified` rules at
G58, so `hardware_pending.json` is intentionally empty. That registry is
orthogonal evidence and can never alter classification or the applicable
denominator.

## G59 semantic evidence pack

M59 replays six explicitly bounded instruction populations without changing
the worker, comparison contracts, fixtures, classifications, or CPU
implementation. It records initial, expected, and actual architectural state
in deterministic case tables and keeps architectural and all-16-bit FLAGS
fingerprint observations separate.

Generate the pack only from the exact M59 analysis implementation commit:

```sh
python3 tools/qa/upd9002_semantics_evidence.py generate \
  --root . \
  --dataset-root /path/to/pinned-v20 \
  --worker build/ssts/sdl2/vaeg \
  --analysis-evaluated-sha 40-hex-implementation-commit \
  --output /tmp/g59
```

Validate the generator and a committed pack with:

```sh
python3 tools/qa/upd9002_semantics_evidence.py selftest
python3 tools/qa/upd9002_semantics_evidence.py verify-static --root .
```

The canonical format and bounded same-environment gzip reproducibility claim
are defined in `schema/evidence-pack-v1.md`. Diagnostic replay of a known gap
does not change its G58 classification, applicable denominator, or official
skip outcome.

## G60a FLAGS-materialization transition

M60a uses `tools/qa/upd9002_m60a_evidence.py` to bind each candidate
scoreboard and transition explicitly to approved G59 commit
`e7f2325bc81310532091a8ca82914030fdb8b6ba`. The driver keeps the M58
fail-closed defaults, but permits changed failure signatures only when it can
write and verify their complete deterministic shards. It never approves those
changes.

The G60a families are:

```text
scoreboard/g60a_architectural_ci.json
scoreboard/g60a_architectural_full.json
scoreboard/g60a_fingerprint_full.json
transitions/g60a_architectural_ci_from_g59.json
transitions/g60a_architectural_full_from_g59.json
transitions/g60a_flags_materialization_summary.json
```

Run the positive/fail-closed and committed-artifact checks with:

```sh
python3 tools/qa/upd9002_m60a_evidence.py selftest
python3 tools/qa/upd9002_m60a_evidence.py verify-static \
  --root . \
  --predecessor-sha e7f2325bc81310532091a8ca82914030fdb8b6ba
```

The focused summary proves the direct FLAGS-family transition separately from
dependent interrupt-frame effects in BOUND and divide-error populations.
PUSHF's pre-existing segment-wrap anomaly remains blocking and is not
reclassified.

## G60d synchronous interrupt-frame closure

M60d uses `tools/qa/upd9002_m60d_frame_audit.py` to replay the complete CC,
CD, CE, and BOUND populations together with the exact 214-hash M60a
divide-exception dependency set. The audit compares event classification,
termination, stack addresses, saved IP/CS/FLAGS, vector fetch, final CS:IP,
post-entry TF/IF, and final represented RAM. It keeps BOUND range decisions
and DIV/IDIV arithmetic outside frame ownership.

Run the positive/fail-closed and committed-artifact checks with:

```sh
python3 tools/qa/upd9002_m60d_frame_audit.py selftest
python3 tools/qa/upd9002_m60d_frame_audit.py verify-static --root .
```

The versioned format and Path A closure requirements are defined in
`schema/synchronous-frame-audit-v1.md`. An evidence-only closure requires
zero frame residuals and exact G60c pass, failure, signature, selected, and
applicable identities.
