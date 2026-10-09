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
#include "cpucore.h"
#include "hostfat.h"

enum {
	/* PC-Engine non-IBM block header: 13 bytes, then command fields. */
	HOSTFAT_PACKET_SIZE = 22,
	HOSTFAT_PACKET_UNIT = 1,
	HOSTFAT_PACKET_COMMAND = 2,
	HOSTFAT_PACKET_TRANSFER = 14,
	HOSTFAT_PACKET_COUNT = 18,
	HOSTFAT_PACKET_START = 20,
	HOSTFAT_COMMAND_READ = 4,
	HOSTFAT_MAX_REQUEST_SECTORS = 128
};

typedef struct {
	BYTE *image;
	UINT32 digest;
	BYTE identity[HOSTFAT_IDENTITY_SIZE];
} HOSTFAT_STATE;

struct hostfat_prepared_image {
	BYTE *image;
	UINT32 digest;
	BYTE identity[HOSTFAT_IDENTITY_SIZE];
};

typedef struct {
	UINT32 state[8];
	UINT64 bit_count;
	BYTE block[64];
	UINT block_size;
} HOSTFAT_SHA256;

static HOSTFAT_STATE hostfat_state;

static const UINT32 sha256_round_constants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
    0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
    0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
    0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
    0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
    0xc67178f2U};

static UINT32 rotate_right(UINT32 value, UINT count) {
	return ((value >> count) | (value << (32 - count)));
}

static void sha256_transform(HOSTFAT_SHA256 *sha, const BYTE *block) {
	UINT32 words[64];
	UINT32 a;
	UINT32 b;
	UINT32 c;
	UINT32 d;
	UINT32 e;
	UINT32 f;
	UINT32 g;
	UINT32 h;
	UINT32 first;
	UINT32 second;
	UINT index;

	for (index = 0; index < 16; index++) {
		words[index] = ((UINT32)block[index * 4] << 24) | ((UINT32)block[index * 4 + 1] << 16) |
		               ((UINT32)block[index * 4 + 2] << 8) | block[index * 4 + 3];
	}
	for (index = 16; index < 64; index++) {
		first = rotate_right(words[index - 15], 7) ^ rotate_right(words[index - 15], 18) ^
		        (words[index - 15] >> 3);
		second = rotate_right(words[index - 2], 17) ^ rotate_right(words[index - 2], 19) ^
		         (words[index - 2] >> 10);
		words[index] = words[index - 16] + first + words[index - 7] + second;
	}
	a = sha->state[0];
	b = sha->state[1];
	c = sha->state[2];
	d = sha->state[3];
	e = sha->state[4];
	f = sha->state[5];
	g = sha->state[6];
	h = sha->state[7];
	for (index = 0; index < 64; index++) {
		first = h + (rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25)) +
		        ((e & f) ^ ((~e) & g)) + sha256_round_constants[index] + words[index];
		second = (rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22)) +
		         ((a & b) ^ (a & c) ^ (b & c));
		h = g;
		g = f;
		f = e;
		e = d + first;
		d = c;
		c = b;
		b = a;
		a = first + second;
	}
	sha->state[0] += a;
	sha->state[1] += b;
	sha->state[2] += c;
	sha->state[3] += d;
	sha->state[4] += e;
	sha->state[5] += f;
	sha->state[6] += g;
	sha->state[7] += h;
}

static void sha256_initialize(HOSTFAT_SHA256 *sha) {
	static const UINT32 initial[8] = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
	                                  0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};

	ZeroMemory(sha, sizeof(*sha));
	CopyMemory(sha->state, initial, sizeof(initial));
}

static void sha256_update(HOSTFAT_SHA256 *sha, const BYTE *data, UINT32 size) {
	UINT amount;

	sha->bit_count += (UINT64)size * 8;
	while (size != 0) {
		amount = 64 - sha->block_size;
		if (amount > size) {
			amount = size;
		}
		CopyMemory(sha->block + sha->block_size, data, amount);
		sha->block_size += amount;
		data += amount;
		size -= amount;
		if (sha->block_size == 64) {
			sha256_transform(sha, sha->block);
			sha->block_size = 0;
		}
	}
}

