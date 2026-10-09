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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
-->
# M67 uPD9002 divergence consolidation

M67 consolidates the current divergence, uncertainty, zero-coverage, and
diagnostic evidence domains into one content-addressed canonical registry.

M67 makes no CPU semantic, target-policy, classification, applicability,
dataset, fixture, or comparison-contract change.

G67 remains unapproved pending human review.

The next milestone has not been started.

## Identity

- Approved predecessor gate: G66b
- Approved predecessor SHA: `97f760e8da573888edf089c2875c623895a3c2c9`
- Branch: `topic/m67-upd9002-divergence-consolidation`
- Canonical registry: `tests/ssts/divergence/g67/registry.json`
- Registry SHA-256: `6ab9865a9a0617b575f630cef57533998446eaed9411b3237670c74f92ca9ae5`
- Registry records: 31
- Artifact tree SHA-256: `882cb94c94c9b887d1b671b4178ffe05ffb8befd72894e63b7824e7956830f2a`

## Record counts by kind

| Kind | Count |
|---|---:|
| `documented_target_absence` | 3 |
| `fingerprint_only_diagnostic` | 1 |
| `hardware_evidence_pending` | 1 |
| `reserved_behavior_question` | 1 |
| `state_compatibility_exception` | 1 |
| `target_support_unverified` | 19 |
| `upstream_nonblocking` | 3 |
| `zero_coverage_evidence_backlog` | 2 |

## Source migration

- Source records: 78
- Migration map: `tests/ssts/divergence/g67/source_migration.json`
- Migration map SHA-256: `411b617779680002ccc9fd427e2775cab921e4d0db3b7df33ab0047aa20ca610`
- Historical or superseded sources are preserved; no approved historical report is rewritten.
- Conflict count: 0

## Protected domains

- M65j target-support-unverified groups: 19
- M65j hash count: 5908
- M65j hash-set SHA-256: `240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5`
- M65j records remain implemented=false, applicable=false, officially_executed=false, passing_claim=false.
- 6C-6F remain documented target absence outside the blocking denominator; production-handler cleanup remains a separate evidence question.
- 66/67/FPO2 remain upstream-nonblocking / hardware-question records; monitor-disassembler absence is not silicon absence.
- BRKEM `0F FF imm8` remains zero-coverage evidence backlog with no approved executable corpus.
- BRKFEM `0F FE imm8` remains evidence backlog; immediate/vector, entry mode, frame/stack, BRKEM, RETEM, and CALLN questions remain unresolved.
- Fingerprint full remains diagnostic only: 1,402,202 pass / 72,392 fail; blocking_architectural=false.
- The G66b state migration bridge remains exact: CPU286 v0 + UPD9002 v0 migrates to UPD9CPU v1 + UPD9002 v0; broader CPU286 compatibility is prohibited.

## No-change proof

- Worker SHA-256 reused from G66b: `3ae0c8823e5983e983dd85ee34d223072a9c3f9bcdf3dda0e13a84f0124119ca`
- Target policy before/after: `upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6` / `upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`
- Classification changes: 0
- Gap-kind changes: 0
- Newly passing: 0
- Newly applicable: 0
- Newly failing: 0
- Changed failures: 0

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash | Pass digest | Failure digest | Signature digest |
|---|---:|---:|---:|---:|---:|---:|---|---|---|
| architectural_ci | 180000 | 169300 | 169300 | 0 | 0 | 0 | `6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` |
| architectural_full | 1562502 | 1474594 | 1474594 | 0 | 0 | 0 | `4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` |
| fingerprint_full | 1562502 | 1474594 | 1402202 | 72392 | 0 | 0 | `ea521512c9f49b3a73558db6ccf0a01c6b889d1df8a82fb897a9d9d1af8316f4` | `0692676136061b956d0b7f1c06a35cfc4c5ffff7b925ba83f2d07d37310f22c5` | `79913b4f99c54d263315235829f6f937c5956268d9239a4b371301e8acbcdee8` |

## Generated views

- Approved target divergences view: `tests/ssts/divergence/g67/approved_target_divergences_view.json`
- Hardware-pending view: `tests/ssts/divergence/g67/hardware_pending_view.json`
- Evidence-backlog view: `tests/ssts/divergence/g67/evidence_backlog_view.json`
- Zero-coverage view: `tests/ssts/divergence/g67/zero_coverage_view.json`
- Fingerprint diagnostics view: `tests/ssts/divergence/g67/fingerprint_diagnostics_view.json`
- State compatibility exceptions view: `tests/ssts/divergence/g67/state_compatibility_exceptions_view.json`

## Validation

- M67 source inventory verification: pass
- Schema and record-ID stability checks: pass
- Migration-map completeness: pass
- Ownership and M65j union/non-overlap verification: pass
- Generated-view equivalence: pass
- Zero coverage not described as passing: pass
- Fingerprint-only records not architectural blocking: pass
- State bridge not broadened: pass
- Deterministic double generation: pass
- Hosted CI: to be supplied by final handoff.

## Known limitations

- M67 is a registry and evidence-consolidation milestone. It does not claim complete uPD9002 silicon validation.
- BRKEM, BRKFEM, FPO2, prefix/restart, and reserved behavior questions remain intentionally unresolved until separate approved evidence gates.

## Next predecessor wording

The next milestone may start only after G67 is formally approved at the final 40-hex candidate SHA.
