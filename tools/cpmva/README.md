# CP/MVA installation assistant

`install_cpmva.py` builds a local CP/MVA installation from a user-owned,
FAT-formatted PC-Engine D88 boot disk. It automatically downloads and verifies
locked CPMVA, CP/M, `vt100-games`, and BDS C archives, builds the 64K CP/M 2.2
CCP/BDOS, constructs `CPM.SYS`, and creates deterministic CP/M disks.

The installer never modifies the input disk. Downloads, extracted sources, and
generated images are local user artifacts and are not part of the VAEG source
distribution. The installer does not relicense CP/M, CPMVA, the games, or BDS C.

## Prerequisites

- Python 3.10 or newer.
- `z80asm` 1.8, supplied with `--assembler`, selected through `VAEG_Z80ASM`,
  or available in `PATH`. Otherwise the installer downloads the locked
  `z80asm_1.8.orig.tar.gz` (`assembler` in `sources.lock.json`), verifies its
  size and SHA-256, and compiles it with the host C compiler (`$CC`, `cc`,
  `gcc` or `clang`) into `<cache>/tools/`. A program named `z80asm` that is
  not version 1.8, such as the z88dk assembler, is ignored. `--download-only`
  caches the source for later `--offline` runs.
- `lha` or `unar` for extracting the CPMVA `.LZH` archive. Both lha-1.14i
  and lhasa (the Debian/Ubuntu `lhasa` package, as `lha` or `lhasa`) work;
  the installer detects lhasa, uses its `xw=DIR` syntax, and restores the
  upper-case member names that lhasa lower-cases.
- The repository-native PC-Engine D88/FAT tool is used by the default `native`
  backend. `imgtool` is optional for `--image-backend imgtool`.

No NEC CP/M distribution disk, `MOVCPM5.COM`, `DDT.COM`, `SAVE`, M80, L80, or
MASM is required. The installer never executes downloaded DOS or CP/M programs
on the host.

## Online installation

The primary command is:

```sh
python3 tools/cpmva/install_cpmva.py \
    --boot-disk ~/images/pcengine-boot.d88 \
    --output-dir ~/images/cpmva-ready \
    --accept-cpm-license \
    --accept-cpmva-license \
    --accept-games-license \
    --accept-bdsc-license
```

The four acceptance flags are required for non-interactive use. Interactive
runs display the notices and ask for any missing acceptance. `--download-only`
fetches and verifies all locked sources without creating images.

The generated files are:

```text
pcengine-boot-cpmva.d88  modified boot disk copy
cpmva-tools.d88         EXIT/FCONV/DO, five games, and MESCC tools
cpmva-source.d88        game source, documentation, headers, and license text
cpmva-dev.d88           BDS C compiler, linker/runtime files, and documentation
cpmva-build-manifest.json
cpmva-install-report.txt
```

The generated CP/M files are padded to CP/M's 128-byte record boundary. The
manifest records both the original source size/digest and the stored size.

## Sources and provenance

