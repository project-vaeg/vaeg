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
# Virtual Machine Architecture

This note describes how the portable vaeg build constructs and runs the
emulated machine. It is a code-structure note, not a hardware manual. For
the current PC-88VA ROM-side boot analysis, see
`docs/modernization/pc88va-boot-sequence.md`.

## Overall Shape

vaeg currently builds one virtual machine per process. It does not allocate
a single explicit `VM` object. Instead, the machine is assembled from the
global singleton state inherited from Neko Project II:

```text
SDL2 frontend
  -> pccore reset/run
    -> CPU core: cpu/upd9002
    -> memory: uPD9002 memory dispatcher plus PC-88VA memoryva layer
    -> I/O: io, cbus, FDC, DMAC, sound boards
    -> video: vram, vramva, scrndraw/scrndrawva
    -> sound: fm/psg/adpcm/beep/FDD motor sources
    -> storage: FDD and SXSI/SASI/SCSI image layers
    -> ROM: BIOS and PC-88VA ROM loaders
```

This makes the current design simple to keep faithful to the legacy code,
but it also means multi-instance emulation would require a larger state
ownership redesign.

## Host Startup

The portable entry point is `sdl2/np2.c:main()`. The startup order is:

1. Parse command-line options and optional FDD image paths.
2. Initialize SDL and the portable file/path layer.
3. Load `vaeg.cfg` through `initload()`.
4. Validate any positional FDD images.
5. Create the SDL window, renderer, and ImGui GUI.
6. Initialize host-facing managers: sound, communication, system, task,
   keyboard/input, and tracing.
7. Call `pccore_init()` to construct the emulator core.
8. Load writable PC-88VA backup memory.
9. Call `pccore_reset()` to put the virtual machine into reset state.
10. Call `scrndraw_redraw()` so the initial PC-88VA palette and draw
    tables exist after reset.
11. Mount command-line FDD images.
12. Enter the frame loop.

Configuration lookup prefers `vaeg.cfg` beside the executable before
checking the platform user-state directory. Obsolete NP2 filenames and
`vaeg.ini` are intentionally not read.

Backup-memory lookup follows the same ordering: an existing
`vabkupmem.dat` beside the executable is loaded and saved in place;
otherwise the per-user copy is used.

The important implementation range is `sdl2/np2.c:599-662`.

## Configuration to Machine State

`np2cfg` is the persistent configuration. `pccore_set()` converts it into
the runtime `pccore` model state:

- `pc_model=88VA1` selects `PCMODEL_VA1`.
- `pc_model=88VA2` selects `PCMODEL_VA2`.
- Non-VA models leave `pccore.model_va = PCMODEL_NOTVA`.
- `clk_base` and `clk_mult` become `pccore.baseclock`, `pccore.multiple`,
  and `pccore.realclock`.
- `SNDboard` becomes `pccore.sound`, which determines which sound board
  binds during reset. `100` selects the VA built-in YM2203/OPN path;
  `200` selects the YM2608/OPNA path used by VA Sound Board II and VA2/VA3.
- DIP switches and memory size affect CPU mode, extension memory, and HDD
  interface selection.

The conversion lives in `machine/pccore.c:140-219`.

## Core Construction

`pccore_init()` constructs the long-lived emulator subsystems before any
guest reset:

- CPU core initialization.
- Palette and draw-table initialization.
- PC-88VA text, sprite, graphics, and subsystem helpers. The active
  CMake/SDL2 tree is always built with PC-88VA support.
- FDD, SXSI, font, GDC, sound, serial, MIDI, PC-9861K, and I/O core
  construction.

This is in `machine/pccore.c:264-301`. It is mostly allocation and table setup;
it is not the hardware reset state yet.

## Reset and Hardware Binding

`pccore_reset()` is the virtual machine reset. It clears RAM and VRAM,
restores memory switch bytes, recomputes the runtime model, resets the
event queue, and forces V30 mode for PC-88VA models.

After that it opens storage image state, resets sound/FDD/I/O/CBUS/FM
state, builds memory maps, binds I/O handlers, initializes video/palette
state, loads BIOS ROMs, and sets the CPU reset vector:

