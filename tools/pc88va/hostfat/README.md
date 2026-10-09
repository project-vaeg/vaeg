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
# HOSTFAT.SYS read-only host-folder drive

`HOSTFAT.SYS` is a clean-room PC-Engine CONFIG.SYS block driver distributed
under the repository's two-clause BSD terms. It exposes the immutable
read-only FAT12 snapshot created by vaeg's `--hostfat-dir` option. It does not
use INT 2FH redirector internals and never sends guest writes to the host.

The implementation was authored from the factual
[clean-room contract](../../../docs/agents/research/m54_hostfat_cleanroom_spec.md)
under the recorded
[input attestation](../../../docs/agents/research/m54_hostfat_cleanroom_attestation.md).
The final source fixes the NASM CPU level at 8086 so a V30 cannot encounter
80386 `0F 8x` near conditional-jump encodings. The generated-driver checker
fails if such an encoding or an unexpected command-dispatch edge appears.

The driver explicitly accepts PC-Engine's open and close lifecycle requests
as successful no-ops. These notifications bracket operations such as COPY;
all actual file data still arrives through the read-only sector path. Write
and write-with-verify requests continue to return write-protect.

Build with CMake when NASM is available:

```sh
cmake --preset linux-release
cmake --build --preset linux-release --target hostfat_sys
python3 tools/pc88va/hostfat/check_driver.py \
  --input build/linux-release/guest/hostfat.sys
```

Or assemble directly:

```sh
nasm -f bin -o hostfat.sys tools/pc88va/hostfat/hostfat.asm
```

The M55 PC-Engine-compatible FAT12-max output is 528 bytes with SHA-256
`393226edcde6b0cc8648ce9f8b380804c44e2bec7c3d762cb60f0bc211b1767e`.

Copy the generated file to the PC-Engine boot disk and add:

```ini
DEVICE=HOSTFAT.SYS
```

Then either start vaeg with a source tree for this session:

```sh
vaeg --hostfat-dir /path/to/read-only-root
```

or use Emulate -> Configure -> HOSTFAT read-only host folder. The GUI setting
persists as `HOSTFAT` and `HOSTFATDIR` in `vaeg.cfg`. Browse selects a folder;
OK builds a complete replacement image on a worker thread. The previous image
stays mounted while the progress indicator advances. A successful commit is
atomic and resets the guest so PC-Engine and `HOSTFAT.SYS` re-read the BPB.
A failed build leaves the old image mounted and reports the precise error.

The GUI does not persist a changed `HOSTFATDIR` until the replacement image
has been built and mounted successfully. Closing vaeg while a rebuild is in
progress therefore leaves the previous configuration and snapshot intact.
If a persisted HOSTFAT directory is empty or cannot be rebuilt at startup,
vaeg reports a warning, disables HOSTFAT in `vaeg.cfg`, and continues booting.
The directory path is retained so it can be corrected and re-enabled later.
Disable HOSTFAT to unmount and reset.

Snapshots never change while mounted. Additions, removals, and modifications
on the host become visible only after an explicit Rebuild + reset on OK. Save
states carry the mounted snapshot's SHA-256 identity. A matching image resumes
normally; a missing or different image rejects the load before any live state
is modified and opens a visible rejection dialog. The dialog's optional
`Force load` is explicit rather than automatic: it keeps the current HOSTFAT
mount state and read-only snapshot, then restores the remaining saved machine
state. Use it only when accepting the same risk as changing a read-only disk
while DOS may retain cached FAT, directory, open-file, or file data. Cancel
continues the current guest unchanged. States made without HOSTFAT remain
loadable when no HOSTFAT image is mounted.

Files and directories preserve the host's local last-write time in FAT
date/time fields. FAT has no timezone and stores seconds in two-second units;
earlier odd seconds are therefore displayed rounded down. Values outside
FAT's representable range clamp to `1980-01-01 00:00:00` or
`2107-12-31 23:59:58`.

On Windows, the selected HOSTFAT root may be a junction or another directory
reparse point such as a redirected Dropbox folder; vaeg canonicalizes that
selected root before taking the immutable snapshot. Links and reparse points
that resolve outside the selected root remain rejected. The snapshot accepts
at most 1024 entries and eight directory levels. A valid, unique ASCII 8.3
name using letters, digits, `_`, and `-` is retained after uppercase folding.
Other valid UTF-8 host names receive deterministic 8.3 aliases; duplicate
aliases are resolved deterministically. DOS device names, invalid UTF-8,
links/reparse points that escape the selected root, special files, containment
escapes, and files whose
identity or size changes while copied reject the whole rebuild.
The backing snapshot is fixed at 64 MiB. The driver advertises 65,362 logical
sectors of 1024 bytes (63.830078125 MiB) with 16 sectors per cluster. PC-Engine
therefore counts 4084 data clusters and selects FAT12 rather than FAT16. The
remaining 174 backing sectors are inaccessible through the guest service.
The six final FAT12-reserved cluster identifiers are marked reserved in both
FAT copies, so allocation stops at cluster `0FEFH`: at most 63.71875 MiB of
cluster payload is available before directory and per-file 16 KiB rounding.

This 1024-byte-sector geometry approaches the practical PC-Engine FAT12 limit
while retaining the driver's 16-bit sector number. PC-Engine's free-space
display still multiplies each free FAT entry by 2 KiB, so `DIR` reports about
8 MiB; a G55 test nevertheless copied byte-identical data allocated beyond
60 MiB. The rejected 2048-byte-sector/32 KiB geometry truncated a 96 KiB test
file to 6144 bytes, whereas the corrected 16 KiB geometry copied all 98,304
bytes. Historical PC-88VA SASI HDD and SCSI MO support use dedicated storage
paths. HOSTFAT does not emulate either interface: `HOSTFAT.SYS` itself is the
PC-Engine block driver.

## Private protocol

The driver uses the emulator-private channels that vaeg already reserves:

- `07EDH`: four-byte little-endian values;
- `07EFH`: command and response strings.

Writing `check_hostfat` to `07EFH` returns the two-byte signature `H1` only
when a compatible snapshot is mounted. For `read_hostfat1`, the driver first
writes the PC-Engine request-packet far pointer to `07EDH`, least-significant
byte first, then writes the command to `07EFH` and reads one result byte from
`07EDH`. Result 0 means success; all other results mean no guest bytes were
written. The non-IBM PC-Engine packet is 22 bytes, with its transfer pointer,
sector count, and starting sector at offsets `0EH`, `12H`, and `14H`.
vaeg validates the complete request packet, command, unit, sector
count, LBA range, and destination range before copying. Guest write commands
are rejected inside `HOSTFAT.SYS` with write-protect status and never reach
this service.
