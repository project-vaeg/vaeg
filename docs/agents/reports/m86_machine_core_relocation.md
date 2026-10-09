# M86: machine-core relocation report

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

## Scope and status

M86 starts from the G85-approved main continuation at
[9d4ea365](https://github.com/nakatamaho/vaeg/commit/9d4ea3657ca1684ab852b625fb8dfffb4f4372a0).
The implementation candidate was merged to `main` at
[74a5eac8](https://github.com/nakatamaho/vaeg/commit/74a5eac8bc0fa145fc0c4bf5ed66e3ff5368c0ae)
after the rename, reference-fixup, and validation commits below.
G86 human validation passed on 2026-08-12.

The implementation is deliberately split into the required layout commits:

- [78572be2](https://github.com/nakatamaho/vaeg/commit/78572be2cac1ae314dc50e668dd5b9570a43d9f6)
  moves the active root machine-core files into `machine/` using rename-only
  operations.
- [b04c6203](https://github.com/nakatamaho/vaeg/commit/b04c620316b3fccc00d457dab656bccdc9c3d019)
  updates CMake source lists, include paths, active source references, QA
  paths, and current documentation.

## Relocated files

| Area | Files | Result |
|---|---|---|
| Machine runtime | `pccore.c/.h`, `nevent.c/.h`, `timing.c/.h`, `calendar.c/.h` | Moved to `machine/` with no implementation change beyond include paths. |
| Input and events | `keystat.c/.h/.tbl`, `debugsub.c/.h` | Moved to `machine/`; keyboard and event behavior are unchanged. |
| State and scaling | `statsave.c/.h/.tbl`, `clockscale.h` | Moved to `machine/`; state section order, payloads, and clock semantics are unchanged. |

The active CMake source list now names `machine/*`, and the machine directory is
an include search path. Keeping `cpu/upd9002/upd9002_ops.mcr` byte-identical
preserves the M50 protected artifact while still resolving its relocated
`clockscale.h` include. No binary payload, ROM, disk image, or generated asset
was changed.

The following deferred boundaries remain at their original paths:

- `common.h` remains the project-wide type and macro boundary.
- `np2ver.h` remains the release and packaging identity boundary.
- `oprecord.c` and `oprecord.h` remain deferred from the earlier operation
  recording audit.

## Reference and documentation updates

Current C and C++ includes, the CMake source list, QA path assumptions, and
current architecture/decision documentation now use `machine/` paths. Historical
M48-M72 evidence and frozen-reference path names were not rewritten. The move
contains no CPU instruction, VA I/O, storage, timing, keyboard, or state-format
change.

## Verification

| Check | Result |
|---|---|
| `git diff --check` | PASS |
| UTF-8 encoding validator | PASS; 0 violations |
| LF EOL validator | PASS; 0 violations |
| Case validator | PASS; 0 findings |
| `upd9002_rename.py --root .` | PASS |
| `upd9002_native_invariant.py --root .` | PASS |
| `upd9002_rep0f_transition.py --root .` | PASS |
| `upd9002_m66_identity.py selftest` | PASS |
| `upd9002_m66_state.py --root . verify-m66a` | PASS |
| Protected deletion CTest checks | PASS |
| `cmake --preset linux-ci-gcc` and build | PASS |
| `ctest --test-dir build/linux-ci-gcc --output-on-failure` | PASS; 83/83, external SST corpus test skipped |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-gcc/sdl2/vaeg --selftest --model va` | PASS; all tests passed |
| `cmake --preset mingw-cross` and MinGW build | PASS |
| MinGW artifact | PE32+ `build/mingw-cross/sdl2/vaeg.exe`; SHA-256 `33e60f8e087332dbc4edd7aefa08c26ab56022b2a468ea55cf554d24cbcf9f37` |
| Post-merge MinGW build at `74a5eac8` | PASS; PE32+ `build/mingw-cross/sdl2/vaeg.exe`; SHA-256 `cfa823a83ffe56099c046972566a3799cd5d2f6b2d2fdc65473d7f501a827753` |
| Post-merge Linux-debug build at `74a5eac8` | PASS |
| Post-merge VA selftest at `74a5eac8` | PASS; all tests passed, exit 0 |
| Post-merge repository validators | PASS; case 0 findings, UTF-8 and LF checks clean |

The standalone `upd9002_protected_reachability.py --root .` validator still
reports `post-M48 graph differs from source regeneration` on both this
candidate and the integrated `main` control checkout at `9d4ea365`. This is a
pre-existing golden-graph drift, not a difference in the M86 source move; it is
recorded as an unresolved baseline validation item and was not changed
speculatively in this layout milestone.

## Gate state

M86 implementation, machine checks, and the standard G86 human gate are
complete. The implementation was merged to `main` at
[74a5eac8](https://github.com/nakatamaho/vaeg/commit/74a5eac8bc0fa145fc0c4bf5ed66e3ff5368c0ae)
at the maintainer's explicit request; the maintainer subsequently confirmed
that the clean-checkout V3/demo/OS/simple-operation gate passed.
