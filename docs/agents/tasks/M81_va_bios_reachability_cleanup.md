# M81 - Audit VA BIOS reachability and remove 98-only BIOS handlers

M81 audits `bios/` and related VA BIOS hooks, then removes only BIOS handlers
that are proven 98-only and unreachable in the active VA product.

Predecessor: approved G80.

Branch: `topic/m81-va-bios-reachability-cleanup`

Commit prefix: `M81:`

Candidate gate: `G81`

Report: `docs/agents/reports/m81_va_bios_reachability_cleanup.md`

Do not start M82. Do not merge M81 to `main` before G81 approval. Do not
declare G81 passed.

## Scope

M81 must:

- audit BIOS entry points reached by PC-88VA boot, DOS, demo/game software,
  FDC, SASI/SCSI, HOSTFAT, display, sound, keyboard, mouse, and save/load;
- preserve VA-reachable BIOS hooks;
- remove only handlers proven 98-only and inactive;
- document any retained legacy-looking BIOS code with its active dependency.

## Non-goals

M81 must not change CPU instruction semantics, I/O dispatcher structure, or
FDC subsystem CPU organization.

## Validation

Run repository checks, normal builds, native tests, BIOS-focused smoke tests
where available, save/load checks, and manual VA boot/application gates.
