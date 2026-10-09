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

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kAcknowledgePort = 0x102;
using Status = std::array<std::uint8_t, vaeg::z80_compat::kRevision1Size>;

struct Event {
	char kind;
	std::uint32_t address;
	std::uint8_t data;

	bool operator==(const Event &other) const {
		return kind == other.kind && address == other.address && data == other.data;
	}
};

[[noreturn]] void Fail(const std::string &message) {
	std::fprintf(stderr, "compat-wrapper: %s\n", message.c_str());
	std::exit(1);
}

void Require(bool condition, const std::string &message) {
	if (!condition) {
		Fail(message);
	}
}

class Harness final : public IMemoryAccess, public IIOAccess, public IClock, public IClockCounter {
  public:
	std::array<std::uint8_t, 65536> memory{};
	std::vector<Event> events;
	Z80CompatCpu cpu;
	std::uint32_t clock_value = 0;
	std::int32_t remain = 0;
	std::int32_t multiplier = 1;
	std::uint64_t consumed = 0;
	std::uint8_t acknowledge = 0x00;
	std::uint8_t input_value = 0xff;
	std::uint16_t callback_mirror_pc = 0;
	std::uint16_t callback_live_pc = 0;
	bool observe_callback_pc = false;

	Harness() {
		Require(cpu.Init(this, this, this, this, static_cast<int>(kAcknowledgePort)),
		        "Init failed");
	}

	std::uint32_t IFCALL Read8(std::uint32_t address) override {
		const std::uint16_t masked = static_cast<std::uint16_t>(address);
		const std::uint8_t value = memory[masked];
		events.push_back({'r', masked, value});
		return value;
	}

	void IFCALL Write8(std::uint32_t address, std::uint32_t data) override {
		const std::uint16_t masked = static_cast<std::uint16_t>(address);
		const std::uint8_t value = static_cast<std::uint8_t>(data);
		memory[masked] = value;
		events.push_back({'w', masked, value});
	}

	std::uint32_t IFCALL In(std::uint32_t port) override {
		if (observe_callback_pc) {
			callback_mirror_pc = cpu.GetReg()->pc;
			callback_live_pc = static_cast<std::uint16_t>(cpu.GetPC());
		}
		if (port == kAcknowledgePort) {
			events.push_back({'a', port, acknowledge});
			return acknowledge;
		}
		const std::uint8_t value = input_value;
		events.push_back({'i', port, value});
		return value;
	}

	void IFCALL Out(std::uint32_t port, std::uint32_t data) override {
		if (observe_callback_pc) {
			callback_mirror_pc = cpu.GetReg()->pc;
			callback_live_pc = static_cast<std::uint16_t>(cpu.GetPC());
		}
		const std::uint8_t value = static_cast<std::uint8_t>(data);
		events.push_back({'o', port, value});
		if (port == 0xf0) {
			acknowledge = value;
		}
	}

	std::uint32_t IFCALL now() override {
		return clock_value;
	}

	void IFCALL past(std::int32_t clocks) override {
		consumed += static_cast<std::uint32_t>(clocks);
		remain -= clocks * multiplier;
	}

	std::int32_t IFCALL GetRemainclock() override {
		return remain;
	}

	void IFCALL SetRemainclock(std::int32_t clocks) override {
		remain = clocks;
	}

	void Install(std::uint16_t address, std::initializer_list<std::uint8_t> program) {
		std::uint16_t cursor = address;
		for (const std::uint8_t byte : program) {
			memory[cursor] = byte;
			cursor = static_cast<std::uint16_t>(cursor + 1);
		}
	}

	void Advance(std::uint32_t delta) {
		clock_value += delta;
		cpu.Exec();
	}

	Status Save() {
		Status status{};
		Require(cpu.SaveStatus(status.data()), "SaveStatus failed");
		return status;
	}

	void Load(const Status &status) {
		Require(cpu.LoadStatus(status.data()), "LoadStatus failed");
		clock_value = static_cast<std::uint32_t>(
		    static_cast<std::uint32_t>(status[vaeg::z80_compat::kOffsetLastClock]) |
		    (static_cast<std::uint32_t>(status[vaeg::z80_compat::kOffsetLastClock + 1]) << 8) |
		    (static_cast<std::uint32_t>(status[vaeg::z80_compat::kOffsetLastClock + 2]) << 16) |
		    (static_cast<std::uint32_t>(status[vaeg::z80_compat::kOffsetLastClock + 3]) << 24));
	}

	std::size_t AcknowledgeCount() const {
		return static_cast<std::size_t>(
		    std::count_if(events.begin(), events.end(), [](const Event &event) {
			    return event.kind == 'a';
		    }));
	}
};

vaeg::z80_compat::LegacyState BasicState() {
	vaeg::z80_compat::LegacyState state;
	state.registers.sp = 0xf000;
	return state;
}

Status Encode(const vaeg::z80_compat::LegacyState &state) {
	Status status{};
	vaeg::z80_compat::EncodeRevision1(state, status.data());
	return status;
}

std::uint16_t Af(Harness &harness) {
	return harness.cpu.GetReg()->af;
}

std::uint8_t A(Harness &harness) {
	return static_cast<std::uint8_t>(Af(harness) >> 8);
}

void TestResetAndDeterministicSave() {
	Harness harness;
	Require(harness.cpu.GetStatusSize() == vaeg::z80_compat::kRevision1Size,
	        "revision-1 size is not 68 bytes");
	Require(harness.cpu.GetPC() == 0 && harness.cpu.GetReg()->pc == 0, "reset PC is not zero");
	Require(harness.cpu.GetReg()->af == 0 && harness.cpu.GetReg()->sp == 0 && harness.remain == 0,
	        "reset register state is not deterministic");
	const Status first = harness.Save();
	harness.cpu.Reset();
	const Status second = harness.Save();
	Require(first == second, "reset save image is not deterministic");
	for (std::size_t index = 0; index < first.size(); ++index) {
		const std::uint8_t expected = index == vaeg::z80_compat::kOffsetRevision ? 1 : 0;
		Require(first[index] == expected, "reset save image contains unexpected data");
	}
}

