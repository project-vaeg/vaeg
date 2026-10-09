# M85 - State-save section cleanup and compatibility report

M85 audits and cleans state-save sections after the VA-only source-tree
consolidation milestones have closed active dependencies.

Predecessor: approved G84.

Branch: `topic/m85-state-save-section-cleanup`

Commit prefix: `M85:`

Candidate gate: `G85`

Report: `docs/agents/reports/m85_state_save_section_cleanup.md`

Do not start M86. Do not merge M85 to `main` before G85 approval. Do not
declare G85 passed.

## Scope

M85 must:

- inventory every state-save section remaining from retired 98-only code;
- preserve required VA sections;
- define compatibility behavior for removed sections;
- preserve HOSTFAT identity checks and current VA save/load behavior;
- document any incompatible old-state rejection intentionally introduced by
  approved cleanup.

## Non-goals

M85 must not use state cleanup to hide behavior changes from earlier
milestones.

## Validation

Run save/load round-trip tests, incompatible-state negative tests where
required, repository checks, builds, native tests, and manual save/load gates.
