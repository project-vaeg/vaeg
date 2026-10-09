# M88 - Final VA-only source-tree audit

M88 performs the final audit of the VA-only active source tree after the BASIC,
SCSI, uPD9002 emulation-mode authority, I/O, BIOS, FDC subsystem CPU,
`cpucva`, state-save, machine-core relocation, and legacy tool/ROM
regeneration milestones. The M81 report records the completed LIO disposition.

Predecessor: approved G87.

Branch: `topic/m88-final-va-only-source-tree-audit`

Commit prefix: `M88:`

Candidate gate: `G88`

Report: `docs/agents/reports/m88_final_va_only_source_tree_audit.md`

Do not start M89. Do not merge M88 to `main` before G88 approval. Do not
declare G88 passed.

## Scope

M88 must:

- prove the active tree is organized around PC-88VA, not general PC-98/98x1
  compatibility;
- list all retained legacy-looking files and the active VA dependency for
  each;
- verify that `cbus/` retains only VA-supported expansion-board paths or
  explicitly deferred evidence gaps;
- verify that `cpu/upd9002/` and `cpu/upd780/` have clear ownership;
- verify that the M76 uPD9002 emulation-mode authority conclusion and the FDC
  subsystem uPD780-compatible CPU ownership remain distinct;
- verify that `machine/` has clear ownership for reset, events, timing,
  calendar, keyboard state, and state save/load;
- verify that state-save, HOSTFAT, SASI/SCSI, FDD, display, sound, keyboard,
  mouse, and manual runtime gates still pass.

## Non-goals

M88 must not implement new hardware behavior. Any remaining evidence gap must
be reported as backlog rather than silently removed.

## Validation

Run the full repository invariant checks, normal builds, native tests,
available platform builds, save/load checks, hosted CI, and the standard human
VA gate.
