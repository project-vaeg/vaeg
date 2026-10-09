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
# SDL2 Frontend

This is the SDL2 frontend for the portable PC-98 / PC-88VA build. It
links the CMake `vaeg_core`, `vaeg_va`, and `vaeg_common` targets and
includes the M10 Dear ImGui menu layer. See `../BUILD.md` for OS-level
build recipes.

## Build

```sh
cmake --preset linux-debug
cmake --build build/linux-debug --target vaeg_sdl2
```

The executable is written to:

```text
build/linux-debug/sdl2/vaeg
```

## Native CRT

The optional librashader path uses Metal on macOS, D3D11 on Windows, and
OpenGL on Linux. Setup, package layout, capture boundaries, and recovery steps
are in the [Native CRT user guide](../docs/modernization/native-crt-user-guide.md)
and [troubleshooting guide](../docs/modernization/native-crt-troubleshooting.md).
The backend ownership and raw-capture boundary are documented in the
[presentation architecture](../docs/architecture/native-crt-presentation.md).

SDL2 is discovered through `find_package(SDL2)` first, then pkg-config.
`VAEG_FETCH_SDL2=ON` is reserved for the MinGW cross preset and fetches
the pinned SDL2 release recorded in ADR-0006.

## Run

```sh
./build/linux-debug/sdl2/vaeg [options]
```

### Command-line options

| Area | Options |
|------|---------|
| Machine | `--model va|va2` |
| Sound | `--fmbackend np2|ymfm`, `--fmsound opn|opna`, `--ymfm-fidelity minimum|medium|maximum`, `--samplerate 11025|22050|44100`, `--soundbuffer 40..1000`, `--mute` |
| Media | `--fdd1 path|none`, `--fdd2 path|none`, `--fdd1-image N`, `--fdd2-image N` (disk N, from 1, of a D88 file holding several disks), `--sasi1 path|none`, `--sasi2 path|none`, `--scsi0 path|none` through `--scsi6 path|none`, `--hostfat-dir path`, `--roms path` |
| Persistence | `--cfg path`, `--no-cfg`, `--bkupmem path`, `--no-bkupmem` |
| Execution | `--cpumult 1..32`, `--sgp model|follow-cpu|1..16`, `--nowait`, `--frameskip auto|full|2|3|4` |
| Display/input | `--fullscreen`, `--windowed`, `--effect unfiltered|linear|scanline|crt-lite`, `--scaling native|fit|fit-8dot|integer|stretch`, `--controller joystick|mouse`, `--keyboard-layout jis|us|custom` |
| Diagnostics/information | `--smoke`, `--selftest`, `--debug`, `--fdctrace`, `--scsitrace`, `--pacelog`, `--trace-cpu N`, `--headless-input-script path`, `--debug-script path`, `--debug-output-dir directory`, `--screen-dump path`, `--screenshot FRAME:PATH`, `--screen-tvram-dump path`, `--version`, `--help`, `-h` |

Run `vaeg --help` for the built-in list. Enum values are ASCII
case-insensitive, and the last occurrence wins when an option is repeated.
Positional FDD arguments have been removed; use `--fdd1` and `--fdd2`.

Tape (V1/V2 BASIC, a vaeg extension: the VA itself has no cassette)
loads a raw `.cmt` or `.t88` tape image or records to a raw image;
`LOAD "CAS:"`, `BLOAD` and `SAVE "CAS:"` then use it. `--tape` and
`--tape-save` do the same from the command line. Loading and saving play
the tape's FSK sound; Sound > Tape volume sets its level separately.

Emulate > モニタ selects the monitor DIP switch (SW1): 24 kHz, or 15 kHz,
where 200-line modes show with scanlines as on a 15 kHz monitor. It
takes effect at the reset it triggers.

V1/V2 mode: Emulate > Z80 mode selects V2 H, V2 S, V1 H or V1 S (the VA's
memory switch for V1/V2; standard speed S is a vaeg extension) and resets.
An optional 32 KiB PC-8001/8801 N-BASIC ROM in the ROM directory, which the
VA itself does not carry, named `n80.1.8.rom` or `n80.1.2.rom` (known dumps,
identified by SHA-1) or `n80.rom` (any content), enables N (PC-8001) entries, which starts in N-BASIC without a disk, and lets V1 S N-88
BASIC enter N-BASIC with `NEW ON 1`. N-BASIC boots a disk left in drive 1.
See `docs/modernization/v1v2-n-basic-mode.md`.

`--headless-input-script path` starts the emulator with dummy SDL video/audio
drivers and injects commands through the normal guest keyboard path. Each
nonempty script line is submitted with Return appended; blank lines and lines
whose first non-whitespace character is `#` are ignored. `@enter` submits a
bare Return, `@wait N` waits N guest frames before continuing, and `@fdd1 PATH`
or `@fdd2 PATH` performs a normal delayed floppy replacement on the selected
drive. `@text TEXT` submits text without Return. `@hold KEY N` holds one
unmodified repeat-eligible key for N guest frames before releasing it. It accepts
lowercase letters, digits, Backspace, and the four cursor keys. The command
injects one make and one release; it does not synthesize repeats, so the guest
keyboard path under test remains responsible for repeating input. `@key NAME`
sends a mapped
guest make/break pair. Names include `ctrl-c`, `ctrl-z`, `ctrl-left`,
`ctrl-right`, `backspace`, `escape`, `up`, `down`, `left`, `right`, `home`,
`help`, `insert`, `delete`, `f1` through `f10`, `shift-f1` through `shift-f10`,
and `shift-up`, `shift-down`, `shift-left`, or `shift-right`. Chords press the
modifier before the key and release it after the key; each make/break transition
is separated by three guest frames. An active text
paste completes before the next key command. Unknown names and unavailable key
mappings fail without injecting input, and held keys are released when the
script is reset or cleared. These commands automate frontend input only; they
do not emulate guest behavior. The option does not terminate the emulator; combine it with
`VAEG_SCREEN_EXIT_MS` and `VAEG_SCREEN_TVRAM_DUMP` for a bounded TVRAM capture
run.

