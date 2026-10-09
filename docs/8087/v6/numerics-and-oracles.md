# Numerics, 8087-specific behavior, and independent oracles

> Applies only to Stage B, after the explicit user OCR handoff and acceptance in
> [goal-contract.md](goal-contract.md) section 0. Stage A must not execute this work.

Normative with [goal-contract.md](goal-contract.md). Numerical test policy is not a promise
of silicon-identical microcode. Source IDs refer to [source-register.md](source-register.md).

## 1. Exact scope of SoftFloat

SoftFloat-3e supplies basic software floating-point operations, not an 8087 device and not
the five transcendental instructions. Audit the `8086` specialization rather than equating
its name with 8087 fidelity. Retain the raw-class barrier in `architecture.md`: do not send
non-canonical extF80 values to a backend whose behavior for them is not guaranteed.

Map PC's 24/53/64 significand bits to SoftFloat extF80 precision settings 32/64/80 only for
operations where that setting applies. Respect source-specific PC/RC effects on loads,
constants, stores, integer/BCD conversion, rounding, sqrt, remainder, scale, and extraction.
The reserved PC encoding needs a documented deterministic policy; do not invoke a backend
with an invalid precision. NaN selection, projective infinity, denormal-operand exceptions,
old encodings, tags, IEM/IC, and unmasked results remain VAEG responsibilities.

SoftFloat's IEEE remainder is not a drop-in FPREM: quotient rounding, partial reduction,
C2, quotient condition bits, and repeated execution differ. Likewise do not implement FSCALE
by unconstrained host ldexp or FXTRACT by a host logarithm. I01 Table 4 includes 8087-specific
FPREM quotient-bit retention cases; freeze them from the primary source rather than always
writing all three bits as on a generic later implementation.

`testsoftfloat` compares against its independent slow reference and validates the selected
SoftFloat build. Run it against the exact vendored objects/configuration. Ordinary generated
vectors whose expected values are also SoftFloat are useful for wiring, but do not prove
8087 wrapper semantics independently. Keep these claims distinct.

## 2. Operand-class and exception matrix

Before closing any arithmetic family, specify each applicable combination of normal,
zero signs, denormal, infinity signs, quiet/signaling NaN, unnormal, pseudo-NaN,
pseudo-infinity, pseudo-denormal, and other source-defined classes. Preserve raw payloads
when the instruction requires preservation; numeric normalization is instruction-specific.

Define priority when more than one exception is possible. Record defined, preserved, and
undefined condition bits separately. Determine result delivery, overflow/underflow response,
pop/push, store suppression, pointers, flags, and INT independently for each unmasked class.
Do not use a universal 'all unmasked exceptions suppress everything' rule. Test values at
integer/BCD range limits and one representable neighbor on either side under all RC modes.

## 3. Domain and operand-order reconciliation is an early deliverable

I01 Table 5 and its notes are a starting point, not a substitute for the detailed Intel
Numerics Supplement. Record a per-instruction domain, closed/open endpoints, source text
location, special operands, result order, and selected conflict resolution before tests
are frozen. Preserve 8087 partial-function restrictions; do not silently extend the range
using a 387 manual or modern mathematical API.

The old v4 list must not be copied blindly. In particular, I01's FSQRT note includes +infinity
and signed zero, and its FPATAN domain note uses a strict operand inequality. Resolve those
cases and the actual FPATAN ratio/result order against detailed original documentation.
A condensed table can contain ambiguity; log conflicts rather than choose the easier code.

For FPTAN, specify the two physical result values and logical stack order in addition to
the ratio relation. A ratio alone does not uniquely determine an output pair. If primary
documentation constrains only the ratio, choose a deterministic pair policy, label the raw
pair `PROVISIONAL_DETERMINISTIC`, and retain separate ratio and raw-pair tests. Never infer
that a component must equal 1 merely from later-x87 documentation. Normalized sine/cosine
is a candidate policy only if it satisfies every established 8087 output constraint.

## 4. Bounded implementation strategy

Prototype the following approaches before freezing production transcendental code:

- Represent intermediate quantities using SoftFloat binary128 (113 significand bits) and
  explicit exponent scaling, or a small fixed-limb 192/256-bit helper if measured error
  requires more guard bits. Binary128 alone has no wider exponent range than extF80; avoid
  overflowing or flushing intermediates when the final scaled result is representable.
- Use cancellation-safe expm1/log1p algorithms for F2XM1 and FYL2XP1. Do not subtract one
  from a rounded exponential or add a tiny x to one before taking a rounded logarithm.
