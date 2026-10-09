# P13 numerical oracle record

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

`tools/8087/mpfr_oracle.c` is a developer-only executable. It is not linked
into VAEG and is not an arithmetic substitute for the vendored Berkeley
SoftFloat Release 3e backend. Inputs are constructed from exact integers,
powers of two, and MPFR constants; host floating-point and libm are not used.

The certificate cases cover `exp2`, `log2`, `log1p`, `sin`, `cos`, and `atan`,
including `pi/4` and the strict FYL2XP1 boundary. A deliberately insufficient
directed interval is rejected. The exact local command and result were:

```text
pkg-config --cflags --libs mpfr gmp
cc -std=c99 -O2 tools/8087/mpfr_oracle.c -o /private/tmp/vaeg-8087-mpfr-oracle $(pkg-config --cflags --libs mpfr gmp)
/private/tmp/vaeg-8087-mpfr-oracle --selftest
P13_MPFR_ORACLE PASS cases=9 precision=192/512
```

Source SHA-256 at this run:

```text
10c4efcf702da561d61d863c8b735a939ab7445d70b61ae07337f41cb2443292  tools/8087/mpfr_oracle.c
```

This is a numerical certificate gate, not a silicon or host-libm equivalence
claim.
