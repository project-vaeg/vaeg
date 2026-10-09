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

#include "cpu/z80_compat_cpu.h"
#include "cpu/z80_compat_state.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

using Status = std::array<std::uint8_t, vaeg::z80_compat::kRevision1Size>;

struct Fixture {
	const char *name;
	const char *hex;
};

constexpr Fixture kFixtures[] = {
    {"ordinary",
     "20330000bc9a0000785600003412000000000000000000000090000000000000000000000000000000000000110000000007000000000000000033010000000074000000"},
    {"alternate",
     "0000000000000000000000000000000057130000682400000090000045de0000bc9a000078560000341200001a000000000d0000000000000000000100000000b6000000"},
    {"im0-iff-enabled",
     "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000050000000005000001010000000000010000000028000000"},
    {"im1-iff-enabled",
     "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000050000000005000101010000000000010000000028000000"},
    {"im2-iff-enabled",
     "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000050000000005000201010000000000010000000028000000"},
    {"iff2-only",
     "440000000000000000000000000000000000000000000000fe8f00000000000000000000000000000000000066000000000400000001000000000001eaffffff2c000000"},
    {"halt",
     "440000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000200000000000000010001f8ffffff10000000"},
    {"external-wait",
     "440000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000100000000000000020001f3ffffff15000000"},
    {"asserted-irq-after-exec",
     "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000002000000000000010000010000000010000000"},
    {"negative-remainclock",
     "440000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000200000000000000000001f9ffffff09000000"},
    {"nonzero-lastclock",
     "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000002000000000000000000010000000010000000"},
    {"ei-nop",
     "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000030000000003000001010000000000010000000018000000"},
    {"ei-nop-accepted-irq",
     "440000000000000000000000000000000000000000000000000000000000000000000000000000000000000003000000000400000000000001000001deffffff18000000"},
    {"ei-di",
     "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000002000000000000010000010000000010000000"},
    {"ei-ei",
     "4400000000000000000000000000000000000000000000000000000000000000000000000000000000000000020000000002000000000000010000010000000010000000"},
};

[[noreturn]] void Fail(const std::string &message) {
	std::fprintf(stderr, "legacy-contract: %s\n", message.c_str());
	std::exit(1);
}

std::uint8_t HexDigit(char digit) {
	if (digit >= '0' && digit <= '9') {
		return static_cast<std::uint8_t>(digit - '0');
	}
	if (digit >= 'a' && digit <= 'f') {
		return static_cast<std::uint8_t>(digit - 'a' + 10);
	}
	Fail("invalid retained fixture hex digit");
}

Status Parse(const char *hex) {
	const std::string text(hex);
	if (text.size() != vaeg::z80_compat::kRevision1Size * 2) {
		Fail("retained fixture has an invalid encoded length");
	}
	Status status{};
	for (std::size_t index = 0; index < status.size(); ++index) {
		status[index] = static_cast<std::uint8_t>((HexDigit(text[index * 2]) << 4) |
		                                          HexDigit(text[index * 2 + 1]));
	}
	return status;
}

bool SameRegisters(const Z80CompatReg &left, const Z80CompatReg &right) {
	return left.af == right.af && left.hl == right.hl && left.de == right.de &&
	       left.bc == right.bc && left.ix == right.ix && left.iy == right.iy &&
	       left.sp == right.sp && left.r_af == right.r_af && left.r_hl == right.r_hl &&
	       left.r_de == right.r_de && left.r_bc == right.r_bc && left.pc == right.pc &&
	       left.ireg == right.ireg && left.rreg == right.rreg && left.rreg7 == right.rreg7 &&
	       left.intmode == right.intmode && left.iff1 == right.iff1 && left.iff2 == right.iff2;
}

bool SameState(const vaeg::z80_compat::LegacyState &left,
               const vaeg::z80_compat::LegacyState &right) {
	return SameRegisters(left.registers, right.registers) && left.halted == right.halted &&
	       left.external_wait == right.external_wait && left.irq_asserted == right.irq_asserted &&
	       left.ei_inhibited == right.ei_inhibited && left.remainclock == right.remainclock &&
	       left.lastclock == right.lastclock;
}

