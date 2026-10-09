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
#include "compiler.h"
#include "dosio.h"
#include "romcheck.h"

typedef struct {
	UINT32 state[5];
	UINT64 total;
	UINT used;
	BYTE block[64];
} SHA1CTX;

static UINT32 rotate_left(UINT32 value, UINT bits) {
	return ((value << bits) | (value >> (32 - bits)));
}

static void sha1_transform(SHA1CTX *ctx, const BYTE block[64]) {
	UINT32 w[80];
	UINT32 a;
	UINT32 b;
	UINT32 c;
	UINT32 d;
	UINT32 e;
	UINT32 f;
	UINT32 k;
	UINT32 temp;
	UINT i;

	for (i = 0; i < 16; i++) {
		w[i] = ((UINT32)block[i * 4] << 24) | ((UINT32)block[i * 4 + 1] << 16) |
		       ((UINT32)block[i * 4 + 2] << 8) | (UINT32)block[i * 4 + 3];
	}
	for (i = 16; i < 80; i++) {
		w[i] = rotate_left(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
	}

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	for (i = 0; i < 80; i++) {
		if (i < 20) {
			f = (b & c) | ((~b) & d);
			k = 0x5a827999;
		} else if (i < 40) {
			f = b ^ c ^ d;
			k = 0x6ed9eba1;
		} else if (i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8f1bbcdc;
		} else {
			f = b ^ c ^ d;
			k = 0xca62c1d6;
		}
		temp = rotate_left(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = rotate_left(b, 30);
		b = a;
		a = temp;
	}
	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
}

static void sha1_init(SHA1CTX *ctx) {
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xefcdab89;
	ctx->state[2] = 0x98badcfe;
	ctx->state[3] = 0x10325476;
	ctx->state[4] = 0xc3d2e1f0;
	ctx->total = 0;
	ctx->used = 0;
}

static void sha1_update(SHA1CTX *ctx, const BYTE *data, UINT size) {
	UINT take;

	ctx->total += size;
	while (size != 0) {
		take = 64 - ctx->used;
		if (take > size) {
			take = size;
		}
		CopyMemory(ctx->block + ctx->used, data, take);
		ctx->used += take;
		data += take;
		size -= take;
		if (ctx->used == 64) {
			sha1_transform(ctx, ctx->block);
			ctx->used = 0;
		}
	}
}

static void sha1_final(SHA1CTX *ctx, BYTE digest[20]) {
	UINT64 bits;
	BYTE padding[64];
	BYTE length[8];
	UINT pad_size;
	UINT i;

	bits = ctx->total * 8;
	ZeroMemory(padding, sizeof(padding));
	padding[0] = 0x80;
	pad_size = (ctx->used < 56) ? (56 - ctx->used) : (120 - ctx->used);
	sha1_update(ctx, padding, pad_size);
	for (i = 0; i < 8; i++) {
		length[7 - i] = (BYTE)(bits >> (i * 8));
	}
	sha1_update(ctx, length, sizeof(length));
	for (i = 0; i < 5; i++) {
		digest[i * 4] = (BYTE)(ctx->state[i] >> 24);
		digest[i * 4 + 1] = (BYTE)(ctx->state[i] >> 16);
		digest[i * 4 + 2] = (BYTE)(ctx->state[i] >> 8);
		digest[i * 4 + 3] = (BYTE)ctx->state[i];
	}
}

static UINT32 crc32_update(UINT32 crc, const BYTE *data, UINT size) {
	UINT32 value;
	UINT i;
	UINT bit;

	for (i = 0; i < size; i++) {
		value = crc ^ data[i];
		for (bit = 0; bit < 8; bit++) {
			value = (value >> 1) ^ ((value & 1) ? 0xedb88320 : 0);
		}
		crc = value;
	}
	return (crc);
}

void romcheck_buffer(const void *data, UINT size, ROMCHECKSUM *result) {
	SHA1CTX sha1;

	sha1_init(&sha1);
	sha1_update(&sha1, (const BYTE *)data, size);
	sha1_final(&sha1, result->sha1);
	result->size = size;
	result->crc32 = crc32_update(0xffffffff, (const BYTE *)data, size) ^ 0xffffffff;
}

BOOL romcheck_file(const char *path, ROMCHECKSUM *result) {
	BYTE buffer[8192];
	FILEH fh;
	SHA1CTX sha1;
	UINT32 crc;
	UINT total;
	UINT expected;
	UINT read_size;

	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		return (FAILURE);
	}
	expected = file_getsize(fh);
	sha1_init(&sha1);
	crc = 0xffffffff;
	total = 0;
	while (total < expected) {
		read_size = expected - total;
		if (read_size > sizeof(buffer)) {
			read_size = sizeof(buffer);
		}
		read_size = file_read(fh, buffer, read_size);
		if (read_size == 0) {
			file_close(fh);
			return (FAILURE);
		}
		sha1_update(&sha1, buffer, read_size);
		crc = crc32_update(crc, buffer, read_size);
		total += read_size;
	}
	file_close(fh);
	sha1_final(&sha1, result->sha1);
	result->size = total;
	result->crc32 = crc ^ 0xffffffff;
	return (SUCCESS);
}

void romcheck_sha1_string(const BYTE sha1[20], char text[41]) {
	static const char hex[] = "0123456789abcdef";
	UINT i;

	for (i = 0; i < 20; i++) {
		text[i * 2] = hex[sha1[i] >> 4];
		text[i * 2 + 1] = hex[sha1[i] & 0x0f];
	}
	text[40] = '\0';
}
