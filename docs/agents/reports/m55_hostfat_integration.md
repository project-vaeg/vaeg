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
# M55 HOSTFAT integration

Status: corrected implementation validated locally and by hosted CI. The
maintainer explicitly declared G55 passed on 2026-07-22 after the focused
PC-Engine HOSTFAT override and standard human-gate checks.

## Identity

- Branch: `topic/m55-hostfat-integration`
- Starting and approved supplemental G54 SHA:
  `e0bafbaa3cc0b12f945e18c231c843fc17ff0392`
- Current override implementation SHA:
  `40b96acaea8b925873d50c33f6fd3fc52dd71eb1`
- Current implementation/documentation and remote SHA:
  `e021e356185102911563ee9ee70d49637bcdd741`
- Final and remote SHA: supplied after the corrected validation/report commit.

## Commits

1. `bf13d19` — `M55: define maximum FAT12 HOSTFAT geometry`
2. `ace68a7` — `M55: expand HOSTFAT to the FAT12 limit`
3. `a24ec58` — `M55: bind save states to HOSTFAT snapshots`
4. `15ed5c4` — `M55: harden snapshot names and file identity`
5. `0f4dfd1` — `M55: add persistent asynchronous HOSTFAT controls`
6. `1d87511` — `M55: document FAT12-max HOSTFAT workflow`
7. `57e3deb` — `M55: allow sanitizer time for FAT12-max checks`
8. `ac7537a` — `M55: record HOSTFAT integration evidence`
9. `c9b70c4` — `M55: reserve FAT12 tail clusters explicitly`
10. `5366547` — `M55: record hosted validation result`
11. `14157f7` — `M55: use PC-Engine-compatible HOSTFAT clusters`
12. `5e83dfc` — `M55: fix HOSTFAT folder browser popup`
13. `063cf7c` — `M55: document PC-Engine HOSTFAT limits`
14. `1910dc1` — `M55: correct HOSTFAT validation geometry note`
15. `a8f6d3d` — `M55: record corrected G55 validation`
16. `40b96ac` — `M55: add explicit HOSTFAT state override`
17. `e021e35` — `M55: document HOSTFAT state override`
18. `710982f` — `M55: record HOSTFAT state override validation`

## Files changed

- Geometry, image ownership, identity, and transport:
  `io/hostfat.c`, `io/hostfat.h`, `statsave.c`, `statsave.h`, `statsave.tbl`.
- Snapshot creation and frontend lifecycle:
  `sdl2/hostfat_snapshot.cpp`, `sdl2/hostfat_snapshot.h`,
  `sdl2/hostfat_manager.cpp`, `sdl2/hostfat_manager.h`, `sdl2/np2.c`,
  `sdl2/np2.h`, `sdl2/ini.c`, `sdl2/gui/gui.cpp`, `sdl2/selftest.c`, and
  `CMakeLists.txt`.
- Guest driver and deterministic checker:
  `tools/pc88va/hostfat/hostfat.asm`, `check_driver.py`, and `README.md`.
- Planning and user documentation: `docs/agents/ROADMAP.md`, the M55 task,
  `docs/modernization/bug-fixes.md`, `sdl2/README.md`, `sdl2/gui/README.md`,
  and this report.

No frozen-reference file, ROM, disk image, font, icon, cursor, or wave payload
changed.

## FAT12-max geometry

| Property | M55 value |
|---|---:|
| Logical sector | 1,024 bytes |
| Sectors per cluster | 16 |
| Cluster size | 16 KiB |
| Reserved sectors | 0 |
| FAT copies / sectors per FAT | 2 / 7 |
| Root entries / sectors | 128 / 4 |
| DOS-visible sectors | 65,362 |
| Backing sectors | 65,536 |
| DOS-visible capacity | 66,930,688 bytes (63.830078125 MiB) |
| Data-cluster count used for FAT type | 4,084 |
| Highest allocatable cluster | `0FEFH` |
| Allocatable payload | 66,813,952 bytes (63.71875 MiB) |

The cluster count stays below the 4,085-cluster FAT16 cutoff. Allocation also
stops before FAT12's `0FF0H` reserved identifiers. The six geometrically
present tail clusters are marked `0FF0H` in both FAT copies, so a DOS free-space
scan cannot advertise them for allocation. The PC-Engine request packet
retains its 16-bit starting-sector contract. `HOSTFAT.SYS` advertises the same
BPB as the snapshot generator, and the generated-driver checker fails if the
geometry or cutoff changes.

The corrected generated driver is 528 bytes with SHA-256
`393226edcde6b0cc8648ce9f8b380804c44e2bec7c3d762cb60f0bc211b1767e`.
Direct NASM plus GCC, Clang, ASan, and MinGW CMake outputs reproduced it
byte-identically.

