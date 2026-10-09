# Clock and execution: SERIAL_TIMED_V1

> Applies only to Stage B, after the explicit user OCR handoff and acceptance in
> [goal-contract.md](goal-contract.md) section 0. Stage A must not execute this work.

Normative with [goal-contract.md](goal-contract.md). Values here are emulator policy unless
explicitly attributed to I01. The model is deliberately serial, not a partial asynchronous
coprocessor implementation.

## 1. Configuration contract

Use the repository's key naming convention, with these meanings:

| Logical field | Default | Validation | Apply point |
|---|---:|---|---|
| `enabled` | false | Boolean; only valid machine models | Machine reset |
| `clock_hz` | 10000000 | Integer, 1000000 through 20000000 inclusive | Machine reset |

The 1-20 MHz editor range is an explicit VAEG policy, not a manufacturer's operating-voltage
or electrical guarantee. Offer 5, 8, and 10 MHz presets plus a custom value. Persist integer
Hz. The display may format MHz using integer division/remainders; do not use host floating
point to derive the active frequency or service-time calculations. The headless parser has
the same validation as the GUI. Missing old keys use the defaults; malformed explicit values
produce a clear error or rejected setting, not silent wraparound or a substituted zero.

Keep **requested configuration** separate from the running device's **active configuration**.
Changing the checkbox or clock while running sets a reset-required indication. It does not
change an in-flight instruction, instantiate a second device, reset guest state silently, or
change CPU speed. A machine reset applies both settings and clears the timing residue.
Guest FINIT and FSAVE's architectural initialization do not change the oscillator, consume
pending user configuration, or reset the clock conversion phase.

Savestate restore uses the saved active clock/residue for deterministic continuation and
shows a mismatch with requested settings when applicable. It does not overwrite user disk
configuration. Validate before mutation; an invalid saved clock is an invalid payload, not
a reason to clamp and continue with different timing.

## 2. Integer conversion from NDP clocks

Prefer the existing scheduler's fixed machine-time unit. Let `T` be ticks per second,
`f` the active FPU frequency in Hz, `C` the selected NDP service clocks, and `r` a residual
numerator with `0 <= r < f`. Convert exactly:

```text
numerator = C * T + r
elapsed_ticks = numerator // f
next_residue = numerator % f
```

These are integer operations. Use a checked, portable wide multiply/divide or a bounded
limb implementation if the product can overflow. Do not require native 128-bit integers
on unsupported compilers. Prove the bounds or check them; do not cast after overflowing.
The total conversion must equal `floor((sum(C) * T + initial_residue) / f)`. Independent
rounding of each instruction is forbidden because it introduces accumulating clock drift.

If the repository has no fixed-rate scheduler unit, define an equivalent exact rational
adapter and document its interaction with CPU clock changes. Do not silently equate an
8087 clock with one CPU cycle. Reuse an existing rational-time helper where correct rather
than inventing a second global scheduler. Oscillator phase error at observable boundaries
must be less than one scheduler tick; it must not grow with instruction count.

Reference checks use synthetic service counts, not claimed instruction timings:

```text
100 NDP clocks at 10000000 Hz = 10 microseconds
100 NDP clocks at  5000000 Hz = 20 microseconds
```

Test an awkward custom frequency such as 7159091 Hz, long sequences, chunk splitting,
near-overflow inputs, reset, and save/restore against independent arbitrary-precision integer
arithmetic. A host wall-clock benchmark is not a test of emulated clock correctness.

## 3. Nominal latency table

Create a source-cited machine-readable latency table, keyed by documented semantic form
and relevant operand class when known. I01 Table 5 is the starting source. Preserve its
range, units, and footnotes separately from the chosen emulator cost.

For the first release, use the documented upper endpoint as the deterministic nominal cost
for an in-domain instruction/form unless a more precise source-backed rule is selected.
Apply a documented pop adjustment only when the base row excludes it. Account for register,
memory, conversion, and administrative variants explicitly. Do not apply every footnote to
every instruction. For a documented operation with no resolved cost, record a conservative
project estimate and its rationale before P04/P17 pass; do not silently assign one clock.

FWAIT/POLL is a CPU instruction: use the audited active NEC CPU cost. Do not treat the
8086 FWAIT row in I01 as NDP clocks or scale CPU polling cost by `clock_hz`. The NDP latency
table is for actual ESC operations; FPO2 also retains its native CPU-only accounting.

