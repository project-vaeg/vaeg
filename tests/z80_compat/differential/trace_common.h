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

#ifndef TESTS_COMPAT_DIFFERENTIAL_TRACE_COMMON_H
#define TESTS_COMPAT_DIFFERENTIAL_TRACE_COMMON_H

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vaeg::z80_compat::differential {

constexpr std::size_t kMemorySize = 65536;
constexpr std::size_t kStatusSize = 68;
constexpr const char *kTraceSchema = "vaeg-compat-trace-v1";

struct Registers {
	std::uint16_t af = 0;
	std::uint16_t bc = 0;
	std::uint16_t de = 0;
	std::uint16_t hl = 0;
	std::uint16_t ix = 0;
	std::uint16_t iy = 0;
	std::uint16_t sp = 0xf000;
	std::uint16_t pc = 0;
	std::uint16_t alternate_af = 0;
	std::uint16_t alternate_bc = 0;
	std::uint16_t alternate_de = 0;
	std::uint16_t alternate_hl = 0;
	std::uint8_t i = 0;
	std::uint8_t r = 0;
	std::uint8_t im = 0;
	bool iff1 = false;
	bool iff2 = false;
};

struct State {
	Registers registers;
	bool halted = false;
	bool external_wait = false;
	bool irq = false;
	bool ei_inhibited = false;
	std::int32_t remainclock = 0;
	std::int32_t lastclock = 0;
};

struct Patch {
	std::uint16_t address;
	std::vector<std::uint8_t> bytes;
};

enum class ActionKind {
	Exec,
	SetIrq,
	SetWait,
	Nmi,
	SaveLoad,
	SetAcknowledge,
	SetInput
};

struct Action {
	ActionKind kind;
	std::uint32_t value = 0;
	std::uint32_t auxiliary = 0;
	std::string reason;
	bool normalized = false;
};

struct Scenario {
	std::string identifier;
	std::uint32_t seed = 0;
	State initial;
	std::array<std::uint8_t, kStatusSize> retained_image{};
	bool use_retained_image = false;
	std::vector<Patch> patches;
	std::vector<Action> actions;
};

struct Event {
	std::uint32_t order = 0;
	std::string type;
	std::uint16_t address = 0;
	std::uint8_t data = 0;
	bool has_cpu_context = false;
	std::uint16_t live_pc = 0;
	std::uint16_t public_pc = 0;
};

struct Snapshot {
	State state;
	std::uint16_t live_pc = 0;
	std::uint16_t public_pc = 0;
	std::int64_t consumed_clocks = 0;
	std::uint64_t full_memory_hash = 0;
	std::uint64_t device_memory_hash = 0;
	std::vector<Event> events;
};

class Backend {
  public:
	virtual ~Backend() = default;
	virtual bool Initialize(const std::array<std::uint8_t, kStatusSize> &status,
	                        const std::vector<Patch> &patches) = 0;
	virtual void AdvanceClock(std::uint32_t delta) = 0;
	virtual void Exec() = 0;
	virtual void SetIrq(bool asserted) = 0;
	virtual void SetWait(bool asserted) = 0;
	virtual void Nmi() = 0;
	virtual bool SaveLoad() = 0;
	virtual void SetAcknowledge(std::uint8_t value) = 0;
	virtual void SetInput(std::uint8_t port, std::uint8_t value) = 0;
	virtual Snapshot Capture() = 0;
};

using BackendFactory = std::function<std::unique_ptr<Backend>()>;

std::array<std::uint8_t, kStatusSize> EncodeState(const State &state);
bool DecodeState(const std::array<std::uint8_t, kStatusSize> &image, State *state);
std::vector<Scenario> MakeDirectedCorpus();
std::vector<Scenario> MakeGeneratedCorpus(std::uint32_t seed, std::size_t count);
std::vector<Scenario> MakeStateCorpus();
std::vector<Scenario> MakeSchedulingRiskCorpus();
std::vector<Scenario> MakeSchedulingConvergenceCorpus();
std::uint64_t HashMemory(const std::array<std::uint8_t, kMemorySize> &memory, std::size_t begin,
                         std::size_t end);
int RunTraceMain(int argc, char **argv, const std::string &backend_name,
                 const BackendFactory &factory);

} // namespace vaeg::z80_compat::differential

#endif
