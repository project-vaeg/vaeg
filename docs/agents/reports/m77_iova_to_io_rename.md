# M77: VA I/O source move report

## Scope

M77 moves the active VA I/O source files from `iova/` to `io/` with
rename-only semantics. This candidate was recreated from the current remote
`main`, after the previous M77 branch was deleted.

- Base: `2ef9716d9628ce8eefdf61a1feedca0be5921077`
- Rename commit: `2f2aeaf84646b3d8f2b512ef7b29f6b6d8bea30f`
- Candidate branch: `topic/m77-iova-to-io-rename`
- Candidate gate: G77

## Rename result

The rename commit moves 36 tracked files: 18 source files and 18 headers.
Git detects all entries as `R100`. The commit contains zero insertions and
zero deletions. File contents, symbols, state-save section names, dispatcher
ownership, and public interfaces are unchanged.

The moved files are:

```text
iova/bkupmemva.c/.h       iova/boardsb2.c/.h
iova/cgromva.c/.h         iova/fdsubsys.c/.h
iova/gactrlva.c/.h        iova/i8255.c/.h
iova/iocoreva.c/.h        iova/memctrlva.c/.h
iova/mouseifva.c/.h       iova/sgp.c/.h
iova/subsystem.cpp/.h     iova/subsystemif.c/.h
iova/subsystemmx.c/.h     iova/sysportva.c/.h
iova/tsp.c/.h             iova/upd9002_regs.c/.h
iova/va91.c/.h            iova/videova.c/.h
```

CMake source lists, include paths, and current documentation references were
updated in the follow-up reference-fixup commit. No compatibility symlink is part of this branch.
No M78, CPU, FDC, SDL, or behavior change was introduced.

## Validation

The following checks passed for the rename and reference-fixup changes:

- `git diff --check`.
- Staged diff inspection: 36 `R100` renames, zero insertions/deletions.
- Repository encoding, EOL, and case validators.

The recreated M77 branch was configured and built successfully after
the rename and reference fixups. The branch no longer requires the M78 path
fixups before a normal build can use the moved files; an old `iova -> io`
symlink must not be committed as a compatibility workaround.

## Gate status

The rename and reference-fixup candidate is ready for G77 review. G77 is not declared passed; M78 must not start and M77 must not be merged to `main` until G77 approval.
