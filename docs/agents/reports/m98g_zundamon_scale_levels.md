<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98g - Zundamon 30-level scale-set report

Evaluated predecessor: `a08bbb5283a219cea576b16845bbb4571e0d35aa`

Status: **G98g machine gate passed on 2026-08-31; 30-level contract revalidated by M98j; `LOCAL_SCALE_SET_PASS`**

## Result

M98g adds a standard-library generator for exactly 30 center-sampled
nearest-neighbor VA8 frames. It implements the frozen integer dimension
formula, four-byte row pitch, zero row padding, 16-byte frame alignment, and
pixel-center anchor projection. Every level has an independent descriptor,
including levels whose small dimensions duplicate a neighbor.

The output is an intermediate stream plus a deterministic private report.
It deliberately has no atlas header, descriptor binary format, CRC, BMS bank,
or bank-boundary padding. Those contracts remain owned by M98h and M98i.
CLI output is a single neutral success token or a stable error code with
neutral detail.

The approved local bundle passed the same generator without modifying its
inputs. Its ignored stream contains exactly 30 ordered in-range frames and
passed local structural checks. This establishes `LOCAL_SCALE_SET_PASS`; no
private values or identities are recorded here.

## Machine evidence

The M98g standard-library suite ran seven test methods and passed:

```text
OK
M98G_TEST_PASS
```

Coverage includes exact dimension sequences, duplicate-dimension retention,
level-30 source identity, explicit center-sampling and anchor examples, an
independent all-frame pixel oracle, zero row and frame padding, 16-byte frame
offsets, descriptor agreement, byte reproducibility, input immutability,
overwrite refusal, focused stable error codes, and path-redacted CLI success
and failure.

M98b-M98f regression tests, Python compilation, JSON, repository encoding,
EOL, case, diff, and M98 privacy checks passed. Generated local outputs remain
ignored and untracked.

## Boundary

G98g is a host machine gate only. M98g makes no final-atlas, BMS, guest, VAEG,
or physical-machine claim. M98h remains unassigned.
