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

#include "cpu/upd780/upd780_disasm.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef VAEG_UPD780_DISASM_GOLDEN_PATH
#error VAEG_UPD780_DISASM_GOLDEN_PATH must name the reviewed golden file
#endif

namespace {

constexpr std::uint32_t kCpuStateCookie = UINT32_C(0x4d343044);

[[noreturn]] void Fail(const std::string &message) {
	std::fprintf(stderr, "upd780-disasm: %s\n", message.c_str());
	std::exit(1);
}

void Require(bool condition, const std::string &message) {
	if (!condition) {
		Fail(message);
	}
}

struct Memory {
	std::array<std::uint8_t, 65536> bytes{};
	std::vector<std::uint16_t> reads;
	std::uint32_t cpu_state_cookie = kCpuStateCookie;

	static std::uint8_t Read(void *opaque, std::uint16_t address) {
		Memory *memory = static_cast<Memory *>(opaque);
		memory->reads.push_back(address);
		return memory->bytes[address];
	}

	void Install(std::uint16_t address, const std::vector<std::uint8_t> &values) {
		for (std::uint8_t value : values) {
			bytes[address] = value;
			address = static_cast<std::uint16_t>(address + 1U);
		}
	}
};

std::uint8_t ExpectedLength(const Memory &memory, std::uint16_t start) {
	std::uint16_t cursor = start;
	auto read = [&]() {
		const std::uint8_t value = memory.bytes[cursor];
		cursor = static_cast<std::uint16_t>(cursor + 1U);
		return value;
	};

	bool indexed = false;
	unsigned prefixes = 0;
	std::uint8_t opcode = read();
	while (opcode == 0xdd || opcode == 0xfd) {
		indexed = true;
		++prefixes;
		opcode = read();
		if (prefixes == 32 && (opcode == 0xdd || opcode == 0xfd)) {
			return 0;
		}
	}

	if (opcode == 0xcb) {
		if (indexed) {
			read();
		}
		read();
		return static_cast<std::uint8_t>(static_cast<std::uint16_t>(cursor - start));
	}

	if (opcode == 0xed) {
		const std::uint8_t extended = read();
		const unsigned x = extended >> 6;
		const unsigned z = extended & 7U;
		if (x == 1 && z == 3) {
			read();
			read();
		}
		return static_cast<std::uint8_t>(static_cast<std::uint16_t>(cursor - start));
	}

	const unsigned x = opcode >> 6;
	const unsigned y = (opcode >> 3) & 7U;
	const unsigned z = opcode & 7U;
	const unsigned p = y >> 1;
	const unsigned q = y & 1U;
	unsigned extra = 0;

	if (x == 0) {
		if (z == 0 && y >= 2) {
			extra = 1;
		} else if (z == 1 && q == 0) {
			extra = 2;
		} else if (z == 2 && p >= 2) {
			extra = 2;
		} else if ((z == 4 || z == 5) && indexed && y == 6) {
			extra = 1;
		} else if (z == 6) {
			extra = 1 + (indexed && y == 6 ? 1U : 0U);
		}
	} else if (x == 1) {
		if (indexed && !(y == 6 && z == 6) && (y == 6 || z == 6)) {
			extra = 1;
		}
	} else if (x == 2) {
		if (indexed && z == 6) {
			extra = 1;
		}
	} else if (z == 2 || z == 4) {
		extra = 2;
	} else if (z == 3) {
		if (y == 0) {
			extra = 2;
		} else if (y == 2 || y == 3) {
			extra = 1;
		}
	} else if (z == 5 && q != 0 && p == 0) {
		extra = 2;
	} else if (z == 6) {
		extra = 1;
	}

	for (unsigned i = 0; i < extra; ++i) {
		read();
	}
	return static_cast<std::uint8_t>(static_cast<std::uint16_t>(cursor - start));
}

std::string CaseName(const char *page, unsigned opcode, unsigned displacement = 0x100) {
	if (displacement <= 0xff) {
		char buffer[80];
		std::snprintf(buffer, sizeof(buffer), "%s opcode=%02x displacement=%02x", page, opcode,
		              displacement);
		return buffer;
	}
	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), "%s opcode=%02x", page, opcode);
	return buffer;
}