void TestOrdinaryExecutionAndWrapping() {
	Harness ordinary;
	ordinary.Install(0, {0x3e, 0x42});
	ordinary.Advance(7);
	Require(A(ordinary) == 0x42 && ordinary.cpu.GetPC() == 2 && ordinary.cpu.GetReg()->pc == 2 &&
	            ordinary.remain == 0,
	        "ordinary LD execution failed");

	Harness wrapping;
	wrapping.cpu.SetPC(0xffff);
	wrapping.memory[0xffff] = 0x3e;
	wrapping.memory[0x0000] = 0x5a;
	wrapping.Advance(7);
	Require(A(wrapping) == 0x5a && wrapping.cpu.GetPC() == 1, "16-bit fetch wrapping failed");
	Require(wrapping.events.size() >= 2 && wrapping.events[0].address == 0xffff &&
	            wrapping.events[1].address == 0x0000,
	        "wrapped memory trace is incorrect");
}

void TestIoMasking() {
	Harness immediate;
	immediate.Install(0, {0x3e, 0x5a, 0xd3, 0xf0, 0xdb, 0xe1});
	immediate.Advance(18);
	Require(immediate.acknowledge == 0x5a, "immediate OUT did not reach port f0");
	immediate.input_value = 0x66;
	immediate.Advance(11);
	Require(A(immediate) == 0x66, "immediate IN did not return the fake value");
	Require(std::find(immediate.events.begin(), immediate.events.end(), Event{'o', 0xf0, 0x5a}) !=
	                immediate.events.end() &&
	            std::find(immediate.events.begin(), immediate.events.end(),
	                      Event{'i', 0xe1, 0x66}) != immediate.events.end(),
	        "immediate I/O port was not masked to eight bits");

	Harness c_port;
	vaeg::z80_compat::LegacyState state = BasicState();
	state.registers.bc = 0x12ab;
	state.registers.af = 0x7700;
	c_port.Load(Encode(state));
	c_port.Install(0, {0xed, 0x79, 0xed, 0x78});
	c_port.Advance(12);
	c_port.input_value = 0x33;
	c_port.Advance(12);
	Require(A(c_port) == 0x33, "IN A,(C) did not return the fake value");
	Require(std::find(c_port.events.begin(), c_port.events.end(), Event{'o', 0xab, 0x77}) !=
	                c_port.events.end() &&
	            std::find(c_port.events.begin(), c_port.events.end(), Event{'i', 0xab, 0x33}) !=
	                c_port.events.end(),
	        "(C)-port I/O exposed a 16-bit port");

	Harness outi;
	state = BasicState();
	state.registers.bc = 0x12ab;
	state.registers.hl = 0x2000;
	outi.Load(Encode(state));
	outi.memory[0x2000] = 0x91;
	outi.Install(0, {0xed, 0xa3});
	outi.Advance(16);
	Require(std::find(outi.events.begin(), outi.events.end(), Event{'o', 0xab, 0x91}) !=
	            outi.events.end(),
	        "OUTI exposed a 16-bit port");

	Harness ini;
	state = BasicState();
	state.registers.bc = 0x12ab;
	state.registers.hl = 0x2000;
	ini.Load(Encode(state));
	ini.input_value = 0x6c;
	ini.Install(0, {0xed, 0xa2});
	ini.Advance(16);
	Require(std::find(ini.events.begin(), ini.events.end(), Event{'i', 0xab, 0x6c}) !=
	                ini.events.end() &&
	            ini.memory[0x2000] == 0x6c,
	        "INI port masking or memory write failed");
}

void TestClockAndWaitContract() {
	Harness zero;
	zero.Install(0, {0x00});
	zero.Advance(4);
	Require(zero.remain == 0 && zero.cpu.GetPC() == 1, "zero clock balance failed");

	Harness negative;
	negative.Install(0, {0x00, 0x00});
	negative.Advance(3);
	Require(negative.remain == -1 && negative.cpu.GetPC() == 1,
	        "final instruction did not overshoot the budget");
	negative.Advance(1);
	Require(negative.remain == 0 && negative.cpu.GetPC() == 1,
	        "clock debt accumulation executed without positive credit");
	negative.Advance(4);
	Require(negative.remain == 0 && negative.cpu.GetPC() == 2,
	        "execution did not resume after debt repayment");

	Harness waiting;
	waiting.multiplier = 2;
	waiting.Install(0, {0x00});
	waiting.cpu.Wait(true);
	waiting.Advance(5);
	Require(waiting.cpu.GetPC() == 0 && waiting.remain == -5,
	        "external WAIT did not drain without execution");
	const Status wait_status = waiting.Save();
	Require((wait_status[vaeg::z80_compat::kOffsetWait] & vaeg::z80_compat::kWaitExternal) != 0,
	        "external WAIT was not serialized");
	waiting.cpu.Wait(false);
	waiting.Advance(9);
	Require(waiting.cpu.GetPC() == 1 && waiting.remain == -4,
	        "execution did not resume after WAIT release");
}

void TestLiveAndMirrorPc() {
	Harness harness;
	harness.cpu.SetPC(0x1732);
	harness.Install(0x1732, {0xd3, 0xfe});
	harness.observe_callback_pc = true;
	harness.Advance(11);
	Require(harness.callback_mirror_pc == 0x1732,
	        "I/O callback did not observe the legacy-stale mirror PC");
	Require(harness.callback_live_pc == 0x1734, "GetPC did not expose the authoritative live PC");
	Require(harness.cpu.GetReg()->pc == 0x1734,
	        "public mirror was not refreshed when Exec returned");
}

