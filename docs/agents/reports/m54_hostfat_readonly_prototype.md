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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# M54 read-only HOSTFAT prototype

Status: the maintainer passed G54 for the functional prototype at
`19626dc6be4337eb4666181d0a42f0bfcb2f38ce`. A later source-provenance audit
required a clean-room guest-driver rewrite; its supplemental human gate is
pending and no M55 work has begun.

The final report/remote SHA is supplied in the handoff because this file
cannot contain the SHA of the commit that contains itself.

## Identity and scope

- Branch: `topic/m54-hostfat-readonly-prototype`
- Starting SHA: `e5741b83fe12f121bb2eb955cbd7b8f0d2af61f2`
- Pre-report implementation SHA: `b661490322a998a20ca9881442c6a06c8c312958`
- Human-gate packet-layout correction SHA:
  `a07a8c4a764a2b5d8560bdbaea8f5ebc5c0edae4`
- Human-gate COPY lifecycle correction SHA:
  `bf6896d801c2d021f44cec43b7070531030c780a`
- Human-gate FAT12 geometry correction SHA:
  `5faa8ca0b04aac954a1da3d08c882c32651a0033`
- Hosted implementation run:
  [build 29813271492](https://github.com/nakatamaho/vaeg/actions/runs/29813271492)
  (result recorded below)
- Hosted packet-layout correction run:
  [build 29817436136](https://github.com/nakatamaho/vaeg/actions/runs/29817436136)
  (seven of seven jobs passed)
- Hosted COPY correction run:
  [build 29820788369](https://github.com/nakatamaho/vaeg/actions/runs/29820788369)
  (seven of seven jobs passed)
- Final and remote SHA: supplied in the G54 handoff

The worktree began clean at the stated starting SHA. M54 adds only a
session-scoped, read-only host-folder snapshot and the guest block driver that
reads it. HOSTFAT remains disabled unless `--hostfat-dir` is supplied. No M55
GUI, persistence, refresh, save-state identity, long-name mapping, or writable
behavior is present.

## Commits before this report

1. `f34f7b5d685ca60d8abe936800fea2a89ff0d2ba` —
   `M54: define read-only HOSTFAT milestones`;
2. `567e04d2fc1ccd288d26bf927b4f052290c3f308` —
   `M54: build deterministic read-only FAT snapshots`;
3. `f79b677c1e48071779349a4ac3b404ed291f821a` —
   `M54: expose a fail-closed HOSTFAT sector service`;
4. `2263cb36c58bad5c7075fb0d997fe28009d714ce` —
   `M54: add the PC-Engine HOSTFAT block driver`;
5. `752a791ca812ea5201655cca7f83c9629f7b5913` —
   `M54: add HOSTFAT fail-closed regression coverage`;
6. `ef4d9e8054fc9f98eaf6465295cd9c1932e4f32e` —
   `M54: record private I/O channel correction`;
7. `2f527cca552a92fdbbf49c45acb35240fc40f0f4` —
   `M54: harden snapshot race and link rejection`;
8. `b661490322a998a20ca9881442c6a06c8c312958` —
   `M54: reject cross-platform hard links`.
9. `a07a8c4a764a2b5d8560bdbaea8f5ebc5c0edae4` —
   `M54: fix PC-Engine HOSTFAT request layout`.
10. `bf6896d801c2d021f44cec43b7070531030c780a` —
    `M54: support PC-Engine HOSTFAT copy lifecycle`;
11. `5faa8ca0b04aac954a1da3d08c882c32651a0033` —
    `M54: keep HOSTFAT geometry within FAT12`.

## Files changed

```text
BUILD.md
CMakeLists.txt
docs/agents/ROADMAP.md
docs/agents/tasks/M54_hostfat_readonly_prototype.md
docs/agents/tasks/M55_hostfat_integration.md
docs/modernization/bug-fixes.md
io/hostfat.c
io/hostfat.h
io/np2sysp.c
sdl2/README.md
sdl2/cliopts.c
sdl2/cliopts.h
sdl2/hostfat_snapshot.cpp
sdl2/hostfat_snapshot.h
sdl2/np2.c
sdl2/selftest.c
tools/pc88va/hostfat/README.md
tools/pc88va/hostfat/check_driver.py
tools/pc88va/hostfat/hostfat.asm
```

No frozen-reference file, ROM, disk image, font, icon, wave payload, or
accepted uPD9002 golden was changed.

## Snapshot architecture

`--hostfat-dir <path>` is parsed into the session options only. After normal
configuration loading and CLI validation, but before `pccore_init()` and the
first machine reset, the frontend scans the source and builds an 8 MiB image
in temporary host memory. `hostfat_mount_image()` commits a private copy only
after the complete candidate image succeeds. A failed rebuild leaves an
already accepted image intact. The mounted image survives ordinary guest
reset and is freed on frontend shutdown or startup-error unwind.

The FAT12 geometry is fixed and matches the RDBMS block-driver convention:

| Field | Value |
|---|---:|
| bytes per sector | 1024 |
| sectors per cluster | 2 |
| reserved sectors | 0 |
| FAT copies | 2 |
| sectors per FAT | 7 |
| root entries | 128 |
| DOS-visible sectors | 8186 |
| backing sectors | 8192 |
| DOS-visible data clusters | 4084 |
| media descriptor | `F0H` |

Both FAT copies are generated from the same packed table. Allocation starts
at cluster 2, uses deterministic depth-first order after each directory is
sorted by its folded 11-byte DOS name, and never allocates FAT12 values
`FF0H`--`FFFH` as data clusters. File and directory timestamps preserve host
local last-write time at FAT's two-second resolution and clamp to the FAT
1980--2107 range. For unchanged source metadata in the same timezone,
attributes, timestamps, volume label, directory ordering, unused bytes, and
cluster padding are deterministic.

M54 accepts only ASCII letters, digits, `_`, and `-` in 8.3 names. Lowercase
ASCII folds to uppercase. Unsupported names and folded collisions reject the
whole candidate rather than being omitted. The scan also rejects symbolic
links, detectable hard links, non-regular special files, more than 1024
entries, more than eight directory levels, unrepresentable sizes, and images
that exceed the fixed capacity. File type, size, modification time, and link
identity are checked before and after copying; a second bounded-buffer read
must reproduce the captured bytes before commit. This includes empty files
and detects ordinary same-size modification races.

The source path itself is neither stored in the image nor printed in the
snapshot summary. Generated snapshot and driver binaries remain untracked.

## Emulator-private transport

M54 uses vaeg's existing emulator-private channels and does not claim physical
PC-88VA hardware ports:

- `07EDH`: four-byte little-endian value channel;
- `07EFH`: command/response string channel;
- `check_hostfat` -> `H1` only while an M54 image is mounted;
- request far pointer via `07EDH`, then `read_hostfat1` via `07EFH`;
- one-byte result via `07EDH` (`0` success, nonzero no transfer).

The service reads the full 22-byte PC-Engine request packet and validates the
packet span, length, unit, command, nonzero transfer count limit, LBA interval,
destination interval, and mounted state before its first guest-memory write.
Zero-sector reads succeed without writing. Invalid packet, unit, command,
count, LBA, destination, and unmounted cases are deterministic and leave the
destination unchanged.

The packet uses PC-Engine's 13-byte non-IBM common header. Its command fields
begin after eight reserved bytes at offsets 5--12: media/unit is at `0DH`, the
transfer pointer at `0EH`, count or BPB pointer at `12H`, and starting sector
at `14H`.

The audit also demonstrated and corrected a pre-existing binding defect:
`np2sysp_bind()` had attached both value and string callbacks to `07EFH`, and
only to the generic table. The value callbacks now use `07EDH`, the string
callbacks remain on `07EFH`, and both are attached to the generic and VA I/O
tables. The permanent correctness ledger records the evidence and fix.

## PC-Engine block driver

`tools/pc88va/hostfat/hostfat.asm` is an independently written non-IBM-format
CONFIG.SYS block driver. It implements initialization, media check, BuildBPB,
read, removable query, write/write-with-verify rejection, and deterministic
unknown-command handling. Device-open and device-close notifications are
successful no-ops. Writes return the PC-Engine write-protect status inside
the driver and never invoke the host service. Initialization installs one
unit after an `H1` probe and prints:

```text
HOSTFAT read-only drive ready
```

Without a mounted image it installs zero units and prints an unavailable
message. NASM is optional for emulator builds; when present, CMake builds and
structurally checks `guest/hostfat.sys`. A clean configure with NASM forced
unavailable also succeeded and disabled only the guest-driver target.

The initial G54 handoff driver used IBM-sized field offsets. Although its
header and BPB passed the original shallow checker, live PC-Engine
initialization interpreted its misplaced resident-end pointer as `CS:001CH`,
reclaimed the remaining driver, and later entered overwritten code. The
packet-layout-corrected driver had these deterministic identities:

- size: 455 bytes;
- SHA-256: `7be6d3be6f22fa32130eac7e7b2224146dae1f20448802d449c211814291c4bc`.

Two independent corrected NASM invocations are byte-identical. The
strengthened checker verifies every relevant `0DH`--`14H` displacement in the
emitted machine code and rejects the former binary at its first old-layout
field.

After the COPY corrections, the final generated driver identity is:

- size: 468 bytes;
- SHA-256: `62e04cffcd0d883becde965a9d87f8e0001d0f0d6f78a35aeff59367af4adc29`.

The checker also requires explicit `0DH` and `0EH` command comparisons and
derives the data-cluster count from the emitted BPB. A BPB at or beyond the
4085-cluster FAT16 cutoff fails validation.

The generated binary is not committed.

## Human-gate packet-layout correction

The first G54 boot printed the driver ready message and then stopped during
CONFIG.SYS processing. A debugger capture of the loaded driver proved that
PC-Engine had reclaimed and overwritten the bytes following offset `001CH`.
The next request was therefore dispatched through corrupted code. This
excluded host snapshot construction, the command-line path, and the private
I/O channel as causes.

Comparison with the independently retained RDBMS source established the
missing eight-byte reserved region in the common request header. After both
sides were moved to the complete 22-byte layout, the same private live
PC-Engine integration boot printed the ready message and reached its command
prompt. No private image name, host path, screenshot, save state, or asset
digest is retained in Git.

## Human-gate COPY corrections

Root DIR then worked, but COPY reported that the drive driver could not
execute the command. A comparison with RDBMS command dispatch showed that
PC-Engine brackets COPY with device-open command `0DH` and device-close
command `0EH`. HOSTFAT had returned unknown-command status `8103H`; the final
driver accepts both notifications as successful no-ops without weakening its
write-protect path.

After that correction, COPY advanced to source reads but reported a read
failure. Captured 22-byte requests proved that PC-Engine followed packed
FAT12 cluster 2 to cluster `0040H` and eventually requested invalid LBA
`AE18H`. The 8192-sector BPB yielded 4087 data clusters, so PC-Engine correctly
classified the volume as FAT16 while HOSTFAT had generated FAT12. The final
BPB exposes 8186 sectors, exactly 4084 data clusters. The 8192-sector backing
allocation is retained, but the host service rejects its final six hidden
sectors before any guest-memory write.

A private live PC-Engine retest copied neutral `TEST.TXT` from HOSTFAT to a
writable RAM disk, returned to `Ready` without an error, and reported the
exact 3958-byte destination length. A separate file larger than the RAM
disk's 112,640 free bytes reached the ordinary destination-full error rather
than a HOSTFAT driver or source-read error. No private asset identity,
screenshot, path, or digest is committed. Subdirectory TYPE,
write-protection, reset retention, and disabled-mode behavior remain for the
maintainer G54 gate.

## Automated evidence

The ROM-less M54 coverage verifies:

- root files plus one subdirectory, FAT-copy identity, FAT chains, directory
  entries, file bytes, fixed volume metadata, and reserved-cluster exclusion;
- byte-identical image regeneration;
- ASCII folding, folded-name collision, invalid name, symbolic/hard link,
  excessive depth, entry-count, and capacity rejection;
- transactional preservation of the accepted image after rejected rebuilds;
- default-disabled and explicit CLI parsing;
- generic and VA `07EDH`/`07EFH` signature probes;
- a valid 22-byte sector request whose eight reserved bytes are nonzero, plus
  rejection of malformed length, unit, command,
  count, LBA, destination, and unmounted requests without destination changes;
- rejection of the first hidden backing sector at DOS-visible LBA 8186;
- mount retention across `np2sysp_reset()` and explicit unmount behavior;
- deterministic assembly and structural validation of `HOSTFAT.SYS`, including
  open/close dispatch and a computed 4084-cluster FAT12 boundary.

The M52 BMS and M54 transport tests run after the accepted eight-step M42 CPU
trace has been exhausted. This prevents test setup I/O from contaminating the
frozen trace; the trace golden itself was not changed.

## Validation commands and results

All commands below exited 0 unless a deviation is stated.

```text
cmake --preset linux-ci-gcc -B .../m54-linux-ci-gcc --fresh
cmake --build .../m54-linux-ci-gcc --parallel 4
ctest --test-dir .../m54-linux-ci-gcc --output-on-failure

cmake --preset linux-ci-clang -B .../m54-linux-ci-clang --fresh
cmake --build .../m54-linux-ci-clang --parallel 4
ctest --test-dir .../m54-linux-ci-clang --output-on-failure

cmake --preset linux-ci-asan -B .../m54-linux-ci-asan --fresh
cmake --build .../m54-linux-ci-asan --parallel 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir .../m54-linux-ci-asan \
  --output-on-failure
```

Each corrected tests-enabled profile reported 35 passed, one external-dataset
test skipped because no external SingleStepTests path was configured, and
zero failed out of 36. The M42 graph, constructor, trace, 156-case harness,
ABI and state fixtures; M43 accepted sidecars; M44 state boundary; M45 native
execution; M46 normalization; M48 522-case diagnostic atomicity; and
M49--M51 guards all passed unchanged.

The corrected sanitizer CTest invocation first used LeakSanitizer defaults
and failed because this managed execution environment runs tests under
ptrace. It was rerun with only leak detection disabled. ASan and UBSan
remained enabled; the full suite then passed. Existing documented sound and
vendored Z80 shift diagnostics remain outside M54 and did not fail the
configured suite.

```text
cmake --preset linux-release -B .../m54-linux-release --fresh
cmake --build .../m54-linux-release --parallel 4
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy .../vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy .../vaeg --smoke
```

The tests-disabled production cache records `VAEG_ENABLE_TESTS=OFF`.
`readelf --dyn-syms` found no HOSTFAT or M54 audit-only exported symbol. The
ROM-less selftest and headless smoke run passed.

```text
cmake --preset mingw-cross -B .../m54-mingw-cross --fresh
cmake --build .../m54-mingw-cross --parallel 4
WINEDEBUG=-all WINEPREFIX=... wine64 .../vaeg.exe --selftest
```

The MinGW build and Wine ROM-less selftest passed, including snapshot link and
file-stability checks and the VA transport. Direct NASM output compared equal
to CMake output, and `check_driver.py` accepted the header, BPB, protocol
markers, 22-byte field displacements, size, and digest.

```text
cmake --preset linux-ci-gcc -B .../m54-linux-no-nasm --fresh \
  -DVAEG_NASM_EXECUTABLE=VAEG_NASM_EXECUTABLE-NOTFOUND
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py
git diff --check
git diff --name-only <start> -- win9x i286x cpuxva/memoryva.x86 hlp romimage
```

The optional-NASM configure succeeded. Encoding, EOL, and case checks passed;
the unreferenced scan exited 0 with the same established 70 findings and no
M54 source; `git diff --check` passed; and the frozen/payload path diff was
empty.

The human-gate correction additionally used clean `m54-fix-*` build trees.
The corrected GCC, Clang, and ASan/UBSan matrices again reported zero failed
tests, the tests-disabled Linux release selftest and smoke run passed, and the
MinGW build passed its Wine selftest. Direct NASM assembly produced the same
455-byte corrected driver as both CMake builds. The strengthened checker
accepted its `7be6d3be...` digest and rejected the former driver because it
did not write the media-check result at packet offset `0EH`.

The subsequent COPY correction was rebuilt from clean `m54-copy-*` build
trees. GCC CTest reported 35 passed, one external-dataset skip, and zero
failed out of 36. The tests-disabled Linux release selftest and smoke run
both exited zero, and the MinGW binary passed the Wine selftest. Direct NASM,
Linux CMake, and MinGW CMake outputs were byte-identical 468-byte drivers with
SHA-256 `62e04cffcd0d883becde965a9d87f8e0001d0f0d6f78a35aeff59367af4adc29`.
The checker accepted the explicit open/close lifecycle handling and computed
4084 FAT12 data clusters from the emitted BPB. The transport regression
rejected DOS-visible LBA 8186 without modifying guest memory.

## Hosted CI

[Run 29813271492](https://github.com/nakatamaho/vaeg/actions/runs/29813271492)
completed successfully at the pre-report implementation SHA. All seven jobs
passed: Ubuntu GCC, Ubuntu Clang, Ubuntu ASan/UBSan, Windows MSYS2 MinGW64,
macOS FetchContent SDL2, standalone Z80 conformance, and repository
invariants. The final handoff supplies the later report-only remote SHA.

[Correction run 29817436136](https://github.com/nakatamaho/vaeg/actions/runs/29817436136)
completed successfully at packet-layout fix SHA
`a07a8c4a764a2b5d8560bdbaea8f5ebc5c0edae4`. The same seven jobs passed,
including the corrected generated-driver structure check and HOSTFAT
transport regression. This run supersedes the earlier run for the
human-gate packet-layout correction.

[COPY correction run 29820788369](https://github.com/nakatamaho/vaeg/actions/runs/29820788369)
completed successfully at FAT12 geometry fix SHA
`5faa8ca0b04aac954a1da3d08c882c32651a0033`. All seven jobs passed. This is
the authoritative hosted run for the lifecycle and FAT12 corrections made in
response to the COPY failure.

## Deviations and remaining risks

- The external pinned SingleStepTests corpus was not configured locally; its
  committed M43 identity/sidecar checks passed, and hosted CI is authoritative
  for the normal repository matrix.
- LeakSanitizer cannot run under the managed ptrace wrapper, so only its leak
  detector was disabled; ASan/UBSan instrumentation remained active.
- The corrected private live boot proves PC-Engine initialization, INT 83H
  message display, resident-end handling, drive-letter assignment, root DIR,
  and neutral small-file COPY to a writable RAM disk. The maintainer later
  completed subdirectory DIR/TYPE, copied-byte comparison, write protection,
  reset retention, and disabled-mode checks at G54. Those checks must be
  repeated for the clean-room binary.
- The prototype is a fixed startup snapshot. GUI configuration, persistent
  selection, refresh semantics, save-state identity, broader deterministic
  name mapping, and final containment UX are deferred to gated M55.
- No claim is made that a hostile process with concurrent write access cannot
  deliberately race every metadata and duplicate-content check. M54 detects
  ordinary source mutation and fails transactionally; users should select a
  stable source tree for snapshot creation.

## Clean-room driver replacement

The original driver was functionally accepted at G54, but its authoring
process had consulted RDBMS source. It is superseded at the active branch tip
by a separately authored BSD-2 implementation. The factual contract,
isolation procedure, attestation, final source/binary hashes, demonstrated
V30 dispatch correction, private DIR/TYPE/COPY result, distribution boundary,
and replacement human gate are recorded in the
[M54 clean-room report](m54_hostfat_cleanroom_reimplementation.md).

The original source remains in published Git history because no history
rewrite is authorized. M55 remains blocked until the maintainer explicitly
approves the clean-room replacement.

## Original G54 human-review checklist

- [x] Build from a clean checkout and create a host folder containing only
  ASCII 8.3 files plus one ASCII 8.3 subdirectory.
- [x] Build `HOSTFAT.SYS`, put it on a PC-Engine system disk, and add
  `DEVICE=HOSTFAT.SYS` to CONFIG.SYS.
- [x] Start vaeg with `--hostfat-dir <folder>` and confirm
  `HOSTFAT read-only drive ready`.
- [x] Identify the assigned drive, run root DIR, and copy a neutral small file
  to a writable RAM disk without a HOSTFAT source-read or driver error.
- [x] Run DIR in a subdirectory and TYPE a text file.
- [x] Compare the copied bytes with the source files.
- [x] Attempt create, overwrite, and delete; confirm write-protect and confirm
  the host folder remains byte-for-byte unchanged.
- [x] Reset the guest and confirm the same snapshot remains readable.
- [x] Start without `--hostfat-dir`; confirm zero-unit/unavailable behavior and
  unchanged V3 boot, bundled VA demo, OS, RDBMS, and MSE operation.

The maintainer reported this original checklist all green. The clean-room
replacement checklist in the focused report is a new required approval. Do
not begin M55 before that supplemental approval.