static void sha256_finish(HOSTFAT_SHA256 *sha, BYTE identity[HOSTFAT_IDENTITY_SIZE]) {
	UINT64 bit_count;
	UINT index;

	bit_count = sha->bit_count;
	sha->block[sha->block_size++] = 0x80;
	if (sha->block_size > 56) {
		ZeroMemory(sha->block + sha->block_size, 64 - sha->block_size);
		sha256_transform(sha, sha->block);
		sha->block_size = 0;
	}
	ZeroMemory(sha->block + sha->block_size, 56 - sha->block_size);
	for (index = 0; index < 8; index++) {
		sha->block[63 - index] = (BYTE)(bit_count >> (index * 8));
	}
	sha256_transform(sha, sha->block);
	for (index = 0; index < 8; index++) {
		identity[index * 4] = (BYTE)(sha->state[index] >> 24);
		identity[index * 4 + 1] = (BYTE)(sha->state[index] >> 16);
		identity[index * 4 + 2] = (BYTE)(sha->state[index] >> 8);
		identity[index * 4 + 3] = (BYTE)sha->state[index];
	}
}

static UINT16 load_word(const BYTE *source) {
	return ((UINT16)(source[0] | ((UINT16)source[1] << 8)));
}

static UINT32 digest_image(const BYTE *image, UINT32 size) {
	UINT32 digest;
	UINT32 position;

	digest = 2166136261U;
	for (position = 0; position < size; position++) {
		digest ^= image[position];
		digest *= 16777619U;
	}
	return (digest);
}

static void identify_image(const BYTE *image, UINT32 size, BYTE identity[HOSTFAT_IDENTITY_SIZE]) {
	HOSTFAT_SHA256 sha;

	sha256_initialize(&sha);
	sha256_update(&sha, image, size);
	sha256_finish(&sha, identity);
}