The lock file pins the exact archive size and SHA-256. The game package is the
fixed commit from the [vt100-games repository](https://git.imzadi.de/acn/vt100-games).
It supplies `FTM.COM`, `ROBOTS.COM`, `BACKGMMN.COM`, `CPMTRIS.COM`, and
`MAZEZAM.COM`, plus their source and license evidence. The license labels used
by the installer are source-derived:

| Program | Recorded license |
| --- | --- |
| `FTM.COM` | GPL-3.0-only |
| `ROBOTS.COM` | GPL-2.0-only; the source says GPL Version 2 |
| `BACKGMMN.COM` | Public domain |
| `CPMTRIS.COM` | GPL-2.0-or-later |
| `MAZEZAM.COM` | GPL, version unspecified |

The BDS C archive is downloaded from the author's
[BDS C page](https://www.bdsoft.com/resources/bdsc.html). The author states
that BDS C, its binaries, source, utilities, and documentation are public
domain. Its exact archive notice is preserved in the cache and on
`cpmva-dev.d88`.

HI-TECH C is intentionally not downloaded or inserted by this installer.
The available historical mirrors identify copyright ownership but do not
provide a clear redistributable license for the compiler package. A future
addition requires a primary, reviewable redistribution grant and a pinned
archive; the installer fails closed instead of treating an archival mirror as
permission.

The repository does not upload or package the local reference directories
`docs/roms/`, `docs/disks/`, or `docs/tekumani/`. A task may read an image from
`docs/disks/` as a user-supplied test input, but the path and private contents
are not copied into generated manifests or archives.

## Offline and local-source installation

The default cache is `~/.cache/vaeg/cpmva`. A separate cache can be selected:

```sh
python3 tools/cpmva/install_cpmva.py \
    --boot-disk ~/images/pcengine-boot.d88 \
    --output-dir ~/images/cpmva-ready \
    --cache-dir ~/cache/vaeg-cpmva \
    --offline \
    --accept-cpm-license \
    --accept-cpmva-license \
    --accept-games-license \
    --accept-bdsc-license
```

Offline mode forbids network access and fails with `OFFLINE_MISS` when a locked
source or license is absent from the cache. Local overrides are still checked
against the lock:

```sh
python3 tools/cpmva/install_cpmva.py \
    --boot-disk ~/images/pcengine-boot.d88 \
    --output-dir ~/images/cpmva-ready \
    --cpmva-archive /path/to/CPMVA.LZH \
    --cpm22-archive /path/to/cpm2-asm.zip \
    --vt100-games-archive /path/to/vt100-games.zip \
    --bdsc-archive /path/to/bdsc-all.zip \
    --accept-cpm-license \
    --accept-cpmva-license \
    --accept-games-license \
    --accept-bdsc-license
```

`--dry-run` validates the input and reports intended actions without
modifying files. `--verify-only` validates the input and all locked sources
without replacing output files. `--force` may replace output files but never
the input disk. `--keep-work` preserves the temporary extraction/build tree.

The maintainer-only lock refresh command is explicit and never runs during a
normal installation:

```sh
python3 tools/cpmva/refresh_sources_lock.py --confirm --write
```

Review every old/new URL, size, and digest before accepting a lock diff.

## CP/M construction

`CPM22.Z80` is patched by `patches/cpm22-64k.patch` and assembled with the
pinned open-source `z80asm` 1.8. Symbol and size checks require CCP at `E400h`,
BDOS at `EC00h`, BIOS at `FA00h`, and exactly `0x1600` bytes from `E400h` to
`F9FFh`. The original `CPMBIOS.COM` supplies the final `0x0600` bytes, making
`CPM.SYS` exactly `0x1C00` / 7168 bytes. `MKSYS.BAS` constants are checked
before accepting this composition.

The tools, source, and development disks are deterministic PC-8801 2D D88
images with 327680 raw bytes and a CP/M directory at `0x4000`. Every disk is
unwrapped and parsed again before the output is written.
The CP/MVA BIOS DPB uses `EXM=1`, not `EXM=0`. Therefore the writer groups
two 16 KiB sub-extents into one directory entry. A file of 30,592 bytes is
encoded as one entry with `EX=1`, `RC=111`, and 15 allocation blocks; a full
32 KiB entry uses `EX=1`, `RC=128`. The validator applies the CP/M extent
formula, rejects duplicate logical extents from the old one-entry-per-16-KiB
layout, and rejects gaps before reconstructing a file. This is required for
large programs such as `BACKGMMN.COM` and `CC2.COM`; changing the BIOS or
emulator is not a substitute for matching its DPB.

## Test programs (M101)

`zex/zexbuild.py` rebuilds the stock ZEXDOC/ZEXALL from `external/zex` and
must reproduce the stock binary hashes. It also builds the file-output
variants (`zex/*-fileout.patch`), ZEX13S (`zex/zex13s.patch`), and the
independent probes `zex/flagprb.asm` and `zex/daadump.asm` (with
`zex/cpmio.asm`). The installer adds all of them to `cpmva-tools.d88`, and
also writes them alone to `cpmva-zexall-test.d88`, which leaves room for their
output files. The manifest records them under `test_programs` and
`test_disk`. See
[`docs/cpmva-setup.md`](../../docs/cpmva-setup.md) for use and comparison.

## VAEG procedure

1. Boot `pcengine-boot-cpmva.d88` as the PC-Engine boot disk.
2. Run `CPMVA.BAT` or `CPMVA`.
3. When CPMVA asks for a CP/M disk in FD1, replace FD1 with `cpmva-tools.d88`.
4. Press a key and confirm the CP/M `A>` prompt.
5. Run `EXIT` to return to PC-Engine.
6. To run a game, development utility, or inspect sources, swap FD1 to
   `cpmva-source.d88` or `cpmva-dev.d88` as needed.

The installer does not modify `AUTOEXEC.BAT` by default. `--autostart` creates
a recoverable backup, preserves line endings, and appends a marked
`CALL CPMVA.BAT` block. Automatic CP/M startup still requires the FD1 swap.

## Troubleshooting and limitations

- `SOURCE_DIGEST` or `SOURCE_SIZE`: the archive does not match the locked
  bytes; do not update the digest silently.
- `OFFLINE_MISS`: run once online with the same cache or supply every local
  archive override and a cached permission text.
- `ARCHIVE_TOOL`: install `lha`, `lhasa` or `unar`; archives are never executed.
- `BOOT_SPACE`: use a writable PC-Engine FAT D88 with enough free clusters.
- `ASSEMBLER_MISSING`, `ASSEMBLER_VERSION`: the supplied `z80asm` cannot run
  or is not version 1.8; fix `--assembler`/`VAEG_Z80ASM` or unset them.
- `ASSEMBLER_COMPILER`: install a C compiler, or supply `z80asm` 1.8 with
  `--assembler`.
- `ASSEMBLER_BUILD`, `ASSEMBLER_SOURCE`: the automatic z80asm build failed;
  the message contains the compiler output.
- `ZEX_STOCK_MISMATCH`, `ZEX_MSBT_MOVED`, `ZEX_PREFIX_CHANGED`: the rebuilt
  exercisers do not match the stock layout; do not use the generated disk.
- Existing output files require `--force`; the input disk is always protected.
- `--vaeg-binary` records a supplied emulator digest for provenance only. It
  does not prove CP/M reaches `A>` because CPMVA still requires an FD1 swap.
- GUI input and disk swapping are not automated by this milestone.

For the permanent provenance record, see
[`docs/modernization/cpmva-provenance.md`](../../docs/modernization/cpmva-provenance.md).
