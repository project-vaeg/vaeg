# P17 — VA machine integration and production interrupt route

**Initial status:** NOT_STARTED  
**Prerequisites:** P16

## Work

Use available VAEG/VA machine evidence to finish the production BUSY/INT adapter, native-mode gating, controller polarity/masking/acknowledgment, pending restore, and complete nominal timing table.

## Acceptance

A self-authored guest raises an unmasked 8087 exception through a real ESC, enters via the production emulated controller, clears/acknowledges, returns, and continues. Timer ordering, CPU masking, mode transitions, absent/model-disabled paths, and full baseline pass.

Run:

```sh
python3 tools/8087/gate.py --package P17
```

## Required outputs

Evidence-backed production route, guest handler tests, complete timing audit.

## Boundaries

If route evidence is truly unavailable, mark INTEGRATION_BLOCKED; never invent PC/AT IRQ13/NMI/#MF.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
