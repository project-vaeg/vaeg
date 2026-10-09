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

/*
 * This is a developer-only interval certificate tool.  It is deliberately
 * built outside the VAEG target and must never be linked into the emulator.
 * Every input is constructed from exact integers and powers of two; no host
 * floating-point value or libm operation participates in the certificate.
 */

#include <gmp.h>
#include <mpfr.h>

#include <stdio.h>
#include <string.h>

#define CERTIFICATE_PRECISION 192
#define REFERENCE_PRECISION 512

typedef int (*unary_operation)(mpfr_ptr, mpfr_srcptr, mpfr_rnd_t);

static int fail(const char *name, const char *reason) {
	fprintf(stderr, "P13_MPFR_ORACLE FAIL %s: %s\n", name, reason);
	return 1;
}

static void set_ratio(mpfr_t value, unsigned numerator, unsigned shift) {
	mpfr_set_ui(value, numerator, MPFR_RNDN);
	mpfr_div_2ui(value, value, shift, MPFR_RNDN);
}

static int certify(const char *name, unary_operation operation, const mpfr_t input) {
	mpfr_t lower;
	mpfr_t upper;
	mpfr_t reference_lower;
	mpfr_t reference_upper;
	int result;

	mpfr_init2(lower, CERTIFICATE_PRECISION);
	mpfr_init2(upper, CERTIFICATE_PRECISION);
	mpfr_init2(reference_lower, REFERENCE_PRECISION);
	mpfr_init2(reference_upper, REFERENCE_PRECISION);
	operation(lower, input, MPFR_RNDD);
	operation(upper, input, MPFR_RNDU);
	operation(reference_lower, input, MPFR_RNDD);
	operation(reference_upper, input, MPFR_RNDU);
	result = mpfr_cmp(lower, reference_lower) <= 0 &&
	         mpfr_cmp(reference_lower, reference_upper) <= 0 &&
	         mpfr_cmp(reference_upper, upper) <= 0;
	if (!result) {
		mpfr_clear(lower);
		mpfr_clear(upper);
		mpfr_clear(reference_lower);
		mpfr_clear(reference_upper);
		return fail(name, "directed interval did not enclose reference");
	}
	mpfr_clear(lower);
	mpfr_clear(upper);
	mpfr_clear(reference_lower);
	mpfr_clear(reference_upper);
	return 0;
}

static int certify_rejection(const mpfr_t input) {
	mpfr_t reference_lower;
	mpfr_t reference_upper;
	mpfr_t insufficient_lower;
	int result;

	mpfr_init2(reference_lower, REFERENCE_PRECISION);
	mpfr_init2(reference_upper, REFERENCE_PRECISION);
	mpfr_init2(insufficient_lower, CERTIFICATE_PRECISION);
	mpfr_sin(reference_lower, input, MPFR_RNDD);
	mpfr_sin(reference_upper, input, MPFR_RNDU);
	/* A lower endpoint rounded upward is intentionally not a certificate. */
	mpfr_set(insufficient_lower, reference_upper, MPFR_RNDD);
	mpfr_nextabove(insufficient_lower);
	result = mpfr_cmp(insufficient_lower, reference_lower) > 0;
	mpfr_clear(reference_lower);
	mpfr_clear(reference_upper);
	mpfr_clear(insufficient_lower);
	return result ? 0 : fail("insufficient", "bad certificate was accepted");
}

int main(int argc, char **argv) {
	mpfr_t input;
	mpfr_t pi;
	mpfr_t root_two;
	mpfr_t boundary;
	unsigned cases = 0;

	if (argc != 2 || strcmp(argv[1], "--selftest") != 0) {
		fprintf(stderr, "usage: %s --selftest\n", argv[0]);
		return 2;
	}
	mpfr_init2(input, REFERENCE_PRECISION);
	mpfr_init2(pi, REFERENCE_PRECISION);
	mpfr_init2(root_two, REFERENCE_PRECISION);
	mpfr_init2(boundary, REFERENCE_PRECISION);

	set_ratio(input, 1, 3);
	if (certify("exp2(1/8)", mpfr_exp2, input) != 0) {
		return 1;
	}
	cases++;
	set_ratio(input, 5, 2);
	if (certify("log2(5/4)", mpfr_log2, input) != 0) {
		return 1;
	}
	cases++;
	set_ratio(input, 1, 3);
	if (certify("log1p(1/8)", mpfr_log1p, input) != 0) {
		return 1;
	}
	cases++;
	set_ratio(input, 1, 2);
	if (certify("sin(1/2)", mpfr_sin, input) != 0 ||
	    certify("cos(1/2)", mpfr_cos, input) != 0 ||
	    certify("atan(1/2)", mpfr_atan, input) != 0) {
		return 1;
	}
	cases += 3;
	if (certify_rejection(input) != 0) {
		return 1;
	}
	mpfr_const_pi(pi, MPFR_RNDN);
	mpfr_div_2ui(input, pi, 2, MPFR_RNDN);
	if (certify("sin(pi/4)", mpfr_sin, input) != 0 ||
	    certify("cos(pi/4)", mpfr_cos, input) != 0) {
		return 1;
	}
	cases += 2;
	mpfr_set_ui(root_two, 2, MPFR_RNDN);
	mpfr_sqrt(root_two, root_two, MPFR_RNDN);
	mpfr_div_2ui(root_two, root_two, 1, MPFR_RNDN);
	mpfr_ui_sub(boundary, 1, root_two, MPFR_RNDN);
	if (mpfr_cmp_ui(boundary, 0) <= 0 ||
	    certify("log1p(1-sqrt(2)/2)", mpfr_log1p, boundary) != 0) {
		return 1;
	}
	cases++;
	mpfr_clear(input);
	mpfr_clear(pi);
	mpfr_clear(root_two);
	mpfr_clear(boundary);
	printf("P13_MPFR_ORACLE PASS cases=%u precision=%u/%u\n", cases,
	       CERTIFICATE_PRECISION, REFERENCE_PRECISION);
	return 0;
}
