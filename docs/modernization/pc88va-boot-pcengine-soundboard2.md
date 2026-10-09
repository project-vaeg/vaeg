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

# PC-88VA PC-Engine Sound Board II System Disk Analysis

## Scope

This document analyzes the original-PC-88VA disk named
`PC-Engine システムディスク (サウンドボードII対応版).d88`. It follows the
boot path from the disk IPL through the initial PC-Engine drivers and then
identifies the static evidence for Sound Board II support.

The preceding original-VA ROM decision, including the PC key, SW7, V3 IPL
detection, and V1/V2 fallback, is documented in
[pc88va-boot-sequence.md](pc88va-boot-sequence.md). The corresponding analyses
of the numbered PC-Engine releases are:

- [PC-Engine 1.00](pc88va-boot-pcengine1.00.md)
- [PC-Engine 1.05](pc88va-boot-pcengine1.05.md)
- [PC-Engine 1.1](pc88va-boot-pcengine1.1.md)

This is static analysis of a maintainer-owned disk and ROM. Neither payload is
distributed by this repository.

## Analyzed Media

| Item | Value |
| ---- | ----- |
| Disk filename | `PC-Engine システムディスク (サウンドボードII対応版).d88` |
| File size | 1,331,888 bytes |
| SHA-1 | `788ea533cfefb3e8755fe976f380d8e20507af24` |
| SHA-256 | `1cdee106d86ff9299755207c7ce2f56d7241d911d5ac00bd7cb3cc0470cf9ebb` |
| D88 internal name | `HxCFE` |
| D88 write-protect byte | `00h` |
| D88 disk type | `20h` (2HD) |
| Original-VA ROM | `varom1.rom` |
| ROM SHA-1 | `54536dc03238b4668c8bb76337efade001ec7826` |

The D88 structures were interpreted according to
[fdd/d88head.h](../../fdd/d88head.h). The active loader computes a sector size
as `128 << N`, as shown in [fdd/fdd_d88.c](../../fdd/fdd_d88.c).

## Physical Disk Form

The image has 160 populated D88 track entries, C0/H0 through C79/H1. Every
track has eight sectors, and all 1,280 sectors have the same form:

```text
N=3
stored data size=0400h
D88 status=00h
```

The decoded disk area is therefore 1,310,720 bytes. Unlike the analyzed
PC-Engine 1.05 image, this image has no extra C80/H0 track. No sector has a
stored D88 CRC/error status.

The first sector, C0/H0/R1, is a 1,024-byte IPL sector with SHA-256:

```text
af3b7b80ec48b8b5009a1bae7990c95a9e054a8b789c13c481b666e80cdef490
```

Its executable routines occupy offsets `0000h-0168h`. Later bytes are disk
parameter and work-area initial data, not a continuation of the instruction
stream. This IPL is distinct from the IPLs analyzed for PC-Engine 1.00, 1.05,
and 1.1. Only the initial 13-byte segment/stack setup is common.

## On-Disk Layout

The boot-relevant raw-sector layout is:

| LBA | Contents |
| ---: | -------- |
| 0 | 1,024-byte IPL sector |
| 1-2 | FAT copy 1 |
| 3-4 | FAT copy 2 |
| 5-10 | Root directory |
| 11-14 | `ENGINEIO.SYS` |
| 15 onward | `PCENGINE.SYS` |

The root directory contains:

| File | Start cluster | Size | Directory timestamp |
| ---- | ------------: | ---: | ------------------- |
| `ENGINEIO.SYS` | 2 | 4,096 | 1987-09-10 19:48:18 |
| `PCENGINE.SYS` | 6 | 59,956 | 1987-10-20 19:45:12 |
| `PCENGINE.COM` | 65 | 5 | 1987-10-15 11:00:00 |
| `ADVGBIOS.SYS` | 66 | 16,356 | 1986-03-16 15:11:28 |
| `CHKDSK.COM` | 82 | 8,558 | 1987-08-26 15:46:14 |
| `HDFORM.COM` | 91 | 3,750 | 1987-03-05 14:44:12 |
| `NECGAIJI.DAT` | 95 | 8,664 | 1987-03-03 23:30:30 |
| `VA11SAMP.BAS` | 104 | 8,349 | 1987-03-03 23:24:44 |
| `KYBDSAMP.BAS` | 113 | 6,534 | 1987-10-15 16:41:42 |
| `RTHMSAMP.BAS` | 120 | 15,784 | 1987-10-13 17:58:06 |

