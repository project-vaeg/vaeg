# Documented instruction coverage

Maintain independent axes:

- `source_class`: `DOCUMENTED_8087`, `RESERVED_8087`,
  `UNDOCUMENTED_UNKNOWN`, `LATER_X87_NOT_8087`
- `implementation_state`: `PENDING`, `IN_PROGRESS`, `COMPLETE_TESTED`
- `evidence_status`: `NORMATIVE_CONFIRMED`, `PROVISIONAL_DETERMINISTIC`,
  `HARDWARE_TRACE_PENDING`

Every `DOCUMENTED_8087` form must end as `COMPLETE_TESTED`.

Mandatory families include all documented forms of:

- FLD/FST/FSTP, FILD/FIST/FISTP, FBLD/FBSTP
- FADD/FMUL/FSUB/FSUBR/FDIV/FDIVR and integer/pop variants
- FCOM/FCOMP/FCOMPP, FICOM/FICOMP, FTST, FXAM
- FLD constants
- FXCH, FFREE, FINCSTP, FDECSTP, FABS, FCHS
- FSQRT, FRNDINT, FPREM, FSCALE, FXTRACT
- F2XM1, FYL2X, FYL2XP1, FPTAN, FPATAN
- FINIT/FNINIT, FCLEX/FNCLEX, FDISI/FNDISI, FENI/FNENI
- FLDCW, FSTCW/FNSTCW, FSTSW/FNSTSW to documented destinations
- FLDENV, FSTENV/FNSTENV, FSAVE/FNSAVE, FRSTOR
- FNOP and CPU WAIT/POLL sequencing

Do not add later-only FSIN, FCOS, FSINCOS, FPREM1, FUCOM, FCOMI, FCMOV,
FISTTP, or FNSTSW AX without original 8087 evidence.

Audit all 8 D8-DF primary bytes × 256 ModR/M values = 2048 decoder slots.
The production opcode description and the independent normative inventory must
not be generated from the same source.
