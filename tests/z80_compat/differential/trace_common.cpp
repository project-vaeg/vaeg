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

#include "trace_common.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace vaeg::z80_compat::differential {
namespace {

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
constexpr std::uint8_t kWaitEi = 0x04;

void WriteWord(std::array<std::uint8_t, kStatusSize> *image, std::size_t offset,
               std::uint16_t value) {
	(*image)[offset] = static_cast<std::uint8_t>(value);
	(*image)[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

std::uint16_t ReadWord(const std::array<std::uint8_t, kStatusSize> &image, std::size_t offset) {
	return static_cast<std::uint16_t>(image[offset]) |
	       static_cast<std::uint16_t>(image[offset + 1] << 8);
}

void WriteSigned(std::array<std::uint8_t, kStatusSize> *image, std::size_t offset,
                 std::int32_t value) {
	const std::uint32_t bits = static_cast<std::uint32_t>(value);
	for (std::size_t index = 0; index < 4; ++index) {
		(*image)[offset + index] = static_cast<std::uint8_t>(bits >> (index * 8));
	}
}

std::int32_t ReadSigned(const std::array<std::uint8_t, kStatusSize> &image, std::size_t offset) {
	std::uint32_t bits = 0;
	for (std::size_t index = 0; index < 4; ++index) {
		bits |= static_cast<std::uint32_t>(image[offset + index]) << (index * 8);
	}
	if (bits <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
		return static_cast<std::int32_t>(bits);
	}
	return static_cast<std::int32_t>(static_cast<std::int64_t>(bits) - (INT64_C(1) << 32));
}

Action Exec(std::uint32_t clocks, const char *reason = "exec-return", bool normalized = true) {
	return {ActionKind::Exec, clocks, 0, reason, normalized};
}

Action SetIrq(bool asserted) {
	return {ActionKind::SetIrq, asserted ? 1U : 0U, 0, "", false};
}

Action SetWait(bool asserted) {
	return {ActionKind::SetWait, asserted ? 1U : 0U, 0, "", false};
}

Action SetAck(std::uint8_t value) {
	return {ActionKind::SetAcknowledge, value, 0, "", false};
}

Action SetInput(std::uint8_t port, std::uint8_t value) {
	return {ActionKind::SetInput, port, value, "", false};
}

Action SaveLoad(const char *reason = "save-load-boundary", bool normalized = true) {
	return {ActionKind::SaveLoad, 0, 0, reason, normalized};
}

Action Nmi(const char *reason = "completed-nmi") {
	return {ActionKind::Nmi, 0, 0, reason, true};
}

Scenario OneInstruction(const std::string &identifier, const std::vector<std::uint8_t> &program,
                        const State &initial = State{}) {
	Scenario scenario;
	scenario.identifier = identifier;
	scenario.initial = initial;
	scenario.patches.push_back({initial.registers.pc, program});
	scenario.actions.push_back(Exec(1));
	return scenario;
}

std::uint8_t HexNibble(char value) {
	if (value >= '0' && value <= '9') {
		return static_cast<std::uint8_t>(value - '0');
	}
	if (value >= 'a' && value <= 'f') {
		return static_cast<std::uint8_t>(value - 'a' + 10);
	}
	throw std::runtime_error("invalid retained fixture hex digit");
}

std::array<std::uint8_t, kStatusSize> ParseStatus(const char *hex) {
	const std::string text(hex);
	if (text.size() != kStatusSize * 2) {
		throw std::runtime_error("invalid retained fixture length");
	}
	std::array<std::uint8_t, kStatusSize> image{};
	for (std::size_t index = 0; index < image.size(); ++index) {
		image[index] = static_cast<std::uint8_t>((HexNibble(text[index * 2]) << 4) |
		                                         HexNibble(text[index * 2 + 1]));
	}
	return image;
}

std::uint32_t NextRandom(std::uint32_t *state) {
	std::uint32_t value = *state;
	value ^= value << 13;
	value ^= value >> 17;
	value ^= value << 5;
	*state = value;
	return value;
}

std::string Hex(std::uint64_t value, int width) {
	std::ostringstream stream;
	stream << std::hex << std::nouppercase << std::setfill('0') << std::setw(width) << value;
	return stream.str();
}

std::string ProgramDescription(const Scenario &scenario) {
	if (scenario.patches.empty()) {
		return "none";
	}
	std::ostringstream stream;
	stream << Hex(scenario.patches.front().address, 4) << ':';
	for (std::uint8_t byte : scenario.patches.front().bytes) {
		stream << Hex(byte, 2);
	}
	return stream.str();
}

std::string ActionDescription(const Scenario &scenario) {
	std::ostringstream stream;
	for (std::size_t index = 0; index < scenario.actions.size(); ++index) {
		if (index != 0) {
			stream << ',';
		}
		const Action &action = scenario.actions[index];
		stream << static_cast<unsigned>(action.kind) << ':' << action.value << ':'
		       << action.auxiliary;
	}
	return stream.str();
}

void WriteSnapshot(std::ostream &output, const std::string &identifier, std::uint32_t seed,
                   std::uint32_t checkpoint, const Action &action, const Snapshot &snapshot) {
	for (const Event &event : snapshot.events) {
		output << "event test=" << identifier << " checkpoint=" << Hex(checkpoint, 8)
		       << " order=" << Hex(event.order, 8) << " type=" << event.type
		       << " address=" << Hex(event.address, 4) << " data=" << Hex(event.data, 2)
		       << " live_pc=" << (event.has_cpu_context ? Hex(event.live_pc, 4) : "-")
		       << " public_pc=" << (event.has_cpu_context ? Hex(event.public_pc, 4) : "-") << '\n';
	}
	const Registers &reg = snapshot.state.registers;
	output << "checkpoint test=" << identifier << " seed=" << Hex(seed, 8)
	       << " index=" << Hex(checkpoint, 8) << " reason=" << action.reason
	       << " normalized=" << (action.normalized ? 1 : 0) << " af=" << Hex(reg.af, 4)
	       << " bc=" << Hex(reg.bc, 4) << " de=" << Hex(reg.de, 4) << " hl=" << Hex(reg.hl, 4)
	       << " ix=" << Hex(reg.ix, 4) << " iy=" << Hex(reg.iy, 4) << " sp=" << Hex(reg.sp, 4)
	       << " pc=" << Hex(reg.pc, 4) << " af2=" << Hex(reg.alternate_af, 4)
	       << " bc2=" << Hex(reg.alternate_bc, 4) << " de2=" << Hex(reg.alternate_de, 4)
	       << " hl2=" << Hex(reg.alternate_hl, 4) << " i=" << Hex(reg.i, 2)
	       << " r=" << Hex(reg.r, 2) << " r_low=" << Hex(reg.r & 0x7f, 2)
	       << " r_bit7=" << ((reg.r & 0x80) != 0 ? 1 : 0) << " iff1=" << (reg.iff1 ? 1 : 0)
	       << " iff2=" << (reg.iff2 ? 1 : 0) << " im=" << static_cast<unsigned>(reg.im)
	       << " halt=" << (snapshot.state.halted ? 1 : 0)
	       << " wait=" << (snapshot.state.external_wait ? 1 : 0)
	       << " irq=" << (snapshot.state.irq ? 1 : 0)
	       << " ei=" << (snapshot.state.ei_inhibited ? 1 : 0)
	       << " live_pc=" << Hex(snapshot.live_pc, 4) << " public_pc=" << Hex(snapshot.public_pc, 4)
	       << " remain=" << snapshot.state.remainclock << " last=" << snapshot.state.lastclock
	       << " consumed=" << snapshot.consumed_clocks
	       << " memory_hash=" << Hex(snapshot.full_memory_hash, 16)
	       << " device_hash=" << Hex(snapshot.device_memory_hash, 16) << '\n';
}

} // namespace

std::array<std::uint8_t, kStatusSize> EncodeState(const State &state) {
	std::array<std::uint8_t, kStatusSize> image{};
	WriteWord(&image, kOffsetAf, state.registers.af);
	WriteWord(&image, kOffsetHl, state.registers.hl);
	WriteWord(&image, kOffsetDe, state.registers.de);
	WriteWord(&image, kOffsetBc, state.registers.bc);
	WriteWord(&image, kOffsetIx, state.registers.ix);
	WriteWord(&image, kOffsetIy, state.registers.iy);
	WriteWord(&image, kOffsetSp, state.registers.sp);
	WriteWord(&image, kOffsetAlternateAf, state.registers.alternate_af);
	WriteWord(&image, kOffsetAlternateHl, state.registers.alternate_hl);
	WriteWord(&image, kOffsetAlternateDe, state.registers.alternate_de);
	WriteWord(&image, kOffsetAlternateBc, state.registers.alternate_bc);
	std::uint16_t legacy_pc = state.registers.pc;
	if (state.halted) {
		legacy_pc = static_cast<std::uint16_t>(legacy_pc - 1);
	}
	WriteWord(&image, kOffsetPc, legacy_pc);
	image[kOffsetI] = state.registers.i;
	image[kOffsetRLow] = state.registers.r & 0x7f;
	image[kOffsetRBit7] = state.registers.r & 0x80;
	image[kOffsetIm] = state.registers.im & 0x03;
	image[kOffsetIff1] = state.registers.iff1 ? 1 : 0;
	image[kOffsetIff2] = state.registers.iff2 ? 1 : 0;
	image[kOffsetIrq] = state.irq ? 1 : 0;
	image[kOffsetWait] = static_cast<std::uint8_t>((state.halted ? kWaitHalt : 0) |
	                                               (state.external_wait ? kWaitExternal : 0) |
	                                               (state.ei_inhibited ? kWaitEi : 0));
	image[kOffsetXf] = static_cast<std::uint8_t>(state.registers.af) & 0x28;
	image[kOffsetRevision] = 1;
	WriteSigned(&image, kOffsetRemainClock, state.remainclock);
	WriteSigned(&image, kOffsetLastClock, state.lastclock);
	return image;
}

bool DecodeState(const std::array<std::uint8_t, kStatusSize> &image, State *state) {
	if (state == nullptr || image[kOffsetRevision] != 1) {
		return false;
	}
	State decoded;
	decoded.registers.af = ReadWord(image, kOffsetAf);
	decoded.registers.hl = ReadWord(image, kOffsetHl);
	decoded.registers.de = ReadWord(image, kOffsetDe);
	decoded.registers.bc = ReadWord(image, kOffsetBc);
	decoded.registers.ix = ReadWord(image, kOffsetIx);
	decoded.registers.iy = ReadWord(image, kOffsetIy);
	decoded.registers.sp = ReadWord(image, kOffsetSp);
	decoded.registers.alternate_af = ReadWord(image, kOffsetAlternateAf);
	decoded.registers.alternate_hl = ReadWord(image, kOffsetAlternateHl);
	decoded.registers.alternate_de = ReadWord(image, kOffsetAlternateDe);
	decoded.registers.alternate_bc = ReadWord(image, kOffsetAlternateBc);
	decoded.registers.pc = ReadWord(image, kOffsetPc);
	decoded.registers.i = image[kOffsetI];
	decoded.registers.r =
	    static_cast<std::uint8_t>((image[kOffsetRLow] & 0x7f) | (image[kOffsetRBit7] & 0x80));
	decoded.registers.im = image[kOffsetIm] & 0x03;
	decoded.registers.iff1 = image[kOffsetIff1] != 0;
	decoded.registers.iff2 = image[kOffsetIff2] != 0;
	decoded.irq = image[kOffsetIrq] != 0;
	decoded.halted = (image[kOffsetWait] & kWaitHalt) != 0;
	decoded.external_wait = (image[kOffsetWait] & kWaitExternal) != 0;
	decoded.ei_inhibited = (image[kOffsetWait] & kWaitEi) != 0;
	decoded.remainclock = ReadSigned(image, kOffsetRemainClock);
	decoded.lastclock = ReadSigned(image, kOffsetLastClock);
	if (decoded.halted) {
		decoded.registers.pc = static_cast<std::uint16_t>(decoded.registers.pc + 1);
	}
	*state = decoded;
	return true;
}

std::vector<Scenario> MakeDirectedCorpus() {
	std::vector<Scenario> corpus;
	State state;

	corpus.push_back(OneInstruction("load-a-immediate", {0x3e, 0x42}));
	state.registers.af = 0x1100;
	corpus.push_back(OneInstruction("add-immediate-flags", {0xc6, 0x22}, state));
	state.registers.af = 0x0140;
	corpus.push_back(OneInstruction("conditional-jr-not-taken", {0x20, 0x7f}, state));
	state.registers.af = 0x0100;
	corpus.push_back(OneInstruction("conditional-jr-taken", {0x20, 0x02}, state));

	state = State{};
	corpus.push_back(OneInstruction("call", {0xcd, 0x34, 0x12}, state));
	Scenario ret = OneInstruction("ret", {0xc9}, state);
	ret.patches.push_back({0xf000, {0x78, 0x56}});
	corpus.push_back(ret);
	state.registers.bc = 0x1234;
	corpus.push_back(OneInstruction("push-bc", {0xc5}, state));
	Scenario pop = OneInstruction("pop-de", {0xd1}, State{});
	pop.patches.push_back({0xf000, {0x78, 0x56}});
	corpus.push_back(pop);

	state = State{};
	state.registers.af = 0x1234;
	state.registers.alternate_af = 0xabcd;
	corpus.push_back(OneInstruction("exchange-af", {0x08}, state));
	state.registers.bc = 0x1111;
	state.registers.de = 0x2222;
	state.registers.hl = 0x3333;
	state.registers.alternate_bc = 0xaaaa;
	state.registers.alternate_de = 0xbbbb;
	state.registers.alternate_hl = 0xcccc;
	corpus.push_back(OneInstruction("exchange-alternates", {0xd9}, state));
	corpus.push_back(OneInstruction("ix-load", {0xdd, 0x21, 0x34, 0x12}));
	corpus.push_back(OneInstruction("iy-load", {0xfd, 0x21, 0x78, 0x56}));

	state = State{};
	state.registers.pc = 0xffff;
	corpus.push_back(OneInstruction("address-wrap-fetch", {0x3e, 0xa5}, state));
	state = State{};
	state.registers.af = 0x5a00;
	corpus.push_back(OneInstruction("memory-write-order", {0x32, 0x00, 0x20}, state));
	Scenario read = OneInstruction("memory-read-order", {0x3a, 0x00, 0x20});
	read.patches.push_back({0x2000, {0xa6}});
	corpus.push_back(read);

	const struct BlockMemoryCase {
		const char *name;
		std::uint8_t opcode;
		std::uint16_t bc;
	} block_memory_cases[] = {{"memory-ldi", 0xa0, 2},
	                          {"memory-ldd", 0xa8, 2},
	                          {"memory-ldir", 0xb0, 2},
	                          {"memory-lddr", 0xb8, 2}};
	for (const BlockMemoryCase &block : block_memory_cases) {
		state = State{};
		state.registers.af = 0x1201;
		state.registers.bc = block.bc;
		state.registers.de = 0x2100;
		state.registers.hl = 0x2000;
		Scenario scenario = OneInstruction(block.name, {0xed, block.opcode}, state);
		scenario.patches.push_back({0x2000, {0x34, 0x56}});
		corpus.push_back(scenario);
	}

	state = State{};
	state.registers.bc = 0x8100;
	corpus.push_back(OneInstruction("prefix-cb", {0xcb, 0x00}, state));
	state.registers.af = 0x0100;
	corpus.push_back(OneInstruction("prefix-ed", {0xed, 0x44}, state));
	corpus.push_back(OneInstruction("prefix-dd", {0xdd, 0x23}, State{}));
	corpus.push_back(OneInstruction("prefix-fd", {0xfd, 0x2b}, State{}));
	state = State{};
	state.registers.ix = 0x2000;
	Scenario ddcb = OneInstruction("prefix-ddcb", {0xdd, 0xcb, 0x01, 0x46}, state);
	ddcb.patches.push_back({0x2001, {0x01}});
	corpus.push_back(ddcb);
	state.registers.iy = 0x2001;
	Scenario fdcb = OneInstruction("prefix-fdcb", {0xfd, 0xcb, 0xff, 0xc6}, state);
	fdcb.patches.push_back({0x2000, {0x00}});
	corpus.push_back(fdcb);

	state = State{};
	state.registers.r = 0xff;
	corpus.push_back(OneInstruction("refresh-prefix-wrap", {0xed, 0x5f}, state));

	state = State{};
	state.registers.iff1 = false;
	state.registers.iff2 = true;
	Scenario retn = OneInstruction("interrupt-retn", {0xed, 0x45}, state);
	retn.patches.push_back({0xf000, {0x34, 0x12}});
	corpus.push_back(retn);
	state.registers.iff2 = false;
	Scenario retn_clear = OneInstruction("interrupt-retn-iff2-clear", {0xed, 0x45}, state);
	retn_clear.patches.push_back({0xf000, {0xbc, 0x9a}});
	corpus.push_back(retn_clear);
	state.registers.iff2 = true;
	Scenario reti = OneInstruction("interrupt-reti", {0xed, 0x4d}, state);
	reti.patches.push_back({0xf000, {0x78, 0x56}});
	corpus.push_back(reti);

	state = State{};
	state.registers.af = 0x5a00;
	Scenario out_imm = OneInstruction("io-out-immediate", {0xd3, 0xf0}, state);
	corpus.push_back(out_imm);
	Scenario in_imm = OneInstruction("io-in-immediate", {0xdb, 0xe1}, state);
	in_imm.actions.insert(in_imm.actions.begin(), SetInput(0xe1, 0xa5));
	corpus.push_back(in_imm);
	state.registers.bc = 0x12e2;
	corpus.push_back(OneInstruction("io-out-c-masked", {0xed, 0x79}, state));
	Scenario in_c = OneInstruction("io-in-c-masked", {0xed, 0x78}, state);
	in_c.actions.insert(in_c.actions.begin(), SetInput(0xe2, 0x3c));
	corpus.push_back(in_c);

	const struct BlockCase {
		const char *name;
		std::uint8_t opcode;
		bool input;
		std::uint16_t bc;
		std::uint16_t hl;
	} block_cases[] = {
	    {"io-ini", 0xa2, true, 0x02e0, 0x2000},   {"io-ind", 0xaa, true, 0x02e0, 0x2001},
	    {"io-inir", 0xb2, true, 0x02e0, 0x2000},  {"io-indr", 0xba, true, 0x02e0, 0x2001},
	    {"io-outi", 0xa3, false, 0x02e0, 0x2000}, {"io-outd", 0xab, false, 0x02e0, 0x2001},
	    {"io-otir", 0xb3, false, 0x02e0, 0x2000}, {"io-otdr", 0xbb, false, 0x02e0, 0x2001},
	};
	for (const BlockCase &block : block_cases) {
		state = State{};
		state.registers.bc = block.bc;
		state.registers.hl = block.hl;
		Scenario scenario = OneInstruction(block.name, {0xed, block.opcode}, state);
		scenario.patches.push_back({0x2000, {0x34, 0x56}});
		if (block.input) {
			scenario.actions.insert(scenario.actions.begin(), SetInput(0xe0, 0xa5));
		}
		corpus.push_back(scenario);
	}

	state = State{};
	state.registers.iff1 = true;
	state.registers.iff2 = true;
	state.registers.im = 1;
	Scenario di = OneInstruction("interrupt-di-asserted", {0xf3}, state);
	di.actions.insert(di.actions.begin(), SetIrq(true));
	corpus.push_back(di);

	state.registers.iff1 = false;
	state.registers.iff2 = false;
	Scenario ei;
	ei.identifier = "interrupt-ei-following-instruction";
	ei.initial = state;
	ei.patches.push_back({0x0000, {0xfb, 0x00}});
	ei.actions = {SetAck(0xff), SetIrq(true), Exec(1, "private-ei-boundary", false),
	              Exec(14, "completed-interrupt-acceptance", true)};
	corpus.push_back(ei);

	Scenario ei_ack;
	ei_ack.identifier = "interrupt-ei-following-changes-ack";
	ei_ack.initial = state;
	ei_ack.initial.registers.af = 0xcf00;
	ei_ack.initial.registers.im = 0;
	ei_ack.patches.push_back({0x0000, {0xfb, 0xd3, 0xf0}});
	ei_ack.actions = {SetAck(0x00), SetIrq(true), Exec(1, "private-ei-boundary", false),
	                  Exec(20, "completed-interrupt-acceptance", true)};
	corpus.push_back(ei_ack);

	Scenario deassert = ei;
	deassert.identifier = "interrupt-deasserted-before-acceptance";
	deassert.actions = {SetIrq(true), Exec(1, "private-ei-boundary", false), SetIrq(false),
	                    Exec(4, "deasserted-slice", true)};
	corpus.push_back(deassert);

	state = State{};
	state.registers.iff1 = true;
	state.registers.iff2 = true;
	state.registers.im = 1;
	Scenario persistent = OneInstruction("interrupt-persistent-level", {0x00}, state);
	persistent.patches.push_back({0x0038, {0xfb, 0x00}});
	persistent.actions = {SetIrq(true), Exec(1, "initial-acceptance", true),
	                      Exec(17, "reaccepted-persistent-level", true)};
	corpus.push_back(persistent);

	const struct Im0Case {
		const char *name;
		std::uint8_t acknowledge;
		std::vector<std::uint8_t> continuation;
	} im0_cases[] = {
	    {"interrupt-im0-00", 0x00, {}},
	    {"interrupt-im0-7f", 0x7f, {}},
	    {"interrupt-im0-rst-08", 0xcf, {}},
	    {"interrupt-im0-rst-38", 0xff, {}},
	    {"interrupt-im0-multibyte", 0xc3, {0x34, 0x12}},
	    {"interrupt-im0-prefix-cb", 0xcb, {0x00}},
	    {"interrupt-im0-prefix-ed", 0xed, {0x44}},
	    {"interrupt-im0-prefix-dd", 0xdd, {0x21, 0x34, 0x12}},
	};
	for (const Im0Case &item : im0_cases) {
		state = State{};
		state.registers.bc = 0x8100;
		state.registers.af = 0x0100;
		state.registers.iff1 = true;
		state.registers.iff2 = true;
		state.registers.im = 0;
		Scenario scenario = OneInstruction(item.name, {0x00}, state);
		if (!item.continuation.empty()) {
			scenario.patches.push_back({0x0001, item.continuation});
		}
		scenario.actions.insert(scenario.actions.begin(), SetIrq(true));
		scenario.actions.insert(scenario.actions.begin(), SetAck(item.acknowledge));
		corpus.push_back(scenario);
	}

	state = State{};
	state.registers.iff1 = true;
	state.registers.iff2 = true;
	state.registers.im = 1;
	Scenario im1 = OneInstruction("interrupt-im1", {0x00}, state);
	im1.actions.insert(im1.actions.begin(), SetIrq(true));
	im1.actions.insert(im1.actions.begin(), SetAck(0xa5));
	corpus.push_back(im1);

	state.registers.im = 2;
	state.registers.i = 0x12;
	Scenario im2 = OneInstruction("interrupt-im2-vector-order", {0x00}, state);
	im2.patches.push_back({0x1234, {0x78, 0x56}});
	im2.actions.insert(im2.actions.begin(), SetIrq(true));
	im2.actions.insert(im2.actions.begin(), SetAck(0x34));
	corpus.push_back(im2);

	state = State{};
	state.halted = true;
	state.registers.pc = 1;
	state.registers.iff1 = true;
	state.registers.iff2 = true;
	state.registers.im = 1;
	Scenario halt_wake;
	halt_wake.identifier = "interrupt-halt-wake";
	halt_wake.initial = state;
	halt_wake.patches.push_back({0x0000, {0x76}});
	halt_wake.actions = {SetIrq(true), Exec(1, "halt-exit", true)};
	corpus.push_back(halt_wake);

	state = State{};
	state.registers.pc = 0x2222;
	state.registers.sp = 0xf000;
	state.registers.iff1 = true;
	state.registers.iff2 = false;
	Scenario nmi;
	nmi.identifier = "interrupt-nmi";
	nmi.initial = state;
	nmi.actions = {Nmi()};
	corpus.push_back(nmi);

	Scenario priority = nmi;
	priority.identifier = "interrupt-nmi-priority";
	priority.actions = {SetIrq(true), Nmi("nmi-priority"), Exec(1, "post-nmi-slice", true)};
	priority.patches.push_back({0x0066, {0x00}});
	corpus.push_back(priority);

	Scenario halt_entry = OneInstruction("halt-entry", {0x76});
	corpus.push_back(halt_entry);

	Scenario wait;
	wait.identifier = "external-wait-drain-resume";
	wait.patches.push_back({0x0000, {0x00}});
	wait.actions = {SetWait(true), Exec(5, "wait-drain", true), SetWait(false),
	                Exec(1, "wait-resume", true)};
	corpus.push_back(wait);

	return corpus;
}

std::vector<Scenario> MakeGeneratedCorpus(std::uint32_t seed, std::size_t count) {
	const std::vector<std::vector<std::uint8_t>> templates = {
	    {0x00},       {0x03},       {0x13},       {0x23},       {0x33},       {0x0b},
	    {0x1b},       {0x2b},       {0x3b},       {0x41},       {0x4a},       {0x53},
	    {0x5c},       {0x78},       {0x7d},       {0x80},       {0x89},       {0x92},
	    {0x9b},       {0xa4},       {0xad},       {0xb6},       {0xbf},       {0x04},
	    {0x0d},       {0x14},       {0x1d},       {0x3c},       {0x3d},       {0xc6, 0x00},
	    {0xce, 0x00}, {0xd6, 0x00}, {0xde, 0x00}, {0xe6, 0x00}, {0xee, 0x00}, {0xf6, 0x00},
	    {0xfe, 0x00}};
	std::vector<Scenario> corpus;
	corpus.reserve(count);
	std::uint32_t random = seed;
	for (std::size_t index = 0; index < count; ++index) {
		const std::uint32_t choice = NextRandom(&random);
		std::vector<std::uint8_t> program = templates[choice % templates.size()];
		if (program.size() == 2 &&
		    (program[0] == 0xc6 || program[0] == 0xce || program[0] == 0xd6 || program[0] == 0xde ||
		     program[0] == 0xe6 || program[0] == 0xee || program[0] == 0xf6 ||
		     program[0] == 0xfe)) {
			program[1] = static_cast<std::uint8_t>(NextRandom(&random));
		}
		State initial;
		initial.registers.af = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.bc = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.de = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.hl = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.ix = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.iy = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.sp =
		    static_cast<std::uint16_t>(0xe000U | (NextRandom(&random) & 0x0ffeU));
		initial.registers.alternate_af = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.alternate_bc = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.alternate_de = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.alternate_hl = static_cast<std::uint16_t>(NextRandom(&random));
		initial.registers.i = static_cast<std::uint8_t>(NextRandom(&random));
		initial.registers.r = static_cast<std::uint8_t>(NextRandom(&random));
		std::ostringstream identifier;
		identifier << "generated-" << Hex(seed, 8) << '-' << Hex(index, 6);
		Scenario scenario = OneInstruction(identifier.str(), program, initial);
		scenario.seed = seed;
		scenario.patches.push_back({initial.registers.hl, {0x5a}});
		corpus.push_back(std::move(scenario));
	}
	return corpus;
}

std::vector<Scenario> MakeStateCorpus() {
	struct Fixture {
		const char *name;
		const char *hex;
	};
	constexpr Fixture fixtures[] = {
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
	std::vector<Scenario> corpus;
	for (const Fixture &fixture : fixtures) {
		Scenario scenario;
		scenario.identifier = std::string("fixture-") + fixture.name;
		scenario.retained_image = ParseStatus(fixture.hex);
		scenario.use_retained_image = true;
		scenario.actions = {SaveLoad("fixture-load-save")};
		corpus.push_back(scenario);
	}

	State state;
	state.remainclock = 17;
	Scenario positive;
	positive.identifier = "state-positive-clock";
	positive.initial = state;
	positive.actions = {SaveLoad()};
	corpus.push_back(positive);
	state.remainclock = 0;
	Scenario zero;
	zero.identifier = "state-zero-clock";
	zero.initial = state;
	zero.actions = {SaveLoad()};
	corpus.push_back(zero);
	state.remainclock = -17;
	state.lastclock = 23;
	Scenario negative;
	negative.identifier = "state-negative-clock";
	negative.initial = state;
	negative.actions = {SaveLoad()};
	corpus.push_back(negative);

	state = State{};
	state.halted = true;
	state.registers.pc = 1;
	Scenario halt;
	halt.identifier = "state-halt-resume";
	halt.initial = state;
	halt.patches.push_back({0x0000, {0x76}});
	halt.actions = {SaveLoad(), Exec(1, "halt-resumed-slice")};
	corpus.push_back(halt);

	state = State{};
	state.external_wait = true;
	Scenario wait;
	wait.identifier = "state-wait-resume";
	wait.initial = state;
	wait.patches.push_back({0x0000, {0x00}});
	wait.actions = {SaveLoad(), Exec(5, "restored-wait-drain"), SetWait(false),
	                Exec(1, "restored-wait-release")};
	corpus.push_back(wait);

	state = State{};
	state.irq = true;
	state.registers.iff1 = true;
	state.registers.iff2 = true;
	state.registers.im = 1;
	Scenario irq;
	irq.identifier = "state-irq-resume";
	irq.initial = state;
	irq.patches.push_back({0x0000, {0x00}});
	irq.actions = {SetAck(0xa5), SaveLoad(), Exec(1, "restored-irq-acceptance")};
	corpus.push_back(irq);

	state = State{};
	state.registers.im = 1;
	Scenario ei;
	ei.identifier = "state-ei-inhibition-boundary";
	ei.initial = state;
	ei.patches.push_back({0x0000, {0xfb, 0x00}});
	ei.actions = {SetIrq(true), Exec(1, "private-ei-boundary", false),
	              SaveLoad("private-ei-save-load", false),
	              Exec(14, "restored-ei-next-slice", true)};
	corpus.push_back(ei);
	return corpus;
}

std::vector<Scenario> MakeSchedulingRiskCorpus() {
	State state;
	state.registers.af = 0x5a00;
	Scenario scenario;
	scenario.identifier = "cycle-fdd-io-scheduling";
	scenario.initial = state;
	scenario.patches.push_back({0x0000, {0x18, 0x02, 0x00, 0x00, 0xd3, 0xf4}});
	scenario.actions = {Exec(1, "branch-return", true), Exec(7, "fdd-io-slice", true)};
	return {scenario};
}

std::vector<Scenario> MakeSchedulingConvergenceCorpus() {
	State state;
	state.registers.af = 0x5a00;
	Scenario scenario;
	scenario.identifier = "cycle-fdd-io-eventual-convergence";
	scenario.initial = state;
	scenario.patches.push_back({0x0000, {0x18, 0x02, 0x00, 0x00, 0xd3, 0xf4}});
	scenario.actions = {Exec(1, "branch-return", false), Exec(7, "fdd-io-slice", false),
	                    Exec(4, "convergence-wait-slice", false),
	                    Exec(1, "eventual-convergence", true)};
	return {scenario};
}

std::uint64_t HashMemory(const std::array<std::uint8_t, kMemorySize> &memory, std::size_t begin,
                         std::size_t end) {
	std::uint64_t hash = UINT64_C(1469598103934665603);
	end = std::min(end, memory.size());
	for (std::size_t index = begin; index < end; ++index) {
		hash ^= memory[index];
		hash *= UINT64_C(1099511628211);
	}
	return hash;
}

int RunTraceMain(int argc, char **argv, const std::string &backend_name,
                 const BackendFactory &factory) {
	std::string suite = "directed";
	std::string output_path;
	std::size_t generated_count = 128;
	std::vector<std::uint32_t> seeds;
	for (int index = 1; index < argc; ++index) {
		const std::string argument(argv[index]);
		if (argument == "--suite" && index + 1 < argc) {
			suite = argv[++index];
		} else if (argument == "--output" && index + 1 < argc) {
			output_path = argv[++index];
		} else if (argument == "--cases" && index + 1 < argc) {
			generated_count = static_cast<std::size_t>(std::stoull(argv[++index]));
		} else if (argument == "--seed" && index + 1 < argc) {
			seeds.push_back(static_cast<std::uint32_t>(std::stoul(argv[++index], nullptr, 0)));
		} else {
			std::cerr << "unknown or incomplete option: " << argument << '\n';
			return 2;
		}
	}
	if (output_path.empty()) {
		std::cerr << "--output is required\n";
		return 2;
	}
	if (seeds.empty()) {
		seeds = {0x4d383001U, 0x4d383002U, 0x4d383003U, 0x4d383004U};
	}

	std::vector<Scenario> scenarios;
	if (suite == "directed" || suite == "all") {
		std::vector<Scenario> directed = MakeDirectedCorpus();
		scenarios.insert(scenarios.end(), directed.begin(), directed.end());
	}
	if (suite == "state" || suite == "all") {
		std::vector<Scenario> state = MakeStateCorpus();
		scenarios.insert(scenarios.end(), state.begin(), state.end());
	}
	if (suite == "generated" || suite == "all") {
		for (std::uint32_t seed : seeds) {
			std::vector<Scenario> generated = MakeGeneratedCorpus(seed, generated_count);
			scenarios.insert(scenarios.end(), generated.begin(), generated.end());
		}
	}
	if (suite == "convergence" || suite == "all") {
		std::vector<Scenario> convergence = MakeSchedulingConvergenceCorpus();
		scenarios.insert(scenarios.end(), convergence.begin(), convergence.end());
	}
	if (suite == "scheduling") {
		scenarios = MakeSchedulingRiskCorpus();
	}
	if (scenarios.empty()) {
		std::cerr << "unknown or empty suite: " << suite << '\n';
		return 2;
	}

	std::ofstream output(output_path, std::ios::binary);
	if (!output) {
		std::cerr << "cannot create trace: " << output_path << '\n';
		return 2;
	}
	output << "trace schema=" << kTraceSchema << " backend=" << backend_name << " suite=" << suite
	       << '\n';

	for (const Scenario &scenario : scenarios) {
		std::unique_ptr<Backend> backend = factory();
		const std::array<std::uint8_t, kStatusSize> status =
		    scenario.use_retained_image ? scenario.retained_image : EncodeState(scenario.initial);
		if (!backend->Initialize(status, scenario.patches)) {
			std::cerr << "cannot initialize scenario " << scenario.identifier << '\n';
			return 1;
		}
		output << "test id=" << scenario.identifier << " seed=" << Hex(scenario.seed, 8)
		       << " program=" << ProgramDescription(scenario)
		       << " script=" << ActionDescription(scenario) << '\n';
		std::uint32_t checkpoint = 0;
		for (const Action &action : scenario.actions) {
			switch (action.kind) {
			case ActionKind::Exec:
				backend->AdvanceClock(action.value);
				backend->Exec();
				break;
			case ActionKind::SetIrq:
				backend->SetIrq(action.value != 0);
				break;
			case ActionKind::SetWait:
				backend->SetWait(action.value != 0);
				break;
			case ActionKind::Nmi:
				backend->Nmi();
				break;
			case ActionKind::SaveLoad:
				if (!backend->SaveLoad()) {
					std::cerr << "save/load failed in " << scenario.identifier << '\n';
					return 1;
				}
				break;
			case ActionKind::SetAcknowledge:
				backend->SetAcknowledge(static_cast<std::uint8_t>(action.value));
				break;
			case ActionKind::SetInput:
				backend->SetInput(static_cast<std::uint8_t>(action.value),
				                  static_cast<std::uint8_t>(action.auxiliary));
				break;
			}
			if (!action.reason.empty()) {
				WriteSnapshot(output, scenario.identifier, scenario.seed, checkpoint++, action,
				              backend->Capture());
			}
		}
	}
	output.close();
	if (!output) {
		std::cerr << "failed while writing trace: " << output_path << '\n';
		return 1;
	}
	std::cout << backend_name << ' ' << suite << ": " << scenarios.size()
	          << " scenarios written to " << output_path << '\n';
	return 0;
}

} // namespace vaeg::z80_compat::differential
