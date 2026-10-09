# M65 — target-correct residue re-plan

M65 is complete and pushed as an unapproved G65 candidate pending human review.
No generated M65a-or-later task has been started; M66a, M66b, and M67 remain
unstarted.

## Gate and identity

The fixed predecessor is G64, SHA
`9b151923f9468555043152ffe8651c97b9ecac5b`, branch
`topic/m64-upd9002-div-idiv`, evaluated worker
`99c6388df903dfc69432730cc9fa908a83946774`, and hosted CI
[30223337112](https://github.com/nakatamaho/vaeg/actions/runs/30223337112).
M65 uses branch `topic/m65-upd9002-residue-plan`. The audit/planning commits
are `4a72102` and `f5f21c7`; the final evidence candidate SHA is
`5a540a9cdc853a9789c27eaa6f68aac6e2783f82`, and the approved CI head SHA is
the same. The dedicated worktree was based directly on the fixed
G64 SHA; the unrelated primary worktree was not modified.

The verified dataset is
`ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4`.
Architectural contract: `upd9002-v20-architectural-v1`, SHA
`aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5`.
Fingerprint contract: `upd9002-v20-fingerprint-v1`, SHA
`47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e`.
The G64 target policy and all selected/applicable identities are unchanged.

## G64 reuse and residue

The committed G64 evidence was verified without a full profile rerun because
the worker, dataset, contracts, policy, selected/applicable sets, raw results,
sidecars, and ranking are byte- and identity-bound and unchanged. G64 remains
180,000 selected / 169,300 applicable CI (168,531 pass, 769 fail) and
1,562,502 selected / 1,474,594 applicable full (1,467,083 pass, 7,511 fail);
fingerprint full is 1,394,692 pass / 79,902 fail. Timeout and crash counts are
zero.

`tests/ssts/evidence/g65/architectural_residue.json` contains all 7,511 exact
failure rows. Ownership is disjoint and exact:

| owner | form | count |
| --- | --- | ---: |
| M65a | FF /7 | 5,000 |
| M65b | BOUND (`62`) | 1,244 |
| M65c | F7 /2 | 1,113 |
| M65d | FF /6 | 144 |
| M65e | exact ten-case tail (`61`×3, `81.6`, `FF.5`, `A5`, `9C`, `D1.6`, `C8`, `C4`) | 10 |
| **total** | | **7,511** |

The exact failure-set digest is recorded in the evidence manifest and
ownership manifest. No generic long-tail bucket remains.

## Implementation-missing inventory

The live G64 policy resolves exactly 5,908 `known_target_gap /
implementation_missing` hashes after excluding completed G62/G64 forms. The
complete selector rows, hashes, counts, and sorted-hash digests are in
`tests/ssts/evidence/g65/implementation_missing_inventory.json`. They are
owned by M65j only as a planning/re-decomposition inventory: it is not a
generic semantic implementation task and authorizes no CPU change. Each
selector row retains its own exact hashes, count, and digest in the inventory;
future semantic tasks must split those rows by independently reviewable
primitive. They are not mixed with applicable failures. The taxonomy
cross-check is documented/silicon-absent
32,000, implementation-missing 5,908, target-support-unverified 0.

## Zero coverage and authority work

BRKEM is explicitly deferred under the approved M64 amendment: metadata and
monitor authority are present, but `tests/ssts/evidence/g64/brkem/0fff_cases.json.gz`
has zero rows; selected and executed are both zero, implementation is false,
and passing is not claimed. M65g is a two-stage corpus/provenance/evidence
gate followed by conditional implementation. BRKFEM is separately evidence
only (M65h), and no RETEM/CALLN implementation is implied. 6C–6F remain
target-absent and outside the blocking denominator; M65f separates reserved
behavior evidence from any later cleanup. 66/67/FPO2 remains the approved
M60c underdetermined monitor-authority result, not an implementation task by
metadata inference.

## Generated plan and protections

Generated tasks M65a–M65m are in `docs/agents/tasks/`; their dependency graph,
schedule, ownership, and coverage manifests are under `tests/ssts/plans/g65/`.
The graph records prerequisites, consumers, shared-risk boundaries, policy and
hardware/corpus conditions, and human gates. No task overlaps another owner;
unowned applicable failures, unowned implementation-missing hashes, and
unexplained zero-coverage authority items are empty. M66a/M66b/M67 identifiers
were not renumbered.

Protected G60b–G64 artifacts, target policy, comparison contracts, fixtures,
and all CPU sources are unchanged. `git diff --exit-code G64...HEAD --
cpu/upd9002/` is clean. M65 makes no production semantic change.

## Validation and limitations

Milestone discovery/selftests, G64 static/evidence/policy verification,
protected predecessor validators, documentation, encoding, EOL, path-case,
native non-external checks, M65 selftest, ownership arithmetic, and
deterministic regeneration passed. Regenerating the evidence family twice in
the pinned environment produced byte-identical files. Full G64 profiles were
not rerun under the identity-bound reuse rule. The evidence uses the committed
G64 scoreboard state; any deeper expected/actual execution telemetry remains
the responsibility of each generated task.

Human review commands:

```sh
python3 tools/qa/upd9002_m65_residue_replan.py selftest
python3 tools/qa/milestone_ids.py --selftest --audit --discover
git diff --check
git diff G64_SHA...HEAD -- cpu/upd9002/
```

M65 makes no production semantic change. Every remaining G64 architectural
failure and every remaining implementation-missing hash has one exact
non-overlapping owner or a precise prerequisite evidence task. BRKEM remains
deferred under the approved M64 scope amendment.
