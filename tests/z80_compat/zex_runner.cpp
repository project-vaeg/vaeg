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

#include "z80.hpp"

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

namespace {

constexpr std::uint16_t kLoadAddress = 0x0100;
constexpr std::uint16_t kExpectedHaltPC = 0xff04;
constexpr std::uint8_t kHaltFlag = 0x80;
constexpr std::uint64_t kDefaultMaxClocks = 60000000000ULL;
constexpr unsigned int kDefaultMaxSeconds = 600;
constexpr int kClockBatch = 35795450;

struct Machine {
	std::array<std::uint8_t, 65536> memory{};
	std::string line;
	std::string recent_output;
	bool saw_error = false;
	bool saw_completion = false;
	bool unsupported_io = false;

	static unsigned char read(void *opaque, unsigned short address) {
		Machine *machine = static_cast<Machine *>(opaque);
		return machine->memory[address];
	}

	static void write(void *opaque, unsigned short address, unsigned char value) {
		Machine *machine = static_cast<Machine *>(opaque);
		machine->memory[address] = value;
	}

	static unsigned char input(void *opaque, unsigned short port) {
		Machine *machine = static_cast<Machine *>(opaque);
		std::cerr << "zex: unexpected input from port 0x" << std::hex
		          << static_cast<unsigned int>(port) << std::dec << '\n';
		machine->unsupported_io = true;
		return 0;
	}

	static void output(void *opaque, unsigned short port, unsigned char value) {
		Machine *machine = static_cast<Machine *>(opaque);
		if ((port & 0xffU) != 0) {
			std::cerr << "zex: unexpected output to port 0x" << std::hex
			          << static_cast<unsigned int>(port) << " value 0x"
			          << static_cast<unsigned int>(value) << std::dec << '\n';
			machine->unsupported_io = true;
			return;
		}

		const char character = static_cast<char>(value);
		std::cout.put(character);
		machine->recent_output.push_back(character);
		if (machine->recent_output.size() > 2048) {
			machine->recent_output.erase(0, machine->recent_output.size() - 2048);
		}
		if (character == '\n') {
			if (machine->line.find("ERROR") != std::string::npos) {
				machine->saw_error = true;
			}
			if (machine->line.find("Tests complete") != std::string::npos) {
				machine->saw_completion = true;
			}
			machine->line.clear();
		} else if (character != '\r') {
			machine->line.push_back(character);
		}
	}

	bool load(const std::string &path) {
		std::ifstream input_file(path, std::ios::binary | std::ios::ate);
		if (!input_file) {
			std::cerr << "zex: cannot open artifact: " << path << '\n';
			return false;
		}
		const std::streamoff length = input_file.tellg();
		if (length <= 0 || length > static_cast<std::streamoff>(0xfe00 - kLoadAddress)) {
			std::cerr << "zex: invalid artifact size: " << length << '\n';
			return false;
		}
		input_file.seekg(0);
		input_file.read(reinterpret_cast<char *>(memory.data() + kLoadAddress), length);
		if (!input_file) {
			std::cerr << "zex: short read from artifact: " << path << '\n';
			return false;
		}
		install_cpm_bdos();
		return true;
	}

