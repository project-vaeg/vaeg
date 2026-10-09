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

#ifndef CPU_Z80_COMPAT_STATE_H
#define CPU_Z80_COMPAT_STATE_H

#include "z80_compat_registers.h"

#include <cstddef>
#include <cstdint>

namespace vaeg::z80_compat {

constexpr std::size_t kRevision1Size = 68;
constexpr std::uint8_t kRevision1 = 1;

constexpr std::size_t kOffsetAf = 0;
constexpr std::size_t kOffsetHl = 4;
constexpr std::size_t kOffsetDe = 8;
constexpr std::size_t kOffsetBc = 12;
constexpr std::size_t kOffsetIx = 16;
constexpr std::size_t kOffsetIy = 20;
constexpr std::size_t kOffsetSp = 24;
constexpr std::size_t kOffsetAlternateAf = 28;
constexpr std::size_t kOffsetAlternateHl = 32;
constexpr std::size_t kOffsetAlternateDe = 36;
constexpr std::size_t kOffsetAlternateBc = 40;
constexpr std::size_t kOffsetPc = 44;
constexpr std::size_t kOffsetI = 48;
constexpr std::size_t kOffsetRLow = 49;
constexpr std::size_t kOffsetRBit7 = 50;
constexpr std::size_t kOffsetIm = 51;
constexpr std::size_t kOffsetIff1 = 52;
constexpr std::size_t kOffsetIff2 = 53;
constexpr std::size_t kOffsetIrq = 56;
constexpr std::size_t kOffsetWait = 57;
constexpr std::size_t kOffsetXf = 58;
constexpr std::size_t kOffsetRevision = 59;
constexpr std::size_t kOffsetRemainClock = 60;
constexpr std::size_t kOffsetLastClock = 64;

constexpr std::uint8_t kWaitHalt = 0x01;
constexpr std::uint8_t kWaitExternal = 0x02;
constexpr std::uint8_t kWaitEiInhibited = 0x04;

struct LegacyState {
	Z80CompatReg registers{};
	bool halted = false;
	bool external_wait = false;
	bool irq_asserted = false;
	bool ei_inhibited = false;
	std::int32_t remainclock = 0;
	std::int32_t lastclock = 0;
};

bool DecodeRevision1(const std::uint8_t *image, LegacyState *state);
void EncodeRevision1(const LegacyState &state, std::uint8_t *image);

} // namespace vaeg::z80_compat

#endif
