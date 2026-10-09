# M65 campaign expected/actual evidence reconstruction

Approved G65 SHA: `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`

Previous materialization checkpoint: `8ad6ec57519cd2cf0b56e3228d1983add5655563`

Worker SHA-256: `5611c26224fd060dfdcaaca02ed3a57ce9e30156d8617eaca2d9a6fd9f593199`

Dataset: `ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4`

Architectural contract: `upd9002-v20-architectural-v1` / `aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5`

The reconstructed expected states come exclusively from the approved SST
corpus. Actual states come from the exact approved worker under an
identity-bound selective replay.

The reconstruction does not alter the G65 baseline, target policy,
classifications, selected sets, applicable sets, or official SST results.

## Reconciliation

- Replayed cases: `7511`
- Reused complete raw cases: `0`
- Timeout: `0`
- Crash: `0`
- Official G65 failure signatures: reconciled for every case
- Determinism: byte-identical normalized rows across two replays

## Task Readiness

| Task | Rows | Hash digest | Status |
| --- | ---: | --- | --- |
| M65a | 5000 | `6028d5dcd4b6a3dcded2aaf69fb186e502f7f5a4d094180572f802c86240039a` | execution_ready |
| M65b | 1244 | `2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d` | execution_ready |
| M65c | 1113 | `69bf316c8a0751f7aed67504d0ea606fd2530e8d254b2b4e73ead66ccbc30ccc` | execution_ready |
| M65d | 144 | `ce1bc644ee5a5bc73ae872440ad4446cb0dbccbad626ba93372082fe7add9076` | execution_ready |
| M65e | 10 | `7b228418bf0391884381514282e60ea9ccaf3af8c0f1f7f5a1b038a24de230a1` | execution_ready |

M65h is `conditional_nonblocking` under the maintainer BRKFEM evidence
amendment. BRKFEM is not implemented, not applicable, not officially
executed, and not claimed passing.

Remaining blockers: none

## No-Change Proof

No `cpu/upd9002/` source, target policy, comparison contract, fixture,
selected set, applicable set, or official SST result is changed by this
checkpoint. The amended M65j 5,908-hash backlog remains separate:
`240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5`.

Intermediate campaign checkpoints remain unapproved. Formal human approval is
deferred to terminal G65m. M66 and M67 remain untouched.
