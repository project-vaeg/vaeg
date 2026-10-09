# 8087 execution-clock evidence

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

The timing source is the supplied I04 user-OCR execution-clock table. The
first complete rows occur at OCR line 28261 and the table continues through
line 36410. It covers the documented F2XM1/FYL2X/FPTAN/FPATAN, FPREM,
FSCALE, FXTRACT, transfer, arithmetic, compare, control, environment, and
packed-BCD entries. The OCR identity is already checked by the P00 evidence
gate; this record does not copy the manual or OCR into VAEG.

`records/timing-table.tsv` contains one row for every one of the 111 inventory
forms. Its expansion is 1,597 documented slots. The `nominal_ndp_clocks`
column is the table's nominal base value; `ea=1` records the source's separate
`+ EA` term. `upd8087_instruction_cycles()` returns that nominal base, while
the CPU-side effective-address/bus work remains in the native CPU path. This
separation is intentional: the v7 scheduler converts the selected NDP base
service clocks with the configured integer oscillator and carries its residue.

The content audit is executable:

```text
python3 tools/8087/audit_timing.py
P19_TIMING rows=111 slots=1597
```

The production core test additionally checks that every independently
enumerated documented slot has a nonzero timing entry. The table is a local
8087 timing record, not a claim of cycle-exact CPU/NDP overlap or a VAEG
hardware interrupt-route proof.
