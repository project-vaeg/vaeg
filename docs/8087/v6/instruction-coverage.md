# Documented-instruction coverage and independent validation

> Applies only to Stage B, after the explicit user OCR handoff and acceptance in
> [goal-contract.md](goal-contract.md) section 0. Stage A must not execute this work.

Normative with [goal-contract.md](goal-contract.md). Intel I01 Table 5 supplies an initial
inventory; detailed 8087 and assembler sources resolve forms and aliases. Modern host x87
behavior is not the inventory authority.

## 1. Separate source membership, implementation, and evidence

Maintain three axes rather than a label that can hide unfinished work:

| Axis | Values and meaning |
|---|---|
| `source_class` | `DOCUMENTED_8087`, `RESERVED_8087`, `UNDOCUMENTED_UNKNOWN`, `LATER_X87_NOT_8087` |
| `implementation_state` | `PENDING`, `IN_PROGRESS`, `COMPLETE_TESTED`; non-document entries use `NOT_APPLICABLE` |
| `evidence_status` | `NORMATIVE_CONFIRMED`, `PROVISIONAL_DETERMINISTIC`, `HARDWARE_TRACE_PENDING` per behavior |

The final generated display may call a documented, complete, tested entry `IMPLEMENTED_8087`.
Every documented entry must reach that status. Changing a documented entry to reserved,
unknown, later-only, or not-applicable to make the gate pass is forbidden. Documented 8087
semantics take precedence when a byte pattern has a different later-generation meaning.
An opcode is not excluded merely because a later processor also uses those bytes.

Unknown/reserved policy must consume the native CPU's proper instruction length and required
bus effects, then apply a clearly non-authentic deterministic no-device-effect policy. Do not
inject a later #UD/#NM/#MF or expose later arithmetic. Preserve a trace hook for future
hardware evidence. FPO2 and 8080 instructions live in separate CPU-interface inventories.

## 2. Minimum documented family checklist

For all entries include documented memory widths, register choices, operand order, aliases,
reverse/pop forms, and actual bytes. These are semantic families, not a fixed mnemonic count.

| Family | Required members |
|---|---|
| Real transfers | FLD m32/m64/m80 and ST(i); FST m32/m64 and ST(i); FSTP m32/m64/m80 and ST(i) |
| Integer transfers | FILD m16/m32/m64; FIST m16/m32; FISTP m16/m32/m64 |
| Packed decimal | FBLD and FBSTP, ten-byte packed 18-digit/sign format |
| Add/multiply | FADD/FADDP/FIADD; FMUL/FMULP/FIMUL |
| Subtract | FSUB/FSUBR/FSUBP/FSUBRP/FISUB/FISUBR |
| Divide | FDIV/FDIVR/FDIVP/FDIVRP/FIDIV/FIDIVR |
| Compare | FCOM/FCOMP/FCOMPP; FICOM/FICOMP; FTST; FXAM |
| Constants | FLD1/FLDZ/FLDPI/FLDL2T/FLDL2E/FLDLG2/FLDLN2 |
| Stack/sign | FXCH, FFREE, FINCSTP, FDECSTP, FABS, FCHS |
| Other arithmetic | FSQRT, FRNDINT, FPREM, FSCALE, FXTRACT |
| Transcendentals | F2XM1, FYL2X, FYL2XP1, FPTAN, FPATAN |
| Control/status | FINIT/FNINIT; FCLEX/FNCLEX; FDISI/FNDISI; FENI/FNENI; FLDCW; FSTCW/FNSTCW; FSTSW/FNSTSW to memory |
| Environment | FLDENV; FSTENV/FNSTENV; FSAVE/FNSAVE; FRSTOR |
| Synchronization | FNOP; CPU 9Bh FWAIT/POLL and applicable WAIT-prefixed sequences |

