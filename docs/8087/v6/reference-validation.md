# Optional external reference validation

Normative in Stage B only. This complements, and does not replace, the mandatory independent
oracles and semantic tests in numerics-and-oracles.md and verification.md.

## E8087: full emulator versus related libraries

Intel I03 chapter 2 documents a full software emulator and interface libraries. It is a
valuable independent implementation candidate, but a vendor's equivalence statement is
not evidence that a particular recovered binary matches every 8087 silicon revision.
Identify full E8087 versus PE8087 and high-level libraries; do not compare a library SIN
routine as though FSIN existed on the 8087. Confirm release, object format and entry path.

A raw hardware-oriented .COM program does not automatically execute through E8087. Build
a separate, self-authored harness using the documented assembly/link/interface convention,
when the required tools and binary are supplied. Validate that emulation actually runs
with the physical/emulated FPU disabled, and record the resulting generated instruction
bytes. Freeze one semantic case format shared by the hardware and library harnesses,
not an unsupported claim that the exact executable is portable between both paths.

Normalize only documented harness differences. Record input/output raw bits, memory,
CW/SW/tags, TOP, operation form and exception classification. Pointer fields, trap entry
and OS/compiler glue require separate comparison rules, not wholesale removal from tests.
E8087 is not a BUSY, bus-cycle, physical INT-wiring or clock-latency oracle. Its documented
software trap route is not evidence for the VA interrupt route.

When unavailable use `NOT_AVAILABLE`/`NOT_RUN`; do not create an E8087-shaped stub. Its
absence does not block P00-P19 or license a less accurate runtime. Its presence does not
replace directed MPFR/integer certificates or the independent source inventory.

## Other emulator executables

Optional comparisons use external separately approved binaries and self-authored guest
cases only. Record machine/FPU model, options, version/hash, executable path externally,
input corpus hash and exact differences. Do not read their source to resolve a result.
Multiple implementations can share ancestry or errors; agreement is corroboration, not
proof. Host x87 is not an original-8087 silicon oracle.

## Silicon-evidence ledger

After authorization, create records/silicon-evidence.md only for actual approved observations.
For each entry record: source ID; actual chip/revision/machine if known; exact observation
or behavior report; input CW/raw operands; output state and exception; trace/record hash;
source page or observation method; uncertainty; VAEG test IDs; resolution status.

Evidence-only articles and extracted microcode are not automatically consumed. If a
separate evidence reviewer supplies a behavior-only report, record what was independently
confirmed and what is only a microcode interpretation. Do not copy microcode bytes or
translated control flow into production, and do not call such a report silicon testing.

## Machine-readable evidence axes at P19

Report separately:

```text
mandatory_local_semantics: VERIFIED | FAILED | NOT_RUN
e8087_comparison: VERIFIED | DISCREPANCY | NOT_AVAILABLE | NOT_RUN
other_emulator_comparison: VERIFIED | DISCREPANCY | NOT_AVAILABLE | NOT_RUN
real_va_hardware: VERIFIED | DISCREPANCY | NOT_RUN
silicon_behavior_study: REVIEWED_WITH_LIMITS | NOT_REVIEWED
silicon_numerics: VERIFIED | PARTIAL | PENDING_EVIDENCE
```

`VERIFIED` always refers to the recorded corpus and field mask, never every possible input.
Optional evidence does not enter the final mandatory-test denominator. All documented
8087 forms remain mandatory regardless of these fields; software completion still requires
the production VA route and every local gate in the implementation contract.
