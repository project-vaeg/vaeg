<!-- Copyright (c) 2026 Nakata Maho -->
# M65b — BOUND

M65b is complete within the serial M65 residue campaign.

Campaign base:
G65 at `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`

Campaign predecessor:
`057489a98aac5f976b82530916d15c73541036a5`

Task evaluated SHA:
`d0e01694a9b82b4cd16500743d77e45459c74be1`

Validator-only SHA:
`49867cfdc82fc4f389ccfed775aee6ab66646def`

Task checkpoint:
supplied by the final handoff for the commit containing this report.

Approval status:
not independently approved

Formal human approval:
deferred to terminal gate G65m

Next task:
M65c

## Scope

M65b owns the exact G65 architectural failure population for primary opcode
`62` BOUND.

- Count: `1,244`
- Hash-set SHA-256:
  `2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d`
- Current classification: `applicable`
- Target-policy transition: none
- Selected/applicable transition: none

The amended M65j backlog remains separate:

- Count: `5,908`
- Hash-set SHA-256:
  `240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5`
- Gap kind: `target_support_unverified`
- Disposition: `approved_nonblocking_defer`

## Evidence sources

The expected/actual reconstruction for M65b is:

- Case table:
  `tests/ssts/campaigns/g65m/reconstruction/m65b_bound_cases.json.gz`
- Case table SHA-256:
  `4425d22a0dde0e7e6f1a8f41d7c4e1b8e56e581b0aa8e29ce6f741d50213328a`
- Observable contract:
  `tests/ssts/campaigns/g65m/reconstruction/m65b_contract.json`
- Contract SHA-256:
  `1ee1cc1689c191270012f74ec285ac0d2b1d0023bf66881c99d37f200d4faf61`

The reconstructed expected states come exclusively from the approved SST
corpus. Actual G65 states came from the exact approved worker during the
identity-bound reconstruction checkpoint. The M65b post-fix replay used the
new M65b evaluated worker and the same verified corpus shard set.

## Contract audit

The 1,244 owned rows partition into two predecessor failure directions:

- `611` rows expected a type-5 event but the predecessor completed normally.
- `633` rows expected normal completion but the predecessor entered type 5.

The complete case table proves the observable contract for M65b:

- BOUND reads a signed 16-bit register operand selected by the ModR/M `reg`
  field.
- BOUND reads signed 16-bit lower and upper bounds from the two consecutive
  little-endian words at the effective memory address.
- The lower and upper boundaries are inclusive.
- A type-5 event is taken when the signed register value is below the lower
  bound or above the upper bound.
- Normal completion advances `IP` by the represented instruction length.
- On normal completion, BOUND preserves represented registers, FLAGS, and RAM
  other than the normal `IP` advancement.
- On type-5 event, BOUND uses the existing synchronous event-entry path; the
  frame machinery, saved `IP`, saved `CS`, saved `FLAGS`, vector fetch, and
  final handler target are not forked or duplicated by M65b.

The reconstruction includes segment overrides, displacements, offset wrapping,
physical wrapping, negative-only ranges, positive-only ranges, and ranges
crossing zero. Internal transient ordering remains underdetermined where it is
not represented by final architectural state.

## Root cause and correction

The active BOUND implementation compared the register and bounds as unsigned
16-bit values. This inverted the range decision whenever signed ordering and
unsigned ordering differed. The result produced both false normal completions
and false type-5 events.

M65b changes only the BOUND range-decision primitive: the selected register,
lower bound, and upper bound are converted to `SINT16`, then compared with an
inclusive signed range check. It continues to use the existing `CALC_EA`,
memory read, `IP`, and `INT_NUM(5, I286_IP)` event-entry paths.

## Validation

Commands executed:

```bash
cmake --preset linux-ci-gcc
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc -R vaeg_upd9002_m65b_bound --output-on-failure
ctest --test-dir build/linux-ci-gcc -R vaeg_upd9002_m65a_ff7 --output-on-failure
python3 tools/qa/upd9002_m65b_bound.py selftest
python3 tools/qa/upd9002_m65b_bound.py verify --shard-root /tmp/ssts-v20-v1_native --worker build/linux-ci-gcc/sdl2/vaeg --write-summary
python3 tools/qa/upd9002_m65_reconstruct.py verify
python3 tools/qa/upd9002_rep0f_transition.py --root . --selftest
python3 tools/qa/upd9002_rep0f_transition.py --root .
python3 tools/qa/upd9002_protected_deletion.py --root . --selftest
python3 tools/qa/upd9002_protected_deletion.py --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
ctest --test-dir build/linux-ci-gcc -LE external --output-on-failure
git diff --check
```

Results:

- Focused BOUND test: pass
- M65b selective replay: `1,244 pass / 0 fail`
- Timeout/crash: `0 / 0`
- Newly passing: `1,244`
- Newly failing: `0`
- M65a protection: `5,000 pass / 0 fail`
- M65d guard: `144` FF `/6` SP-alias failures preserved with official G65
  signatures
- BOUND frame protection: `3,565 pass / 0 fail`
- Reconstruction validator: pass, `7,511` cases reconciled
- Dispatch/protected-deletion validators: pass
- Milestone-ID validation: pass
- Encoding/EOL/path-case checks: pass
- Native non-external CTest: `66/66` pass
- Worker SHA-256:
  `1cf04ba49f76be3269a7545f15c4a7d07d92dbc12adf2d2f1acb7d85d0546012`
- Replay summary:
  `tests/ssts/campaigns/g65m/checkpoints/m65b_replay_summary.json`
- Replay summary SHA-256:
  `bc26d5ecfdc79155d0f37d45e886708c8a7356a126f2a8d65dd4b03b55f043eb`

Full architectural CI/full/fingerprint campaign profiles are deferred to the
terminal G65m closure unless a later campaign node requires them earlier.

## Protected scope

M65b did not change:

- target policy;
- classifications or taxonomy;
- selected or applicable sets;
- SST corpus data;
- comparison contracts;
- synchronous event-frame construction;
- saved FLAGS, saved IP, saved CS, vector fetch, or generic event entry;
- IRET;
- M65a FF `/7`;
- M65d FF `/6`;
- F7 `/2`;
- the exact ten-case tail;
- the amended M65j 5,908-hash target-support-unverified backlog.

## Known limitations

The reconstruction proves the observable architectural final state. It does
not claim an unobservable transient read/write order where multiple internal
orders produce the same final state.

M65b is complete within the serial M65 residue campaign.

This checkpoint is not independently approved.

Formal human approval is deferred to terminal gate G65m.

The next campaign task is M65c.

M66 and M67 remain untouched.
