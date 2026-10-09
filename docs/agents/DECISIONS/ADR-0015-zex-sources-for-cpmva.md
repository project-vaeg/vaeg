<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# ADR-0015: Track ZEX sources for user-built CP/MVA disks

## Status

Accepted by the maintainer for M101 on 2026-09-29. Amends the "ZEX
acquisition" section of ADR-0011 for the source tree only.

## Context

ADR-0011 kept the ZEXDOC/ZEXALL inputs out of Git and out of every source and
release archive. They were fetched by hash for conformance CI only.

M101 needs the exercisers on the real PC-88VA2 and in vaeg: the stock
programs, variants that also write their console output to a file, and a
ZEXDOC-derived split of the `<daa,cpl,scf,ccf>` group. The real-machine
evidence in `docs/modernization/uPD9002-zex-results.md` could not record the
source of its modified exercisers (O5). The CP/MVA installer already builds
user-run CP/M disks locally from locked sources with z80asm 1.8, and those
disks are never distributed by the project.

## Decision

- Track the upstream `zexdoc.src`, `zexall.src` and the bundled `LICENSE.txt`
  from suzukiplan/z80 `test-ex` at the ADR-0011 base commit
  `e3926769a790fab0af1c34a5540e317f8d4f0ddc`, byte-for-byte, under
  `external/zex/`. `external/zex/provenance.txt` records URLs and SHA-256.
- The stock `.cim` binaries stay untracked. Conformance CI keeps fetching
  them by hash. `tools/cpmva` rebuilds them from the tracked sources and must
  reproduce the stock hashes byte-for-byte.
- ZEX-derived sources and patches are GPL-2.0-or-later, following the source
  notices, and are marked as such. They live beside the tracked ZEX sources
  or the CP/MVA tooling, never in the emulator core.
- No ZEX source, derived source, or assembled program is compiled into,
  linked into, or packaged with a vaeg executable or release archive. The
  resulting CP/M programs exist only on disks that a user generates with the
  CP/MVA installer.
- `tests/z80_compat/check_zex_archive.py` has two modes:
  - release mode (the default) rejects every ZEX name and content hash, as
    before;
  - source mode accepts exactly the three recorded `external/zex/` upstream
    files, and ZEX-derived files at their recorded paths, and still rejects
    them anywhere else. The stock `.cim` hashes stay rejected everywhere.
  CI uses source mode only for the `git archive` check.

## Consequences

- The vaeg source archive contains GPL test sources as a separate aggregate.
  The emulator itself and all release packages stay free of GPL code.
- ADR-0011's statement that the ZEX `.src` files never enter source archives
  is superseded for the recorded `external/zex/` paths only.
