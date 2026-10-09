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

# M98f - Zundamon VA 8-bpp conversion report

Evaluated predecessor: `9fcb3c15f0c960bc81bceb941c1c730e9e711539`

Status: **G98f machine gate passed on 2026-08-31; `LOCAL_VA8_PASS`**

## Result

M98f adds a standard-library converter from M98d palette indices and RGB888
entries to VA 8-bpp `GGGRRRBB`. It applies the frozen nearest-integer channel
formula, preserves source index 0 as transparent byte zero, and replaces an
opaque zero result with the nearest nonzero decoded VA color. Squared RGB
error is minimized and the lower VA byte breaks ties.

The converter writes row-major raw pixels and a deterministic private JSON
report only to a new output directory. The report contains per-entry errors,
opaque-zero repairs, usage, collisions, and aggregate error. CLI output is a
single neutral success token or a stable error code with neutral detail.

The approved local bundle passed the same converter without modifying its
inputs. Its ignored outputs passed local structural checks. This establishes
`LOCAL_VA8_PASS`; no private values or identities are recorded here.

## Machine evidence

The M98f standard-library suite ran seven test methods and passed:

```text
OK
M98F_TEST_PASS
```

Coverage includes exact channel bit order, channel-rounding boundaries,
decode representatives, opaque-zero nearest-color repair and tie breaking,
an independent whole-fixture oracle, zero exclusivity, collision and error
reporting, byte reproducibility, input immutability, overwrite refusal,
focused stable error codes, and path-redacted CLI success and failure.

M98b-M98e regression tests, Python compilation, JSON, repository encoding,
EOL, case, diff, and M98 privacy checks passed. Generated local outputs remain
ignored and untracked.

## Boundary

G98f is a host machine gate only. M98f makes no scaling, atlas, guest, VAEG,
or physical-machine claim. M98g remains unassigned.