There is no separate `PCENGINE.SB2` file. The boot configuration still names
`PCENGINE.SYS`, and the Sound Board II implementation is integrated into that
file. This differs from PC-Engine 1.05, whose disk carries both a normal
`PCENGINE.SYS` and a larger `PCENGINE.SB2` alternative.

Selected component identities are:

| Component | SHA-256 | Comparison |
| --------- | ------- | ---------- |
| IPL sector | `af3b7b80ec48b8b5009a1bae7990c95a9e054a8b789c13c481b666e80cdef490` | Unique among the four analyzed disks |
| `ENGINEIO.SYS` | `7ded86927015579c3aded10d97bf8751d065131df06498d43021717a04f65183` | Byte-identical to PC-Engine 1.05/1.1 |
| `PCENGINE.SYS` | `afb53e0634ab1d706c75c9541b619cd77b96958f8b759db5e94369979c3f3cdc` | Sound Board II-specific integrated driver |
| `PCENGINE.COM` | `bfbcdfa65eee2acd963cce8036c2c708fb46772a0ce156a807f0e5f98af8c163` | Same five-byte far jump used by the other analyzed releases |
| `ADVGBIOS.SYS` | `8d9b646deec147744763b1bd4eb3693705ed1b0a9e5f5e6d6457ffe8957000bc` | Byte-identical to PC-Engine 1.00 |
| `RTHMSAMP.BAS` | `dc7beaa7f61132c2a696c871195966fb3df62487016939fc5fe11fd9b40daa42` | Rhythm sample, embedded label `Version 1.0` |

`ADVGBIOS.SYS` identifies itself as:

```text
Graphics BIOS Ver 1.00 87/03/01 (with ROM03,04,05)
```

The dates and component reuse show that this is a distinct 1987 composition,
but they do not establish an official PC-Engine version number. No reliable
PC-Engine version string was found in this disk's `PCENGINE.SYS`, so this
document does not assign one.

## IPL Entry At 3000:0000

The original-VA ROM loads C0/H0/R1 at `3000:0000`. The disk IPL begins with
the standard temporary segment and stack setup:

```asm
3000:0000  mov ax,3000h
3000:0003  mov ds,ax
3000:0005  cli
3000:0006  mov ss,ax
3000:0008  mov sp,1FFEh
3000:000B  sti
```

Its top-level path is:

```asm
3000:000C  call 00EEh       ; initialize four work bytes to FFh
3000:000F  call 00FCh       ; set bit 0 in master PIC mask port 018Ah
3000:0012  call 0106h       ; initialize work pointers and probe memory
3000:0015  jc   0031h       ; stop if no writable region is found

3000:0017  cli
3000:0018  sub  bp,1000h
3000:001C  mov  ss,bp
3000:001E  mov  sp,FFFEh
3000:0021  sti

3000:0022  call 0032h       ; prepare disk state and load the next stage
3000:0025  jc   0031h       ; stop on load failure
3000:0027  call 1340:0000   ; enter ENGINEIO.SYS
3000:002C  call F000:940D   ; enter ROM-side OS initialization
3000:0031  hlt              ; terminal path if control returns
```

The active emulator maps `018Ah` to the master interrupt controller mask, as
shown in [io/pic.c](../../io/pic.c). The IPL preserves its current value and
sets bit 0 before continuing.

The `00EEh` routine writes two `FFFFh` words at `1500:3E93`. Static analysis
does not establish the semantic name of these four work bytes. They should
not be labeled as Sound Board II state without runtime or ROM evidence.

## Memory Detection

The routine at `3000:0106` installs two work-area values:

```text
1040:05F8 = 1500h
1040:05FA = 0400h
```

It then reads port `0152h`, temporarily selects a `B000h` memory mapping,
reads `B000:1FC4`, restores the original port value, and reduces the result to
three bits. That value determines an initial memory limit. The IPL probes
downward in `2000h`-paragraph, or 128 KiB, steps using the sentinel `6DDBh`
until it finds writable memory. The resulting upper bound is returned in
`BP`, and the permanent stack is placed one `1000h`-paragraph block below it.

This detector is more hardware-aware than the simpler probe in the analyzed
PC-Engine 1.00 IPL. Its exact interpretation of the bits read through
`B000:1FC4` remains unresolved.

## Stage-Two Read

The routine at `3000:0032` builds a disk parameter block at `3000:0200`, a
second work pointer at `3000:0400`, and selects the ROM descriptor at
`F000:9410` or `F000:9420` according to `0040:2F0F`. It calls `F000:DCFA` to
populate media-dependent fields and then calculates:

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