vaeg::z80_compat::LegacyState InterruptState(std::uint8_t mode, bool enabled = true) {
	vaeg::z80_compat::LegacyState state = BasicState();
	state.registers.intmode = mode;
	state.registers.iff1 = enabled;
	state.registers.iff2 = enabled;
	return state;
}

void TestInterruptLevelAndEi() {
	Harness di;
	di.Load(Encode(InterruptState(1)));
	di.Install(0, {0xf3});
	di.cpu.IRQ(0, 1);
	di.Advance(4);
	Require(di.AcknowledgeCount() == 0 && di.cpu.GetPC() == 1 && !di.cpu.GetReg()->iff1,
	        "DI did not suppress level IRQ acceptance");

	Harness ei;
	ei.Load(Encode(InterruptState(1, false)));
	ei.Install(0, {0xfb, 0x00});
	ei.cpu.IRQ(0, 1);
	ei.Advance(4);
	const Status inhibited = ei.Save();
	Require(ei.AcknowledgeCount() == 0 && ei.cpu.GetPC() == 1 &&
	            (inhibited[vaeg::z80_compat::kOffsetWait] & vaeg::z80_compat::kWaitEiInhibited) !=
	                0,
	        "EI inhibition was not retained at the instruction boundary");
	ei.Advance(4);
	Require(ei.AcknowledgeCount() == 1 && ei.cpu.GetPC() == 0x0038,
	        "IRQ was not accepted after the instruction following EI");

	Harness changed_ack;
	vaeg::z80_compat::LegacyState changed_state = InterruptState(0, false);
	changed_state.registers.af = 0xcf00;
	changed_ack.Load(Encode(changed_state));
	changed_ack.Install(0, {0xfb, 0xd3, 0xf0});
	changed_ack.cpu.IRQ(0, 1);
	changed_ack.Advance(4);
	changed_ack.acknowledge = 0x00;
	changed_ack.Advance(11);
	Require(changed_ack.AcknowledgeCount() == 1 && changed_ack.cpu.GetPC() == 0x0008,
	        "acknowledge byte was not sampled after the following instruction");

	Harness deasserted;
	deasserted.Load(Encode(InterruptState(1, false)));
	deasserted.Install(0, {0xfb, 0x00});
	deasserted.cpu.IRQ(0, 1);
	deasserted.Advance(4);
	deasserted.cpu.IRQ(0, 0);
	deasserted.Advance(4);
	Require(deasserted.AcknowledgeCount() == 0 && deasserted.cpu.GetPC() == 2,
	        "deasserted IRQ was accepted speculatively");

	Harness persistent;
	persistent.Load(Encode(InterruptState(1)));
	persistent.Install(0, {0x00});
	persistent.Install(0x0038, {0xfb, 0x00});
	persistent.cpu.IRQ(0, 1);
	persistent.Advance(4);
	Require(persistent.AcknowledgeCount() == 1,
	        "initial persistent IRQ was not accepted exactly once");
	persistent.Advance(12);
	Require(persistent.AcknowledgeCount() == 1 && persistent.cpu.GetPC() == 0x0039,
	        "persistent IRQ ignored EI inhibition");
	persistent.Advance(4);
	Require(persistent.AcknowledgeCount() == 2 && persistent.cpu.GetPC() == 0x0038,
	        "persistent level was not accepted after re-enabling IRQ");
}

void ConfigureIm0(Harness *harness, std::uint8_t acknowledge) {
	harness->Load(Encode(InterruptState(0)));
	harness->acknowledge = acknowledge;
	harness->Install(0, {0x00});
	harness->cpu.IRQ(0, 1);
}

