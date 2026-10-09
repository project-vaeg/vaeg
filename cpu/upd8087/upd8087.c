/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "upd8087.h"

#include <limits.h>
#include <string.h>

#include "platform.h"
#include "softfloat.h"

enum {
	RAW_EXP_MASK = 0x7fff,
	RAW_SIGN_MASK = 0x8000,
	RAW_INTEGER_BIT = UINT64_C(0x8000000000000000),
	RAW_FRACTION_MASK = UINT64_C(0x7fffffffffffffff),
	STATUS_EXCEPTION_MASK = 0x003f,
	CONTROL_MASKS = 0x003f,
	CONTROL_IEM = 0x0080,
	CONTROL_PC = 0x0300,
	CONTROL_RC = 0x0c00,
	CONTROL_IC = 0x1000,
	STATUS_C3 = 0x4000,
	STATE_FLAG_ENABLED = 0x0001,
	STATE_FLAG_BUSY = 0x0002,
	STATE_FLAG_PENDING = 0x0004,
	STATE_MAGIC0 = 0x55,
	STATE_MAGIC1 = 0x38,
	STATE_MAGIC2 = 0x37,
	STATE_MAGIC3 = 0x53,
};

static const UPD8087_RAW80 raw_zero = {UINT64_C(0), 0};
static const UPD8087_RAW80 raw_one = {RAW_INTEGER_BIT, 0x3fff};
static const UPD8087_RAW80 raw_half = {RAW_INTEGER_BIT, 0x3ffe};
static const UPD8087_RAW80 raw_two = {RAW_INTEGER_BIT, 0x4000};
static const UPD8087_RAW80 raw_pi = {UINT64_C(0xc90fdaa22168c235), 0x4000};
static const UPD8087_RAW80 raw_pi_over_four = {UINT64_C(0xc90fdaa22168c235), 0x3ffe};
static const UPD8087_RAW80 raw_l2t = {UINT64_C(0xd49a784bcd1b8afe), 0x4000};
static const UPD8087_RAW80 raw_l2e = {UINT64_C(0xb8aa3b295c17f0bc), 0x3fff};
static const UPD8087_RAW80 raw_lg2 = {UINT64_C(0x9a209a84fbcff799), 0x3ffd};
static const UPD8087_RAW80 raw_ln2 = {UINT64_C(0xb17217f7d1cf79ac), 0x3ffe};
static const UPD8087_RAW80 raw_fyl2xp1_limit = {UINT64_C(0x95f619980c433000), 0x3ffd};
static const UPD8087_RAW80 raw_indefinite = {
	UINT64_C(0xc000000000000000), (uint16_t)(RAW_SIGN_MASK | RAW_EXP_MASK)
};
static const UPD8087_RAW80 raw_qnan = {UINT64_C(0xffffc00000000000), 0x7fff};

typedef struct {
	uint_fast8_t rounding_mode;
	uint_fast8_t detect_tininess;
	uint_fast8_t exception_flags;
	uint_fast8_t rounding_precision;
} SOFTFLOAT_CONTEXT;

static void configure_softfloat(const UPD8087_STATE *state);
static bool raw_negative(UPD8087_RAW80 value);

static void softfloat_context_save(SOFTFLOAT_CONTEXT *context) {
	context->rounding_mode = softfloat_roundingMode;
	context->detect_tininess = softfloat_detectTininess;
	context->exception_flags = softfloat_exceptionFlags;
	context->rounding_precision = extF80_roundingPrecision;
}

static void softfloat_context_restore(const SOFTFLOAT_CONTEXT *context) {
	softfloat_roundingMode = context->rounding_mode;
	softfloat_detectTininess = context->detect_tininess;
	softfloat_exceptionFlags = context->exception_flags;
	extF80_roundingPrecision = context->rounding_precision;
}

