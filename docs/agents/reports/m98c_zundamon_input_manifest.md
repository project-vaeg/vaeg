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

# M98c - Zundamon orbit local-input manifest report

Evaluated predecessor: `8ad1aac0b4632c0eca5873aaedf23a694647721c`

Status: **G98c machine gate passed on 2026-08-31**

## Result

M98c freezes `vaeg-zundamon-orbit-input-v1` as a source-neutral local-input
contract. A manifest names one sibling `bmp32` image and one sibling
16-entry `rgb888` palette by lowercase basename, then declares a bounded crop,
an exact RGB transparency value, and a crop-top-left-relative integer anchor.

The closed schema has no free-form or source-identifying field. It requires
neutral local basenames and rejects absolute paths, directory separators, and
parent references. The validator does not open referenced files and does not
echo the manifest path or referenced basenames in diagnostics.

M98c validates manifest structure only. Image dimensions and pixels, palette
bytes, crop bounds against the actual image, and color membership remain M98d
work.

## Machine evidence

The standard-library suite ran seven test methods and passed:

```text
OK
M98C_TEST_PASS
```

Those methods cover the neutral example, schema/validator constant and key-set
agreement, 20 isolated one-mutation cases, non-object input, duplicate JSON
members, BOM, invalid UTF-8, malformed JSON, excessive size, CLI success
without referenced files, and path-redacted CLI read failure. Every negative
case asserts an exact `M98C_*` error code.

The standalone validator reported:

```text
M98C_MANIFEST_PASS
```

Python compilation with an explicit temporary bytecode cache, repository
encoding, EOL, case, diff, and M98 privacy scans passed. No maintainer-supplied
input was read. M98c makes no file-content, image-approval, guest, VAEG, or
physical-machine claim. M98d remains unassigned.