`--debug-script PATH --debug-output-dir DIRECTORY` runs the versioned M74
sequential debug harness. The trace-enabled build is required when a script
contains `trace`. The two options start dummy SDL video/audio and may not be
combined with `--trace-cpu` or `--headless-input-script`. A minimal script is:

```text
debug-script 1
limit-frame 3000
resource boot ../../private-media/boot.d88
counter service-entry e000:0180
wait-frame 600
enter
wait-frame 720
input-line basic
mount-fdd 1 boot
wait-pc e000:0180 1
trace service-event 128
capture service-event registers tvram screen
exit
```

The required `limit-frame` declaration is an absolute deterministic guest-frame
ceiling, including when a selected PC is never reached. `wait-frame` uses the
absolute number of completed guest frames. `input-line`
appends Return; `enter` sends Return alone. `wait-pc` counts the selected
pre-instruction `CS:IP` after it is armed. On the selected ordinal, the CPU
pauses without consuming the instruction or advancing the guest clock. A
contiguous `trace` begins with that instruction, while `capture` writes the
pre-instruction register state plus optional TVRAM and rendered BMP before the
CPU resumes. `mount-fdd` accepts only a previously declared neutral resource
identifier; `none` ejects the selected drive.

Capture IDs become deterministic filenames under the output directory:
`.registers.tsv`, `.tvram.bin`, `.screen.bmp`, and `.trace.log`. `events.tsv`
contains event and final counter rows. The logs contain neutral IDs rather than
resource paths. Keep scripts, media, ROMs, and generated captures outside Git.
For repeatable local runs, `tools/m74-diagnostics/run_debug_case.sh` accepts one
neutral case ID and resolves the worker, script directory, output root, model,
and optional ROM directory from `VAEG_M74_*` environment variables.

`--screen-dump PATH` or `VAEG_SCREEN_DUMP=PATH` captures the final SDL
render-target image after scaling, viewport, and display effects have been
rendered (BMP by default, or PNG when the path ends in `.png`).
`--screen-tvram-dump PATH` or
`VAEG_SCREEN_TVRAM_DUMP=PATH` retains the raw `VAEGSCN1` TVRAM diagnostic used
by the QA decoder. If no capture option is supplied, the normal default
window, scaling, and effect settings are unchanged.

`--screenshot FRAME:PATH` captures the rendered screen immediately after the
absolute completed guest frame `FRAME` and continues execution. The option is
repeatable, supports `.bmp` and `.png` paths (case-insensitive), and exits
normally after the highest requested frame has been captured. Requests for the
same frame are written without running another guest frame between them. Frame
numbers are guest frames, not host display frames or wall-clock times. For
example:

```sh
./vaeg \
  --model va2 \
  --roms ./roms \
  --fdd1 demo.d88 \
  --screenshot 1800:docs/images/demo.png
```

Multiple documentation images can be captured in one run:

```sh
./vaeg \
  --model va2 \
  --roms ./roms \
  --fdd1 demo.d88 \
  --screenshot 600:docs/images/boot.png \
  --screenshot 1200:docs/images/title.png \
  --screenshot 1800:docs/images/demo.png
```

`--model va` selects `88VA1` and its unsuffixed ROM set. `--model va2` selects
the `88VA2` compatibility model and its `*_va2.rom` set. The effective model
and FM hardware must be compatible, so `--model va2 --fmsound opn` is an
explicit startup error. A changed model otherwise uses the same default-sound
policy as the GUI.

Named FDD options accept existing direct image files. SASI options go further
than a file-existence check: the image is opened and accepted only when its
recognized geometry is usable through the SASI interface and the declared
sector data is present. Use `none` to make a named drive empty for the session.
An invalid media path or removed positional argument fails before SDL machine
initialization.

`--hostfat-dir` enables the read-only HOSTFAT drive for PC-Engine. Before the
machine starts, vaeg copies the selected directory into an immutable FAT12
snapshot. The M55 geometry uses 1024-byte sectors and 16 KiB clusters: its
DOS-visible size is 63.830078125 MiB and up to 63.71875 MiB of cluster payload is
allocatable before directory and per-file rounding. Valid unique ASCII 8.3
names are retained (and folded to uppercase); longer, spaced, or Unicode UTF-8
names receive deterministic 8.3 aliases. Invalid UTF-8, links/reparse points
that escape the selected root, special files, excessive depth/count, and
content that does not fit are rejected rather than omitted. On Windows, the
selected root and contained links/reparse points are canonicalized when they
remain inside the selected root.

Unpatched PC-Engine reports HOSTFAT free space as if every free FAT entry were
2 KiB, so `DIR` shows approximately 8 MiB even though 16 KiB cluster reads are
used. This is a display limitation, not the readable snapshot limit: the G55
integration check copied byte-identical data from beyond 60 MiB. PC-88VA
40 MB SASI disks use a separate built-in storage path and likewise demonstrate
16 KiB FAT12 clusters; 32 KiB clusters are not accepted by this CONFIG.SYS
driver path.

