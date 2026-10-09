<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD. -->

# G64 architectural-full failure ranking

Total remaining failures: **7,511**.

| Rank | Form | Pass | Fail | Change from G62 | Cumulative |
| ---: | :--- | ---: | ---: | ---: | ---: |
| 1 | `FF.7` | 0 | 5,000 | +0 | 66.57% |
| 2 | `62` | 3,756 | 1,244 | +0 | 83.13% |
| 3 | `F7.2` | 3,887 | 1,113 | +0 | 97.95% |
| 4 | `FF.6` | 4,856 | 144 | +0 | 99.87% |
| 5 | `61` | 4,997 | 3 | +0 | 99.91% |
| 6 | `81.6` | 4,999 | 1 | +0 | 99.92% |
| 7 | `9C` | 4,999 | 1 | +0 | 99.93% |
| 8 | `A5` | 1,869 | 1 | +0 | 99.95% |
| 9 | `C4` | 4,999 | 1 | +0 | 99.96% |
| 10 | `C8` | 999 | 1 | +0 | 99.97% |
| 11 | `D1.6` | 4,999 | 1 | +0 | 99.99% |
| 12 | `FF.5` | 4,999 | 1 | +0 | 100.00% |
| 13 | `00` | 5,000 | 0 | +0 | 100.00% |
| 14 | `01` | 5,000 | 0 | +0 | 100.00% |
| 15 | `02` | 5,000 | 0 | +0 | 100.00% |
| 16 | `03` | 5,000 | 0 | +0 | 100.00% |
| 17 | `04` | 5,000 | 0 | +0 | 100.00% |
| 18 | `05` | 5,000 | 0 | +0 | 100.00% |
| 19 | `06` | 5,000 | 0 | +0 | 100.00% |
| 20 | `07` | 5,000 | 0 | +0 | 100.00% |
| 21 | `08` | 5,000 | 0 | +0 | 100.00% |
| 22 | `09` | 5,000 | 0 | +0 | 100.00% |
| 23 | `0A` | 5,000 | 0 | +0 | 100.00% |
| 24 | `0B` | 5,000 | 0 | +0 | 100.00% |
| 25 | `0C` | 5,000 | 0 | +0 | 100.00% |
| 26 | `0D` | 5,000 | 0 | +0 | 100.00% |
| 27 | `0E` | 5,000 | 0 | +0 | 100.00% |
| 28 | `0F10` | 5,000 | 0 | +0 | 100.00% |
| 29 | `0F11` | 5,000 | 0 | +0 | 100.00% |
| 30 | `0F12` | 5,000 | 0 | +0 | 100.00% |

All requested DIV/IDIV and SST-covered monitor `0F` forms have explicit zero-failure rows in the machine-readable ranking. `0FFF BRKEM` has metadata but no v20 SST shard, so it has zero selected and executed cases and is recorded separately rather than inferred to pass. `0F28 ROL4` and `0F2A ROR4` remain green. Omission from this top-30 view is not proof of passing.