std::string RunCase(const std::string &name, std::uint16_t pc,
                    const std::vector<std::uint8_t> &bytes) {
	Memory memory;
	memory.Install(pc, bytes);
	const std::uint8_t expected_length = ExpectedLength(memory, pc);
	Require(expected_length != 0, name + ": expected-length classifier failed");

	std::array<char, 68> guarded{};
	guarded.fill(static_cast<char>(0x5a));
	char *const destination = guarded.data() + 2;
	memory.reads.clear();
	const VaegUpd780DisasmResult result =
	    VaegUpd780Disassemble(pc, destination, 64, &Memory::Read, &memory);

	Require(result.status == VAEG_UPD780_DISASM_OK, name + ": decoder returned an error status");
	Require(result.length == expected_length, name + ": instruction length mismatch");
	Require(result.next_pc == static_cast<std::uint16_t>(pc + expected_length),
	        name + ": next PC mismatch");
	Require(destination[0] != '\0', name + ": empty output");
	Require(guarded[0] == static_cast<char>(0x5a) && guarded[1] == static_cast<char>(0x5a) &&
	            guarded[66] == static_cast<char>(0x5a) && guarded[67] == static_cast<char>(0x5a),
	        name + ": destination guard was overwritten");
	Require(memory.cpu_state_cookie == kCpuStateCookie, name + ": CPU-state sentinel changed");
	Require(memory.reads.size() == expected_length, name + ": unexpected memory-read count");
	for (std::size_t i = 0; i < memory.reads.size(); ++i) {
		Require(memory.reads[i] == static_cast<std::uint16_t>(pc + i),
		        name + ": memory-read address/order mismatch");
	}

	const std::string first(destination);
	std::array<char, 64> repeated{};
	memory.reads.clear();
	const VaegUpd780DisasmResult second = VaegUpd780Disassemble(
	    pc, repeated.data(), static_cast<std::uint32_t>(repeated.size()), &Memory::Read, &memory);
	Require(second.next_pc == result.next_pc && second.length == result.length &&
	            second.status == result.status && repeated.data() == first,
	        name + ": output was not deterministic");
	return first;
}

void TestExhaustivePages() {
	std::size_t cases = 0;
	for (unsigned opcode = 0; opcode < 256; ++opcode) {
		RunCase(CaseName("base", opcode), 0x2000,
		        {static_cast<std::uint8_t>(opcode), 0, 0, 0, 0, 0});
		RunCase(CaseName("cb", opcode), 0x2000, {0xcb, static_cast<std::uint8_t>(opcode), 0, 0});
		RunCase(CaseName("ed", opcode), 0x2000, {0xed, static_cast<std::uint8_t>(opcode), 0, 0});
		RunCase(CaseName("dd", opcode), 0x2000,
		        {0xdd, static_cast<std::uint8_t>(opcode), 0, 0, 0, 0});
		RunCase(CaseName("fd", opcode), 0x2000,
		        {0xfd, static_cast<std::uint8_t>(opcode), 0, 0, 0, 0});
		cases += 5;
	}

	constexpr std::uint8_t displacements[] = {0x80, 0xff, 0x00, 0x01, 0x7f};
	for (std::uint8_t displacement : displacements) {
		for (unsigned opcode = 0; opcode < 256; ++opcode) {
			RunCase(CaseName("ddcb", opcode, displacement), 0x3000,
			        {0xdd, 0xcb, displacement, static_cast<std::uint8_t>(opcode)});
			RunCase(CaseName("fdcb", opcode, displacement), 0x3000,
			        {0xfd, 0xcb, displacement, static_cast<std::uint8_t>(opcode)});
			cases += 2;
		}
	}

	RunCase("repeated-dd-fd", 0xfffc, {0xdd, 0xdd, 0xfd, 0x21, 0x34, 0x12});
	RunCase("ignored-index-ed", 0xfffd, {0xfd, 0xed, 0x43, 0x34, 0x12});
	RunCase("wrap-immediate", 0xffff, {0x3e, 0x42});
	RunCase("wrap-ddcb", 0xfffe, {0xdd, 0xcb, 0xff, 0x36});
	cases += 4;

	Require(cases == 3844, "exhaustive corpus case count changed");
	std::printf("upd780-disasm: exhaustive pages passed (%zu cases)\n", cases);
}

unsigned ParseHex(const std::string &text) {
	char *end = nullptr;
	const unsigned long value = std::strtoul(text.c_str(), &end, 16);
	Require(end != text.c_str() && *end == '\0',
	        "invalid hexadecimal value in golden file: " + text);
	return static_cast<unsigned>(value);
}

std::vector<std::uint8_t> ParseBytes(const std::string &text) {
	std::istringstream input(text);
	std::vector<std::uint8_t> result;
	std::string token;
	while (input >> token) {
		const unsigned value = ParseHex(token);
		Require(value <= 0xff, "golden byte is out of range");
		result.push_back(static_cast<std::uint8_t>(value));
	}
	Require(!result.empty(), "golden row has no bytes");
	return result;
}

