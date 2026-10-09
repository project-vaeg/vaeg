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

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "compiler.h"
#include "io/subsystem.h"
#include "machine/pccore.h"
#include "io/iocore.h"
#include "io/subsystemif.h"

namespace {

constexpr std::size_t kStatusSize = 68;
constexpr std::size_t kIff1Offset = 52;
constexpr std::size_t kIff2Offset = 53;
constexpr std::size_t kIrqOffset = 56;
constexpr std::size_t kWaitOffset = 57;
constexpr std::size_t kRevisionOffset = 59;
constexpr std::size_t kRemainOffset = 60;
constexpr std::size_t kLastClockOffset = 64;
constexpr std::uint8_t kWaitHalt = 0x01;
constexpr std::uint8_t kWaitExternal = 0x02;
constexpr std::uint8_t kWaitEi = 0x04;
constexpr const char *kCoreName = "uPD780/suzukiplan";

using Status = std::array<std::uint8_t, kStatusSize>;

[[noreturn]] void Fail(const char *message) {
	std::fprintf(stderr, "subsystem-integration[%s]: %s\n", kCoreName, message);
	std::exit(1);
}

void Require(bool condition, const char *message) {
	if (!condition) {
		Fail(message);
	}
}

std::uint8_t HexDigit(char value) {
	if (value >= '0' && value <= '9') {
		return static_cast<std::uint8_t>(value - '0');
	}
	if (value >= 'a' && value <= 'f') {
		return static_cast<std::uint8_t>(value - 'a' + 10);
	}
	Fail("invalid fixture hex digit");
}

Status ParseStatus(const char *hex) {
	Status result{};
	Require(std::strlen(hex) == result.size() * 2, "fixture status has the wrong encoded size");
	for (std::size_t i = 0; i < result.size(); ++i) {
		result[i] =
		    static_cast<std::uint8_t>((HexDigit(hex[i * 2]) << 4) | HexDigit(hex[i * 2 + 1]));
	}
	return result;
}

Status OrdinaryFixture() {
	return ParseStatus("20330000bc9a0000785600003412000000000000000000000090000000000000"
	                   "0000000000000000000000001100000000070000000000000000330100000000"
	                   "74000000");
}

void Put16(Status *status, std::size_t offset, std::uint16_t value) {
	(*status)[offset] = static_cast<std::uint8_t>(value);
	(*status)[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

void Put32(Status *status, std::size_t offset, std::int32_t value) {
	const std::uint32_t bits = static_cast<std::uint32_t>(value);
	for (unsigned i = 0; i < 4; ++i) {
		(*status)[offset + i] = static_cast<std::uint8_t>(bits >> (i * 8));
	}
}

void Install(std::uint16_t address, std::initializer_list<std::uint8_t> bytes) {
	subsystem_upd780_test_install(address, bytes.begin(), static_cast<UINT>(bytes.size()));
}

void Reset() {
	subsystem_upd780_test_reset();
}

void ExecAt(std::uint32_t now) {
	subsystem_upd780_test_set_clock(now);
	subsystem_exec();
}

Status Save() {
	Status result{};
	Require(subsystem_getcpustatussize() == result.size(),
	        "production status size is not revision-1 size");
	Require(subsystem_savecpustatus(result.data()) != FALSE, "production state save failed");
	return result;
}

VAEG_UPD780_INTEGRATION_CPU_STATE State() {
	VAEG_UPD780_INTEGRATION_CPU_STATE result{};
	Require(subsystem_upd780_test_get_state(&result) != FALSE,
	        "production state inspection failed");
	return result;
}

VAEG_UPD780_INTEGRATION_TRACE_STATE Trace() {
	VAEG_UPD780_INTEGRATION_TRACE_STATE result{};
	subsystem_upd780_test_get_trace(&result);
	return result;
}

void TestOrdinaryAndStateBridge() {
	char diagnostic[64] = {};

	Reset();
	Install(0x0000, {0xaf, 0x3e, 0x42});
	Require(subsystem_disassemble_bounded(0x0000, diagnostic, sizeof(diagnostic)) == 0x0001 &&
	            std::strcmp(diagnostic, "xor a") == 0,
	        "bounded production disassembler was unavailable");
	std::memset(diagnostic, 0, sizeof(diagnostic));
	Require(subsystem_disassemble(0x0000, diagnostic) == 0x0001 &&
	            std::strcmp(diagnostic, "xor a") == 0,
	        "production disassembler compatibility adapter failed");
	subsystem_upd780_test_set_pc(0x0000);
	ExecAt(11);
	const VAEG_UPD780_INTEGRATION_CPU_STATE executed = State();
	Require((executed.af >> 8) == 0x42 && executed.live_pc == 0x0003 &&
	            executed.public_pc == 0x0003 && executed.remainclock == 0,
	        "ordinary production execution or public mirror failed");

	const Status saved = Save();
	subsystem_upd780_test_set_pc(0x1234);
	Require(subsystem_loadcpustatus(saved.data()) != FALSE,
	        "new-to-same-core production load failed");
	Require(State().live_pc == 0x0003 && Save() == saved,
	        "new-to-same-core production round trip changed state");

	Status retained = OrdinaryFixture();
	Require(subsystem_loadcpustatus(retained.data()) != FALSE,
	        "retained legacy fixture did not load through production bridge");
	Require(State().live_pc == 0x0011, "retained fixture live PC mapping failed");

	const Status before_invalid = Save();
	retained[kRevisionOffset] = 2;
	Require(subsystem_loadcpustatus(retained.data()) == FALSE,
	        "unsupported revision was silently accepted");
	Require(Save() == before_invalid, "failed state load mutated production CPU state");
}

void TestClockStateAndWait() {
	for (const std::int32_t remain : {17, 0, -9}) {
		Reset();
		Status state = OrdinaryFixture();
		Put32(&state, kRemainOffset, remain);
		Put32(&state, kLastClockOffset, 37);
		Require(subsystem_loadcpustatus(state.data()) != FALSE, "clock-state fixture load failed");
		const Status saved = Save();
		Require(std::memcmp(saved.data() + kRemainOffset, state.data() + kRemainOffset, 8) == 0,
		        "remainclock or lastclock did not round trip");
	}

	Reset();
	Install(0x0000, {0x00});
	subsystem_upd780_test_set_wait(TRUE);
	ExecAt(8);
	VAEG_UPD780_INTEGRATION_CPU_STATE waiting = State();
	Require(waiting.live_pc == 0 && waiting.remainclock == 0 &&
	            (waiting.wait_flags & kWaitExternal) != 0,
	        "external WAIT did not drain without execution");
	const Status wait_status = Save();

	Reset();
	Install(0x0000, {0x00});
	Require(subsystem_loadcpustatus(wait_status.data()) != FALSE,
	        "external WAIT state did not restore");
	Require(Trace().wait_active != FALSE, "subsystem WAIT mirror did not restore with CPU state");
	ExecAt(12);
	Require(State().live_pc == 0, "restored external WAIT executed an instruction");
	subsystem_upd780_test_set_wait(FALSE);
	ExecAt(16);
	Require(State().live_pc == 1, "execution did not resume after restored WAIT release");
}

void TestHaltAndIrqState() {
	Reset();
	Install(0x0000, {0x76});
	ExecAt(4);
	const Status halted = Save();
	Require((halted[kWaitOffset] & kWaitHalt) != 0, "HALT was absent at production save boundary");
	Reset();
	Install(0x0000, {0x76});
	Require(subsystem_loadcpustatus(halted.data()) != FALSE, "HALT state did not load");
	ExecAt(8);
	Require((Save()[kWaitOffset] & kWaitHalt) != 0,
	        "HALT did not survive a resumed production slice");

	Status irq = OrdinaryFixture();
	irq[kIrqOffset] = 1;
	Reset();
	Require(subsystem_loadcpustatus(irq.data()) != FALSE, "asserted IRQ state did not load");
	Require(State().irq == 1 && Save()[kIrqOffset] == 1,
	        "asserted IRQ level was not inspectable/restorable");
	subsystem_irq(FALSE);
	Require(Save()[kIrqOffset] == 0, "IRQ deassertion did not update the saved level");
}

void TestProductionIrqAcknowledge() {
	Status enabled{};
	Put16(&enabled, 24, 0xf000);
	enabled[51] = 1;
	enabled[kIff1Offset] = 1;
	enabled[kIff2Offset] = 1;
	enabled[kRevisionOffset] = 1;

	Reset();
	Install(0x0000, {0xf3, 0x00});
	Require(subsystem_loadcpustatus(enabled.data()) != FALSE, "DI setup state did not load");
	ExecAt(4);
	subsystem_upd780_test_reset_trace();
	subsystem_irq(TRUE);
	Require(Trace().acknowledge_count == 0, "IRQ acknowledge was read speculatively");
	ExecAt(8);
	Require(Trace().acknowledge_count == 0, "DI did not suppress production IRQ acceptance");

	Reset();
	Install(0x0000, {0x00});
	Require(subsystem_loadcpustatus(enabled.data()) != FALSE, "IM1 acceptance state did not load");
	subsystem_irq(TRUE);
	Require(Trace().acknowledge_count == 0,
	        "IRQ assertion performed a speculative acknowledge read");
	ExecAt(4);
	Require(Trace().acknowledge_count == 1,
	        "accepted IM1 IRQ did not read acknowledge exactly once");
}

void TestSleepPath(std::uint16_t pc, std::uint8_t expected_path, bool needs_memory_marker) {
	Reset();
	Install(pc, {0xdb, 0xfe, 0x00});
	if (needs_memory_marker) {
		Install(0x7f67, {0xff});
	}
	subsystem_upd780_test_set_pc(pc);
	ExecAt(0);
	ExecAt(11);
	VAEG_UPD780_INTEGRATION_TRACE_STATE trace = Trace();
	VAEG_UPD780_INTEGRATION_CPU_STATE state = State();
	std::fprintf(
	    stderr,
	    "subsystem-integration[%s]: sleep path=%u expected=%u "
	    "port=%02x memory=%02x live=%04x public=%04x "
	    "wait-assert=%u wait-release=%u wait-active=%u "
	    "state-pc=%04x state-public=%04x wait-flags=%02x "
	    "remain=%d last=%u\n",
	    kCoreName, static_cast<unsigned>(trace.sleep_path), static_cast<unsigned>(expected_path),
	    static_cast<unsigned>(trace.sleep_port_value),
	    static_cast<unsigned>(trace.sleep_memory_value), static_cast<unsigned>(trace.sleep_live_pc),
	    static_cast<unsigned>(trace.sleep_public_pc),
	    static_cast<unsigned>(trace.wait_assert_count),
	    static_cast<unsigned>(trace.wait_release_count), static_cast<unsigned>(trace.wait_active),
	    static_cast<unsigned>(state.live_pc), static_cast<unsigned>(state.public_pc),
	    static_cast<unsigned>(state.wait_flags), state.remainclock,
	    static_cast<unsigned>(state.lastclock));
	Require(trace.fe_read_count == 1 && trace.sleep_path == expected_path &&
	            trace.sleep_public_pc == pc && trace.sleep_live_pc == pc + 2 &&
	            trace.wait_assert_count == 1 && trace.wait_active != FALSE &&
	            (state.wait_flags & kWaitExternal) != 0,
	        "SLEEP_HACK callback mirror or WAIT assertion failed");
	if (needs_memory_marker) {
		Require(trace.sleep_memory_value == 0xff, "VA sleep memory condition was not observed");
	}

	ExecAt(19);
	Require(State().live_pc == pc + 2 && State().remainclock == 0,
	        "SLEEP_HACK WAIT did not drain clocks without execution");
	subsystem_businportc(0x80);
	trace = Trace();
	Require(trace.wait_release_count == 1 && trace.wait_active == FALSE,
	        "ATN wake did not release external WAIT");
	ExecAt(23);
	Require(State().live_pc == pc + 3, "execution did not resume after SLEEP_HACK wake");
}

void TestEiBeforeSleepHypothesis() {
	Reset();
	Install(0x1731, {0xfb, 0xdb, 0xfe});
	Install(0x7f67, {0xff});
	subsystem_upd780_test_set_pc(0x1731);
	ExecAt(1);
	ExecAt(5);
	const VAEG_UPD780_INTEGRATION_TRACE_STATE trace = Trace();
	Require(trace.fe_read_count == 1 && trace.sleep_path == 1 && trace.sleep_public_pc == 0x1732 &&
	            trace.wait_active != FALSE,
	        "EI-following production sleep boundary was not preserved");
}

void TestFddBoundary() {
	Reset();
	Install(0x0000, {0xaf, 0x3e, 0x5a, 0xd3, 0xf4, 0x00});
	subsystem_upd780_test_set_pc(0x0000);
	ExecAt(22);
	VAEG_UPD780_INTEGRATION_TRACE_STATE trace = Trace();
	Require(trace.f4_count == 1 && trace.f4_last_value == 0x5a,
	        "production FDD control event was missing or duplicated");
	const Status between_calls = Save();

	ExecAt(26);
	Require(subsystem_loadcpustatus(between_calls.data()) != FALSE,
	        "FDD-boundary state did not reload");
	ExecAt(26);
	trace = Trace();
	Require(trace.f4_count == 1 && trace.f4_last_value == 0x5a,
	        "FDD-boundary resume duplicated or changed port 0xf4 output");
}

void TestEiStateBoundary() {
	Reset();
	Status enabled{};
	Put16(&enabled, 24, 0xf000);
	enabled[51] = 1;
	enabled[kIff1Offset] = 1;
	enabled[kIff2Offset] = 1;
	enabled[kRevisionOffset] = 1;
	Require(subsystem_loadcpustatus(enabled.data()) != FALSE, "EI setup state did not load");
	Install(0x0000, {0xfb, 0x3c});
	subsystem_irq(TRUE);
	ExecAt(4);
	const Status inhibited = Save();
	Require((inhibited[kWaitOffset] & kWaitEi) != 0 && Trace().acknowledge_count == 0,
	        "production EI boundary did not save inhibition");

	Reset();
	Install(0x0000, {0xfb, 0x3c});
	Require(subsystem_loadcpustatus(inhibited.data()) != FALSE,
	        "production EI inhibition did not restore");
	subsystem_upd780_test_reset_trace();
	ExecAt(8);
	Require(Trace().acknowledge_count == 1 && (State().af >> 8) == 1 &&
	            (Save()[kWaitOffset] & kWaitEi) == 0,
	        "restored EI boundary changed following instruction or IRQ timing");
}

// The fast transfer protocol reads both data ports on each side, so all
// four cross-wired data paths must carry values in both directions.
void TestCrossWiredDataPorts() {
	Reset();
	iocore_create();
	Require(iocore_build() == SUCCESS, "main I/O map was not built");
	subsystemif_initialize();
	subsystemif_reset();
	subsystemif_bind();
	iocore_out8(0xff, 0x93); // Main: ports A and B input.
	Install(0x0000, {0x3e, 0x80, 0xd3, 0xff, 0x3e, 0x5a, 0xd3, 0xfc, 0x3e, 0xa5, 0xd3,
	                 0xfd, 0x3e, 0x92, 0xd3, 0xff, 0xdb, 0xfd, 0x47, 0xdb, 0xfc});
	subsystem_upd780_test_set_pc(0x0000);
	ExecAt(54); // Sub: ports A and B output, A=5Ah, B=A5h.
	Require(iocore_inp8(0xfd) == 0x5a && iocore_inp8(0xfc) == 0xa5,
	        "sub-to-main data ports are not cross-wired");
	ExecAt(72); // Sub: ports A and B input.
	iocore_out8(0xff, 0x80);
	iocore_out8(0xfc, 0x3c);
	iocore_out8(0xfd, 0xc3);
	ExecAt(98); // Sub: B <- IN FDh, A <- IN FCh.
	const VAEG_UPD780_INTEGRATION_CPU_STATE state = State();
	Require((state.bc >> 8) == 0x3c && (state.af >> 8) == 0xc3 && state.live_pc == 0x0015,
	        "main-to-sub data ports are not cross-wired");
	iocore_destroy();
}

} // namespace

extern "C" int vaeg_upd780_subsystem_integration_test(void) {
	subsystem_initialize();
	TestOrdinaryAndStateBridge();
	TestClockStateAndWait();
	TestHaltAndIrqState();
	TestProductionIrqAcknowledge();
	TestSleepPath(0x1732, 1, true);
	TestSleepPath(0x700e, 2, false);
	TestEiBeforeSleepHypothesis();
	TestFddBoundary();
	TestEiStateBoundary();
	TestCrossWiredDataPorts();
	std::fprintf(stderr, "subsystem-integration[%s]: all tests passed\n", kCoreName);
	return SUCCESS;
}
