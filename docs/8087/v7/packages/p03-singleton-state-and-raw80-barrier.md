# P03 — Singleton state and raw80 barrier

**Initial status:** NOT_STARTED  
**Prerequisites:** P01, P02

## Work

Implement one optional machine-owned 8087 state, eight raw80 physical registers, TOP/tags, CW/SW/pointers/opcode/pending state, explicit codecs/classifier, reset, and scoped SoftFloat state management.

## Acceptance

All raw classes round-trip bit-exactly through codecs; TOP/alias tests pass; repeated init/reset never creates a second device; SoftFloat mutable state never leaks across calls or early exits.

Run:

```sh
python3 tools/8087/gate.py --package P03
```

## Required outputs

State implementation, codecs/classifier, state/reset/backend-context tests.

## Boundaries

No device arrays, raw-struct serialization, or native floating formatting in core.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
