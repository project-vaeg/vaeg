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
# M54 HOSTFAT clean-room reimplementation

Status: implementation and private PC-Engine integration validation complete;
supplemental maintainer gate pending. M55 has not begun.

## Identity and reason for the correction

- Branch: `topic/m54-hostfat-cleanroom-driver`
- Starting/approved G54 SHA:
  `19626dc6be4337eb4666181d0a42f0bfcb2f38ce`
- Final and remote SHA: supplied in the handoff after this report is committed
- Original G54 result: maintainer reported all checks green

The first M54 guest driver was functionally accepted, but its authoring
process consulted `RDBMS.ASM`. Merely placing a two-clause BSD header on that
expression did not establish that it could be redistributed under BSD-2.
The maintainer selected a clean-room rewrite instead of retaining it with
uncertain provenance or rewriting published history.

## Clean-room separation

The reviewing agent first wrote the implementation-neutral factual contract
in
[`m54_hostfat_cleanroom_spec.md`](../research/m54_hostfat_cleanroom_spec.md).
It contains packet offsets, status values, BPB bytes, I/O-port protocol, and
observable command behavior, but no labels, instruction sequences, control
flow, or source formatting.

An isolated implementation agent received no conversation or repository
context and was allowed to read only that contract. It was explicitly barred
from RDBMS material, current or historical HOSTFAT source/binaries,
disassembly, the checker, reports, and Git history. Its complete input list,
seed source/binary hashes, and prohibited-input affirmation are recorded in
[`m54_hostfat_cleanroom_attestation.md`](../research/m54_hostfat_cleanroom_attestation.md).

After independent authorship, a normalized source comparison found no
nontrivial copied expression:

- superseded HOSTFAT versus clean-room seed ratio: `0.077098`;
- RDBMS versus clean-room seed ratio: `0.029070`;
- the only two-line RDBMS matches were generic `push ds` / `push es` and
  `pop es` / `pop ds` register-save pairs;
- longer common blocks were mandated device-header/name or BPB data.

The comparison was an audit after authorship, never an implementation input.

## Integration defect and correction

The isolated seed assembled to 512 bytes, but NASM's unspecified CPU level
relaxed long conditional branches to 80386 `0F 84H` encodings. On V30,
`0FH` belongs to a different instruction space. Initialization used a short
branch and appeared to work, while read-command dispatch entered unintended
V30 behavior. A private boot reached `Ready`, but `DIR C:` printed the ready
message and stopped instead of listing the snapshot.

