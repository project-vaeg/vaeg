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

# M98b - Zundamon orbit public-fixture report

Evaluated predecessor: `49d222048bd050638255aa410b335c42096670df`

Status: **G98b machine gate passed on 2026-08-31**

## Result

M98b adds a source-neutral public fixture under `demos/zundamon-orbit/`. It is
a generated 23x19 abstract indexed marker paired with a 16-entry RGB888
palette and canonical manifest. It is not a depiction of the named subject
and contains no maintainer-supplied bytes or metadata.

The builder writes only to a new directory. The inspector checks canonical
JSON, dimensions, filenames, byte counts, encoding, digests, exact generated
content, index bounds, transparency, near-black, isolated pixels, diagonals,
and transparent holes.

## Machine evidence

All three focused standard-library tests passed and the suite emitted
`M98B_TEST_PASS`.

The tests proved byte-for-byte reproducibility and exercised two independent
stable failure codes:

- `M98B_FIXTURE_PIXELS_SHA` after one indexed-byte mutation;
- `M98B_FIXTURE_OUTPUT_EXISTS` when rebuilding into an existing directory.

The standalone build and inspection reported:

```text
M98B_FIXTURE_BUILD_PASS
manifest_sha256=a857c3860208165149ec4c6cfb09be0f16fe16c7751da94741aabca320e32c18
M98B_FIXTURE_INSPECT_PASS
```

Repository encoding, EOL, case, and diff checks passed. M98b makes no guest,
VAEG, private-input, or physical-machine claim. M98c remains unassigned.
