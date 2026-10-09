# M79 - Consolidate the VA I/O dispatcher

M79 makes the VA I/O dispatcher the canonical active I/O dispatcher and
removes the unnecessary `iocore` / `iocoreva` split where evidence permits.

Predecessor: approved G78.

Branch: `topic/m79-va-io-dispatcher-consolidation`

Commit prefix: `M79:`

Candidate gate: `G79`

Report: `docs/agents/reports/m79_va_io_dispatcher_consolidation.md`

Do not start M80. Do not merge M79 to `main` before G79 approval. Do not
declare G79 passed.

## Scope

M79 owns dispatcher structure only:

- audit `iomode_va`, `iocore_*`, and the moved VA port table;
- make the active VA I/O routing canonical;
- preserve all active VA devices, C-bus board routes, HOSTFAT, SASI, SCSI,
  keyboard, mouse, sound, display, FDC, DMA, PIC, PIT, and state behavior;
- remove duplicate dispatcher structure only when behavior-neutral.

## Non-goals

M79 must not delete 98-only device implementations merely because they remain
compiled. That cleanup is M80.

## Validation

Run focused I/O dispatcher tests if available, repository checks, normal
builds, native tests, save/load checks, and manual VA boot/device validation.