	void install_cpm_bdos() {
		const std::array<std::uint8_t, 8> low_memory = {0xc3, 0x03, 0xff, 0x00,
		                                                0x00, 0xc3, 0x06, 0xfe};
		const std::array<std::uint8_t, 23> bdos = {0x79, 0xfe, 0x02, 0x28, 0x05, 0xfe, 0x09, 0x28,
		                                           0x05, 0x76, 0x7b, 0xd3, 0x00, 0xc9, 0x1a, 0xfe,
		                                           0x24, 0xc8, 0xd3, 0x00, 0x13, 0x18, 0xf7};
		std::copy(low_memory.begin(), low_memory.end(), memory.begin());
		std::copy(bdos.begin(), bdos.end(), memory.begin() + 0xfe06);
		memory[0xff03] = 0x76;
	}
};

bool parse_unsigned(const char *text, std::uint64_t &value) {
	char *end = nullptr;
	const unsigned long long parsed = std::strtoull(text, &end, 10);
	if (text[0] == '\0' || end == nullptr || *end != '\0') {
		return false;
	}
	value = parsed;
	return true;
}

void dump_failure(const Z80 &cpu, const Machine &machine, std::uint64_t clocks,
                  const std::string &reason) {
	std::cerr << "zex: " << reason << '\n'
	          << std::hex << std::setfill('0') << "zex: PC=" << std::setw(4) << cpu.reg.PC
	          << " SP=" << std::setw(4) << cpu.reg.SP << " IX=" << std::setw(4) << cpu.reg.IX
	          << " IY=" << std::setw(4) << cpu.reg.IY << '\n'
	          << "zex: AF=" << std::setw(2) << static_cast<unsigned int>(cpu.reg.pair.A)
	          << std::setw(2) << static_cast<unsigned int>(cpu.reg.pair.F) << " BC=" << std::setw(2)
	          << static_cast<unsigned int>(cpu.reg.pair.B) << std::setw(2)
	          << static_cast<unsigned int>(cpu.reg.pair.C) << " DE=" << std::setw(2)
	          << static_cast<unsigned int>(cpu.reg.pair.D) << std::setw(2)
	          << static_cast<unsigned int>(cpu.reg.pair.E) << " HL=" << std::setw(2)
	          << static_cast<unsigned int>(cpu.reg.pair.H) << std::setw(2)
	          << static_cast<unsigned int>(cpu.reg.pair.L) << '\n'
	          << "zex: I=" << std::setw(2) << static_cast<unsigned int>(cpu.reg.I)
	          << " R=" << std::setw(2) << static_cast<unsigned int>(cpu.reg.R)
	          << " IFF=" << std::setw(2) << static_cast<unsigned int>(cpu.reg.IFF)
	          << " IM=" << std::setw(2) << static_cast<unsigned int>(cpu.reg.interrupt & 0x03)
	          << std::dec << " clocks=" << clocks << '\n'
	          << "zex: recent output follows:\n"
	          << machine.recent_output << '\n';
}

void usage(const char *program) {
	std::cerr << "usage: " << program << " [--max-clocks count] [--max-seconds seconds] file.cim\n";
}

} // namespace

int main(int argc, char **argv) {
	std::uint64_t max_clocks = kDefaultMaxClocks;
	std::uint64_t max_seconds = kDefaultMaxSeconds;
	std::string artifact;

	for (int index = 1; index < argc; ++index) {
		const std::string argument = argv[index];
		if ((argument == "--max-clocks" || argument == "--max-seconds") && index + 1 < argc) {
			std::uint64_t parsed = 0;
			if (!parse_unsigned(argv[++index], parsed) || parsed == 0) {
				usage(argv[0]);
				return 2;
			}
			if (argument == "--max-clocks") {
				max_clocks = parsed;
			} else {
				max_seconds = parsed;
			}
		} else if (!argument.empty() && argument[0] == '-') {
			usage(argv[0]);
			return 2;
		} else if (artifact.empty()) {
			artifact = argument;
		} else {
			usage(argv[0]);
			return 2;
		}
	}
	if (artifact.empty() || max_seconds > std::numeric_limits<unsigned int>::max()) {
		usage(argv[0]);
		return 2;
	}

	Machine machine;
	if (!machine.load(artifact)) {
		return 1;
	}
	Z80 cpu(Machine::read, Machine::write, Machine::input, Machine::output, &machine);
	cpu.reg.PC = kLoadAddress;

	const auto started = std::chrono::steady_clock::now();
	std::uint64_t clocks = 0;
	while (clocks < max_clocks) {
		const std::uint64_t remaining = max_clocks - clocks;
		const int budget = remaining > static_cast<std::uint64_t>(kClockBatch)
		                       ? kClockBatch
		                       : static_cast<int>(remaining);
		const int consumed = cpu.execute(budget);
		if (consumed <= 0) {
			dump_failure(cpu, machine, clocks, "core returned a non-positive clock count");
			return 1;
		}
		clocks += static_cast<std::uint64_t>(consumed);

		if ((cpu.reg.IFF & kHaltFlag) != 0) {
			if (cpu.reg.PC == kExpectedHaltPC && machine.saw_completion && !machine.saw_error &&
			    !machine.unsupported_io) {
				std::cout << "zex: PASS clocks=" << clocks << '\n';
				return 0;
			}
			dump_failure(cpu, machine, clocks, "halted before a clean test completion");
			return 1;
		}
		if (machine.saw_error || machine.unsupported_io) {
			dump_failure(cpu, machine, clocks, "test output or I/O reported a failure");
			return 1;
		}

		const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
		    std::chrono::steady_clock::now() - started);
		if (static_cast<std::uint64_t>(elapsed.count()) >= max_seconds) {
			dump_failure(cpu, machine, clocks, "wall-clock timeout");
			return 1;
		}
	}

	dump_failure(cpu, machine, clocks, "emulated-clock limit exceeded");
	return 1;
}
