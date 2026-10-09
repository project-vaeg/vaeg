# M73 - uPD9002 post-M49 runtime performance regression isolation

M73 isolates and, if evidence permits, corrects the runtime performance
regression that the maintainer observed between the approved M49 and M50
checkpoints.

Predecessor: approved G72 at
`643d9f7289d817c67f343bf01be368b546bc1438`.

Branch: `topic/m73-upd9002-post-m49-performance-regression`

Commit prefix: `M73:`

Candidate gate: `G73`

Report: `docs/agents/reports/m73_upd9002_post_m49_performance_regression.md`

Do not start a dependent follow-on milestone. Do not merge M73 to `main`
before G73 approval. Do not declare G73 passed.

## Scope

M73 owns only the post-M49 runtime performance regression. The initial
maintainer-observed boundary is:

```text
M49 checkpoint: runtime OK
M50 checkpoint: runtime slow
```

The first suspected production change is the M50 replacement of the former
`8E /r MOV Sreg,r/m16` implementation with the reserved-instruction path.
M73 must verify the boundary instead of assuming the cause.

M73 must:

- resolve the exact approved M49 and M50 SHAs from canonical repository
  evidence;
- build comparable M49, M50, current predecessor, and diagnostic workers with
  matching compiler, target ABI, build type, ROM-less options, and runtime
  configuration;
- reproduce the slowdown with a deterministic maintainer-usable runtime
  scenario, including the PC-88VA demonstration workload when available;
- collect instruction-dispatch evidence sufficient to compare at least
  `8E`, `0F`, reserved-instruction hits, interrupt/exception paths, frame
  pacing, and guest progress;
- create disposable diagnostic builds for the minimal suspected M50 hunks,
  starting with an `8E` restoration control and, if needed, an `0F` control;
- determine whether the M50 slowdown is caused by a wrong guest-visible
  instruction-path change, a host-side performance regression, or an unrelated
  workload/configuration issue;
- implement the smallest production correction only when the root cause is
  proven and the correction is within M73 scope;
- preserve all approved uPD9002 semantic gates and later protected behavior.

## Non-goals

M73 must not:

- start the VA1 N88 BASIC V3 command-hang investigation;
- implement SCSI support;
- implement uPD9002 uPD780 emulation mode;
- move `iova/*`, `io/*`, BIOS, CPU, or machine-core files;
- change SST fixtures, corpus records, comparison contracts, or target-policy
  records merely to hide a performance or semantic difference;
- reintroduce generic 286 protected-mode support;
- restore broad V20, V30, i286, i386, FS, or GS compatibility;
- weaken M68 mapped-memory dispatch, M69 IDP status composition, M70
  prefix/string closure, M71 dispatch folding, or M72 cleanup protections.

## Required evidence

The report must include:

- starting, M49, M50, evaluated, final local, upstream, and remote SHAs;
- exact M49 and M50 commit list relevant to production execution;
- source hunks by suspected runtime-impact selector;
- worker SHA-256 values and build configurations;
- runtime scenario, configuration, elapsed-time/FPS results, and host machine
  observations;
- opcode or dispatch histogram evidence, including `8E`, `0F`, reserved hits,
  and interrupt/exception paths;
- diagnostic build diffs and results;
- root cause, or an explicit blocker if the cause remains unproven;
- the smallest proposed or implemented correction;
- proof that no unrelated behavior was changed.

## Validation

Run the smallest focused validation needed for each diagnostic build, then run
the complete protected validation required by the final correction. At minimum,
the final candidate must run:

- repository invariant checks;
- `git diff --check`;
- normal production build;
- native non-external tests;
- M68 mapped-memory protection;
- M69 IDP status-composition protection;
- M70 prefix/string protection;
- M71 dispatch-fold protection;
- M72 cleanup protection;
- relevant uPD9002 SST identity or campaign checks required by the correction;
- the maintainer runtime performance scenario;
- hosted CI against the exact final candidate.

If the root cause is outside M73's safe correction scope, M73 may close with a
bounded regression report and a proposed follow-on milestone instead of a
production fix.
