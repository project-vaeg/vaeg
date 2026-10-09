# M78 - Normalize references after the VA I/O move

M78 normalizes include paths, CMake entries, documentation references, and
repository-local path assumptions after M77 moves `iova/*` into `io/`.

Predecessor: approved G77.

Branch: `topic/m78-iova-io-reference-fixups`

Commit prefix: `M78:`

Candidate gate: `G78`

Report: `docs/agents/reports/m78_iova_io_reference_fixups.md`

Do not start M79. Do not merge M78 to `main` before G78 approval. Do not
declare G78 passed.

## Scope

M78 owns only reference normalization after the move:

- update include paths and build lists to the new `io/` locations;
- update current task/report references that describe active paths;
- leave approved historical reports intact except where repository policy
  requires a generated current-path view;
- preserve state-save section names and runtime behavior.

## Non-goals

M78 must not integrate the dispatcher, delete legacy devices, or rename
production symbols beyond path-derived include guards when required.

## Validation

Run repository invariant checks, normal builds, native tests, and VA boot
smoke/manual validation.
