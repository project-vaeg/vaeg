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

#include "z80_compat_state.h"

#include <climits>
#include <cstring>
#include <limits>

namespace vaeg::z80_compat {
namespace {

static_assert(CHAR_BIT == 8, "revision-1 requires eight-bit bytes");
static_assert(sizeof(std::uint16_t) == 2, "revision-1 requires 16-bit register values");
static_assert(sizeof(std::uint32_t) == 4, "revision-1 requires 32-bit legacy words");
static_assert(sizeof(std::int32_t) == 4, "revision-1 requires 32-bit signed clocks");
static_assert(sizeof(bool) == 1, "current revision-1 build family requires one-byte bool");
static_assert(kOffsetLastClock + 4 == kRevision1Size,
              "revision-1 offsets must cover exactly 68 bytes");

std::uint16_t ReadWord(const std::uint8_t *image, std::size_t offset) {
	return static_cast<std::uint16_t>(image[offset]) |
	       static_cast<std::uint16_t>(image[offset + 1] << 8);
}

std::int32_t ReadSigned(const std::uint8_t *image, std::size_t offset) {
	const std::uint32_t value = static_cast<std::uint32_t>(image[offset]) |
	                            (static_cast<std::uint32_t>(image[offset + 1]) << 8) |
	                            (static_cast<std::uint32_t>(image[offset + 2]) << 16) |
	                            (static_cast<std::uint32_t>(image[offset + 3]) << 24);
	if (value <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
		return static_cast<std::int32_t>(value);
	}
	return static_cast<std::int32_t>(static_cast<std::int64_t>(value) - (INT64_C(1) << 32));
}

void WriteWord(std::uint8_t *image, std::size_t offset, std::uint16_t value) {
	image[offset] = static_cast<std::uint8_t>(value);
	image[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void WriteSigned(std::uint8_t *image, std::size_t offset, std::int32_t value) {
	const std::uint32_t encoded = static_cast<std::uint32_t>(value);
	image[offset] = static_cast<std::uint8_t>(encoded);
	image[offset + 1] = static_cast<std::uint8_t>(encoded >> 8);
	image[offset + 2] = static_cast<std::uint8_t>(encoded >> 16);
	image[offset + 3] = static_cast<std::uint8_t>(encoded >> 24);
}

} // namespace

bool DecodeRevision1(const std::uint8_t *image, LegacyState *state) {
	if (image == nullptr || state == nullptr || image[kOffsetRevision] != kRevision1) {
		return false;
	}

	LegacyState decoded;
	decoded.registers.af = ReadWord(image, kOffsetAf);
	decoded.registers.hl = ReadWord(image, kOffsetHl);
	decoded.registers.de = ReadWord(image, kOffsetDe);
	decoded.registers.bc = ReadWord(image, kOffsetBc);
	decoded.registers.ix = ReadWord(image, kOffsetIx);
	decoded.registers.iy = ReadWord(image, kOffsetIy);
	decoded.registers.sp = ReadWord(image, kOffsetSp);
	decoded.registers.r_af = ReadWord(image, kOffsetAlternateAf);
	decoded.registers.r_hl = ReadWord(image, kOffsetAlternateHl);
	decoded.registers.r_de = ReadWord(image, kOffsetAlternateDe);
	decoded.registers.r_bc = ReadWord(image, kOffsetAlternateBc);
	decoded.registers.pc = ReadWord(image, kOffsetPc);
	decoded.registers.ireg = image[kOffsetI];
	decoded.registers.rreg = image[kOffsetRLow] & 0x7f;
	decoded.registers.rreg7 = image[kOffsetRBit7] & 0x80;
	decoded.registers.intmode = image[kOffsetIm] & 0x03;
	decoded.registers.iff1 = image[kOffsetIff1] != 0;
	decoded.registers.iff2 = image[kOffsetIff2] != 0;
	decoded.irq_asserted = image[kOffsetIrq] != 0;
	decoded.halted = (image[kOffsetWait] & kWaitHalt) != 0;
	decoded.external_wait = (image[kOffsetWait] & kWaitExternal) != 0;
	decoded.ei_inhibited = (image[kOffsetWait] & kWaitEiInhibited) != 0;
	decoded.remainclock = ReadSigned(image, kOffsetRemainClock);
	decoded.lastclock = ReadSigned(image, kOffsetLastClock);

	if (decoded.halted) {
		decoded.registers.pc = static_cast<std::uint16_t>(decoded.registers.pc + 1);
	}

	*state = decoded;
	return true;
}

void EncodeRevision1(const LegacyState &state, std::uint8_t *image) {
	if (image == nullptr) {
		return;
	}
	std::memset(image, 0, kRevision1Size);

	WriteWord(image, kOffsetAf, state.registers.af);
	WriteWord(image, kOffsetHl, state.registers.hl);
	WriteWord(image, kOffsetDe, state.registers.de);
	WriteWord(image, kOffsetBc, state.registers.bc);
	WriteWord(image, kOffsetIx, state.registers.ix);
	WriteWord(image, kOffsetIy, state.registers.iy);
	WriteWord(image, kOffsetSp, state.registers.sp);
	WriteWord(image, kOffsetAlternateAf, state.registers.r_af);
	WriteWord(image, kOffsetAlternateHl, state.registers.r_hl);
	WriteWord(image, kOffsetAlternateDe, state.registers.r_de);
	WriteWord(image, kOffsetAlternateBc, state.registers.r_bc);
	std::uint16_t legacy_pc = state.registers.pc;
	if (state.halted) {
		legacy_pc = static_cast<std::uint16_t>(legacy_pc - 1);
	}
	WriteWord(image, kOffsetPc, legacy_pc);
	image[kOffsetI] = state.registers.ireg;
	image[kOffsetRLow] = state.registers.rreg & 0x7f;
	image[kOffsetRBit7] = state.registers.rreg7 & 0x80;
	image[kOffsetIm] = state.registers.intmode & 0x03;
	image[kOffsetIff1] = state.registers.iff1 ? 1 : 0;
	image[kOffsetIff2] = state.registers.iff2 ? 1 : 0;
	image[kOffsetIrq] = state.irq_asserted ? 1 : 0;
	image[kOffsetWait] = static_cast<std::uint8_t>((state.halted ? kWaitHalt : 0) |
	                                               (state.external_wait ? kWaitExternal : 0) |
	                                               (state.ei_inhibited ? kWaitEiInhibited : 0));
	image[kOffsetXf] = static_cast<std::uint8_t>(state.registers.af) & 0x28;
	image[kOffsetRevision] = kRevision1;
	WriteSigned(image, kOffsetRemainClock, state.remainclock);
	WriteSigned(image, kOffsetLastClock, state.lastclock);
}

} // namespace vaeg::z80_compat
