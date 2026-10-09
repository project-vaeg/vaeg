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

88VA Eternal Grafx Rel.20260814 distribution README
================================================

This archive contains the portable SDL2/Dear ImGui build of 88VA
Eternal Grafx. Source archives are attached automatically by GitHub
Releases for the release tag; this archive contains one platform binary
package.

Disk images and guest WAV payloads are not included. ROMs are not
included and must be extracted from hardware you own.

Files supplied by the release package
-------------------------------------

Setup guides
------------

The release notes are in `CHANGES.20260814.md`. User setup guides are
available in the tagged source tree:

- SCSI support: https://github.com/nakatamaho/vaeg/blob/rel-20260814/docs/modernization/scsi-support.md
- HOSTFAT: https://github.com/nakatamaho/vaeg/blob/rel-20260814/docs/modernization/hostfat.md
- PC-88VA EMS and utility media: https://github.com/nakatamaho/vaeg/blob/main/docs/modernization/pc88va-utility-media.md
- CP/MVA: https://github.com/nakatamaho/vaeg/blob/rel-20260814/docs/cpmva-setup.md

The SCSI guide covers support-D88 preparation, target IDs, image attachment,
and SCFORM. The HOSTFAT guide covers `DEVICE=HOSTFAT.SYS`, the
`--hostfat-dir` option, GUI rebuild/reset, and read-only behavior.


Keep this relative layout after unpacking:

- vaeg or vaeg.exe
- HOSTFAT.SYS
- SQEMM98.SYS
- README-PC88VA-drivers.txt
- SHA256SUMS
- CHANGES.20260814.md
- assets/OFL.txt
- assets/NOTICE.md
- licenses/HOSTFAT.txt
- licenses/SQEMM98.txt
- licenses/suzukiplan-z80.txt
- README-dist.txt

The Dear ImGui Japanese font, historical VAEG startup graphic, historical
VAEG application icon, and MIT-licensed suzukiplan compatibility core are embedded in
the executable. OFL.txt, NOTICE.md, and licenses/suzukiplan-z80.txt document
their licenses and provenance and must stay with the package.

HOSTFAT.SYS is the PC-Engine read-only host-folder block driver generated
from the clean-room source in the VAEG repository. Configure it with
`DEVICE=HOSTFAT.SYS`; the emulator must be started with `--hostfat-dir` or
the equivalent GUI setting for a host-folder snapshot to be available.

SQEMM98.SYS is the Open Watcom-built PC-88VA EMS manager for vaeg's EMS
Board. It loads between EMMVA01.SYS and EMMVA02.SYS; those adapter drivers
and the optional RDEMS.SYS RAM-disk driver are supplied through the separate
supplemental-disk workflow. README-PC88VA-drivers.txt gives the exact
CONFIG.SYS order. The paired HOSTFAT and SQEMM98 licenses are under
`licenses/`, and SHA256SUMS covers the complete guest-driver subset.

Additional platform runtime files:

- Windows: SDL2, LibArchive, compression libraries, and the MinGW GCC,
  libstdc++, and winpthread runtimes are statically linked into vaeg.exe.
  The GUI font, startup image, and application icon are embedded. No
  companion DLL or frontend asset file is required.
- Linux: SDL2 and LibArchive are system dependencies. Install your
  distribution's SDL2 and LibArchive runtime packages. Rel.20260814 is built
  and tested on the GitHub Actions ubuntu-latest runner using the
  linux-ci-gcc preset, libsdl2-dev, and libarchive-dev. Frontend assets are
  embedded; no system-wide desktop entry is installed.
- macOS: the release preset statically links pinned FetchContent SDL2,
  LibArchive, and compression libraries. macOS system frameworks remain
  operating-system dependencies. Frontend assets are embedded. This release
  is a plain executable rather than an app bundle, so it has no Finder
  bundle icon.

Files you must supply
---------------------

Place ROMs beside vaeg or vaeg.exe. The model ROM sets are separate:

- VA: vadic.rom, vafont.rom, varom00.rom, varom08.rom, varom1.rom
- VA2/VA3: vadic_va2.rom, vafont_va2.rom, varom00_va2.rom,
  varom08_va2.rom, varom1_va2.rom
- Shared extra FDD subsystem ROM: vasubsys.rom

VA2/VA3 does not fall back to the unsuffixed VA names. The _va2.rom files
have model-specific contents and checksums; do not create them by renaming
or copying the VA ROMs. The frontend warns about size, CRC32, or SHA-1
differences but does not block startup solely for a checksum mismatch.

Supply your own disk images. You can mount floppy images from the GUI or
pass up to two image paths on the command line:

    ./vaeg disk1.d88 disk2.d88

On Windows:

    vaeg.exe disk1.d88 disk2.d88

VA configuration prerequisites
------------------------------

For VA booting, vaeg.cfg must select the VA machine, matching sound
hardware, and the VA clock domain:

    pc_model=88VA1
    SNDboard=100
    clk_base=3993600
    clk_mult=2

SNDboard=100 is the VA built-in YM2203/OPN. Use SNDboard=200 for a VA
with Sound Board II, or with pc_model=88VA2 for YM2608/OPNA. The GUI
selects these defaults when the boot model changes. If you reuse an
existing config, invalid combinations can stop at a V2 screen, leave
sound hardware unbound, or run in the wrong clock domain.

Config and state file locations
-------------------------------

The portable frontend normally stores writable files outside the unpacked
distribution directory:

- Linux: $XDG_CONFIG_HOME/vaeg or $HOME/.config/vaeg
- Windows: %APPDATA%\vaeg
- macOS: ~/Library/Application Support/vaeg

vaeg.cfg, vabkupmem.dat, and fixed GUI save-state slots normally live
there. An executable-local vaeg.cfg or existing vabkupmem.dat takes
priority over its user-state counterpart and is saved in place. Save
states and keyboard sidecars remain in the user directory.
Save-state files are tied to the architecture and build family that
created them; do not move them between old Win32 builds, portable
builds, different CPU architectures, or different operating systems.

macOS unsigned binary note
--------------------------

The macOS command-line binary is not code-signed. If Finder or Gatekeeper
adds quarantine metadata after download, remove it from the unpacked
release directory before running:

    xattr -dr com.apple.quarantine vaeg-rel260805-macos-arm64

You can also remove it from just the binary:

    xattr -d com.apple.quarantine ./vaeg
