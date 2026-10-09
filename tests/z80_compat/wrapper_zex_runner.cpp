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
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

constexpr std::uint16_t kLoadAddress = 0x0100;
constexpr std::uint16_t kExpectedHaltPc = 0xff04;
constexpr std::uint64_t kDefaultMaxClocks = 60000000000ULL;
constexpr std::uint64_t kClockBatch = 10000000ULL;
constexpr unsigned int kDefaultMaxSeconds = 600;

class Machine final : public IMemoryAccess, public IIOAccess, public IClock, public IClockCounter {
  public:
	std::array<std::uint8_t, 65536> memory{};
	Z80CompatCpu cpu;
	std::string line;
	std::string recent_output;
	std::uint32_t current_clock = 0;
	std::int32_t remaining_clock = 0;
	std::uint64_t consumed_clocks = 0;
	bool saw_error = false;
	bool saw_completion = false;
	bool unsupported_io = false;
	// One entry per finished test group: "OK" or "ERROR <expected> <found>".
	std::vector<std::string> results;

	Machine() {
		if (!cpu.Init(this, this, this, this, 0x102)) {
			std::cerr << "zex-wrapper: Z80CompatCpu::Init failed\n";
			std::exit(1);
		}
	}

	std::uint32_t IFCALL Read8(std::uint32_t address) override {
		return memory[static_cast<std::uint16_t>(address)];
	}

	void IFCALL Write8(std::uint32_t address, std::uint32_t value) override {
		memory[static_cast<std::uint16_t>(address)] = static_cast<std::uint8_t>(value);
	}

	std::uint32_t IFCALL In(std::uint32_t port) override {
		std::cerr << "zex-wrapper: unexpected input from port 0x" << std::hex << port << std::dec
		          << '\n';
		unsupported_io = true;
		return 0;
	}

	void IFCALL Out(std::uint32_t port, std::uint32_t value) override {
		if (port != 0) {
			std::cerr << "zex-wrapper: unexpected output to port 0x" << std::hex << port
			          << " value 0x" << value << std::dec << '\n';
			unsupported_io = true;
			return;
		}
		const char character = static_cast<char>(value & 0xffU);
		std::cout.put(character);
		recent_output.push_back(character);
		if (recent_output.size() > 2048) {
			recent_output.erase(0, recent_output.size() - 2048);
		}
		if (character == '\n') {
			RecordResult(line);
			if (line.find("ERROR") != std::string::npos) {
				saw_error = true;
			}
			if (line.find("Tests complete") != std::string::npos) {
				saw_completion = true;
			}
			line.clear();
		} else if (character != '\r') {
			line.push_back(character);
		}
	}

	void RecordResult(const std::string &text) {
		const std::string::size_type expected = text.find("expected:");
		const std::string::size_type found = text.find("found:");
		if (text.size() >= 4 && text.compare(text.size() - 4, 4, "  OK") == 0) {
			results.push_back("OK");
		} else if (expected != std::string::npos && found != std::string::npos) {
			results.push_back("ERROR " + text.substr(expected + 9, 8) + " " +
			                  text.substr(found + 6, 8));
		}
	}

	std::uint32_t IFCALL now() override {
		return current_clock;
	}

	void IFCALL past(std::int32_t clocks) override {
		remaining_clock -= clocks;
		consumed_clocks += static_cast<std::uint32_t>(clocks);
	}

	std::int32_t IFCALL GetRemainclock() override {
		return remaining_clock;
	}

	void IFCALL SetRemainclock(std::int32_t clocks) override {
		remaining_clock = clocks;
	}

	bool LoadProgram(const std::string &path) {
		std::ifstream input(path, std::ios::binary | std::ios::ate);
		if (!input) {
			std::cerr << "zex-wrapper: cannot open artifact: " << path << '\n';
			return false;
		}
		const std::streamoff length = input.tellg();
		if (length <= 0 || length > static_cast<std::streamoff>(0xfe00 - kLoadAddress)) {
			std::cerr << "zex-wrapper: invalid artifact size: " << length << '\n';
			return false;
		}
		input.seekg(0);
		input.read(reinterpret_cast<char *>(memory.data() + kLoadAddress), length);
		if (!input) {
			std::cerr << "zex-wrapper: short artifact read\n";
			return false;
		}
		InstallBdos();
		cpu.SetPC(kLoadAddress);
		return true;
	}