void TestGoldenOutput() {
	std::ifstream input{VAEG_UPD780_DISASM_GOLDEN_PATH};
	Require(input.good(), "could not open reviewed golden file");
	std::string line;
	unsigned line_number = 0;
	unsigned cases = 0;
	while (std::getline(input, line)) {
		++line_number;
		if (line.empty() || line[0] == '#') {
			continue;
		}
		const std::size_t first = line.find('|');
		const std::size_t second = line.find('|', first + 1);
		const std::size_t third = line.find('|', second + 1);
		Require(first != std::string::npos && second != std::string::npos &&
		            third != std::string::npos,
		        "malformed golden row at line " + std::to_string(line_number));
		const std::uint16_t pc = static_cast<std::uint16_t>(ParseHex(line.substr(0, first)));
		const std::vector<std::uint8_t> bytes =
		    ParseBytes(line.substr(first + 1, second - first - 1));
		const std::uint16_t expected_next =
		    static_cast<std::uint16_t>(ParseHex(line.substr(second + 1, third - second - 1)));
		const std::string expected_text = line.substr(third + 1);
		const std::string name = "golden line " + std::to_string(line_number);
		const std::string actual_text = RunCase(name, pc, bytes);
		Memory length_memory;
		length_memory.Install(pc, bytes);
		Require(static_cast<std::uint16_t>(pc + ExpectedLength(length_memory, pc)) == expected_next,
		        name + ": reviewed next PC mismatch");
		Require(actual_text == expected_text, name + ": text mismatch: expected '" + expected_text +
		                                          "', got '" + actual_text + "'");
		++cases;
	}
	Require(cases >= 30, "golden corpus is unexpectedly small");
	std::printf("upd780-disasm: reviewed golden output passed (%u cases)\n", cases);
}

void TestBufferAndMalformedInputs() {
	Memory memory;
	memory.Install(0x4000, {0xdd, 0x36, 0xfe, 0x7f});

	char untouched = 'Q';
	const VaegUpd780DisasmResult zero =
	    VaegUpd780Disassemble(0x4000, &untouched, 0, &Memory::Read, &memory);
	Require(untouched == 'Q' && zero.length == 4,
	        "zero-capacity output was written or decoded incorrectly");

	std::array<char, 2> one{{'Q', 'Q'}};
	VaegUpd780Disassemble(0x4000, one.data(), 1, &Memory::Read, &memory);
	Require(one[0] == '\0' && one[1] == 'Q', "one-byte output did not remain bounded");

	std::array<char, 64> full{};
	VaegUpd780Disassemble(0x4000, full.data(), full.size(), &Memory::Read, &memory);
	const std::size_t exact_capacity = std::strlen(full.data()) + 1;
	std::vector<char> exact(exact_capacity + 1, 'Q');
	VaegUpd780Disassemble(0x4000, exact.data(), static_cast<std::uint32_t>(exact_capacity),
	                      &Memory::Read, &memory);
	Require(std::strcmp(exact.data(), full.data()) == 0 && exact.back() == 'Q',
	        "exact-capacity output was truncated or overrun");

	std::array<char, 6> truncated{{'Q', 'Q', 'Q', 'Q', 'Q', 'Q'}};
	VaegUpd780Disassemble(0x4000, truncated.data(), 5, &Memory::Read, &memory);
	Require(std::strcmp(truncated.data(), "ld (") == 0 && truncated[5] == 'Q',
	        "truncated output was not terminated within capacity");

	std::array<char, 4096> large{};
	VaegUpd780Disassemble(0x4000, large.data(), large.size(), &Memory::Read, &memory);
	Require(std::strcmp(large.data(), full.data()) == 0,
	        "large-capacity output changed the result");

	std::array<char, 32> invalid{};
	const VaegUpd780DisasmResult invalid_reader =
	    VaegUpd780Disassemble(0x1234, invalid.data(), invalid.size(), nullptr, nullptr);
	Require(invalid_reader.status == VAEG_UPD780_DISASM_INVALID_READER &&
	            invalid_reader.length == 0 && invalid_reader.next_pc == 0x1234 &&
	            std::strcmp(invalid.data(), "<invalid-reader>") == 0,
	        "null reader was not rejected safely");

	Memory prefixes;
	prefixes.bytes.fill(0xdd);
	std::array<char, 64> prefix_text{};
	const VaegUpd780DisasmResult prefix_result = VaegUpd780Disassemble(
	    0xfff0, prefix_text.data(), prefix_text.size(), &Memory::Read, &prefixes);
	Require(prefix_result.status == VAEG_UPD780_DISASM_PREFIX_LIMIT && prefix_result.length == 0 &&
	            prefix_result.next_pc == 0xfff0 && prefixes.reads.size() == 33 &&
	            std::strcmp(prefix_text.data(), "<invalid-prefix-sequence>") == 0,
	        "unbounded prefix stream was not rejected deterministically");

	std::printf("upd780-disasm: buffer and malformed-input tests passed\n");
}

} // namespace

int main() {
	TestExhaustivePages();
	TestGoldenOutput();
	TestBufferAndMalformedInputs();
	std::printf("upd780-disasm: all tests passed\n");
	return 0;
}
