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
# uPD9002 target-authority epoch schema version 1

M60b separates target policy from the immutable SST dataset and comparison
contracts. It changes the blocking denominator for one content-addressed
target-authority correction and does not describe a CPU semantic improvement.

## Canonical representation

Every JSON artifact uses UTF-8, lexicographically sorted object keys, compact
separators, ASCII escaping, and one trailing LF. Identity arrays use the
documented lexical key. Hexadecimal byte and address fields use lowercase
fixed-width notation. Canonical identity excludes timestamps and local source
paths.

Gzip shards use the deterministic writer defined by scoreboard schema version
1: one member, an empty filename, compression level 9, `mtime=0`, XFL 2, and
OS 255. Validators check the member, canonical uncompressed JSON, CRC-32,
uncompressed size, compressed-byte SHA-256, and canonical-content SHA-256.
Byte identity is claimed only within the recorded Python and zlib
environment.

## ROM authority pack

`tests/ssts/authority/g60b/manifest.json` has schema
`vaeg-upd9002-rom-authority-manifest-v1`. It identifies G60b, the approved
G60a SHA, the source ROM by size and cryptographic digest, the ROM mapping
convention, table boundaries, record counts, extraction algorithm, debugger
evidence digest, and every generated artifact by path, size, row count, and
SHA-256.

The files named by the manifest use these schema identifiers:

- `vaeg-upd9002-rom-source-provenance-v1`
- `vaeg-upd9002-rom-map-v1`
- `vaeg-upd9002-rom-dispatch-table-v1`
- `vaeg-upd9002-rom-expanded-dispatch-v1`
- `vaeg-upd9002-rom-mnemonic-map-v1`
- `vaeg-upd9002-rom-string-pool-audit-v1`
- `vaeg-upd9002-debugger-evidence-v1`
- `vaeg-upd9002-rom-authority-conclusions-v1`

The complete ROM is never part of the pack. The manifest binds lawful,
minimal decoded evidence to the independently supplied source ROM.
The main-dispatch authority records all 140 `(mask,value,group)` entries at
ROM file offsets `0x66350` through `0x664f3`, their 140 parallel mnemonic
entries, and the decoder/pointer bytes that prove both boundaries. The
separate twelve-record group subdispatch near `0x66900` is retained as
corroborating structure and is not used alone to infer primary-opcode
absence.

## Target policy

`tests/ssts/target_policy/g60b.json` has schema
`vaeg-upd9002-target-policy-v1`. Its digest is SHA-256 over the complete
canonical object except `target_policy_id` and `target_policy_sha256`.
`target_policy_id` is `upd9002-g60b-` followed by that digest.

The policy records dataset and comparison-contract identities, selected hash
sets, predecessor and candidate applicable hash sets, the ROM-authority
manifest digest, taxonomy totals, and all structural selector rules. M60b
permits only decoded primary opcodes `0x6c` through `0x6f` to transition from
`applicable` to `known_target_gap/documented_silicon_absent`. Prefix decoding,
not the first instruction byte, determines the primary opcode. The selector
union covers all selected records for those opcodes and must be disjoint and
independent of execution outcome.

The predecessor target-policy ID is derived from the exact approved G60a
support map, known-gap taxonomy, divergence registry, and hardware-pending
registry. Approved G60a scoreboards are not rewritten.

## Retired-applicable and classification shards

`vaeg-upd9002-retired-applicable-v1` shards preserve each retired hash's G60a
pass or failure result. A failed row also preserves its failure-signature
digest. `vaeg-upd9002-classification-changes-v1` shards enumerate the same
formerly applicable hash set and its authorized structural transition.

Retired pass and failure sets must be disjoint and complete. Neither set is
`newly_passing`, and retired passes are not candidate passes. Counts and hash
digests are verified against the approved G60a architectural scoreboard.

## Scoreboards and transitions

`vaeg-upd9002-ssts-scoreboard-v2` adds `target_policy_id` and
`target_policy_sha256` to scoreboard version 1. All other architectural,
fingerprint, structural-record, and deterministic failure-shard contracts
remain unchanged.

`vaeg-upd9002-target-authority-transition-v1` compares the exact intersection
of the G60a and G60b applicable sets and separately records retired
applicable hashes. It requires:

- `transition_kind=target_authority_correction`;
- explicit G60a gate and SHA;
- unchanged dataset, comparison contract, and selected set;
- an authority-manifest digest;
- exact retired pass/failure and classification-change shards;
- no changed result or failure signature in the unaffected applicable set;
- no new passing or failing hash;
- zero timeout and crash counts.

The transition fails closed for an opcode outside `0x6c` through `0x6f`,
incomplete or overlapping retired sets, an outcome-derived selector, or a
change to protected 0F28 or 66/67 policy.

The executable generator, validator, positive tests, and fail-closed tests
are in `tools/qa/upd9002_m60b_authority.py`.
