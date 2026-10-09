# Clock and timing: SERIAL_TIMED_V1

## Configuration

- default active clock: `8000000` Hz
- accepted emulator setting: integer `1000000..20000000` Hz
- presets: 5, 8, 10 MHz
- requested changes apply on machine reset
- FINIT and FSAVE architectural reset behavior do not alter the oscillator

The configured frequency must affect virtual NDP service time. A decorative UI
frequency is a failure.

## Exact conversion

Use the existing VAEG scheduler timebase where possible. For scheduler ticks per
second `T`, FPU frequency `f`, selected service clocks `C`, and residue `r`:

```text
numerator = C * T + r
elapsed_ticks = numerator // f
next_residue = numerator % f
```

Use checked integer arithmetic and preserve the residue across VAEG savestates.
Do not accumulate independently rounded per-instruction durations.

Reference conversion checks:

```text
100 NDP clocks at  8 MHz = 12.5 microseconds
100 NDP clocks at 10 MHz = 10 microseconds
100 NDP clocks at  5 MHz = 20 microseconds
```

These are conversion tests, not claims about a specific instruction's latency.

## SERIAL_TIMED_V1

1. production CPU decodes and performs required CPU-side bus work;
2. FPU evaluates and stages permitted architectural effects;
3. scheduler advances by the selected NDP service time while the CPU does not
   execute its next guest instruction;
4. non-CPU machine events due during the interval are serviced normally;
5. effects are committed once at the completion boundary.

Do not fake an asynchronous overlap model by committing a result and leaving an
ordinary BUSY countdown after return. True CPU/NDP overlap is outside v7.