static BOOL sha256_selftest(void) {
	static const BYTE expected[HOSTFAT_IDENTITY_SIZE] = {
	    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
	    0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
	    0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
	BYTE actual[HOSTFAT_IDENTITY_SIZE];

	identify_image((const BYTE *)"abc", 3, actual);
	return (memcmp(actual, expected, sizeof(expected)) ? FAILURE : SUCCESS);
}

BOOL hostfat_mount_image(const void *image, UINT32 size) {
	HOSTFAT_PREPARED_IMAGE *prepared;
	BOOL result;

	if (hostfat_prepare_image(image, size, &prepared, NULL) != SUCCESS) {
		return (FAILURE);
	}
	result = hostfat_commit_prepared_image(prepared);
	hostfat_destroy_prepared_image(prepared);
	return (result);
}

BOOL hostfat_prepare_image(const void *image, UINT32 size, HOSTFAT_PREPARED_IMAGE **prepared,
                           UINT32 *digest) {
	HOSTFAT_PREPARED_IMAGE *replacement;

	if (prepared != NULL) {
		*prepared = NULL;
	}
	if ((image == NULL) || (size != HOSTFAT_IMAGE_SIZE) || (prepared == NULL) ||
	    (sha256_selftest() != SUCCESS)) {
		return (FAILURE);
	}
	replacement = (HOSTFAT_PREPARED_IMAGE *)_MALLOC(sizeof(*replacement), "hostfat-prepared");
	if (replacement == NULL) {
		return (FAILURE);
	}
	ZeroMemory(replacement, sizeof(*replacement));
	replacement->image = (BYTE *)_MALLOC(size, "hostfat-image");
	if (replacement->image == NULL) {
		_MFREE(replacement);
		return (FAILURE);
	}
	CopyMemory(replacement->image, image, size);
	replacement->digest = digest_image(replacement->image, size);
	identify_image(replacement->image, size, replacement->identity);
	if (digest != NULL) {
		*digest = replacement->digest;
	}
	*prepared = replacement;
	return (SUCCESS);
}

BOOL hostfat_commit_prepared_image(HOSTFAT_PREPARED_IMAGE *prepared) {
	BYTE *previous;

	if ((prepared == NULL) || (prepared->image == NULL)) {
		return (FAILURE);
	}
	previous = hostfat_state.image;
	hostfat_state.image = prepared->image;
	hostfat_state.digest = prepared->digest;
	CopyMemory(hostfat_state.identity, prepared->identity, HOSTFAT_IDENTITY_SIZE);
	prepared->image = NULL;
	if (previous != NULL) {
		_MFREE(previous);
	}
	return (SUCCESS);
}

void hostfat_destroy_prepared_image(HOSTFAT_PREPARED_IMAGE *prepared) {
	if (prepared != NULL) {
		if (prepared->image != NULL) {
			_MFREE(prepared->image);
		}
		_MFREE(prepared);
	}
}

void hostfat_unmount(void) {
	if (hostfat_state.image != NULL) {
		_MFREE(hostfat_state.image);
	}
	ZeroMemory(&hostfat_state, sizeof(hostfat_state));
}

BOOL hostfat_is_mounted(void) {
	return (hostfat_state.image != NULL);
}

UINT32 hostfat_image_digest(void) {
	return (hostfat_state.digest);
}

BOOL hostfat_snapshot_identity(void *identity, UINT size) {
	if ((hostfat_state.image == NULL) || (identity == NULL) || (size != HOSTFAT_IDENTITY_SIZE)) {
		return (FAILURE);
	}
	CopyMemory(identity, hostfat_state.identity, HOSTFAT_IDENTITY_SIZE);
	return (SUCCESS);
}

BOOL hostfat_read_sector(UINT32 sector, void *destination) {
	if ((hostfat_state.image == NULL) || (destination == NULL) ||
	    (sector >= HOSTFAT_TOTAL_SECTORS)) {
		return (FAILURE);
	}
	CopyMemory(destination, hostfat_state.image + (sector * HOSTFAT_SECTOR_SIZE),
	           HOSTFAT_SECTOR_SIZE);
	return (SUCCESS);
}

UINT8 hostfat_service_request(UINT32 request_far_pointer) {
	BYTE packet[HOSTFAT_PACKET_SIZE];
	UINT request_offset;
	UINT request_segment;
	UINT transfer_offset;
	UINT transfer_segment;
	UINT16 count;
	UINT16 start;
	UINT32 request_address;
	UINT32 transfer_address;
	UINT32 transfer_size;
	UINT32 image_offset;
	UINT32 position;

	if (hostfat_state.image == NULL) {
		return (HOSTFAT_RESULT_NOT_MOUNTED);
	}
	request_offset = LOW16(request_far_pointer);
	request_segment = (UINT)(request_far_pointer >> 16);
	request_address = ((UINT32)request_segment << 4) + request_offset;
	if ((request_offset > (0x10000U - HOSTFAT_PACKET_SIZE)) ||
	    (request_address >= UPD9002_MAINRAM_LIMIT) ||
	    (HOSTFAT_PACKET_SIZE > (UPD9002_MAINRAM_LIMIT - request_address))) {
		return (HOSTFAT_RESULT_BAD_REQUEST);
	}
	for (position = 0; position < sizeof(packet); position++) {
		packet[position] = upd9002_memoryread(request_address + position);
	}
	if ((packet[0] < HOSTFAT_PACKET_SIZE) || (packet[HOSTFAT_PACKET_UNIT] != 0) ||
	    (packet[HOSTFAT_PACKET_COMMAND] != HOSTFAT_COMMAND_READ)) {
		return (HOSTFAT_RESULT_BAD_REQUEST);
	}
	count = load_word(packet + HOSTFAT_PACKET_COUNT);
	start = load_word(packet + HOSTFAT_PACKET_START);
	if (count == 0) {
		return (HOSTFAT_RESULT_OK);
	}
	if ((count > HOSTFAT_MAX_REQUEST_SECTORS) || (start >= HOSTFAT_TOTAL_SECTORS) ||
	    ((UINT32)count > (HOSTFAT_TOTAL_SECTORS - start))) {
		return (HOSTFAT_RESULT_RANGE);
	}
	transfer_offset = load_word(packet + HOSTFAT_PACKET_TRANSFER);
	transfer_segment = load_word(packet + HOSTFAT_PACKET_TRANSFER + 2);
	transfer_address = ((UINT32)transfer_segment << 4) + transfer_offset;
	transfer_size = (UINT32)count * HOSTFAT_SECTOR_SIZE;
	if ((transfer_address >= UPD9002_MAINRAM_LIMIT) ||
	    (transfer_size > (UPD9002_MAINRAM_LIMIT - transfer_address))) {
		return (HOSTFAT_RESULT_RANGE);
	}

	image_offset = (UINT32)start * HOSTFAT_SECTOR_SIZE;
	for (position = 0; position < transfer_size; position++) {
		upd9002_memorywrite(transfer_address + position,
		                    hostfat_state.image[image_offset + position]);
	}
	return (HOSTFAT_RESULT_OK);
}