HOSTFAT can also be enabled persistently under Emulate -> Configure. Selecting
a folder and pressing OK builds the replacement snapshot on a worker thread,
leaves the current mounted image unchanged during the build, then atomically
commits it and resets the guest. Disable HOSTFAT to unmount and reset. Host
changes are intentionally invisible until this explicit rebuild. A save state
records the SHA-256 identity of the mounted image; loading with a missing or
different mounted snapshot fails transactionally before live machine state is
changed. Build and install the matching `HOSTFAT.SYS` as described in
[`tools/pc88va/hostfat/README.md`](../tools/pc88va/hostfat/README.md).

All setting and media options are session-only. They are applied after
`vaeg.cfg` is loaded and restored before its normal shutdown save as long as
the active value still matches the CLI-applied value. A setting changed
through the GUI during the run can therefore persist. A pre-existing managed
archive image remains protected from pruning while a CLI FDD override is
active.

`--pacelog` prints pacing counters once per second for jitter diagnosis.

Disk images may also be dragged onto the SDL window. One drop operation is
sorted by case-insensitive basename: the first image mounts as FDD1, the
second as FDD2, and later images are reported as ignored. Supported direct
extensions are `.d88`, `.88d`, `.d98`, `.98d`, `.fdi`, `.xdf`, `.hdm`,
`.dup`, `.2hd`, `.tfd`, and `.img`. A supported single disk image compressed as
`.d88.xz` (or another supported image suffix followed by `.xz`) is also
accepted and extracted to bounded managed storage. ZIP, 7z, and LZH drops
extract only supported images to bounded managed storage under the platform
user-state directory when LibArchive support is built. Archive mounts are saved in `FDD1FILE` and
`FDD2FILE`, so they remain valid through reset and application restart.
Unreferenced managed images are removed after eject or replacement; an image
still mounted in either drive is retained.

When a mounted image came from a ZIP, 7z, LZH, or single-image XZ stream, FDD1/FDD2 Open
starts in the directory that contained the source archive instead of exposing
the managed extraction directory. This association is kept per drive and is
restored with persistent managed mounts after an application restart.

New FDD image creates formatted MS-DOS 2HD and 2DD (640 KB) images, and
unformatted D88 images (all sectors present, filled with E5h) of 2DD
720 KB, 2D 320 KB (N88-BASIC layout) and 2D 360 KB.

The FDD Open and New FDD dialogs provide a Windows host-drive selector
above `Target Dir`; the HDD Open, New SASI, and New SCSI dialogs provide the
same selector. It lists available `C:`, `D:`, and other host drives, changes
the browser to that drive's root, and updates the New-image default filename
there. Linux and macOS retain their normal filesystem-root navigation.

FDD1/FDD2 Open also accepts ZIP, 7z, and LZH when LibArchive support is built.
Opening an archive from FDD1 mounts the first two basename-sorted images as
FDD1/FDD2. Opening from FDD2 mounts only the first image as FDD2 and leaves
FDD1 unchanged. The same extraction limits, traversal/link rejection,
persistent managed storage, and ignored-image reporting used by drag and drop
apply to menu-selected archives.

The `linux-release`, MinGW, and macOS release presets link the pinned archive
stack statically. Linux development builds use a system LibArchive when one is
available and otherwise report archive loading as unavailable.

The FDD menu can also create an empty FAT12 data disk as Japanese MS-DOS 2HD
(1.232 MB) or 2DD (640 KB). D88 preserves track and sector metadata; IMG is a
headerless raw sector image that can be accessed directly with tools such as
`mdir -i disk.img ::` and `mcopy -i disk.img`. The filename is editable,
existing files are never replaced, and the result can be mounted immediately
as FDD1 or FDD2. The generated image is formatted but does not contain MS-DOS
system files and is not bootable. 2D creation remains deferred pending a
separate compatibility audit.

Headless smoke check:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-debug/sdl2/vaeg --smoke
```

`--smoke` initializes video, audio, and the PC-98 core, runs a short fixed
frame loop, then exits with status 0 when initialization succeeds.

Normal startup displays the historical 320x200 VAEG graphic from
`assets/vaeg.bmp` for at least 1.5 seconds while continuing to process SDL
events. CMake embeds the graphic in the executable; it is not a runtime file.
ROM-less `--smoke` and `--selftest` runs skip the graphic and delay. There is
no alternate-image fallback.

CMake also embeds the byte-identical historical VAEG icon from
`assets/vaeg.ico`. The frontend decodes the embedded ICO and supplies it to
SDL as the window icon on every platform. Windows builds additionally compile
it as a native executable resource. No adjacent icon file is required.

## SASI HDD Images

SASI HDD images are configured through `vaeg.cfg`:

```ini
HDD1FILE=/path/to/disk.hdi
HDD2FILE=
```

The SDL2 GUI also exposes HardDisk -> New SASI image plus SASI-1/SASI-2
Open and Remove, and SCSI ID 0 through SCSI ID 6 Open and Remove. New SASI
image creates HDI images using the existing 5/10/15/20/30/40 MB SASI
geometry table and refuses to overwrite an existing file. Its default name is
`new-sasi-hdd.hdd`. HardDisk -> New SCSI image creates VHD-format images in
5/10/20/40/80/160 MB sizes and can assign the new image to SCSI ID 0 through
SCSI ID 6. The default names are `new-scsi-hdd_id0.hdi` through
`new-scsi-hdd_id6.hdi`; both `.hdi` and `.hdd` names are accepted for the
corresponding VHD image. New SASI is listed above New SCSI. SCSI mounting
updates `SCSIHDD0` through `SCSIHDD6`; bootable PC-Engine support-disk
assembly remains a separate documented flow.
The same SCSI images can be attached without opening the GUI, for example
with `--scsi0 disk.hdi --scsi1 none --scsi2 none --scsi3 none` and
`--scsi4 none --scsi5 none --scsi6 none`. The command line validates the VHD geometry before starting the guest and applies the
paths to the same `SCSIHDD0` through `SCSIHDD6` configuration entries.
After changing a SASI or SCSI image, reset the guest so the existing
SxSI/SASI/SCSI open and bind path is rebuilt. IDE GUI mounting is not
implemented.

For disposable guest-level storage checks, the M75 harness retains a
same-run screen and trace pair. The existing SASI/HOSTFAT smoke and HOSTFAT
read-only file-I/O checks are:

```sh
python3 tools/qa/m75_storage_regression.py --guest-io \
  --worker build/linux-debug/sdl2/vaeg \
  --support-d88 /path/to/pcengine-support-hostfat.d88 \
  --roms /path/to/roms --hostfat-drive D --exit-ms 40000
