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
# PC-88VA Network Adapter Research

This note records the available evidence for using Ethernet adapters with the
PC-88VA family and outlines a possible vaeg implementation. It is a research
note, not a statement that vaeg currently emulates a network adapter.

## Conclusion

No evidence found in this investigation identifies a built-in or dedicated NEC
PC-88VA Ethernet adapter. The demonstrated approach is to install a PC-98 C-Bus
Ethernet adapter in a PC-88VA expansion slot and use a packet driver patched for
the VA environment.

The two adapter families found in PC-88VA reports are:

| Adapter | Hardware role | PC-88VA evidence |
|---|---|---|
| ICM IF-2771ET | Combined SCSI and Ethernet adapter | A VA3 experiment used a VA-patched `IF27PDVA.COM` packet driver and assigned Ethernet to INT2. |
| MELCO/Buffalo LGY-98J-T | Ethernet adapter | A VA2 report identifies a PC-98 driver and a corresponding VA patch. |

Buffalo describes the
[LGY-98J-T](https://www.buffalo.jp/product/detail/lgy-98j-t.html) as a LAN
adapter for PC-98 and EPSON desktop systems. NEC's
[PC-88VA2 specification](https://support.nec-lavie.jp/support/product/data/spec/cpu/b048-1.html)
documents the expansion slots but does not list Ethernet as a standard device.

## Reported PC-88VA Configuration

The [Asayan PC-88VA3 report](https://www.asayan-town.com/nec/pc-88va3/)
describes an ICM IF-2771ET installation with the following resource plan:

| Resource | Assignment in the report |
|---|---|
| Ethernet interrupt | INT2 |
| Ethernet I/O | Adapter default; the exact value is not stated on the page |
| SCSI I/O | `0CCxh` |
| SCSI interrupt | INT0 |
| Internal VA3 2TD interrupt | INT1 |
| Standard FDD DMA | DMA2 |
| 2TD DMA | DMA3 |

The report generated `IF27PDVA.COM` by placing the PC-98 packet driver and
`vapatch.com` together. Its software stack included `PCPAT.SYS`, `MSE312.SYS`,
`IF27PDVA.COM`, and TEENE. The route configuration appeared plausible, but the
author reported unresolved broadcast behavior. It therefore demonstrates
hardware and driver bring-up, not a completely verified network session.

Additional VA2 reports indexed under the
[PC-88VA article tag](https://blogtag.ameba.jp/news/PC-88VA) mention both the
ICM IF-2771ET and MELCO LGY-98J-T, VA patches for their PC-98 drivers, and
experiments with TEEN/TEENE. These are individual experimental reports rather
than manufacturer specifications and should be treated accordingly.

## DOS Software Path

The expected guest software stack is:

```text
PC-98 C-Bus Ethernet adapter
  -> VA-patched DOS packet driver
    -> DOS packet-driver interface
      -> TEENE or another packet-driver TCP/IP stack
        -> guest network applications
```

TEENE expects a packet driver to be loaded first. A general
[PC-98 DOS networking overview](https://www.watakatsu.com/dos/dosterm.html)
describes this packet-driver-to-TEENE sequence. A modern
[PC-98 TEEN setup example](https://asanobuturi.github.io/document/2023/10/)
shows an LGY packet driver followed by TEENE, but its PC-98 command-line values
must not be assumed to be valid for the VA-patched driver.

The exact VA packet-driver command line, software interrupt vector, patch
behavior, and redistribution license remain unverified. Driver binaries must
not be added to the repository without a separate provenance and license
review.

## Adapter Hardware Model

Both reported families are NE2000/DP8390-class adapters with PC-98 C-Bus port
layouts. They must not be modeled as a PC/AT NE2000 at a conventional linear
I/O base.

A historical
[Linux/98 NE2000 C-Bus patch](https://www.cs.helsinki.fi/linux/linux-kernel/2003-11/1295.html)
classifies MELCO LGY-98 adapters as hardware type 4 and ICM IF-2766ET/
IF-2771ET adapters as type 5. It provides the following implementation-level
port mappings:

| Family | Register base | Remote-DMA data port | Reset/probe area |
|---|---:|---:|---:|
| ICM IF-27xxET | `56D0h` | base + `100h` | base + `10Fh` |
| MELCO LGY-98 | selectable `00D0h` through `70D0h` | base + `200h` | base + `300h` area |

The [FreeBSD PC-98 hardware notes](https://www.freebsd.org/releases/5.0R/hardware-pc98/)
independently list LGY-98 as type 4 and IF-2766ET/IF-2771ET as type 5 DS8390
adapters.

The [Neko Project 21/W LGY-98 analysis](https://simk98.github.io/np21w/docs/lgy98.html)
documents the non-linear port layout, selectable INT0/INT1/INT2/INT5 interrupt
lines, adapter-specific probe registers, and the remote-DMA-complete behavior
expected by the DOS driver. Its
[download page](https://simk98.github.io/np21w/download.html) identifies the
current `lgy98*.*` implementation as MIT-licensed. Any future reuse still
requires an exact version, source-header, and provenance audit before code is
imported.

## Current vaeg State

The active CMake/SDL2 tree has no Ethernet adapter or host network backend.
There is no DP8390/NE2000 device, packet RAM, pcap/TAP adapter, or user-mode NAT
backend in the active source list.

The removed legacy HOSTDRV code was a DOS host-shared-drive service reached
through the NP2 system port. It was not Ethernet emulation and could not
satisfy a DOS packet driver. The current host-folder feature is the separate
read-only HOSTFAT path.

The existing infrastructure that could host a future adapter is:

- `cbus/cbuscore.c`: C-Bus device reset and bind boundary.
- `io/iocore.c` and `io/iocore.h`: full 16-bit I/O handler attachment.
- `io/pic.c` and `io/pic.h`: interrupt assertion and the existing `IRQ_INT2`
  mapping.

No current configuration or GUI entry should imply that networking is
available.

## Recommended Implementation Boundary

The first implementation should emulate one verified adapter rather than a
generic label named only "network card." ICM IF-2771ET Ethernet is the closest
match to the documented VA3 `IF27PDVA.COM` path. LGY-98 is also a viable first
target if a known-good VA-patched LGY packet driver can be verified.

The implementation can be separated into these layers:

1. A C DP8390/NE2000 core implementing register pages, packet RAM, ring-buffer
   receive, transmit, remote DMA, ISR/IMR behavior, and reset.
2. A thin C-Bus adapter implementing the selected card's non-linear I/O map,
   board-specific probe behavior, MAC address storage, and reset semantics.
3. VA interrupt routing, initially fixed to INT2 where that matches the tested
   guest configuration.
4. A host packet backend behind a narrow interface so guest hardware does not
   depend directly on SDL2 or a host networking library.
5. SDL2 configuration and GUI controls for adapter enablement, backend, and MAC
   address only after the hardware path passes ROM-less tests.

For IF-2771ET, the first scope should emulate only its Ethernet function. The
combined card's SCSI function should remain separate from vaeg's existing
SASI/SCSI path until its coupling and resource conflicts are understood.

A user-mode NAT backend such as libslirp is the preferred initial host backend:
it is portable and does not require privileged TAP setup. Raw TAP or pcap
bridging can be a later, explicit option for LAN broadcast and non-IP traffic.
The dependency, exact version, license, static-link implications, and platform
availability require an ADR before adoption.

## Verification Plan

ROM-less tests should cover:

- DP8390 reset values and register-page selection.
- Remote DMA read/write and packet-memory ring wrap.
- Transmit completion, receive insertion, ISR/IMR behavior, and IRQ release.
- Exact ICM or LGY non-linear I/O address translation.
- INT2 assertion and deassertion through the existing PIC.
- Device reset, disabled-card behavior, and persistent MAC parsing.
- Host-backend packet length and bounds validation.

The human gate should use a legally obtained, known-good VA packet driver and
test at least:

- packet-driver probe and initialization;
- TEENE startup;
- communication with the user-mode gateway;
- DNS plus one simple TCP operation such as FTP or Telnet;
- VA2 and VA3 resource configurations;
- VA3 operation without conflicting with the internal 2TD interrupt;
- no regression with the adapter disabled.

## Open Questions

- Which adapter and VA-patched packet-driver version will be the compatibility
  target?
- What are the exact packet-driver invocation, software interrupt, and expected
  probe sequence?
- Is the reported IF-2771ET broadcast failure a driver issue, adapter resource
  issue, TEENE configuration issue, or a limitation of that experiment?
- Does every VA model decode all adapter-specific C-Bus registers identically?
- Which packet-driver and patch sources can be legally retained for automated
  or human testing?
- Is user-mode NAT sufficient, or is raw Ethernet required by the target guest
  software?

Until these questions are resolved, network support should remain a documented
future feature rather than an enabled but approximate device.
