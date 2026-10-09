# M65e — exact ten-case tail campaign checkpoint

M65e is complete within the serial M65 residue campaign.

This checkpoint is technically validated but not independently approved.

Formal human approval is deferred to terminal gate G65m.

The next campaign task is M65f.

M66 and M67 remain untouched.

## Fixed campaign context

- Campaign base: G65 at `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`
- Campaign branch: `topic/m65-residue-campaign`
- Campaign predecessor: M65d checkpoint `ef44acbf5183ac5a8233ac007b07de72fd61eae8`
- M65e initial semantic SHA: `83dc3da0355b2c56d8fb5b11afcecd565c82376a`
- M65e validator tooling SHA: `c4aed4a763c38146934c0f3d3d325cdf2fded8fb`
- M65e final evaluated SHA: `c7bb5ee274441d608096e4a33e2eca5a2d5af3a4`
- M65e checkpoint/evidence SHA: supplied by handoff after this evidence commit

## Ownership

- Owned selector set: exact ten-case tail
- Owned original G65 failures: 10
- Owned hash-set SHA-256: `7b228418bf0391884381514282e60ea9ccaf3af8c0f1f7f5a1b038a24de230a1`
- Case table: `tests/ssts/campaigns/g65m/reconstruction/m65e_tail_cases.json.gz`
- Case-table SHA-256: `862ad515b136850a3db146c9811d2d2cf661c814c195d7f9036506882037cf48`
- Observable contract: `tests/ssts/campaigns/g65m/reconstruction/m65e_contract.json`
- Contract SHA-256: `0f7a90566cec518f13c60ade987edefe88eff5a540e57d9ffca1b4884488b0e6`

The exact structural forms are:

- `61` POPA: 3 rows
- `81 /6` word XOR r/m16, imm16: 1 row
- `FF /5` far JMP m16:16: 1 row
- `A5` REP MOVSW: 1 row
- `9C` PUSHF: 1 row
- `D1 /6` word shift r/m16, 1: 1 row
- `C8` ENTER: 1 row
- `C4` LES r16, m16:16: 1 row

## Root cause and correction

All ten rows exercise word-sized reads or writes whose second byte crosses a
16-bit segment offset boundary. The inherited implementation used contiguous
physical word accesses in several paths, so the high byte was read from or
written to the linear address after `base + 0xffff` instead of the segment
base plus wrapped offset `0x0000`.

M65e adds segment-offset word helpers that read and write each byte through
the target segment-offset calculation. The fix is applied only to the proven
tail paths:

- `POPA` slow-path stack reads;
- V30 `PUSHF` stack write;
- memory word ALU and shift paths at offset wrap or inhibited word access;
- `MOVSW` source and destination word accesses;
- `LES`/`LDS` and far `CALL`/`JMP` pointer word reads;
- `ENTER` frame-copy word reads and writes.

The generic stack macros remain protected and unchanged from the approved
predecessor surface. The existing flags-materialization guard was updated only
for the exact M65e-owned `PUSHF` segment-wrap tail case; interrupt-frame,
trace, and protected-deletion checks remain green.

No target policy, classification, selected set, applicable set, corpus,
fixture, or comparison contract changed.

## Replay and protection

- M65e owned replay: 10 pass / 0 fail
- Original G65 architectural residue replay: 7,511 pass / 0 fail
- Timeout/crash: 0 / 0
- Newly passing: 10
- Newly failing: 0
- Worker SHA-256: `26892c7321f68804c19989867323eb7496a9f21a30f45af4e2df860e37c53340`

Protected populations:

- M65a `FF /7`: 5,000 pass / 0 fail
- M65b BOUND owned: 1,244 pass / 0 fail
- BOUND frame protection: 3,565 pass / 0 fail
- M65c complete selected `F7 /2`: 5,000 pass / 0 fail
- M65d complete selected `FF /6`: 5,000 pass / 0 fail

The exact M65j 5,908-hash target-support-unverified backlog is unchanged,
unimplemented, not applicable, not officially executed, and not claimed
passing.

## Validation

- `cmake --build --preset linux-ci-gcc`: pass
- `ctest --test-dir build/linux-ci-gcc -R 'vaeg_upd9002_m65(a_ff7|b_bound|c_f72|d_ff6|e_tail10)' --output-on-failure`: 5/5 pass
- `ctest --test-dir build/linux-ci-gcc -R 'vaeg_upd9002_(flags_materialization|protected_deletion|protected_deletion_selftest|trace_equivalence|m65e_tail10)' --output-on-failure`: 5/5 pass
- `python3 tools/qa/upd9002_m65e_tail10.py selftest`: pass
- `python3 tools/qa/upd9002_m65e_tail10.py verify --root . --shard-root /tmp/ssts-v20-v1_native --worker build/linux-ci-gcc/sdl2/vaeg --write-summary`: pass
- `python3 tools/qa/upd9002_m65_reconstruct.py verify --root .`: pass
- `python3 tools/qa/milestone_ids.py --selftest --audit --discover`: pass
- `ctest --test-dir build/linux-ci-gcc -LE external --output-on-failure`: 69/69 pass
- `git diff --check`: pass

## Campaign arithmetic

The cumulative original G65 applicable-failure improvements are now:

- M65a `FF /7`: 5,000
- M65b BOUND: 1,244
- M65c `F7 /2`: 1,113
- M65d `FF /6`: 144
- M65e exact tail: 10
- Total: 7,511

The exact original G65 architectural failure set now replays as
7,511 pass / 0 fail under the campaign worker. If no later closure-only or
conditional task changes the denominator, the terminal architectural-full
cross-check remains:

- applicable: 1,474,594
- pass: 1,474,594
- fail: 0

This arithmetic is a cross-check only; exact hash identities and transition
digests remain authoritative.

## Known limitations

M65e does not claim transient bus ordering for wrapped word reads or writes.
The reconstructed contract proves the final architectural state only.
