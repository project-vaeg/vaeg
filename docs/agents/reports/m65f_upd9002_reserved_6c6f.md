# M65f — 6C–6F reserved behavior evidence checkpoint

M65f is complete within the serial M65 residue campaign.

This checkpoint is technically validated but not independently approved.

Formal human approval is deferred to terminal gate G65m.

The next campaign task is M65g.

M66 and M67 remain untouched.

## Campaign context

- Campaign base: G65 at `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`
- Campaign predecessor: M65e checkpoint `8350ca5d8345f3414e1864dcb6d70e391ea60cc1`
- M65f evidence/checkpoint SHA: supplied by handoff after this evidence commit

## Disposition

M65f owns no applicable failure hashes. It records the 6C–6F reserved-behavior
question as evidence backlog only.

Approved G60b evidence keeps primary opcodes `6C`, `6D`, `6E`, and `6F` as
`known_target_gap / documented_silicon_absent`. They remain outside the
blocking denominator. M65f does not implement V20/V30 string I/O semantics,
does not remove production handlers, and does not change reserved-opcode
behavior.

The required future sequence is:

1. acquire target-wide reserved-opcode behavior evidence;
2. decide debugger, trace, and disassembly presentation;
3. only then consider production cleanup or unreachable-handler removal.

## Evidence

- Backlog artifact: `tests/ssts/campaigns/g65m/evidence_backlog/reserved_6c6f.json`
- Backlog SHA-256: `1739f2ed71c25f84266c16dba3fabc4864b5d20eed770a5bae4075a569a9bc5b`
- Owned hash count: 0
- Owned hash-set SHA-256: `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`

## Protection

No CPU source, target policy, classification, selected set, applicable set,
SST fixture, or comparison contract changed. The M65j 5,908-hash
target-support-unverified backlog remains separate and unchanged.
