# G65m serial residue campaign protocol

M65a through M65m execute serially on `topic/m65-residue-campaign` from
approved G65 `efd96b7e46717e7ee56e086f7d27ba42b04b49d3`. M65j is the first
internal checkpoint and decomposes the implementation-missing inventory.
Each subsequent node uses the immediately preceding checkpoint SHA.
Intermediate checkpoints are complete when technically validated but are not
independently approved. Formal approval is deferred to terminal G65m.

M66a requires approved G65m. No task may be skipped, and a blocked mandatory
task blocks G65m. Internal work packages use `M65j.NN` identifiers only and
are not formal milestones or gates. Task scopes and hash ownership from the
approved G65 plan remain unchanged.
