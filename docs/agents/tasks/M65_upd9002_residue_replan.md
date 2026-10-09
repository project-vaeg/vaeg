<!-- Copyright (c) 2026 Nakata Maho -->
# M65 — Re-plan the complete target-correct residue after G64

M65 is planning and evidence only. It uses approved G64
`9b151923f9468555043152ffe8651c97b9ecac5b` and changes no production CPU,
fixtures, contracts, target policy, selected/applicable sets, classifications,
or taxonomy. No generated M65a-or-later task starts here.

Branch: `topic/m65-upd9002-residue-plan`; commit prefix: `M65:`; gate: `G65`;
report: `docs/agents/reports/m65_upd9002_residue_replan.md`.

Re-enumerate every exact G64 architectural failure (7,511 hashes) and every
live `implementation_missing` hash (5,908 hashes), with pairwise-disjoint
owners, selectors, sorted-hash digests, evidence, prerequisites, and human
gates. Completed G62/G64 families are regression dependencies only: AAM,
ROL4/ROR4, BCD/ASCII adjust, shifts, DIV/IDIV, ADD4S/SUB4S/CMP4S, and
TEST1/CLR1/SET1/NOT1.

BRKEM `0F FF imm8` has metadata and monitor authority but no `0fff.json.gz`
corpus shard: selected=0, executed=0, implemented=false, and passing is not
claimed. It is deferred to a two-stage corpus/evidence approval then
conditional implementation workflow. This deferral applies only to BRKEM.
BRKFEM `0F FE imm8` is a separate evidence task; neither is implemented here.

Generate executable future tasks M65a, M65b, … for FF `/7`, BOUND residual,
F7 `/2`, FF `/6`, the exact ten-case tail, 6C–6F reserved behavior, BRKEM,
BRKFEM, 66/67/FPO2, remaining NEC 0F, reserved-opcode policy, prefix/restart
evidence, and fingerprint-only diagnostics. Use the domains
`applicable_semantic_failure`, `implementation_missing_with_executable_corpus`,
`target_authority_evidence_required`, `corpus_required_before_implementation`,
`production_cleanup_only`, `reserved_opcode_policy`,
`prefix_or_restart_semantics`, `diagnostic_only`, and `no_further_action`.

Architectural owners must union exactly to 7,511; implementation-gap owners
must union exactly to 5,908; zero-coverage authority items must not gain fake
hashes. Unowned failure/gap/authority sets must be empty. Preserve M66a,
M66b, and M67 identifiers unchanged. Stop at G65 human review.

## Campaign execution amendment

Under the maintainer-approved terminal-only protocol, M65j is the first
internal checkpoint and performs selector/hash decomposition. M65a through
M65m then execute serially on `topic/m65-residue-campaign`; intermediate
checkpoints are technically validated but not independently approved. Only
G65m is a formal approval gate, and M66a requires approved G65m.