The first G55 run disproved the original 2,048-byte-sector/32 KiB-cluster
proposal. A generated 96 KiB file occupied three FAT entries, but PC-Engine
copied only 6144 bytes. The corrected layout follows the demonstrated PC-88VA
40 MB SASI layout's 1024-byte sectors and 16 KiB clusters while retaining the
FAT12 maximum of 4084 data clusters. The same 96 KiB source then occupied six
FAT entries and copied byte-identically. A separate marker allocated after a
60 MiB filler also copied byte-identically, proving access beyond 60 MiB.

Unpatched PC-Engine still reports free space as two KiB per free FAT entry, so
`DIR` displays approximately 8 MiB. This is not the readable snapshot limit.
Historical SASI HDD and SCSI MO support use dedicated storage paths; HOSTFAT
does not emulate either interface and `HOSTFAT.SYS` remains the independent
PC-Engine CONFIG.SYS block device.

## Persistent frontend and refresh lifecycle

`HOSTFAT` and `HOSTFATDIR` are persisted in `vaeg.cfg`. Emulate -> Configure
provides an enable switch, UTF-8 path input, and directory-only browser.
The initial browser button opened the popup under the child widget's ImGui ID
stack but attempted to begin it from the parent stack, so the IDs did not
match and the button appeared inert. The corrected code records the request
inside the child and opens the popup from the same parent scope as
`BeginPopupModal`. An Xvfb-driven GUI run visibly opened the folder selector.
Enabling or rebuilding creates a complete candidate on an SDL worker thread.
Scan, copy, and image-preparation progress is synchronized through an SDL
mutex; the ImGui/event/render loop continues normally.

The currently mounted image is retained throughout a rebuild. File copying,
full-image SHA-256, and preparation of core-owned storage all occur on the
worker. The frontend commit is a constant-time pointer swap, followed by the
normal FDD-preserving guest reset. A failed build retains the prior mount and
reports the exact error. Disabling explicitly unmounts and resets. A build in
progress blocks another rebuild or unmount. Host changes are never synthesized
into a live mounted FAT view.

The command-line `--hostfat-dir` remains a session-only startup override. A
persistently enabled empty path is a clear startup error rather than a silent
unmounted boot.

## Snapshot identity and save states

Each complete image receives a SHA-256 identity; the existing short FNV-1a
digest remains display-only. A new version-1 `HOSTFAT` state section is 36
bytes: format version, mounted flag, two required-zero bytes, and the 32-byte
identity. It contains no host path or private source metadata.

State preflight applies this policy before any live subsystem is imported:

- mounted and matching identity: accept;
- saved mounted image but current image missing or different: reject with a
  clear compatibility error;
- saved unmounted state while an image is mounted: reject as a mismatch;
- legacy state without a HOSTFAT section: accept only when HOSTFAT is not
  mounted.

Every rejection is presented in a root-scope modal rather than only in the
State menu that closes when a slot is selected. Strict matching remains the
default. If rechecking while bypassing only the valid HOSTFAT identity mismatch
leaves no blocker except the existing disk-change warning, the modal offers
`Force load`. The button warns that DOS may retain cached FAT, directory,
open-file, or file data. It restores the saved guest state while deliberately
retaining the current HOSTFAT mount state and read-only image; it never tries
to reconstruct or silently substitute the saved image. Malformed/truncated
HOSTFAT sections and unrelated preflight failures do not gain this override.

The selftest changes both CPU IP and guest memory before attempting a
mismatched load and verifies that both remain unchanged after strict
rejection. It then changes them again, explicitly overrides the identity,
requires both values to return to their saved values, and verifies that the
current HOSTFAT digest remains unchanged. Linux and Wine passed this test. A
PC-Engine GUI run with neutral A/B snapshots displayed the modal and restored
the earlier guest clock/state after `Force load`; the maintainer accepted the
focused interaction as provisionally passed. No private media name, path,
identity, or screenshot is recorded in Git. The M44 transactional
CPU286/UPD9002 adapter, opaque residue policy, section sizes, and CPU_SHUT
`FLAGS 0000` behavior are unchanged.

## Name mapping and host-path safety

Valid unique ASCII 8.3 names are retained after uppercase folding. Longer,
spaced, and valid Unicode UTF-8 names receive deterministic hash-derived 8.3
aliases with deterministic collision resolution. DOS device names do not pass
through as literal short names. Invalid UTF-8, control characters, excessive
depth/count, and data that cannot fit reject the entire rebuild.

The builder canonicalizes the root, rejects symlinks and Windows reparse
points, requires every traversed entry to remain below that root, and accepts
only regular files and directories. POSIX device/inode identity or Windows
volume/file identity, type, size, and last-write metadata are checked before
and after each copy. Hard-link aliasing and source replacement during a build
fail closed. A partial candidate is never mounted.