```text
CS = F000h
IP = FFF0h
physical = 0xFFFF0
```

The reset implementation is `machine/pccore.c:360-480`.

## ROM and Memory

For PC-88VA, `romva_initialize()` selects a model-specific ROM filename set
and loads it into the emulator buffers:

| Buffer | VA | VA2/VA3 |
|---|---|---|
| `fontmem` | `vafont.rom` | `vafont_va2.rom` |
| `dicmem` | `vadic.rom` | `vadic_va2.rom` |
| `rom0mem` banks 0-7 | `varom00.rom` | `varom00_va2.rom` |
| `rom0mem` banks 8-9 | `varom08.rom` | `varom08_va2.rom` |
| `rom1mem` | `varom1.rom` | `varom1_va2.rom` |
| FDD subsystem ROM | `vasubsys.rom` | `vasubsys.rom` |

VA2/VA3 uses the existing `PCMODEL_VA2` core model and the MAME-compatible
`pc88va2` filenames. It does not fall back to the unsuffixed VA ROM set, which
prevents same-named but model-specific ROM data from being mixed. VA3 currently
shares the VA2 ROM set and emulation path. `vasubsys.rom` is a separate vaeg
extra shared by both models rather than one of MAME's five active model ROMs.

The filename selection and loader are in
[bios/romva.c](../../bios/romva.c); the expected MAME CRC32/SHA-1 values are
recorded in `docs/agents/tasks/M18_rom_layout.md`.

The installed memory differs between the original VA and VA2/VA3:

| Resource | VA | VA2/VA3 |
|---|---:|---:|
| Main RAM | 512KB | 512KB |
| Backup RAM | 8KB | 16KB |
| Text VRAM (also used for sprites) | 64KB | 256KB |
| Graphics VRAM | 256KB | 256KB |
| Sound RAM/VRAM | none | 256KB |

