<!-- Copyright (c) 2026 Nakata Maho -->

# uPD9002 FPO2 target-authority audit schema v1

License: BSD-2-Clause.

This schema defines the deterministic M60c audit of SST primary opcodes
`66` and `67`, the monitor ordinary and group dispatch paths, and the
D8-DF FPU decoder. It records target authority; it does not define silicon
instruction semantics.

## Identity

Every G60c authority manifest records:

- schema and schema version;
- milestone, candidate gate, approved predecessor gate and SHA;
- the analysis/evaluated commit SHA;
- the G60b ROM, authority-manifest, target-policy, dataset, and comparison
  contract identities;
- selected and applicable hash-set digests;
- every artifact path, byte count, row count, and SHA-256 digest.

Canonical JSON uses sorted keys, compact separators, lowercase fixed-width
hexadecimal, LF, and no timestamp. The case table is canonical JSON compressed
by the repository deterministic gzip writer.

## SST audit

The full case table is keyed and ordered by the SHA-256 of each canonical SST
record. Each row records the upstream test hash, complete instruction bytes,
decoded prefix sequence, primary opcode, upstream metadata name/status/
architecture, top-level classification, dispatch/support-map ownership,
CI/full selection, and execution status. Nonexecuted rows have explicit zero
pass/fail counts; absence from a failure index is never treated as passing.

The summary partitions all selected 66/67 rows by opcode, scope, prefix class,
classification, ModR/M mode, and length. Structural selection decodes the
primary opcode after every recognized prefix and rejects first-byte-only
selection.

## ROM authority

The group-dispatch record schema records raw `(mask, value, group)` bytes,
ordered expansion, handler-pointer ownership, and the complete twelve-record
boundary. Overlap is accepted only for the exact ordered group candidates
proved by the decoder; unexplained overlap is rejected.

The FPU record schema is `(mask16-le, value16-le, group8)`. The matched
16-bit word has the primary opcode in the high byte and the following byte in
the low byte. Each of the four bounded tables has a parallel high-bit-
terminated ASCII mnemonic table and a group-handler pointer table.

Decoder-path records contain a bounded ROM range, raw-byte digest, incoming
condition, outgoing target, reachability from the normal disassembler entry,
operand-byte consumption, and neutral pseudocode. A support conclusion cannot
be derived from generic mnemonic strings, ordinary-main-table absence,
failure-list absence, or behavior of another CPU core.

## Formal support conclusion

Each resolved population has exactly one value:

- `target_support_proven`;
- `target_absence_proven`;
- `target_support_unverified`.

`target_support_proven` requires a reachable positive dispatch and operand
mapping. `target_absence_proven` requires every reachable alternative to be
bounded and to positively exclude the encoding. Otherwise the result is
`target_support_unverified`.

M60c cannot change top-level classification, selected/applicable membership,
or comparison contracts. With no taxonomy or hardware-pending change, the
G60b policy ID is preserved and the transition kind is
`target_authority_audit`.
