<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# M54 HOSTFAT clean-room implementation specification

## Purpose and provenance boundary

This document contains only the externally observable PC-Engine block-device
contract and vaeg-private transport contract required to implement the M54
read-only HOSTFAT guest driver. It deliberately specifies no implementation
labels, instruction sequences, control-flow layout, or source formatting.

The implementation author must not inspect any historical RAM-disk driver,
the superseded HOSTFAT assembly source or binary, its disassembly, structural
checker, or the commits that contain those materials. General NEC V30/8086
assembly knowledge and NASM documentation are permitted.

## Build and licensing requirements

- Write a new standalone NASM source for a 16-bit flat binary.
- Restrict emitted instructions to the 8086-compatible subset implemented by
  the NEC V30. In particular, do not emit the 80386 `0F 80H`--`0F 8FH` near
  conditional-jump encodings: V30 assigns the `0FH` space different meanings.
- Set an explicit NASM CPU level so branch relaxation cannot silently select
  instructions introduced after the V30.
- The output is a PC-Engine CONFIG.SYS non-IBM block-device driver.
- The source must carry the repository-standard two-clause BSD notice with
  `Copyright (c) 2026 Nakata Maho`.
- The implementation must not include or adapt third-party source text.
- The generated `.SYS` file is a build artifact and is not committed.

## Device header contract

The flat image begins with this byte-level device header:

| Offset | Size | Meaning | Required value |
|---:|---:|---|---|
| `00H` | 4 | next-device far pointer | `FFFFFFFFH` |
| `04H` | 2 | attributes | `2000H` (non-IBM block device) |
| `06H` | 2 | strategy entry offset | implementation-defined offset |
| `08H` | 2 | interrupt entry offset | implementation-defined offset |
| `0AH` | 1 | unit count | `1` |
| `0BH` | 2 | device-name offset | offset of the name below |
| `0DH` | 2 | device-name segment | `0000H` |

The zero-terminated device name is ASCII `HOSTFAT`.

PC-Engine calls the strategy entry with `ES:BX` pointing to the current
request packet. The strategy entry records that far pointer for the next
interrupt-entry call and returns far. The interrupt entry completes the
request and returns far. It must restore the caller-visible general and
segment registers that it saves.

## Request-packet contract

Use the complete 22-byte PC-Engine non-IBM packet layout:

| Offset | Size | Meaning |
|---:|---:|---|
| `00H` | 1 | packet length |
| `01H` | 1 | unit |
| `02H` | 1 | command |
| `03H` | 2 | completion status |
| `05H` | 8 | reserved common-header bytes |
| `0DH` | 1 | command-dependent media/unit field |
| `0EH` | 4 | command-dependent transfer/resident-end far pointer |
| `12H` | 4 | command-dependent count/BPB far pointer |
| `14H` | 2 | starting logical sector |

All numeric values and far pointers are little-endian.

## Completion status values

| Meaning | Value |
|---|---:|
| successful completion | `0100H` |
| removable-query response | `0200H` |
| write protected | `8100H` |
| unknown command | `8103H` |
| read/sector failure | `8108H` |

Every command stores its final status word at request offset `03H`.

## Required command behavior

### `00H`: initialize

Probe the emulator-private service described below.

When the service responds with protocol signature `H1`:

- store unit count `1` at request offset `0DH`;
- return at request offset `12H` a far pointer to a one-element list whose
  element is the offset of the BPB in the driver's current code segment;
- return at request offset `0EH` a paragraph-aligned resident-end far pointer;
- display the zero-terminated message
  `CR LF HOSTFAT read-only drive ready CR LF`;
- complete with `0100H`.

When the service is absent or gives a different signature:

- store unit count `0` at request offset `0DH`;
- clear the far pointer at request offset `12H`;
- still return the correctly rounded resident-end far pointer at `0EH`;
- display the zero-terminated message
  `CR LF HOSTFAT unavailable (start vaeg with --hostfat-dir) CR LF`;
- complete with `0100H`.

The resident end must retain every byte that a later request can execute or
read. Its offset component is zero and its segment is the first paragraph
after the retained image, relative to the load segment.

PC-Engine message display uses software interrupt `83H`, function `AH=02H`,
with `DX=8000H` and the zero-terminated message addressed in the driver's
current data segment.

### `01H`: media check

- store `1` in the first byte of the transfer field at offset `0EH`;
- complete with `0100H`.

### `02H`: build BPB

- store media byte `F0H` at offset `0DH`;
- store a far pointer to the BPB at offset `12H`;
- complete with `0100H`.

### `04H`: read sectors

Send the far pointer to the unchanged 22-byte request packet to the private
service. The host validates and performs the entire transfer.

- private result byte `0`: complete with `0100H`;
- any other result: set the sector count word at `12H` to zero and complete
  with `8108H`.

### `08H` and `09H`: write and write-with-verify

Do not invoke the private service. Complete with `8100H`.

### `0DH` and `0EH`: device open and close

These lifecycle notifications bracket operations such as COPY. They are
successful no-ops and complete with `0100H`.

### `0FH`: removable query

Complete with `0200H`.

### Every other command

Complete deterministically with `8103H`.

## BPB contract

The BPB is exactly this packed byte sequence, with no inserted alignment:

| Field | Value |
|---|---:|
| bytes per sector | 1024 |
| sectors per cluster | 2 |
| reserved sectors | 0 |
| FAT copies | 2 |
| root entries | 128 |
| DOS-visible total sectors | 8186 |
| media descriptor | `F0H` |
| sectors per FAT | 7 |
| trailing PC-Engine compatibility byte | `90H` |

This geometry describes 4084 data clusters and therefore remains below the
4085-cluster FAT12/FAT16 cutoff.

## vaeg-private transport contract

The driver uses only these emulator-private I/O channels:

- `07EDH`: byte stream for little-endian scalar values;
- `07EFH`: command/response byte stream.

### Availability probe

Write the 13 ASCII bytes `check_hostfat` in order to `07EFH`, then read two
bytes from `07EFH`. Only the exact response `H1` means available.

### Read transaction

1. Write the four bytes of the recorded request far pointer, least-significant
   byte first, to `07EDH`.
2. Write the 13 ASCII bytes `read_hostfat1` in order to `07EFH`.
3. Read one result byte from `07EDH`.

The driver never sends write data or host paths through this protocol.

## Acceptance properties

- NASM assembles the source without input from generated or third-party code.
- Initialization succeeds and returns a safe resident end.
- The exact ready/unavailable messages remain observable.
- Root and subdirectory DIR/TYPE and HOSTFAT-to-writable-disk COPY work.
- Open and close cannot reach the host sector service.
- Writes always return write-protect and cannot reach the host service.
- The first hidden sector at LBA 8186 remains rejected by the host service.
- Reset and disabled-mode behavior remain as accepted at G54.
- The implementation author supplies a signed-off textual attestation listing
  every input consulted and affirming that no prohibited source was viewed.
