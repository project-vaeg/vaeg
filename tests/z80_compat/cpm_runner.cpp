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

// Minimal CP/M 2.2 host runner for the M101 probe programs.
//
// The program is loaded at 0100h. BDOS calls reach a trap at FE00h, and a
// warm boot (JP 0) ends the run. Supported BDOS functions: 2, 9, 13, 14, 15,
// 16, 19, 21, 22 and 26. Files are mapped by their 8.3 FCB name into one host
// directory. Any other function stops the run with an error.
// --profile selects the core flag profile (zilog by default, or upd9002).

#include "z80.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr std::uint16_t kLoadAddress = 0x0100;
constexpr std::uint16_t kBdosAddress = 0xfe00;
constexpr std::uint8_t kTrapPort = 0x00;
constexpr std::uint8_t kExitPort = 0x01;
constexpr int kRecordSize = 128;
constexpr int kClockBatch = 1 << 24;
constexpr std::uint64_t kDefaultMaxClocks = 200000000000ULL;

struct Machine {
	std::array<std::uint8_t, 65536> memory{};
	Z80 *cpu = nullptr;
	std::string directory = ".";
	std::string console;
	std::uint16_t dma = 0x0080;
	bool finished = false;
	bool failed = false;
	std::string failure;
	// Open files keyed by the FCB address; each holds the host path.
	std::map<std::uint16_t, std::string> open_files;

	void Fail(const std::string &reason) {
		if (!failed) {
			failed = true;
			failure = reason;
		}
		finished = true;
		cpu->requestBreak();
	}

	std::string FcbName(std::uint16_t fcb) const {
		std::string name;
		for (int i = 1; i <= 8; ++i) {
			const char c = static_cast<char>(memory[(fcb + i) & 0xffff] & 0x7f);
			if (c != ' ') {
				name.push_back(c);
			}
		}
		std::string extension;
		for (int i = 9; i <= 11; ++i) {
			const char c = static_cast<char>(memory[(fcb + i) & 0xffff] & 0x7f);
			if (c != ' ') {
				extension.push_back(c);
			}
		}
		if (!extension.empty()) {
			name += "." + extension;
		}
		return name;
	}

