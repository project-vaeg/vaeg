# M84a: retire non-VA C-bus sound-board dependencies

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

## Scope and status

M84a is the first implementation checkpoint of M84. It retires the
explicitly approved non-VA C-bus sound-board family and its dependency
closure. M84b continues the remaining `cpucva/` boundary cleanup. Both
The checkpoints were under the single G84 human gate. G84 passed after
human validation, and M84 closed at
[`9aeb6512e59da7e794ffede50b7a184f601d137e`](https://github.com/nakatamaho/vaeg/commit/9aeb6512e59da7e794ffede50b7a184f601d137e),
which was fast-forwarded to `main`.

## Deleted implementation closure

The requested board and I/O files were deleted:

- `cbus/amd98.[ch]`
- `cbus/board26k.[ch]`
- `cbus/board86.[ch]`
- `cbus/board118.[ch]`
- `cbus/pcm86io.[ch]`
- `cbus/cs4231io.[ch]`

The audit found two direct dependency chains, so the following non-VA
implementations were deleted as well:

- `cbus/boardx2.[ch]`, which binds `pcm86io`;
- `sound/pcm86.[ch]`, `sound/pcm86c.c`, and `sound/pcm86g.c`;
- `sound/cs4231.[ch]`, `sound/cs4231c.c`, and `sound/cs4231g.c`.

CMake, FM-board dispatch, generic I/O fallback, DMA dispatch, state-save FM
branches, event tables, GUI volume/configuration entries, and the unused
DIP-switch bitmap entry points were updated so no active source reference
requires the retired implementations. The VA OPN/OPNA paths, board14 and
Sound Board II paths, SASI/SCSI storage, and common DMA/FDC paths remain.

## Compatibility boundary

The two old sound-board option bytes and the old PCM volume byte remain in
`PCCORE` as named reserved padding. They preserve the serialized `PCCORE`
field positions for VA save states; they are no longer configurable or read
by active non-VA sound code. Event-number gaps for the removed PCM86 and
CS4231 events are also preserved by leaving later event numbers unchanged,
while their obsolete callback entries are removed.

This is a source-tree retirement, not a claim that old non-VA FM-board save
states remain loadable. VA save/load layout and VA sound dispatch remain the
supported compatibility contract.

## Verification

- `cmake --preset linux-ci-gcc`
- `cmake --build --preset linux-ci-gcc -j4`

The Linux CI-profile build passed after the M84a source deletion. The MinGW
cross build also passed. Repository checks and focused state-save tests remain
part of the final M84 handoff; G84 human validation passed and M84 is closed
at `9aeb6512e59da7e794ffede50b7a184f601d137e`, merged to `main`.

## Commit

The M84a implementation checkpoint is
[`bf553ce41602b9ff3a4a8879412c77d5a8e70f4a`](https://github.com/nakatamaho/vaeg/commit/bf553ce41602b9ff3a4a8879412c77d5a8e70f4a).
The documentation checkpoint records this implementation commit; M84b
continues on top of it.