The local technical-manual material describes the original VA text VRAM as
64KB at `A0000H-AFFFFH`. NEC's product specifications independently list
64KB for the [PC-88VA](https://support.nec-lavie.jp/support/product/data/spec/cpu/b047-1.html)
and 256KB for both the
[PC-88VA2](https://support.nec-lavie.jp/support/product/data/spec/cpu/b048-1.html)
and [PC-88VA3](https://support.nec-lavie.jp/support/product/data/spec/cpu/b049-1.html).
The [PC-88VA hardware comparison](http://www.pc88.gr.jp/~va/va-hard.html#mem)
records the same split and also identifies the backup-RAM and sound-RAM
differences shown above.

Consequently, the active memory layer exposes only `A0000H-AFFFFH` as TVRAM
for VA1 and treats `B0000H-DFFFFH` as open bus. VA2/VA3 uses the complete
256KB `A0000H-DFFFFH` bank-1 backing. The VA2/VA3 address extent is the active
implementation's mapping of the documented capacity and is covered by the
VA2 V3 BASIC regression test; the cited product specifications state the
capacity but do not themselves document the CPU address decode.

The main CPU memory entry points use the active `upd9002_*` internal names.
When the VA memory mode is active, `cpu/upd9002/memory.c` dispatches memory
accesses into `memoryva/memoryva.c`:

```text
upd9002_memoryread()     -> upd9002_memoryread_va()
upd9002_memoryread_w()   -> upd9002_memoryread_va_w()
upd9002_memorywrite()    -> upd9002_memorywrite_va()
upd9002_memorywrite_w()  -> upd9002_memorywrite_va_w()
```

The switch points are `cpu/upd9002/memory.c:805-930`. The PC-88VA map routines
are in `memoryva/memoryva.c:770-812`.

This is why the reset vector at physical `0xFFFF0` reads from `VAROM1.ROM`
for a VA machine.

## I/O and Devices

The I/O core is rebuilt and rebound on reset:

```text
iocore_build()
iocore_bind()
cbuscore_bind()
fmboard_bind()
```

These calls are in `machine/pccore.c:451-454`. The PC-88VA extended I/O table is
implemented under `io/`; the common and VA registration maps are owned by
`io/iocore.c`, whose canonical `iocore_out*`/`iocore_inp*` dispatcher selects
the VA map through `iomode_va`. VA registrations use `iocore_attachvaout` and
`iocore_attachvainp`; the two maps remain distinct because common and VA
devices may share port numbers with different handlers.

Sound-hardware configuration matters for PC-88VA boot. VA defaults to
`SNDboard=100` (built-in OPN) and may select `200` for Sound Board II.
VA2/VA3 defaults to and requires `SNDboard=200` (OPNA). The emulator warns
about invalid model/sound combinations at SDL2 startup. Selecting a boot
model in the GUI applies that model's default; changing sound hardware
performs a guest reset so the I/O ports and sound streams are rebound.

## Frame Execution

The frame loop calls `pccore_exec(draw)` for guest time. Inside
`pccore_exec()`:

1. The display/vsync event is scheduled. PC-98 uses GDC timing; PC-88VA
   uses the VA screen-display event path.
2. The CPU runs until the frame event clears `screendispflag`.
3. The scheduler calls `v30c_step()` directly for each instruction. No active
   build or runtime state can select an 80286 instruction dispatcher.
4. For PC-88VA, the FDD subsystem and SGP are stepped alongside the main
   CPU.
5. After the frame, disk, calendar, S98, and sound callbacks run.

Normal initialization and reset always establish V30/uPD9002 native state.
The legacy serialized `cpu_type` byte remains for save compatibility and is
validated only by the state adapter; it is not runtime control. CPU_SHUT alone
preserves its historical 286-style register-initializer output, including the
recorded upper-FLAGS anomaly, but still returns to the same V30 dispatcher.

The SGP model-default execution clock is 3.9936 MHz for VA and 7.9872 MHz for
VA2/VA3, corresponding to the 4 MHz and 8 MHz clocks documented in
[Inside PC-88VA Wiki section 4.4.6](http://www.pc88.gr.jp/inside88va/wiki/index.php?%A5%B0%A5%E9%A5%D5%A5%A3%A5%C3%A5%AF).
The V30/uPD9002 CPU model default is 7.9872 MHz for all three models. CPU
multiplier changes do not alter the SGP default; Follow CPU and Custom SGP
modes explicitly scale it.

The key implementation is `pccore_exec()` in `machine/pccore.c`.

## PC-88VA Guest Boot Path

Once reset completes, the guest is executing PC-88VA ROM code, not a
frontend boot script. The current working model is:

```text
1. Reset selects V30/uPD9002 native mode.
2. CPU starts at F000:FFF0.
3. The VA memory map routes that address to VAROM1.ROM.
4. VAROM1.ROM:FFF0 jumps to F000:0000.
5. F000:0000 jumps through the ROM table to F000:12B8.
6. ROM code initializes display/TSP, video, ROM banks, backup RAM,
   uPD9002 control ports, I/O traps, PIT, PIC, and DMAC.
7. The visible V2S/mode-lamp phase is expected around the system-port and
   mode-switch updates in this early V30 setup.
8. A likely BRKEM2 handoff path enters the uPD9002 uPD70008-compatible
   mode (historically uPD780/Z80-compatible) through `0F FE 90`.
9. ROM-side BIOS/device initialization continues and eventually reaches
   boot media access.
```

The exact BRKEM2 return path and the producer of the later RAM code at
`1000:C003` remain open investigation items.
The emulator currently has a
shared Z80 compatibility backend for the FDD uPD780C subsystem
(`cpu/z80_compat_cpu.cpp`), but it does not yet emulate the main CPU's
uPD70008-compatible mode entered by BRKEM2.

## Practical Consequences

- Changing machine model at runtime must be followed by `pccore_reset()`
  and a redraw-table rebuild.
- ROM availability affects real VA boot, but ROM-less smoke mode can still
  test SDL/core/GUI startup.
- VA storage behavior depends on both the FDC path and the DMA/bank path,
  so FDD debugging usually needs FDC, DMAC, and bank traces together.
- The frozen legacy v141 tier remains useful as a behavioral reference for
  reset, V30, DMA, and FDC sequencing.
