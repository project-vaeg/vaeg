# P13 — Certified numerical oracles and bounded transcendental prototypes

> STAGE B ONLY. Do not begin this package during acquisition or while awaiting user OCR.
> The section 0 handoff gate in [goal-contract.md](../goal-contract.md) is mandatory.

**Initial status:** NOT_STARTED. This document is an implementation task, not a PASS record.
**Prerequisites:** P01, P02, P03, P09.
**Authority:** [goal-contract.md](../goal-contract.md), relevant normative companions, and
[work-plan.md](../work-plan.md). Update actual execution state in [progress](../records/progress.md).

## Entry preparation

Read the prerequisite evidence and inspect the current diff. Record the exact production/test
files to change, active source digest, and gate command mapping before edits. Names of runtime
files must come from the repository audit, not from invented paths in this task document.
If equivalent work already exists, test and repair it rather than implement it twice.

## Work

Follow reference-validation.md: E8087 is an optional corroborating black-box reference,
not a mandatory numerical oracle or a replacement for certified MPFR/integer references.
A linked E8087 harness is not assumed to be the same .COM as the hardware guest test.
Record unavailable external references as NOT_RUN; never fabricate comparison results.


Implement the external-only generator/certificate path and prototype numerical kernels before merging production transcendental handlers. Resolve domain endpoints, FPATAN operand order, FPTAN pair constraints, PC/RC, and known special cases from original sources.

Compare a small number of bounded algorithms with sufficient guard precision, cancellation-safe evaluation, and exponent scaling. Freeze exact constants, corpus seeds/counts, target precision, pair policy, error metric, and algorithm iteration bounds in records/numerical-design.md.

## Acceptance

Certify oracle intervals with directed rounding, not repeated-digit stability. Prove exact raw-input conversion and guest-format exponent/subnormal quantization. Test the generator with exact identities and difficult rounding cases, and verify it rejects an intentionally insufficient certificate.

Run prototype corpora at the floors in numerics-and-oracles.md, measuring worst errors and termination. P13 passes only with a viable measured algorithm and reference data, not an aspirational design document. No production handler is called COMPLETE_TESTED here.

Run the package gate after implementation, plus impacted baseline tests:

```sh
python3 tools/8087/gate.py --package P13
```

The runner is created in P00; it is not distributed as a fake passing runner with this pack.
A changed prerequisite invalidates dependent evidence until its relevant tests are rerun.

## Required outputs

External generator source/lock/commands, independent certified corpora, exact constants, bounded prototype report and frozen numerical policies.

## Boundaries and failure handling

No runtime oracle linkage, host double detour, arbitrary raw FPTAN pair inferred from ratio alone, or weakened tolerance. A failed prototype triggers a different algorithm/guard precision before escalating a blocker.

On failure, preserve work, record the exact failing case and commands, repair and rerun. Do
not weaken the gate. Continue independent unblocked packages when one needs external input.
For rollback, revert only this package's own changes using reviewed patches; never discard
unrelated dirty files or rewrite history. Record local commits only when permitted.