	void InstallBdos() {
		const std::array<std::uint8_t, 8> low_memory = {0xc3, 0x03, 0xff, 0x00,
		                                                0x00, 0xc3, 0x06, 0xfe};
		const std::array<std::uint8_t, 23> bdos = {0x79, 0xfe, 0x02, 0x28, 0x05, 0xfe, 0x09, 0x28,
		                                           0x05, 0x76, 0x7b, 0xd3, 0x00, 0xc9, 0x1a, 0xfe,
		                                           0x24, 0xc8, 0xd3, 0x00, 0x13, 0x18, 0xf7};
		std::copy(low_memory.begin(), low_memory.end(), memory.begin());
		std::copy(bdos.begin(), bdos.end(), memory.begin() + 0xfe06);
		memory[0xff03] = 0x76;
	}

	void Advance(std::uint32_t clocks) {
		current_clock += clocks;
		cpu.Exec();
	}

	bool IsHalted() {
		std::array<std::uint8_t, vaeg::z80_compat::kRevision1Size> status{};
		return cpu.SaveStatus(status.data()) &&
		       (status[vaeg::z80_compat::kOffsetWait] & vaeg::z80_compat::kWaitHalt) != 0;
	}
};

bool ParseUnsigned(const char *text, std::uint64_t *value) {
	char *end = nullptr;
	const unsigned long long parsed = std::strtoull(text, &end, 10);
	if (text[0] == '\0' || end == nullptr || *end != '\0') {
		return false;
	}
	*value = parsed;
	return true;
}

void DumpFailure(Machine *machine, const std::string &reason) {
	const Z80CompatReg *reg = machine->cpu.GetReg();
	std::cerr << "zex-wrapper: " << reason << '\n'
	          << std::hex << std::setfill('0') << "zex-wrapper: PC=" << std::setw(4) << reg->pc
	          << " SP=" << std::setw(4) << reg->sp << " IX=" << std::setw(4) << reg->ix
	          << " IY=" << std::setw(4) << reg->iy << '\n'
	          << "zex-wrapper: AF=" << std::setw(4) << reg->af << " BC=" << std::setw(4) << reg->bc
	          << " DE=" << std::setw(4) << reg->de << " HL=" << std::setw(4) << reg->hl << '\n'
	          << "zex-wrapper: I=" << std::setw(2) << static_cast<unsigned int>(reg->ireg)
	          << " R=" << std::setw(2)
	          << static_cast<unsigned int>((reg->rreg & 0x7f) | (reg->rreg7 & 0x80))
	          << " IM=" << static_cast<unsigned int>(reg->intmode) << std::dec
	          << " clocks=" << machine->consumed_clocks << " remain=" << machine->remaining_clock
	          << '\n'
	          << "zex-wrapper: recent output follows:\n"
	          << machine->recent_output << '\n';
}

void Usage(const char *program) {
	std::cerr << "usage: " << program
	          << " [--max-clocks count] [--max-seconds seconds] [--profile zilog|upd9002]"
	             " [--expect file] file.cim\n";
}

// Read "<group> OK" or "<group> ERROR <expected> <found>" lines; '#' starts a
// comment. Groups must be numbered 1..n in order.
bool LoadExpectations(const std::string &path, std::vector<std::string> *expected) {
	std::ifstream input(path);
	if (!input) {
		std::cerr << "zex-wrapper: cannot open expectation file: " << path << '\n';
		return false;
	}
	std::string text;
	while (std::getline(input, text)) {
		const std::string::size_type comment = text.find('#');
		if (comment != std::string::npos) {
			text.erase(comment);
		}
		while (!text.empty() && (text.back() == ' ' || text.back() == '\r')) {
			text.pop_back();
		}
		if (text.empty()) {
			continue;
		}
		const std::string::size_type space = text.find(' ');
		if (space == std::string::npos ||
		    text.substr(0, space) != std::to_string(expected->size() + 1)) {
			std::cerr << "zex-wrapper: malformed expectation line: " << text << '\n';
			return false;
		}
		expected->push_back(text.substr(space + 1));
	}
	return !expected->empty();
}

