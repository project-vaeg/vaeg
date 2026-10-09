<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# ADR-0005: Platform State Locations

Date: 2026-07-06

Amended: 2026-07-11

Status: Accepted

## Decision

M11 should move portable frontend user state out of ROM/executable
locations and into per-user platform state/config directories: Linux uses
the existing XDG base directory behavior for `vaeg.cfg` and stores
portable state files such as `vabkupmem.dat` and save states under the
same `vaeg` user area unless a user-selected absolute path is provided;
Windows uses `%APPDATA%\vaeg` instead of the legacy win9x exe-relative
directory so installed builds remain writable and Unicode-safe; macOS
uses `~/Library/Application Support/vaeg`. The legacy `win9x/` lineage
remains exe-relative and unchanged. Exe-relative portable lookup is at
most a compatibility fallback for explicit assets such as ROM/WAV files,
not the primary location for writable user state.

The active frontend later adds a portable-mode exception for
configuration and backup RAM. It accepts `vaeg.cfg` beside the executable
before checking the per-user directory. An existing executable-local
`vabkupmem.dat` likewise takes priority over the user-state copy. Both
files are saved back to the selected location. Obsolete `np2.cfg`,
`np2.ini`, and `vaeg.ini` files are not read, and backup RAM no longer
falls back to `getbiospath()` for migration. Keyboard sidecars and save
states remain in the per-user state directory.

`vabkupmem.dat` is writable VA user state. The portable frontend passes
the selected path into `iova/bkupmemva.c`; the core file does not depend
on SDL executable-path APIs.

## Writable Biospath Audit

- `iova/bkupmemva.c`: `vabkupmem.dat` is writable state and uses the
  frontend-selected executable-local or per-user path described above.
- Other portable `getbiospath()` users found in the audit are read-only
  ROM, WAV, font, dictionary, or key-map assets:
  `bios/romva.c`, `bios/bios.c`, `sound/soundrom.c`,
  `sound/rhythmc.c`, `fdd/fdd_mtr.c`, `iova/va91.c`, and `machine/keystat.c`.
- Fixed GUI save-state slots (`state0.nps` through `state9.nps`) are
  writable state but were not using `getbiospath()`; M11 stores them in
  the same per-platform state directory.
