<!-- Copyright (c) 2026 Nakata Maho -->
# M66a — Remove obsolete CPU286 save-state compatibility

M66a is complete within the combined M66 bundle.

This checkpoint is technically validated but not independently approved.

Formal human approval is deferred to terminal gate G66b.

M66b has not yet been completed.

## Approved predecessor

- Approved predecessor gate: `G65m`
- Approved predecessor SHA:
  `81887aae14f718d7d4d0f2a7bd3fe05d5ea80630`
- Approved predecessor branch: `topic/m65-residue-campaign`
- Approved predecessor CI:
  `https://github.com/nakatamaho/vaeg/actions/runs/30326346909`
- Bundle branch: `topic/m66-upd9002-remove-i286-compat`
- Bundle protocol SHA:
  `4d70784261d066b00e502ae608ad8317658b3dd8`

## Commits

- Audit/inventory SHA:
  `9ce61d3fdf08780b4dfd0b158faf9460635ccbdb`
- Audit classification correction SHA:
  `08d4ba6ebc990dffc5d8753d2b33cfb75a3cb8a2`
- Evaluated SHA:
  `45bc97d4bbac401272b27e477121b77afa03edc5`
- Validator/provenance SHA:
  `fc5a78de52116ce9f9533357129cdf7790041f35`
- Evidence/checkpoint SHA:
  supplied by the final handoff for the commit containing this report.

## State-format decision

M66a uses an explicit current schema revision.

- Before: CPU runtime section `CPU286`, version `0`, 112-byte payload
  `Cpu286StateCompat`, plus the existing `UPD9002` register section.
- After: CPU runtime section `UPD9CPU`, version `1`, 112-byte payload
  `Upd9002StateImage`, plus the unchanged `UPD9002` register section.
- The CPU payload byte layout remains 112 bytes and little-endian.
- New saves no longer emit the obsolete `CPU286` section.

The approved-predecessor transitional format is handled only when the old
`CPU286` CPU section is present with the current uPD9002 `UPD9002` marker.
That transitional state loads and re-saves as `UPD9CPU` version 1. A legacy
`CPU286` state without the uPD9002 marker fails closed before load mutation.

## Removed compatibility paths

- `statsave.tbl`: current writer moved from `CPU286` v0 to `UPD9CPU` v1.
- `statsave.c`: old `CPU286` state no longer appears as a current table entry;
  old input is validated only for the predecessor transitional migration or
  rejected before load mutation.
- `cpu/upd9002/upd9002_state.h`: state API names and diagnostics are
  uPD9002-owned.
- `cpu/upd9002/upd9002_state.c`: import/export uses `Upd9002StateImage`.

## Compatibility matrix

- Current M66a `UPD9CPU` + `UPD9002`: write and read.
- Approved-predecessor transitional `CPU286` + `UPD9002`: migrate/read, then
  save as `UPD9CPU`.
- Legacy `CPU286` without `UPD9002`: reject.
- Wrong CPU identity: reject.
- Unsupported protected-mode bit: reject.
- Malformed payload size: reject.
- Truncated current payload: reject.

All rejection cases leave the live CPU state, uPD9002 register section, PCCORE,
and represented memory unchanged.

## Evidence

- Before inventory:
  `tests/ssts/campaigns/g66b/state_compat_inventory_before.json`
  - SHA-256:
    `1ab16cb89c7570b87a9c794115dcb491afc54de284da605096d99ec0c733867d`
  - Active CPU286 state-compat occurrences before: `176`
- After inventory:
  `tests/ssts/campaigns/g66b/state_compat_inventory_after.json`
  - SHA-256:
    `41f984e8b5e6c73d84411eccc48d7cb8cf068355fb53f51cda485eb24fcfd0dd`
  - Active CPU286 state-compat occurrences after: `0`
- Current state format:
  `tests/ssts/campaigns/g66b/current_state_format.json`
  - SHA-256:
    `11d8705211894230f506f54a20c1e59c69178fb2a9eb887a3a9f27eb25b0a075`
- Removed compatibility matrix:
  `tests/ssts/campaigns/g66b/removed_state_compat.json`
  - SHA-256:
    `bb3281eea9ab35c7d6d36ad9c7cb4e3a4017fb73a26d749eedf527ce1a96bbcb`
- M66a checkpoint:
  `tests/ssts/campaigns/g66b/checkpoints/m66a.json`
  - SHA-256:
    `904698f3e1fe39d9688a505a5bb2ad414ffafddd2819b66f2fee70d0b53a1f8c`

## Validation

- `python3 tools/qa/upd9002_m66_state.py --root . verify-m66a`: pass
- Focused state CTest subset:
  `vaeg_upd9002_abi`, `vaeg_upd9002_state_boundary`,
  `vaeg_upd9002_state_payload_probe`, `vaeg_upd9002_m66a_state_verify`:
  4/4 pass
- `ctest --test-dir build/linux-ci-gcc -L romless --output-on-failure`:
  61/61 pass; the pre-existing external SST CI test is skipped by its existing
  external label configuration.
- `tools/qa/upd9002_m65_reconstruct.py verify`: pass
- `tools/qa/upd9002_m65j_campaign.py`: pass
- `tools/qa/milestone_ids.py --selftest --audit --discover`: pass
- `tools/repo/check_encoding.py`, `tools/repo/check_eol.py`,
  `tools/repo/check_case.py`: pass
- `git diff --check`: pass

Worker SHA-256 for the validated Linux CI GCC build:
`9d2700b225f8bb3124a9cdf72932735c35a8335149357387c8a2a5ad341b73f5`.

## Policy and semantic invariants

- Target policy before/after:
  `upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`
- Architectural CI selected/applicable identities are unchanged:
  `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` /
  `6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f`
- Full selected/applicable identities are unchanged:
  `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` /
  `4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c`

No CPU instruction semantics, target policy, classification, selected set,
applicable set, SST fixture, or comparison contract changed.

## Known limitations

M66a intentionally does not remove broad active `i286`/`i286c` implementation
identity. That is the M66b bundle phase.

Next bundle phase: M66b.