void TestIm0RawOpcodes() {
	{
		Harness harness;
		ConfigureIm0(&harness, 0x00);
		harness.Advance(4);
		Require(harness.cpu.GetPC() == 1 && harness.AcknowledgeCount() == 1,
		        "IM0 raw 00 did not execute as NOP");
	}
	{
		Harness harness;
		ConfigureIm0(&harness, 0x7f);
		vaeg::z80_compat::LegacyState state = InterruptState(0);
		state.registers.af = 0x5a00;
		harness.Load(Encode(state));
		harness.cpu.IRQ(0, 1);
		harness.acknowledge = 0x7f;
		harness.Advance(4);
		Require(A(harness) == 0x5a && harness.cpu.GetPC() == 1,
		        "IM0 raw 7f did not execute as LD A,A");
	}
	for (std::uint8_t vector = 0; vector < 8; ++vector) {
		Harness harness;
		ConfigureIm0(&harness, static_cast<std::uint8_t>(0xc7U + vector * 8U));
		harness.Advance(4);
		Require(harness.cpu.GetPC() == vector * 8U && harness.cpu.GetReg()->sp == 0xeffe &&
		            harness.memory[0xefff] == 0x00 && harness.memory[0xeffe] == 0x01,
		        "IM0 RST-family dispatch failed");
	}
	{
		Harness harness;
		ConfigureIm0(&harness, 0xcd);
		harness.Install(1, {0x34, 0x12});
		harness.Advance(4);
		Require(harness.cpu.GetPC() == 0x1234 && harness.memory[0xeffe] == 0x03,
		        "IM0 multi-byte CALL failed");
	}
	{
		Harness harness;
		ConfigureIm0(&harness, 0xcb);
		vaeg::z80_compat::LegacyState state = InterruptState(0);
		state.registers.bc = 0x8100;
		harness.Load(Encode(state));
		harness.acknowledge = 0xcb;
		harness.cpu.IRQ(0, 1);
		harness.Install(1, {0x00});
		harness.Advance(4);
		Require(harness.cpu.GetReg()->bc == 0x0300 && harness.cpu.GetPC() == 2 &&
		            harness.cpu.GetReg()->rreg == 3,
		        "IM0 CB prefix failed");
	}
	{
		Harness harness;
		ConfigureIm0(&harness, 0xed);
		vaeg::z80_compat::LegacyState state = InterruptState(0);
		state.registers.af = 0x0100;
		harness.Load(Encode(state));
		harness.acknowledge = 0xed;
		harness.cpu.IRQ(0, 1);
		harness.Install(1, {0x44});
		harness.Advance(4);
		Require(A(harness) == 0xff && harness.cpu.GetPC() == 2, "IM0 ED prefix failed");
	}
	{
		Harness harness;
		ConfigureIm0(&harness, 0xdd);
		harness.Install(1, {0x21, 0x34, 0x12});
		harness.Advance(4);
		Require(harness.cpu.GetReg()->ix == 0x1234 && harness.cpu.GetPC() == 4,
		        "IM0 DD prefix failed");
	}
	{
		Harness harness;
		ConfigureIm0(&harness, 0xfd);
		harness.Install(1, {0x21, 0x78, 0x56});
		harness.Advance(4);
		Require(harness.cpu.GetReg()->iy == 0x5678 && harness.cpu.GetPC() == 4,
		        "IM0 FD prefix failed");
	}
	{
		Harness harness;
		ConfigureIm0(&harness, 0xdd);
		vaeg::z80_compat::LegacyState state = InterruptState(0);
		state.registers.ix = 0x2000;
		harness.Load(Encode(state));
		harness.acknowledge = 0xdd;
		harness.cpu.IRQ(0, 1);
		harness.Install(1, {0xcb, 0x02, 0x46});
		harness.memory[0x2002] = 1;
		harness.Advance(4);
		Require(harness.cpu.GetPC() == 4, "IM0 DDCB prefix failed");
	}
	{
		Harness harness;
		ConfigureIm0(&harness, 0xfd);
		vaeg::z80_compat::LegacyState state = InterruptState(0);
		state.registers.iy = 0x2100;
		harness.Load(Encode(state));
		harness.acknowledge = 0xfd;
		harness.cpu.IRQ(0, 1);
		harness.Install(1, {0xcb, 0xff, 0x86});
		harness.memory[0x20ff] = 0xff;
		harness.Advance(4);
		Require(harness.cpu.GetPC() == 4 && harness.memory[0x20ff] == 0xfe,
		        "IM0 FDCB prefix failed");
	}
}

void TestIm1Im2HaltAndNmi() {
	Harness im1;
	im1.Load(Encode(InterruptState(1)));
	im1.acknowledge = 0xcd;
	im1.Install(0, {0x00});
	im1.cpu.IRQ(0, 1);
	im1.Advance(4);
	Require(im1.AcknowledgeCount() == 1 && im1.cpu.GetPC() == 0x0038,
	        "IM1 did not acknowledge once and dispatch to 0038");

	Harness im2;
	vaeg::z80_compat::LegacyState im2_state = InterruptState(2);
	im2_state.registers.ireg = 0x20;
	im2.Load(Encode(im2_state));
	im2.acknowledge = 0x34;
	im2.memory[0x2034] = 0x78;
	im2.memory[0x2035] = 0x56;
	im2.Install(0, {0x00});
	im2.cpu.IRQ(0, 1);
	im2.Advance(4);
	Require(im2.AcknowledgeCount() == 1 && im2.cpu.GetPC() == 0x5678,
	        "IM2 acknowledge/vector dispatch failed");
	const auto acknowledge =
	    std::find_if(im2.events.begin(), im2.events.end(), [](const Event &event) {
		    return event.kind == 'a';
	    });
	const auto vector_low =
	    std::find(im2.events.begin(), im2.events.end(), Event{'r', 0x2034, 0x78});
	const auto vector_high =
	    std::find(im2.events.begin(), im2.events.end(), Event{'r', 0x2035, 0x56});
	Require(acknowledge != im2.events.end() && vector_low != im2.events.end() &&
	            vector_high != im2.events.end() && acknowledge < vector_low &&
	            vector_low < vector_high,
	        "IM2 vector reads did not follow actual acknowledge");

	Harness halted;
	halted.Load(Encode(InterruptState(1)));
	halted.Install(0, {0x76});
	halted.Advance(4);
	Status halt_status = halted.Save();
	Require(halted.cpu.GetPC() == 1 &&
	            (halt_status[vaeg::z80_compat::kOffsetWait] & vaeg::z80_compat::kWaitHalt) != 0 &&
	            halt_status[vaeg::z80_compat::kOffsetPc] == 0,
	        "HALT entry or legacy opcode-PC translation failed");
	const std::uint64_t clocks_before_idle = halted.consumed;
	halted.Advance(4);
	Require(halted.cpu.GetPC() == 1 && halted.consumed == clocks_before_idle + 4 &&
	            halted.cpu.GetReg()->rreg == 2,
	        "HALT idle M1 did not consume clocks or update R");
	halted.cpu.IRQ(0, 1);
	halted.Advance(4);
	Require(halted.AcknowledgeCount() == 1 && halted.cpu.GetPC() == 0x0038 &&
	            (halted.Save()[vaeg::z80_compat::kOffsetWait] & vaeg::z80_compat::kWaitHalt) == 0,
	        "maskable interrupt did not wake HALT");

	Harness nmi;
	vaeg::z80_compat::LegacyState nmi_state = InterruptState(1);
	nmi_state.registers.iff2 = false;
	nmi_state.registers.rreg = 0x7f;
	nmi.Load(Encode(nmi_state));
	nmi.cpu.IRQ(0, 1);
	const std::uint16_t mirrored_pc_before_nmi = nmi.cpu.GetReg()->pc;
	nmi.cpu.NMI();
	Require(nmi.AcknowledgeCount() == 0 && nmi.cpu.GetPC() == 0x0066 && !nmi.cpu.GetReg()->iff1 &&
	            nmi.cpu.GetReg()->iff2 && nmi.cpu.GetReg()->rreg == 0 &&
	            nmi.cpu.GetReg()->pc == mirrored_pc_before_nmi && nmi.cpu.GetReg()->sp == 0xeffe,
	        "NMI state, legacy mirror, R, or acknowledge isolation failed");

	Harness refresh;
	vaeg::z80_compat::LegacyState r_state = BasicState();
	r_state.registers.rreg = 0x7f;
	r_state.registers.rreg7 = 0x80;
	refresh.Load(Encode(r_state));
	refresh.Install(0, {0xcb, 0x00});
	refresh.Advance(8);
	Require(refresh.cpu.GetReg()->rreg == 1 && refresh.cpu.GetReg()->rreg7 == 0x80,
	        "R refresh counter did not count both prefix M1 fetches");

	Harness load_r;
	vaeg::z80_compat::LegacyState load_r_state = BasicState();
	load_r_state.registers.af = 0x0001;
	load_r_state.registers.rreg = 0x7f;
	load_r_state.registers.rreg7 = 0x80;
	load_r.Load(Encode(load_r_state));
	load_r.Install(0, {0xed, 0x5f});
	load_r.Advance(4);
	Require(A(load_r) == 0x81 && static_cast<std::uint8_t>(Af(load_r)) == 0x81,
	        "LD A,R did not materialize architectural flags");

	Harness reti;
	vaeg::z80_compat::LegacyState reti_state = BasicState();
	reti_state.registers.iff2 = true;
	reti.Load(Encode(reti_state));
	reti.memory[0xf000] = 0x34;
	reti.memory[0xf001] = 0x12;
	reti.Install(0, {0xed, 0x4d});
	reti.Advance(4);
	Require(reti.cpu.GetReg()->iff1 && reti.cpu.GetPC() == 0x1234,
	        "RETI did not restore IFF1 from IFF2");
}