## Automated validation

Unless noted as an expected environmental result, every command below exited
zero.

```text
cmake --preset linux-ci-gcc -B /tmp/vaeg-m55-override-gcc --fresh
cmake --build /tmp/vaeg-m55-override-gcc --parallel 2
ctest --test-dir /tmp/vaeg-m55-override-gcc --output-on-failure

cmake --preset linux-ci-clang -B /tmp/vaeg-m55-override-clang --fresh
cmake --build /tmp/vaeg-m55-override-clang --parallel 2
ctest --test-dir /tmp/vaeg-m55-override-clang --output-on-failure
```

GCC and Clang each passed 35 tests, skipped only the unavailable external
SingleStepTests corpus test, and had zero failures out of 36 in 30.83 and
31.07 seconds respectively. These runs cover
the accepted M42--M54 dispatch, trace, ABI, state, REP+0F, protected-state,
rename, Z80, pacing, I/O-bank, and HOSTFAT gates. In particular, final dispatch
and accepted provenance artifacts, the 522-case diagnostic-stop suite, state
payload fixtures, and embedded M43 CI/full expectation summaries remained
unchanged.

```text
cmake --preset linux-ci-asan -B /tmp/vaeg-m55-override-asan --fresh
cmake --build /tmp/vaeg-m55-override-asan --parallel 2
ASAN_OPTIONS=detect_leaks=0 \
  ctest --test-dir /tmp/vaeg-m55-override-asan --output-on-failure \
    --parallel 2
```

The first ASan/UBSan matrix ran concurrently with GCC and Clang. All tests
except the existing trace-equivalence wrapper passed; that wrapper reached its
old 180-second timeout because it invokes the now larger complete selftest
three times. M55 raises only that test timeout to 420 seconds. The final
corrected matrix passed 35 tests, skipped only the external corpus, and had
zero failures out of 36 in 125.31 seconds; trace equivalence took 118.65
seconds. LeakSanitizer was disabled for the managed environment; address and
undefined-behavior instrumentation remained enabled.

```text
cmake --preset mingw-cross --fresh
cmake --build --preset mingw-cross --parallel 2
WINEDEBUG=-all \
WINEPREFIX=/home/maho/vaeg/build/m54-wine-prefix \
  wine64 build/mingw-cross/sdl2/vaeg.exe --selftest
```

The MinGW build and Wine ROM-less selftest passed, including the Windows
file-identity code path, asynchronous build/commit, state identity, FAT12
generation, and transport. Reparse-point rejection is compiled on Windows;
the selftest exercises it when the host permits creation of a test link. No
Wine-only failure occurred.

```text
python3 tools/pc88va/hostfat/check_driver.py \
  --input /tmp/vaeg-m55-override-gcc/guest/hostfat.sys
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py
cmake --preset linux-ci-gcc -B /tmp/vaeg-m55-override-no-nasm --fresh \
  -DVAEG_NASM_EXECUTABLE=VAEG_NASM_EXECUTABLE-NOTFOUND
git diff --check
git diff --name-only e0bafbaa3cc0b12f945e18c231c843fc17ff0392 \
  -- win9x i286x cpuxva/memoryva.x86 hlp romimage
```

The driver checker passed. Encoding and EOL checks were silent; case checking
reported zero findings. The unreferenced scan retained the established 70
findings and did not list the new manager. Diff checking passed, and the
frozen/binary-payload comparison was empty. The optional-NASM configuration
also passed while disabling only the generated guest-driver target.

```text
cmake --preset linux-release --fresh
cmake --build --preset linux-release --parallel 2
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  build/linux-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  build/linux-release/sdl2/vaeg --smoke
readelf --dyn-syms --wide build/linux-release/sdl2/vaeg
```

The tests-disabled cache records `VAEG_ENABLE_TESTS=OFF`. The ROM-less
selftest and headless smoke run passed. Dynamic-symbol filtering found no M55,
HOSTFAT selftest, snapshot-test, or audit seam exported by the production
binary. The release executable SHA-256 is
`06d4effb0b56671ecc7f8347305ed94cba66eeb86aa87cd104b3ed8ca40404ce`;
the MinGW executable SHA-256 is
`a33eeb863fa3ff57cd79a6857f9e2a9189028fa6c109c19ef444fc8c631460e1`.

The final FAT-boundary review additionally marked physical tail-cluster
entries `0FF0H`--`0FF5H` reserved in both FAT copies. Corrected clean GCC,
Clang, ASan/UBSan, Linux release, and MinGW builds passed. The three CTest
matrices completed in 30.83, 31.07, and 125.31 seconds plus successful release
and Wine runs. The snapshot selftest asserts all six packed FAT12 entries
exactly, requires a 96 KiB file to occupy six 16 KiB clusters, and verifies
allocation of a marker after a 60 MiB filler.

