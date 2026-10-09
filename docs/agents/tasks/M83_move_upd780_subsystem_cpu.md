# M83 - Move the FDC subsystem CPU to cpu/upd780

M83 creates `cpu/upd780/` and moves the FDC subsystem uPD780-compatible CPU
wrapper/backend there.

Predecessor: approved G82.

Branch: `topic/m83-move-upd780-subsystem-cpu`

Commit prefix: `M83:`

Candidate gate: `G83`

Report: `docs/agents/reports/m83_move_upd780_subsystem_cpu.md`

Do not start M84. Do not merge M83 to `main` before G83 approval. Do not
declare G83 passed.

## Scope

M83 owns the move from `cpucva/` to `cpu/upd780/` for the FDC subsystem CPU
files identified by M82.

Use rename-only commits for moves and separate reference-fixup commits.

The final tree should make the CPU roles explicit:

```text
cpu/upd9002/   main uPD9002 instruction core
cpu/upd780/    FDC subsystem uPD780-compatible CPU wrapper/backend
```

## Non-goals

M83 must not implement a new CPU core, change FDC semantics, move unrelated
VA memory code, or rename the FDC subsystem protocol beyond path-accurate
documentation.

## Validation

Run repository checks, builds, uPD780/Z80 wrapper tests, FDC subsystem tests,
save/load checks, and manual FDD boot/access gates.
