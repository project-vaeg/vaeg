# P17 integration blocker record

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Status: **INTEGRATION_BLOCKED** (2026-09-20 local audit)

The supplied read-only evidence workspace was enumerated after P00
verification. Its available payloads are the six Intel/NEC PDF sources, their
six user-supplied OCR companions, the manual-sync hash record, and the
SoftFloat Release 3e archive and license/source records. It contains no VAEG
machine schematic, PC-88VA interrupt-controller map, guest interrupt vector,
production NDP handler, or executable VA route trace.

The continuation audit on 2026-09-20 re-enumerated the external root and found
no additional schematic, netlist, BIOS/guest handler, route trace, or hardware
capture beyond those listed payload classes. The repository audit likewise
found that the circuit notes explicitly leave the VA2 netlist reconstruction
open; no current or historical VAEG source supplied the missing pin-level
connection. This preserves the evidence boundary rather than treating a
filename, socket listing, or legacy bit definition as route proof.

Therefore the local implementation keeps the adapter as an explicit
repository-local integration seam and does not promote the inferred
`IRQ_NDP` wiring to evidence-backed completion. In particular, this record
does not invent an IRQ13, NMI, `#MF`, polarity, or controller route. The
production selftest verifies the real ESC path, configured service timing,
pending exception state, save/restore behavior, and a synthetic guest handler
through the repository's actual V30/PIC path. The handler executes FCLEX,
sends slave/master EOIs, and returns with IRET; the test result is:

```text
selftest: 8087 guest interrupt route ok
```

The same local selftest now also drives the guest-visible PIC2 ICW path with
base `40h` and observes the NDP source at vector `46h`, and separately checks
edge-triggered held-high suppression and level-triggered re-presentation after
slave/master EOI. Its additional result is:

```text
selftest: 8087 PIC input modes ok
```

These are synthetic controller-input tests through the repository's existing
PIC implementation. They do not establish the IC19.23/IC19.32 PCB nets,
signal polarity, or the physical VA2 acknowledgement circuit.

That executable result is useful local production coverage, but it cannot
replace the missing VAEG-specific evidence for the inferred controller route.

## Open software-integration issue: VA2/VA3 BASIC /87

The VA2/VA3 main-ROM `/87` path is now recorded as a production guest target;
the user supplied that it starts the 8087-using N88 Japanese BASIC V3.1 path
and applies only to VA2/VA3. See the dedicated
[BASIC /87 route issue](issue-basic87-va2-int-route.md).

This provides a concrete workload for checking real guest ESC dispatch,
service timing, and—if the BASIC path deliberately produces an unmasked
exception—guest exception-handler entry and recovery. It does not, by itself,
prove the IC19.23 BUSY or IC19.32 INT PCB nets. A successful guest run must
therefore be recorded as software/guest evidence separately from the physical
route, and P17/P19 remain fail-closed until both evidence classes are present.

The repository-side evidence audit found only partial corroboration. The
existing [PC-88VA circuit notes](../../../modernization/88va_circuit.md)
identify an optional `8087-1` DIP-40 on VA2 and identify page 248 as the CPU
interrupt-logic sheet, while the historical VAEG `IO/PIC.H` source retained in
the repository history contains the pre-existing `PIC_NDP` slave-PIC bit. Those
facts establish an optional NDP footprint and a legacy software symbol, but do
not establish the NDP pin-level BUSY/INT net, polarity, interrupt vector,
controller acknowledgement, or a guest route. The current `IRQ_NDP` value is
therefore treated as an implementation seam tested by the synthetic guest
program, not as a hardware-backed route claim.

The fail-closed gate result is:

```text
P17_INTEGRATION_BLOCKED: no supplied VAEG/VA evidence establishes the 8087 INT/BUSY controller route; the external evidence tree contains only Intel/NEC manuals and SoftFloat
P17: INTEGRATION_BLOCKED
```

This is the exact contract-defined blocker. Independent core, arithmetic,
timing, configuration, persistence, and zero-omission audits continue, but
the final state must not be called `SOFTWARE_VERIFIED` while this record is
the only route result.