## Production isolation and hosted CI

- Tests-disabled Linux release and ROM-less smoke: passed.
- Dynamic symbol inspection: passed; no M55-only test/audit seam exported.
- [Hosted run 29906518052](https://github.com/nakatamaho/vaeg/actions/runs/29906518052)
  tested the corrected implementation/documentation SHA
  `e021e356185102911563ee9ee70d49637bcdd741` and completed successfully.
- All seven jobs passed: Linux GCC, Linux Clang, ASan/UBSan, Windows
  MSYS2/MinGW64, macOS FetchContent SDL2, standalone Z80 conformance, and
  repository invariants.
- GitHub's Node.js 20 deprecation annotations apply to the pinned Actions
  runtime and did not report a source, build, or test failure.
- The clean Linux release, MinGW release, and matching generated
  `HOSTFAT.SYS` were copied to the maintainer-local handoff directory and
  reproduced their build-artifact SHA-256 values byte-for-byte.

## Deviations and residual risks

- The ASan trace timeout was raised from 180 to 420 seconds because the test
  deliberately ran the former 128 MiB snapshot suite three times. The
  corrected 64 MiB geometry retains the timeout margin. No test
  content, dispatch artifact, or expected output changed.
- An atomic rebuild temporarily retains the old 64 MiB image while creating
  and preparing its replacement. Peak host memory can approach 192 MiB; this
  is bounded and avoids exposing a partial image or blocking the UI during the
  bulk copy/hash.
- PC-Engine's approximately 8 MiB `DIR` free-space figure does not reflect the
  16 KiB cluster reads. The byte-identical marker copied from beyond 60 MiB is
  the capacity evidence; changing PC-Engine's display is outside the read-only
  CONFIG.SYS block-device contract.
- `Force load` is deliberately opt-in. It protects host data because HOSTFAT
  remains read-only, but guest code can observe inconsistent cached FAT,
  directory, open-file, or file data if it was using the old snapshot. Normal
  load therefore remains fail-closed.
- Real uPD9002 REP+0F semantics remain unknown and unchanged from the accepted
  M48 fail-closed policy. No CPU, timing, memory, DMA, interrupt, I/O, or
  protected-state policy was altered.

## G55 human gate

Maintainer result: **passed on 2026-07-22**.

- [ ] From a clean checkout of the reported SHA, build Linux and/or Windows
  release plus `HOSTFAT.SYS`; verify the driver is 528 bytes with SHA-256
  `393226edcde6b0cc8648ce9f8b380804c44e2bec7c3d762cb60f0bc211b1767e`.
- [ ] Put that `HOSTFAT.SYS` on a copy of the PC-Engine boot disk and retain
  `DEVICE=HOSTFAT.SYS` in `CONFIG.SYS`.
- [ ] In Configure, enable HOSTFAT, browse to a test folder, press OK, and
  confirm progress while menus/window/input remain responsive.
- [ ] Confirm the successful commit resets the guest, prints the HOSTFAT ready
  message, and reaches the OS prompt without hanging.
- [ ] Run `DIR` and note its known approximately 8 MiB free-space display;
  confirm this is not treated as the readable-capacity limit.
- [ ] `DIR`/`TYPE` a root file, subdirectory file, long/spaced name, and Unicode
  host name; record the deterministic 8.3 aliases.
- [ ] Copy a file larger than 16 KiB from HOSTFAT to writable guest media and
  compare its complete bytes with the host source.
- [ ] Place a small marker after at least 60 MiB of preceding source data,
  rebuild, copy the marker, and compare its complete bytes with the host.
- [ ] Add or change a host file and confirm it is invisible before rebuild;
  rebuild, observe the progress/reset, then confirm it appears.
- [ ] Save and load with the same snapshot and continue operation.
- [ ] Change/rebuild the snapshot, then load the old state and confirm the
  modal explains the mismatch. Cancel and confirm the running guest is left
  unchanged.
- [ ] Repeat the mismatch, choose `Force load`, confirm the earlier guest state
  returns, and confirm reads use the current read-only HOSTFAT snapshot rather
  than silently restoring the saved snapshot.
- [ ] Confirm create, overwrite, rename, and delete on HOSTFAT return
  write-protect and leave the host tree unchanged.
- [ ] Disable HOSTFAT and confirm unmount/reset; restart vaeg and confirm the
  enabled/path setting persists when re-enabled.
- [ ] Repeat core HOSTFAT checks on both Windows and Linux.
- [ ] Complete the standard V3 boot, bundled VA demo, OS boot, keyboard, FDD,
  Sound Board II, save/load, reset, and no-test-seam checks.

G55 was explicitly approved by the maintainer. No later milestone is started
by this report.