Commit
[`bdcbeae89b254dd02b8916104baac81c94f94a4d`](https://github.com/nakatamaho/vaeg/commit/bdcbeae89b254dd02b8916104baac81c94f94a4d)
fixes the source at the 8086 instruction level and expresses each dispatch as
a short `JNE` around an 8086 short or near `JMP`. The checker decodes all nine
command edges and rejects every `0F 80H`--`0F 8FH` byte pair.

An initially suspected resident-end ordering issue was rejected. Two
otherwise identical 8086-safe binaries, differing only in whether the
resident pointer was returned before or after the initialization message,
both completed `DIR`. The final source retains the isolated implementer's
original ordering; no unproved ordering change was kept.

## Final artifact identity

- `tools/pc88va/hostfat/hostfat.asm`: SHA-256
  `aa91ed4768a789398a63fa26669da843f91c28947b929b1051d542abe8f04788`;
- generated `HOSTFAT.SYS`: 528 bytes, SHA-256
  `c036b88178f058295eaeedae8c9dffd0bcf13addb13449c307b2fba921a8f675`;
- generated binary remains untracked.

Direct NASM and CMake outputs must reproduce that identity. The checker also
validates the complete 22-byte packet offsets, BPB and FAT12 cutoff, status
paths, open/close lifecycle, write protection, resident extent, protocol
signature, and V30-safe dispatch.

## Private PC-Engine integration evidence

A temporary copy of a maintainer-supplied PC-Engine system disk was reduced
to the normal system files plus `CONFIG.SYS` and the clean-room
`HOSTFAT.SYS`. The original image was not modified. A temporary neutral host
tree was mounted with `--hostfat-dir`; no private path, image identity, or raw
screen capture is tracked.

With `--keyboard-layout us`, the final 528-byte binary produced these results:

- PC-Engine reached `Ready` without a CONFIG.SYS hang;
- `DIR C:` identified volume `HOSTFAT` and listed one file plus one
  subdirectory;
- `TYPE C:TEST.TXT` completed and returned to `Ready`;
- `COPY C:TEST.TXT A:COPY.TXT` completed and returned to `Ready`;
- re-extracting `COPY.TXT` from the temporary D88 produced 6,780 bytes and
  exactly matched the source, SHA-256
  `aaf129053802416c8a20fd9b3ac1fe0eabb2adb44d03c66beb2e19382e6f25ae`.

The unsupported-encoding seed and the accepted pre-clean-room driver were
also run under the same setup. The seed reproduced the dispatch failure; the
old and corrected clean-room binaries both completed `DIR`. This isolates the
demonstrated root cause from disk geometry, host snapshot generation, and the
emulator transport.

## Host timestamp correction

The supplemental gate then showed every HOSTFAT entry as `80-01-01`. The
snapshot builder had captured each regular file's last-write time only for
change detection while unconditionally writing FAT time `0000H` and date
`0021H` to every directory entry. This was deterministic but discarded
guest-visible source metadata.

Commit
[`9c707a93bc64ded691c756e205a2b7a0ef42c899`](https://github.com/nakatamaho/vaeg/commit/9c707a93bc64ded691c756e205a2b7a0ef42c899)
converts the host last-write time to FAT local civil time. Seconds are reduced
to FAT's two-second resolution, values before 1980 clamp to
`1980-01-01 00:00:00`, and values after 2107 clamp to
`2107-12-31 23:59:58`. The timestamp is recorded for files, directories,
volume label, `.` and `..` entries. Directory type and last-write identity are
now rechecked before and after snapshot construction, extending the existing
transactional file checks.

This does not make HOSTFAT live. The metadata and content remain fixed for the
session and are refreshed only by fully restarting vaeg in M54.

## Validation

All commands below ran from the clean-room branch worktree. Unless an expected
negative check is identified explicitly, every command exited zero.

```text
cmake --preset linux-ci-gcc -B /tmp/vaeg-m54-cr-final-gcc --fresh
cmake --build /tmp/vaeg-m54-cr-final-gcc --parallel 4
ctest --test-dir /tmp/vaeg-m54-cr-final-gcc --output-on-failure

cmake --preset linux-ci-clang -B /tmp/vaeg-m54-cr-final-clang --fresh
cmake --build /tmp/vaeg-m54-cr-final-clang --parallel 4
ctest --test-dir /tmp/vaeg-m54-cr-final-clang --output-on-failure

cmake --preset linux-ci-asan -B /tmp/vaeg-m54-cr-final-asan --fresh
cmake --build /tmp/vaeg-m54-cr-final-asan --parallel 4
ASAN_OPTIONS=detect_leaks=0 \
  ctest --test-dir /tmp/vaeg-m54-cr-final-asan --output-on-failure
```

Each tests-enabled profile configured and built cleanly, then reported 35
passed, one external SingleStepTests-corpus skip, and zero failures out of 36.
LeakSanitizer alone was disabled because the managed execution environment
runs tests under tracing; AddressSanitizer and UndefinedBehaviorSanitizer
remained active. These suites include the accepted M42--M54 regression gates,
including the HOSTFAT snapshot, packet, FAT12, lifecycle, transport, and
write-protect tests.

```text
cmake --preset linux-release -B /tmp/vaeg-m54-cr-final-release --fresh
cmake --build /tmp/vaeg-m54-cr-final-release --parallel 4
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /tmp/vaeg-m54-cr-final-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  /tmp/vaeg-m54-cr-final-release/sdl2/vaeg --smoke
readelf --dyn-syms --wide \
  /tmp/vaeg-m54-cr-final-release/sdl2/vaeg
```

The tests-disabled cache records `VAEG_ENABLE_TESTS=OFF`. ROM-less selftest
and headless smoke passed. Dynamic-symbol inspection found no `M54`, audit, or
HOSTFAT test seam exported by the production executable.

```text
cmake --preset mingw-cross -B /tmp/vaeg-m54-cr-final-mingw --fresh
cmake --build /tmp/vaeg-m54-cr-final-mingw --parallel 2
WINEDEBUG=-all \
WINEPREFIX=/home/maho/vaeg/build/m54-cleanroom-final-wine-prefix \
WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 /tmp/vaeg-m54-cr-final-mingw/sdl2/vaeg.exe --selftest
```

The MinGW build completed. Wine returned zero after all ROM-less tests,
including `HOSTFAT snapshot` and `HOSTFAT transport`; the unavailable ALSA or
WASAPI endpoint diagnostics did not affect the selftest result.

```text
nasm -f bin -o /tmp/hostfat-cleanroom-direct.sys \
  tools/pc88va/hostfat/hostfat.asm
python3 tools/pc88va/hostfat/check_driver.py \
  --input /tmp/hostfat-cleanroom-direct.sys
sha256sum /tmp/hostfat-cleanroom-direct.sys \
  /tmp/vaeg-m54-cr-final-release/guest/hostfat.sys \
  /tmp/vaeg-m54-cr-final-mingw/guest/hostfat.sys
```

Direct NASM, Linux CMake, and MinGW CMake outputs are byte-identical 528-byte
files with SHA-256 `c036b88178f058295eaeedae8c9dffd0bcf13addb13449c307b2fba921a8f675`.
The checker accepted each final binary. It rejected the isolated 512-byte seed
with the expected nonzero exit and diagnostic `386 near conditional jump is
not valid in the V30 driver`.

```text
cmake --preset linux-ci-gcc -B /tmp/vaeg-m54-cr-final-no-nasm --fresh \
  -DVAEG_NASM_EXECUTABLE=VAEG_NASM_EXECUTABLE-NOTFOUND
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py
git diff --check
git diff --name-only 19626dc6be4337eb4666181d0a42f0bfcb2f38ce -- \
  win9x i286x cpuxva/memoryva.x86 hlp romimage
```

The optional-NASM configure succeeded while disabling only the generated
guest-driver target. Encoding and EOL checks were silent-success; case
checking reported zero findings. The unreferenced scan exited zero with the
established 70 findings and no clean-room M54 source. Diff checking passed,
and the frozen-reference and binary-payload path comparison was empty.

Hosted [run 29886944384](https://github.com/nakatamaho/vaeg/actions/runs/29886944384)
completed successfully at documentation-and-implementation SHA
`e85400cf6ac15fae1030ff0112dada02ce8d5573`. All seven jobs passed: Ubuntu
GCC, Ubuntu Clang, Ubuntu ASan/UBSan, Windows MSYS2 MinGW64, macOS
FetchContent SDL2, standalone Z80 conformance, and repository invariants.
The final handoff supplies the later CI-record-only remote SHA.

The timestamp correction was then rebuilt with GCC, Clang, ASan/UBSan and
MinGW. Each Linux CTest profile again reported 35 passed, one external-corpus
skip and zero failures out of 36. The selftest compares the generated FAT
time/date fields for a root file, subdirectory, nested file, volume label,
`.` and `..`; it also checks exact packing and both representable-range clamps.
Wine exercised the Windows `FILETIME` conversion and returned zero. Final
fresh-tree and hosted results are supplied in the handoff after this report
update.

## Distribution boundary

The branch tip and source archives generated from it contain the clean-room
driver expression under BSD-2. Published Git history still contains the
superseded M54 source because this correction deliberately does not rewrite
history. A distribution that includes the complete historical object database
therefore also includes those old objects; history sanitization would require
separate explicit maintainer and legal approval.

## Supplemental M54 human gate

- [ ] Build from a clean checkout and verify generated `HOSTFAT.SYS` is 528
  bytes with SHA-256 `c036b881...`.
- [ ] Replace the old SYS on a copy of the boot disk; leave
  `DEVICE=HOSTFAT.SYS` in CONFIG.SYS.
- [ ] Start with `--hostfat-dir` and confirm the ready message and OS prompt.
- [ ] Run root and subdirectory DIR plus TYPE.
- [ ] Confirm files and directories show their host last-write date/time,
  rounded down to FAT's two-second resolution rather than `80-01-01`.
- [ ] COPY at least one file to writable media and compare its bytes.
- [ ] Confirm create, overwrite, and delete on HOSTFAT return write-protect and
  do not change the host tree.
- [ ] Reset and confirm the same snapshot remains readable.
- [ ] Start without `--hostfat-dir` and confirm unavailable/zero-unit behavior
  without affecting ordinary V3, VA demo, OS, RDBMS, or MSE operation.

The maintainer must explicitly approve this replacement before M55 begins.