```

The SASI lifecycle check uses the actual `HDFORM.COM` from the supplied
PC-Engine 1.1 D88. It copies that D88 to the output directory, runs
`HDFORM C:`, then boots separate guest processes to create a file, close and
reopen the SASI image for byte-exact readback to A:, and delete the file. It
also confirms the 40MB SASI image and the positive free-space screen result:

```sh
python3 tools/qa/m75_storage_regression.py --sasi-format \
  --worker build/linux-debug/sdl2/vaeg \
  --support-d88 /path/to/pcengine-support-hostfat.d88 \
  --sasi-source "/path/to/PC-Engine 1.1.d88" \
  --roms /path/to/roms --output-dir /tmp/m75-sasi-format
```

The SCSI G75 check creates a disposable blank 40MiB VHD through the native
image-creation path, runs SCFORM, then performs separate guest boots for
create, close/reopen readback, and delete:

```sh
python3 tools/qa/m75_storage_regression.py --g75-scsi \
  --worker build/linux-debug/sdl2/vaeg \
  --support-d88 /path/to/pcengine-support-hostfat.d88 \
  --roms /path/to/roms --output-dir /tmp/m75-g75-scsi
```

The two-target regression uses a disposable D88 whose `CONFIG.SYS` contains
both `SCHD.SYS -I0` and `SCHD.SYS -I1`. It formats both targets, then tests
create, close/reopen readback, and delete on C: (SCSI ID 0) and D: (SCSI ID 1):

```sh
python3 tools/qa/m75_storage_regression.py --g75-scsi-two \
  --worker build/linux-debug/sdl2/vaeg \
  --support-d88 /path/to/pcengine-support-hostfat.d88 \
  --roms /path/to/roms --output-dir /tmp/m75-g75-scsi-two
```

Use `--full-g75` with `--sasi-source` to run SASI HDFORM, the one-disk SCSI
flow, and the two-target SCSI flow. The support D88, ROM directory, and
source D88 are never modified. For SCSI, the script compares read-back file
bytes with the source `SCFORM.COM`, validates both FAT copies and positive
free clusters, and checks that deleted files are absent from both guest
screens and backing images.

The support D88 must contain `HOSTFAT.SYS` in `CONFIG.SYS` for `--guest-io`;
the SCSI flow does not require HOSTFAT to be mounted. The script creates all
headless input files, validates screen/trace identity and process-exit
termination, and stores disposable captures under `--output-dir`.

The storage script divides the checks as follows:

| Automated by `m75_storage_regression.py` | Remains a human/environment gate |
|---|---|
| SASI HDFORM, create/readback/delete, and positive free-space screen | Supplying the owned ROM set and correct source/support D88 |
| SCSI blank 40MiB VHD creation, SCFORM initialization, FAT validation | Reviewing screen/trace captures when accepting a release |
| One-disk SCSI file creation and `G75TEST.COM` root/FAT verification | GUI Configure / Rebuild + reset interaction |
| Two-target SCSI ID 0/1 formatting and file-operation verification | Non-SCSI disk regression gates |
| Separate-process close/reopen readback and host byte comparison | Real hardware comparison |
| Separate-process delete and backing-image absence check | Manual hardware multi-disk comparison |
| HOSTFAT `TYPE` success and read-only `DEL` rejection | None of the disposable guest steps is a substitute for release review |

### Storage harness operation and artifacts

The harness is `tools/qa/m75_storage_regression.py`. It accepts exactly one
storage mode per invocation. `--selftest` is fixture-only and does not boot
the emulator. The guest modes require a worker executable, the support D88,
and a ROM directory; SASI modes additionally require a source D88 containing
`HDFORM.COM`.

Each guest step is run by `tools/qa/m75_scsi_harness.py` with a generated
headless-input script. The harness checks a zero worker exit, a `process-exit`
termination, and a matching run ID in the screen and trace pair. With
`--output-dir OUT`, the disposable evidence is arranged as follows:

```text
OUT/
  sasi-format/
    boot.d88  sasi.hdi
    guest/    format screen/trace/input
    create/   create screen/trace/input
    readback/ readback screen/trace/input
    delete/   delete screen/trace/input
  g75-scsi/          one-target SCSI lifecycle
  g75-scsi-two/      SCSI ID 0 and ID 1 lifecycle