static uint16_t load16(const uint8_t *p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t load32(const uint8_t *p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

static uint64_t load64(const uint8_t *p) {
	uint64_t value = 0;
	unsigned i;

	for (i = 0; i < 8; i++) {
		value |= (uint64_t)p[i] << (i * 8);
	}
	return value;
}

static void store16(uint8_t *p, uint16_t value) {
	p[0] = (uint8_t)value;
	p[1] = (uint8_t)(value >> 8);
}

static void store32(uint8_t *p, uint32_t value) {
	p[0] = (uint8_t)value;
	p[1] = (uint8_t)(value >> 8);
	p[2] = (uint8_t)(value >> 16);
	p[3] = (uint8_t)(value >> 24);
}

static void store64(uint8_t *p, uint64_t value) {
	unsigned i;

	for (i = 0; i < 8; i++) {
		p[i] = (uint8_t)(value >> (i * 8));
	}
}

static bool raw_is_zero(UPD8087_RAW80 value) {
	return ((value.signif == 0) && ((value.sign_exp & RAW_EXP_MASK) == 0));
}

static bool raw_is_denormal(UPD8087_RAW80 value) {
	return (((value.sign_exp & RAW_EXP_MASK) == 0) && (value.signif != 0));
}

static bool raw_is_infinity(UPD8087_RAW80 value) {
	return (((value.sign_exp & RAW_EXP_MASK) == RAW_EXP_MASK) &&
	        (value.signif == RAW_INTEGER_BIT));
}

static bool raw_is_nan(UPD8087_RAW80 value) {
	return (((value.sign_exp & RAW_EXP_MASK) == RAW_EXP_MASK) &&
	        (value.signif != RAW_INTEGER_BIT));
}

static bool raw_is_unnormal(UPD8087_RAW80 value) {
	uint16_t exponent = value.sign_exp & RAW_EXP_MASK;

	return (exponent != 0) && (exponent != RAW_EXP_MASK) &&
	       ((value.signif & RAW_INTEGER_BIT) == 0);
}

static bool raw_is_normal(UPD8087_RAW80 value) {
	uint16_t exponent = value.sign_exp & RAW_EXP_MASK;

	return (exponent != 0) && (exponent != RAW_EXP_MASK) &&
	       ((value.signif & RAW_INTEGER_BIT) != 0);
}

static bool raw_is_unnormal_operand(UPD8087_RAW80 value) {
	return raw_is_denormal(value) || raw_is_unnormal(value);
}

static uint8_t raw_tag(UPD8087_RAW80 value) {
	if (raw_is_zero(value)) {
		return UPD8087_TAG_ZERO;
	}
	if (raw_is_unnormal(value)) {
		return UPD8087_TAG_VALID;
	}
	if ((value.sign_exp & RAW_EXP_MASK) == 0 || raw_is_nan(value) ||
	    raw_is_infinity(value)) {
		return UPD8087_TAG_SPECIAL;
	}
	return UPD8087_TAG_VALID;
}

static UPD8087_RAW80 raw_neg(UPD8087_RAW80 value) {
	value.sign_exp ^= RAW_SIGN_MASK;
	return value;
}

static UPD8087_RAW80 raw_abs(UPD8087_RAW80 value) {
	value.sign_exp &= (uint16_t)~RAW_SIGN_MASK;
	return value;
}

static UPD8087_RAW80 raw_with_sign(UPD8087_RAW80 value, bool negative) {
	value.sign_exp = (uint16_t)((value.sign_exp & (uint16_t)~RAW_SIGN_MASK) |
	                           (negative ? RAW_SIGN_MASK : 0));
	return value;
}

static UPD8087_RAW80 raw_larger_nan(UPD8087_RAW80 lhs, UPD8087_RAW80 rhs) {
	return (lhs.signif >= rhs.signif) ? lhs : rhs;
}

typedef struct {
	uint64_t significand;
	int32_t exponent;
	bool zero;
} RAW_MAGNITUDE;

static RAW_MAGNITUDE raw_magnitude(UPD8087_RAW80 value) {
	RAW_MAGNITUDE result;

	result.significand = value.signif;
	result.exponent = (int32_t)(value.sign_exp & RAW_EXP_MASK);
	result.zero = value.signif == 0;
	if (result.zero) {
		return result;
	}
	if (result.exponent == 0) {
		/* Temporary-real denormals have the same scale as exponent one
		 * before their missing leading bit is normalized. */
		result.exponent = 1;
	}
	while ((result.significand & RAW_INTEGER_BIT) == 0) {
		result.significand <<= 1;
		--result.exponent;
	}
	return result;
}

static UPD8087_RAW80 raw_backend_value(UPD8087_RAW80 value) {
	RAW_MAGNITUDE magnitude;
	unsigned shift;

	if (raw_is_nan(value) || raw_is_infinity(value)) {
		return value;
	}
	magnitude = raw_magnitude(value);
	if (magnitude.zero) {
		return raw_with_sign(raw_zero, raw_negative(value));
	}
	if (magnitude.exponent >= 1 && magnitude.exponent < RAW_EXP_MASK) {
		return (UPD8087_RAW80){magnitude.significand,
			(uint16_t)((value.sign_exp & RAW_SIGN_MASK) |
			           (uint16_t)magnitude.exponent)};
	}
	if (magnitude.exponent >= 1) {
		return value;
	}
	shift = (unsigned)(1 - magnitude.exponent);
	if (shift >= 64) {
		return raw_with_sign(raw_zero, raw_negative(value));
	}
	return (UPD8087_RAW80){magnitude.significand >> shift,
		(uint16_t)(value.sign_exp & RAW_SIGN_MASK)};
}

static UPD8087_RAW80 raw_for_compare(UPD8087_RAW80 value) {
	uint16_t exponent;

	if (value.signif == 0 && (value.sign_exp & RAW_EXP_MASK) != 0) {
		return raw_with_sign(raw_zero, raw_negative(value));
	}
	if (raw_is_denormal(value)) {
		/* A denormal fetched from temporary-real memory is first viewed as
		 * its exact equivalent unnormal in the work area. */
		value.sign_exp = (uint16_t)((value.sign_exp & RAW_SIGN_MASK) | 1);
	}
	if (!raw_is_unnormal(value)) {
		return value;
	}
	exponent = value.sign_exp & RAW_EXP_MASK;
	while ((value.signif & RAW_INTEGER_BIT) == 0) {
		value.signif <<= 1;
		if (exponent > 1) {
			--exponent;
		} else {
			exponent = 0;
			break;
		}
	}
	value.sign_exp = (uint16_t)((value.sign_exp & RAW_SIGN_MASK) | exponent);
	return value;
}

/* In projective closure both encodings of infinity denote the same value.
 * Keep the stored representation stable, but present the canonical positive
 * encoding to arithmetic operations.  Instructions with documented signed
 * infinity behavior (for example FPATAN) deliberately bypass this helper. */
static UPD8087_RAW80 raw_for_arithmetic(const UPD8087_STATE *state,
	UPD8087_RAW80 value) {
	if (!(state->control & CONTROL_IC) && raw_is_infinity(value)) {
		value.sign_exp &= (uint16_t)~RAW_SIGN_MASK;
	}
	return raw_backend_value(value);
}

static int raw_abs_magnitude_compare(UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs) {
	RAW_MAGNITUDE left = raw_magnitude(raw_abs(lhs));
	RAW_MAGNITUDE right = raw_magnitude(raw_abs(rhs));

	if (left.zero || right.zero) {
		if (left.zero && right.zero) {
			return 0;
		}
		return left.zero ? -1 : 1;
	}
	if (left.exponent != right.exponent) {
		return left.exponent < right.exponent ? -1 : 1;
	}
	if (left.significand == right.significand) {
		return 0;
	}
	return left.significand < right.significand ? -1 : 1;
}

static UPD8087_RAW80 raw_result_as_unnormal(UPD8087_RAW80 value) {
	uint16_t exponent;

	if (!raw_is_normal(value)) {
		return value;
	}
	exponent = value.sign_exp & RAW_EXP_MASK;
	if (exponent >= RAW_EXP_MASK - 1) {
		/* There is no finite temporary-real encoding above exponent
		 * 0x7FFE; retain the SoftFloat overflow result. */
		return value;
	}
	value.signif >>= 1;
	value.sign_exp = (uint16_t)((value.sign_exp & RAW_SIGN_MASK) |
	                           (uint16_t)(exponent + 1));
	return value;
}

static UPD8087_RAW80 raw_binary_result_class(UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs, unsigned operation, UPD8087_RAW80 result) {
	bool unnormal_lhs = raw_is_unnormal_operand(lhs);
	bool unnormal_rhs = raw_is_unnormal_operand(rhs);
	bool preserve_unnormal = false;

	if (operation == 2) {
		preserve_unnormal = unnormal_lhs || unnormal_rhs;
	} else if (operation == 3) {
		preserve_unnormal = unnormal_lhs;
	} else if (operation == 0 || operation == 1) {
		int dominant = raw_abs_magnitude_compare(lhs, rhs);

		preserve_unnormal = (dominant > 0 && unnormal_lhs) ||
		                    (dominant < 0 && unnormal_rhs);
	}
	if (preserve_unnormal) {
		result = raw_result_as_unnormal(result);
	}
	return result;
}

static extFloat80_t sf_from_raw(UPD8087_RAW80 value) {
	extFloat80_t result;

	result.signif = value.signif;
	result.signExp = value.sign_exp;
	return result;
}

static UPD8087_RAW80 raw_from_sf(extFloat80_t value) {
	UPD8087_RAW80 result;

	result.signif = value.signif;
	result.sign_exp = value.signExp;
	return result;
}

static UPD8087_RAW80 raw_from_sf_for_arithmetic(const UPD8087_STATE *state,
	extFloat80_t value) {
	return raw_for_arithmetic(state, raw_from_sf(value));
}

static UPD8087_RAW80 raw_from_i32(int32_t value) {
	extFloat80_t converted;
	SOFTFLOAT_CONTEXT context;

	softfloat_context_save(&context);
	i32_to_extF80M(value, &converted);
	softfloat_context_restore(&context);
	return raw_from_sf(converted);
}

static UPD8087_RAW80 raw_from_i64(int64_t value) {
	extFloat80_t converted;
	SOFTFLOAT_CONTEXT context;

	softfloat_context_save(&context);
	i64_to_extF80M(value, &converted);
	softfloat_context_restore(&context);
	return raw_from_sf(converted);
}

static UPD8087_RAW80 raw_from_u64(uint64_t value) {
	extFloat80_t converted;
	SOFTFLOAT_CONTEXT context;

	softfloat_context_save(&context);
	ui64_to_extF80M(value, &converted);
	softfloat_context_restore(&context);
	return raw_from_sf(converted);
}

static uint8_t softfloat_to_status(uint_fast8_t flags) {
	uint8_t result = 0;

	if (flags & softfloat_flag_invalid) {
		result |= UPD8087_STATUS_IE;
	}
	if (flags & softfloat_flag_infinite) {
		result |= UPD8087_STATUS_ZE;
	}
	if (flags & softfloat_flag_overflow) {
		result |= UPD8087_STATUS_OE;
	}
	if (flags & softfloat_flag_underflow) {
		result |= UPD8087_STATUS_UE;
	}
	if (flags & softfloat_flag_inexact) {
		result |= UPD8087_STATUS_PE;
	}
	return result;
}

static void configure_softfloat(const UPD8087_STATE *state) {
	uint16_t precision = state->control & CONTROL_PC;
	uint16_t rounding = (state->control & CONTROL_RC) >> 10;

	switch (rounding) {
	case 1:
		softfloat_roundingMode = softfloat_round_min;
		break;
	case 2:
		softfloat_roundingMode = softfloat_round_max;
		break;
	case 3:
		softfloat_roundingMode = softfloat_round_minMag;
		break;
	default:
		softfloat_roundingMode = softfloat_round_near_even;
		break;
	}
	switch (precision) {
	case 0x0000:
		extF80_roundingPrecision = 32;
		break;
	case 0x0200:
		extF80_roundingPrecision = 53;
		break;
	default:
		extF80_roundingPrecision = 64;
		break;
	}
}

static void raise_status(UPD8087_STATE *state, uint8_t flags) {
	state->working_flags |= flags;
	state->status |= flags;
}

static void softfloat_context_begin(const UPD8087_STATE *state,
	SOFTFLOAT_CONTEXT *context) {
	softfloat_context_save(context);
	configure_softfloat(state);
	softfloat_exceptionFlags = 0;
}

static void softfloat_context_finish(UPD8087_STATE *state,
	const SOFTFLOAT_CONTEXT *context) {
	uint_fast8_t flags = softfloat_exceptionFlags;

	softfloat_context_restore(context);
	raise_status(state, softfloat_to_status(flags));
}

static extFloat80_t sf_binary(UPD8087_STATE *state, UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs, unsigned operation) {
	extFloat80_t left = sf_from_raw(raw_for_arithmetic(state, lhs));
	extFloat80_t right = sf_from_raw(raw_for_arithmetic(state, rhs));
	extFloat80_t result;
	SOFTFLOAT_CONTEXT context;

	softfloat_context_begin(state, &context);
	switch (operation) {
	case 0:
		result = extF80_add(left, right);
		break;
	case 1:
		result = extF80_sub(left, right);
		break;
	case 2:
		result = extF80_mul(left, right);
		break;
	default:
		result = extF80_div(left, right);
		break;
	}
	softfloat_context_finish(state, &context);
	return result;
}

static UPD8087_RAW80 sf_unary(UPD8087_STATE *state, UPD8087_RAW80 value,
	unsigned operation) {
	extFloat80_t input = sf_from_raw(raw_for_arithmetic(state, value));
	extFloat80_t result;
	SOFTFLOAT_CONTEXT context;

	softfloat_context_begin(state, &context);
	if (operation == 0) {
		result = extF80_sqrt(input);
	} else {
		result = extF80_roundToInt(input, softfloat_roundingMode, false);
	}
	softfloat_context_finish(state, &context);
	return raw_from_sf_for_arithmetic(state, result);
}

static UPD8087_RAW80 sf_mul(UPD8087_STATE *state, UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs) {
	return raw_from_sf_for_arithmetic(state, sf_binary(state, lhs, rhs, 2));
}

static UPD8087_RAW80 sf_div(UPD8087_STATE *state, UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs) {
	return raw_from_sf_for_arithmetic(state, sf_binary(state, lhs, rhs, 3));
}

static UPD8087_RAW80 sf_add(UPD8087_STATE *state, UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs) {
	return raw_from_sf_for_arithmetic(state, sf_binary(state, lhs, rhs, 0));
}

static UPD8087_RAW80 sf_add_silent(const UPD8087_STATE *state,
	UPD8087_RAW80 lhs, UPD8087_RAW80 rhs) {
	extFloat80_t result;
	SOFTFLOAT_CONTEXT context;

	softfloat_context_begin(state, &context);
	result = extF80_add(sf_from_raw(raw_for_arithmetic(state, lhs)),
	                    sf_from_raw(raw_for_arithmetic(state, rhs)));
	softfloat_context_restore(&context);
	return raw_from_sf_for_arithmetic(state, result);
}

static UPD8087_RAW80 sf_sub(UPD8087_STATE *state, UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs) {
	return raw_from_sf_for_arithmetic(state, sf_binary(state, lhs, rhs, 1));
}

static UPD8087_RAW80 sf_round(UPD8087_STATE *state, UPD8087_RAW80 value) {
	if (raw_is_infinity(value)) {
		return value;
	}
	return sf_unary(state, value, 1);
}

static bool state_can_commit_ignoring(const UPD8087_STATE *state,
	uint8_t ignored_flags) {
	uint8_t working_flags = state->working_flags & (uint8_t)~ignored_flags;

	/* The original 8087 distinguishes exceptions detected before an
	 * operation (invalid, denormalized operand, zero divide, and stack
	 * faults) from exceptions detected after a result exists (overflow,
	 * underflow, and precision).  The latter do not cause a blanket rollback:
	 * register results are committed and an unmasked exception requests an
	 * interrupt.  Memory stores apply the additional destination rule in
	 * store_can_commit(). */
	if ((working_flags & (UPD8087_STATUS_IE | UPD8087_STATUS_SF)) &&
	    !(state->control & UPD8087_STATUS_IE)) {
		return false;
	}
	if ((working_flags & UPD8087_STATUS_DE) &&
	    !(state->control & UPD8087_STATUS_DE)) {
		return false;
	}
	if ((working_flags & UPD8087_STATUS_ZE) &&
	    !(state->control & UPD8087_STATUS_ZE)) {
		return false;
	}
	return true;
}

static bool state_can_commit(const UPD8087_STATE *state) {
	return state_can_commit_ignoring(state, 0);
}

static bool store_can_commit(const UPD8087_STATE *state) {
	if (!state_can_commit(state)) {
		return false;
	}
	return (state->working_flags & (UPD8087_STATUS_OE | UPD8087_STATUS_UE) &
	        (uint8_t)~state->control) == 0;
}

static void sync_top(UPD8087_STATE *state) {
	state->status = (uint16_t)((state->status & (uint16_t)~UPD8087_STATUS_TOP) |
	                           ((uint16_t)(state->top & 7) << 11));
}

static unsigned physical_index(const UPD8087_STATE *state, unsigned logical) {
	return (state->top + logical) & 7;
}

static UPD8087_RAW80 stack_get(const UPD8087_STATE *state, unsigned logical) {
	return state->reg[physical_index(state, logical)];
}

static uint8_t stack_tag(const UPD8087_STATE *state, unsigned logical) {
	return state->tag[physical_index(state, logical)];
}

static void stack_set(UPD8087_STATE *state, unsigned logical, UPD8087_RAW80 value) {
	unsigned index = physical_index(state, logical);

	state->reg[index] = value;
	state->tag[index] = raw_tag(value);
}

static bool stack_valid(const UPD8087_STATE *state, unsigned logical) {
	return stack_tag(state, logical) != UPD8087_TAG_EMPTY;
}

static bool stack_require(UPD8087_STATE *state, unsigned logical) {
	unsigned index;

	if ((logical >= 8) || !stack_valid(state, logical)) {
		raise_status(state, UPD8087_STATUS_IE | UPD8087_STATUS_SF);
		state->status |= UPD8087_STATUS_C1;
		if (logical < 8 && state_can_commit(state)) {
			index = physical_index(state, logical);
			state->reg[index] = raw_indefinite;
			state->tag[index] = UPD8087_TAG_SPECIAL;
		}
		return false;
	}
	return true;
}

static bool stack_push_policy(UPD8087_STATE *state, UPD8087_RAW80 value,
	uint8_t ignored_flags) {
	unsigned new_top = (state->top + 7) & 7;
	bool can_commit;

	if (state->tag[new_top] != UPD8087_TAG_EMPTY) {
		raise_status(state, UPD8087_STATUS_IE | UPD8087_STATUS_SF);
		state->status |= UPD8087_STATUS_C1;
		can_commit = state_can_commit_ignoring(state, ignored_flags);
		if (!can_commit) {
			return false;
		}
		/* A masked stack overflow still consumes the destination stack
		 * position, replacing its old contents with real indefinite. */
		state->top = (uint8_t)new_top;
		state->reg[new_top] = raw_indefinite;
		state->tag[new_top] = UPD8087_TAG_SPECIAL;
		sync_top(state);
		return true;
	}
	state->top = (uint8_t)new_top;
	state->reg[new_top] = value;
	state->tag[new_top] = raw_tag(value);
	sync_top(state);
	return true;
}

static bool stack_push(UPD8087_STATE *state, UPD8087_RAW80 value) {
	return stack_push_policy(state, value, 0);
}

static void stack_pop(UPD8087_STATE *state) {
	unsigned index = state->top;

	state->tag[index] = UPD8087_TAG_EMPTY;
	state->top = (uint8_t)((state->top + 1) & 7);
	sync_top(state);
}

static uint8_t bus_read8(const UPD8087_OPERAND *operand, uint32_t offset,
	bool *ok) {
	if (operand->first_word_valid && offset < 2) {
		return (uint8_t)(operand->first_word >> (offset * 8));
	}
	if ((operand->bus.read8 == NULL) || (ok == NULL)) {
		if (ok != NULL) {
			*ok = false;
		}
		return 0;
	}
	return operand->bus.read8(operand->bus.opaque, operand->address + offset);
}

static bool bus_read(const UPD8087_OPERAND *operand, uint32_t offset,
	uint8_t *data, unsigned size) {
	unsigned i;
	bool ok = true;

	for (i = 0; i < size; i++) {
		data[i] = bus_read8(operand, offset + i, &ok);
	}
	return ok;
}

static bool bus_write(const UPD8087_OPERAND *operand, uint32_t offset,
	const uint8_t *data, unsigned size) {
	unsigned i;

	if (operand->bus.write8 == NULL) {
		return false;
	}
	for (i = 0; i < size; i++) {
		operand->bus.write8(operand->bus.opaque, operand->address + offset + i, data[i]);
	}
	return true;
}

static bool read_u16(const UPD8087_OPERAND *operand, uint32_t offset, uint16_t *value) {
	uint8_t data[2];

	if (!bus_read(operand, offset, data, sizeof(data))) {
		return false;
	}
	*value = load16(data);
	return true;
}

static bool read_u32(const UPD8087_OPERAND *operand, uint32_t offset, uint32_t *value) {
	uint8_t data[4];

	if (!bus_read(operand, offset, data, sizeof(data))) {
		return false;
	}
	*value = load32(data);
	return true;
}

static bool read_u64(const UPD8087_OPERAND *operand, uint32_t offset, uint64_t *value) {
	uint8_t data[8];

	if (!bus_read(operand, offset, data, sizeof(data))) {
		return false;
	}
	*value = load64(data);
	return true;
}

static bool write_u16(const UPD8087_OPERAND *operand, uint32_t offset, uint16_t value) {
	uint8_t data[2];

	store16(data, value);
	return bus_write(operand, offset, data, sizeof(data));
}

static bool write_u32(const UPD8087_OPERAND *operand, uint32_t offset, uint32_t value) {
	uint8_t data[4];

	store32(data, value);
	return bus_write(operand, offset, data, sizeof(data));
}

static bool write_u64(const UPD8087_OPERAND *operand, uint32_t offset, uint64_t value) {
	uint8_t data[8];

	store64(data, value);
	return bus_write(operand, offset, data, sizeof(data));
}

static bool read_raw80(const UPD8087_OPERAND *operand, uint32_t offset,
	UPD8087_RAW80 *value) {
	uint8_t data[10];

	if (!bus_read(operand, offset, data, sizeof(data))) {
		return false;
	}
	value->signif = load64(data);
	value->sign_exp = load16(data + 8);
	return true;
}

static bool write_raw80(const UPD8087_OPERAND *operand, uint32_t offset,
	UPD8087_RAW80 value) {
	uint8_t data[10];

	store64(data, value.signif);
	store16(data + 8, value.sign_exp);
	return bus_write(operand, offset, data, sizeof(data));
}

static UPD8087_RAW80 raw_from_f32(uint32_t bits, UPD8087_STATE *state) {
	float32_t input = {bits};
	extFloat80_t converted;
	SOFTFLOAT_CONTEXT context;
	UPD8087_RAW80 result;

	if ((bits & UINT32_C(0x7f800000)) == 0 &&
	    (bits & UINT32_C(0x007fffff)) != 0) {
		raise_status(state, UPD8087_STATUS_DE);
		/* I02 Table S-24 loads a short-real denormal as its exact
		 * temporary-real equivalent unnormal.  SoftFloat normalizes this
		 * conversion, so preserve the zero integer bit explicitly. */
		result.signif = (uint64_t)(bits & UINT32_C(0x007fffff)) << 40;
		result.sign_exp = (uint16_t)((bits >> 16 & 0x8000) | 16257);
		return result;
	}

	softfloat_context_begin(state, &context);
	f32_to_extF80M(input, &converted);
	softfloat_context_finish(state, &context);
	return raw_from_sf_for_arithmetic(state, converted);
}

static UPD8087_RAW80 raw_from_f64(uint64_t bits, UPD8087_STATE *state) {
	float64_t input = {bits};
	extFloat80_t converted;
	SOFTFLOAT_CONTEXT context;
	UPD8087_RAW80 result;

	if ((bits & UINT64_C(0x7ff0000000000000)) == 0 &&
	    (bits & UINT64_C(0x000fffffffffffff)) != 0) {
		raise_status(state, UPD8087_STATUS_DE);
		/* I02 Table S-24 gives the same masked-load rule for long-real
		 * denormals.  Keep the exact value as an unnormal. */
		result.signif = (bits & UINT64_C(0x000fffffffffffff)) << 11;
		result.sign_exp = (uint16_t)((bits >> 48 & 0x8000) | 15361);
		return result;
	}

	softfloat_context_begin(state, &context);
	f64_to_extF80M(input, &converted);
	softfloat_context_finish(state, &context);
	return raw_from_sf_for_arithmetic(state, converted);
}

static uint32_t f32_from_raw(UPD8087_STATE *state, UPD8087_RAW80 value) {
	float32_t converted;
	SOFTFLOAT_CONTEXT context;

	softfloat_context_begin(state, &context);
	converted = extF80_to_f32(sf_from_raw(raw_for_arithmetic(state, value)));
	softfloat_context_finish(state, &context);
	return converted.v;
}

static uint64_t f64_from_raw(UPD8087_STATE *state, UPD8087_RAW80 value) {
	float64_t converted;
	SOFTFLOAT_CONTEXT context;

	softfloat_context_begin(state, &context);
	converted = extF80_to_f64(sf_from_raw(raw_for_arithmetic(state, value)));
	softfloat_context_finish(state, &context);
	return converted.v;
}

static UPD8087_RAW80 raw_from_memory(UPD8087_STATE *state,
	const UPD8087_OPERAND *operand, unsigned size, bool *ok) {
	uint32_t short_bits;
	uint64_t long_bits;
	UPD8087_RAW80 value;

	*ok = false;
	if (size == 4) {
		if (!read_u32(operand, 0, &short_bits)) {
			return raw_qnan;
		}
		value = raw_from_f32(short_bits, state);
	} else if (size == 8) {
		if (!read_u64(operand, 0, &long_bits)) {
			return raw_qnan;
		}
		value = raw_from_f64(long_bits, state);
	} else if (size == 10) {
		if (!read_raw80(operand, 0, &value)) {
			return raw_qnan;
		}
		if (raw_is_denormal(value)) {
			raise_status(state, UPD8087_STATUS_DE);
		}
	} else {
		return raw_qnan;
	}
	*ok = true;
	return value;
}

static int raw_compare(UPD8087_STATE *state, UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs, bool *unordered) {
	*unordered = false;
	if (raw_is_nan(lhs) || raw_is_nan(rhs)) {
		*unordered = true;
		raise_status(state, UPD8087_STATUS_IE);
		return 0;
	}
	if (!(state->control & CONTROL_IC) &&
	    (raw_is_infinity(lhs) || raw_is_infinity(rhs))) {
		/* In projective closure both signed infinity encodings compare
		 * equal to each other.  Infinity versus any finite value remains
		 * unordered and invalid, as specified by Table S-27. */
		if (raw_is_infinity(lhs) && raw_is_infinity(rhs)) {
			return 0;
		}
		*unordered = true;
		raise_status(state, UPD8087_STATUS_IE);
		return 0;
	}
	if (raw_is_denormal(lhs) || raw_is_denormal(rhs) ||
	    (raw_is_unnormal(lhs) && lhs.signif != 0) ||
	    (raw_is_unnormal(rhs) && rhs.signif != 0)) {
		raise_status(state, UPD8087_STATUS_DE);
	}
	lhs = raw_for_arithmetic(state, raw_for_compare(lhs));
	rhs = raw_for_arithmetic(state, raw_for_compare(rhs));
	SOFTFLOAT_CONTEXT context;
	softfloat_context_begin(state, &context);
	if (extF80_eq(sf_from_raw(lhs), sf_from_raw(rhs))) {
		softfloat_context_finish(state, &context);
		return 0;
	}
	if (extF80_lt(sf_from_raw(lhs), sf_from_raw(rhs))) {
		softfloat_context_finish(state, &context);
		return -1;
	}
	softfloat_context_finish(state, &context);
	return 1;
}

static void set_compare_conditions(UPD8087_STATE *state, int comparison,
	bool unordered) {
	state->status &= (uint16_t)~(UPD8087_STATUS_C0 | UPD8087_STATUS_C1 |
	                              UPD8087_STATUS_C2 | STATUS_C3);
	if (unordered) {
		state->status |= UPD8087_STATUS_C0 | UPD8087_STATUS_C2 | STATUS_C3;
	} else if (comparison < 0) {
		state->status |= UPD8087_STATUS_C0;
	} else if (comparison == 0) {
		state->status |= STATUS_C3;
	}
}

static UPD8087_RAW80 exp2_series(UPD8087_STATE *state, UPD8087_RAW80 value) {
	UPD8087_RAW80 term = raw_one;
	UPD8087_RAW80 sum = raw_one;
	UPD8087_RAW80 x = sf_mul(state, value, raw_ln2);
	unsigned n;

	for (n = 1; n <= 96; n++) {
		term = sf_div(state, sf_mul(state, term, x), raw_from_u64(n));
		sum = sf_add(state, sum, term);
		if (raw_is_zero(term)) {
			break;
		}
	}
	return sum;
}

static UPD8087_RAW80 log2_mantissa(UPD8087_STATE *state, UPD8087_RAW80 value) {
	UPD8087_RAW80 numerator = sf_sub(state, value, raw_one);
	UPD8087_RAW80 denominator = sf_add(state, value, raw_one);
	UPD8087_RAW80 y = sf_div(state, numerator, denominator);
	UPD8087_RAW80 y2 = sf_mul(state, y, y);
	UPD8087_RAW80 term = y;
	UPD8087_RAW80 sum = y;
	unsigned n;

	for (n = 3; n <= 191; n += 2) {
		term = sf_mul(state, term, y2);
		sum = sf_add(state, sum, sf_div(state, term, raw_from_u64(n)));
		if (raw_is_zero(term)) {
			break;
		}
	}
	return sf_mul(state, sf_mul(state, sum, raw_from_u64(2)), raw_l2e);
}

static UPD8087_RAW80 log2_value(UPD8087_STATE *state, UPD8087_RAW80 value) {
	uint16_t exponent;
	int32_t unbiased;
	UPD8087_RAW80 mantissa;

	if (raw_is_zero(value)) {
		raise_status(state, UPD8087_STATUS_IE);
		return raw_qnan;
	}
	if (raw_is_denormal(value)) {
		/* Normalize a denormal using the same exact integer significand path. */
		UPD8087_RAW80 two = raw_from_i32(2);
		unsigned i;
		mantissa = value;
		for (i = 0; i < 64 && raw_is_denormal(mantissa); i++) {
			mantissa = sf_mul(state, mantissa, two);
		}
		exponent = mantissa.sign_exp & RAW_EXP_MASK;
		unbiased = (int32_t)exponent - 16383 - (int32_t)i;
	} else {
		exponent = value.sign_exp & RAW_EXP_MASK;
		unbiased = (int32_t)exponent - 16383;
		mantissa = value;
	}
	mantissa.sign_exp = (uint16_t)((mantissa.sign_exp & RAW_SIGN_MASK) | 0x3fff);
	return sf_add(state, raw_from_i32(unbiased), log2_mantissa(state, mantissa));
}

static UPD8087_RAW80 log2_one_plus_value(UPD8087_STATE *state,
	UPD8087_RAW80 value) {
	UPD8087_RAW80 denominator;
	UPD8087_RAW80 y;
	UPD8087_RAW80 y_squared;
	UPD8087_RAW80 term;
	UPD8087_RAW80 sum;
	unsigned n;

	/* log2(1+x) = 2 * atanh(x/(2+x)) * log2(e).  Keeping x in the
	 * numerator avoids first rounding 1+x away from one. */
	denominator = sf_add(state, raw_two, value);
	y = sf_div(state, value, denominator);
	y_squared = sf_mul(state, y, y);
	term = y;
	sum = y;
	for (n = 3; n <= 191; n += 2) {
		term = sf_mul(state, term, y_squared);
		sum = sf_add(state, sum, sf_div(state, term, raw_from_u64(n)));
		if (raw_is_zero(term)) {
			break;
		}
	}
	return sf_mul(state, sf_mul(state, sum, raw_from_u64(2)), raw_l2e);
}

static UPD8087_RAW80 sin_series(UPD8087_STATE *state, UPD8087_RAW80 value) {
	UPD8087_RAW80 x2 = sf_mul(state, value, value);
	UPD8087_RAW80 term = value;
	UPD8087_RAW80 sum = value;
	unsigned n;

	for (n = 1; n <= 48; n++) {
		UPD8087_RAW80 denominator = sf_mul(state, raw_from_u64(2 * n),
		                                  raw_from_u64(2 * n + 1));
		term = sf_div(state, sf_mul(state, term, x2), denominator);
		if (n & 1) {
			sum = sf_sub(state, sum, term);
		} else {
			sum = sf_add(state, sum, term);
		}
		if (raw_is_zero(term)) {
			break;
		}
	}
	return sum;
}

static UPD8087_RAW80 cos_series(UPD8087_STATE *state, UPD8087_RAW80 value) {
	UPD8087_RAW80 x2 = sf_mul(state, value, value);
	UPD8087_RAW80 term = raw_one;
	UPD8087_RAW80 sum = raw_one;
	unsigned n;

	for (n = 1; n <= 48; n++) {
		UPD8087_RAW80 denominator = sf_mul(state, raw_from_u64(2 * n - 1),
		                                  raw_from_u64(2 * n));
		term = sf_div(state, sf_mul(state, term, x2), denominator);
		if (n & 1) {
			sum = sf_sub(state, sum, term);
		} else {
			sum = sf_add(state, sum, term);
		}
		if (raw_is_zero(term)) {
			break;
		}
	}
	return sum;
}

static UPD8087_RAW80 atan_value(UPD8087_STATE *state, UPD8087_RAW80 value) {
	bool negative = (value.sign_exp & RAW_SIGN_MASK) != 0;
	UPD8087_RAW80 absolute = raw_abs(value);
	UPD8087_RAW80 result;
	UPD8087_RAW80 one = raw_one;
	UPD8087_RAW80 pi_over_two = sf_div(state, raw_pi, raw_from_i32(2));
	UPD8087_RAW80 z;
	UPD8087_RAW80 z2;
	UPD8087_RAW80 term;
	UPD8087_RAW80 sum;
	unsigned n;
	bool reciprocal = false;
	bool unordered;

	if (raw_compare(state, one, absolute, &unordered) < 0 && !unordered) {
		reciprocal = true;
		z = sf_div(state, one, absolute);
	} else {
		z = absolute;
	}
	z2 = sf_mul(state, z, z);
	term = z;
	sum = z;
	for (n = 1; n <= 191; n++) {
		term = sf_mul(state, term, z2);
		if (n & 1) {
			sum = sf_sub(state, sum, sf_div(state, term, raw_from_u64(2 * n + 1)));
		} else {
			sum = sf_add(state, sum, sf_div(state, term, raw_from_u64(2 * n + 1)));
		}
		if (raw_is_zero(term)) {
			break;
		}
	}
	result = reciprocal ? sf_sub(state, pi_over_two, sum) : sum;
	return negative ? raw_neg(result) : result;
}

static UPD8087_RAW80 raw_power_of_two(int32_t amount) {
	UPD8087_RAW80 result;
	int32_t shift;

	if (amount > 16383) {
		return (UPD8087_RAW80){RAW_INTEGER_BIT, RAW_EXP_MASK};
	}
	if (amount >= -16382) {
		return (UPD8087_RAW80){RAW_INTEGER_BIT, (uint16_t)(16383 + amount)};
	}
	if (amount < -16445) {
		return raw_zero;
	}
	shift = -16382 - amount;
	if (shift >= 64) {
		return raw_zero;
	}
	result.signif = RAW_INTEGER_BIT >> shift;
	result.sign_exp = 0;
	return result;
}

static UPD8087_RAW80 raw_scale_power(UPD8087_STATE *state, UPD8087_RAW80 value,
	int32_t amount) {
	uint16_t exponent;
	int32_t next;
	UPD8087_RAW80 power;

	if (raw_is_zero(value) || raw_is_nan(value) || raw_is_infinity(value)) {
		return value;
	}
	exponent = value.sign_exp & RAW_EXP_MASK;
	if (!raw_is_denormal(value)) {
		next = (int32_t)exponent + amount;
		if ((next > 0) && (next < (int32_t)RAW_EXP_MASK)) {
			value.sign_exp = (uint16_t)((value.sign_exp & RAW_SIGN_MASK) | next);
			return value;
		}
	}
	power = raw_power_of_two(amount);
	return raw_from_sf_for_arithmetic(state, sf_binary(state, value, power, 2));
}

static bool raw_scale_amount(UPD8087_RAW80 value, int32_t *amount) {
	uint16_t exponent;
	int32_t unbiased;
	unsigned shift;
	uint64_t magnitude;

	if (raw_is_zero(value)) {
		*amount = 0;
		return true;
	}
	if (!raw_is_normal(value)) {
		return false;
	}
	exponent = value.sign_exp & RAW_EXP_MASK;
	unbiased = (int32_t)exponent - 16383;
	if (unbiased < 0) {
		/* I02 specifies 0 < |ST(1)| < 1 as undefined for FSCALE. */
		return false;
	}
	if (unbiased > 15) {
		return false;
	}
	shift = (unsigned)(63 - unbiased);
	/* FSCALE truncates the scale operand toward zero. */
	magnitude = value.signif >> shift;
	if ((value.sign_exp & RAW_SIGN_MASK) != 0) {
		if (magnitude > UINT32_C(32768)) {
			return false;
		}
		if ((magnitude == UINT32_C(32768)) &&
		    (value.signif & UINT64_C(0x0000ffffffffffff)) != 0) {
			/* The lower endpoint is inclusive, but a negative fractional
			 * value below -32768 is outside the documented range. */
			return false;
		}
		*amount = (magnitude == UINT32_C(32768)) ? -32768 :
		          -(int32_t)magnitude;
	} else {
		if (magnitude >= UINT32_C(32768)) {
			return false;
		}
		*amount = (int32_t)magnitude;
	}
	return true;
}

static UPD8087_RAW80 packed_bcd_to_raw(UPD8087_STATE *state,
	const UPD8087_OPERAND *operand, bool *ok) {
	uint8_t data[10];
	UPD8087_RAW80 result;
	uint64_t integer = 0;
	unsigned i;
	bool negative;

	*ok = false;
	if (!bus_read(operand, 0, data, sizeof(data))) {
		return raw_qnan;
	}
	/* The 8087 assumes packed-BCD digits are 0..9 and does not validate
	 * A..F encodings.  Their result is architecturally undefined, so retain
	 * the deterministic digit walk without inventing an exception. */
	negative = (data[9] & 0x80) != 0;
	for (i = 9; i != 0; i--) {
		uint8_t byte = data[i - 1];

		integer = integer * 10 + (byte >> 4);
		integer = integer * 10 + (byte & 0x0f);
	}
	result = raw_from_u64(integer);
	if (negative) {
		result.sign_exp |= RAW_SIGN_MASK;
	}
	*ok = true;
	return result;
}

static bool raw_to_packed_bcd(UPD8087_STATE *state, UPD8087_RAW80 input,
	uint8_t data[10]) {
	UPD8087_RAW80 rounded;
	UPD8087_RAW80 absolute;
	UPD8087_RAW80 maximum;
	uint64_t integer;
	unsigned i;
	SOFTFLOAT_CONTEXT context;

	memset(data, 0, 10);
	if (raw_is_nan(input) || raw_is_infinity(input) ||
	    raw_is_denormal(input) || raw_is_unnormal(input)) {
		raise_status(state, UPD8087_STATUS_IE);
		data[9] = 0xff;
		return state_can_commit(state);
	}
	/* FBSTP rounds by adding 0.5 to the magnitude and chopping.  This is
	 * independent of the binary RC field; a negative result receives the
	 * sign after the magnitude conversion. */
	rounded = raw_abs(input);
	rounded = sf_add_silent(state, rounded, raw_half);
	absolute = raw_abs(rounded);
	maximum = raw_from_u64(UINT64_C(999999999999999999));
	if (raw_is_infinity(absolute) || raw_is_nan(absolute) ||
		raw_compare(state, absolute, maximum, &(bool){false}) > 0) {
		raise_status(state, UPD8087_STATUS_IE);
		memset(data, 0, 10);
		data[9] = 0xff;
		return state_can_commit(state);
	}
	softfloat_context_begin(state, &context);
	integer = extF80_to_ui64(sf_from_raw(absolute), softfloat_round_minMag, false);
	softfloat_context_finish(state, &context);
	if ((state->working_flags & UPD8087_STATUS_IE) ||
	    (integer > UINT64_C(999999999999999999))) {
		raise_status(state, UPD8087_STATUS_IE);
		memset(data, 0, 10);
		data[9] = 0xff;
		return state_can_commit(state);
	}
	for (i = 0; i < 9; i++) {
		data[i] = (uint8_t)(integer % 10);
		integer /= 10;
		data[i] |= (uint8_t)((integer % 10) << 4);
		integer /= 10;
	}
	if ((input.sign_exp & RAW_SIGN_MASK) != 0) {
		data[9] = 0x80;
	}
	return true;
}

static bool raw_to_integer(UPD8087_STATE *state, UPD8087_RAW80 input,
	unsigned bits, int64_t *signed_value) {
	UPD8087_RAW80 rounded;
	int64_t value;
	SOFTFLOAT_CONTEXT context;

	if (raw_is_nan(input) || raw_is_infinity(input) ||
	    raw_is_denormal(input) || raw_is_unnormal(input)) {
		raise_status(state, UPD8087_STATUS_IE);
		return false;
	}
	rounded = sf_round(state, input);
	softfloat_context_begin(state, &context);
	value = extF80_to_i64(sf_from_raw(rounded), softfloat_round_minMag, false);
	softfloat_context_finish(state, &context);
	if ((state->working_flags & UPD8087_STATUS_IE) ||
	    ((bits == 16) && (value < -32768 || value > 32767)) ||
	    ((bits == 32) && (value < INT32_MIN || value > INT32_MAX))) {
		raise_status(state, UPD8087_STATUS_IE);
		return false;
	}
	*signed_value = value;
	return true;
}

static bool operation_compare(UPD8087_STATE *state, UPD8087_RAW80 rhs,
	bool pop) {
	bool unordered;
	bool stack_fault;
	int comparison;

	if (!stack_require(state, 0) && !state_can_commit(state)) {
		return false;
	}
	stack_fault = (state->working_flags & UPD8087_STATUS_SF) != 0;
	comparison = raw_compare(state, stack_get(state, 0), rhs, &unordered);
	if (state_can_commit(state)) {
		set_compare_conditions(state, comparison, unordered);
		if (stack_fault) {
			state->status |= UPD8087_STATUS_C1;
		}
	}
	if (pop && state_can_commit(state)) {
		stack_pop(state);
	}
	return true;
}

/* The original NDP's infinity control and invalid-operation responses are
 * not the same as the later IEEE x87 defaults.  Resolve the cases which
 * SoftFloat intentionally leaves to IEEE before entering the arithmetic
 * substrate. */
static bool binary_special_case(UPD8087_STATE *state, UPD8087_RAW80 lhs,
	UPD8087_RAW80 rhs, unsigned operation, UPD8087_RAW80 *result) {
	bool lhs_infinity = raw_is_infinity(lhs);
	bool rhs_infinity = raw_is_infinity(rhs);
	bool lhs_zero = raw_is_zero(lhs);
	bool rhs_zero = raw_is_zero(rhs);
	bool negative;

	if (raw_is_nan(lhs) || raw_is_nan(rhs)) {
		raise_status(state, UPD8087_STATUS_IE);
		*result = raw_is_nan(lhs) && raw_is_nan(rhs) ?
			raw_larger_nan(raw_abs(lhs), raw_abs(rhs)) :
			(raw_is_nan(lhs) ? lhs : rhs);
		return true;
	}
	if (raw_is_denormal(lhs) || raw_is_denormal(rhs)) {
		raise_status(state, UPD8087_STATUS_DE);
	}

	if (operation == 0 || operation == 1) {
		if (lhs_infinity && rhs_infinity) {
			if (!(state->control & CONTROL_IC) ||
			    ((operation == 0) == (raw_negative(lhs) != raw_negative(rhs)))) {
				raise_status(state, UPD8087_STATUS_IE);
				*result = raw_indefinite;
				return true;
			}
			/* Affine +inf + +inf, -inf + -inf, +inf - -inf, and
			 * -inf - +inf retain the left infinity. */
			*result = lhs;
			return true;
		}
		if (lhs_infinity) {
			*result = lhs;
			return true;
		}
		if (rhs_infinity) {
			*result = (operation == 0) ? rhs : raw_neg(rhs);
			return true;
		}
		return false;
	}

	if (operation == 2) {
		if ((lhs_infinity && rhs_zero) || (rhs_infinity && lhs_zero)) {
			raise_status(state, UPD8087_STATUS_IE);
			*result = raw_indefinite;
			return true;
		}
		if (lhs_infinity || rhs_infinity) {
			negative = raw_negative(lhs) != raw_negative(rhs);
			*result = raw_with_sign(lhs_infinity ? lhs : rhs, negative);
			return true;
		}
		return false;
	}

	/* Division and remainder reject a denormal/unnormal divisor with
	 * Invalid Operation rather than accepting SoftFloat's IEEE path. */
	if (raw_is_denormal(rhs) || raw_is_unnormal(rhs)) {
		raise_status(state, UPD8087_STATUS_IE);
		*result = raw_indefinite;
		return true;
	}
	if ((lhs_infinity && rhs_infinity) || (lhs_zero && rhs_zero)) {
		raise_status(state, UPD8087_STATUS_IE);
		*result = raw_indefinite;
		return true;
	}
	if (lhs_infinity || rhs_infinity) {
		negative = raw_negative(lhs) != raw_negative(rhs);
		if (lhs_infinity) {
			*result = raw_with_sign(lhs, negative);
		} else {
			*result = raw_with_sign(raw_zero, negative);
		}
		return true;
	}
	return false;
}

static bool binary_stack_operation(UPD8087_STATE *state, UPD8087_RAW80 rhs,
	unsigned operation, bool reverse, bool pop, unsigned destination) {
	UPD8087_RAW80 lhs;
	UPD8087_RAW80 left;
	UPD8087_RAW80 right;
	UPD8087_RAW80 result;

	if (!stack_require(state, 0) && !state_can_commit(state)) {
		return false;
	}
	if (destination != 0 && !stack_require(state, destination) &&
	    !state_can_commit(state)) {
		return false;
	}
	lhs = stack_get(state, destination);
	left = reverse ? rhs : lhs;
	right = reverse ? lhs : rhs;
	if (!binary_special_case(state, left, right, operation, &result)) {
		result = raw_from_sf_for_arithmetic(state,
			sf_binary(state, left, right, operation));
		result = raw_binary_result_class(left, right, operation, result);
	}
	if (state_can_commit(state)) {
		stack_set(state, destination, result);
		if (pop) {
			stack_pop(state);
		}
	}
	return true;
}

static void fxam(UPD8087_STATE *state) {
	UPD8087_RAW80 value = stack_get(state, 0);
	uint8_t tag = stack_tag(state, 0);
	unsigned code;
	bool negative = (value.sign_exp & RAW_SIGN_MASK) != 0;

	state->status &= (uint16_t)~(UPD8087_STATUS_C0 | UPD8087_STATUS_C1 |
	                              UPD8087_STATUS_C2 | STATUS_C3);
	if (tag == UPD8087_TAG_EMPTY) {
		code = negative ? 0x0f : 0x09;
	} else if (raw_is_zero(value)) {
		code = negative ? 0x0a : 0x08;
	} else if (raw_is_denormal(value)) {
		code = negative ? 0x0e : 0x0c;
	} else if (raw_is_infinity(value)) {
		code = negative ? 0x07 : 0x05;
	} else if (raw_is_nan(value)) {
		code = negative ? 0x03 : 0x01;
	} else if (raw_is_unnormal(value)) {
		code = negative ? 0x02 : 0x00;
	} else {
		code = negative ? 0x06 : 0x04;
	}
	if (code & 8) {
		state->status |= STATUS_C3;
	}
	if (code & 4) {
		state->status |= UPD8087_STATUS_C2;
	}
	if (code & 2) {
		state->status |= UPD8087_STATUS_C1;
	}
	if (code & 1) {
		state->status |= UPD8087_STATUS_C0;
	}
}

static bool is_memory_form(uint8_t modrm) {
	return (modrm & 0xc0) != 0xc0;
}

UPD8087_SLOT_CLASS upd8087_classify(uint8_t opcode, uint8_t modrm) {
	unsigned reg = (modrm >> 3) & 7;
	unsigned rm = modrm & 7;

	if ((opcode < 0xd8) || (opcode > 0xdf)) {
		return UPD8087_SLOT_UNDEFINED;
	}
	if (is_memory_form(modrm)) {
		switch (opcode) {
		case 0xd8:
		case 0xda:
		case 0xdc:
		case 0xde:
			return UPD8087_SLOT_MEMORY_ARITH;
		case 0xd9:
			return (reg == 1) ? UPD8087_SLOT_UNDEFINED :
			       UPD8087_SLOT_MEMORY_TRANSFER;
		case 0xdb:
			return (reg == 0 || reg == 2 || reg == 3 || reg == 5 || reg == 7) ?
			       UPD8087_SLOT_INTEGER : UPD8087_SLOT_UNDEFINED;
		case 0xdd:
			return (reg == 0 || reg == 2 || reg == 3 || reg == 4 || reg == 6 || reg == 7) ?
			       UPD8087_SLOT_MEMORY_TRANSFER : UPD8087_SLOT_UNDEFINED;
		case 0xdf:
			if (reg == 1) {
				return UPD8087_SLOT_UNDEFINED;
			}
			return (reg == 4 || reg == 6) ? UPD8087_SLOT_PACKED_BCD :
			       UPD8087_SLOT_INTEGER;
		default:
			return UPD8087_SLOT_UNDEFINED;
		}
	}

	switch (opcode) {
	case 0xd8:
		return UPD8087_SLOT_REGISTER_ARITH;
	case 0xd9:
		if (reg == 0 || reg == 1) {
			return UPD8087_SLOT_REGISTER_TRANSFER;
		}
		if ((reg == 2 && rm == 0) || (reg == 4 && (rm == 0 || rm == 1 || rm == 4 || rm == 5)) ||
		    (reg == 5 && rm <= 6) || (reg == 6 && (rm <= 4 || rm >= 6)) ||
		    (reg == 7 && (rm == 0 || rm == 1 || rm == 2 || rm == 4 || rm == 5))) {
			return (reg == 5 || reg == 6 || reg == 7) ?
			       UPD8087_SLOT_TRANSCENDENTAL : UPD8087_SLOT_REGISTER_TRANSFER;
		}
		return UPD8087_SLOT_UNDEFINED;
	case 0xda:
		return UPD8087_SLOT_UNDEFINED;
	case 0xdb:
		return (reg == 4 && rm <= 3) ? UPD8087_SLOT_CONTROL : UPD8087_SLOT_UNDEFINED;
	case 0xdc:
		return (reg == 0 || reg == 1 || reg == 4 || reg == 5 || reg == 6 || reg == 7) ?
		       UPD8087_SLOT_REGISTER_ARITH : UPD8087_SLOT_UNDEFINED;
	case 0xdd:
		return (reg == 0 || reg == 2 || reg == 3) ?
		       UPD8087_SLOT_REGISTER_TRANSFER : UPD8087_SLOT_UNDEFINED;
	case 0xde:
		return (reg == 0 || reg == 1 || reg == 4 || reg == 5 || reg == 6 || reg == 7 ||
		        (reg == 3 && rm == 1)) ? UPD8087_SLOT_REGISTER_ARITH :
		       UPD8087_SLOT_UNDEFINED;
	case 0xdf:
	default:
		return UPD8087_SLOT_UNDEFINED;
	}
}

uint32_t upd8087_instruction_cycles(uint8_t opcode, uint8_t modrm) {
	UPD8087_SLOT_CLASS slot = upd8087_classify(opcode, modrm);
	unsigned reg = (modrm >> 3) & 7;
	unsigned rm = modrm & 7;

	if (slot == UPD8087_SLOT_UNDEFINED) {
		return 0;
	}
	if (is_memory_form(modrm)) {
		/* These are the nominal NDP base clocks from I04's 8087 execution
		 * table.  The source's separate +EA term is recorded in
		 * timing-table.tsv; this API deliberately returns the base value. */
		switch (opcode) {
		case 0xd8:
			switch (reg) {
			case 0: return 105; /* FADD m32real */
			case 1: return 118; /* FMUL m32real */
			case 2: return 65;  /* FCOM m32real */
			case 3: return 68;  /* FCOMP m32real */
			case 4:
			case 5: return 105; /* FSUB/FSUBR m32real */
			case 6: return 220; /* FDIV m32real */
			case 7: return 221; /* FDIVR m32real */
			default: break;
			}
			break;
		case 0xd9:
			switch (reg) {
			case 0: return 43;  /* FLD m32real */
			case 2: return 87;  /* FST m32real */
			case 3: return 89;  /* FSTP m32real */
			case 4: return 40;  /* FLDENV */
			case 5: return 10;  /* FLDCW */
			case 6: return 45;  /* FSTENV */
			case 7: return 15;  /* FSTCW */
			default: break;
			}
			break;
		case 0xda:
			switch (reg) {
			case 0: return 125; /* FIADD m32int */
			case 1: return 136; /* FIMUL m32int */
			case 2: return 85;  /* FICOM m32int */
			case 3: return 87;  /* FICOMP m32int */
			case 4:
			case 5: return 125; /* FISUB/ FISUBR m32int */
			case 6: return 236; /* FIDIV m32int */
			case 7: return 237; /* FIDIVR m32int */
			default: break;
			}
			break;
		case 0xdb:
			switch (reg) {
			case 0: return 56; /* FILD m32int */
			case 2: return 88; /* FIST m32int */
			case 3: return 90; /* FISTP m32int */
			case 5: return 57; /* FLD m80real */
			case 7: return 55; /* FSTP m80real */
			default: break;
			}
			break;
		case 0xdc:
			switch (reg) {
			case 0: return 110; /* FADD m64real */
			case 1: return 161; /* FMUL m64real */
			case 2: return 70;  /* FCOM m64real */
			case 3: return 72;  /* FCOMP m64real */
			case 4:
			case 5: return 110; /* FSUB/FSUBR m64real */
			case 6: return 225; /* FDIV m64real */
			case 7: return 226; /* FDIVR m64real */
			default: break;
			}
			break;
		case 0xdd:
			switch (reg) {
			case 0: return 46;  /* FLD m64real */
			case 2: return 100; /* FST m64real */
			case 3: return 102; /* FSTP m64real */
			case 4:
			case 6: return 202; /* FRSTOR/FSAVE */
			case 7: return 15;  /* FSTSW */
			default: break;
			}
			break;
		case 0xde:
			switch (reg) {
			case 0: return 120; /* FIADD m16int */
			case 1: return 130; /* FIMUL m16int */
			case 2: return 80;  /* FICOM m16int */
			case 3: return 82;  /* FICOMP m16int */
			case 4:
			case 5: return 120; /* FISUBR/FSUB m16int */
			case 6:
			case 7: return 230; /* FIDIVR/FIDIV m16int */
			default: break;
			}
			break;
		case 0xdf:
			switch (reg) {
			case 0: return 50;  /* FILD m16int */
			case 2: return 86;  /* FIST m16int */
			case 3: return 88;  /* FISTP m16int */
			case 4: return 300; /* FBLD */
			case 5: return 64;  /* FILD m64int */
			case 6: return 530; /* FBSTP */
			case 7: return 100; /* FISTP m64int */
			default: break;
			}
			break;
		default:
			break;
		}
		return 0;
	}

	switch (opcode) {
	case 0xd8:
		switch (reg) {
		case 0:
		case 4: return 85;  /* FADD/FSUB */
		case 1: return 138; /* FMUL */
		case 2: return 45;  /* FCOM */
		case 3: return 47;  /* FCOMP */
		case 5: return 87;  /* FSUBR */
		case 6: return 198; /* FDIV */
		case 7: return 199; /* FDIVR */
		default: break;
		}
		break;
	case 0xd9:
		if (reg == 0) return 20; /* FLD ST(i) */
		if (reg == 1) return 12; /* FXCH ST(i) */
		if (reg == 2 && rm == 0) return 13; /* FNOP */
		if (reg == 4) {
			switch (rm) {
			case 0: return 15; /* FCHS */
			case 1: return 14; /* FABS */
			case 4: return 42; /* FTST */
			case 5: return 17; /* FXAM */
			default: break;
			}
		}
		if (reg == 5) {
			switch (rm) {
			case 0: return 18; /* FLD1 */
			case 1: return 19; /* FLDL2T */
			case 2: return 18; /* FLDL2E */
			case 3: return 19; /* FLDPI */
			case 4: return 21; /* FLDLG2 */
			case 5: return 20; /* FLDLN2 */
			case 6: return 14; /* FLDZ */
			default: break;
			}
		}
		if (reg == 6) {
			switch (rm) {
			case 0: return 500; /* F2XM1 */
			case 1: return 950; /* FYL2X */
			case 2: return 450; /* FPTAN */
			case 3: return 650; /* FPATAN */
			case 4: return 50;  /* FXTRACT */
			case 6: return 9;   /* FDECSTP */
			case 7: return 9;   /* FINCSTP */
			default: break;
			}
		}
		if (reg == 7) {
			switch (rm) {
			case 0: return 125; /* FPREM */
			case 1: return 850; /* FYL2XP1 */
			case 2: return 183; /* FSQRT */
			case 4: return 45;  /* FRNDINT */
			case 5: return 35;  /* FSCALE */
			default: break;
			}
		}
		break;
	case 0xdb:
		if (reg == 4 && rm <= 3) return 5; /* FENI..FINIT */
		break;
	case 0xdc:
		switch (reg) {
		case 0: return 85;  /* FADD */
		case 1: return 138; /* FMUL */
		case 4: return 87;  /* FSUBR */
		case 5: return 85;  /* FSUB */
		case 6: return 199; /* FDIVR */
		case 7: return 198; /* FDIV */
		default: break;
		}
		break;
	case 0xdd:
		if (reg == 0) return 11; /* FFREE */
		if (reg == 2) return 18; /* FST ST(i) */
		if (reg == 3) return 20; /* FSTP ST(i) */
		break;
	case 0xde:
		switch (reg) {
		case 0: return 90;  /* FADDP */
		case 1: return 142; /* FMULP */
		case 3: return 50;  /* FCOMPP (rm=1) */
		case 4:
		case 5: return 90;  /* FSUBRP/FSUBP */
		case 6: return 203; /* FDIVRP */
		case 7: return 202; /* FDIVP */
		default: break;
		}
		break;
	default:
		break;
	}
	return 0;
}

void upd8087_config_default(UPD8087_CONFIG *config) {
	if (config != NULL) {
		config->enabled = false;
		config->clock_hz = UPD8087_DEFAULT_CLOCK_HZ;
	}
}

bool upd8087_clock_valid(uint32_t clock_hz) {
	return (clock_hz >= UPD8087_MIN_CLOCK_HZ) && (clock_hz <= UPD8087_MAX_CLOCK_HZ);
}

void upd8087_reset(UPD8087_STATE *state) {
	bool enabled;
	uint32_t clock_hz;

	if (state == NULL) {
		return;
	}
	enabled = state->enabled;
	clock_hz = state->clock_hz;
	memset(state, 0, sizeof(*state));
	state->enabled = enabled;
	state->clock_hz = upd8087_clock_valid(clock_hz) ? clock_hz : UPD8087_DEFAULT_CLOCK_HZ;
	state->control = UPD8087_DEFAULT_CONTROL;
	state->top = 0;
	state->status = 0;
	for (unsigned i = 0; i < 8; i++) {
		state->tag[i] = UPD8087_TAG_EMPTY;
		state->reg[i] = raw_zero;
	}
	sync_top(state);
}

void upd8087_initialize(UPD8087_STATE *state, const UPD8087_CONFIG *config) {
	UPD8087_CONFIG selected;

	if (state == NULL) {
		return;
	}
	upd8087_config_default(&selected);
	if (config != NULL) {
		selected = *config;
	}
	memset(state, 0, sizeof(*state));
	state->enabled = selected.enabled;
	state->clock_hz = upd8087_clock_valid(selected.clock_hz) ? selected.clock_hz :
	                   UPD8087_DEFAULT_CLOCK_HZ;
	upd8087_reset(state);
}

void upd8087_apply_config(UPD8087_STATE *state, const UPD8087_CONFIG *config) {
	if ((state == NULL) || (config == NULL)) {
		return;
	}
	state->enabled = config->enabled;
	state->clock_hz = upd8087_clock_valid(config->clock_hz) ? config->clock_hz :
	                  UPD8087_DEFAULT_CLOCK_HZ;
	upd8087_reset(state);
}

uint32_t upd8087_service_ticks(UPD8087_STATE *state, uint32_t ndp_cycles,
	uint32_t cpu_clock_hz) {
	uint64_t total;
	uint64_t result;

	if ((state == NULL) || !state->enabled || (state->clock_hz == 0) ||
	    (cpu_clock_hz == 0)) {
		return 0;
	}
	total = (uint64_t)ndp_cycles * cpu_clock_hz + state->service_remainder;
	result = total / state->clock_hz;
	state->service_remainder = total % state->clock_hz;
	state->last_ndp_cycles = ndp_cycles;
	state->last_cpu_ticks = (result > UINT32_MAX) ? UINT32_MAX : (uint32_t)result;
	return state->last_cpu_ticks;
}

uint16_t upd8087_status(const UPD8087_STATE *state) {
	return (state == NULL) ? 0 : state->status;
}

uint16_t upd8087_tag_word(const UPD8087_STATE *state) {
	uint16_t result = 0;
	unsigned i;

	if (state == NULL) {
		return 0xffff;
	}
	for (i = 0; i < 8; i++) {
		result |= (uint16_t)((state->tag[i] & 3) << (i * 2));
	}
	return result;
}

uint16_t upd8087_control(const UPD8087_STATE *state) {
	return (state == NULL) ? UPD8087_DEFAULT_CONTROL : state->control;
}

void upd8087_debug_view(const UPD8087_STATE *state, UPD8087_DEBUG_VIEW *view) {
	if ((state == NULL) || (view == NULL)) {
		return;
	}
	memcpy(view->reg, state->reg, sizeof(view->reg));
	memcpy(view->tag, state->tag, sizeof(view->tag));
	view->enabled = state->enabled;
	view->clock_hz = state->clock_hz;
	view->top = state->top;
	view->control = state->control;
	view->status = state->status;
	view->tag_word = upd8087_tag_word(state);
	view->instruction_address = state->instruction_address;
	view->data_address = state->data_address;
	view->opcode = state->opcode;
	view->service_remainder = state->service_remainder;
	view->last_ndp_cycles = state->last_ndp_cycles;
	view->last_cpu_ticks = state->last_cpu_ticks;
	view->busy = state->busy;
	view->pending_interrupt = state->pending_interrupt;
}

void upd8087_set_control(UPD8087_STATE *state, uint16_t control) {
	uint16_t unmasked;

	if (state == NULL) {
		return;
	}
	state->control = control;
	unmasked = (uint16_t)(state->status & STATUS_EXCEPTION_MASK &
	                     (uint16_t)~control);
	if (unmasked != 0) {
		state->status |= UPD8087_STATUS_IR;
		state->status |= UPD8087_STATUS_B;
		state->busy = true;
		state->pending_interrupt = (control & CONTROL_IEM) == 0;
	} else {
		state->status &= (uint16_t)~(UPD8087_STATUS_IR | UPD8087_STATUS_B);
		state->busy = false;
		state->pending_interrupt = false;
	}
}

void upd8087_clear_exceptions(UPD8087_STATE *state) {
	if (state == NULL) {
		return;
	}
	state->status &= (uint16_t)~(STATUS_EXCEPTION_MASK | UPD8087_STATUS_SF |
	                              UPD8087_STATUS_IR);
	state->pending_interrupt = false;
	state->busy = false;
	state->status &= (uint16_t)~UPD8087_STATUS_B;
}

bool upd8087_busy(const UPD8087_STATE *state) {
	return (state != NULL) && state->busy;
}

bool upd8087_interrupt_pending(const UPD8087_STATE *state) {
	return (state != NULL) && state->pending_interrupt;
}

bool upd8087_wait_blocked(const UPD8087_STATE *state) {
	return (state != NULL) && state->enabled &&
	       (state->busy || state->pending_interrupt);
}

void upd8087_acknowledge_interrupt(UPD8087_STATE *state) {
	if (state != NULL) {
		/* Controller acknowledgement removes the external request pulse.
		 * IR and BUSY describe the still-live unmasked exception and are
		 * cleared by FCLEX/FINIT or a state/environment restore. */
		state->pending_interrupt = false;
	}
}

void upd8087_store_environment(const UPD8087_STATE *state, uint8_t image[14]) {
	uint16_t instruction_high;
	uint16_t data_high;

	if ((state == NULL) || (image == NULL)) {
		return;
	}
	memset(image, 0, 14);
	store16(image + 0, state->control);
	store16(image + 2, (uint16_t)(state->status & (uint16_t)~UPD8087_STATUS_B));
	store16(image + 4, upd8087_tag_word(state));
	store16(image + 6, (uint16_t)state->instruction_address);
	instruction_high = (uint16_t)((((state->instruction_address >> 16) & 0x0f) << 12) |
	                              (state->opcode & 0x07ff));
	store16(image + 8, instruction_high);
	store16(image + 10, (uint16_t)state->data_address);
	data_high = (uint16_t)(((state->data_address >> 16) & 0x0f) << 12);
	store16(image + 12, data_high);
}

void upd8087_load_environment(UPD8087_STATE *state, const uint8_t image[14]) {
	uint16_t instruction_high;
	uint16_t data_high;
	unsigned i;

	if ((state == NULL) || (image == NULL)) {
		return;
	}
	state->control = load16(image + 0);
	state->status = load16(image + 2);
	state->top = (uint8_t)((state->status >> 11) & 7);
	for (i = 0; i < 8; i++) {
		state->tag[i] = (uint8_t)((load16(image + 4) >> (i * 2)) & 3);
	}
	state->instruction_address = load16(image + 6);
	instruction_high = load16(image + 8);
	state->instruction_address |= (uint32_t)((instruction_high >> 12) & 0x000f) << 16;
	state->opcode = instruction_high & 0x07ff;
	state->data_address = load16(image + 10);
	data_high = load16(image + 12);
	state->data_address |= (uint32_t)((data_high >> 12) & 0x000f) << 16;
	state->pending_interrupt = ((state->status & STATUS_EXCEPTION_MASK &
	                             (uint16_t)~state->control) != 0) &&
	                           !(state->control & CONTROL_IEM);
	if ((state->status & STATUS_EXCEPTION_MASK &
	     (uint16_t)~state->control) != 0) {
		state->status |= UPD8087_STATUS_IR;
		state->status |= UPD8087_STATUS_B;
		state->busy = true;
	} else {
		state->status &= (uint16_t)~(UPD8087_STATUS_IR | UPD8087_STATUS_B);
		state->busy = false;
	}
	sync_top(state);
}

void upd8087_store_state(const UPD8087_STATE *state, uint8_t image[94]) {
	unsigned i;

	if ((state == NULL) || (image == NULL)) {
		return;
	}
	upd8087_store_environment(state, image);
	for (i = 0; i < 8; i++) {
		UPD8087_RAW80 value = stack_get(state, i);
		store64(image + 14 + i * 10, value.signif);
		store16(image + 22 + i * 10, value.sign_exp);
	}
}

void upd8087_load_state(UPD8087_STATE *state, const uint8_t image[94]) {
	unsigned i;

	if ((state == NULL) || (image == NULL)) {
		return;
	}
	upd8087_load_environment(state, image);
	for (i = 0; i < 8; i++) {
		unsigned index = physical_index(state, i);
		state->reg[index].signif = load64(image + 14 + i * 10);
		state->reg[index].sign_exp = load16(image + 22 + i * 10);
	}
}

bool upd8087_store_machine_state(const UPD8087_STATE *state, uint8_t *image,
	uint32_t image_size) {
	unsigned i;

	if ((state == NULL) || (image == NULL) ||
	    (image_size != UPD8087_STATE_IMAGE_SIZE)) {
		return false;
	}
	memset(image, 0, image_size);
	image[0] = STATE_MAGIC0;
	image[1] = STATE_MAGIC1;
	image[2] = STATE_MAGIC2;
	image[3] = STATE_MAGIC3;
	store16(image + 4, UPD8087_STATE_IMAGE_VERSION);
	store16(image + 6, (uint16_t)((state->enabled ? STATE_FLAG_ENABLED : 0) |
	                              (state->busy ? STATE_FLAG_BUSY : 0) |
	                              (state->pending_interrupt ? STATE_FLAG_PENDING : 0)));
	store32(image + 8, state->clock_hz);
	store64(image + 12, state->service_remainder);
	store32(image + 20, state->last_ndp_cycles);
	store32(image + 24, state->last_cpu_ticks);
	store32(image + 28, state->instruction_address);
	store32(image + 32, state->data_address);
	store16(image + 36, state->opcode);
	store16(image + 38, state->control);
	store16(image + 40, state->status);
	image[42] = state->top;
	image[43] = state->working_flags;
	for (i = 0; i < 8; i++) {
		image[44 + i] = state->tag[i];
		store64(image + 52 + i * 10, state->reg[i].signif);
		store16(image + 60 + i * 10, state->reg[i].sign_exp);
	}
	return true;
}

bool upd8087_load_machine_state(UPD8087_STATE *state, const uint8_t *image,
	uint32_t image_size) {
	UPD8087_STATE loaded;
	uint16_t flags;
	uint16_t control;
	uint16_t status;
	uint32_t clock_hz;
	uint32_t instruction_address;
	uint32_t data_address;
	uint64_t service_remainder;
	unsigned i;

	if ((state == NULL) || (image == NULL) ||
	    (image_size != UPD8087_STATE_IMAGE_SIZE) ||
	    (image[0] != STATE_MAGIC0) || (image[1] != STATE_MAGIC1) ||
	    (image[2] != STATE_MAGIC2) || (image[3] != STATE_MAGIC3) ||
	    (load16(image + 4) != UPD8087_STATE_IMAGE_VERSION)) {
		return false;
	}
	flags = load16(image + 6);
	clock_hz = load32(image + 8);
	service_remainder = load64(image + 12);
	control = load16(image + 38);
	status = load16(image + 40);
	instruction_address = load32(image + 28);
	data_address = load32(image + 32);
	if ((flags & (uint16_t)~(STATE_FLAG_ENABLED | STATE_FLAG_BUSY |
	                         STATE_FLAG_PENDING)) != 0 ||
	    !upd8087_clock_valid(clock_hz) || image[42] >= 8 ||
	    service_remainder >= clock_hz ||
	    (instruction_address & UINT32_C(0xfff00000)) != 0 ||
	    (data_address & UINT32_C(0xfff00000)) != 0 ||
	    (load16(image + 36) & (uint16_t)~UINT16_C(0x07ff)) != 0 ||
	    (control & (uint16_t)~UINT16_C(0x1fff)) != 0 ||
	    (control & CONTROL_PC) == UINT16_C(0x0100) ||
	    (image[43] & (uint8_t)~STATUS_EXCEPTION_MASK) != 0 ||
	    (((flags & STATE_FLAG_BUSY) != 0) !=
	     ((status & UPD8087_STATUS_B) != 0)) ||
	    ((flags & STATE_FLAG_PENDING) != 0 &&
	     (status & UPD8087_STATUS_IR) == 0) ||
	    (!(flags & STATE_FLAG_ENABLED) &&
	     (flags & (STATE_FLAG_BUSY | STATE_FLAG_PENDING)))) {
		return false;
	}
	for (i = 0; i < 8; i++) {
		if (image[44 + i] > UPD8087_TAG_EMPTY) {
			return false;
		}
	}
	memset(&loaded, 0, sizeof(loaded));
	loaded.enabled = (flags & STATE_FLAG_ENABLED) != 0;
	loaded.busy = (flags & STATE_FLAG_BUSY) != 0;
	loaded.pending_interrupt = (flags & STATE_FLAG_PENDING) != 0;
	loaded.clock_hz = clock_hz;
	loaded.service_remainder = service_remainder;
	loaded.last_ndp_cycles = load32(image + 20);
	loaded.last_cpu_ticks = load32(image + 24);
	loaded.instruction_address = instruction_address;
	loaded.data_address = data_address;
	loaded.opcode = load16(image + 36);
	loaded.control = control;
	loaded.status = status;
	loaded.top = image[42];
	loaded.working_flags = image[43];
	for (i = 0; i < 8; i++) {
		loaded.tag[i] = image[44 + i];
		loaded.reg[i].signif = load64(image + 52 + i * 10);
		loaded.reg[i].sign_exp = load16(image + 60 + i * 10);
	}
	if ((loaded.status & UPD8087_STATUS_TOP) !=
	    ((uint16_t)loaded.top << 11)) {
		return false;
	}
	*state = loaded;
	return true;
}

/*
 * The routines below are deliberately kept on the device side of the CPU
 * seam.  The CPU supplies an already decoded primary byte, ModR/M byte, and
 * (for memory forms) an effective address plus the first-word latch.  No
 * instruction stream is fetched here.
 */

static bool raw_negative(UPD8087_RAW80 value) {
	return (value.sign_exp & RAW_SIGN_MASK) != 0;
}

static bool read_signed16(const UPD8087_OPERAND *operand, int16_t *value) {
	uint16_t bits;

	if (!read_u16(operand, 0, &bits)) {
		return false;
	}
	*value = (bits & 0x8000) ? (int16_t)(-((int32_t)((~bits + 1) & 0xffff))) :
		       (int16_t)bits;
	return true;
}

static bool read_signed32(const UPD8087_OPERAND *operand, int32_t *value) {
	uint32_t bits;

	if (!read_u32(operand, 0, &bits)) {
		return false;
	}
	if (bits & UINT32_C(0x80000000)) {
		*value = (int32_t)(-((int64_t)((~bits + 1) & UINT32_MAX)));
	} else {
		*value = (int32_t)bits;
	}
	return true;
}

static bool read_signed64(const UPD8087_OPERAND *operand, int64_t *value) {
	uint64_t bits;
	uint64_t magnitude;

	if (!read_u64(operand, 0, &bits)) {
		return false;
	}
	if (bits & UINT64_C(0x8000000000000000)) {
		magnitude = (~bits) + 1;
		if (magnitude == UINT64_C(0x8000000000000000)) {
			*value = INT64_MIN;
		} else {
			*value = -(int64_t)magnitude;
		}
	} else {
		*value = (int64_t)bits;
	}
	return true;
}

static bool store_raw(UPD8087_STATE *state,
	const UPD8087_OPERAND *operand, unsigned size, UPD8087_RAW80 value) {
	uint32_t short_bits;
	uint64_t long_bits;
	uint16_t exponent;

	if ((size == 4 || size == 8) && raw_is_unnormal(value)) {
		exponent = value.sign_exp & RAW_EXP_MASK;
		/* A temporary unnormal above the destination's underflow boundary
		 * is an Invalid Operation for short/long stores.  At or below the
		 * boundary it is allowed to take the normal underflow conversion
		 * path. */
		if ((size == 4 && exponent > UINT16_C(16257)) ||
		    (size == 8 && exponent > UINT16_C(15361))) {
			raise_status(state, UPD8087_STATUS_IE);
			value = raw_indefinite;
			if (!state_can_commit(state)) {
				return true;
			}
		} else {
			/* An in-range unnormal is below the destination's normal
			 * boundary.  It is stored through the underflow path. */
			raise_status(state, UPD8087_STATUS_UE);
		}
	}

	if (size == 4) {
		short_bits = f32_from_raw(state, value);
		if (!store_can_commit(state)) {
			return true;
		}
		return write_u32(operand, 0, short_bits);
	}
	if (size == 8) {
		long_bits = f64_from_raw(state, value);
		if (!store_can_commit(state)) {
			return true;
		}
		return write_u64(operand, 0, long_bits);
	}
	if (size == 10) {
		if (!store_can_commit(state)) {
			return true;
		}
		return write_raw80(operand, 0, value);
	}
	return false;
}

static bool store_integer(UPD8087_STATE *state,
	const UPD8087_OPERAND *operand, unsigned bits, UPD8087_RAW80 value) {
	int64_t converted;

	if (!raw_to_integer(state, value, bits, &converted)) {
		if (!state_can_commit(state)) {
			return true;
		}
		if (bits == 16) {
			return write_u16(operand, 0, UINT16_C(0x8000));
		}
		if (bits == 32) {
			return write_u32(operand, 0, UINT32_C(0x80000000));
		}
		return write_u64(operand, 0, UINT64_C(0x8000000000000000));
	}
	if (!state_can_commit(state)) {
		return true;
	}
	if (bits == 16) {
		return write_u16(operand, 0, (uint16_t)converted);
	}
	if (bits == 32) {
		return write_u32(operand, 0, (uint32_t)converted);
	}
	return write_u64(operand, 0, (uint64_t)converted);
}

static bool stack_source_value(UPD8087_STATE *state, UPD8087_RAW80 *value) {
	if (stack_valid(state, 0)) {
		*value = stack_get(state, 0);
		return true;
	}
	raise_status(state, UPD8087_STATUS_IE | UPD8087_STATUS_SF);
	state->status |= UPD8087_STATUS_C1;
	*value = raw_indefinite;
	return state_can_commit(state);
}

static void fpu_initialize_architecture(UPD8087_STATE *state) {
	unsigned i;

	state->control = UPD8087_DEFAULT_CONTROL;
	state->status = 0;
	state->top = 0;
	state->instruction_address = 0;
	state->data_address = 0;
	state->opcode = 0;
	state->busy = false;
	state->pending_interrupt = false;
	state->working_flags = 0;
	for (i = 0; i < 8; i++) {
		state->reg[i] = raw_zero;
		state->tag[i] = UPD8087_TAG_EMPTY;
	}
	sync_top(state);
}

static void set_pending_if_unmasked(UPD8087_STATE *state) {
	uint16_t exceptions = state->status & STATUS_EXCEPTION_MASK;
	uint16_t unmasked = (uint16_t)(exceptions & (uint16_t)~state->control);

	if (unmasked != 0) {
		state->status |= UPD8087_STATUS_IR;
		state->status |= UPD8087_STATUS_B;
		state->busy = true;
		state->pending_interrupt = (state->control & CONTROL_IEM) == 0;
	} else {
		state->status &= (uint16_t)~UPD8087_STATUS_IR;
		state->pending_interrupt = false;
	}
}

static void set_c2(UPD8087_STATE *state, bool set) {
	if (set) {
		state->status |= UPD8087_STATUS_C2;
	} else {
		state->status &= (uint16_t)~UPD8087_STATUS_C2;
	}
}

static void set_remainder_quotient(UPD8087_STATE *state, uint64_t quotient) {
	state->status &= (uint16_t)~(UPD8087_STATUS_C0 | UPD8087_STATUS_C1 |
	                              UPD8087_STATUS_C2 | STATUS_C3);
	if (quotient & 4) {
		state->status |= UPD8087_STATUS_C0;
	}
	if (quotient & 2) {
		state->status |= UPD8087_STATUS_C1;
	}
	if (quotient & 1) {
		state->status |= STATUS_C3;
	}
}

static unsigned highest_bit_u64(uint64_t value) {
	unsigned result = 0;

	while (value >>= 1) {
		result++;
	}
	return result;
}

/* Divide (significand << shift) by a nonzero 64-bit significand.  The
 * dividend is at most 127 bits and the quotient is at most 64 bits.  The
 * two-limb subtract/compare path keeps FPREM independent of host
 * floating-point and compiler-specific 128-bit integer types. */
static void divide_shifted_u64(uint64_t significand, unsigned shift,
	uint64_t divisor, uint64_t *quotient, uint64_t *remainder) {
	uint64_t dividend_high = 0;
	uint64_t dividend_low;
	uint64_t result = 0;
	unsigned dividend_bits;
	unsigned divisor_bits = highest_bit_u64(divisor) + 1;
	int qbit;

	if (shift == 0) {
		dividend_low = significand;
	} else {
		dividend_high = significand >> (64 - shift);
		dividend_low = significand << shift;
	}
	dividend_bits = dividend_high ? highest_bit_u64(dividend_high) + 65 :
		highest_bit_u64(dividend_low) + 1;
	if (dividend_bits < divisor_bits) {
		*quotient = 0;
		*remainder = dividend_low;
		return;
	}
	for (qbit = (int)(dividend_bits - divisor_bits); qbit >= 0; qbit--) {
		uint64_t divisor_high;
		uint64_t divisor_low;
		uint64_t next_high;
		uint64_t next_low;
		bool borrow;
		bool take;

		if (qbit >= 64) {
			divisor_high = divisor << (qbit - 64);
			divisor_low = 0;
		} else if (qbit == 0) {
			divisor_high = 0;
			divisor_low = divisor;
		} else {
			divisor_high = divisor >> (64 - qbit);
			divisor_low = divisor << qbit;
		}
		take = (dividend_high > divisor_high) ||
		       ((dividend_high == divisor_high) &&
		        (dividend_low >= divisor_low));
		if (!take) {
			continue;
		}
		borrow = dividend_low < divisor_low;
		next_low = dividend_low - divisor_low;
		next_high = dividend_high - divisor_high - (borrow ? 1U : 0U);
		dividend_high = next_high;
		dividend_low = next_low;
		if (qbit < 64) {
			result |= UINT64_C(1) << qbit;
		}
	}
	*quotient = result;
	*remainder = dividend_low;
}

static void normalize_fprem_magnitude(uint64_t *significand, int32_t *power) {
	while ((*significand & RAW_INTEGER_BIT) == 0) {
		*significand <<= 1;
		(*power)--;
	}
}

static bool raw_fprem_magnitude(UPD8087_RAW80 value, uint64_t *significand,
	int32_t *power) {
	uint16_t exponent = value.sign_exp & RAW_EXP_MASK;

	if (value.signif == 0) {
		return false;
	}
	/* For exponent zero, the extended-format subnormal scale is the same
	 * as exponent one; the missing leading bit is carried by significand. */
	*significand = value.signif;
	*power = (int32_t)(exponent == 0 ? 1 : exponent) - 16446;
	normalize_fprem_magnitude(significand, power);
	return true;
}

static UPD8087_RAW80 raw_from_fprem_magnitude(uint64_t significand,
	int32_t power, bool negative) {
	int32_t exponent;
	unsigned right_shift;

	if (significand == 0) {
		return (UPD8087_RAW80){0, negative ? RAW_SIGN_MASK : 0};
	}
	normalize_fprem_magnitude(&significand, &power);
	exponent = power + 16446;
	if (exponent >= 1 && exponent <= RAW_EXP_MASK - 1) {
		return (UPD8087_RAW80){significand,
			(uint16_t)((negative ? RAW_SIGN_MASK : 0) | exponent)};
	}
	if (exponent <= 0) {
		right_shift = (unsigned)(1 - exponent);
		if (right_shift >= 64) {
			return (UPD8087_RAW80){0, negative ? RAW_SIGN_MASK : 0};
		}
		return (UPD8087_RAW80){significand >> right_shift,
			(uint16_t)(negative ? RAW_SIGN_MASK : 0)};
	}
	return (UPD8087_RAW80){RAW_INTEGER_BIT,
		(uint16_t)((negative ? RAW_SIGN_MASK : 0) | RAW_EXP_MASK)};
}

static bool execute_fprem(UPD8087_STATE *state) {
	UPD8087_RAW80 numerator;
	UPD8087_RAW80 denominator;
	UPD8087_RAW80 remainder;
	uint64_t numerator_significand;
	uint64_t denominator_significand;
	uint64_t quotient_magnitude;
	uint64_t remainder_magnitude;
	uint64_t quotient_bits;
	int32_t numerator_power;
	int32_t denominator_power;
	int32_t exponent_difference;
	int32_t remainder_power;
	bool partial = false;
	bool quotient_negative;

	if ((!stack_require(state, 0) && !state_can_commit(state)) ||
	    (!stack_require(state, 1) && !state_can_commit(state))) {
		return false;
	}
	numerator = stack_get(state, 0);
	denominator = stack_get(state, 1);
	if (raw_is_nan(numerator) || raw_is_nan(denominator) ||
		raw_is_infinity(numerator) || raw_is_zero(denominator) ||
		raw_is_denormal(denominator) || raw_is_unnormal(denominator)) {
		raise_status(state, UPD8087_STATUS_IE);
		if (state_can_commit(state)) {
			if (raw_is_nan(numerator) && raw_is_nan(denominator)) {
				numerator = raw_larger_nan(raw_abs(numerator), raw_abs(denominator));
			} else if (raw_is_nan(numerator)) {
				/* Preserve the sole NaN numerator for the masked result. */
			} else if (raw_is_nan(denominator)) {
				numerator = denominator;
			} else {
				numerator = raw_indefinite;
			}
			stack_set(state, 0, numerator);
			set_remainder_quotient(state, 0);
		}
		return true;
	}
	if (raw_is_denormal(numerator) || raw_is_denormal(denominator)) {
		raise_status(state, UPD8087_STATUS_DE);
	}
	if (raw_is_zero(numerator)) {
		if (state_can_commit(state)) {
			stack_set(state, 0, numerator);
			set_remainder_quotient(state, 0);
			set_c2(state, false);
		}
		return true;
	}
	if (!raw_fprem_magnitude(numerator, &numerator_significand,
	                         &numerator_power) ||
	    !raw_fprem_magnitude(denominator, &denominator_significand,
                         &denominator_power)) {
		return false;
	}
	exponent_difference = numerator_power - denominator_power;
	quotient_negative = raw_negative(numerator) != raw_negative(denominator);
	if (exponent_difference < 0) {
		quotient_magnitude = 0;
		remainder_magnitude = numerator_significand;
		remainder_power = numerator_power;
	} else {
		unsigned shift = (exponent_difference >= 64) ? 63U :
		                 (unsigned)exponent_difference;

		divide_shifted_u64(numerator_significand, shift,
		                   denominator_significand, &quotient_magnitude,
		                   &remainder_magnitude);
		if (exponent_difference >= 64) {
			partial = true;
			remainder_power = denominator_power + exponent_difference - 63;
		} else {
			remainder_power = denominator_power;
		}
	}
	quotient_bits = quotient_negative ? (UINT64_C(0) - quotient_magnitude) :
	               quotient_magnitude;
	remainder = raw_from_fprem_magnitude(remainder_magnitude, remainder_power,
	                                     raw_negative(numerator));
	if (state_can_commit(state)) {
		stack_set(state, 0, remainder);
		if (partial) {
			state->status &= (uint16_t)~(UPD8087_STATUS_C0 |
			                              UPD8087_STATUS_C1 | STATUS_C3);
			set_c2(state, true);
		} else {
			set_remainder_quotient(state, quotient_bits);
		}
	}
	return true;
}

static bool execute_fxtract(UPD8087_STATE *state) {
	UPD8087_RAW80 value;
	UPD8087_RAW80 exponent_value;
	UPD8087_RAW80 significand;
	int32_t exponent;

	if (!stack_require(state, 0) && !state_can_commit(state)) {
		return false;
	}
	value = stack_get(state, 0);
	if (raw_is_zero(value)) {
		exponent_value = value;
		significand = value;
	} else if (raw_is_nan(value) || raw_is_unnormal(value) ||
	           raw_is_infinity(value)) {
		raise_status(state, UPD8087_STATUS_IE);
		/* Table S-32 specifies real indefinite for the invalid infinity
		 * case.  The same staged invalid response is used for noncanonical
		 * FXTRACT operands; an unmasked before-exception leaves ST intact. */
		if (state_can_commit(state)) {
			stack_set(state, 0, raw_indefinite);
		}
		return true;
	} else {
		exponent = (int32_t)(value.sign_exp & RAW_EXP_MASK) - 16383;
		exponent_value = raw_from_i32(exponent);
		significand = value;
		significand.sign_exp = (uint16_t)((significand.sign_exp & RAW_SIGN_MASK) | 0x3fff);
	}
	if (state_can_commit(state) && stack_push(state, significand)) {
		stack_set(state, 1, exponent_value);
		return true;
	}
	return true;
}

static bool execute_fptan(UPD8087_STATE *state) {
	UPD8087_RAW80 value;
	UPD8087_RAW80 sine;
	UPD8087_RAW80 cosine;
	UPD8087_RAW80 tangent;
	bool unordered;

	if (!stack_require(state, 0) && !state_can_commit(state)) {
		return false;
	}
	value = stack_get(state, 0);
	if (!raw_is_normal(value) || raw_negative(value) ||
	    raw_compare(state, value, raw_pi_over_four, &unordered) >= 0 || unordered) {
		/* The original 8087 documents invalid/out-of-range transcendental
		 * operands as undefined without signalling an exception. */
		return true;
	}
	sine = sin_series(state, value);
	cosine = cos_series(state, value);
	tangent = sf_div(state, sine, cosine);
	if (state_can_commit(state)) {
		stack_set(state, 0, tangent);
		set_c2(state, false);
		if (!stack_push(state, raw_one)) {
			return false;
		}
	}
	return true;
}

static bool execute_fpatan(UPD8087_STATE *state) {
	UPD8087_RAW80 y;
	UPD8087_RAW80 x;
	UPD8087_RAW80 result;
	bool unordered;

	if ((!stack_require(state, 0) && !state_can_commit(state)) ||
	    (!stack_require(state, 1) && !state_can_commit(state))) {
		return false;
	}
	y = stack_get(state, 1);
	x = stack_get(state, 0);
	if (!raw_is_normal(y) || !raw_is_normal(x) || raw_negative(y) ||
	    raw_negative(x) ||
	    raw_compare(state, y, x, &unordered) >= 0 || unordered) {
		return true;
	}
	result = atan_value(state, sf_div(state, y, x));
	if (state_can_commit(state)) {
		stack_set(state, 1, result);
		stack_pop(state);
	}
	return true;
}

static bool execute_fsqrt(UPD8087_STATE *state) {
	UPD8087_RAW80 value;
	UPD8087_RAW80 result;

	if (!stack_require(state, 0) && !state_can_commit(state)) {
		return false;
	}
	value = stack_get(state, 0);
	if (raw_is_nan(value)) {
		raise_status(state, UPD8087_STATUS_IE);
		result = value;
	} else if (raw_is_denormal(value) || raw_is_unnormal(value) ||
	           (!raw_is_zero(value) && raw_negative(value)) ||
	           (raw_is_infinity(value) &&
	            (!(state->control & CONTROL_IC) || raw_negative(value)))) {
		raise_status(state, UPD8087_STATUS_IE);
		result = raw_indefinite;
	} else if (raw_is_infinity(value)) {
		result = value;
	} else {
		result = sf_unary(state, value, 0);
	}
	if (state_can_commit(state)) {
		stack_set(state, 0, result);
	}
	return true;
}

static bool execute_fscale(UPD8087_STATE *state) {
	UPD8087_RAW80 value;
	UPD8087_RAW80 multiplier;
	UPD8087_RAW80 result;
	int32_t amount;

	if ((!stack_require(state, 0) && !state_can_commit(state)) ||
	    (!stack_require(state, 1) && !state_can_commit(state))) {
		return false;
	}
	value = stack_get(state, 0);
	multiplier = stack_get(state, 1);
	if (raw_is_nan(value) || raw_is_nan(multiplier)) {
		raise_status(state, UPD8087_STATUS_IE);
		result = raw_is_nan(value) && raw_is_nan(multiplier) ?
			raw_larger_nan(raw_abs(value), raw_abs(multiplier)) :
			(raw_is_nan(value) ? value : multiplier);
	} else if (raw_is_infinity(multiplier)) {
		if (raw_is_zero(value)) {
			result = value;
		} else {
			raise_status(state, UPD8087_STATUS_IE);
			result = raw_indefinite;
		}
	} else if (raw_is_infinity(value)) {
		result = value;
	} else if (!raw_scale_amount(multiplier, &amount)) {
		/* The original source leaves the non-integral/out-of-range
		 * FSCALE domain undefined without a new exception. */
		return true;
	} else {
		result = raw_scale_power(state, value, amount);
	}
	if (state_can_commit(state)) {
		stack_set(state, 0, result);
	}
	return true;
}

static bool execute_transcendental(UPD8087_STATE *state, unsigned operation) {
	UPD8087_RAW80 value;
	UPD8087_RAW80 result;

	if (operation == 2) {
		return execute_fptan(state);
	}
	if (operation == 3) {
		return execute_fpatan(state);
	}
	if (operation == 4) {
		return execute_fxtract(state);
	}
	if (operation == 7) {
		return execute_fprem(state);
	}
	if (operation == 6) {
		return execute_fscale(state);
	}
	if (!stack_require(state, 0) && !state_can_commit(state)) {
		return false;
	}
	value = stack_get(state, 0);
	switch (operation) {
	case 0: /* F2XM1 */
		if ((!raw_is_zero(value) && !raw_is_normal(value)) ||
		    (!raw_is_zero(value) && raw_negative(value)) ||
		    raw_compare(state, value, raw_half, &(bool){false}) > 0) {
			return true;
		}
		result = sf_sub(state, exp2_series(state, value), raw_one);
		break;
	case 1: /* FYL2X */
		if (!stack_require(state, 1) && !state_can_commit(state)) {
			return false;
		}
		if (!raw_is_normal(value) || raw_negative(value) ||
		    ((!raw_is_zero(stack_get(state, 1))) &&
		     !raw_is_normal(stack_get(state, 1))) ||
		    raw_is_infinity(stack_get(state, 1))) {
			return true;
		}
		result = sf_mul(state, stack_get(state, 1), log2_value(state, value));
		if (state_can_commit(state)) {
			stack_set(state, 1, result);
			stack_pop(state);
		}
		return true;
	case 5: /* reserved in the original 8087 table */
		return false;
	default:
		return false;
	}
	if (state_can_commit(state)) {
		stack_set(state, 0, result);
	}
	return true;
}

static bool execute_fyl2xp1(UPD8087_STATE *state) {
	UPD8087_RAW80 x;
	UPD8087_RAW80 result;

	if ((!stack_require(state, 0) && !state_can_commit(state)) ||
	    (!stack_require(state, 1) && !state_can_commit(state))) {
		return false;
	}
	x = stack_get(state, 0);
	if (raw_is_zero(x) || !raw_is_normal(x) ||
	    raw_compare(state, raw_abs(x), raw_fyl2xp1_limit, &(bool){false}) >= 0) {
		return true;
	}
	if ((!raw_is_zero(stack_get(state, 1)) &&
	     !raw_is_normal(stack_get(state, 1))) ||
	    raw_is_infinity(stack_get(state, 1))) {
		return true;
	}
	result = sf_mul(state, stack_get(state, 1), log2_one_plus_value(state, x));
	if (state_can_commit(state)) {
		stack_set(state, 1, result);
		stack_pop(state);
	}
	return true;
}

static bool execute_memory_instruction(UPD8087_STATE *state, uint8_t opcode,
	uint8_t modrm, const UPD8087_OPERAND *operand) {
	unsigned reg = (modrm >> 3) & 7;
	UPD8087_RAW80 value;
	int16_t short_integer;
	int32_t integer;
	int64_t long_integer;
	uint16_t control;
	uint16_t status;
	uint8_t image[94];
	uint8_t bcd[10];
	bool ok;

	switch (opcode) {
	case 0xd8:
	case 0xdc:
		value = raw_from_memory(state, operand, opcode == 0xd8 ? 4 : 8, &ok);
		if (!ok || (!stack_require(state, 0) && !state_can_commit(state))) {
			return ok;
		}
		if (reg == 2 || reg == 3) {
			return operation_compare(state, value, reg == 3);
		}
		return binary_stack_operation(state, value, reg == 6 ? 3 :
			(reg == 7 ? 3 : (reg == 1 ? 2 : (reg == 4 || reg == 5 ? 1 : 0))),
			(reg == 5 || reg == 7), false, 0);
	case 0xda:
	case 0xde:
		if (opcode == 0xda) {
			if (!read_signed32(operand, &integer)) {
				return false;
			}
			value = raw_from_i32(integer);
		} else {
			if (!read_signed16(operand, &short_integer)) {
				return false;
			}
			value = raw_from_i32(short_integer);
		}
		if (!stack_require(state, 0) && !state_can_commit(state)) {
			return true;
		}
		if (reg == 2 || reg == 3) {
			return operation_compare(state, value, reg == 3);
		}
		return binary_stack_operation(state, value, reg == 6 ? 3 :
			(reg == 7 ? 3 : (reg == 1 ? 2 : (reg == 4 || reg == 5 ? 1 : 0))),
			opcode == 0xda ? (reg == 5 || reg == 7) :
			(reg == 4 || reg == 6),
			false, 0);
	case 0xd9:
		switch (reg) {
		case 0:
			value = raw_from_memory(state, operand, 4, &ok);
			if (!ok || !state_can_commit(state)) {
				return ok;
			}
			return stack_push(state, value);
		case 2:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			return store_raw(state, operand, 4, value);
		case 3:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			ok = store_raw(state, operand, 4, value);
			if (ok && store_can_commit(state)) {
				stack_pop(state);
			}
			return ok;
		case 4:
			if (!bus_read(operand, 0, image, 14)) {
				return false;
			}
			upd8087_load_environment(state, image);
			return true;
		case 5:
			if (!read_u16(operand, 0, &control)) {
				return false;
			}
			upd8087_set_control(state, control);
			return true;
		case 6:
			upd8087_store_environment(state, image);
			if (!bus_write(operand, 0, image, 14)) {
				return false;
			}
			/* FSTENV/FNSTENV masks all exception classes after the
			 * environment has been written, but leaves IEM unchanged.  Use
			 * the control-word path so a previously pending request is
			 * withdrawn when its exception is now masked. */
			upd8087_set_control(state, (uint16_t)(state->control | CONTROL_MASKS));
			return true;
		case 7:
			upd8087_store_environment(state, image);
			return write_u16(operand, 0, load16(image));
		default:
			return false;
		}
	case 0xdb:
		switch (reg) {
		case 0:
			if (!read_signed32(operand, &integer)) {
				return false;
			}
			value = raw_from_i32(integer);
			return state_can_commit(state) && stack_push(state, value);
		case 2:
		case 3:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			ok = store_integer(state, operand, 32, value);
			if (ok && (reg == 3) && state_can_commit(state)) {
				stack_pop(state);
			}
			return ok;
		case 5:
			value = raw_from_memory(state, operand, 10, &ok);
			if (!ok || !state_can_commit(state)) {
				return ok;
			}
			return stack_push(state, value);
		case 7:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			ok = store_raw(state, operand, 10, value);
			if (ok && store_can_commit(state)) {
				stack_pop(state);
			}
			return ok;
		default:
			return false;
		}
	case 0xdd:
		switch (reg) {
		case 0:
			value = raw_from_memory(state, operand, 8, &ok);
			if (!ok || !state_can_commit(state)) {
				return ok;
			}
			return stack_push(state, value);
		case 2:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			return store_raw(state, operand, 8, value);
		case 3:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			ok = store_raw(state, operand, 8, value);
			if (ok && store_can_commit(state)) {
				stack_pop(state);
			}
			return ok;
		case 4:
			if (!bus_read(operand, 0, image, 94)) {
				return false;
			}
			upd8087_load_state(state, image);
			return true;
		case 6:
			upd8087_store_state(state, image);
			if (!bus_write(operand, 0, image, 94)) {
				return false;
			}
			/* FSAVE/FNSAVE initializes the NDP exactly as FINIT/FNINIT
			 * after the complete image has reached memory.  Oscillator and
			 * scheduler configuration are machine state, not FPU reset state,
			 * and therefore remain unchanged by this architectural reset. */
			fpu_initialize_architecture(state);
			return true;
		case 7:
			status = upd8087_status(state);
			return write_u16(operand, 0, status);
		default:
			return false;
		}
	case 0xdf:
		switch (reg) {
		case 0:
			if (!read_signed16(operand, &short_integer)) {
				return false;
			}
			value = raw_from_i32(short_integer);
			return state_can_commit(state) && stack_push(state, value);
		case 2:
		case 3:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			ok = store_integer(state, operand, 16, value);
			if (ok && reg == 3 && state_can_commit(state)) {
				stack_pop(state);
			}
			return ok;
		case 4:
			value = packed_bcd_to_raw(state, operand, &ok);
			if (!ok || !state_can_commit(state)) {
				return ok;
			}
			return stack_push(state, value);
		case 5:
			if (!read_signed64(operand, &long_integer)) {
				return false;
			}
			value = raw_from_i64(long_integer);
			return state_can_commit(state) && stack_push(state, value);
		case 6:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			if (!raw_to_packed_bcd(state, value, bcd)) {
				return true;
			}
			if (!state_can_commit(state) || !bus_write(operand, 0, bcd, 10)) {
				return state_can_commit(state);
			}
			stack_pop(state);
			return true;
		case 7:
			if (!stack_source_value(state, &value)) {
				return true;
			}
			ok = store_integer(state, operand, 64, value);
			if (ok && state_can_commit(state)) {
				stack_pop(state);
			}
			return ok;
		default:
			return false;
		}
	default:
		return false;
	}
}

static bool execute_register_instruction(UPD8087_STATE *state, uint8_t opcode,
	uint8_t modrm) {
	unsigned reg = (modrm >> 3) & 7;
	unsigned rm = modrm & 7;
	UPD8087_RAW80 value;
	UPD8087_RAW80 result;

	switch (opcode) {
	case 0xd8:
		if (reg == 2 || reg == 3) {
			if (!stack_require(state, rm) && !state_can_commit(state)) {
				return false;
			}
			return operation_compare(state, stack_get(state, rm), reg == 3);
		}
		if (!stack_require(state, rm) && !state_can_commit(state)) {
			return false;
		}
		return binary_stack_operation(state, stack_get(state, rm), reg == 6 ? 3 :
			(reg == 7 ? 3 : (reg == 1 ? 2 : (reg == 4 || reg == 5 ? 1 : 0))),
			(reg == 5 || reg == 7), false, 0);
	case 0xd9:
		if (reg == 0) {
			bool source_valid = stack_require(state, rm);

			if (!source_valid && !state_can_commit(state)) {
				return false;
			}
			return stack_push(state, source_valid ? stack_get(state, rm) : raw_indefinite);
		}
		if (reg == 1) {
			unsigned top_index = physical_index(state, 0);
			unsigned other_index = physical_index(state, rm);
			bool top_empty = state->tag[top_index] == UPD8087_TAG_EMPTY;
			bool other_empty = state->tag[other_index] == UPD8087_TAG_EMPTY;

			if (top_empty || other_empty) {
				raise_status(state, UPD8087_STATUS_IE | UPD8087_STATUS_SF);
				state->status |= UPD8087_STATUS_C1;
				if (!state_can_commit(state)) {
					return false;
				}
				if (top_empty) {
					state->reg[top_index] = raw_indefinite;
					state->tag[top_index] = UPD8087_TAG_SPECIAL;
				}
				if (other_empty) {
					state->reg[other_index] = raw_indefinite;
					state->tag[other_index] = UPD8087_TAG_SPECIAL;
				}
			}
			value = state->reg[top_index];
			state->reg[top_index] = state->reg[other_index];
			state->reg[other_index] = value;
			/* Swapping the values also swaps their tags; do not infer a tag
			 * from the indefinite payload or from an unnormal operand. */
			{
				uint8_t tag = state->tag[top_index];
				state->tag[top_index] = state->tag[other_index];
				state->tag[other_index] = tag;
			}
			return true;
		}
		if (reg == 2 && rm == 0) {
			return true;
		}
		if (reg == 4) {
			if (!stack_require(state, 0) && !state_can_commit(state)) {
				return false;
			}
			value = stack_get(state, 0);
			switch (rm) {
			case 0:
				result = raw_neg(value);
				break;
			case 1:
				result = raw_abs(value);
				break;
			case 4:
				return operation_compare(state, raw_zero, false);
			case 5:
				fxam(state);
				return true;
			default:
				return false;
			}
			if (state_can_commit(state)) {
				stack_set(state, 0, result);
			}
			return true;
		}
		if (reg == 5) {
			static const UPD8087_RAW80 constants[] = {
				raw_one, raw_l2t, raw_l2e, raw_pi, raw_lg2, raw_ln2, raw_zero
			};
			if (rm > 6 || !state_can_commit(state)) {
				return false;
			}
			return stack_push(state, constants[rm]);
		}
		if (reg == 6) {
			if (rm <= 4) {
				return execute_transcendental(state, rm);
			}
			if (rm == 6) {
				state->top = (uint8_t)((state->top + 7) & 7);
				sync_top(state);
				return true;
			}
			if (rm == 7) {
				state->top = (uint8_t)((state->top + 1) & 7);
				sync_top(state);
				return true;
			}
			return false;
		}
		if (reg == 7) {
			switch (rm) {
			case 0:
				return execute_fprem(state);
			case 1:
				return execute_fyl2xp1(state);
			case 2:
				return execute_fsqrt(state);
			case 4:
				if (!stack_require(state, 0) && !state_can_commit(state)) {
					return false;
				}
				result = sf_round(state, stack_get(state, 0));
				if (state_can_commit(state)) {
					stack_set(state, 0, result);
				}
				return true;
			case 5:
				return execute_fscale(state);
			default:
				return false;
			}
		}
		return false;
	case 0xdb:
		if (reg != 4 || rm > 3) {
			return false;
		}
		switch (rm) {
		case 0: /* FENI */
			state->control &= (uint16_t)~CONTROL_IEM;
			set_pending_if_unmasked(state);
			return true;
		case 1: /* FDISI */
			state->control |= CONTROL_IEM;
			set_pending_if_unmasked(state);
			return true;
		case 2: /* FCLEX */
			upd8087_clear_exceptions(state);
			return true;
		default: /* FINIT */
			fpu_initialize_architecture(state);
			return true;
		}
	case 0xdc:
		if (reg == 2 || reg == 3) {
			return false;
		}
		if ((!stack_require(state, 0) && !state_can_commit(state)) ||
		    (!stack_require(state, rm) && !state_can_commit(state))) {
			return false;
		}
		return binary_stack_operation(state, stack_get(state, 0),
			reg == 6 || reg == 7 ? 3 : (reg == 4 || reg == 5 ? 1 :
			(reg == 1 ? 2 : 0)), reg == 4 || reg == 6, false, rm);
	case 0xdd:
		if (reg == 0) {
			state->tag[physical_index(state, rm)] = UPD8087_TAG_EMPTY;
			return true;
		}
		if (!stack_source_value(state, &value)) {
			return false;
		}
		if (state_can_commit(state)) {
			stack_set(state, rm, value);
			if (reg == 3) {
				stack_pop(state);
			}
		}
		return true;
	case 0xde:
		if (reg == 3 && rm == 1) {
			if ((!stack_require(state, 0) && !state_can_commit(state)) ||
			    (!stack_require(state, 1) && !state_can_commit(state))) {
				return false;
			}
			if (operation_compare(state, stack_get(state, 1), false) && state_can_commit(state)) {
				stack_pop(state);
				stack_pop(state);
			}
			return true;
		}
		if ((!stack_require(state, 0) && !state_can_commit(state)) ||
		    (!stack_require(state, rm) && !state_can_commit(state))) {
			return false;
		}
		return binary_stack_operation(state, stack_get(state, 0),
			reg == 6 || reg == 7 ? 3 :
			(reg == 1 ? 2 : (reg == 4 || reg == 5 ? 1 : 0)),
			reg == 4 || reg == 6, true, rm);
	default:
		return false;
	}
}

static unsigned memory_operand_size(uint8_t opcode, uint8_t modrm) {
	unsigned reg = (modrm >> 3) & 7;

	switch (opcode) {
	case 0xd8:
		return 4;
	case 0xd9:
		switch (reg) {
		case 0:
		case 2:
		case 3:
			return 4;
		case 4:
		case 6:
			return 14;
		case 5:
		case 7:
			return 2;
		default:
			return 0;
		}
	case 0xda:
		return 4;
	case 0xdb:
		switch (reg) {
		case 0:
		case 2:
		case 3:
			return 4;
		case 5:
		case 7:
			return 10;
		default:
			return 0;
		}
	case 0xdc:
		return 8;
	case 0xdd:
		switch (reg) {
		case 0:
		case 2:
		case 3:
			return 8;
		case 4:
		case 6:
			return 94;
		case 7:
			return 2;
		default:
			return 0;
		}
	case 0xde:
		return 2;
	case 0xdf:
		switch (reg) {
		case 0:
		case 2:
		case 3:
			return 2;
		case 4:
			return 10;
		case 5:
		case 7:
			return 8;
		default:
			return 0;
		}
	default:
		return 0;
	}
}

static bool memory_operand_writes(uint8_t opcode, uint8_t modrm) {
	unsigned reg = (modrm >> 3) & 7;

	switch (opcode) {
	case 0xd9:
		return reg == 2 || reg == 3 || reg == 6 || reg == 7;
	case 0xdb:
		return reg == 2 || reg == 3 || reg == 7;
	case 0xdd:
		return reg == 2 || reg == 3 || reg == 6 || reg == 7;
	case 0xdf:
		return reg == 2 || reg == 3 || reg == 6 || reg == 7;
	default:
		return false;
	}
}

static bool memory_operand_reads(uint8_t opcode, uint8_t modrm) {
	return !memory_operand_writes(opcode, modrm);
}

static bool memory_bus_ready(uint8_t opcode, uint8_t modrm,
	const UPD8087_OPERAND *operand) {
	unsigned size;

	if (operand == NULL) {
		return false;
	}
	if (memory_operand_writes(opcode, modrm) && operand->bus.write8 == NULL) {
		return false;
	}
	if (!memory_operand_reads(opcode, modrm)) {
		return true;
	}
	size = memory_operand_size(opcode, modrm);
	return (operand->bus.read8 != NULL) ||
	       ((size <= 2) && operand->first_word_valid);
}

UPD8087_EXEC_RESULT upd8087_execute(UPD8087_STATE *state, uint8_t opcode,
	uint8_t modrm, const UPD8087_OPERAND *operand) {
	UPD8087_SLOT_CLASS slot;
	bool ok;

	if ((state == NULL) || !state->enabled) {
		return UPD8087_EXEC_ABSENT;
	}
	slot = upd8087_classify(opcode, modrm);
	if (slot == UPD8087_SLOT_UNDEFINED) {
		return UPD8087_EXEC_UNDEFINED;
	}
	if (is_memory_form(modrm) && !memory_bus_ready(opcode, modrm, operand)) {
		return UPD8087_EXEC_BUS_ERROR;
	}
	state->working_flags = 0;
	state->busy = true;
	state->status |= UPD8087_STATUS_B;
	state->opcode = (uint16_t)(((uint16_t)(opcode & 7) << 8) | modrm);
	ok = is_memory_form(modrm) ?
		((operand != NULL) && execute_memory_instruction(state, opcode, modrm, operand)) :
		execute_register_instruction(state, opcode, modrm);
	state->busy = false;
	state->status &= (uint16_t)~UPD8087_STATUS_B;
	set_pending_if_unmasked(state);
	state->working_flags = 0;
	(void)ok;
	return UPD8087_EXECUTED;
}
