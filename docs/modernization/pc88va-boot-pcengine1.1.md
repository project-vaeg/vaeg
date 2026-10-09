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

# PC-88VA PC-Engine 1.1 Boot Analysis

## Scope

This document follows one concrete original-PC-88VA boot from the VA ROM IPL
loader into NEC PC-Engine 1.1. It uses the same analysis boundary as the
[PC-Engine 1.05 boot analysis](pc88va-boot-pcengine1.05.md):

1. the original-VA ROM boot decision;
2. the disk IPL loaded at `3000:0000`;
3. `ENGINEIO.SYS` and the beginning of `PCENGINE.SYS` loaded at `1340:0000`;
4. the ROM-side OS initialization entered through `F000:940D`.

The preceding ROM decision, including the PC key, SW7, V3 IPL detection, and
V1/V2 fallback, is documented in
[pc88va-boot-sequence.md](pc88va-boot-sequence.md). The original-VA flowchart
is from the [*PC-88VA Technical Manual*, page 12](https://archive.org/details/PC88VA/page/12/mode/2up).

This is static analysis of a maintainer-owned disk and ROM. Neither payload is
distributed by this repository.

## Analyzed Media

| Item | Value |
| ---- | ----- |
| Disk filename | `PC-Engine 1.1.d88` |
| D88 internal name | `HxCFE` |
| File size | 1,338,960 bytes |
| SHA-1 | `b3a6d968aeccbef63dcb0351f9f4efa7f272aac5` |
| SHA-256 | `47215b9c81c05093c61349f928177dc02345d644a60fb6c9ebff17c8b16a4f7a` |
| D88 write-protect byte | `00h` |
| D88 disk type | `20h` (2HD) |
| Original-VA ROM | `varom1.rom` |
| ROM SHA-1 | `54536dc03238b4668c8bb76337efade001ec7826` |

The D88 structures were interpreted according to
[fdd/d88head.h](../../fdd/d88head.h). The active loader computes the requested
transfer size as `128 << N`, as shown in
[fdd/fdd_d88.c](../../fdd/fdd_d88.c).

## Disk Geometry

The image has 161 populated D88 track entries:

- C0/H0 through C79/H1 contain eight 1024-byte sectors per track;
- C80/H0 contains 26 256-byte sectors;
- the 1280 normal-area sectors have D88 status `00h`;
- the 26 sectors on C80/H0 have D88 status `10h`.

The extra C80/H0 track is outside the boot path analyzed here. The boot track
C0/H0 consists of R1 through R8 with `N=3`, so each sector is 1024 bytes and
has no D88 error status.

## Boot Sector Form

The first sector header is:

```text
C=0 H=0 R=1 N=3
sectors per track=8
stored data size=0400h
D88 status=00h
```

`N=3` means `128 << 3`, or 1024 bytes. The original-VA ROM begins with a
`0200h` transfer count and doubles it when the detected sector form requires
it. It therefore loads all of C0/H0/R1 at `3000:0000`.

Only bytes `0000h-0175h` of the sector contain IPL code or data. Bytes
`0200h-03FFh` are zero but remain part of the physical 1024-byte sector and
the ROM transfer.

### Relationship To The PC-Engine 1.05 IPL

The executable IPL bytes at offsets `0000h-0175h` are byte-for-byte identical
to those in `PC-Engine 1.05.d88`. The only differences in the first
512 bytes are:

| Offset | PC-Engine 1.1 | PC-Engine 1.05 |
| -----: | ------------: | -------------: |
| `01FEh` | `00h` | `05h` |
| `01FFh` | `00h` | `01h` |

No control flow reaches these two bytes, and their format or meaning has not
been established. They are recorded as an unresolved tail marker, not treated
as executable code or a proven version field.

## On-Disk Layout Used By The IPL

The normal 80-cylinder, two-sided area comprises 1,310,720 bytes in 1280
1024-byte sectors after removing the D88 per-sector headers. Its boot-relevant
layout is:

| LBA | Contents |
| ---: | -------- |
| 0 | 1024-byte IPL sector |
| 1-2 | FAT copy 1 |
| 3-4 | FAT copy 2 |
| 5-10 | Root directory |
| 11-14 | `ENGINEIO.SYS` |
| 15 onward | Start of `PCENGINE.SYS` |

The root directory and FAT12 chains identify:

| File | Attributes | Start cluster | Size | FAT chain |
| ---- | ---------: | ------------: | ---: | --------- |
| `ENGINEIO.SYS` | `27h` | 2 | 4,096 bytes | `2 -> 3 -> 4 -> 5 -> FFFh` |
| `PCENGINE.SYS` | `27h` | 6 | 62,347 bytes | `6 -> 7 -> ... -> 66 -> FFFh` |
| `ADVGBIOS.SYS` | `27h` | 67 | 16,364 bytes | `67 -> 68 -> ... -> 82 -> FFFh` |
| `PCENGINE.COM` | `21h` | 83 | 5 bytes | Starts at cluster 83 |
| `HDFORM.COM` | - | 84 | 6,706 bytes | Starts at cluster 84 |

`ENGINEIO.SYS` and `PCENGINE.SYS` are contiguous at the beginning of the data
area. As in PC-Engine 1.05, this allows the IPL to use a fixed multi-sector
read without first implementing a general FAT file lookup.

## IPL Entry At 3000:0000

The original-VA ROM enters the disk IPL at `3000:0000`. Its first instructions
establish a temporary stack in the IPL segment:

```asm
3000:0000  mov ax,3000h
3000:0003  mov ds,ax
3000:0005  cli
3000:0006  mov ss,ax
3000:0008  mov sp,1FFEh
3000:000B  sti
```

Because the executable bytes are identical to PC-Engine 1.05, the top-level
control flow is also identical:

```asm
3000:000C  call 00F2h       ; prepare initial work state
3000:000F  call 0109h       ; update control state through port 018Ah
3000:0012  call 0113h       ; detect writable main-memory extent
3000:0015  jc   0034h       ; stop if memory setup fails
3000:0017  call 0100h       ; initialize system work state

3000:001A  cli
3000:001B  sub  bp,1000h    ; reserve stack below detected memory limit
3000:001F  mov  ss,bp
3000:0021  mov  sp,FFFEh
3000:0024  sti

3000:0025  call 0036h       ; prepare disk state and load the next stage
3000:0028  jc   0034h       ; stop on load failure
3000:002A  call 1340:0000   ; enter ENGINEIO.SYS
3000:002F  call F000:940D   ; enter ROM-side OS initialization
3000:0034  jmp  0034h       ; terminal path if control returns
```

The memory detector reads the original-VA memory switch through port `0152h`,
probes memory in `2000h`-paragraph steps, and leaves the detected upper bound
in `BP`. The permanent IPL stack is placed one `1000h`-paragraph block below
that bound. A memory-probe failure and a next-stage load failure both converge
on the loop at `3000:0034`.

## Disk Parameter Setup And Stage-Two Read

The routine at `3000:0036` uses the zero-filled second half of the IPL sector
as work space. It constructs a disk control/parameter block beginning at
`3000:0200`, selects a ROM disk-service descriptor at `F000:9410` or
`F000:9420`, and calls `F000:DCFA` to populate media-dependent fields.

The fixed-size second-stage request is:

```asm
mov ax,1C00h
div word [disk_parameter + 0Dh]  ; bytes per sector

mov ax,[disk_parameter + 14h]   ; first data-sector LBA
mov [read_lba],ax
mov [read_count],cx
mov [destination_offset],0000h
mov [destination_segment],1340h
```

For this disk:

```text
bytes per sector = 0400h
read count       = 1C00h / 0400h = 7 sectors
first data LBA   = 11
destination      = 1340:0000
```

The far call to `F000:F110` performs the ROM disk-service transfer. It reads
LBA 11 through 17, inclusive, for a total of `1C00h` bytes.

## Resulting Memory Layout

| Guest address | Physical range | Loaded contents |
| ------------- | -------------- | --------------- |
| `1340:0000-0FFF` | `13400h-143FFh` | All 4096 bytes of `ENGINEIO.SYS` |
| `1340:1000-1BFF` | `14400h-14FFFh` | First 3072 bytes of `PCENGINE.SYS` |
| `1500:0000` onward | `15000h` onward | IPL/ROM system work area |
| `3000:0000-03FF` | `30000h-303FFh` | Original 1024-byte IPL sector |

The IPL does not load all 62,347 bytes of PC-Engine 1.1. It loads one complete
bootstrap driver and only the first three sectors of the following driver.

## ENGINEIO.SYS Entry At 1340:0000

The 4096-byte `ENGINEIO.SYS` in PC-Engine 1.1 is byte-for-byte identical to
the file in PC-Engine 1.05. Its SHA-256 is:

```text
7ded86927015579c3aded10d97bf8751d065131df06498d43021717a04f65183
```

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

`FFFFh` identifies the original PC-88VA. On that model, `ENGINEIO.SYS`
installs far-jump stubs in segment `1088h` and records ROM-service hooks in
the system work area. On a VA2/VA3 identification value, this original-VA
patch path is skipped.

The exact file identity proves that the version-specific divergence has not
yet occurred at this entry point. PC-Engine 1.1 and 1.05 use the same
original-VA integration bootstrap here.

## PCENGINE.SYS Header At 1340:1000

The following loaded sector begins with a DOS character-device header:

```text
next device pointer = FFFF:FFFF
attributes          = 8000h
strategy entry      = C197h
interrupt entry     = C1A2h
device name         = "__ENGINE"
```

This differs substantially from PC-Engine 1.05, whose header names strategy
and interrupt entries `0016h` and `0021h`. The PC-Engine 1.1 offsets `C197h`
and `C1A2h` are far beyond the 3072-byte portion loaded by the IPL. Therefore,
the complete `PCENGINE.SYS` must be loaded or relocated before those device
entry points can execute.

This header is direct evidence for that requirement, but static analysis of
the IPL alone does not identify the later loader path. It would be incorrect
to claim that the initial seven-sector transfer makes either entry callable.

## Version-Specific Contents

Strings recovered from the PC-Engine 1.1 filesystem include:

```text
Graphics BIOS Ver 1.10 87/04/22 (with ROM03,04,05) by h.godai & y.shimizu
HDFORM v1.1
```

These are component-identification strings, not a reason to reinterpret the
disk filename or the unresolved two-byte IPL tail marker. The Graphics BIOS
version and date should also not be used to infer a simple chronological
ordering against every component on the PC-Engine 1.05 disk.

## ROM-Side OS Handoff

After `ENGINEIO.SYS` returns, the IPL calls `F000:940D`. In the analyzed
original-VA ROM this entry jumps to `F000:B61C`, which performs broad ROM-side
OS initialization: work-area setup, memory sizing, interrupt-vector setup,
device-table setup, and subsequent initialization calls.

The statically confirmed chain is:

```text
Original-VA ROM boot decision
  -> load C0/H0/R1 at 3000:0000
  -> execute the disk IPL
  -> read LBA 11-17 at 1340:0000
  -> execute ENGINEIO.SYS at 1340:0000
  -> install original-VA PC-Engine hooks
  -> expose the initial PCENGINE.SYS header at 1340:1000
  -> enter ROM-side OS initialization through F000:940D
  -> continue PC-Engine 1.1 startup
```

Thus `3000:0000` is the disk IPL rather than the PC-Engine body. The first
clearly PC-Engine-specific code executed by the IPL is the common
`ENGINEIO.SYS` entry at `1340:0000`.

## Comparison With PC-Engine 1.05

| Property | PC-Engine 1.1 | PC-Engine 1.05 |
| -------- | ------------- | -------------- |
| IPL executable bytes `0000h-0175h` | Identical | Identical |
| IPL bytes `01FEh-01FFh` | `00 00` | `05 01` |
| Stage-two read | 7 sectors from LBA 11 | 7 sectors from LBA 11 |
| `ENGINEIO.SYS` | 4,096 bytes, identical | 4,096 bytes, identical |
| `PCENGINE.SYS` size | 62,347 bytes | 52,090 bytes |
| `PCENGINE.SYS` strategy/interrupt | `C197h` / `C1A2h` | `0016h` / `0021h` |
| `ADVGBIOS.SYS` size | 16,364 bytes | 30,956 bytes |
| `PCENGINE.SB2` | Not present | 62,250-byte root entry |
| Graphics BIOS string | `Ver 1.10 87/04/22` | `Ver 1.12 87/03/10` |

The comparison shows a shared boot mechanism and integration bootstrap, but a
substantially different main driver and auxiliary-file layout. In particular,
the identical seven-sector IPL read must not be interpreted as loading an
equivalent amount of executable PC-Engine state in both versions.

## Confirmed And Unresolved Points

### Confirmed By Static Evidence

- The boot sector is a valid 1024-byte C0/H0/R1 sector with D88 status `00h`.
- Its executable IPL is identical to PC-Engine 1.05 through offset `0175h`.
- The ROM transfers the sector to `3000:0000` and enters it there.
- The IPL computes a seven-sector, `1C00h`-byte read to `1340:0000`.
- Those sectors contain all of `ENGINEIO.SYS` and the first three sectors of
  `PCENGINE.SYS`.
- `ENGINEIO.SYS` is identical to the PC-Engine 1.05 copy, tests the
  original-VA model identifier, and installs original-VA hooks.
- The following data begins with the `__ENGINE` DOS device header.
- The header's entry offsets lie outside the initially loaded three sectors.
- The IPL enters ROM-side initialization through `F000:940D`.

### Still Requiring Runtime Trace Or Additional Documentation

- The meaning of the two bytes at IPL offsets `01FEh-01FFh`.
- Exact names and contracts of every ROM service called by the IPL.
- Exact meaning of the IPL's `INT 80h` operations after the stage-two read.
- The path that loads or relocates the remainder of `PCENGINE.SYS` before
  `C197h` or `C1A2h` can be called.
- Which ROM-side initialization call becomes the final non-returning transfer
  into the running PC-Engine 1.1 environment.
- Whether VA2/VA3 uses the same disk layout with a different `ENGINEIO.SYS`
  path or relies entirely on its later ROM initialization.
