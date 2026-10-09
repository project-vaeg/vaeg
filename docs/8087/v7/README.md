# VAEG + Intel 8087 implementation pack v7

v7 starts directly from the already prepared external evidence workspace:

`/Users/maho/work/vaeg-8087-evidence-v6`

The goal is to implement one optional Intel 8087 in the checked-out VAEG
repository and verify it through the production CPU, memory, scheduler,
configuration, savestate, and interrupt paths.

## Fixed user requirements

- maximum installed 8087 devices per emulated machine: **one**
- default active 8087 clock: **8,000,000 Hz**
- clock: configurable and persisted independently from CPU frequency
- clock changes: applied at machine reset
- FPO2: never connected to the sole 8087 and never used for a second 8087
- target ISA: Intel 8087 only, not a later x87 superset
- all documented 8087 instruction/operand/encoding forms are mandatory
- first timing model: `SERIAL_TIMED_V1`

## Configuration surface

The SDL2 Configure dialog exposes the optional NDP with an enabled checkbox,
5/8/10 MHz presets, and a custom integer frequency from 1,000,000 through
20,000,000 Hz. The requested values are persisted as `NDP8087` and
`NDP8087Hz` in `vaeg.cfg`, while the active oscillator remains visible in the
dialog and changes take effect only at guest reset. There is no FPO2 or
second-device control.

## Evidence workspace

The external evidence workspace is read-only to the implementation work unless a
package explicitly creates a new derived report under the VAEG build/test area.
Do not move or rewrite the original PDFs or OCR Markdown.

See [evidence-layout.md](evidence-layout.md) for exact mappings.

## Start

Run Codex from the VAEG repository root and submit the exact text in
[activation.txt](activation.txt).

The goal is implementation, not a planning-only session. Continue automatically
through P00-P19 after each passing gate.
