# M86 - Move root machine-core sources under machine

M86 relocates active machine-core sources that still live in the repository
root into a dedicated `machine/` directory.

Predecessor: approved G85.

Branch: `topic/m86-machine-core-relocation`

Commit prefix: `M86:`

Candidate gate: `G86`

Report: `docs/agents/reports/m86_machine_core_relocation.md`

Do not start M87. Do not merge M86 to `main` before G86 approval. Do not
declare G86 passed.

## Scope

M86 owns source-tree layout only. It should move active root machine-core
files such as:

- `pccore.c` / `pccore.h`;
- `nevent.c` / `nevent.h`;
- `timing.c` / `timing.h`;
- `calendar.c` / `calendar.h`;
- `keystat.c` / `keystat.h` / `keystat.tbl`;
- `statsave.c` / `statsave.h` / `statsave.tbl`;
- `debugsub.c` / `debugsub.h`;
- `clockscale.h`.

Use rename-only commits for file moves and separate commits for CMake,
include-path, documentation, and validation fixups.

## Deferred files

Do not move these files in M86 unless a focused audit proves the move is safe
and the maintainer explicitly keeps them in scope:

- `common.h`, because it is a broad project-wide type and macro boundary;
- `np2ver.h`, because it is release/product identity used by frontend and
  packaging paths;
- `oprecord.c` / `oprecord.h`, because operation recording was explicitly
  deferred from M72 and needs a dedicated audit.

## Non-goals

M86 must not change machine behavior, state-save payloads, timing semantics,
keyboard behavior, event scheduling, or uPD9002 instruction semantics.

## Validation

Run repository invariant checks, normal builds, native tests, save/load
checks, M68-M72 protected checks where available, and the standard manual VA
gate.