```

`screen.bin` is the decoded text-plane capture, `trace.log` is the
same-run emulator trace, and `headless-input.txt` records the exact DOS
commands and waits. The JSON result on standard output includes the
per-step screen/trace digests, image digests, and byte-comparison result.
The output directory is disposable; the source D88, support D88, and ROM
files are read-only inputs.

Typical verification order is:

```sh
python3 tools/qa/m75_storage_regression.py --selftest
python3 tools/qa/test_m75_storage_regression.py -v
python3 tools/qa/m75_storage_regression.py --full-g75 \
  --worker build/linux-debug/sdl2/vaeg \
  --support-d88 /path/to/pcengine-support-hostfat.d88 \
  --sasi-source "/path/to/PC-Engine 1.1.d88" \
  --roms /path/to/roms --output-dir /tmp/m75-full-g75
```

Use `--sasi-format`, `--g75-scsi`, or `--g75-scsi-two` to rerun only one
media family. Use `--guest-io` separately for HOSTFAT `TYPE` success and
read-only delete rejection; it is intentionally not part of `--full-g75`.

## M75 Human Gate: guest storage

The automated harness is evidence for the disposable path. The M75 human
gate confirms the same operations through the normal PC-Engine frontend and
keyboard path. Run this gate with disposable copies of every D88 and disk
image; never use the source support D88 as the writable test disk.

### Preparation

1. Use the MinGW executable produced by the cross build:
   `build/mingw-cross/sdl2/vaeg.exe` relative to the checkout.
2. Prepare a support D88 containing `PCPLUS.SYS` and `SCHD.SYS`. For the
   two-target check, `CONFIG.SYS` must load both `SCHD.SYS -I0` and
   `SCHD.SYS -I1`. Keep `HOSTFAT.SYS` present when testing HOSTFAT.
3. Create disposable blank SASI and SCSI images with the GUI, or copy the
   images created by the automation into a temporary test directory.
4. Record the executable, D88/image copies, model, and configuration before
   booting. A failed test must leave the source media untouched.

### Guest storage sequence

Perform the following sequence and record the final screen after each
reset or process restart:

| Area | Operation | Pass condition |
|---|---|---|
| SASI | Run `HDFORM C:` and confirm the format | Format completes and reports positive free capacity |
| SASI | `COPY A:\HDFORM.COM C:\G75SASI.COM` | The file is listed with size 6706 bytes |
| SASI | Reset/reopen, then `COPY C:\G75SASI.COM A:\G75SASB.COM` | The A: copy is byte-identical to `HDFORM.COM` |
| SASI | `DEL C:\G75SASI.COM`, then `DIR C:` | The file is absent and free capacity increases |
| SCSI ID 0 | Format C:, create/read back/delete one file | Create, byte-identical readback, delete, and persistence succeed |
| SCSI ID 1 | Format D:, create/read back/delete one file | The second target is visible and has the same successful lifecycle |
| HOSTFAT | `TYPE D:\REGRESS.TXT` | The expected text is displayed |
| HOSTFAT | `DEL D:\REGRESS.TXT` | Delete is rejected for the read-only snapshot |

For SCSI, use `C:` for target ID 0 and `D:` for target ID 1. After creating
each file, close/reopen or reset before the readback step. Compare the
readback on the A: disk with the original source file on the host. Repeat
the final `DIR` after deletion and confirm that no stale directory entry is
shown.

### GUI reset and non-SCSI checks

Also verify the frontend path used by normal users:

- Configure a valid HOSTFAT directory and press `Rebuild + reset on OK`.
  The guest resets and the new directory is visible after reboot.
- Configure an invalid or unsupported directory. The operation reports a
  red error beside the rebuild control or in the visible status area, and
  the emulator remains restartable without deleting `vaeg.cfg` manually.
- Boot the existing non-SCSI disk path, run its normal format/read/write or
  simple file operation, and confirm that its image is unchanged by the
  SCSI/SASI tests.
- Boot the bundled VA demo and perform the standard simple OS operation
  required by the project human gate.

The human gate passes only when every row succeeds, both SCSI target IDs
remain usable after reset, and no source D88/image or configuration recovery
is required. Attach the final screen or a short recording, the tested
binary/configuration identity, and any failing image copy to the review.


## ROM Placement

ROMs are not included and must be extracted from hardware you own. Place the
selected set beside the executable:

| Model | Model ROM files |
|---|---|
| VA | `vadic.rom`, `vafont.rom`, `varom00.rom`, `varom08.rom`, `varom1.rom` |
| VA2/VA3 | `vadic_va2.rom`, `vafont_va2.rom`, `varom00_va2.rom`, `varom08_va2.rom`, `varom1_va2.rom` |

The VA2/VA3 names follow MAME's `pc88va2` `ROM_START` declaration in
[`src/mame/nec/pc88va.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp).
VA2/VA3 does not fall back to the unsuffixed VA files. Both models also use
`vasubsys.rom` as an extra: unlike MAME's currently unconnected FDD subsystem
ROM entry, vaeg executes the Z80 FDD subsystem.

After resolving a complete set, the frontend compares each file's size,
CRC32, and SHA-1 with MAME's `pc88va` or `pc88va2` declaration. The extra
`vasubsys.rom` uses the CRC32/SHA-1 from MAME's disabled FDD subsystem
declaration. A mismatch logs a warning with expected and actual values but
does not stop the emulator.

The active frontend resolves ROMs in this order:

1. the executable directory, using the filename set selected by model;
2. the current working directory, for development.

If neither complete set exists, the executable directory remains the expected
root and the frontend reports the selected model and first missing ROM. The
old `biospath` INI key is ignored by SDL2 and is no longer written.
`np2cfg.biospath` remains the shared core loader path but is derived at
runtime.

Use `Emulate -> Boot model -> VA` for `pc_model=88VA1` and unsuffixed files.
Use `VA2/VA3` for `pc_model=88VA2` with the optional 8087 disabled, or
`VA2/VA3 + 8087` for the same ROM set with the single optional 8087 enabled.
Changing the selection performs the existing reset flow and retains configured
FDD and SASI media.