std::uint8_t HexNibble(char value) {
	if (value >= '0' && value <= '9') {
		return static_cast<std::uint8_t>(value - '0');
	}
	if (value >= 'a' && value <= 'f') {
		return static_cast<std::uint8_t>(value - 'a' + 10);
	}
	Fail("invalid retained fixture hex digit");
}

Status ParseStatus(const char *hex) {
	const std::string text(hex);
	Require(text.size() == vaeg::z80_compat::kRevision1Size * 2,
	        "retained fixture has an invalid encoded length");
	Status result{};
	for (std::size_t index = 0; index < result.size(); ++index) {
		result[index] = static_cast<std::uint8_t>((HexNibble(text[index * 2]) << 4) |
		                                          HexNibble(text[index * 2 + 1]));
	}
	return result;
}

struct RetainedFixture {
	const char *name;
	const char *hex;
};

constexpr RetainedFixture kRetainedFixtures[] = {
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

void TestRetainedFixturesAndCodec() {
	for (const RetainedFixture &fixture : kRetainedFixtures) {
		const Status input = ParseStatus(fixture.hex);
		Require((input[vaeg::z80_compat::kOffsetWait] & vaeg::z80_compat::kWaitEiInhibited) == 0,
		        std::string(fixture.name) + " unexpectedly uses reserved wait bit 2");
		Harness restored;
		restored.Load(input);
		const Status output = restored.Save();
		Status normalized = input;
		normalized[vaeg::z80_compat::kOffsetXf] =
		    static_cast<std::uint8_t>(input[vaeg::z80_compat::kOffsetAf] & 0x28);
		Require(output == normalized,
		        std::string(fixture.name) + " failed revision-1 load/save normalization");
	}

	Harness round_trip;
	vaeg::z80_compat::LegacyState state = BasicState();
	state.registers.af = 0x1234;
	state.registers.hl = 0x2345;
	state.registers.de = 0x3456;
	state.registers.bc = 0x4567;
	state.registers.ix = 0x5678;
	state.registers.iy = 0x6789;
	state.registers.sp = 0x789a;
	state.registers.r_af = 0x89ab;
	state.registers.r_hl = 0x9abc;
	state.registers.r_de = 0xabcd;
	state.registers.r_bc = 0xbcde;
	state.registers.pc = 0xcdef;
	state.registers.ireg = 0xde;
	state.registers.rreg = 0x6f;
	state.registers.rreg7 = 0x80;
	state.registers.intmode = 2;
	state.registers.iff1 = true;
	state.registers.iff2 = false;
	state.halted = true;
	state.external_wait = true;
	state.irq_asserted = true;
	state.ei_inhibited = true;
	state.remainclock = 17;
	state.lastclock = -23;
	const Status encoded = Encode(state);
	Require((encoded[vaeg::z80_compat::kOffsetWait] & 0x07) == 0x07,
	        "HALT, external WAIT, and EI bits conflict");
	round_trip.Load(encoded);
	Require(round_trip.Save() == encoded && round_trip.cpu.GetPC() == state.registers.pc &&
	            round_trip.remain == 17,
	        "complete revision-1 field round trip failed");

	const Status before = round_trip.Save();
	Status unsupported = before;
	unsupported[vaeg::z80_compat::kOffsetRevision] = 2;
	Require(!round_trip.cpu.LoadStatus(unsupported.data()) && round_trip.Save() == before,
	        "unsupported revision was not rejected without mutation");

	Harness irq_restore;
	vaeg::z80_compat::LegacyState irq_state = InterruptState(1);
	irq_state.irq_asserted = true;
	irq_restore.Load(Encode(irq_state));
	Require(irq_restore.Save()[vaeg::z80_compat::kOffsetIrq] == 1,
	        "restored level-sensitive IRQ was not inspectable");
	irq_restore.Install(0, {0x00});
	irq_restore.Advance(4);
	Require(irq_restore.AcknowledgeCount() == 1, "restored level-sensitive IRQ was not accepted");
	irq_restore.cpu.IRQ(0, 0);
	Require(irq_restore.Save()[vaeg::z80_compat::kOffsetIrq] == 0,
	        "deasserted level-sensitive IRQ remained serialized");

	Harness halt_restore;
	vaeg::z80_compat::LegacyState halt_state = InterruptState(1);
	halt_state.registers.pc = 1;
	halt_state.halted = true;
	halt_state.irq_asserted = true;
	halt_restore.Load(Encode(halt_state));
	halt_restore.Advance(1);
	Require(halt_restore.AcknowledgeCount() == 1 && halt_restore.cpu.GetPC() == 0x0038,
	        "restored HALT state did not wake on restored IRQ level");

	Harness wait_restore;
	vaeg::z80_compat::LegacyState wait_state = BasicState();
	wait_state.external_wait = true;
	wait_restore.Load(Encode(wait_state));
	wait_restore.Install(0, {0x00});
	wait_restore.Advance(3);
	Require(wait_restore.cpu.GetPC() == 0 && wait_restore.remain == 0,
	        "restored external WAIT did not drain clock credit");
	wait_restore.cpu.Wait(false);
	wait_restore.Advance(4);
	Require(wait_restore.cpu.GetPC() == 1, "execution did not resume after restored WAIT release");
}

void TestEiSaveBoundary() {
	Harness baseline;
	baseline.Load(Encode(InterruptState(1, false)));
	baseline.Install(0, {0xfb, 0x3c});
	baseline.cpu.IRQ(0, 1);
	baseline.Advance(4);
	const Status boundary = baseline.Save();
	Require((boundary[vaeg::z80_compat::kOffsetWait] & vaeg::z80_compat::kWaitEiInhibited) != 0,
	        "new EI boundary save did not record reserved bit 2");

	Harness restored;
	restored.memory = baseline.memory;
	restored.Load(boundary);
	baseline.events.clear();
	restored.events.clear();
	baseline.Advance(4);
	restored.Advance(4);
	Require(baseline.AcknowledgeCount() == 1 && restored.AcknowledgeCount() == 1 &&
	            baseline.events == restored.events && baseline.memory == restored.memory &&
	            baseline.Save() == restored.Save() &&
	            baseline.cpu.GetReg()->af == restored.cpu.GetReg()->af &&
	            baseline.remain == restored.remain,
	        "EI frame-boundary restore changed execution, trace, or clocks");
}

struct TestCase {
	const char *name;
	void (*run)();
};

} // namespace

