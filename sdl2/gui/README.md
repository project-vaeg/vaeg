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
# SDL2 Dear ImGui GUI

The M10 GUI uses Dear ImGui with `imgui_impl_sdl2` and
`imgui_impl_sdlrenderer2`. It renders on the same `SDL_Renderer` that
`scrnmng` uses for the guest `SDL_Texture`.

## Input Routing

The main menu bar provides a `Pause / Break` button. It suspends guest CPU
execution, guest timing, and host audio while leaving the GUI responsive;
clicking `Resume` continues execution without a wall-clock catch-up burst.

The configured `Fast forward` binding is first handled as a frontend-global
hold-to-fast-forward shortcut, so its keyup always clears the transient state
even while ImGui captures input. It defaults to F11 and is configurable in
the Keyboard mapping window. The `Mouse capture` host action is likewise
listed there; it is active when the existing F12 binding menu is set to Mouse.
The same menu can route F12 to the guest PC key without enabling mouse capture.
It can also route F12 to `Full speed (No Wait)`, which uses the same temporary
hold-to-fast-forward path as the configured host action.
All events are then passed to Dear ImGui. Other keyboard events and
SDL_TEXTINPUT reach the guest only when `ImGuiIO::WantCaptureKeyboard` is
false. Mouse events reach guest-side routing only when
`ImGuiIO::WantCaptureMouse` is false. When ImGui wants the keyboard,
mouse, or text input, the guest does not receive that event.

M14 adds a keyboard binding capture mode. While it waits for the next
host scancode, the captured keydown and matching keyup are consumed by
the GUI and never sent to the guest.

Roman-Kana input parses A-Z and apostrophe host scancodes before guest
routing. It emits guest keyboard make/break sequences through
`sdl2/kbdinject.c`; it never injects Unicode text or guest memory bytes.
The menu selects JIS-Kana or Roman-Kana as the kana input method. The
assigned KANA key controls guest kana mode: one press locks KANA, and the
next press unlocks it. Roman-Kana consumes A-Z scancodes only while that
KANA lock mirror is active.

M24 adds `Edit -> Paste`, and this hotfix restores `Edit -> Copy screen text`.
Clipboard UTF-8 is filtered to printable ASCII and line breaks, converted to
M14 guest key roles, and paced through `kbdinject`. ImGui keyboard/text capture
prevents shortcut paste and pauses an active queue. Reset, state load, focus
loss, and shutdown cancel the queue and release any synthetic key. No Unicode,
IME text, or guest memory injection is used; Copy screen text handles guest-to-host text.

M26 routes mouse events only while SDL relative mouse mode is active and
ImGui does not want the mouse. The persistent capture request is independent
of temporary focus, menu, modal, and binding-capture blockers. Those blockers
release guest buttons and pending movement, suspend relative mode, and restore
it after the blocker clears. Button-up, focus-loss, reset, state-load, quit,
and shutdown cleanup are processed even when normal guest mouse input is
blocked, preventing stuck buttons. F12 is a host capture toggle only when its
binding is Mouse; middle click is always a host capture toggle outside ImGui.

## Storage Menus

FDD1/FDD2 Open and Eject act immediately through the existing floppy
mount path. The GUI Reset command reapplies the mounted FDD paths after
the core reset. The paths are saved as `FDD1FILE` / `FDD2FILE` in
`vaeg.cfg`, so mounted FDDs are restored across reset and application
restart; Eject clears the corresponding saved path.

FDD1/FDD2 Open accepts ZIP, 7z, and LZH in LibArchive-enabled builds. FDD1
Open assigns the first two basename-sorted archive images to FDD1/FDD2. FDD2
Open assigns only the first image to FDD2 without replacing FDD1 and reports
additional images as ignored. Archive selection reuses the same bounded,
traversal-safe extraction and persistent managed storage as SDL drop events.

