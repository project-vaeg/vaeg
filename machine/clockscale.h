/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_CLOCKSCALE_H
#define VAEG_CLOCKSCALE_H

typedef struct {
	UINT32 numerator;
	UINT32 denominator;
	UINT64 remainder;
} CLOCKSCALE;

static INLINE BOOL clockscale_configure(CLOCKSCALE *scale, UINT32 numerator, UINT32 denominator) {
	if ((scale == NULL) || (numerator == 0) || (denominator == 0)) {
		return (FAILURE);
	}
	scale->numerator = numerator;
	scale->denominator = denominator;
	scale->remainder = 0;
	return (SUCCESS);
}

static INLINE void clockscale_reset(CLOCKSCALE *scale) {
	if (scale != NULL) {
		scale->remainder = 0;
	}
}

static INLINE UINT64 clockscale_apply(CLOCKSCALE *scale, UINT32 value) {
	UINT64 total;
	UINT64 result;

	if ((scale == NULL) || (scale->denominator == 0)) {
		return (0);
	}
	total = ((UINT64)value * scale->numerator) + scale->remainder;
	result = total / scale->denominator;
	scale->remainder = total % scale->denominator;
	return (result);
}

#endif