// M101: the uPD9002 profile never holds F bits 5/3 (R1, storage variant),
// including values written by the adapter outside the core. The Zilog
// profile keeps them.
void TestFlagProfileStorage() {
	for (const bool upd9002 : {false, true}) {
		const std::uint8_t kept = upd9002 ? 0xd7 : 0xff;
		const char *name = upd9002 ? "upd9002" : "zilog";

		Harness import;
		import.cpu.SetFlagProfile(upd9002 ? Z80CompatFlagProfile::kUpd9002
		                                  : Z80CompatFlagProfile::kZilog);
		Z80CompatReg reg{};
		reg.af = 0x12ff;
		reg.r_af = 0x34ff;
		import.cpu.SetReg(reg);
		Require((import.cpu.GetReg()->af & 0xff) == kept &&
		            (import.cpu.GetReg()->r_af & 0xff) == kept,
		        std::string("SetReg F storage mismatch: ") + name);
		import.cpu.SetMainReg(reg);
		Require((import.cpu.GetReg()->af & 0xff) == kept,
		        std::string("SetMainReg F storage mismatch: ") + name);

		vaeg::z80_compat::LegacyState state = BasicState();
		state.registers.af = 0x00ff;
		state.registers.r_af = 0x00ff;
		Harness load;
		load.cpu.SetFlagProfile(upd9002 ? Z80CompatFlagProfile::kUpd9002
		                                : Z80CompatFlagProfile::kZilog);
		load.Load(Encode(state));
		Require((load.cpu.GetReg()->af & 0xff) == kept && (load.cpu.GetReg()->r_af & 0xff) == kept,
		        std::string("LoadStatus F storage mismatch: ") + name);

		// LD A,I with I = 28h: the adapter materializes F from I.
		Harness materialize;
		materialize.cpu.SetFlagProfile(upd9002 ? Z80CompatFlagProfile::kUpd9002
		                                       : Z80CompatFlagProfile::kZilog);
		materialize.Install(0, {0x3e, 0x28, 0xed, 0x47, 0xed, 0x57});
		materialize.Advance(7 + 9 + 9);
		Require(A(materialize) == 0x28 && materialize.cpu.GetPC() == 6,
		        std::string("LD A,I sequence did not run: ") + name);
		Require((Af(materialize) & 0x28) == (upd9002 ? 0x00 : 0x28),
		        std::string("LD A,I F bits 5/3 mismatch: ") + name);

		// POP AF through the core.
		Harness pop;
		pop.cpu.SetFlagProfile(upd9002 ? Z80CompatFlagProfile::kUpd9002
		                               : Z80CompatFlagProfile::kZilog);
		pop.Install(0, {0x31, 0x00, 0x80, 0x01, 0xff, 0xff, 0xc5, 0xf1});
		pop.Advance(10 + 10 + 11 + 10);
		Require(pop.cpu.GetPC() == 8 && (Af(pop) & 0xff) == kept,
		        std::string("POP AF F storage mismatch: ") + name);
	}
}

