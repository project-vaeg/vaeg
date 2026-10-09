# Numerics and oracle policy

## SoftFloat

Use only the exact archive already present at:

`/Users/maho/work/vaeg-8087-evidence-v6/upstream/S03/SoftFloat-3e.zip`

Verify its recorded SHA-256 and license before vendoring. Vendor the minimum
source closure required by the implementation while preserving upstream files
unchanged; keep VAEG wrappers/build glue outside the upstream subtree.

SoftFloat provides basic software floating-point operations. It is not an 8087
device model and does not implement the five transcendental instructions.

Save/set/clear/call/capture/restore all mutable SoftFloat state around each use.
Do not leak rounding mode, tininess mode, extF80 precision, or exception flags.

## Independent oracles

Basic finite arithmetic may use independent exact-integer/rational checks,
TestFloat validation of the exact SoftFloat build, and external MPFR/GMP tooling.

MPFR/GMP/TestFloat are developer/test tools only. They must not be linked into the
shipping VAEG runtime.

For transcendental functions, generate independent high-precision interval
references outside the runtime. Use directed rounding and guest-format exponent/
subnormal modeling. Do not use host `double`, `long double`, libm, or host x87.

## Provisional numerical gate

For the frozen finite in-domain validation corpora, require at most four target
ulps unless P01/P13 establish a stronger source-backed contract. This is an
engineering acceptance bound, not a claim about silicon-identical low bits.

Architecture-visible domains, zero signs, stack effects, flags, condition codes,
exceptions, and special operands are exact contract checks and may not be hidden
by a numerical tolerance.
