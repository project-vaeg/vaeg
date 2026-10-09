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

# PC-88VA PC-Engine 1.05 Boot Analysis

## Scope

This document follows one concrete original-PC-88VA boot from the VA ROM IPL
loader into NEC PC-Engine 1.05. It separates four stages that can otherwise
be mistaken for one program:

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
| Disk filename | `PC-Engine 1.05.d88` |
| File size | 1,338,960 bytes |
| SHA-1 | `7323da5474d8ac12e0a8b72d8d48f34af4da7002` |
| SHA-256 | `35a05a48e8aafab0b746deec280be3072a28f06d6d36c75d760da8b6fb0d6176` |
| D88 disk type | `20h` (2HD) |
| Original-VA ROM | `varom1.rom` |
| ROM SHA-1 | `54536dc03238b4668c8bb76337efade001ec7826` |

The D88 structures were interpreted according to
[fdd/d88head.h](../../fdd/d88head.h). The active loader computes the requested
transfer size as `128 << N`, as shown in
[fdd/fdd_d88.c](../../fdd/fdd_d88.c).

## Boot Sector Form

The first sector header is:

```text
C=0 H=0 R=1 N=3
sectors per track=8
stored data size=0400h
D88 status=00h
```

`N=3` means `128 << 3`, or 1024 bytes. Track 0, side 0 contains eight
1024-byte sectors, and the IPL sector has no D88 error status.

The original-VA ROM begins with a `0200h` transfer count and doubles it when
the detected sector form requires it. For this disk the effective transfer is
therefore `0400h`, and the ROM loads all of C0/H0/R1 at `3000:0000`.

Only the first 512 bytes contain IPL code and data. Bytes `0200h-03FFh` of the
1024-byte sector are zero, but they are still part of the physical sector and
the ROM transfer.

## On-Disk Layout Used By The IPL

The normal 80-cylinder, two-sided area comprises 1,310,720 bytes in 1280
1024-byte sectors after removing D88 per-sector headers. The image also has an
extra C80/H0 track containing 26 256-byte sectors with D88 status `10h`; that
track is not referenced by the boot path analyzed here. The boot-relevant
normal-area layout is:

| LBA | Contents |
| ---: | -------- |
| 0 | 1024-byte IPL sector |
| 1-2 | FAT copy 1 |
| 3-4 | FAT copy 2 |
| 5-10 | Root directory |
| 11-14 | `ENGINEIO.SYS` |
| 15 onward | Start of `PCENGINE.SYS` |

The root directory and FAT12 chains identify:

| File | Start cluster | Size | Initial chain |
| ---- | ------------: | ---: | ------------- |
| `ENGINEIO.SYS` | 2 | 4096 bytes | `2 -> 3 -> 4 -> 5 -> FFFh` |
| `PCENGINE.SYS` | 6 | 52,090 bytes | `6 -> 7 -> ... -> 56 -> FFFh` |
| `PCENGINE.SB2` | 57 | 62,250 bytes | Starts at cluster 57 |
| `ADVGBIOS.SYS` | 69 | 30,956 bytes | Starts at cluster 69 |

The two boot files are contiguous at the start of the data area. This is why
the IPL can use one fixed multi-sector transfer rather than performing a
general FAT file lookup.

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

The complete top-level control flow is:

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
in `BP`. The permanent IPL stack is then placed one `1000h`-paragraph block
below that bound. A memory-probe failure and a next-stage load failure both
converge on the loop at `3000:0034`.

## Disk Parameter Setup And Stage-Two Read

The routine at `3000:0036` uses the zero-filled second half of the IPL sector
as work space. It constructs a disk control/parameter block beginning at
`3000:0200`, selects a ROM disk-service descriptor at `F000:9410` or
`F000:9420`, and calls `F000:DCFA` to populate media-dependent fields.

The IPL then calculates the second-stage read:

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

The IPL therefore does not load all of PC-Engine 1.05. It loads one complete
bootstrap driver and enough of the following driver for the ROM/DOS startup
path to continue.

## ENGINEIO.SYS Entry At 1340:0000