// M102: opcodes without a handler in the vendored core (undefined ED, NEG/
// RETN/IM duplicates, DD/FD before a non-index opcode) used to call a null
// handler under Z80_NO_EXCEPTION. The Zilog profile executes them as on a
// Zilog Z80; the uPD9002 profile applies the semantics measured on a real
// PC-88VA2 (docs/agents/reports/m102_zexund_qa, runs 2-8).
void TestUndefinedOpcodes() {
	for (const bool upd9002 : {false, true}) {
		const Z80CompatFlagProfile profile =
		    upd9002 ? Z80CompatFlagProfile::kUpd9002 : Z80CompatFlagProfile::kZilog;
		for (const std::uint8_t prefix : {0xed, 0xdd, 0xfd}) {
			for (int op = 0; op < 256; ++op) {
				Harness harness;
				harness.cpu.SetFlagProfile(profile);
				Z80CompatReg reg{};
				reg.bc = 0x0001; // keep the uPD9002 word-LDIR forms short
				reg.sp = 0x8000;
				reg.pc = 0x0100;
				harness.cpu.SetReg(reg);
				harness.Install(0x0100, {prefix, static_cast<std::uint8_t>(op), 0x00, 0x00});
				harness.Advance(1);
			}
		}
		Harness neg;
		Z80CompatReg reg{};
		reg.af = 0x0100;
		reg.pc = 0x0100;
		if (upd9002) {
			// ED FE/FF: two-byte NOPs.
			for (const std::uint8_t op : {0xfe, 0xff}) {
				Harness harness;
				harness.cpu.SetFlagProfile(profile);
				Z80CompatReg r{};
				r.af = 0x1200;
				r.sp = 0x8000;
				r.pc = 0x0100;
				harness.cpu.SetReg(r);
				harness.Install(0x0100, {0xed, op});
				harness.Advance(8);
				Require(harness.cpu.GetPC() == 0x0102 && harness.cpu.GetReg()->af == 0x1200,
				        "uPD9002 ED FE/FF is not a two-byte NOP");
			}
			// ED C0: consumes one operand byte without any visible effect.
			Harness three;
			three.cpu.SetFlagProfile(profile);
			Z80CompatReg r{};
			r.af = 0x1200;
			r.sp = 0x8000;
			r.pc = 0x0100;
			three.cpu.SetReg(r);
			three.Install(0x0100, {0xed, 0xc0, 0x55});
			three.Advance(12);
			Require(three.cpu.GetPC() == 0x0103 && three.cpu.GetReg()->af == 0x1200,
			        "uPD9002 ED C0 is not a three-byte NOP");
			// ED 00 without a native reader: BC = 0000, F = Z and parity.
			Harness vec;
			vec.cpu.SetFlagProfile(profile);
			r = Z80CompatReg{};
			r.af = 0x12d7;
			r.bc = 0x0130;
			r.pc = 0x0100;
			vec.cpu.SetReg(r);
			vec.Install(0x0100, {0xed, 0x00});
			vec.Advance(16);
			Require(vec.cpu.GetPC() == 0x0102 && vec.cpu.GetReg()->bc == 0x0000 &&
			            (vec.cpu.GetReg()->af & 0xff) == 0x44,
			        "uPD9002 ED 00 default vector page is not zero");
			// ED 10 with a native reader: DE = IVT word at C/(C+1) mod 256,
			// F = sign/zero/parity as measured (EDPRB2 set 0: C=30h -> 2800h).
			Harness ivt;
			ivt.cpu.SetFlagProfile(profile);
			ivt.cpu.SetNativeVectorRead(
			    [](void *, std::uint8_t index) -> std::uint8_t {
				    switch (index) {
					    case 0x30: return 0x00;
					    case 0x31: return 0x28;
					    default: return 0xee;
				    }
			    },
			    nullptr);
			r = Z80CompatReg{};
			r.af = 0x12d7;
			r.bc = 0x0130;
			r.pc = 0x0100;
			ivt.cpu.SetReg(r);
			ivt.Install(0x0100, {0xed, 0x10});
			ivt.Advance(16);
			Require(ivt.cpu.GetPC() == 0x0102 && ivt.cpu.GetReg()->de == 0x2800 &&
			            (ivt.cpu.GetReg()->af & 0xff) == 0x04,
			        "uPD9002 ED 10 does not read the native vector page");
			// ED 4C duplicates RETN on the uPD9002 (EDPRB run 2).
			Harness retn;
			retn.cpu.SetFlagProfile(profile);
			r = Z80CompatReg{};
			r.sp = 0x8000;
			r.pc = 0x0100;
			retn.cpu.SetReg(r);
			retn.Install(0x0100, {0xed, 0x4c});
			retn.Install(0x8000, {0x34, 0x12});
			retn.Advance(14);
			Require(retn.cpu.GetPC() == 0x1234 && retn.cpu.GetReg()->sp == 0x8002,
			        "uPD9002 ED 4C is not RETN");
			// ED 80: one word copied from (HL) to (DE), BC-1, F unchanged.
			Harness ldi;
			ldi.cpu.SetFlagProfile(profile);
			r = Z80CompatReg{};
			r.af = 0x00d7;
			r.bc = 0x0130;
			r.de = 0x4000;
			r.hl = 0x5000;
			r.pc = 0x0100;
			ldi.cpu.SetReg(r);
			ldi.Install(0x0100, {0xed, 0x80});
			ldi.Install(0x5000, {0x34, 0x35});
			ldi.Advance(24);
			Require(ldi.cpu.GetPC() == 0x0102 && ldi.memory[0x4000] == 0x34 &&
			            ldi.memory[0x4001] == 0x35 && ldi.cpu.GetReg()->bc == 0x012f &&
			            ldi.cpu.GetReg()->de == 0x4002 && ldi.cpu.GetReg()->hl == 0x5002 &&
			            (ldi.cpu.GetReg()->af & 0xff) == 0xd7,
			        "uPD9002 ED 80 is not a word LDI");
			// ED 64: RRD that also writes (HL+1) (EDPRB2 set 0).
			Harness rrd;
			rrd.cpu.SetFlagProfile(profile);
			r = Z80CompatReg{};
			r.af = 0x5ad7;
			r.hl = 0x5000;
			r.pc = 0x0100;
			rrd.cpu.SetReg(r);
			rrd.Install(0x0100, {0xed, 0x64});
			rrd.Install(0x5000, {0x34, 0x35});
			rrd.Advance(20);
			Require((rrd.cpu.GetReg()->af >> 8) == 0x54 && rrd.memory[0x5000] == 0xa3 &&
			            rrd.memory[0x5001] == 0x34 && (rrd.cpu.GetReg()->af & 0xff) == 0x00,
			        "uPD9002 ED 64 is not the measured RRD variant");
		} else {
			const std::uint8_t znops[] = {0x00, 0x3f, 0x77, 0x7f, 0x80, 0xa4, 0xbf, 0xc0, 0xff};
			for (const std::uint8_t op : znops) {
				Harness harness;
				harness.cpu.SetFlagProfile(profile);
				Z80CompatReg r{};
				r.af = 0x1200;
				r.sp = 0x8000;
				r.pc = 0x0100;
				harness.cpu.SetReg(r);
				harness.Install(0x0100, {0xed, op});
				harness.Advance(8);
				Require(harness.cpu.GetPC() == 0x0102 && harness.cpu.GetReg()->af == 0x1200 &&
				            harness.consumed == 8,
				        "undefined ED opcode is not a two-byte NOP");
			}
			neg.cpu.SetFlagProfile(profile);
			neg.cpu.SetReg(reg);
			neg.Install(0x0100, {0xed, 0x4c});
			neg.Advance(8);
			Require(A(neg) == 0xff && neg.cpu.GetPC() == 0x0102, "ED 4C is not NEG");
		}
		Harness prefixed;
		prefixed.cpu.SetFlagProfile(profile);
		prefixed.cpu.SetReg(reg);
		prefixed.Install(0x0100, {0xfd, 0x3c});
		prefixed.Advance(8);
		Require(A(prefixed) == 0x02 && prefixed.cpu.GetPC() == 0x0102, "FD 3C is not INC A");
	}
}