## Configuration

The configuration syntax is unchanged. By default, the SDL2 frontend reads and
writes `vaeg.cfg` in the process current working directory. Relative paths are
resolved from that same directory; there is no executable-directory or user
state-directory fallback.

Use `--cfg path` to select a different configuration file. A missing selected
file starts from built-in defaults and is created when settings are normally
saved. Use `--no-cfg` to disable both configuration reads and writes for the
session. `--cfg` and `--no-cfg` are mutually exclusive.

Backup memory also defaults to the current working directory and is separated
by boot model: VA uses `vabkupmem.dat`, while VA2/VA3 uses `va2bkupmem.dat`.
Use `--bkupmem path` to override either model default, or `--no-bkupmem` to
disable both backup-memory reads and writes. These two options are mutually
exclusive. No implicit migration or fallback reads old user-state copies.

For VA/VA2 models, `Main_RAM` is the installed conventional-RAM capacity.
Menu -> デバイス -> メインメモリ容量 selects 256, 384, 512, or 640 KB and
whether to synchronize the BIOS backup-memory capacity at startup/reset:

```ini
[NekoProjectII]
Main_RAM=640
Main_RAM_Auto=true
```

Defaults are 640 KB and automatic synchronization ON. Existing valid Main_RAM
values are preserved; a missing Main_RAM_Auto key defaults to ON. Unsupported
capacities produce a warning and use 640 KB. Menu changes are saved immediately
to the active vaeg.cfg (including a --cfg override), but installed RAM and
backup-memory synchronization change together at the next reset. The menu shows
the currently applied capacity. Failed configuration writes leave the previous
file/settings intact and display an error; --no-cfg prevents menu persistence.

With automatic synchronization ON, startup/reset updates only the capacity bits
and their checksum in an existing backup image, preserving other BIOS settings.
A missing or truncated image receives the existing initial BIOS record when ON;
when OFF, it starts cleared without seeding a capacity record. With OFF, the
emulator performs no capacity correction, including the former missing-file
initialization; the guest BIOS remains free to modify backup memory. This allows
intentional discrepancies for diagnostics. No continuous correction runs while
the guest is executing. Normal backup-file saving persists the resulting state;
--no-bkupmem still disables backup-file I/O, not the in-memory synchronization.

Addresses above the applied capacity are unavailable to the guest CPU. BMS/EMS
and MEMswtch do not define or increase installed conventional memory. Automatic
synchronization is an emulator convenience, not a replacement for guest RAM
measurement. Only resetting applies pending changes; save-state loading does
not itself synchronize the backup-memory record.

Obsolete `np2.cfg`, `np2.ini`, and `vaeg.ini` files are not read. Fixed GUI
save-state slots and keyboard sidecars remain in the user state directory.

## Mouse Input

`Device -> Mouse` separates three settings: host pointer capture, the VA
controller-port device, and rapid buttons. Capture uses SDL relative mouse
mode and feeds movement plus active-low left/right buttons through the
existing guest mouse interface. It does not write guest coordinates or BIOS
state directly.

Select `VA controller port -> Mouse` for VA mouse software; `Joystick` keeps
the original controller-pad path. `Capture mouse` traps the host pointer only
while the vaeg window has focus and Dear ImGui is not using the mouse. F12
toggles capture when `Keyboard -> F12 binding -> Mouse` is selected. The same
menu can route F12 to the guest PC key. Middle
click also toggles capture outside the GUI. Focus loss, reset, state load, and
shutdown release both guest buttons and pending movement.

The settings use the original-compatible `vaeg.cfg` keys:

```ini
Mouse_sw=0
Mouse_VA=0
MS_RAPID=0
```

`Mouse_VA=0` selects joystick and `Mouse_VA=1` selects mouse. Existing
configuration files without these keys remain uncaptured in joystick mode.

## Clipboard Copy and Paste

`Edit -> Copy screen text` copies the visible logical TVRAM text to the host
clipboard. Cmd+C is supported on macOS; Ctrl+Shift+C is supported on
Linux/Windows. HCCODE ASCII, half-width kana, and JIS kanji are converted to
UTF-8, with trailing line spaces removed.
Copy uses the rendered 80-/40-column viewport rather than the frame's two
horizontal guard cells, and collapses paired left/right HCCODE kanji cells.

`Edit -> Paste` sends host clipboard text to the guest as paced keyboard
make/break input. The shortcut is Command+V on macOS and Control+V on
Linux/Windows. Printable ASCII and CR/LF line breaks are supported; CRLF is
one Return. Each make or break transition is separated by 20 ms. Unsupported
control characters and non-ASCII UTF-8 code points are skipped and counted in
the Edit menu.

The queue finishes an in-flight key release, then pauses while Dear ImGui
captures keyboard/text input or a modal is open. Reset, state load, focus
loss, quit, and explicit Cancel Paste stop the queue and release synthetic
keys. Paste does not access guest memory or text buffers. Japanese/IME paste
and guest-to-host copy is provided by Copy screen text.

## Display

The SDL2 window is resizable and keeps the guest framebuffer fixed at
640x400 RGB565. Screen -> Scaling selects Native, Fit, Fit 8-dot, Integer, or
Stretch. Aspect correction is independent of scaling. Native/x2/x3 remain
window-size presets, and Custom accepts a logical width and height. High-DPI
drawable dimensions are calculated separately and are never saved as logical
window size.

