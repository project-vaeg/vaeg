# M84b: clean up the remaining cpucva boundary

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

## Scope and status

M84b follows the M84a non-VA C-bus sound-board retirement. It completes the
remaining `cpucva/` boundary cleanup without changing uPD9002 instruction
semantics, FDC uPD780 behavior, I/O dispatch, or save-state formats. M84a and
M84b was under the single G84 human gate. G84 passed after human
validation, and M84 closed at
[`9aeb6512e59da7e794ffede50b7a184f601d137e`](https://github.com/nakatamaho/vaeg/commit/9aeb6512e59da7e794ffede50b7a184f601d137e),
which was fast-forwarded to `main`.

## Boundary decision

The audit found that the remaining files had two different ownership roles:

| Path family | Role | M84b disposition |
| --- | --- | --- |
| `cpucva/upd9002_upd70008.*` | uPD9002 main-CPU uPD70008-compatible adapter | retained |
| `cpucva/z80_compat_*` | shared suzukiplan-backed compatibility backend | retained |
| `cpucva/memoryva.*` | PC-88VA memory dispatcher and backing storage | moved to `memoryva/` |
| `cpucva/gvramva.*` | PC-88VA GVRAM implementation | moved to `memoryva/` |

`memoryva/` is the active-tree owner for VA CPU memory mapping, backing
storage, and GVRAM access. This keeps platform memory ownership separate from
both the uPD9002 instruction engine and the compatibility CPU adapters. The
M68 mapped-memory dispatcher remains the same: `cpu/upd9002/memory.c` still
selects the VA entry points, whose implementations are now in
`memoryva/memoryva.c`.

## Reference closure

The rename was followed by the required reference-only fixup:

- CMake compiles `memoryva/gvramva.c` and `memoryva/memoryva.c` and exposes
  `memoryva/` as an include root.
- The SDL screen path, build documentation, boot-sequence documentation,
  architecture note, and uPD9002 rename validator use the new owner path.
- `cpucva/` no longer contains VA memory or GVRAM sources; it contains only
  the retained main-CPU adapter and shared compatibility backend.
- Historical evidence inventories retain their original paths as historical
  records and were not rewritten.

## Compatibility and non-goals

The move is path and build ownership cleanup only. It does not alter exported
VA memory symbols, memory-map tables, CPU dispatch selection, state-save
sections, or runtime data layout. No binary payloads were changed.

M84a remains the separate source retirement checkpoint for the explicitly
approved non-VA sound-board closure. VA OPN/OPNA, board14, Sound Board II,
SASI/SCSI, common DMA, and FDC paths remain retained.

## Verification

- `python3 tools/repo/check_case.py`: 0 findings.
- `python3 tools/repo/check_encoding.py --expect utf8`: 0 violations.
- `python3 tools/repo/check_eol.py --enforce`: 0 violations.
- `python3 tools/qa/upd9002_rename.py`: PASS.
- `cmake --preset linux-ci-gcc` and `cmake --build --preset linux-ci-gcc -j4`: PASS.
- `ctest --test-dir build/linux-ci-gcc --output-on-failure`: 83 tests,
  100% passed; the external SST test is intentionally skipped.
- `cmake --preset mingw-cross` and
  `cmake --build --preset mingw-cross -j4`: PASS; the result is a PE32+
  x86-64 Windows executable.

## Commits

- M84a implementation: [`bf553ce41602b9ff3a4a8879412c77d5a8e70f4a`](https://github.com/nakatamaho/vaeg/commit/bf553ce41602b9ff3a4a8879412c77d5a8e70f4a).
- M84a record: [`9f3c010536a0d2f68ab95a4f223113c0e127251e`](https://github.com/nakatamaho/vaeg/commit/9f3c010536a0d2f68ab95a4f223113c0e127251e).
- M84b rename-only checkpoint: [`088dacf6c7aafa0d364a845ead94f0796583eadc`](https://github.com/nakatamaho/vaeg/commit/088dacf6c7aafa0d364a845ead94f0796583eadc).
- M84b reference fixup: [`890996ecb28627ec77c332c7917c61af29e1c23a`](https://github.com/nakatamaho/vaeg/commit/890996ecb28627ec77c332c7917c61af29a).

G84 human validation passed; M84 is closed and merged to `main` at
[`9aeb6512e59da7e794ffede50b7a184f601d137e`](https://github.com/nakatamaho/vaeg/commit/9aeb6512e59da7e794ffede50b7a184f601d137e).
