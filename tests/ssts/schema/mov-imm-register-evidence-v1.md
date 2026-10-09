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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# MOV-immediate register evidence schema v1

M61 records every selected C6 and C7 SST case with side-by-side initial,
expected, and actual state. Case rows are ordered by the canonical SST case
hash. Structural partitions are derived exclusively from instruction bytes and
metadata, before execution outcomes are inspected.

The `modrm` object records `mod`, `reg_extension`, and `rm` separately. A
register destination is selected by the executed SST-observed `rm` field. The
`reg_and_rm_same` field distinguishes real pre-fix execution from coincidental
agreement, and `value_coincidence` records whether the initial destination
already equals the immediate without treating that equality as execution.

All JSON uses canonical sorted keys. Gzip uses the repository deterministic
writer with a zero modification time and no embedded filename. Unknown schema
versions, missing or duplicate hashes, missing actual state, drift in governing
identities, unordered rows, and inconsistent counts or digests are rejected.