Screen -> Effect selects Unfiltered, Linear, Scanline, or CRT Lite. Scanline
and CRT Lite are procedural SDL_Renderer overlays aligned to the 400-line
guest raster. CRT Lite adds a restrained RGB pattern and edge darkening. No
MAME renderer code, shader, LUT, mask texture, or artwork is used.

Screen provides immediate Windowed and Exclusive fullscreen choices. Exclusive
fullscreen uses the current desktop resolution on the saved monitor, so no
separate Apply step or resolution setup is required. Failed transitions roll
back to Windowed. The backend retains the legacy `fscrn_cx`, `fscrn_cy`, and
hexadecimal `fscrnmod` fields, but detailed monitor/mode selection and
Borderless desktop are not exposed in the current GUI.

`画面 -> スクリーンショットを保存` writes the most recent 640x400 guest frame as a
PNG directly in the current working directory; it never contains the host GUI
menu or overlays. Names include the local time, the SDL tick count, and a
collision suffix, so existing captures are never overwritten. `PrintScreen`
invokes the same host action and is no longer sent to the guest COPY key.
`デバイス -> キーボード -> F12 binding` can also assign the same action to F12;
existing F12 selections remain unchanged.

## Execution Speed And Pacing

`Emulate -> Configure...` keeps the VA base clock fixed at 3.9936 MHz and
sets independent execution capacity for the V30 and SGP. CPU x2 is the
standard 7.9872 MHz setting for VA, VA2, and VA3. CPU x1-x32 changes only the
amount of V30 work available per unit of machine time. Unlike the CPU, the
SGP model-default clock differs by model.

SGP speed has three modes:

- `Model default`: 3.9936 MHz for VA and 7.9872 MHz for VA2/VA3,
  independent of CPU;
- `Follow CPU`: scales Model default by `clk_mult / 2`;
- `Custom`: scales Model default by an integer x1-x16.

