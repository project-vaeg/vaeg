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

# PC-88VA PC-Engine 1.00 Boot Analysis

## Scope

This document follows one concrete original-PC-88VA boot from the VA ROM IPL
loader into NEC PC-Engine 1.00. This release is important because its boot
layout is the earliest of the four analyzed PC-Engine system disks and is
not simply the later PC-Engine 1.05 IPL with older driver files.

The analysis separates:

1. the original-VA ROM boot decision;
2. the disk IPL loaded at `3000:0000`;
3. the complete 7168-byte `ENGINEIO.SYS` loaded at `1340:0000`;
4. the original-VA ROM hooks installed by `ENGINEIO.SYS`;
5. the later DOS device loading of `PCENGINE.SYS` and `ADVGBIOS.SYS`;
6. the ROM-side OS initialization entered through `F000:940D`.

The preceding ROM decision, including the PC key, SW7, V3 IPL detection, and
V1/V2 fallback, is documented in
[pc88va-boot-sequence.md](pc88va-boot-sequence.md). The original-VA flowchart
is from the [*PC-88VA Technical Manual*, page 12](https://archive.org/details/PC88VA/page/12/mode/2up).

The later disk analyses are:

- [Sound Board II system disk](pc88va-boot-pcengine-soundboard2.md)
- [PC-Engine 1.05](pc88va-boot-pcengine1.05.md)
- [PC-Engine 1.1](pc88va-boot-pcengine1.1.md)

This is static analysis of a maintainer-owned disk and ROM. Neither payload is
distributed by this repository.

## Analyzed Media

| Item | Value |
| ---- | ----- |
| Disk filename | `PC-Engine 1.00.d88` |
| D88 internal name | `PC-Engine 1.0` |
| File size | 1,331,888 bytes |
| SHA-1 | `0a125556876f17061060d806c70672dc2abe9b92` |
| SHA-256 | `87fe82bdeccd4c41c10c6e4dcaa8c0c4089cca7ca783e8b4c3cc11c9c040ab03` |
| D88 write-protect byte | `00h` |
| D88 disk type | `20h` (2HD) |
| Original-VA ROM | `varom1.rom` |
| ROM SHA-1 | `54536dc03238b4668c8bb76337efade001ec7826` |

The D88 structures were interpreted according to
[fdd/d88head.h](../../fdd/d88head.h). The active loader computes the requested
transfer size as `128 << N`, as shown in
[fdd/fdd_d88.c](../../fdd/fdd_d88.c).

All conclusions in this document apply to the exact disk hash above. A D88
with the same label but a different hash must be audited independently.

## Disk Geometry And Error Status

The image has 160 populated D88 track entries, C0/H0 through C79/H1. Every
track contains R1 through R8 with the following form:

```text
N=3
sectors per track=8
stored data size=0400h
D88 status=00h
```

The complete normal area is therefore:

```text
80 cylinders * 2 heads * 8 sectors * 1024 bytes = 1,310,720 bytes
```

All 1280 sectors have D88 status `00h`. No sector in this exact image carries
a stored D88 CRC or controller-error status. This describes the image's D88
metadata; it is not a claim about the history or quality of another dump.

Unlike the analyzed PC-Engine 1.05 and 1.1 D88 files, this image has no extra
C80/H0 track containing status-`10h` sectors. That container-level difference
must not by itself be interpreted as a PC-Engine software requirement.

## Boot Sector Form

The original-VA ROM loads C0/H0/R1, a 1024-byte sector, at `3000:0000` and
enters it there. The extracted sector has:

| Region | SHA-256 |
| ------ | ------- |
| Complete 1024-byte sector | `b935bba809f333536853ee7e4b9412967509135e7c9c224c7df9fa64b65e2d04` |
| First 512 bytes | `3d0ba37da095fa1ed619d25ab1c9f8a7b180fbd6bfb97828b171c30476edbac9` |

The executable IPL occupies offsets `0000h-0115h`. Bytes `0116h-031Fh` are
zero. The tail at `0320h-03FFh` contains nonzero data, including runs of
`FFh` and word-oriented values, so the second half is not equivalent to the
zero-filled second half in PC-Engine 1.05 and 1.1.

The IPL passes `3000:0200` to a ROM disk-service routine as a control or
parameter area. The nonzero tail lies inside that broad work region, but the
exact contract and which tail fields are consumed or replaced have not been
fully decoded. The top-level IPL never branches into this region, so it must
not be disassembled as executable V30 code.

## FAT12 Layout

The filesystem uses 1024-byte sectors and one-sector clusters. The first FAT
starts with media byte `FEh`. The boot-relevant layout is:

| LBA | Contents |
| ---: | -------- |
| 0 | 1024-byte IPL sector |
| 1-2 | FAT copy 1 |
| 3-4 | FAT copy 2 |
| 5-10 | Root directory |
| 11-17 | `ENGINEIO.SYS` |
| 18-56 | `PCENGINE.SYS` |
| 57-72 | `ADVGBIOS.SYS` |
| 73 | `PCENGINE.COM` |

The root directory and FAT12 chains identify:

| File | Attributes | Start cluster | Size | FAT chain |
| ---- | ---------: | ------------: | ---: | --------- |
| `ENGINEIO.SYS` | `27h` | 2 | 7,168 bytes | `2 -> 3 -> ... -> 8 -> FFFh` |
| `PCENGINE.SYS` | `27h` | 9 | 38,996 bytes | `9 -> 10 -> ... -> 47 -> FFFh` |
| `ADVGBIOS.SYS` | `27h` | 48 | 16,356 bytes | `48 -> 49 -> ... -> 63 -> FFFh` |
| `PCENGINE.COM` | `21h` | 64 | 5 bytes | `64 -> FFFh` |
| `NECGAIJI.DAT` | `20h` | 65 | 8,664 bytes | `65 -> 66 -> ... -> 73 -> FFFh` |
| `CHKDSK.COM` | `20h` | 74 | 6,923 bytes | `74 -> 75 -> ... -> 80 -> FFFh` |
| `HDFORM.COM` | `20h` | 81 | 3,750 bytes | `81 -> 82 -> 83 -> 84 -> FFFh` |
| `VA11SAMP.BAS` | `20h` | 85 | 8,349 bytes | `85 -> 86 -> ... -> 93 -> FFFh` |

The boot components have the following reproducible hashes:

| File | SHA-256 |
| ---- | ------- |
| `ENGINEIO.SYS` | `ca526aadbba4ff3cafcadf9f712d8798c1fcd032827cb3d07336af06b8ea4998` |
| `PCENGINE.SYS` | `0c18fff8fb7209bc46cf053675809c6c325e497ae40c11043d3af338b5ae8a6f` |
| `ADVGBIOS.SYS` | `8d9b646deec147744763b1bd4eb3693705ed1b0a9e5f5e6d6457ffe8957000bc` |
| `PCENGINE.COM` | `bfbcdfa65eee2acd963cce8036c2c708fb46772a0ce156a807f0e5f98af8c163` |

## IPL Entry At 3000:0000

The first instructions establish a temporary stack in the IPL segment:

```asm
3000:0000  mov  ax,3000h
3000:0003  mov  ds,ax
3000:0005  cli
3000:0006  mov  ss,ax
3000:0008  mov  sp,1FFEh
3000:000B  sti
```

The complete top-level flow is:

```asm
3000:000C  call 00DEh       ; enable required state through port 018Ah
3000:000F  call 00E8h       ; set work segment and probe writable memory

3000:0012  cli
3000:0013  sub  bp,1000h    ; reserve stack below detected memory limit
3000:0017  mov  ss,bp
3000:0019  mov  sp,FFFEh
3000:001C  sti

3000:001D  call 002Dh       ; prepare disk state and load ENGINEIO.SYS
3000:0020  jc   002Ch       ; stop on load failure
3000:0022  call 1340:0000   ; install original-VA integration hooks
3000:0027  call F000:940D   ; enter ROM-side OS initialization
3000:002C  hlt              ; terminal path if control returns
```

This differs from the later IPL at the first call target after the common
stack prologue. Only the first 13 bytes are common with PC-Engine 1.05; the
rest is a distinct implementation. PC-Engine 1.00 also ends its terminal path
with `HLT`, whereas the later IPL loops at its terminal address.

## Work Segment And Memory Probe

The routine at `3000:00E8` establishes two values in the ROM work area:

```asm
mov es,1040h
mov word [es:05F8h],1500h
mov word [es:05FAh],0400h
```

Both the IPL and `ENGINEIO.SYS` later read `1040:05F8` to select segment
`1500h` for their shared work state.

The same routine probes writable memory in `2000h`-paragraph, or 128-KiB,
steps. It begins with `BP=4000h`, writes and reads a boundary value near the
top of each candidate block, and leaves the last successful boundary in
`BP`. The top-level IPL then subtracts `1000h` paragraphs and places the
permanent stack at `SS:FFFE` below that detected limit.

The port setup at `3000:00DE` performs a read-modify-write on port `018Ah`,
setting bit 0 while interrupts are disabled:

```asm
cli
mov dx,018Ah
in  al,dx
or  al,01h
out dx,al
sti
```

The precise hardware name of this bit is not assigned here without a primary
register definition.

## ROM Disk-Service Setup

At `3000:002D`, the IPL first loads `DS=1500h` through the pointer at
`1040:05F8`. It then prepares these work references:

| Work field | Value | Purpose established by use |
| ---------- | ----- | -------------------------- |
| `1500:0010` | `0200h` | offset of ROM disk parameter area |
| `1500:0012` | `3000h` | segment of ROM disk parameter area |
| `1500:0068` | `0400h` | offset of a second ROM disk work area |
| `1500:006A` | `3000h` | segment of the second work area |

The resulting far pointers are `3000:0200` and `3000:0400`. The IPL selects
the ROM descriptor at `F000:9410` or `F000:9420` according to the byte at
`0040:2F0F`, stores the descriptor in the first parameter block, and calls
`F000:DCFA` to populate media-dependent fields.

## Fixed Seven-Sector Stage Read

Unlike PC-Engine 1.05 and 1.1, the PC-Engine 1.00 IPL does not derive the
sector count by dividing `1C00h` by the detected bytes per sector. It writes a
literal count of seven:

```asm
les bx,[0010h]             ; parameter block at 3000:0200
mov ax,[es:bx+14h]         ; first data-sector LBA
mov [037Bh],ax
mov word [0379h],0007h     ; fixed seven-sector transfer
mov word [0375h],0000h
mov word [0377h],1340h
```

The far call to `F000:F110` performs the transfer:

```text
first data LBA = 11
read count     = 7 sectors
sector size    = 1024 bytes
transfer size  = 1C00h bytes
destination    = 1340:0000
```

LBA 11 through 17 are exactly the seven FAT clusters occupied by
`ENGINEIO.SYS`. Therefore, the initial transfer contains:

```text
7168 bytes of ENGINEIO.SYS
0 bytes of PCENGINE.SYS
```

This is the central structural difference from the later disks. Their
4096-byte `ENGINEIO.SYS` leaves three sectors of the same `1C00h` transfer for
the beginning of `PCENGINE.SYS`; PC-Engine 1.00 fills the entire transfer with
its padded integration driver.

After the transfer, the IPL performs one of two short `INT 80h` sequences
according to `0040:2F0F`. Their exact service names and contracts remain
unresolved.

## Resulting Memory Layout

| Guest address | Physical range | Contents before ROM handoff |
| ------------- | -------------- | ---------------------------- |
| `1340:0000-1BFF` | `13400h-14FFFh` | Complete 7168-byte `ENGINEIO.SYS` |
| `1500:0000` onward | `15000h` onward | Shared IPL/ROM/ENGINEIO work state |
| `3000:0000-03FF` | `30000h-303FFh` | Original 1024-byte IPL sector |
| `3000:0400` onward | `30400h` onward | Secondary ROM disk work area |

The end of `ENGINEIO.SYS` at physical `14FFFh` is immediately below the work
segment at physical `15000h`. The file's last nonzero byte is at offset
`0C65h`; offsets `0C66h-1BFFh` are zero padding. Its directory length still
matches the IPL's seven-sector transfer exactly.

No part of `PCENGINE.SYS` is present at `1340:1000` in this release. Treating
that address as the PC-Engine device header, as in the later disk layout,
would instead point into the middle of the loaded `ENGINEIO.SYS` allocation.

## ENGINEIO.SYS Entry At 1340:0000

The entry first checks the model word at `F000:FFFE`:

```asm
1340:0000  call 000Ch
1340:0003  jnz  000Bh
1340:0005  call 001Bh
1340:0008  call 003Fh
1340:000B  retf

1340:000C  mov  di,F000h
1340:000F  mov  es,di
1340:0011  mov  di,FFFEh
1340:0014  mov  ax,[es:di]
1340:0017  cmp  ax,FFFFh
1340:001A  ret
```

`FFFFh` identifies the original PC-88VA. On that model, the routine at
`1340:001B` writes far-jump stubs into segment `1088h`, and the routine at
`1340:003F` connects those stubs to ROM-side work fields. On a VA2/VA3 model
identifier, the entry skips both original-VA patch routines and returns.

The first 88 bytes of this file are byte-for-byte identical to the beginning
of the PC-Engine 1.05/1.1 `ENGINEIO.SYS`, including the model test and the
far-jump-writing loop. The handler table and subsequent implementation differ.

### Original-VA Hook Table

PC-Engine 1.00 installs 25 far-jump stubs. Each stub is written in segment
`1088h` as opcode `EAh`, a handler offset, and the current `ENGINEIO.SYS`
segment (`1340h`):

| Stub | Handler | Stub | Handler |
| ---- | ------- | ---- | ------- |
| `1088:0014` | `1340:00D9` | `1088:0019` | `1340:00E0` |
| `1088:003C` | `1340:00ED` | `1088:004B` | `1340:011A` |
| `1088:007D` | `1340:02AD` | `1088:008C` | `1340:03C4` |
| `1088:0096` | `1340:0411` | `1088:00A0` | `1340:0476` |
| `1088:00AF` | `1340:04B6` | `1088:00B4` | `1340:04CC` |
| `1088:00C3` | `1340:04F6` | `1088:00C8` | `1340:05C9` |
| `1088:00CD` | `1340:069A` | `1088:00D2` | `1340:0710` |
| `1088:00D7` | `1340:07B9` | `1088:00DC` | `1340:07DA` |
| `1088:00E6` | `1340:088F` | `1088:00EB` | `1340:08C4` |
| `1088:00F5` | `1340:0907` | `1088:0104` | `1340:091E` |
| `1088:0109` | `1340:0981` | `1088:010E` | `1340:0B33` |
| `1088:0140` | `1340:0B59` | `1088:015E` | `1340:0B8D` |
| `1088:0163` | `1340:0BE6` | - | - |

The table proves that `ENGINEIO.SYS` is a resident compatibility and
integration layer rather than a one-shot disk loader. Assigning names to all
25 intercepted ROM paths requires a symbol map or a complete ROM call audit.

## Synthetic CONFIG.SYS Input

`ENGINEIO.SYS` contains these exact ASCII strings at offsets `01EEh-0223h`:

```text
DEVICE=PCENGINE.SYS\r\n
DEVICE=ADVGBIOS.SYS\r\n
\CONFIG.SYS\0
```

The routine at `1340:0224` copies exactly `002Ah` bytes, the two 21-byte
`DEVICE=` lines, into another segment and sets up a pointer to the
`\CONFIG.SYS` name before calling ROM services through the installed
trampoline. This is direct static evidence that the later device loading is
fed through the ROM/DOS configuration path.

It also explains why the IPL does not need to load `PCENGINE.SYS` itself:

1. the IPL loads and enters `ENGINEIO.SYS`;
2. `ENGINEIO.SYS` installs ROM hooks and configuration data;
3. the IPL enters ROM-side OS initialization through `F000:940D`;
4. the configuration path later loads `PCENGINE.SYS` and `ADVGBIOS.SYS` as
   DOS character-device drivers.

The exact sequence of ROM calls that allocates the text buffer and submits it
to the configuration parser still requires a runtime trace. The embedded
text, fixed copy length, and destination setup establish the mechanism without
requiring that every private ROM entry point be named.

## PCENGINE.SYS Device Header

Although it is not part of the initial seven-sector transfer, the full
`PCENGINE.SYS` begins with a standard DOS character-device header:

```text
next device pointer = FFFF:FFFF
attributes          = 8000h
strategy entry      = 0016h
interrupt entry     = 0021h
device name         = "__ENGINE"
```

At offset `0016h`, the strategy entry stores the request-header pointer from
`ES:BX`. The interrupt entry at `0021h` saves registers, reloads that request
pointer, examines the command byte at request offset `+2`, and dispatches the
driver operation.

The header and entry offsets match PC-Engine 1.05, although the file sizes and
driver bodies differ. PC-Engine 1.1 instead records `C197h` and `C1A2h` in its
on-disk header.

## ADVGBIOS.SYS And PCENGINE.COM

`ADVGBIOS.SYS` also begins with a DOS character-device header:

```text
next device pointer = FFFF:FFFF
attributes          = 8000h
strategy entry      = 0016h
interrupt entry     = 0021h
device name         = "ADVGBIOS"
```

Its embedded identification string is:

```text
Graphics BIOS Ver 1.00 87/03/01 (with ROM03,04,05) by h.godai & y.shimizu
```

`PCENGINE.COM` is only five bytes:

```asm
jmp F000:0003
```

The same five-byte command stub appears in the later analyzed disks. It is a
ROM entry front end and is not part of the initial boot transfer.

## ROM-Side OS Handoff

After `ENGINEIO.SYS` returns, the IPL calls `F000:940D`. In the analyzed
original-VA ROM this entry jumps to `F000:B61C`, which performs broad ROM-side
OS initialization: work-area setup, memory sizing, interrupt-vector setup,
device-table setup, and subsequent initialization calls.

The statically established chain is:

```text
Original-VA ROM boot decision
  -> load C0/H0/R1 at 3000:0000
  -> execute the PC-Engine 1.00 IPL
  -> establish work segment 1500h and probe memory
  -> read LBA 11-17 at 1340:0000
  -> execute complete ENGINEIO.SYS at 1340:0000
  -> install 25 original-VA ROM hooks in segment 1088h
  -> prepare DEVICE=PCENGINE.SYS and DEVICE=ADVGBIOS.SYS input
  -> enter ROM-side OS initialization through F000:940D
  -> load the two DOS device drivers through the later configuration path
  -> continue PC-Engine 1.00 startup
```

Thus `3000:0000` is the disk IPL, while `1340:0000` is the resident integration
layer. The main PC-Engine driver is not present in memory merely because the
IPL's initial stage read has completed.

## Comparison With Later Releases

| Property | PC-Engine 1.00 | Sound Board II system disk | PC-Engine 1.05 | PC-Engine 1.1 |
| -------- | -------------- | -------------------------- | -------------- | ------------- |
| Populated D88 tracks | 160 | 160 | 161 | 161 |
| Stored D88 error-status sectors | None | None | 26 on C80/H0 | 26 on C80/H0 |
| IPL implementation | Original 1.00 path | Distinct Sound Board II-era path | Later common path | Same executable path as 1.05 |
| IPL end on return | `HLT` | `HLT` | Infinite loop | Infinite loop |
| Stage read count | Literal 7 | `1C00h / sector size` | `1C00h / sector size` | `1C00h / sector size` |
| Stage destination | `1340:0000` | `1340:0000` | `1340:0000` | `1340:0000` |
| `ENGINEIO.SYS` size | 7,168 bytes | 4,096 bytes | 4,096 bytes | 4,096 bytes |
| Initial PCENGINE bytes | 0 | 3,072 | 3,072 | 3,072 |
| Original-VA hook count | 25 | 26 | 26 | 26 |
| `PCENGINE.SYS` size | 38,996 bytes | 59,956 bytes | 52,090 bytes | 62,347 bytes |
| Strategy/interrupt entries | `0016h` / `0021h` | `0016h` / `0021h` | `0016h` / `0021h` | `C197h` / `C1A2h` |
| `ADVGBIOS.SYS` size | 16,356 bytes | 16,356 bytes | 30,956 bytes | 16,364 bytes |
| Graphics BIOS string | `Ver 1.00 87/03/01` | `Ver 1.00 87/03/01` | `Ver 1.12 87/03/10` | `Ver 1.10 87/04/22` |
| Sound Board II driver form | Not present | Integrated into `PCENGINE.SYS` | Separate `PCENGINE.SB2` | Not present |
| Direct OPNA `0046h/0047h` access | None | Present | Present in `PCENGINE.SB2`, absent from normal `PCENGINE.SYS` | Not analyzed |

The extra C80/H0 tracks in the two later D88 containers may reflect imaging
or conversion history. The table records them as observed container facts and
does not treat them as proof of a release-level disk-format change.

The major boot evolution is instead visible in ordinary filesystem data:

- 1.00 pads `ENGINEIO.SYS` to the entire seven-sector initial load;
- the Sound Board II, 1.05, and 1.1 disks reduce it to four sectors;
- the freed three sectors expose the beginning of `PCENGINE.SYS` during the
  initial transfer;
- all four still use the embedded configuration mechanism to name
  `PCENGINE.SYS` and `ADVGBIOS.SYS` for later device loading.

## Confirmed And Unresolved Points

### Confirmed By Static Evidence

- All 1280 sectors in the exact analyzed D88 have status `00h`.
- The ROM loads a valid 1024-byte C0/H0/R1 sector at `3000:0000`.
- The 1.00 IPL is distinct from the later 1.05/1.1 implementation.
- The IPL establishes segment `1500h` as shared work state and probes memory
  in 128-KiB steps before installing the permanent stack.
- The IPL requests exactly seven sectors from first data LBA 11 to
  `1340:0000`.
- Those seven sectors are exactly the complete 7168-byte `ENGINEIO.SYS`.
- No bytes of `PCENGINE.SYS` are included in the initial transfer.
- `ENGINEIO.SYS` checks the original-VA model word and installs 25 far-jump
  hooks in segment `1088h` on that model.
- `ENGINEIO.SYS` copies two embedded `DEVICE=` lines for the later
  configuration path.
- `PCENGINE.SYS` and `ADVGBIOS.SYS` are DOS character-device drivers.
- The IPL then enters ROM-side initialization through `F000:940D`.

### Still Requiring Runtime Trace Or Additional Documentation

- The complete field-level contract of the blocks at `3000:0200` and
  `3000:0400`.
- Which nonzero bytes at IPL offsets `0320h-03FFh` are consumed by the ROM.
- Exact names and contracts of `F000:DCFA`, `F000:F110`, and every private ROM
  service reached through the `ENGINEIO.SYS` trampoline.
- Exact meaning of the IPL's post-read `INT 80h` operations.
- The precise call sequence from `F000:940D` to configuration parsing and DOS
  device initialization.
- The final non-returning transfer into the running PC-Engine 1.00
  environment.
- The complete VA2/VA3 path after `ENGINEIO.SYS` skips the original-VA hook
  installation.
