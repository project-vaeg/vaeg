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
# uPD9002 semantic evidence-pack schema version 1

M59 evidence is observation only. It does not change the SST comparison
contract, classification, fixture, or production CPU behavior.

The pack root is `tests/ssts/evidence/g59/`. `manifest.json` identifies the
approved G58 predecessor, the G58 implementation SHA, the M59 analysis
implementation SHA, the corpus and comparison contracts, the generation
environment, every artifact's byte count and row count, and every artifact's
SHA-256 digest. The manifest never contains the SHA of its own evidence
commit.

## Canonical representation

JSON objects use lexicographically sorted keys, compact separators, ASCII
escaping, UTF-8 encoding, and one trailing LF. Arrays whose order is part of
identity are sorted by the documented key. Case rows are ordered by canonical
case SHA-256.

Machine case tables use one deterministic gzip member with an empty filename,
compression level 9, `mtime=0`, XFL 2, and OS 255. The payload is canonical
JSON plus one LF. Validation checks the complete DEFLATE member, CRC-32,
uncompressed size, canonical payload, and the committed compressed-byte
SHA-256. Repeated output is byte-identical only within the recorded
Python/zlib environment; other conforming zlib implementations may emit a
different valid DEFLATE stream.

## Case identity and execution

`case_hash` is SHA-256 over the M43 canonical serialization of the complete
upstream SST record. `upstream_case_hash` is the corpus-supplied SHA-1.
`selected=true` means the record belongs to the verified full empty-prefetch
profile. `executed=true` means it was in the G58 architectural applicable
denominator. A non-applicable row may contain an actual state only when
`diagnostic_replayed=true`; that replay remains `official_outcome=skip` and
does not turn the record into a passing reference.

Every row contains initial architectural state, expected final state, actual
final state, expected and actual termination/execution observations,
architectural and all-16-bit FLAGS mismatch kinds, logical and physical
addresses when derivable, structural labels, and an exact conclusion status:
`proven`, `hypothesis`, or `underdetermined`.

The only top-level classifications are:

- `applicable`
- `known_target_gap`
- `expected_target_divergence`
- `unsupported_fixture`
- `upstream_nonblocking`

Known-gap rows carry exactly one approved G58 `gap_kind`. M59 does not edit
that taxonomy.

## Tables, summaries, and representatives

There is one unambiguous case table, summary, and readable Markdown
representative dump for each of the six analysis items. A table is keyed by
the unique ordered `case_hash`. Its `row_count` equals the number of rows.

A summary records exact form/classification populations, primary exclusive
partition coverage, mismatch and termination counts, the table hash-set
digest, representative hashes, specialized item analysis, and labelled
conclusions. Primary partitions must cover every row exactly once.
Representative hashes must exist in the corresponding machine table and be
present in the readable dump.

The executable validator and fail-closed tests are:

```text
python3 tools/qa/upd9002_semantics_evidence.py selftest
python3 tools/qa/upd9002_semantics_evidence.py verify-static --root .
```

They also pin and verify the complete approved G58 artifact tree and immutable
G43/M43 references before accepting an M59 pack.
