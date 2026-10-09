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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# HOSTFAT setup guide

`HOSTFAT.SYS` exposes a read-only snapshot of a host folder as a PC-Engine
DOS drive. It is useful for importing test files and exchanging files with
the guest without modifying the host folder from DOS. It is not a writable
redirector: guest writes and deletes are intentionally rejected.

The current driver is named `HOSTFAT.SYS`. `HOSTDRV.SYS` is an old name and is
not part of the current release.

## 1. Install the release files

Unpack the VAEG release and place the executable, the model ROMs, and
`HOSTFAT.SYS` where they can be found. The release archive already contains
the matching `HOSTFAT.SYS`; do not copy a driver from an older VAEG build.
`README-PC88VA-drivers.txt`, `licenses/HOSTFAT.txt`, and `SHA256SUMS` in the
same archive describe and authenticate the bundled driver.

HOSTFAT also needs a PC-Engine support disk with this line in `CONFIG.SYS`:

```dos
DEVICE=HOSTFAT.SYS
```

The support disk must otherwise be a valid PC-Engine system disk. VAEG does
not redistribute proprietary PC-Engine ROMs or system disks. The technical
driver contract and source-build instructions are in
[`tools/pc88va/hostfat/README.md`](../../tools/pc88va/hostfat/README.md).

## 2. Select a host folder

Create or choose a folder containing only files that may be exposed to the
guest. Start VAEG with:

```sh
./vaeg --hostfat-dir /path/to/read-only-folder
```

On Windows, pass the folder in the normal Windows command-line form, for
example:

```text
vaeg.exe --hostfat-dir "C:\Users\name\Documents\vaeg-share"
```

The GUI equivalent is **Emulate -> Configure -> HOSTFAT read-only host
folder**. Browse selects the folder and **OK** starts a replacement snapshot
build. **Rebuild + reset on OK** makes host changes visible to the guest only
after the rebuild has succeeded and the guest has been reset.

The current snapshot remains mounted while a rebuild is running. If the
folder is invalid, contains an unsupported entry, or cannot be copied
consistently, VAEG displays the error and keeps the previous working mount.
An invalid persisted path disables HOSTFAT for that boot but retains the path
so it can be corrected later; it does not require deleting `vaeg.cfg`.

## 3. Use the drive in DOS

Boot the support D88 and wait for `HOSTFAT.SYS` to load. The drive letter is
selected by the rest of the PC-Engine configuration; use `DIR` to identify
it. If it is `D:`, a basic read-only check is:

```dos
DIR D:
TYPE D:\README.TXT
DEL D:\README.TXT
```

`TYPE` should display the host file. `DEL` should be refused because the
snapshot is write-protected. A DOS `COPY` whose destination is HOSTFAT is
also expected to fail. The host folder itself must remain unchanged.

To test a rebuild, edit or add a host file, press **Rebuild + reset on OK**,
wait for the operation to finish, and run `DIR` again. A host change is not
visible before that explicit rebuild. If the rebuild fails, fix the displayed
path or file error and retry; the last successful snapshot remains usable.

## 4. Snapshot limits and file names

The guest sees a fixed FAT12 volume with 1024-byte logical sectors and
16 KiB clusters. The DOS-visible volume is 63.830078125 MiB; directory and
per-file cluster rounding reduce the space available for files. DOS may show
an approximately 8 MiB free-space value because its display code assumes
2 KiB clusters. That display quirk does not reduce the readable snapshot
capacity.

ASCII 8.3 names are retained after uppercase folding. Other valid UTF-8
names receive deterministic 8.3 aliases. Invalid UTF-8, DOS device names,
special files, links or reparse points that escape the selected root, overly
deep trees, too many entries, and files that change while the snapshot is
being copied are rejected. On Windows, Dropbox folders and contained
junction/reparse paths are supported when canonicalization remains inside
the selected root.

Snapshots are immutable while mounted. Host additions, edits, and removals
become visible only after a successful rebuild and reset. Save states record
the snapshot identity; loading a state with a missing or different snapshot
is rejected before live machine state is changed. Use the explicit force-load
choice only when accepting the consequences of changing the guest's
read-only disk identity.

## 5. Troubleshooting

- **The drive is missing:** confirm `DEVICE=HOSTFAT.SYS`, use the matching
  driver from the release archive, and reboot the support disk.
- **The old folder is still shown:** rebuild and reset; changing files on the
  host does not mutate an already mounted snapshot.
- **The GUI says rebuild failed:** read the red error beside the rebuild
  control, correct the path or offending entry, and press **OK** again.
- **VAEG no longer starts after a bad HOSTFAT path:** repair the path in the
  configuration or temporarily disable HOSTFAT; the startup fallback keeps
  the emulator bootable and preserves the setting for later correction.
- **A write or delete fails:** this is expected HOSTFAT behavior. Use a SCSI
  or SASI image when the guest needs writable storage.