- For logarithms, use exact power-of-two extraction and a bounded reduced-interval log
  series/polynomial with guard precision. For tangent/arctangent use the restricted interval
  and a bounded polynomial/rational or fixed-point CORDIC kernel. Separate approximation
  error, evaluation rounding, range reduction, and final output rounding.
- Keep constants as checked integer limbs/raw bits, generated from the external oracle with
  a recorded certificate/hash. No host decimal-float computation or hidden libm call.
- Bound every loop by a documented degree/iteration count and prove its range termination.
  Do not set a tiny arbitrary iteration limit that passes ordinary values but loses tiny
  inputs, near-unity logarithms, or boundary cancellation.

These are starting candidates, not a requirement to ship all approaches. P13 selects the
smallest approach that passes its independent prototype gate. Do not build a public
arbitrary-precision library or a generic CAS. Split the three log/exp operations and the two
trigonometric operations into separate implementation packages.

## 5. External oracle and certificate

MPFR/GMP are allowed only in separate test-tool targets/work areas; no runtime linkage.
Read inputs from exact integer/raw bit fields, never via double. Record generator source,
tool versions, commands, source references, precision strategy, seeds, and output hashes.
The runtime tests use committed redistributable fixtures without requiring MPFR installed.

Increasing precision until digits repeat is not a certificate. Compute directed lower and
upper bounds for the mathematical target, increasing working precision until the interval
is narrow enough for the required target rounding or tolerance decision. `mpfr_can_round`
is useful only with a justified error bound. Model extF80 exponent limits and subnormal
quantization explicitly; MPFR's default exponent range is not the guest format.

For exact basic operations use independent integer/rational or MPFR-directed references and
source-authored special-class expected results. For transcendental tolerance, store a
certified interval or equivalent exact pass thresholds, not merely a DUT-generated golden
value. Near a rounding boundary, continue interval refinement or classify an exact case;
do not select the endpoint that makes the DUT pass. Production outputs may supply a
regression trace only after independently validating them, never as their own oracle.

## 6. Frozen provisional numerical acceptance

For finite in-domain scalar outputs on the committed validation corpora require error at
most 4 target ulps, plus the source-defined architectural results. This is a VAEG corpus
acceptance threshold, not an all-input proof and not an 8087 silicon specification. Default
target precision is p=64 unless the selected original source establishes different PC
behavior; that choice is a named policy, not silently inherited backend behavior.

For a nonzero normal exact target y and selected p, define one ulp as
`2^(floor(log2(abs(y))) - (p-1))`. At zero/subnormal magnitudes use the target format's
minimum subnormal spacing (for the full extended format, `2^-16445`). Specify reduced-PC
exponent/subnormal treatment from the selected contract rather than blindly recycling this
full-precision definition. Exact endpoint identities and zero signs have exact separate
checks. Overflow/underflow, special operands, and flags use their instruction policies,
not a numeric tolerance to hide the wrong exception.

For FPTAN check documented pair ordering, finite/zero behavior of each component, and a
certified ratio/reconstruction interval at equivalent 4-ulp precision. Also compare exact
raw outputs to the independently approved pair policy. Near zero use the absolute
quantization bound; never divide by a zero component or let arbitrary common scaling hide
an overflowing pair. An architecture result is not excused by a good ratio alone.

Set initial corpus floors per instruction: at least 512 reproducible ordinary in-domain
inputs plus all targeted adversarial/boundary cases in normal CI, and at least 8192
stratified inputs in the extended P13/P14/P15 gate. Stratify exponents, signs where legal,
near-one log inputs, cancellation, tiny values, and domain endpoints; uniform real-number
sampling alone is inadequate. Execute applicable PC/RC combinations. A primary-source
reason can exclude an inapplicable combination, but not shrink counts after a failure.
These floors are engineering budgets, not statistical evidence of universal correctness.

Record worst observed error, input, output, oracle interval, precision, and corpus hash.
Do not weaken 4 ulps or reclassify finite in-domain failures as undocumented to pass. First
increase guard precision or change the numerical kernel, then rerun frozen and new cases.

## 7. Completion distinction

`COMPLETE_TESTED` for a transcendental means a real deterministic handler, complete known
architectural behavior, and passed frozen mathematical/policy tests. It does not mean the
least significant bits reproduce an identified 8087 revision. Silicon traces are a separate
future evidence axis and must never be fabricated or replaced by host x87 measurements.

## v6 reference extension

Follow [reference-validation.md](reference-validation.md). Full Intel E8087 is optional
external corroboration, not an independent mathematical error certificate or proof of
silicon identity. Distinguish full E8087 from PE8087 and callable support-library functions.
Do not make any optional binary, microcode study or third-party emulator a mandatory gate.
All oracle builds and prototypes in this document are Stage B work only.
