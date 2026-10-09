# M84 - Clean up the remaining cpucva boundary

M84 cleans up the remaining `cpucva/` boundary after the FDC subsystem CPU has
moved to `cpu/upd780/`.

Predecessor: approved G83.

Branch: `topic/m84-cpucva-boundary-cleanup`

Commit prefix: `M84:`

Candidate gate: `G84`

Report: `docs/agents/reports/m84_cpucva_boundary_cleanup.md`

Do not start M85. Do not merge M84 to `main` before G84 approval. Do not
declare G84 passed.

## Scope

M84 must:

- audit remaining `cpucva/` contents;
- decide whether VA memory belongs under a future `memory/`, `va/`, or other
  active-tree location;
- keep `cpu/upd9002/` focused on instruction execution rather than platform
  memory mapping;
- preserve the M68 mapped-memory dispatcher boundary;
- move or retire `cpucva/` only when the active dependencies are closed.

## Non-goals

M84 must not change uPD9002 instruction semantics, FDC uPD780 behavior, I/O
dispatcher behavior, or state-save compatibility without a focused approval.

## Validation

Run repository checks, builds, native tests, M68/M69/M70/M71 protections,
save/load checks, and manual VA boot/application gates.
