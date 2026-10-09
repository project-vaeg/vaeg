# M65k — target-wide reserved opcode policy checkpoint

M65k is complete within the serial M65 residue campaign.

This checkpoint is technically validated but not independently approved.

Formal human approval is deferred to terminal gate G65m.

## Campaign identity

- Campaign base: G65 at `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`
- Campaign branch: `topic/m65-residue-campaign`
- Campaign predecessor: M65i at `054d3de07331153729d2dcbf22e9016517171457`
- Canonical task: `docs/agents/tasks/M65k_upd9002_reserved_policy.md`
- Checkpoint: `tests/ssts/campaigns/g65m/checkpoints/m65k.json`

## Scope and result

M65k records the target-wide reserved/undefined opcode policy backlog. It
does not implement CPU semantics and does not change target policy,
classification, selected sets, applicable sets, fixtures, comparison
contracts, or official SST results.

The checkpoint protects the following decisions:

- `FF /7` remains the M65a implemented applicable-failure contract, not a
  target-absent reserved-opcode disposition.
- `6C-6F` remain documented-silicon-absent evidence backlog and are not
  returned to the blocking denominator.
- `66/67/FPO2` remain monitor-disassembler target-absent with complete silicon
  support underdetermined.
- `BRKEM` remains a zero-executable-corpus corpus/evidence backlog.
- `BRKFEM` remains an evidence backlog with no executable architectural
  contract.
- The amended M65j 19-group, 5,908-hash target-support-unverified backlog
  remains exact, non-applicable, unimplemented, and not claimed passing.

## Validation

- `git diff --check`: pass
- `python3 tools/qa/milestone_ids.py --selftest --audit --discover`: pass
- `python3 tools/qa/upd9002_m65_reconstruct.py verify --root .`: pass

## Next task

The next campaign task is M65l.

M66 and M67 remain untouched.
