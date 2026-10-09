# Verification contract

P00 creates or integrates a real gate runner:

```sh
python3 tools/8087/gate.py --list
python3 tools/8087/gate.py --package P00
python3 tools/8087/gate.py --through P07
python3 tools/8087/gate.py --final
```

A missing or skipped required test is a failure. A zero-test discovery is a
failure.

Required layers:

| ID | Layer |
|---|---|
| V00 | repository baseline and existing regressions |
| V01 | OCR/source inventory and independent opcode/form membership |
| V02 | exact SoftFloat 3e build plus TestFloat/backend checks |
| V03 | raw80/state/TOP/tags/CW/SW |
| V04 | clock conversion and scheduler |
| V05 | production CPU/bus/FPO1/FPO2/8080 separation |
| V06 | exception transaction and WAIT/FN |
| V07 | production vertical slice |
| V08 | transfers including BCD |
| V09 | arithmetic/PC/RC |
| V10 | compare/control/constants/stack |
| V11 | 14-byte/94-byte guest images |
| V12 | FPREM/FSCALE/FXTRACT |
| V13 | independent numerical oracle/prototypes |
| V14 | F2XM1/FYL2X/FYL2XP1 |
| V15 | FPTAN/FPATAN |
| V16 | full exceptional-case closure |
| V17 | production VA BUSY/INT route and timer behavior |
| V18 | config/savestate/frontend lifecycle |
| V19 | final zero-omission/provenance audit |

Use self-authored redistributable guest programs executed through the real VAEG
CPU and memory bus. Direct handler-call tests are useful unit tests but cannot
replace integration tests.

Final reporting must separate local software verification from cross-host,
real-hardware, silicon-numerics, and cycle-accuracy evidence.