SDL disk-image drops bypass ImGui capture and use the same FDD insertion path.
Direct images persist like manual mounts. ZIP, 7z, and LZH contents are
extracted under managed user-state storage and their mounted paths persist
through reset and application restart. Eject and replacement prune only
managed images that are no longer referenced by FDD1 or FDD2. Drop errors and
ignored-image counts are shown at the bottom of the FDD menu; successful mount
names are not duplicated there. The menu reads the live FDD state on every
frame and shows each mounted basename in normal text, including mounts
restored after application restart; hovering the name shows its full path.
FDD1/FDD2 Open remembers the source archive directory per drive and starts
there rather than displaying managed extraction storage. A small metadata file
inside each managed image directory preserves that browser location across an
application restart and is pruned together with the extracted image.

FDD -> New FDD image creates an empty FAT12 data disk as Japanese MS-DOS 2HD
(1.232 MB) or 2DD (640 KB). D88 output retains track/sector metadata; IMG
output is a headerless raw image suitable for mtools. The destination filename
is editable, existing files are never replaced, and the new image can be
mounted immediately as FDD1 or FDD2 through the same persistent mount path.
These are formatted data disks, not bootable MS-DOS system disks. 2D creation
remains deferred pending a separate compatibility audit.

HardDisk -> New SASI image creates HDI images through the existing
`newdisk_hdi()` helper. Its default filename is `new-sasi-hdd.hdd`; the
extension is only a descriptive filename choice and the image remains HDI
format. It supports the legacy 5/10/15/20/30/40 MB SASI geometry choices and
refuses to overwrite an existing file.

HardDisk -> SASI-1/SASI-2 Open updates `HDD1FILE` / `HDD2FILE` in
`vaeg.cfg` through the existing `diskdrv_sethdd()` path. HardDisk -> SCSI
SCSI ID 0 through SCSI ID 6 Open updates `SCSIHDD0` through `SCSIHDD6` through the
same path. The corresponding Remove command clears the selected entry.
HardDisk -> New SCSI image creates a VHD-format image using the existing
`newdisk_vhd()` helper, with 5/10/20/40/80/160 MB choices, and can configure it
for SCSI ID 0 through SCSI ID 6 immediately. The default filenames are
`new-scsi-hdd_id0.hdi` through `new-scsi-hdd_id6.hdi`; both `.hdi` and `.hdd`
names are accepted for the VHD image. New SASI is listed above New SCSI.
Headless or scripted runs can attach the same images with `--scsi0` through
`--scsi6`, using `none` to clear a slot. The command-line path validates the
VHD geometry before startup and shares the `SCSIHDD0` through `SCSIHDD6`
configuration entries with the GUI.
Reset the guest after changing a SASI or SCSI image; reset is the reliable
point where `sxsi_open()`, `PCHDD_SASI`/`PCHDD_SCSI`, and the board binding
are rebuilt for the guest. The configured image paths are retained across
the GUI Reset command. The HardDisk menu shows the live mounted basename
below each Remove command and exposes its full path on hover. The separate
PC-Engine support-disk assembly flow remains available for bootable media
such as `pcengine110-scsi-support.d88`.

## Sound Menu

Sound -> FM sound backend selects `NP2` or `ymfm` and persists the choice as
`opn_backend` in `vaeg.cfg`. Both engines mirror FM register writes, so changing
the output selector uses the existing guest-reset flow to rebuild the selected
synthesizer from a clean board state while retaining mounted FDD/SASI paths.
The ymfm option currently replaces only YM2203/YM2608 FM operator synthesis;
NP2 continues to own timer/IRQ, SSG, ADPCM, rhythm, and stream mixing.
Sampling rate selects 11.025, 22.05, or 44.1 kHz for the complete mixed output;
22.05 kHz remains the compatibility default and 44.1 kHz is recommended for
new configurations. Sound buffer provides 40/100/200/500/1000 ms presets and
a validated 40-1000 ms custom value. Both settings rebuild NP2 and ymfm sound
through the media-preserving guest reset path and persist as `SampleHz` and
`Latencys`.

ymfm fidelity selects Minimum, Medium, or Maximum native OPN generation before
box downsampling, approximately 166/333/998 kHz at the configured chip clocks.
Minimum remains the compatibility default; Maximum has the highest CPU cost.
The choice persists as `ymfm_fidelity` and is disabled while
NP2 is selected because NP2 has no equivalent fidelity control.
Sound on/off controls only host audio output. It does not clear `SNDboard` or
detach the guest FM hardware, so muting cannot remove the selected OPN/OPNA
check or stall software waiting for the FM timer.