These nominal clocks correspond to the 4 MHz and 8 MHz model clocks recorded
in [Inside PC-88VA Wiki section 4.4.6](http://www.pc88.gr.jp/inside88va/wiki/index.php?%A5%B0%A5%E9%A5%D5%A5%A3%A5%C3%A5%AF).
The GUI displays both the relative scale and effective clock. CPU and SGP
changes reset the guest through the media-preserving reset path. The settings
are stored as:

```ini
clk_base=3993600
clk_mult=2
sgp_mode=0
sgp_mult=1
```

CPU or SGP scaling does not change VBlank/TSP timing, sound pitch and timers,
FDD timing, RTC, or normal one-to-one host pacing. The `Emulate` menu's speed
slider paces guest frames at 10-400 % of real time in 10 % steps
(`s_speed`); its right end is No Wait (`s_NOWAIT`), which removes host
waiting. Sound follows the guest, so it breaks up below 100 % and above
it (the BEEP worst, as it is built from CPU-timed on/off events). To slow
only the program, use the `CPU 速度` slider below it (`CPU_Speed`, 10-100 %
in 5 % steps): every instruction cycle costs 100 / speed machine clocks, in
V3 (on top of the Configure multiplier) and in the V1/V2 compatible mode,
while frames, timers, FM, BEEP and disk timing keep real time.
`Screen -> Frame skip` selects Auto, Full frame, 1/2, 1/3, or
1/4 presentation without changing guest time. Holding the configured
`Fast forward` keyboard action (F11 by default) temporarily uses No Wait and
draw skip 16; releasing it, losing focus, resetting, loading a state, or
quitting clears the temporary mode. The active host action is not sent to the
guest, and the saved No Wait/frame-skip/CPU/SGP values are not overwritten.
The Keyboard -> F12 binding menu can select `Full speed (No Wait)` as an
alternative hold-to-fast-forward shortcut; in that mode F12 uses the same
temporary path and is not sent to the guest.

`Info -> Show FPS`, `Show CPU clock`, `Show SGP clock`, and `Show frame`
independently control the corresponding suffixes in the native window title.
For a new configuration, CPU and SGP clock display are enabled while FPS and
frame display are off; saved `DspClock` flags are preserved. The measured clocks
are the configured effective clocks scaled by guest frames completed per host
second relative to the nominal 60Hz guest frame rate, so No Wait and F11 show
the actual extra throughput. The measurement uses guest frames rather than
rendered frames and is refreshed approximately once per second; it does not
change frame skip or guest timing. `Info -> Show text`, `Show sprite`, `Show
graphics 0`, and `Show graphics 1` independently enable the four VA composition
layers; graphics 0 and graphics 1 are the VA's two graphics planes. These layer
switches are frontend display filters and do not modify guest VRAM or video
registers. `Info -> Show video info overlay` reports the logical graphics
state (`ON`/`OFF`, logical size, and bpp). `Info -> Show FB info overlay`
lists all four VA framebuffer descriptors vertically (`FB0` through `FB3`).
Each active descriptor is split into `source` (virtual source geometry), `view`
(visible sub-screen geometry), and `DSA` (display source address); unavailable
descriptors are shown as `OFF`. `Info -> About` opens the version and runtime
information dialog.

## OPN/OPNA FM Backend

The Sound menu exposes `FM sound backend -> NP2` and
`FM sound backend -> ymfm`.
The selection is saved in the selected `vaeg.cfg` as:

```ini
opn_backend=ymfm
ymfm_fidelity=minimum
```

`ymfm` is the default and selects the BSD-3-Clause ymfm YM2203/YM2608 FM
operator implementation. Select `np2` for the established NP2 sound behavior.
The backend change performs the normal GUI reset so the selected synthesizer
starts from a fully replayed board state; mounted FDD/SASI paths are retained.
Timer/IRQ, SSG, ADPCM, rhythm, board I/O, and final mixing remain on the NP2
path in this stage. Missing or unknown configuration values fall back to `ymfm`.

This backend choice is independent of emulated sound hardware. The Sound
menu also exposes `FM sound OPN/OPNA`: VA defaults to its built-in YM2203/OPN
(`SNDboard=100`) and can select Sound Board II YM2608/OPNA
(`SNDboard=200`). VA2/VA3 defaults to YM2608/OPNA; its OPN-only choice is
disabled. Hardware changes reset the guest and preserve mounted media.

`Sound on/off` pauses or resumes host audio output without removing the
selected guest OPN/OPNA hardware. The choice is stored separately as
`sound_enabled`; `SNDboard` always remains a valid hardware value so FM timer
polling software continues to run while output is muted.

The Sound menu also selects the mixed output rate and requested buffer length
for both FM backends. Supported rates are 11.025, 22.05, and 44.1 kHz;
22.05 kHz remains the compatibility default, while the GUI recommends 44.1
kHz for new configurations. Buffer presets are 40, 100, 200, 500, and 1000 ms,
with custom values accepted from 40 through 1000 ms. These persist as
`SampleHz` and `Latencys` and rebuild the SDL audio device through the normal
media-preserving guest reset.

When ymfm is selected, `ymfm_fidelity` can be `minimum`, `medium`, or
`maximum`. It controls ymfm's native YM2203/YM2608 generation rate before box
downsampling; Maximum has the highest CPU cost. Minimum preserves the previous
behavior and is the fallback for missing or unknown values. NP2 has no
equivalent control, so the fidelity menu is disabled for that backend.

## VA Configuration Requirements

For PC-88VA booting, check these keys in the selected configuration:

- `pc_model=88VA1` or `pc_model=88VA2`: non-VA models can halt at V2.
- `SNDboard=100` for VA built-in OPN, or `200` for VA Sound Board II.
- `SNDboard=200` for VA2/VA3 built-in OPNA. Other values can leave sound
  hardware unbound and cause a silent hang in FM-timer waits.
- `clk_base=3993600`; `clk_mult=2` is standard, while x1-x32 selects V30
  execution capacity without changing machine/peripheral time.
- `PacingMs=0` disables extra host pacing. Values from 1 through 1000 defer
  guest frames by that many milliseconds while the frontend continues to poll
  input and render ImGui. This is useful for reading transient boot messages;
  it does not change emulated CPU clock accounting.

The frontend logs prominent warnings for stale VA sound-board or clock
settings. It never rewrites the user's configuration silently.

## Keyboard Mapping

The SDL2 keyboard path is scancode based. The default host layout is
`keyboard_host_layout=jis`; `us` is a US-keytop preset for text entry,
and `custom` stores GUI-edited bindings as SDL scancode names in the
user-state sidecar `keyboard.map`.
`keyboard_custom_map=file:keyboard.map` in the selected configuration
points to that
sidecar.

Device / Keyboard in the ImGui menu exposes:

- Host layout: JIS physical, US keytop, Custom
- Kana input: JIS Kana, Roman Kana
- Tenkey overlay: maps YUI/HJK/NM,. to guest keypad 789/456/123/0
- Full key binding table with capture-next-key

JIS physical maps host scancode position to PC-88VA physical key
position. US keytop maps printable US punctuation keytops/chords to
guest keys or guest Shift chords that produce the intended ASCII symbol.
The tenkey overlay is a game-oriented mode for tenkeyless keyboards and
is independent of the host layout preset. No Unicode or text-buffer
injection is used. Set `VAEG_KBD_TRACE=1` to log keyboard event routing
and selected guest actions. On Windows GUI builds, the same trace is written
to `vaeg-kbd-trace.log` in the current directory; set
`VAEG_KBD_TRACE_FILE` to choose another path. For example, in PowerShell:

```powershell
$env:VAEG_KBD_TRACE = "1"
$env:VAEG_KBD_TRACE_FILE = "$pwd\kbdtrace.txt"
.\vaeg.exe
Get-Content .\kbdtrace.txt
```

Roman Kana parses A-Z and apostrophe host scancodes and emits the same
guest keyboard make/break sequence as physical keys. It never injects
Unicode, CP932, BIOS buffers, DOS buffers, RAM, or VRAM. When ImGui
captures keyboard or text input, neither raw keys nor Roman Kana output
reach the guest. The menu selects the kana input method only. Enter and
leave guest kana mode with the assigned KANA key, which defaults to
`RightAlt`: one press locks KANA, the next press unlocks it. When the
menu is set to Roman Kana and KANA is locked, A-Z host scancodes feed the
helper and are not sent as direct alphabetic guest keys; when KANA is
unlocked, A-Z is normal guest input.

The PC key defaults to `ScrollLock`. VA2/3 use PC-held reset or power-on
for the BIOS setup path, and some VA popup helpers use PC key chords such
as PC+D. See `docs/modernization/keyboard-mapping.md` for the full
inventory and evidence table.

## Font Manager Stub

Host GUI text is rendered by Dear ImGui using
the build-time embedded `assets/NotoSansJP-Regular.ttf`; it does not use
an external runtime font or `sdl2/fontmng.c`.
The SDL2 `fontmng` stub remains linked because the shared core still
builds `font/fontmake.c`, whose `makepc98bmp()` path references
`fontmng_create()`, `fontmng_get()`, and `fontmng_destroy()`. Removing the
stub leaves those symbols unresolved. The current SDL2 consumers are
therefore `CMakeLists.txt` and the shared `font/fontmake.c` link path.
