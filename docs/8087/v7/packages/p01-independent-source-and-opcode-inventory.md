# P01 — Independent source and opcode inventory

**Initial status:** NOT_STARTED  
**Prerequisites:** P00

## Work

Freeze the complete documented 8087 instruction/form inventory independently from production decode. Resolve reset values, CW/SW/TW fields, environment/save formats, WAIT/FN forms, special encodings, and early exception rules from I01-I05/N01.

## Acceptance

2048 D8-DF ModR/M slots classified exactly once; independent selected byte fixtures and form membership agree with sources. Critical bit tables and inequalities are checked against PDF images, not OCR text alone.

Run:

```sh
python3 tools/8087/gate.py --package P01
```

## Required outputs

Normative inventory, source/page ledger, initial opcode fixtures, conflict log.

## Boundaries

No deriving the independent inventory from production tables; no later-x87 substitutions.

Update `../records/progress.md` with actual commands, exit codes, test counts,
source digest, changed paths, and next action. Continue automatically after pass.