This table estimates **NDP service time**. CPU instruction fetch, effective-address work,
and existing bus/wait-state charges are audited separately. For each path write down where
those charges occur. Never charge the same CPU-side work twice or add a second NDP cost in
FWAIT after the instruction was already completed. Because the selected table is a nominal
approximation, elapsed time is not advertised as measured silicon latency.

Reserved/unknown/later-only encodings do not execute a documented operation. Use their
explicit non-authentic CPU-only policy, without manufacturing arithmetic or an interrupt.
Absent FPU and FPO2 retain the preexisting audited CPU-side cost, not a fictitious 8087 cost.

## 4. Execution phases and visibility

Use one serialized operation through the production CPU:

1. Decode in native mode, capture opcode/address metadata, and perform required CPU bus
   work once. Latch the first operand word from the CPU read where applicable.
2. Validate stack and operand classes, evaluate the operation, and prepare its source-backed
   exception and state/memory effects without publishing a premature result.
3. Advance emulated machine time by the selected NDP service duration while the CPU cannot
   execute its next guest instruction. Service due non-CPU events through the established
   scheduler. Host computation speed and wall time are irrelevant.
4. At the completion boundary commit the allowed architectural effects, guest stores,
   pending/INT transitions, and final status. Then allow the CPU to continue.

The scheduler adapter must not recursively run the same CPU, starve timers/audio/DMA,
or execute the next guest instruction before completion. If an existing event architecture
requires a yield, retain at most one serialized **CPU continuation** with latched operands.
This is not an asynchronous queue and does not allow overlapping FPU/CPU instruction streams.
No retry may repeat a destructive MMIO read, an already committed write, or a clock charge.

Other machine events may observe pre-completion memory followed by completed memory. Define
the existing scheduler's equal-timestamp ordering and test it. Bus arbitration within the
FPU transfer is a documented approximation, not a reason to bypass memory accessors. A
memory fault or side-effectful accessor follows the active machine's rules; do not invent
an x86 protected-mode page fault or promise rollback of irreversible MMIO.

The software operation and elapsed-time advance are bounded. Service a requested emulator
snapshot, pause, or reset at the instruction-safe boundary, after at most the current
operation. This avoids serializing C pointers or half-applied effects. If an existing
snapshot API requires an earlier boundary, implement an explicit portable continuation
record or report the design conflict; never save an inconsistent busy state.

## 5. BUSY, pending exception, and POLL

Distinguish transient service from the 8087's architectural B/IR/INT rules. Some control
operations have special BUSY behavior; a blanket `B=1 for every latency` is not a specification.
Use the operation's source-backed control-unit policy. Do not expose committed results and
then leave an artificial ordinary BUSY countdown running. No ordinary service remains after
the serialized instruction returns.

An unmasked exception can leave a persistent pending condition after computation. It is
not cleared by consuming nominal latency. POLL/FWAIT must use the CPU's real wait and
interrupt-eligibility mechanism. Do not spin the host, busy-loop the emulator without advancing
events, fake completion, or clear an exception merely to escape a wait. No-WAIT administrative
forms must remain able to perform the documented recovery actions. Test masked CPU interrupt
conditions and emulator pause/reset responsiveness without changing guest architectural state.

## 6. Required timing acceptance

P04 checks arithmetic conversion, residue, and a scheduler fixture. P07 checks actual guest
execution. P17 checks full integration and handler timing. At minimum prove:

- Changing active 5 MHz to 10 MHz halves the isolated NDP service contribution for an exact
  test case, within one timebase tick; CPU/bus overhead is measured separately.
- Identical non-time-dependent guest operands yield identical result/flag/tag/memory traces
  at all tested clocks. Timing-dependent guest programs are not expected to be identical.
- A timer scheduled during a long NDP service is processed in the proper order; the CPU does
  not execute the next instruction early. No duplicate callback, dummy read, or store occurs.
- Long-run fractional accounting and snapshot continuation match the independent reference.
- FINIT does not apply requested clock changes; machine reset does.

`SERIAL_TIMED_V1` intentionally stalls the CPU for NDP service. This can be slower than real
CPU/8087 overlap. Do not call it cycle accurate. A future overlapped model is a separate task,
not permission to make the clock setting decorative in this release.