bool CompareExpectations(const std::vector<std::string> &expected,
                         const std::vector<std::string> &actual) {
	bool match = expected.size() == actual.size();
	for (std::size_t i = 0; i < expected.size() || i < actual.size(); ++i) {
		const std::string want = i < expected.size() ? expected[i] : "(none)";
		const std::string got = i < actual.size() ? actual[i] : "(none)";
		if (want != got) {
			std::cerr << "zex-wrapper: group " << (i + 1) << ": expected \"" << want << "\", got \""
			          << got << "\"\n";
			match = false;
		}
	}
	return match;
}

} // namespace

int main(int argc, char **argv) {
	std::uint64_t max_clocks = kDefaultMaxClocks;
	std::uint64_t max_seconds = kDefaultMaxSeconds;
	std::string artifact;
	std::string expect_path;
	Z80CompatFlagProfile profile = Z80CompatFlagProfile::kZilog;
	for (int index = 1; index < argc; ++index) {
		const std::string argument = argv[index];
		if (argument == "--profile" && index + 1 < argc) {
			const std::string value = argv[++index];
			if (value == "zilog") {
				profile = Z80CompatFlagProfile::kZilog;
			} else if (value == "upd9002") {
				profile = Z80CompatFlagProfile::kUpd9002;
			} else {
				Usage(argv[0]);
				return 2;
			}
		} else if (argument == "--expect" && index + 1 < argc) {
			expect_path = argv[++index];
		} else if ((argument == "--max-clocks" || argument == "--max-seconds") &&
		           index + 1 < argc) {
			std::uint64_t parsed = 0;
			if (!ParseUnsigned(argv[++index], &parsed) || parsed == 0) {
				Usage(argv[0]);
				return 2;
			}
			if (argument == "--max-clocks") {
				max_clocks = parsed;
			} else {
				max_seconds = parsed;
			}
		} else if (!argument.empty() && argument[0] == '-') {
			Usage(argv[0]);
			return 2;
		} else if (artifact.empty()) {
			artifact = argument;
		} else {
			Usage(argv[0]);
			return 2;
		}
	}
	if (artifact.empty() || max_seconds > std::numeric_limits<unsigned int>::max()) {
		Usage(argv[0]);
		return 2;
	}

	std::vector<std::string> expectations;
	if (!expect_path.empty() && !LoadExpectations(expect_path, &expectations)) {
		return 1;
	}

	Machine machine;
	machine.cpu.SetFlagProfile(profile);
	if (!machine.LoadProgram(artifact)) {
		return 1;
	}
	const auto started = std::chrono::steady_clock::now();
	std::uint64_t credited_clocks = 0;
	while (credited_clocks < max_clocks) {
		const std::uint64_t credit = std::min(kClockBatch, max_clocks - credited_clocks);
		machine.Advance(static_cast<std::uint32_t>(credit));
		credited_clocks += credit;

		if (machine.IsHalted()) {
			const bool completed = machine.cpu.GetPC() == kExpectedHaltPc &&
			                       machine.saw_completion && !machine.unsupported_io;
			if (completed && !expectations.empty()) {
				if (CompareExpectations(expectations, machine.results)) {
					std::cout << "zex-wrapper: PASS expectations=" << expectations.size()
					          << " clocks=" << machine.consumed_clocks << '\n';
					return 0;
				}
				DumpFailure(&machine, "group results differ from the expectation file");
				return 1;
			}
			if (completed && !machine.saw_error) {
				std::cout << "zex-wrapper: PASS clocks=" << machine.consumed_clocks << '\n';
				return 0;
			}
			DumpFailure(&machine, "halted before clean test completion");
			return 1;
		}
		if ((machine.saw_error && expectations.empty()) || machine.unsupported_io) {
			DumpFailure(&machine, "test reported an error");
			return 1;
		}
		const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
		    std::chrono::steady_clock::now() - started);
		if (elapsed.count() >= static_cast<long long>(max_seconds)) {
			DumpFailure(&machine, "wall-clock timeout");
			return 1;
		}
	}
	DumpFailure(&machine, "emulated-clock limit reached");
	return 1;
}