// M103a: SetAltReg replaces only AF'/BC'/DE'/HL'; EXX / EX AF,AF' then
// expose them, and SetMainReg leaves them alone.
void TestAltRegSet() {
	Harness harness;
	Z80CompatReg reg{};
	reg.af = 0x1100;
	reg.bc = 0x2222;
	reg.de = 0x3333;
	reg.hl = 0x4444;
	reg.sp = 0x8000;
	reg.pc = 0x0100;
	harness.cpu.SetReg(reg);
	Z80CompatReg alt = *harness.cpu.GetReg();
	alt.r_af = 0xaa00;
	alt.r_bc = 0xbbbb;
	alt.r_de = 0xcccc;
	alt.r_hl = 0xdddd;
	alt.bc = 0xffff; // must be ignored by SetAltReg
	harness.cpu.SetAltReg(alt);
	const Z80CompatReg *now = harness.cpu.GetReg();
	Require(now->bc == 0x2222 && now->r_bc == 0xbbbb && now->r_af == 0xaa00,
	        "SetAltReg touched the main set or missed the alternate set");
	harness.cpu.SetMainReg(reg);
	Require(harness.cpu.GetReg()->r_hl == 0xdddd, "SetMainReg disturbed the alternate set");
	harness.Install(0x0100, {0xd9, 0x08}); // EXX ; EX AF,AF'
	harness.Advance(8);
	Require(harness.cpu.GetReg()->bc == 0xbbbb && harness.cpu.GetReg()->hl == 0xdddd &&
	            harness.cpu.GetReg()->r_bc == 0x2222,
	        "EXX did not expose the alternate set");
	harness.Advance(4);
	Require((harness.cpu.GetReg()->af >> 8) == 0xaa && (harness.cpu.GetReg()->r_af >> 8) == 0x11,
	        "EX AF,AF' did not expose AF'");
}

int main() {
	const TestCase tests[] = {
	    {"reset and deterministic save", TestResetAndDeterministicSave},
	    {"execution and wrapping", TestOrdinaryExecutionAndWrapping},
	    {"I/O masking", TestIoMasking},
	    {"clock and WAIT contract", TestClockAndWaitContract},
	    {"live and mirrored PC", TestLiveAndMirrorPc},
	    {"level IRQ and EI", TestInterruptLevelAndEi},
	    {"IM0 raw opcodes", TestIm0RawOpcodes},
	    {"IM1, IM2, HALT, NMI, R", TestIm1Im2HaltAndNmi},
	    {"revision-1 fixtures and codec", TestRetainedFixturesAndCodec},
	    {"EI save boundary", TestEiSaveBoundary},
	    {"flag profile storage", TestFlagProfileStorage},
	    {"undefined opcodes", TestUndefinedOpcodes},
	    {"alternate register set", TestAltRegSet},
	};
	for (const TestCase &test : tests) {
		test.run();
		std::printf("PASS: %s\n", test.name);
	}
	std::puts("All vaeg compatibility wrapper tests passed");
	return 0;
}
