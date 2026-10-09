# M65l — REPC/REPNC and prefix restart checkpoint

M65l is complete within the serial M65 residue campaign.

This checkpoint is technically validated but not independently approved.

Formal human approval is deferred to terminal gate G65m.

## Campaign identity

- Campaign base: G65 at `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`
- Campaign branch: `topic/m65-residue-campaign`
- Campaign predecessor: M65k at `84f7974095b3335879a01d23a800b0e25dc52447`
- Canonical task: `docs/agents/tasks/M65l_upd9002_prefix_restart.md`
- Checkpoint: `tests/ssts/campaigns/g65m/checkpoints/m65l.json`

## Scope and result

M65l records the REPC/REPNC and prefix/restart audit result for the current
campaign state. The G65 ownership and execution-spec materialization assign
no exact applicable-failure hashes and no exact implementation-missing
selectors to M65l.

This checkpoint therefore performs no CPU semantic implementation. It does
not change target policy, classifications, selected sets, applicable sets,
fixtures, comparison contracts, or official SST results.

Covered audit scope:

- REPC and REPNC.
- REPE and REPNE.
- Multiple prefixes.
- Segment overrides.
- LOCK.
- Restart after interrupt or exception.
- Trace and saved IP behavior.

No zero-coverage item is treated as passing, and the amended M65j backlog
remains separate from prefix/restart work.

## Validation

- `git diff --check`: pass
- `python3 tools/qa/milestone_ids.py --selftest --audit --discover`: pass
- `python3 tools/qa/upd9002_m65_reconstruct.py verify --root .`: pass

## Next task

The next campaign task is M65m terminal closure.

M66 and M67 remain untouched.
