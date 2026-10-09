# M43 — SingleStepTests V20 external semantic baseline

Derived from `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
(master SHA-256: `07c53e542adbe838de3d79999aa3d7acebf7a15f85f4d17aa0f5a5e50a293938`).

## Authoritative context

Before editing, read and obey:

1. `AGENTS.md`
2. `docs/agents/ROADMAP.md`
3. `docs/agents/CONVENTIONS.md`
4. `docs/agents/plans/upd9002-core-consolidation-M42-M49-v6.md`
5. This task file

The master plan is authoritative for shared preconditions, immutable baselines,
state compatibility, dispatch identity, regression traces, SingleStepTests V20
classification, execution protocol, and G42–G49 human-gate requirements. If this
extracted task conflicts with the master plan, stop and report the conflict; do
not improvise.

## Session boundary

Work only on **M43** in this Codex session. Do not begin the next
milestone, do not claim the human gate has passed, and stop after producing the
required report and final SHA. A failed baseline is evidence to investigate, not
permission to re-record a golden, expand a known-gap set, weaken an allowlist, or
change CPU semantics.

## Extracted task specification

Task: docs/agents/tasks/M43_upd9002_singlestep_v20_baseline.md

Goal:
Establish a reproducible, content-addressed comparison between the current
uPD9002/V52 native core and the hardware-generated SingleStepTests V20 native
suite. Add test infrastructure and baselines only; do not change CPU semantics,
dispatch, timing, state layout, or public names.

Prerequisites:
- G42 explicitly accepted.
- The M42 final graph, direct harness, trace, and support-map artifacts are green.
- Worktree clean.

Steps:
1. Inventory and reuse existing SSTS infrastructure:
   a. Search tests/, tools/qa/, CMake/ctest, prior reports, and baselines for an
      existing SingleStepTests adapter, .sst record format, dataset manifest,
      timeout runner, or failure-signature schema.
   b. Extend the existing design rather than creating a second framework.
   c. Record any migration needed to reach the contracts in this task; do not
      silently invalidate a previously accepted baseline.

2. Pin the upstream dataset:
   a. Source: https://github.com/SingleStepTests/v20, v1_native.
   b. Record the exact 40-hex commit, MIT attribution, README-declared version,
      `metadata.json`-declared version, metadata digest, and corpus/selection
      digests. The two version strings are informational and may differ.
   c. Never use a moving branch in a gate and never download unverified data as
      part of an ordinary test run.
   d. Provide an explicit acquisition/verification command and cache-root
      setting. A missing corpus may produce a clear external-data skip only in
      ordinary hosted CI where the task explicitly permits it. It is a hard gate
      failure at G43–G49 and in every milestone report's required local/human
      comparison. Digest mismatch is always a hard failure.

3. Define and test the content-addressed dataset contract:
   a. Canonical serialized record bytes and record_digest.
   b. Per-opcode/form opcode_testset_digest over sorted record digests.
   c. dataset_digest over metadata, all opcode-testset digests, and the selection
      policy.
   d. dataset_id derived from upstream commit identity, content digests, and
      policy version only. Exclude README and metadata version strings.
   e. Baseline comparison first requires exact dataset_id, dataset_digest,
      metadata digest, total, and per-opcode counts. Record both version strings
      as informational fields but never compare identity through them.

4. Create deterministic profiles:
   a. CI profile: empty-prefetch-queue records only; statuses allowed by the
      blocking policy; stable maximum 500 records per opcode/form selected by
      sorted upstream test hash, not idx. Commit
      tests/ssts/baseline/v20_native_ci.json.
   b. Full profile: every applicable empty-queue record. Commit the result
      summary to tests/ssts/baseline/v20_native_full.json; run locally/nightly or
      at the human gate if ordinary CI cannot carry the corpus cost.
   c. Prefetched records are reported separately and remain unsupported_fixture
      unless queue injection is proven faithful. Do not blend them into the
      required semantic profile.

5. Build the target applicability classifier from three independent inputs:
   a. SingleStepTests metadata status/arch/reg/flags-mask information.
   b. The M42 final dispatch graph and opcode/form support map.
   c. The maintainer-approved uPD9002/V52 target policy.
   Resolve every selected record to exactly one category: applicable,
   known_target_gap, expected_target_divergence, unsupported_fixture, or
   upstream_nonblocking. Fail closed on an unclassified record.
   d. Pin the parser to the exact metadata vocabulary observed at the selected
      commit: primary statuses `normal`, `prefix`, `fpu`, `extension`,
      `undocumented`, plus an explicit missing-status state; register/group
      statuses `undefined`, `alias`, and `undocumented`. Unknown values or
      shapes are hard failures. `extension` is structural, not automatically
      nonblocking, and `alias` requires exact canonical-form handling.
   e. Resolve every `0x0F xx` record by second opcode byte and structural form
      against the M42 map. The coarse primary `0x0F` metadata entry must never
      classify the complete extension space.

6. Commit tests/ssts/baseline/upd9002_v20_known_gaps.json:
   a. Record every V20/V30 opcode/form absent from the current uPD9002/V52 target
      that appears in the selected corpus.
   b. Each entry contains an exact structural selector, reason, evidence,
      upstream status/arch, resolved count, sorted test-hash digest, and first
      baseline.
   c. Treat the target's reduced instruction set as a known issue. Do not count
      these records as failures and do not implement the missing instructions.
   d. Forbid idx-based, blanket-all, or open-ended textual allowlists.
   e. Any expected_target_divergence is separate, exact, evidence-backed, and
      explicitly approved; do not disguise a core bug as a target gap.

7. Implement the direct comparison adapter:
   a. Initialize registers, segments, IP, FLAGS, 1 MiB RAM, and deterministic I/O
      from each record; execute the authoritative step primitive to the expected
      termination boundary.
   b. Compare final changed registers/RAM and defined FLAGS after metadata masks.
   c. Record Type-0 divide-interrupt behavior where applicable.
   d. Keep V20 maximum-mode cycle arrays and prefetch traces diagnostic only;
      do not make them V52 timing gates.
   e. Run opcode shards in child processes with deterministic timeout and report
      normal, type0, timeout, signal, crash, and halt termination classes.

8. Preserve upstream-specific edge cases in tests and report output:
   a. REPC/REPNC behavior for applicable string/I/O forms.
   b. REPE/REPNE CMPS operand access order.
   c. C0/C1 and D2/D3 counts that detect improper 5-bit masking.
   d. REP-prefixed IDIV with no V20 semantic effect.
   e. Normal and faulting DIV/IDIV paths and defined/undefined flag masking.

9. Define canonical failure signatures:
   a. Include test_hash, opcode/form, instruction bytes, termination class,
      mismatch kinds, register differences in fixed order, masked FLAGS,
      ascending-address memory differences, and ordered I/O differences.
   b. Use lowercase fixed-width hexadecimal, sorted/deduplicated mismatch kinds,
      canonical JSON or an equally specified serialization, and SHA-256.
   c. Store both readable content and signature digest. Never key approval only by
      idx or raw log text.

10. Record the current baseline without repairing the CPU:
    a. Exact dataset/category counts and resolved hash sets.
    b. Applicable pass and failure test-hash sets.
    c. Complete failure signatures and termination classes.
    d. Known gaps and any separately approved target divergences.
    e. The current core is not required to have zero failures. M44–M49 require
       exact equality because this series is structural and behavior-preserving.

11. Add ctest/CI integration:
    a. Required CI comparison when the verified CI dataset is available under the
       repository's external-data policy. A hosted-CI external-data skip must be
       explicit and visible, but it never satisfies G43 or any later human gate.
    b. A fast adapter selftest using tiny committed synthetic fixtures, always
       runnable without the external corpus.
    c. A full-profile command documented and executed for G43.
    d. A negative test proving dataset/digest mismatch fails before comparison.
    e. A negative test proving a newly unclassified record cannot be silently
       skipped.
    f. A report helper that prints the executed dataset_id, profile, total record
       count, per-category counts, and skip status in canonical form. Every
       M43–M49 milestone report embeds this output.
    g. Both CI and full profiles are mandatory at G43 and every later human gate;
       missing corpus or skip is a gate failure.

12. Prove M42 traces, direct-harness results, final graph, state fixtures, and
    all G41/M23 goldens are unchanged. Report final M43 SHA and stop. Do not
    create pre-upd9002-refactor inside the task. After G43 is explicitly
    accepted, the maintainer creates and pushes that tag at the accepted SHA.

Non-goals:
- No uPD9002/V52 instruction implementation.
- No timing or prefetch-queue emulation change.
- No conversion of V20-only behavior into target requirements.
- No state adapter, CPU_TYPE fold, handler deletion, or rename.

Gate:
- Exact upstream identity and content digests recorded; README and metadata
  version strings recorded separately as informational values.
- CI and full profiles reproducible from test hashes.
- Every selected record classified exactly once.
- Known-gap manifest explicitly approved and no gaps counted as failures.
- Failure signatures and timeout handling deterministic.
- Upstream edge-case fixtures exercised and reported.
- G41/M42 behavior artifacts unchanged.
- CI and full V20 comparisons completed without skip for G43, with executed
  dataset_id and record/category counts in the report; standard human gate green.