WAIT/FN distinctions are actual instruction byte streams; a preceding 9Bh is a separate CPU
instruction, not another D8h-DFh arithmetic opcode. Assemble self-authored tests with a
suitable target mode and inspect emitted bytes. Use literal byte directives for missing
assembler support; do not omit a form because an assembler assumes newer x87.

Excluded later semantics include FSIN, FCOS, FSINCOS, FPREM1, FUCOM-family, FCOMI-family,
FCMOV, FISTTP, protected-mode control operations, and modern save layouts. The 8087 status
store baseline is to memory; do not add FNSTSW AX without original 8087 evidence. Do not
assert historical origin dates for undocumented aliases without evidence.

## 3. Two distinct data sources, not circular proof

**Production description:** a compact machine-readable opcode/form source used by, or
checked against, production decode. Generate its human-readable documentation from this
source. Fields include class, operand width/direction, stack effects, conditions, pointers,
exception family, latency key, handler ID, and test IDs. Select repository-appropriate
JSON/CSV/generated-C tooling at P00; do not add a large schema framework.

**Independent normative inventory and oracle fixtures:** manually or independently extracted
from primary-source facts, with exact source locations, without reading the DUT's generated
expectations. Freeze these before bulk handler implementation. They must include required
form membership, chosen critical byte encodings, operand directions, expected stack and
memory effects, and known exact results. They are not generated from the production table.

One production truth source prevents documentation drift; an independent audit source catches
shared errors. Test generation may enumerate inputs from production data, but expected
membership and semantic results cannot come solely from that same data. An agreement between
a decoder and a test that repeats its table is not evidence of correct 8087 decoding.

## 4. Exhaustive decoder slots and semantic coverage

There are 8 primary ESC bytes times 256 ModR/M values: **2048 decoder slots**, not 2048
distinct instructions and not every possible byte stream. Account for each slot exactly once.
Memory-address variants and displacement bytes are tested separately. Check register forms,
all native 16-bit addressing modes, BP/default segments, overrides, positive/negative
8-bit displacement, 16-bit displacement, wrap behavior, and code-boundary consumption.

For every documented form execute a positive semantic case through the production handler;
for each register encoding vary ST(i) to expose alias/TOP errors. Where applicable test
masked/unmasked exceptions, stack faults, NaNs/old encodings, pointers, memory widths, and
condition bits. Shared test families are allowed only when the executed-case report proves
the specific form and condition were exercised. A string naming a test is not execution.

Instrument handlers with a test-build-only stable ID or equivalent coverage seam. Match
observed production handler IDs and instruction bytes to the expected fixture. Do not use
source grep, function address presence, line coverage, or a successful build as a semantic
coverage substitute.

## 5. Final report and fail-closed gate

Generate Markdown and machine-readable reports containing at least:

```text
decoder_slots_total = 2048
decoder_slots_unclassified = 0
decoder_slots_duplicate = 0
documented_forms_total
documented_forms_complete_tested
documented_forms_missing_handler = 0
documented_forms_missing_executed_positive_test = 0
documented_forms_missing_applicable_special_tests = 0
documented_forms_reaching_stub_or_later_fallback = 0
normative_inventory_mismatches = 0
stale_or_skipped_required_test_results = 0
```

`documented_forms_complete_tested == documented_forms_total` is necessary, not sufficient:
the independent inventory must also match. Report documented forms and expanded decoder
slots with different counters; do not compare incompatible quantities.

The gate must reject deliberately corrupted copies: one documented form reclassified, one
handler removed, one positive test missing, one direction reversed, one result report stale,
and one required test skipped. These audit tests prove the checker is not a count-only stamp.
Keep production sources intact when testing corruption; use temporary fixtures.

During development `PENDING` is honest. At the final gate it is a failure. A legitimate FNOP,
FDISI already disabled, or FFREE of an already empty register can be a semantic no-op for
some inputs; do not ban such outcomes through a naive text scanner. What is forbidden is a
placeholder path substituting no-op behavior for a documented operation.