## Boot Model Menu

Emulate -> Boot model selects `VA`, `VA2/VA3`, or `VA2/VA3 + 8087`. `VA` writes
`pc_model=88VA1` and disables the optional 8087; `VA2/VA3` writes
`pc_model=88VA2` with the 8087 disabled; `VA2/VA3 + 8087` writes the same
model with the single machine-owned 8087 enabled. The VA selection uses the
unsuffixed ROM names, while both VA2 choices use MAME-compatible `*_va2.rom`
names. Both sets are read beside the executable. Selection immediately uses
the existing guest-reset flow, including preservation of configured FDD and
SASI media. Missing ROMs are reported with the selected model and expected
root.

## Configure And Pacing

Emulate -> Configure opens a transactional CPU/SGP speed modal. Pending
values do not touch core or saved configuration until OK. CPU x1-x32 changes
V30 execution capacity; SGP Model default, Follow CPU, and Custom x1-x16
change SGP execution capacity. Model default is 3.9936 MHz on VA and 7.9872
MHz on VA2/VA3; the dialog displays the resulting effective SGP clock. OK
validates and resets through the existing FDD-preserving path. Cancel, Escape,
and window close discard edits.

The same modal contains the persistent HOSTFAT read-only host-folder setting.
Browse selects a directory without exposing files as selectable media.
Rebuild + reset on OK creates the fixed FAT12-max snapshot on a worker thread,
reports scan/copy/preparation progress, and keeps both the GUI and the old
mounted snapshot available until the replacement is complete. Commit is an
atomic pointer swap followed by a guest reset. A build error preserves the old
mount. Disabling HOSTFAT explicitly unmounts it and resets the guest. Host
folder changes are never reflected live inside a mounted FAT view.

The Configure modal also contains the optional Intel 8087 (NDP) setting. The
checkbox enables the single machine-owned 8087, and the integer clock field
accepts 1,000,000 through 20,000,000 Hz. Presets are 5, 8, and 10 MHz; custom
values remain persisted as integer Hz in `vaeg.cfg` under `NDP8087Hz`.
Pending values are shown separately from the active oscillator and are marked
as reset-required until the guest reset performed by OK. The modal reports the
active clock, keeps the one-device policy visible, and does not expose an FPO2
attachment control.

State loads remain strict by default. A missing or different HOSTFAT identity
opens a modal rejection instead of leaving its error hidden in the closed State
menu. If every other preflight result is acceptable, the modal offers
`Force load` with a warning: the current HOSTFAT mount state and read-only
snapshot remain active while the rest of the saved guest state is restored.
Cancel leaves the running guest untouched.

Screen exposes the persisted No Wait and frame-skip controls. The configured
Fast forward host action (F11 by default) is non-persistent: while held it
selects effective No Wait and draw skip 16, then immediately returns to the
saved values on release.

## Display Controls

Screen exposes Unfiltered, Linear, Scanline, and CRT Lite effects; Native,
Fit, Fit 8-dot, Integer, and Stretch scaling; Native/x2/x3/Custom logical
window sizes; and Windowed, Borderless desktop, or Exclusive fullscreen.
Display settings enumerates SDL displays and resolution/refresh combinations.
Custom size and display changes use pending values, and failed fullscreen
changes roll back without saving the failed selection.

The guest shadow framebuffer and texture remain 640x400. The common viewport
uses SDL renderer output pixels for High-DPI placement and keeps the ImGui
menu outside the guest rectangle. SDL2 remains the only renderer backend;
effects are procedural SDL draw operations and contain no MAME assets.

## Embedded Font

The GUI loads `NotoSansJP-Regular.ttf` with
`ImGuiIO::Fonts->GetGlyphRangesJapanese()`. The guest font ROM is never
used for host GUI text.

`assets/NotoSansJP-Regular.ttf` is the canonical source asset. CMake embeds
its bytes in the executable, and Dear ImGui reads the static data through
`AddFontFromMemoryTTF()`. Runtime font files, platform font lookup, and
`VAEG_ASSET_DIR` are not used. Release packages retain `assets/OFL.txt`
and `assets/NOTICE.md` for license compliance and provenance.