	bool ValidName(const std::string &name) const {
		if (name.empty() || name.size() > 12) {
			return false;
		}
		for (const char c : name) {
			if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' ||
			      c == '_')) {
				return false;
			}
		}
		return true;
	}

	std::string HostPath(std::uint16_t fcb) {
		const std::string name = FcbName(fcb);
		if (!ValidName(name)) {
			Fail("invalid FCB file name: " + name);
			return std::string();
		}
		return directory + "/" + name;
	}

	void ResetFcbPosition(std::uint16_t fcb) {
		memory[(fcb + 12) & 0xffff] = 0;
		memory[(fcb + 14) & 0xffff] = 0;
		memory[(fcb + 32) & 0xffff] = 0;
	}

	std::uint8_t WriteSequential(std::uint16_t fcb) {
		const auto found = open_files.find(fcb);
		if (found == open_files.end()) {
			Fail("write to an FCB that is not open");
			return 0xff;
		}
		const std::uint32_t record =
		    static_cast<std::uint32_t>(memory[(fcb + 12) & 0xffff]) * 128U +
		    memory[(fcb + 32) & 0xffff];
		std::fstream file(found->second, std::ios::binary | std::ios::in | std::ios::out);
		if (!file) {
			Fail("cannot reopen " + found->second);
			return 0xff;
		}
		file.seekp(static_cast<std::streamoff>(record) * kRecordSize);
		for (int i = 0; i < kRecordSize; ++i) {
			file.put(static_cast<char>(memory[(dma + i) & 0xffff]));
		}
		if (!file) {
			Fail("write error on " + found->second);
			return 0xff;
		}
		std::uint8_t cr = static_cast<std::uint8_t>(memory[(fcb + 32) & 0xffff] + 1);
		if (cr == 128) {
			cr = 0;
			++memory[(fcb + 12) & 0xffff];
		}
		memory[(fcb + 32) & 0xffff] = cr;
		return 0;
	}

	std::uint8_t Bdos() {
		Z80::Register &reg = cpu->reg;
		const std::uint8_t function = reg.pair.C;
		const std::uint16_t de = static_cast<std::uint16_t>((reg.pair.D << 8) | reg.pair.E);
		switch (function) {
		case 2:
			Put(static_cast<char>(reg.pair.E));
			return 0;
		case 9:
			for (std::uint16_t address = de; memory[address] != '$';
			     address = static_cast<std::uint16_t>(address + 1)) {
				Put(static_cast<char>(memory[address]));
			}
			return 0;
		case 13:
			dma = 0x0080;
			return 0;
		case 14:
			return 0;
		case 15: {
			const std::string path = HostPath(de);
			std::ifstream file(path, std::ios::binary);
			if (path.empty() || !file) {
				return 0xff;
			}
			ResetFcbPosition(de);
			open_files[de] = path;
			return 0;
		}
		case 16:
			// Close updates the directory only; CP/M 2.2 allows further
			// sequential writes through the same FCB (used as a checkpoint).
			return open_files.count(de) != 0 ? 0 : 0xff;
		case 19: {
			const std::string path = HostPath(de);
			if (path.empty()) {
				return 0xff;
			}
			return std::remove(path.c_str()) == 0 ? 0 : 0xff;
		}
		case 21:
			return WriteSequential(de);
		case 22: {
			const std::string path = HostPath(de);
			std::ofstream file(path, std::ios::binary | std::ios::trunc);
			if (path.empty() || !file) {
				return 0xff;
			}
			ResetFcbPosition(de);
			open_files[de] = path;
			return 0;
		}
		case 26:
			dma = de;
			return 0;
		default:
			Fail("unsupported BDOS function " + std::to_string(function));
			return 0xff;
		}
	}

	void Put(char c) {
		console.push_back(c);
		std::cout.put(c);
	}

	static unsigned char Read(void *opaque, unsigned short address) {
		return static_cast<Machine *>(opaque)->memory[address];
	}

	static void Write(void *opaque, unsigned short address, unsigned char value) {
		static_cast<Machine *>(opaque)->memory[address] = value;
	}

	static unsigned char Input(void *opaque, unsigned short port) {
		Machine *machine = static_cast<Machine *>(opaque);
		if ((port & 0xff) != kTrapPort) {
			machine->Fail("unexpected input port");
			return 0xff;
		}
		const std::uint8_t result = machine->Bdos();
		// CP/M returns the result in A and in HL (L = A, H = B = 0).
		machine->cpu->reg.pair.L = result;
		machine->cpu->reg.pair.H = 0;
		machine->cpu->reg.pair.B = 0;
		return result;
	}

	static void Output(void *opaque, unsigned short port, unsigned char) {
		Machine *machine = static_cast<Machine *>(opaque);
		if ((port & 0xff) == kExitPort) {
			machine->finished = true;
			machine->cpu->requestBreak();
			return;
		}
		machine->Fail("unexpected output port");
	}

	// Command tail at 0080h, as the CCP stores it (length, then text).
	void SetTail(const std::string &tail) {
		const std::size_t length = tail.size() < 126 ? tail.size() : 126;
		memory[0x80] = static_cast<std::uint8_t>(length);
		for (std::size_t i = 0; i < length; ++i) {
			memory[0x81 + i] = static_cast<std::uint8_t>(tail[i]);
		}
		memory[0x81 + length] = 0;
	}

	bool Load(const std::string &path) {
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file) {
			std::cerr << "cpm-runner: cannot open " << path << '\n';
			return false;
		}
		const std::streamoff length = file.tellg();
		if (length <= 0 || length > static_cast<std::streamoff>(kBdosAddress - kLoadAddress)) {
			std::cerr << "cpm-runner: invalid program size " << length << '\n';
			return false;
		}
		file.seekg(0);
		file.read(reinterpret_cast<char *>(memory.data() + kLoadAddress), length);
		// 0000h: OUT (1),A ends the run on warm boot. 0005h: JP BDOS.
		const std::uint8_t page_zero[] = {
		    0xd3, kExitPort, 0x00, 0x00, 0x00, 0xc3, kBdosAddress & 0xff, kBdosAddress >> 8};
		std::memcpy(memory.data(), page_zero, sizeof(page_zero));
		// BDOS: IN A,(0) / RET.
		memory[kBdosAddress] = 0xdb;
		memory[kBdosAddress + 1] = kTrapPort;
		memory[kBdosAddress + 2] = 0xc9;
		return static_cast<bool>(file);
	}
};

void Usage(const char *program) {
	std::cerr << "usage: " << program
	          << " [--profile zilog|upd9002] [--dir host_directory] [--console file]"
	             " [--tail text]"
	             " [--max-clocks n] program.com\n";
}

} // namespace