The call to `F000:F110` therefore reads LBA 11 through 17, inclusive:

| Guest address | Loaded contents |
| ------------- | --------------- |
| `1340:0000-0FFF` | All 4,096 bytes of `ENGINEIO.SYS` |
| `1340:1000-1BFF` | First 3,072 bytes of `PCENGINE.SYS` |
| `1500:0000` onward | IPL/ROM system work area |
| `3000:0000-03FF` | Original 1,024-byte IPL sector |

The post-read `INT 80h` operations match the general form used by the later
PC-Engine IPLs, but their exact service contracts remain unresolved.

## ENGINEIO.SYS

`ENGINEIO.SYS` is byte-identical to the 4,096-byte file on the analyzed
PC-Engine 1.05 disk. Its entry checks the model word at `F000:FFFE`:

```asm
1340:000C  mov  di,F000h
1340:000F  mov  es,di
1340:0011  mov  di,FFFEh
1340:0014  mov  ax,[es:di]
1340:0017  cmp  ax,FFFFh
1340:001A  ret
```

`FFFFh` identifies the original PC-88VA. On that model, `ENGINEIO.SYS`
installs the same original-VA service hooks described in the
[PC-Engine 1.05 analysis](pc88va-boot-pcengine1.05.md#engineiosys-entry-at-13400000).
It also carries the configuration text:

```text
DEVICE=PCENGINE.SYS
DEVICE=ADVGBIOS.SYS
\CONFIG.SYS
```

There is no configuration-time substitution to a separate `PCENGINE.SB2`
file on this disk; the named `PCENGINE.SYS` is already the Sound Board II
driver.

## PCENGINE.SYS Device Header

The portion loaded at `1340:1000` begins with a DOS character-device header:

```text
next device pointer = FFFF:FFFF
attributes          = 8000h
strategy entry      = 0016h
interrupt entry     = 0021h
device name         = "__ENGINE"
```

The complete file is 59,956 bytes. The IPL exposes only its first 3,072 bytes;
the later loader path must load the rest before the Sound Board II routines
described below can execute.

## Sound Board II Evidence

### VA Sound Hardware Ports

The active VAEG implementation models the VA built-in OPN and Sound Board II
at these guest ports:

| Port | Active implementation role |
| ---- | -------------------------- |
| `0044h` | OPN/OPNA primary address and status |
| `0045h` | OPN/OPNA primary data |
| `0046h` | OPNA extended address and status |
| `0047h` | OPNA extended data, including ADPCM |
| `019Ch` | Enable sound-board access wait |
| `019Eh` | Disable sound-board access wait |

The bindings are in [io/boardsb2.c](../../io/boardsb2.c). The normal VA
OPN path binds only `0044h/0045h`; the Sound Board II OPNA path additionally
binds `0046h/0047h`, restores six FM channels and rhythm state, and registers
the ADPCM stream. [sound/fmboard.c](../../sound/fmboard.c) selects those paths
as `FMBOARD_VA_OPN` and `FMBOARD_VA_OPNA`.

### Extended OPNA Access In This Driver

This disk's `PCENGINE.SYS` directly reads and writes ports `0046h/0047h`.
The normal PC-Engine 1.05 `PCENGINE.SYS` contains no direct `0046h/0047h`
I/O instructions, while PC-Engine 1.05 `PCENGINE.SB2` and this integrated
driver have the same counts:

| Direct instruction | This `PCENGINE.SYS` | 1.05 `PCENGINE.SB2` | 1.05 normal `PCENGINE.SYS` |
| ------------------ | ------------------: | ------------------: | --------------------------: |
| `IN AL,46h` | 9 | 9 | 0 |
| `OUT 46h,AL` | 7 | 7 | 0 |
| `IN AL,47h` | 3 | 3 | 0 |
| `OUT 47h,AL` | 5 | 5 | 0 |

The main extended-register writer is:

```asm
PCENGINE.SYS:8B6B  cli
PCENGINE.SYS:8B6C  push ax
PCENGINE.SYS:8B6D  mov  al,ah
PCENGINE.SYS:8B6F  out  46h,al
PCENGINE.SYS:8B71  pop  ax
PCENGINE.SYS:8B72  nop
PCENGINE.SYS:8B73  nop
PCENGINE.SYS:8B74  nop
PCENGINE.SYS:8B75  out  47h,al
PCENGINE.SYS:8B77  sti
PCENGINE.SYS:8B78  ret
```

The surrounding routines poll bit 2 of port `0046h`, select extended register
`0Fh`, read data from `0047h`, and program extended registers `02h-0Dh` and
`10h`. These operations are consistent with the OPNA/ADPCM path implemented
by `boardsb2.c`; they are not available through the original VA's three-channel
OPN-only binding.

The relationship to the later PC-Engine 1.05 Sound Board II driver is not
based only on I/O counts. The interval `881Ah-8D07h` in this `PCENGINE.SYS`
is byte-identical to interval `8BCEh-90BBh` in PC-Engine 1.05
`PCENGINE.SB2`: 1,262 identical bytes with SHA-256
`9b9d99d68784b7a14b6338ace57ac616111db5305476a71cda1c89a8f1b0bb37`.
Other large matching blocks and the aligned `0046h/0047h` references show
that the later `.SB2` driver is closely related to this integrated code,
although the complete files are not identical.

### Detection And Service Dispatch

At offset `854Dh`, the driver accepts service numbers in `AH` through `0Ch`,
saves registers, and attempts a primary-port readback:

```asm
cmp  ah,0Ch
ja   return
...
mov  al,FFh
cli
out  44h,al
in   al,45h
sti
dec  al
...
jnz  error_21h
```

This appears to be an availability/readback check before dispatching the
sound service. Static analysis alone does not prove the complete hardware
detection contract, so the exact meaning of the `FFh` readback is left open.

The embedded strings `RHYTHM`, `STATUS`, and `RECORD`, the extended OPNA port
code, and the dedicated `RTHMSAMP.BAS` sample are mutually consistent with
rhythm and recording/ADPCM services. The strings by themselves are not used
as the primary proof; direct `0046h/0047h` I/O is the decisive distinction
from the normal OPN driver.

## Boot Chain

The statically confirmed chain is:

```text
Original-VA ROM boot decision
  -> load C0/H0/R1 at 3000:0000
  -> execute this disk's distinct IPL
  -> initialize work pointers and detect memory
  -> read LBA 11-17 at 1340:0000
  -> execute ENGINEIO.SYS at 1340:0000
  -> install original-VA PC-Engine hooks
  -> expose the integrated Sound Board II PCENGINE.SYS header at 1340:1000
  -> enter ROM-side initialization through F000:940D
  -> later load and execute the remaining PCENGINE.SYS body
  -> provide OPNA extended-register, rhythm, and ADPCM-facing services
```

The Sound Board II support is therefore not implemented by the IPL itself.
The IPL is responsible for memory setup and the first driver transfer; the
Sound Board II-specific implementation resides in the larger integrated
`PCENGINE.SYS`.

## Confirmed And Unresolved Points

### Confirmed By Static Evidence

- The image contains 160 normal 2HD tracks and no stored D88 sector errors.
- The original-VA ROM loads the 1,024-byte IPL at `3000:0000`.
- The IPL reads seven sectors, or `1C00h` bytes, from LBA 11 to `1340:0000`.
- Those sectors contain all of `ENGINEIO.SYS` and the first 3,072 bytes of
  `PCENGINE.SYS`.
- `ENGINEIO.SYS` is the same original-VA integration layer used by PC-Engine
  1.05/1.1.
- This disk has no separate `PCENGINE.SB2`; its `PCENGINE.SYS` directly uses
  OPNA extended ports `0046h/0047h`.
- The extended-I/O instruction counts and a 1,262-byte exact code block match
  the later PC-Engine 1.05 `PCENGINE.SB2` variant.
- The normal PC-Engine 1.05 `PCENGINE.SYS` has no direct `0046h/0047h`
  instructions.
- `ADVGBIOS.SYS` is the Graphics BIOS Ver 1.00 component also found on the
  analyzed PC-Engine 1.00 disk.

### Still Requiring Runtime Trace Or Additional Documentation

- The official PC-Engine version designation for this unnumbered disk.
- The exact contract of the IPL's post-read `INT 80h` calls.
- The exact later path that loads the remainder of `PCENGINE.SYS`.
- The semantic names of all Sound Board II service numbers `00h-0Ch`.
- The precise hardware-detection meaning of the `FFh` primary-port readback.
- Whether this system disk intentionally rejects an original VA without
  Sound Board II or merely returns an unavailable-service error.
- Runtime behavior on VA2/VA3, whose built-in OPNA exposes the same extended
  port class but follows a different model initialization path.