The entry code first checks the model word at `F000:FFFE`:

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

This model check is direct evidence that the code reached at `1340:0000` is
not merely generic DOS data: it is the original-VA PC-Engine integration
layer.

## PCENGINE.SYS Header At 1340:1000

The next loaded sector begins with a DOS character-device header:

```text
next device pointer = FFFF:FFFF
attributes          = 8000h
strategy entry      = 0016h
interrupt entry     = 0021h
device name         = "__ENGINE"
```

The root directory identifies this region as the beginning of
`PCENGINE.SYS`. The complete file is 52,090 bytes, so the three sectors loaded
by the IPL are only its initial bootstrap-visible portion. The exact later
loader call that brings in the remainder still needs a runtime trace.

## PCENGINE.SB2 And SYS2.COM

### Alternate Sound Board II Driver

PC-Engine 1.05 carries two implementations of the `__ENGINE` DOS character
device:

| File | Size | SHA-256 | Guest sound interface |
| ---- | ---: | ------- | --------------------- |
| `PCENGINE.SYS` | 52,090 bytes | `c3e2cc06c7007f05a43ed8c49da4262c47567b86ef501f641d288ab47ec51815` | Original-VA OPN path |
| `PCENGINE.SB2` | 62,250 bytes | `fdcac3749ff449d0d69860affaca0e0194e559fe24e27930a22a1d3d575d00c7` | Sound Board II OPNA path |

Both files have the same DOS device name, `__ENGINE`, and the same strategy
and interrupt entry offsets, `0016h` and `0021h`. `PCENGINE.SB2` is therefore
an alternate implementation of `PCENGINE.SYS`, not a data file consumed by
the normal driver. It identifies itself internally as:

```text
NEC PC-Engine v1.05
Copyright (C) 1987,1988 NEC Corporation
```

The normal driver uses the primary OPN ports `0044h/0045h` and contains no
direct `0046h/0047h` I/O instructions. The `.SB2` driver also accesses the
OPNA extended ports directly:

| Direct instruction | `PCENGINE.SYS` | `PCENGINE.SB2` |
| ------------------ | -------------: | --------------: |
| `IN AL,46h` | 0 | 9 |
| `OUT 46h,AL` | 0 | 7 |
| `IN AL,47h` | 0 | 3 |
| `OUT 47h,AL` | 0 | 5 |

Those ports are the Sound Board II extended address/status and data ports in
the active implementation, as documented by
[io/boardsb2.c](../../io/boardsb2.c). The `.SB2` body includes the related
rhythm, ADPCM, and recording-facing routines. This direct hardware access is
the decisive evidence that `SB2` denotes Sound Board II.

The earlier unnumbered Sound Board II system disk instead carries its OPNA
driver under the canonical name `PCENGINE.SYS`. Its relationship to the 1.05
alternate driver is analyzed in
[pc88va-boot-pcengine-soundboard2.md](pc88va-boot-pcengine-soundboard2.md).

### How SYS2.COM Activates The Alternate Driver

`ENGINEIO.SYS` always emits:

```text
DEVICE=PCENGINE.SYS
```

It never names `PCENGINE.SB2`, so the `.SB2` file is not loaded merely because
it is present in the root directory. `SYS2.COM` contains these three paths:

```text
A:\PCENGINE.SYS
A:\PCENGINE.VA
A:\PCENGINE.SB2
```

Its replacement path uses DOS `INT 21h`, function `56h` to perform:

```text
PCENGINE.SYS -> PCENGINE.VA
PCENGINE.SB2 -> PCENGINE.SYS
```

If the second rename fails, it restores `PCENGINE.VA` to `PCENGINE.SYS`.
Consequently, `PCENGINE.VA` is the preserved normal-driver copy and the Sound
Board II driver becomes loadable under the canonical name expected by
`ENGINEIO.SYS`. The exact user-facing procedure that invokes `SYS2.COM` is not
established by this static analysis, but the file-selection and rollback
mechanism are explicit in the executable.

The analyzed `SYS2.COM` has SHA-256:

