<!-- Copyright (c) 2026 Nakata Maho -->

# uPD9002 synchronous interrupt-frame audit schema v1

License: BSD-2-Clause.

This schema defines the deterministic M60d audit of synchronous interrupt
entry. It separates frame observables from BOUND range decisions, DIV/IDIV
arithmetic, final architectural FLAGS, and full FLAGS fingerprint results.
The audit is evidence about the approved worker; it does not broaden M60d
semantic ownership.

## Identity

Every G60d manifest records:

- schema and schema version;
- milestone, candidate gate, approved predecessor gate and SHA;
- the audit implementation/evaluated commit SHA;
- dataset, comparison-contract, target-policy, selected-set, and
  applicable-set identities;
- the approved primary frame, BOUND partition, and derived divide-exception
  dependency digests;
- the outcome, semantic-change boolean, residual-frame count and digest;
- every artifact path, byte count, row count, and SHA-256 digest.

Canonical JSON uses sorted keys, compact separators, LF, lowercase fixed-width
hexadecimal, and no timestamp. Case tables and failure shards use the
repository deterministic gzip writer.

## Case rows

The complete case table is keyed and ordered by the SHA-256 of each canonical
SST record. Each row records the structural form, approved population roles,
instruction bytes, initial state, expected and actual state, expected and
actual event classification, termination, vector, frame addresses and bytes,
post-entry TF/IF state, mismatch kinds, frame-specific residual reasons, and
the conclusion.

The table covers all 5,000 records for each of CC, CD, CE, and BOUND, plus the
exact 214-hash divide-exception dependency population. CE is partitioned into
taken and non-taken cases. BOUND ownership is partitioned into the exact
former frame-only, range/non-frame residual, and previously passing normal
sets. The partitions are disjoint and complete.

## Conditional outcome

`evidence_only_closure` is valid only when every approved frame population is
complete, every expected synchronous event has identical frame observables,
non-taken CE has no event, the 214 divide-exception dependencies remain green,
and the global architectural-failure scan finds no unexplained in-scope frame
signature. It requires:

```text
semantic_change = false
residual_frame_count = 0
newly_passing = empty
newly_failing = empty
changed_failure_count = 0
```

A nonempty residual is rejected by the Path A generator. Any future Path B
correction requires an immutable pre-fix residual artifact and a separately
bounded semantic commit as specified by the M60d task.

## Protected domains

M60d preserves the G60c target policy, dataset, comparison contracts, selected
and applicable sets, classifications, taxonomy, registries, fixtures, and
protected G43/G58/G59/G60a/G60b/G60c artifacts. Fingerprint-only mismatches
unrelated to post-entry TF/IF or guest-visible frame bytes cannot be labelled
frame residuals.
