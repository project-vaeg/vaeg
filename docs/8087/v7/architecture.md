# Architecture contract

## Ownership

The machine owns zero or one 8087. Do not introduce a device array, slot selector,
device count, or a second FPU. Repeated reset/savestate load must not duplicate
callbacks or device objects.

FPO1 may reach the sole 8087 when model/mode/configuration allow it. FPO2 never
does.

## Architectural state

Maintain:

- eight physical raw 80-bit registers
- TOP and exact 8087 tag classes
- control word, status word, and tag word
- instruction/data pointers and opcode field
- sticky exception state
- pending BUSY/INT state required by the chosen model

Keep configuration intent, active oscillator/timing state, and guest architectural
state separate.

## Raw 80-bit representation

Use explicit integers and exact 10-byte codecs. Preserve all 80 bits, including
noncanonical encodings, in traces and VAEG savestates. Do not serialize raw C
structs or host-endian layouts.

Classify raw values before backend arithmetic. Only backend-safe canonical values
may enter SoftFloat. 8087-specific pseudo classes, projective infinity, tags,
denormal-operand behavior, NaN selection, IEM, IC, and masked responses remain
VAEG responsibilities.

## CPU/device seam

The production CPU decoder owns instruction bytes, ModR/M, displacement, segment
selection, effective address, and CPU-side bus behavior. The FPU must not decode
the guest instruction stream again.

For memory FPO1 forms, preserve the CPU first-word read required by the selected
NEC/Intel interface. Where that word is the first operand data observed by the
8087, pass the latched word to the FPU rather than reading side-effectful MMIO twice.

Native V-series and 8080-compatible execution must remain independent.

## Guest environment

Implement the 8087 14-byte environment and 94-byte FSAVE image exactly after P01
freezes the source-backed layout. Never substitute 287 protected-mode, FXSAVE, or
host layouts.
