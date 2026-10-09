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

# 88VA Eternal Grafx Rel.20260830

Rel.20260830 is a refinement release for the portable PC-88VA build. It
consolidates the native VA implementation, tightens compatibility and storage
paths, and improves the everyday SDL frontend without changing the project
scope beyond PC-88VA and VA2/VA3 operation.

![Double-buffered 65,536-colour software-sprite demo](https://raw.githubusercontent.com/nakatamaho/vaeg/rel-20260830/docs/images/vaeg-20260830-195233-0000880059-000.png)

*Actual SDL-rendered VAEG capture of the double-buffered, 65,536-colour
software-sprite demo. The demo disks are available from
[`demos/disks`](https://github.com/nakatamaho/vaeg/tree/main/demos/disks).*

## VA implementation and graphics

- The active build is now a single native VA path: retired PC-98 routing and
  other non-VA configuration surfaces have been removed while VA C-bus,
  storage, and compatibility boundaries remain explicit.
- The SGP work has been consolidated around the documented descriptor, LINE,
  SCAN, ROP, and transparency behaviour. The renderer and the companion
  visual/contract tests cover 256-colour and 65,536-colour scenes, including
  double-buffered software-sprite demonstrations.
- The SGP results are VAEG visual-regression evidence; they do not claim
  equivalent real-PC-88VA timing or hardware validation.

## Storage, expansion, and development media

- The PC-9801-55-compatible SCSI path has received command-phase, block I/O,
  media-geometry, and diagnostics hardening. SASI, SCSI, and read-only
  HOSTFAT remain separate storage choices.
- Development-disk and SASI-image builders now select the native BMS port,
  reserve the appropriate MSE swap bank, configure the EMS RAM disk, and
  include the additional freely distributable development tools and manuals
  supported by their builders.
- BMS and SCSI initialization captures in the documentation show BMSDRV using
  the native `01D0H` port and SCHD detecting a guest-usable 159MB volume from
  a 160MB virtual SCSI image.

## Frontend usability

- PrintScreen saves a collision-safe PNG screenshot of the raw 640×400 guest
  frame in the current directory. Host menus and overlays are excluded.
- Screenshot capture is available as an F12 multipurpose selection; guest
  COPY is no longer the fixed PrintScreen action. The command-line capture
  path can also target a guest frame.
- The GUI gained a Pause control, clearer Japanese-facing menu labels, and
  more consistent sound and media handling. F12 may be used for the guest PC
  key or full-speed operation through its selectable bindings.
- Archive disk-image drops, PC-Engine filename handling, and the preservation
  of a valid VA sound selection have been tightened.

## Documentation and compatibility guidance

- The CP/MVA/uPD70008, SCSI, MO, XMS/SMM, BMS, and development-media guides
  have been expanded and clarified in English and Japanese where applicable.
- The uPD70008-compatible main-CPU mode remains distinct from the uPD780C
  floppy-controller CPU even though both use the shared Z80 compatibility
  implementation.

## Update notes and limitations

- ROMs, source PC-Engine disks, and private integration media are not bundled.
  VA2/VA3 still requires the model-specific `*_va2.rom` files; VA ROM files
  cannot be substituted by renaming them.
- `HOSTFAT.SYS` remains read-only. Use a SASI or SCSI image for writable guest
  storage.
- MO support packages are retained for hardware-reference workflows; they do
  not make a virtual MO device available in the emulator.
