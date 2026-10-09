# Source and provenance policy

## Normative hierarchy

1. Intel Numerics Supplement and original Intel 8087 documentation for 8087 semantics.
2. Intel 8087 datasheet for instruction/data/control/timing corroboration.
3. NEC V30/uPD70116 manuals for CPU-side FPO1/FPO2/POLL, decode, address, and bus behavior.
4. PC-88VA2/VA3 machine evidence for model gating, socket, mode, BUSY/INT routing.
5. Intel E8087 documentation and optional black-box E8087 execution as corroboration only.
6. self-authored guest tests and real hardware traces for disputed details.
7. later x87 documentation only when explicitly shown to describe the 8087.

When sources conflict, record both exact locations and the selected behavior.

## Clean implementation boundary

Do not inspect, copy, translate, paraphrase, or port FPU implementation code from:

- 86Box
- DOSBox-X
- PCem
- Bochs
- QEMU
- Open Watcom `fpuemu`
- PCjs FPU
- Linux `wm-FPU-emu`
- bitblaze-fuzzball x87 code
- i486SX software FPU implementations
- Granite or other microcode-derived implementation source

This is a project provenance rule, not a claim that reading GPL source changes
license status.

Black-box executable comparison is allowed only as a separate validation axis and
may never replace the primary-source oracle.

## Repository content

Do not commit complete copyrighted manuals, full OCR Markdown, ROMs, proprietary
BIOS/BASIC/DOS images, or private traces. Commit only derived factual tables,
citations, hashes, self-authored tests, permissively redistributable upstream
dependency material, and small quoted fragments within normal documentation use.