```text
f9dc585c161e46f35a69f38f2d819288c88b570ef83e10e10f0d2b7c2d75faff
```

### Resident Main-Memory Footprint

During DOS device initialization, each driver returns its resident-end
address in request-packet fields `0Eh/10h`. On an original VA, identified by
`F000:FFFE == FFFFh`, the returned addresses are:

| Active driver | Resident-end address | Paragraph-rounded image size |
| ------------- | -------------------- | ---------------------------: |
| Normal `PCENGINE.SYS` | `CS:C337h` | 49,984 bytes |
| Sound Board II driver | `CS:EAB7h` | 60,096 bytes |

The Sound Board II implementation therefore retains approximately 10 KiB
more main-memory driver code than the normal OPN implementation. Its complete
62,250-byte file is not all resident: the initialization tail beginning at
`EAB7h` can be discarded after initialization.

On a VA2/VA3 model identifier, both drivers return `CS:004Dh` instead. Only
the 77-byte device header and dispatch stub need remain, with the later model's
ROM-side implementation providing the corresponding environment. These are
resident V30 main-memory ranges; they are not Sound Board II sound RAM or
ADPCM sample RAM.

The three `PCENGINE.SYS` sectors exposed at `1340:1000` by the IPL are a
separate bootstrap placement. The final DOS device instance is loaded at a
DOS-assigned segment and uses the resident-end address above to release its
initialization tail.

## ROM-Side OS Handoff

After `ENGINEIO.SYS` returns, the IPL calls `F000:940D`. In the analyzed
original-VA ROM this entry jumps to `F000:B61C`, which performs broad ROM-side
OS initialization: work-area setup, memory sizing, interrupt-vector setup,
device-table setup, and subsequent initialization calls.

The practical chain is therefore:

```text
Original-VA ROM boot decision
  -> load C0/H0/R1 at 3000:0000
  -> execute the disk IPL
  -> read LBA 11-17 at 1340:0000
  -> execute ENGINEIO.SYS at 1340:0000
  -> install original-VA PC-Engine hooks
  -> expose the PCENGINE.SYS header at 1340:1000
  -> enter ROM-side OS initialization through F000:940D
  -> continue PC-Engine 1.05 startup
```

Thus `3000:0000` is the PC-Engine disk's IPL, not the PC-Engine body itself.
The first clearly PC-Engine-specific execution occurs at `1340:0000`, the
`ENGINEIO.SYS` entry.

## Confirmed And Unresolved Points

### Confirmed By Static Evidence

- The boot sector is a valid 1024-byte C0/H0/R1 sector with D88 status `00h`.
- The ROM transfers that sector to `3000:0000` and enters it there.
- The IPL computes a seven-sector, `1C00h`-byte read to `1340:0000`.
- Those seven sectors contain all of `ENGINEIO.SYS` and the first three
  sectors of `PCENGINE.SYS`.
- `ENGINEIO.SYS` tests the original-VA model identifier and installs hooks.
- The following data begins with the `__ENGINE` DOS device header.
- `PCENGINE.SB2` is the alternate Sound Board II/OPNA implementation of the
  same `__ENGINE` device.
- `SYS2.COM` safely renames the `.SB2` file to the canonical
  `PCENGINE.SYS` name expected by `ENGINEIO.SYS`.
- On an original VA, the normal and Sound Board II drivers request resident
  ends at `C337h` and `EAB7h`, respectively.
- The IPL then enters ROM-side initialization through `F000:940D`.

### Still Requiring Runtime Trace Or Additional Documentation

- Exact names and contracts of every ROM service called by the IPL.
- Exact meaning of the IPL's `INT 80h` operations after the stage-two read.
- The later path that loads the remainder of `PCENGINE.SYS`.
- The exact user-facing operating procedure around `SYS2.COM`.
- Which ROM-side initialization call becomes the final non-returning transfer
  into the running PC-Engine environment.
- Whether VA2/VA3 uses the same disk layout with a different `ENGINEIO.SYS`
  path or relies entirely on its later ROM initialization.