class Memory final : public IMemoryAccess {
  public:
	std::uint32_t IFCALL Read8(std::uint32_t) override {
		return 0;
	}
	void IFCALL Write8(std::uint32_t, std::uint32_t) override {
	}
};

class IO final : public IIOAccess {
  public:
	std::uint32_t IFCALL In(std::uint32_t) override {
		return 0xff;
	}
	void IFCALL Out(std::uint32_t, std::uint32_t) override {
	}
};

class Clock final : public IClock {
  public:
	std::uint32_t IFCALL now() override {
		return 0;
	}
};

class ClockCounter final : public IClockCounter {
  public:
	void IFCALL past(std::int32_t clocks) override {
		remain_ -= clocks;
	}
	std::int32_t IFCALL GetRemainclock() override {
		return remain_;
	}
	void IFCALL SetRemainclock(std::int32_t clocks) override {
		remain_ = clocks;
	}

  private:
	std::int32_t remain_ = 0;
};

void VerifyFixture(const Fixture &fixture) {
	const Status input = Parse(fixture.hex);
	if ((input[vaeg::z80_compat::kOffsetWait] & vaeg::z80_compat::kWaitEiInhibited) != 0) {
		Fail(std::string(fixture.name) + " unexpectedly uses reserved revision-1 wait bit 2");
	}

	vaeg::z80_compat::LegacyState decoded;
	if (!vaeg::z80_compat::DecodeRevision1(input.data(), &decoded)) {
		Fail(std::string(fixture.name) + " codec decode failed");
	}

	Memory memory;
	IO io;
	Clock clock;
	ClockCounter counter;
	Z80CompatCpu cpu;
	if (!cpu.Init(&memory, &io, &clock, &counter, 0x102) ||
	    cpu.GetStatusSize() != vaeg::z80_compat::kRevision1Size || !cpu.LoadStatus(input.data())) {
		Fail(std::string(fixture.name) + " production wrapper load failed");
	}

	Status output{};
	if (!cpu.SaveStatus(output.data())) {
		Fail(std::string(fixture.name) + " production wrapper save failed");
	}
	vaeg::z80_compat::LegacyState round_trip;
	if (!vaeg::z80_compat::DecodeRevision1(output.data(), &round_trip) ||
	    !SameState(decoded, round_trip)) {
		Fail(std::string(fixture.name) + " decoded state changed across wrapper load/save");
	}
	std::printf("fixture.%s=PASS\n", fixture.name);
}

} // namespace

int main() {
	static_assert(vaeg::z80_compat::kRevision1Size == 68, "revision-1 status size changed");
	static_assert(
	    vaeg::z80_compat::kOffsetAf == 0 && vaeg::z80_compat::kOffsetPc == 44 &&
	        vaeg::z80_compat::kOffsetWait == 57 && vaeg::z80_compat::kOffsetRevision == 59 &&
	        vaeg::z80_compat::kOffsetRemainClock == 60 && vaeg::z80_compat::kOffsetLastClock == 64,
	    "revision-1 field offsets changed");

	for (const Fixture &fixture : kFixtures) {
		VerifyFixture(fixture);
	}

	Status unsupported{};
	unsupported[vaeg::z80_compat::kOffsetRevision] = 2;
	vaeg::z80_compat::LegacyState decoded;
	if (vaeg::z80_compat::DecodeRevision1(unsupported.data(), &decoded)) {
		Fail("unsupported revision was accepted");
	}

	std::printf("sizeof.compat_status=%zu\n", vaeg::z80_compat::kRevision1Size);
	std::printf("offset.af=%zu\n", vaeg::z80_compat::kOffsetAf);
	std::printf("offset.pc=%zu\n", vaeg::z80_compat::kOffsetPc);
	std::printf("offset.wait=%zu\n", vaeg::z80_compat::kOffsetWait);
	std::printf("offset.revision=%zu\n", vaeg::z80_compat::kOffsetRevision);
	std::printf("offset.remainclock=%zu\n", vaeg::z80_compat::kOffsetRemainClock);
	std::printf("offset.lastclock=%zu\n", vaeg::z80_compat::kOffsetLastClock);
	std::printf("legacy-contract: %zu retained fixtures passed\n",
	            sizeof(kFixtures) / sizeof(kFixtures[0]));
	return 0;
}