// Native-side physical page 0 (the x86 IVT) of a real PC-88VA2 running
// CP/MVA in V3 mode, reconstructed from the EDPRB4 C sweep (run 8,
// docs/agents/reports/m102_zexund_qa). Only FlagProfile::Upd9002 reads it,
// for the undefined ED 00-3F/74/75/77.
static const unsigned char kUpd9002VectorPage[256] = {
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
	0x1F, 0x23, 0x00, 0xF0, 0x8C, 0x73, 0x00, 0xF0, 0x7E, 0x03, 0x00, 0xF0, 0x8C, 0x03, 0x00, 0xF0,
	0x00, 0x28, 0x00, 0xF0, 0x9A, 0x03, 0x00, 0xF0, 0x1F, 0x23, 0x00, 0xF0, 0x1F, 0x23, 0x00, 0xF0,
	0x2A, 0x23, 0x00, 0xF0, 0x6D, 0x03, 0x00, 0xF0, 0x2A, 0x23, 0x00, 0xF0, 0xA8, 0x03, 0x00, 0xF0,
	0x2A, 0x23, 0x00, 0xF0, 0x16, 0x0D, 0x00, 0xF0, 0x2A, 0x23, 0x00, 0xF0, 0x2A, 0x23, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
	0x26, 0xA4, 0x00, 0xF0, 0x03, 0x94, 0x00, 0xF0, 0xD3, 0x07, 0x00, 0xF0, 0x09, 0x00, 0x00, 0xF0,
	0x06, 0x00, 0x00, 0xF0, 0x50, 0xA4, 0x00, 0xF0, 0x52, 0xA4, 0x00, 0xF0, 0xDC, 0xA4, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0xB6, 0x03, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
	0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0, 0x29, 0x23, 0x00, 0xF0,
};

int main(int argc, char **argv) {
	std::string program;
	std::string console_path;
	std::string directory = ".";
	std::uint64_t max_clocks = kDefaultMaxClocks;
	std::string tail;
	Z80::FlagProfile profile = Z80::FlagProfile::Zilog;
	for (int i = 1; i < argc; ++i) {
		const std::string argument = argv[i];
		if (argument == "--profile" && i + 1 < argc) {
			const std::string value = argv[++i];
			if (value == "zilog") {
				profile = Z80::FlagProfile::Zilog;
			} else if (value == "upd9002") {
				profile = Z80::FlagProfile::Upd9002;
			} else {
				Usage(argv[0]);
				return 2;
			}
		} else if (argument == "--dir" && i + 1 < argc) {
			directory = argv[++i];
		} else if (argument == "--console" && i + 1 < argc) {
			console_path = argv[++i];
		} else if (argument == "--tail" && i + 1 < argc) {
			tail = argv[++i];
		} else if (argument == "--max-clocks" && i + 1 < argc) {
			max_clocks = std::strtoull(argv[++i], nullptr, 10);
		} else if (!argument.empty() && argument[0] != '-' && program.empty()) {
			program = argument;
		} else {
			Usage(argv[0]);
			return 2;
		}
	}
	if (program.empty() || max_clocks == 0) {
		Usage(argv[0]);
		return 2;
	}

	Machine machine;
	machine.directory = directory;
	if (!machine.Load(program)) {
		return 1;
	}
	machine.SetTail(tail);
	Z80 cpu(&Machine::Read, &Machine::Write, &Machine::Input, &Machine::Output, &machine);
	machine.cpu = &cpu;
	cpu.setFlagProfile(profile);
	cpu.upd9002VectorRead = [](void *, unsigned char index) -> unsigned char {
		return kUpd9002VectorPage[index];
	};
	cpu.reg.PC = kLoadAddress;
	cpu.reg.SP = kBdosAddress;
	std::uint64_t clocks = 0;
	while (!machine.finished && clocks < max_clocks) {
		clocks += static_cast<std::uint64_t>(cpu.execute(kClockBatch));
	}
	std::cout.flush();
	if (!console_path.empty()) {
		std::ofstream console(console_path, std::ios::binary | std::ios::trunc);
		console << machine.console;
	}
	if (machine.failed) {
		std::cerr << "cpm-runner: " << machine.failure << '\n';
		return 1;
	}
	if (!machine.finished) {
		std::cerr << "cpm-runner: emulated-clock limit reached\n";
		return 1;
	}
	return 0;
}
