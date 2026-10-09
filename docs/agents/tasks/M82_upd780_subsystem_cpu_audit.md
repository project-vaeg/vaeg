# M82 - Audit the FDC subsystem uPD780-compatible CPU

M82 audits the FDC subsystem CPU path and documents the replacement boundary
for `cpucva/z80_compat_cpu.cpp` as a uPD780-compatible FDC subsystem CPU, not as a
generic Z80 core.

Predecessor: approved G81.

Branch: `topic/m82-upd780-subsystem-cpu-audit`

Commit prefix: `M82:`

Candidate gate: `G82`

Report: `docs/agents/reports/m82_upd780_subsystem_cpu_audit.md`

Do not start M83. Do not merge M82 to `main` before G82 approval. Do not
declare G82 passed.

## Scope

M82 must:

- audit `cpucva/z80_compat_cpu.cpp`, `cpucva/upd780_disasm.cpp`, legacy-state codec,
  `subsystem`, `subsystemif`, and `fdsubsys` consumers;
- identify the current FDC uPD780-compatible CPU contract;
- distinguish FDC subsystem uPD780 behavior from any future main-CPU
  emulation-mode work;
- define the exact files to move under `cpu/upd780/` in M83;
- preserve the suzukiplan-backed wrapper behavior.

## Non-goals

M82 must not move files or change production CPU behavior except for minimal
audit instrumentation disabled in production if absolutely required.

## Validation

Run Z80/uPD780 wrapper tests, FDC subsystem tests, repository checks, builds,
and manual FDD boot/access gates.
