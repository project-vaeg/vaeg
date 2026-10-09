# M65 campaign execution-spec materialization

Approved G65: `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`.
Campaign branch: `topic/m65-residue-campaign`.
Previous campaign head: `78df6d5a56ce646f8cb4f3aefd32b32b067c4377`.
Protocol SHA: `302540c5dff776890f205059235d3710d53a1636`.
Original M65j SHA: `1c1b9740cc7c286d841d296341c3cefd66e35116`.
Amended M65j checkpoint: `78df6d5a56ce646f8cb4f3aefd32b32b067c4377`.

Execution-spec materialization augments approved canonical task ownership
without changing production semantics, target policy, classifications,
selected sets, applicable sets, or SST results. No M65a-or-later semantic task
was started before all mandatory task specifications were audited.

## Readiness

M65a FF/7: `evidence_blocked`, 5,000 hashes.  
M65b BOUND: `evidence_blocked`, 1,244 hashes.  
M65c F7/2: `evidence_blocked`, 1,113 hashes.  
M65d FF/6: `evidence_blocked`, 144 hashes.  
M65e exact tail: `evidence_blocked`, 10 hashes.  
M65f, M65g, M65i, M65j, M65l: `conditional_nonblocking`.  
M65h: `evidence_blocked`.  
M65k and M65m: `closure_only`.

The five applicable-failure owners cover exactly 7,511 hashes with no overlap;
their individual digests are in the JSON specifications. The M65j backlog of
5,908 target-support-unverified hashes remains separate and is not included in
semantic ownership.

## Blockers

The committed G65 failure scoreboard contains normalized hash, mismatch, and
termination rows but no complete expected/actual architectural state tables.
Therefore M65a–M65e cannot yet establish the required implementation contract.
M65h also lacks its canonical evidence contract. No semantic implementation is
permitted until these specifications are materialized from approved raw
sidecars or an identity-bound selective replay.

The campaign stops before M65a. CPU, policy, selected/applicable sets, and SST
results are unchanged. Intermediate checkpoints remain unapproved; formal
approval is deferred to G65m. M66 and M67 remain untouched.
