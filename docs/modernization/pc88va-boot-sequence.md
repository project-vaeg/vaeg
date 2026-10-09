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

# PC-88VA Boot Sequence Notes

## Scope

The original PC-88VA and the VA2 use different ROMs and have different
observable no-media behavior. This document therefore keeps their boot
sequences separate. VA facts are not silently generalized to VA2/VA3, and
VA2 observations are not used to fill gaps in the original-VA sequence.

The VA3 has not been independently analyzed. The active emulator currently
uses the VA2 ROM set and `PCMODEL_VA2` behavior for the VA2/VA3 selection;
that implementation policy is not proof that VA3 firmware behavior is
identical.

Detailed uPD9002 `BRKEM2`, `CALLN`, `RETEM`, and uPD70008-compatible-mode
analysis is kept in [upd9002-upd70008-mode.md](upd9002-upd70008-mode.md).

## Sources And Evidence Levels

| Source | Use | Authority |
| ------ | --- | --------- |
| [*PC-88VA Technical Manual*, page 12](https://archive.org/details/PC88VA/page/12/mode/2up) | Original-VA power-on flow, PC-key and SW7 decisions, V1/V2 fallback, V3 IPL load address, and native-service overview. | Published technical-manual flowchart; original VA only unless stated otherwise. |
| Original-VA `varom1.rom` static disassembly | Reset entry, early initialization, PC-key test, SW7 test, V3 IPL routine, setup entry, and V1/V2 fallback. | Direct firmware evidence from a maintainer-owned ROM, not distributed here. |
| VA2 `varom1_va2.rom` static disassembly | VA2 reset entry, device initialization, vectors, `BRKEM2`, and later native path. | Direct firmware evidence from a maintainer-owned ROM, not distributed here. |
| Active emulator source | Memory mapping and current emulation of keyboard matrix, system ports, ROM banks, TSP, I/O traps, PIT, PIC, and DMAC. | Implementation evidence, not hardware specification. |
| Maintainer hardware observations | Original-VA and VA2/VA3 no-media behavior. | Human gate evidence; useful where the ROM path has not yet been fully traced. |

The ROMs used for the static analysis are identified here so results are not
accidentally attributed to another dump:

| Model | Active filename | SHA-1 | SHA-256 |
| ----- | --------------- | ----- | ------- |
| Original VA | `varom1.rom` | `54536dc03238b4668c8bb76337efade001ec7826` | `a9c4bf7239c44912de421767ae137f5b144d3cb973f6dc947dbce1fe194fd894` |
| VA2/VA3 selection | `varom1_va2.rom` | `dd4f4521bfbb068f15ab3bcdb8d47c7d82b9d1d4` | `ed32a33d19c0a8312791cca4fc39b70cf233c9a9c80f72c778a687ecfbf4f271` |

## Shared Reset Mechanics

Both analyzed ROMs use the same V30 reset-vector mechanism. The active core
selects V30/uPD9002 native execution for VA models and initializes:

```text
CS:IP = F000:FFF0
physical address = FFFF0h
```

Physical `F0000h-FFFFFh` is ROM1. The model-selected ROM is loaded by
[bios/romva.c](../../bios/romva.c) and read through the ROM1 mapping in
[memoryva/memoryva.c](../../memoryva/memoryva.c). Both analyzed ROMs contain:

```asm
F000:FFF0  EA 00 00 00 F0    jmp F000:0000
```

The first jump at `F000:0000` is model-specific:

```text
Original VA  F000:FFF0 -> F000:0000 -> F000:0A6F
VA2          F000:FFF0 -> F000:0000 -> F000:12B8
```

Only the reset-vector mechanism is shared in the discussion below.

## Original PC-88VA (VA)

### Documented Decision Flow

Page 12 of the [technical-manual scan](https://archive.org/details/PC88VA/page/12/mode/2up)
gives this original-VA power-on flow:

```text
Power on
  |
  v
Initialize hardware for V1/V2
  |
  v
PC key held?
  |
  +-- yes --> V3 setup initialization
  |             |
  |             v
  |           BIOS initialization
  |
  +-- no --> SW7 on?
                |
                +-- no --> Configure IDP/TSP for V1/V2
                |            Enable I/O traps
                |            Jump to V1/V2
                |              |
                |              v
                |            N88-BASIC from ROM or disk
                |
                +-- yes --> Floppy has a V3 IPL?
                             |
                             +-- no --> Configure IDP/TSP for V1/V2
                             |            Enable I/O traps
                             |            Jump to V1/V2
                             |
                             +-- yes --> Load one IPL sector at 3000:0000
                                          Initialize V3 hardware and BIOS
                                          Jump to IPL at 3000:0000
                                            |
                                            v
                                          DOS boot
```

The original-VA ROM confirms the major decisions and identifies their code
locations.

### Reset Entry And Initial V1/V2 State

The original-VA ROM starts its practical reset body with:

```asm
F000:0000  E9 6C 0A    jmp 0A6Fh

0A6F: mov ax,0040h
0A72: mov ds,ax
0A74: mov sp,0000h
```

Before testing the PC key, the ROM performs the common V1/V2-oriented
initialization described by the flowchart:

1. It writes the uPD9002 internal-control table at ROM offset `0930h`.
2. It prepares the two I/O-trap ranges at `FFE0h-FFE7h`.
3. It initializes PIT, PIC, DMAC, and related ports from the table at `0953h`.
4. It configures memory-control state through ports including `0152h` and
   `0155h`.
5. The routine reached through `0BA9h` sends a TSP `SYNC` command before the
   PC-key decision.
6. The routine at `0B51h` prepares the low interrupt-vector table.

The internal-control and trap-range writes are:

```text
FFFE=11 FFFD=07 FFFC=01 FFFB=60 FFFA=88 FFF9=A0
FFF6=08 FFF5=80 FFF4=53
FFF2=08
FFF0=00
FFE7=00 FFE6=6F FFE5=00 FFE4=60
FFE3=00 FFE2=5B FFE1=00 FFE0=50
```

This prepares V1/V2 compatibility, but it does not yet enable trapping at
`FFEFh`; trap enable occurs only on the selected V1/V2 fallback path.

### PC-Key Test

The PC-key decision is direct keyboard-matrix I/O:

```asm
0AEA: call 0B4Ah
0AED: jz   0B36h

0B4A: mov dx,000Dh
0B4D: in  al,dx
0B4E: and al,04h
0B50: ret
```

The active keyboard implementation confirms the matrix meaning. In
[io/serial.c](../../io/serial.c), guest PC-key scancode `5Ah` is converted to
VA code `7Ah`, whose matrix position is port `Dh`, bit 2. Matrix keys are
active-low. Consequently:

```text
port 0Dh bit 2 = 0  -> PC key held -> branch to 0B36h
port 0Dh bit 2 = 1  -> PC key clear -> continue to SW7 decision
```

`keyboard_reset()` preserves the matrix across reset, which is required for
holding PC while resetting the emulated machine.

### PC-Key Setup Path

The held-PC path is:

```asm
0B36: mov byte [2F07h],01h
0B3B: mov al,91h
0B3D: out FFh,al
0B3F: mov ax,3000h
0B42: mov ss,ax
0B44: call 18B0h
0B47: jmp  0209h
```

`18B0h` runs common V3/BIOS initialization. The dispatcher at `0209h` then
far-calls `E000:B800`. The early port `0152h` setup has selected ROM0 bank 2,
so this address maps through [io/memctrlva.c](../../io/memctrlva.c) to
`varom00.rom` offset `2B800h`. That location jumps to `E000:C451`, where the
ROM initializes the setup program. Nearby setup data contains entries for
`SW7`, `FDD`, and `ROM` configuration.

This is static ROM confirmation that holding PC enters the original VA V3
setup path; it is not only an inference from the manual diagram.

### SW7 And V3 IPL Search

When PC is not held, the ROM reads SW7 from system port `40h`, bit 3:

```asm
0AEF: in   al,40h
0AF1: and  al,08h
0AF3: jnz  0AF8h
0AF5: call 16A0h
```

The branch and the technical-manual flow together establish active-low switch
semantics:

```text
SW7 ON  -> port 40h bit 3 = 0 -> call 16A0h and search for a V3 IPL
SW7 OFF -> port 40h bit 3 = 1 -> skip the search and use V1/V2
```

The routine at `16A0h` communicates with the FDD subsystem and tests candidate
boot conditions. A successful path reaches `1743h` and does not return to the
caller. Failure returns to `0AF8h`, the same V1/V2 fallback used when SW7 is
off.

The exact on-disk V3-IPL signature is not decoded yet. The routine is located,
but the meaning of all FDD-subsystem commands and candidate values still needs
to be matched against the subsystem protocol.

### V3 IPL Load And Entry

The successful original-VA V3 path is explicit:

```asm
1748: mov cx,0200h
174F: shr dl,1
1751: jnc 175Ah
1753: shl cx,1
175A: mov ax,3000h
175D: mov es,ax
175F: xor di,di
1761: call 1771h
1764: mov ax,3000h
1767: mov ss,ax
1769: call 18B0h
176C: jmp  3000h:0000h
```

The ROM normally prepares a `0200h`-byte transfer and can double it to
`0400h` according to the detected sector form. It therefore loads one 512-
or 1024-byte IPL sector at physical address `30000h`, performs common V3/BIOS
initialization, and enters it at `3000:0000`.

The ROM does not execute `mov sp,FFFEh`. It set `SP=0000h` at `0A74h`, changes
only `SS` before the common initializer, and reaches the far jump with balanced
calls. Thus the IPL entry state is effectively:

```text
CS:IP = 3000:0000
SS:SP = 3000:0000
first PUSH uses 3000:FFFE
```

The manual's stack indication near `3000:FFFE` describes the first occupied
stack location rather than a literal `SP=FFFEh` assignment.

### V1/V2 Fallback

SW7 OFF and unsuccessful V3-IPL search both converge at `0AF8h`:

```text
0AF8  configure the display/TSP path for V1/V2
0AFB  call 0FCCh -> write 03h to FFEFh and enable I/O traps
0AFE  call 0EE7h -> issue TSP command 12h (DSPON)
0B15  execute BRKEM2 90h
```

This confirms the manual's separation between display-processor setup and
uPD9002 I/O-trap enable. They are adjacent boot operations, but they are not
one hardware block. `BRKEM2 90h` then transfers the main uPD9002 into its
uPD70008-compatible execution mode (historically uPD780/Z80-compatible). It must not be routed to the separate
FDD subsystem uPD780C.

### Original-VA Emulator Gap: SW7

The active [io/sysportva.c](../../io/sysportva.c) labels port `40h` bit 3
as SW7, but `sysp_i040()` does not currently include a configurable bit 3 in
its return value. The emulated bit is therefore always zero, which is the
original-VA SW7 ON state. The ROM always attempts V3 IPL detection and falls
back to V1/V2 only after that attempt fails.

This does not prevent fallback, but it means an explicit original-VA SW7 OFF
configuration cannot currently be represented. The old PC-98-oriented
`np2cfg.dipsw` fields must not be connected to this VA switch without a
separate configuration decision and tests.

Status (M103b, maintainer decision): an SW7 GUI/INI option was added and then
withdrawn. vaeg offers no user-facing V1/V2 selection and follows the ROM's
automatic selection from the boot media (traced for the VA2 in
[`v1v2-mode-plan.md`](v1v2-mode-plan.md) §5.2). SW7 therefore stays zero; an
original-VA SW7 OFF configuration would need a new decision.

### Original-VA Remaining Unknowns

- Exact V3 IPL recognition bytes and checks performed by the FDD subsystem.
- Exact meaning of every uPD9002 internal-control register written before the
  PC-key decision.
- Cycle-accurate timing of the PC-key sampling point and FDD retry loop.
- Behavior of an original VA fitted with the VA-91 upgrade board.

## PC-88VA2 / PC-88VA3

### Scope And Confidence Boundary

This section is a partial VA2 ROM trace. It does not claim a complete VA2 boot
algorithm. The VA3 has not been independently traced, and is mentioned only
because the current frontend exposes a combined VA2/VA3 model selection.

In particular, the original-VA `16A0h` IPL routine, `3000:0000` load details,
and V1/V2 fallback behavior must not be copied into the VA2/VA3 model without
VA2-specific evidence.

### VA2 Reset Entry

The VA2 ROM starts with:

```asm
F000:0000  E9 B5 12    jmp 12B8h
```

The body at `12B8h` sets `DS=0040h`, clears `SP`, and begins display, memory,
backup-RAM, uPD9002, and peripheral initialization.

### VA2 Early Device Initialization

The VA2 ROM performs these statically visible operations:

- Sends an initial parameter block through TSP ports `0142h` and `0146h`.
- Accesses ROM-bank control port `0152h` and backup-memory write control
  `019Ah`.
- Reads backup-memory/memory-switch state near `B000:1FC0`.
- Writes mode-switch state through `01C6h`.
- Emits the uPD9002 internal-control and I/O-trap range table at ROM offset
  `0F20h`.
- Initializes PIT, PIC, and DMAC ports from the table at `0F43h`.

The high internal-control writes are:

```text
FFFE=11 FFFD=07 FFFC=01 FFFB=60 FFFA=88 FFF9=A0
FFF6=08 FFF5=80 FFF4=53
FFF2=08
FFF0=00
FFE7=00 FFE6=6F FFE5=00 FFE4=60
FFE3=00 FFE2=5B FFE1=00 FFE0=50
```

These values match the VA2-specific settings summarized from the local
technical notes in
[upd9002-upd70008-mode.md](upd9002-upd70008-mode.md). A complete public
uPD9002 register manual has not been located. Public V40/V50 documentation is
only a family analogy for the integrated DMA, interrupt, timer, serial, and
wait-control model.

### VA2 PC-Key And SW7 Neighborhood

The VA2 ROM installs vectors, then reaches this decision neighborhood:

```asm
1364: call 13EDh       ; install vectors
1367: call 13E6h       ; read port 000Dh, mask bit 2
136A: jz   13D2h
136C: in   al,40h
136E: and  al,08h
1370: jnz  1375h
1372: call 1F90h
```

The port `0Dh` test uses the same active-low keyboard-matrix PC-key bit as the
original VA. The structural similarity makes `1F90h` a VA2 boot-media probe
candidate, but that routine has not been decoded to the same standard as the
original-VA `16A0h` path. Its media test, success destination, and failure
behavior remain open.

The `13D2h` PC-held branch sets a setup-path work flag, writes `91h` to port
`FFh`, and enters shared later initialization instead of executing the nearby
`BRKEM2 90h`. The final VA2 setup UI target has not yet been traced end to end.

### VA2 BRKEM2 Handoff

The VA2 ROM override table at `0F5Eh` installs these relevant vectors:

```text
7C -> F000:1944
7D -> F000:1944
7E -> F000:1920
90 -> 1000:0000
91 -> F000:24B0
95 -> 1000:E000
```

On the non-PC branch, the ROM can enable I/O traps and reaches:

```asm
13A8: mov ax,1000h
13AB: mov ds,ax
13AD: xor ax,ax
13AF: mov es,ax
13B1: 0F FE 90       ; BRKEM2 90h
13B4: cli            ; expected native return address
```

The vector setup and instruction boundary make the opcode identity high
confidence. By the documented uPD9002 model transition and V30 analogy, the
working interpretation is that `BRKEM2 90h` enters main-CPU uPD70008-compatible
code at `1000:0000` and saves `13B4h` as the native return address.

The exact saved frame, mode-latch protection, interrupt interaction, prefetch
behavior, and cycle timing remain unverified. See
[upd9002-upd70008-mode.md](upd9002-upd70008-mode.md) for the detailed boundary.

### VA2 Native Resume And Later Initialization

The bytes after `BRKEM2` form coherent V30 code:

```asm
13B4: cli
13B5: call 18EEh      ; disable I/O traps through FFEFh
13BD: mov dx,0152h
13C0: in  ax,dx
13C1: or  ax,4000h
13C4: out dx,ax
13CD: jmp 1000h:C003h
```

The shared initializer configures PIC and DMAC state, installs BIOS/service
vectors, clears low work memory, checks backup memory, sends TSP commands,
and calls BIOS interrupts and ROM services. The target `1000:C003` is physical
`1C003h`, not ROM1 offset `C003h`.

The technical manual confirms `RETEM` as `ED FD` and describes
`1000:C003` as the destination after V1/V2-to-V3 initialization. However, the
VA2-compatible-mode code that prepares this RAM target and executes the return
has not yet been located in a runtime trace.

### VA2 Display And No-Media Behavior

The VA2 ROM writes mode-switch and system-port state through `01C6h`, `01CDh`,
and `01CFh`; the emulator derives V1/V2/V3 LEDs from this state. A plain ASCII
`V2S` string has not been found in `varom1_va2.rom`, so the exact drawing
routine remains unidentified.

Maintainer observation reports that VA2/VA3-class machines stop at a prompt
equivalent to "insert a system disk" when no suitable disk is present. This is
not the original VA's documented V1/V2 fallback. The exact VA2 ROM branch that
draws and waits at that prompt has not yet been traced.

### VA2/VA3 Remaining Unknowns

- Full semantics and success path of the VA2 routine at `1F90h`.
- Exact VA2 V3-IPL recognition algorithm and load address.
- Complete PC-held setup path after the `13D2h` branch.
- Exact no-media prompt routine and wait loop.
- Runtime producer of RAM code at `1000:C003`.
- Main-CPU uPD70008-compatible-mode execution and precise mode transitions.
- Any VA3-specific firmware, device, or no-media differences.

## Cross-Model Comparison

| Topic | Original VA | VA2 | VA3 |
| ----- | ----------- | --- | --- |
| ROM1 active filename | `varom1.rom` | `varom1_va2.rom` | Currently shares VA2 file in vaeg |
| First reset body | `F000:0A6F` | `F000:12B8` | Not independently traced |
| PC-key matrix test | Port `0Dh`, bit 2, active-low; confirmed | Same port/bit visible; confirmed | Unknown |
| PC-held setup target | `E000:B800` -> ROM0 bank 2 setup code; confirmed | Branch visible, final UI target unresolved | Unknown |
| SW7 read | Port `40h`, bit 3, active-low; confirmed with manual | Same read visible; downstream behavior incomplete | Unknown |
| V3 media probe | `16A0h`; success/failure control flow traced | `1F90h` candidate; incomplete | Unknown |
| V3 IPL load | One 512/1024-byte sector to `3000:0000`; confirmed | Unknown | Unknown |
| No V3 IPL | Falls back to V1/V2 | Hardware observation: system-disk prompt | Hardware observation: system-disk prompt |
| Compatible-mode entry | `BRKEM2 90h` at `0B15h` | `BRKEM2 90h` at `13B1h` | Unknown |
| Later V3 target | Setup path is known; normal return path still partly open | `1000:C003` visible after native resume | Unknown |

## Model Identification

VA software can distinguish the model using the word at `F000:FFFE`:

```text
FFFFh  original PC-88VA
FFFEh  PC-88VA2 or PC-88VA3
FFFDh  original PC-88VA with the PC-88VA-91 version-up board
```

This is why model-specific ROM paths and hardware behavior must remain
separate even when the emulator exposes a combined VA2/VA3 selection.

## Confidence Summary

| Item | Confidence | Basis |
| ---- | ---------- | ----- |
| Shared reset vector | High | Direct bytes in both identified ROMs and active memory mapping. |
| Original-VA PC-key branch | High | Direct ROM I/O test plus active-low keyboard-matrix mapping. |
| Original-VA setup entry | High | Direct branch, ROM-bank mapping, setup entry code, and nearby setup data. |
| Original-VA SW7 polarity and decision | High | ROM branch agrees with the technical-manual flowchart. |
| Original-VA IPL load and entry | High | Direct transfer loop, segment setup, and far jump. |
| Original-VA IPL signature | Low | Routine located, FDD command semantics not fully decoded. |
| VA2 early initialization | High | Direct ROM and I/O-port observations. |
| VA2 `BRKEM2 90h` identity | High | Vector setup and valid instruction boundary. |
| VA2 media probe and IPL entry | Low | Candidate routine located only by control-flow position; not fully decoded. |
| VA2 no-media prompt | Medium as observed behavior; low for ROM location | Maintainer observation without an identified drawing/wait routine. |
| VA3 sequence | Low | No independent ROM or runtime trace. |

## Emulator Work Suggested By This Analysis

1. ~~Add a VA-specific SW7 representation and return it from port `40h`
   bit 3.~~ Withdrawn in M103b: V1/V2 follows the automatic media selection
   (see above).
2. Add ROM-less tests for PC-key active-low matrix behavior across reset.
3. Decode the original-VA FDD command sequence enough to identify the V3 IPL
   signature.
4. ~~Trace VA2 routine `1F90h` before implementing a VA2 media decision from
   analogy.~~ Done in M103b ([`v1v2-mode-plan.md`](v1v2-mode-plan.md) §5.2).
5. Keep VA and VA2/VA3 boot-policy code paths explicit; do not hide unknown
   VA2/VA3 behavior behind the now-understood original-VA path.
