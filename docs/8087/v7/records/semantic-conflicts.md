# v7 semantic conflict decisions

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The normative hierarchy in `source-policy.md` selects the Intel Numerics
Supplement (I02) when its operation description conflicts with a later ASM86
summary table (I04). The conflict is retained here so endpoint behavior is
auditable rather than silently normalized.

| instruction | I02 operation description | I04 summary/table | selected behavior |
| --- | --- | --- | --- |
| FPTAN | `0 < theta < pi/4`, I02 S-37 | `0 <= ST(0) <= pi/4`, I04 6-179 summary | require a positive normalized operand strictly below pi/4 |
| FPATAN | `0 < Y < X < infinity`, I02 S-37 | `0 <= ST(0) < ST(1) < +infinity`, I04 6-179 summary | require positive normalized Y and X with Y strictly below X |
| FYL2XP1 | `0 < abs(X) < (1 - sqrt(2)/2)`, I02 S-38 | same strict lower bound in I04 6-199 | reject signed zero and use the exact upper-bound fixture |
