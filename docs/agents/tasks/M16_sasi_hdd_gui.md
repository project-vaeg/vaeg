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
# M16 — Reactivate SASI HDD support in the active SDL2 build

Status: complete

Branch: `topic/m16-sasi-reactivate`

Gate: G16 passed

Depends on: G15 passed.

## Goal

The frozen Win9x reference defined `SUPPORT_SASI`, but the active
CMake/SDL2 build had left it undefined. M16 restores SASI HDD support in
the active build and exposes the existing SASI-1/SASI-2 Open/Remove
workflow plus SASI HDI image creation through the SDL2 Dear ImGui
HardDisk menu.

The intended path is the existing NP2 configuration path:

```text
np2.cfg
  -> HDD1FILE / HDD2FILE
  -> np2cfg.sasihdd
  -> sxsi_open()
  -> sxsi_issasi()
  -> pccore.hddif |= PCHDD_SASI
  -> sasiio_bind()
```

## Non-goals

- Do not touch uPD9002/uPD70002 naming work.
- Do not mix V30 integration, Z80 replacement, `SUPPORT_PC88VA`,
  `VAEG_FIX`, or `VAEG_EXT` cleanup into this milestone.
- Do not define `VAEG_EXT`; Win9x debugger/extension paths stay out of
  the active build.
- Do not enable `SUPPORT_SCSI` or `SUPPORT_IDEIO`.
- Do not implement SCSI/IDE GUI mounting.
- Do not implement THD/NHD/SCSI/IDE image creation in this milestone.

## Constraints

- Do not modify the frozen reference tier:
  - `win9x/`
  - `i286x/`
  - `cpuxva/memoryva.x86`
  - `hlp/`
- Keep changes minimal and localized.
- Preserve UTF-8 without BOM, LF line endings, and lowercase path rules.
- Do not add ROM, disk, or HDD images to the repository.

## Required work

1. Audit SASI-related compile-time guards and entry points:
   - `SUPPORT_SASI`
   - `PCHDD_SASI`
   - `sasiio_reset`
   - `sasiio_bind`
   - `sxsi_issasi`
   - `sxsibios`
   - SASI DMA entry in `io/dmac.c`
2. Add `SUPPORT_SASI` to the active `vaeg_core` CMake compile
   definitions.
3. Fix only compile/link errors caused by restoring `SUPPORT_SASI`.
4. Add SDL2 Dear ImGui HardDisk menu items:
   - New SASI image...
   - SASI-1 Open...
   - SASI-1 Remove
   - SASI-2 Open...
   - SASI-2 Remove
5. Use the existing config keys:
   - `HDD1FILE`
   - `HDD2FILE`
6. Persist the last GUI HDD browser directory as SDL2 frontend state.
7. Document that GUI Open/Remove updates configuration; reset is the
   reliable point where the guest observes the SASI device binding.
8. Use `newdisk_hdi()` for SASI HDI image creation. Do not overwrite
   an existing file silently.
9. Preserve mounted FDD and configured SASI images across the SDL2 GUI
   Reset command.

## Verification

Run, or explain why unavailable:

```sh
rg -n "SUPPORT_SASI|sasiio|PCHDD_SASI|HDD1FILE|HDD2FILE" \
  CMakeLists.txt cbus fdd bios io pccore.c pccore.h sdl2

cmake --build --preset macos-release
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/macos-release/sdl2/vaeg --smoke
SDL_AUDIODRIVER=dummy ./build/macos-release/sdl2/vaeg --selftest
git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
```

The maintainer may also run the same checks on `linux-release`.

## Gate G16

G16 is a human storage gate:

- configure `HDD1FILE` or use HardDisk -> SASI-1 Open...;
- create a disposable HDI image through HardDisk -> New SASI image...;
- reset the guest after changing the SASI image;
- boot or access a SASI-compatible HDD image;
- confirm read and write behavior on disposable media;
- confirm SASI-1/SASI-2 Remove clears the config and is reflected after
  reset.
- confirm FDD and SASI image selections remain in effect across the GUI
  Reset command.

Residual risk to carry if G16 has not been performed:

- THD/NHD/SCSI/IDE image creation remains unimplemented.
- SCSI/IDE remain out of scope.
- Runtime hot-unplug without reset is not guaranteed.

## Completion record

The active `vaeg_core` target defines `SUPPORT_SASI`; `VAEG_EXT`,
`SUPPORT_SCSI`, and `SUPPORT_IDEIO` remain unchanged. The ImGui HardDisk
menu exposes SASI-1/SASI-2 Open and Remove plus SASI HDI creation using
the existing 5/10/15/20/30/40 MB geometry table. Configuration continues
to use `HDD1FILE` and `HDD2FILE` in `np2.cfg`.

G16 passed after the maintainer verified image creation, Open/Remove,
reset retention, boot/read/write behavior, and FDD/SASI media retention
across GUI Reset. SCSI/IDE and non-HDI image creation remain out of scope.
