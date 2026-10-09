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

# 88VA Eternal Grafx Rel.20260814

This release contains the user-facing PC-88VA EMS, bank-memory, guest-driver,
and development-disk changes made after `rel-260805`.

## CP/MVA and everyday-use fixes

- CP/MVA can be built from the reproducible tools under `tools/cpmva/` and
  booted through the uPD70008-compatible main-CPU mode. The shared Z80
  compatibility implementation remains separate from the uPD780C floppy
  controller CPU.
- The SDL frontend now handles key repeat and the Windows JIS `ろ` key can
  enter an underscore (`_`) again. It also preserves the corrected TVRAM copy
  width and kanji pairing.
- Media and HOSTFAT dialogs can select the host-folder drive. The GUI also
  exposes the 98-font screen selection and reports effective clock/frame
  values, making model and timing configuration easier to verify.

## PC-88VA EMS Board

- Added **Device / EMS Board...** below I/O Bank Memory in the GUI.
- EMS capacity is configurable from disabled (`0`) through 13MB in 1MB
  units. New configurations default to 13MB; an explicit value already saved
  in `vaeg.cfg` is preserved.
- The board exposes the four PC-88VA 16KB page-frame windows at `C0000H`,
  `C4000H`, `C8000H`, and `CC000H` and remains independently configurable
  from I/O Bank Memory.
- Applying a changed capacity follows the normal guest-reset path. Canceling
  or accepting an unchanged value does not reset the guest.
- EMS self-tests cover all 832 logical pages at the 13MB maximum and both VA
  and compatibility I/O paths.

## I/O Bank Memory and VA defaults

- New configurations enable 16MB of BMS (128 128KB banks) at the native
  PC-88VA port `01D0H`.
- The `80000H-9FFFFH` aperture is available only while BMS is enabled, so the
  default 640KB main RAM remains intact. BMS and EMS capacities can be
  changed independently.
- RDBMS on the generated PC-88VA disk uses the native VA port `01D0H`,
  including on original-VA configurations that do not pass device options
  reliably.

## PC-88VA guest-driver bundle

Normal Linux, macOS, and Windows packages now include a matching, hash-pinned
guest-driver bundle:

- `HOSTFAT.SYS` exposes a read-only host-folder snapshot through
  `--hostfat-dir` or the GUI.
- `SQEMM98.SYS` is the Open Watcom-built EMS manager for the VAEG EMS Board.
  With EMMVA 1.5a and RDEMS 1.52, load `EMMVA01.SYS`, `SQEMM98.SYS`,
  `EMMVA02.SYS`, then `RDEMS.SYS` in that order.
- The bundle includes setup instructions, licenses, and `SHA256SUMS`.
  Drivers from an older VAEG release should not be mixed with a newer
  executable.

## [PC-88VA utility media](docs/modernization/pc88va-utility-media.md)

The reproducible PC-Engine 1.1 development-disk workflow now includes the
complete EMS stack and additional VA utilities:

- EMMVA/SQEMM98/RDEMS, with PC-Engine BIOS diagnostics and preserved boot
  files.
- PCPLUS 1.08 plus its bug-fix update, BMSDRVA, RDBMS, and the SCSI/DMA
  helper utilities.
- TSCLVA Rev.51127, RESET Rev.51028, and RDPCM Sound Board II RAM disk
  support.
- DIET processing for reducible `.EXE` and `.COM` files, while retaining the
  original archives and documentation for reproducibility.

ROMs, source PC-Engine disks, and generated development D88 images remain
user-supplied or maintainer-local files; they are not included in normal
binary release packages.

## VA-focused portable tree

The active portable source tree is now explicitly focused on PC-88VA and
VA2/VA3 operation. Retired non-VA model configuration, display backends, font
backends, and unbuilt legacy utilities are no longer part of the current
build. The archived reference tier remains available from its historical tag.

## Known limitations

- MSE 3.52B's `/B` code-swap path in the expanded development-disk recipe can
  prevent startup completion and can make an EMS diagnostic report no driver.
  The exact BMS/MSE interaction is tracked in [Issue #4](https://github.com/nakatamaho/vaeg/issues/4).
- MSE `/A` only works when Alias data has first been embedded with `MSECUST`;
  the stock patched executable does not contain generic Alias data.
- `HOSTFAT.SYS` is read-only by design. Use a SCSI or SASI image for writable
  guest storage.
- ROMs and disk images are not bundled. VA2/VA3 requires its model-specific
  `_va2.rom` files; renaming VA ROMs is not sufficient.
