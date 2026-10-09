# M66b uPD9002 remove active i286 identity

M66a and M66b are complete on one linear bundle history.

M66a was technically validated as an internal checkpoint but was not independently approved.

The complete bundle is presented for one terminal human approval at G66b.

G66b remains unapproved pending human review.

M67 has not been started.

## Approved predecessor

- Gate: G65m
- SHA: `81887aae14f718d7d4d0f2a7bd3fe05d5ea80630`
- Branch: `topic/m65-residue-campaign`
- Approved CI: https://github.com/nakatamaho/vaeg/actions/runs/30326346909

## Bundle commits

- Bundle protocol: `4d70784261d066b00e502ae608ad8317658b3dd8`
- M66a audit: `9ce61d3fdf08780b4dfd0b158faf9460635ccbdb`
- M66a evaluated: `45bc97d4bbac401272b27e477121b77afa03edc5`
- M66a checkpoint: `46228bc6c64287689510c7b2c143afb2fc1d3759`
- M66b audit: `3aa7b8a131ea8d45b78dff3024b34567ca2c12ee`
- M66b rename/validator/documentation commits: `92cc7681d22a3f9125c0fceb84578e590d14b5d9`, `ba2b96bfb43f9d4ccc969730280e58d0da1b4bb7`, `c6d26734af605da027d478d8bdac6f60f8cf7534`, `4adc06a5fec22064fbd01e659648ed2b63a4bf09`, `0ddc50d43370d4214abc02a2329253e4bb1ba385`, `475c97dc7e27e82374de47ffae91386f6f7bf832`
- Final M66b evaluated SHA: `475c97dc7e27e82374de47ffae91386f6f7bf832`
- Validator-only SHA: `f461e02de7dd66f6eed7bb99d993912b3fe8f6a4`
- Final evidence/candidate SHA: supplied by handoff; not embedded in artifacts

## Corpus and worker

- Verified corpus root: `/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21`
- Dataset: `ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4`
- Corpus manifest SHA-256: `f9aa17e0f5a24102f437c6ab5a061a891339f49f1d5af88c73228d54205f2d0b`
- G64 support map SHA-256: `96ffc381ab699bb403b45bf2c0a43eb52684e8dc1f8e410417b4e9885c29cc86`
- Final worker SHA-256: `3ae0c8823e5983e983dd85ee34d223072a9c3f9bcdf3dda0e13a84f0124119ca`
- Target policy: `upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6`

The prompt text for the full selected hash omitted one `b`; the committed approved G65m scoreboards and manifest record `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7`, and G66b uses that repository-authoritative identity.

## M66a state compatibility result

- State format before: `{'compatibility_status': 'obsolete_identity_transitional_current_format', 'cpu_payload_size': 112, 'cpu_payload_type': 'Cpu286StateCompat', 'cpu_section': 'CPU286', 'cpu_section_version': 0, 'supplemental_payload_size': 16, 'supplemental_section': 'UPD9002', 'supplemental_section_version': 0}`
- State format after: `{'compatibility_status': 'current_upd9002_state_format', 'cpu_payload_size': 112, 'cpu_payload_type': 'Upd9002StateImage', 'cpu_section': 'UPD9CPU', 'cpu_section_version': 1, 'supplemental_payload_size': 16, 'supplemental_section': 'UPD9002', 'supplemental_section_version': 0}`
- Current uPD9002 round trip: pass
- Deterministic serialization: pass
- Legacy CPU286 rejection: pass
- Failure atomicity/no partial mutation: pass
- Removed compatibility paths: statsave.tbl: CPU286 section version 0 written -> UPD9CPU section version 1 written; statsave.c: CPU286 section handled as current table entry -> CPU286 section is legacy migration only when UPD9002 marker is present; CPU286-only input fails before load mutation; cpu/upd9002/upd9002_state.h: Cpu286StateCompat public state image type and CPU286 error text -> Upd9002StateImage current state image type and uPD9002 error text; cpu/upd9002/upd9002_state.c: Cpu286StateCompat overlay/import/export naming -> Upd9002StateImage overlay/import/export naming

## M66b identity result

- Active identity count before: 3110
- Active identity count after: 0
- Preserved allowlist count: 262447
- Preserved allowlist digest: `efc46b0b154a7df3cb48027439192cd382ee82824b5b7bade485184fbe4a7df4`
- Exported interface audit: pass; documented uPD9002 API preserved; no documented public legacy i286 ABI was removed.

## Final profiles

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash | Pass digest | Failure digest | Signature digest |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- | --- |
| Architectural CI | 180000 | 169300 | 169300 | 0 | 0 | 0 | `6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` |
| Architectural full | 1562502 | 1474594 | 1474594 | 0 | 0 | 0 | `4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` |
| Fingerprint full | 1562502 | 1474594 | 1402202 | 72392 | 0 | 0 | `ea521512c9f49b3a73558db6ccf0a01c6b889d1df8a82fb897a9d9d1af8316f4` | `0692676136061b956d0b7f1c06a35cfc4c5ffff7b925ba83f2d07d37310f22c5` | `79913b4f99c54d263315235829f6f937c5956268d9239a4b371301e8acbcdee8` |

Newly passing: 0. Newly applicable: 0. Newly failing: 0. Changed failures: 0.

Direct transition digest: `b3c550dddd9b23481289222f5ccf0165f72d97dc3cf82295058cf836abdaba93`. Composed transition digest: `b3c550dddd9b23481289222f5ccf0165f72d97dc3cf82295058cf836abdaba93`. Direct equals composed: true.

## Artifact digests

- Manifest: `tests/ssts/campaigns/g66b/manifest.json`
- Artifact tree: `bfc869fa6e7005ff7fcb4acfd6cfb4e4bb7f9e85b7e7ee7782f112de5b18dda3` (terminal report excluded from this digest to avoid self-reference)
- State compatibility inventory: `1ba33e6c60bd367deb00ccfa8174b0ab2e63a64447b2c833bbfcc9acd9a2a353`
- Identity inventory after: `7c56cdf07544d4f7f8e1b5ea0b5f5b2eb18e23b6603ee7c90e34dcabcd03df82` (compressed full artifact: `e8172f26153e2391a273158ffae37a14e5a75a039939d3ac3ca9dce764f98690`)
- Allowlist: `70b116987967729a074047a514d1b3d79309072788b6296eaade00b0e2a76fe7` (compressed full artifact: `f17a42880270d7b3a3483e739f01d5dfee46f682c6708051cece3184b5479bc3`)
- Closure audit: `77938487890d296dd0c66e78b1345b0b5509f467cf0abf0026a7720a556c1c45`

## Validation

- Corpus verification: pass (`ssts-verify`, 360 files)
- Focused M66 state and identity tests: pass
- Architectural CI/full and fingerprint full profiles: executed against the final M66b evaluated worker
- Scoreboard materialization and verification: pass (`tools/qa/upd9002_g66b_closure.py`)
- Hosted CI: pending final evidence push

## Known limitations

G66b is an unapproved terminal candidate until human review. The final evidence commit SHA is intentionally not embedded in artifacts contained by that commit. M67 must start only after approved G66b.
