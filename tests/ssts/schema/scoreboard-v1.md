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
# uPD9002 SST scoreboard schema version 1

All scoreboard, failure-shard, and transition JSON is UTF-8, serialized with
lexicographically sorted object keys, no insignificant whitespace, and one
trailing LF. A digest of a hash set is SHA-256 over the same canonical JSON
serialization of the unique lowercase hashes in lexical order.

`vaeg-upd9002-ssts-scoreboard-v1` identifies one profile and one scope.
`architectural/ci`, `architectural/full`, and `fingerprint/full` are distinct
artifact families. The architectural profile is blocking and uses the
metadata-defined FLAGS mask. The fingerprint profile is diagnostic,
non-blocking, and compares all 16 FLAGS bits. Both exclude cycles, prefetch,
and bus timing.

Each `records` element is an outcome-independent corpus structural form plus
one top-level classification. `opcode` is a lowercase byte. For opcode `0f`,
`subform` is the lowercase second byte with an optional `.0` through `.7`
group suffix. Other grouped opcodes use `.0` through `.7`; ungrouped opcodes
use `-`. `form` is the corresponding uppercase corpus form. Non-applicable
records have zero `executed`, `pass`, and `fail`; they are never presented as
passes. In applicable records, `selected == executed == pass + fail`.
`termination_classes` and `mismatch_classes` count failed records in that
form; the summary-level termination counter covers every executed record.

`vaeg-upd9002-ssts-scoreboard-failures-v1` shards failed records by the first
hex digit of `form`. Entries are ordered by the canonical record SHA-256 and
contain the upstream test SHA-1, failure-signature SHA-256, exact form,
FLAGS mask, mismatch classes, and expected/actual termination. Gzip uses an
empty filename, compression level 9, `mtime=0`, no optional header fields,
XFL 2, and OS 255. Validation requires one complete member, canonical JSON,
and matching CRC-32 and size. The artifact SHA-256 pins the exact compressed
bytes without requiring different zlib implementations to emit the same
valid DEFLATE stream.

`vaeg-upd9002-ssts-transition-v1` compares the scoreboard with an explicitly
supplied approved predecessor SHA. Hash arrays and classification changes are
ordered by record SHA-256. Changed failure signatures require deterministic
content-addressed shards and human review; the G58 default ratchet rejects
them unless a milestone-specific driver supplies a changed-failure output
directory.

`vaeg-upd9002-ssts-changed-failures-v1` shards every changed signature by the
first hexadecimal digit of `form`. Each record contains the complete
normalized failure entry before and after the candidate, including both
signature digests, forms, upstream identities, mismatch classes, FLAGS masks,
and termination classes. The transition records the exact compressed and
canonical-content SHA-256 digests and the total count. The same deterministic
gzip contract used by ordinary failure shards applies. Merely enumerating a
changed failure does not approve it; the gate still requires human review.

The executable schema validator and positive/negative tests are in
`tools/qa/upd9002_ssts_ratchet.py`. Unknown fields, versions, classifications,
gap kinds, malformed forms, inconsistent counts, nondeterministic order, and
nondeterministic gzip all fail closed.
