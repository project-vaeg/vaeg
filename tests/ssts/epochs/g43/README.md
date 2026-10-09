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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# Immutable G43 SST evidence references

`manifest.json` content-addresses the existing M43 architectural CI and full
summaries, every compressed failure sidecar, the known-gap evidence, and the
G43 transition record without modifying them. It separately records summary,
failure-index, sidecar-content, sidecar-byte, selected, applicable, pass,
failure, and all top-level classification set digests.

The fixed predecessor for this reference is the maintainer-approved G57 commit
`72322d5c9b8e40e4a988312aebe163a8190e2aa5`. G57 reproduced the same M43
behavior and did not establish a new editable baseline.

Verify all references with:

```sh
python3 tools/qa/upd9002_ssts_ratchet.py verify-static --root .
```

The validator rejects a changed byte in any listed M43 file. The corresponding
negative test modifies an isolated copy and proves rejection; it never rewrites
the repository evidence.
