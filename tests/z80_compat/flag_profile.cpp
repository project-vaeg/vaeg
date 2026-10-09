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

// M101 core tests for Z80::FlagProfile and the ADC/SBC HL carry fix.
//
// The probe table is the M101 FLAGPRB table: every expected value was
// derived by hand from the Z80 flag definitions (Zilog) and rules R1-R7
// (Upd9002). The ADC/SBC checks compare the core with an independent
// arithmetic model over operands that exercise the carry chain.

#include "z80.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using Profile = Z80::FlagProfile;

struct Bus {
	std::array<std::uint8_t, 65536> memory{};
};

unsigned char ReadMemory(void *opaque, unsigned short address) {
	return static_cast<Bus *>(opaque)->memory[address];
}

void WriteMemory(void *opaque, unsigned short address, unsigned char value) {
	static_cast<Bus *>(opaque)->memory[address] = value;
}

unsigned char Input(void *, unsigned short) {
	return 0xff;
}

void Output(void *, unsigned short, unsigned char) {
}

constexpr std::uint16_t kCode = 0x0100;
constexpr std::uint16_t kSource = 0x4000;
constexpr std::uint16_t kDestination = 0x5000;

struct Result {
	std::uint8_t a;
	std::uint8_t f;
	std::uint16_t bc;
	std::uint16_t hl;
	std::uint8_t destination;
};

// Load AF through PUSH BC / POP AF, run the body, then capture AF through
// PUSH AF / POP DE (as FLAGPRB captures with PUSH AF).
Result Run(Profile profile, std::uint16_t af, std::uint16_t bc, std::uint16_t hl,
           const std::vector<std::uint8_t> &body, bool capture_to_bc = false) {
	Bus bus;
	Z80 cpu(ReadMemory, WriteMemory, Input, Output, &bus);
	cpu.setFlagProfile(profile);
	std::vector<std::uint8_t> code = {
	    0x01, static_cast<std::uint8_t>(af), static_cast<std::uint8_t>(af >> 8), // LD BC,af
	    0xc5,                                                                    // PUSH BC
	    0xf1,                                                                    // POP AF
	    0x01, static_cast<std::uint8_t>(bc), static_cast<std::uint8_t>(bc >> 8), // LD BC,bc
	    0x21, static_cast<std::uint8_t>(hl), static_cast<std::uint8_t>(hl >> 8), // LD HL,hl
	    0x11, kDestination & 0xff,           kDestination >> 8,                  // LD DE,dst
	};
	code.insert(code.end(), body.begin(), body.end());
	if (!capture_to_bc) {
		code.push_back(0xf5); // PUSH AF
		code.push_back(0xd1); // POP DE
	}
	code.push_back(0x76); // HALT
	for (std::size_t i = 0; i < code.size(); ++i) {
		bus.memory[kCode + i] = code[i];
	}
	cpu.reg.PC = kCode;
	cpu.reg.SP = 0xf000;
	const std::uint16_t end = static_cast<std::uint16_t>(kCode + code.size());
	for (int guard = 0; guard < 1000 && cpu.reg.PC != end; ++guard) {
		cpu.execute(1);
	}
	Result result{};
	result.a = cpu.reg.pair.A;
	result.f = capture_to_bc ? cpu.reg.pair.F : cpu.reg.pair.E;
	result.bc = static_cast<std::uint16_t>((cpu.reg.pair.B << 8) | cpu.reg.pair.C);
	result.hl = static_cast<std::uint16_t>((cpu.reg.pair.H << 8) | cpu.reg.pair.L);
	result.destination = bus.memory[kDestination];
	return result;
}

int failures = 0;

void Expect(const char *name, const char *field, unsigned actual, unsigned expected) {
	if (actual != expected) {
		std::printf("FAIL %s %s: got %02x expected %02x\n", name, field, actual, expected);
		++failures;
	}
}

struct Probe {
	const char *name;
	std::uint16_t af;
	std::uint16_t bc;
	std::uint16_t hl;
	std::vector<std::uint8_t> body;
	int a_zilog;
	std::uint8_t f_zilog;
	std::uint8_t f_upd9002;
	int bc_expected;
	int hl_expected;
};

void RunProbeTable() {
	const std::vector<std::uint8_t> ldi = {0xed, 0xa0};
	const std::vector<Probe> probes = {
	    {"P0 XOR A", 0x5a00, 0, 0, {0xaf}, 0x00, 0x44, 0x44, -1, -1},
	    {"P1 AND 0Fh", 0xff00, 0, 0, {0xe6, 0x0f}, 0x0f, 0x1c, 0x04, -1, -1},
	    {"P2 BIT 0,A", 0x0100, 0, 0, {0xcb, 0x47}, 0x01, 0x10, 0x00, -1, -1},
	    {"P3 RLCA", 0x80d7, 0, 0, {0x07}, 0x01, 0xc5, 0xd7, -1, -1},
	    {"P4a LDI", 0x00d7, 0x0001, kSource, ldi, 0x00, 0xc1, 0xd3, 0x0000, kSource + 1},
	    {"P4b LDI", 0x00d7, 0x0002, kSource, ldi, 0x00, 0xc5, 0xd7, 0x0001, kSource + 1},
	    {"P5a ADD HL,BC", 0x0000, 0x0001, 0x0fff, {0x09}, 0x00, 0x10, 0x00, 0x0001, 0x1000},
	    {"P5b ADD HL,BC", 0x0010, 0x0001, 0x0001, {0x09}, 0x00, 0x00, 0x10, 0x0001, 0x0002},
	    {"P6a ADC HL,BC", 0x0000, 0x0001, 0x000f, {0xed, 0x4a}, 0x00, 0x00, 0x10, 0x0001, 0x0010},
	    {"P6b SBC HL,BC", 0x0000, 0x0001, 0x0010, {0xed, 0x42}, 0x00, 0x02, 0x12, 0x0001, 0x000f},
	    // R8-R11 (DAA/CPL/SCF/CCF); expected values derived by hand.
	    {"R8 CPL", 0x0000, 0, 0, {0x2f}, 0xff, 0x3a, 0x00, -1, -1},
	    {"R8 CPL F=D7h", 0x00d7, 0, 0, {0x2f}, 0xff, 0xff, 0xd7, -1, -1},
	    {"R9 SCF H,N", 0x0012, 0, 0, {0x37}, 0x00, 0x01, 0x13, -1, -1},
	    {"R10 CCF C", 0x0013, 0, 0, {0x3f}, 0x00, 0x10, 0x12, -1, -1},
	    {"R10 CCF", 0x0012, 0, 0, {0x3f}, 0x00, 0x01, 0x13, -1, -1},
	    {"R11 DAA 00h", 0x0000, 0, 0, {0x27}, 0x00, 0x44, 0x40, -1, -1},
	    {"R11 DAA 7Ah", 0x7a00, 0, 0, {0x27}, 0x80, 0x90, 0x94, -1, -1},
	};
	for (const Probe &probe : probes) {
		for (const Profile profile : {Profile::Zilog, Profile::Upd9002}) {
			const Result result = Run(profile, probe.af, probe.bc, probe.hl, probe.body);
			const bool zilog = profile == Profile::Zilog;
			char name[64];
			std::snprintf(name, sizeof(name), "%s [%s]", probe.name, zilog ? "zilog" : "upd9002");
			Expect(name, "A", result.a, static_cast<unsigned>(probe.a_zilog));
			Expect(name, "F", result.f, zilog ? probe.f_zilog : probe.f_upd9002);
			if (probe.bc_expected >= 0) {
				Expect(name, "BC", result.bc, static_cast<unsigned>(probe.bc_expected));
			}
			if (probe.hl_expected >= 0) {
				Expect(name, "HL", result.hl, static_cast<unsigned>(probe.hl_expected));
			}
		}
	}

	// P7: storage of F bits 5/3 through POP AF, EX AF,AF' and a flag-neutral
	// instruction. The Upd9002 values are the R1 storage-variant prediction.
	struct StorageProbe {
		const char *name;
		std::uint16_t bc;
		std::vector<std::uint8_t> body;
		std::uint8_t c_zilog;
		std::uint8_t c_upd9002;
	};
	const std::vector<StorageProbe> storage = {
	    {"P7a POP AF", 0xffff, {0xc5, 0xf1, 0xf5, 0xc1}, 0xff, 0xd7},
	    {"P7b EX AF,AF'", 0xffff, {0xc5, 0xf1, 0x08, 0x08, 0xf5, 0xc1}, 0xff, 0xd7},
	    {"P7c POP AF", 0x0028, {0xc5, 0xf1, 0xf5, 0xc1}, 0x28, 0x00},
	    {"P7d INC DE", 0xffff, {0xc5, 0xf1, 0x13, 0xf5, 0xc1}, 0xff, 0xd7},
	};
	for (const StorageProbe &probe : storage) {
		for (const Profile profile : {Profile::Zilog, Profile::Upd9002}) {
			const Result result = Run(profile, 0x0000, probe.bc, 0, probe.body, true);
			const bool zilog = profile == Profile::Zilog;
			char name[64];
			std::snprintf(name, sizeof(name), "%s [%s]", probe.name, zilog ? "zilog" : "upd9002");
			Expect(name, "C", result.bc & 0xff, zilog ? probe.c_zilog : probe.c_upd9002);
		}
	}
}

// Independent model of ADC/SBC HL,rr flags.
std::uint8_t ModelFlags(bool subtract, std::uint16_t hl, std::uint16_t rr, int carry,
                        Profile profile) {
	const int result = subtract ? hl - rr - carry : hl + rr + carry;
	const std::uint16_t value = static_cast<std::uint16_t>(result);
	const int signed_result =
	    subtract ? static_cast<std::int16_t>(hl) - static_cast<std::int16_t>(rr) - carry
	             : static_cast<std::int16_t>(hl) + static_cast<std::int16_t>(rr) + carry;
	std::uint8_t f = 0;
	f |= (value & 0x8000) ? 0x80 : 0;
	f |= value == 0 ? 0x40 : 0;
	f |= (signed_result > 32767 || signed_result < -32768) ? 0x04 : 0;
	f |= subtract ? 0x02 : 0;
	f |= (result < 0 || result > 0xffff) ? 0x01 : 0;
	if (profile == Profile::Zilog) {
		const int half = subtract ? (hl & 0x0fff) - (rr & 0x0fff) - carry
		                          : (hl & 0x0fff) + (rr & 0x0fff) + carry;
		f |= (half < 0 || half > 0x0fff) ? 0x10 : 0;
		f |= (value >> 8) & 0x28;
	} else {
		const int half = subtract ? (hl & 0x000f) - (rr & 0x000f) - carry
		                          : (hl & 0x000f) + (rr & 0x000f) + carry;
		f |= (half < 0 || half > 0x000f) ? 0x10 : 0;
	}
	return f;
}

void RunAdcSbc() {
	const std::uint16_t values[] = {0x0000, 0x0001, 0x000f, 0x0010, 0x00ff, 0x0100, 0x0fff,
	                                0x1000, 0x3fff, 0x7ffe, 0x7fff, 0x8000, 0x8001, 0xefff,
	                                0xf000, 0xfffe, 0xffff, 0x1234, 0xa5a5, 0x5a5a};
	for (const Profile profile : {Profile::Zilog, Profile::Upd9002}) {
		for (const bool subtract : {false, true}) {
			for (const std::uint16_t hl : values) {
				for (const std::uint16_t rr : values) {
					for (int carry = 0; carry <= 1; ++carry) {
						const std::vector<std::uint8_t> body = {
						    0xed, static_cast<std::uint8_t>(subtract ? 0x42 : 0x4a)};
						const Result result =
						    Run(profile, static_cast<std::uint16_t>(carry), rr, hl, body);
						const std::uint16_t expected_hl = static_cast<std::uint16_t>(
						    subtract ? hl - rr - carry : hl + rr + carry);
						char name[96];
						std::snprintf(name, sizeof(name), "%s HL=%04x BC=%04x C=%d [%s]",
						              subtract ? "SBC" : "ADC", hl, rr, carry,
						              profile == Profile::Zilog ? "zilog" : "upd9002");
						Expect(name, "HL", result.hl, expected_hl);
						Expect(name, "F", result.f, ModelFlags(subtract, hl, rr, carry, profile));
					}
				}
			}
		}
	}
}

// R11: with H=1 the high step needs A > 9Fh (Zilog: A > 99h), so A itself
// differs for A = 9Ah-9Fh.
void RunDaaHighStep() {
	struct Case {
		const char *name;
		std::uint16_t af;
		std::uint8_t a_zilog, f_zilog, a_upd9002, f_upd9002;
	};
	const Case cases[] = {
	    {"R11 DAA 9Ah H", 0x9a10, 0x00, 0x55, 0xa0, 0x90},
	    {"R11 DAA 9Ah H N", 0x9a12, 0x34, 0x23, 0x94, 0x92},
	};
	for (const Case &item : cases) {
		for (const Profile profile : {Profile::Zilog, Profile::Upd9002}) {
			const Result result = Run(profile, item.af, 0, 0, {0x27});
			const bool zilog = profile == Profile::Zilog;
			char name[64];
			std::snprintf(name, sizeof(name), "%s [%s]", item.name, zilog ? "zilog" : "upd9002");
			Expect(name, "A", result.a, zilog ? item.a_zilog : item.a_upd9002);
			Expect(name, "F", result.f, zilog ? item.f_zilog : item.f_upd9002);
		}
	}
}

#if defined(VAEG_M101_QA_DIR)
// Every record of the real PC-88VA2 DAA/CPL/SCF/CCF dumps: A = a,
// F = fi spread over S, Z, H, P/V, N, C; output A, F.
void RunRealMachineDumps() {
	const struct {
		const char *file;
		std::uint8_t opcode;
	} dumps[] = {{"daa.bin", 0x27}, {"cpl.bin", 0x2f}, {"scf.bin", 0x37}, {"ccf.bin", 0x3f}};
	const std::uint8_t spread[6] = {0x01, 0x02, 0x04, 0x10, 0x40, 0x80};
	for (const auto &dump : dumps) {
		const std::string path = std::string(VAEG_M101_QA_DIR) + "/" + dump.file;
		std::FILE *file = std::fopen(path.c_str(), "rb");
		std::vector<std::uint8_t> data(32768);
		const bool read =
		    file != nullptr && std::fread(data.data(), 1, data.size(), file) == data.size();
		if (file != nullptr) {
			std::fclose(file);
		}
		if (!read) {
			std::printf("FAIL cannot read %s\n", path.c_str());
			++failures;
			continue;
		}
		int mismatches = 0;
		for (int fi = 0; fi < 64; ++fi) {
			std::uint8_t f = 0;
			for (int bit = 0; bit < 6; ++bit) {
				if ((fi >> bit) & 1) {
					f = static_cast<std::uint8_t>(f | spread[bit]);
				}
			}
			for (int a = 0; a < 256; ++a) {
				const Result result =
				    Run(Profile::Upd9002, static_cast<std::uint16_t>((a << 8) | f), 0, 0,
				        {dump.opcode});
				const std::size_t offset = static_cast<std::size_t>((fi * 256 + a) * 2);
				if (result.a != data[offset] || result.f != data[offset + 1]) {
					if (mismatches++ < 5) {
						std::printf("FAIL %s fi=%d A=%02x: got %02x/%02x real %02x/%02x\n",
						            dump.file, fi, a, result.a, result.f, data[offset],
						            data[offset + 1]);
					}
				}
			}
		}
		failures += mismatches;
	}
}
#endif

#if defined(VAEG_M102_QA_DIR)
// R12/R13: every record of the real PC-88VA2 CBPRB run (DD/FD CB 02 xx, two
// input sets). IX/IY hold program addresses in the real run, so only A, F,
// BC, DE, HL and the probed byte are compared.
void RunRealMachineCbprb() {
	const std::string path = std::string(VAEG_M102_QA_DIR) + "/cbprb.txt";
	std::FILE *file = std::fopen(path.c_str(), "rb");
	if (file == nullptr) {
		std::printf("FAIL cannot read %s\n", path.c_str());
		++failures;
		return;
	}
	char line[128];
	int records = 0;
	int mismatches = 0;
	while (std::fgets(line, sizeof(line), file) != nullptr) {
		unsigned prefix, op, set, a, f, bc, de, hl, ix, iy, m;
		if (std::sscanf(line,
		                "%2x CB 02 %2x %u A=%2x F=%2x BC=%4x DE=%4x HL=%4x IX=%4x IY=%4x M=%2x",
		                &prefix, &op, &set, &a, &f, &bc, &de, &hl, &ix, &iy, &m) != 11) {
			continue;
		}
		++records;
		Bus bus;
		Z80 cpu(ReadMemory, WriteMemory, Input, Output, &bus);
		cpu.setFlagProfile(Profile::Upd9002);
		const std::uint16_t probe = 0x6000;
		bus.memory[probe] = set == 0 ? 0x81 : 0x7e;
		const std::uint8_t code[] = {0x01,
		                             static_cast<std::uint8_t>(set == 0 ? 0x00 : 0xd7),
		                             0x0a, // LD BC,AF
		                             0xc5,
		                             0xf1, // PUSH BC, POP AF
		                             0x01,
		                             0x0c,
		                             0x0b,
		                             0x11,
		                             0x0e,
		                             0x0d,
		                             0x21,
		                             0x55,
		                             0x44, // BC, DE, HL
		                             0xdd,
		                             0x21,
		                             (probe - 2) & 0xff,
		                             (probe - 2) >> 8, // LD IX
		                             0xfd,
		                             0x21,
		                             (probe - 2) & 0xff,
		                             (probe - 2) >> 8, // LD IY
		                             static_cast<std::uint8_t>(prefix),
		                             0xcb,
		                             0x02,
		                             static_cast<std::uint8_t>(op),
		                             0xf5,
		                             0xe1}; // PUSH AF, POP HL -> F in L
		for (std::size_t i = 0; i < sizeof(code); ++i) {
			bus.memory[kCode + i] = code[i];
		}
		cpu.reg.PC = kCode;
		cpu.reg.SP = 0xf000;
		const std::uint16_t end = static_cast<std::uint16_t>(kCode + sizeof(code) - 2);
		while (cpu.reg.PC != end) {
			cpu.execute(1);
		}
		const unsigned got_a = cpu.reg.pair.A;
		const unsigned got_f = cpu.reg.pair.F;
		const unsigned got_bc = static_cast<unsigned>((cpu.reg.pair.B << 8) | cpu.reg.pair.C);
		const unsigned got_de = static_cast<unsigned>((cpu.reg.pair.D << 8) | cpu.reg.pair.E);
		const unsigned got_hl = static_cast<unsigned>((cpu.reg.pair.H << 8) | cpu.reg.pair.L);
		if (got_a != a || got_f != f || got_bc != bc || got_de != de || got_hl != hl ||
		    bus.memory[probe] != m) {
			if (mismatches++ < 5) {
				std::printf("FAIL cbprb %02X CB 02 %02X set %u\n", prefix, op, set);
			}
		}
	}
	std::fclose(file);
	if (records != 1024) {
		std::printf("FAIL cbprb: %d records, expected 1024\n", records);
		++failures;
	}
	failures += mismatches;
}
#endif

// Upstream 1.11.0 sets WZ to the effective address before an IX/IY-prefixed
// CB instruction, so Zilog BIT n,(i+d) takes X/Y from the high byte of
// i+d. Under Upd9002 bits 5/3 stay 0 (R1) and H is 0 (R3).
void RunIndexedBitWz() {
	struct Case {
		std::uint8_t prefix;
		std::uint8_t op;
		std::uint8_t zilog_f;
		std::uint8_t upd9002_f;
	};
	// (i+d) = 28FFh holds 80h; bit 0 is clear and bit 7 is set. 28h has
	// bits 5 and 3 set, so the Zilog results carry 28h.
	const Case cases[] = {
	    {0xdd, 0x46, 0x7c, 0x44}, // BIT 0,(IX+d): Z, H, P/V, X/Y
	    {0xdd, 0x7e, 0xb8, 0x80}, // BIT 7,(IX+d): S, H, X/Y
	    {0xfd, 0x46, 0x7c, 0x44},
	    {0xfd, 0x7e, 0xb8, 0x80},
	};
	for (const Case &c : cases) {
		for (Profile profile : {Profile::Zilog, Profile::Upd9002}) {
			Bus bus;
			Z80 cpu(ReadMemory, WriteMemory, Input, Output, &bus);
			cpu.setFlagProfile(profile);
			bus.memory[0x28ff] = 0x80;
			const std::uint8_t code[] = {c.prefix, 0xcb, 0x7f, c.op}; // (i+7Fh)
			for (std::size_t i = 0; i < sizeof(code); ++i) {
				bus.memory[kCode + i] = code[i];
			}
			cpu.reg.PC = kCode;
			cpu.reg.IX = 0x2880;
			cpu.reg.IY = 0x2880;
			cpu.reg.WZ = 0x0000; // a stale WZ would give X/Y = 0
			cpu.reg.pair.F = 0x00;
			cpu.execute(1);
			const std::uint8_t expected = profile == Profile::Zilog ? c.zilog_f : c.upd9002_f;
			if (cpu.reg.pair.F != expected || cpu.reg.WZ != 0x28ff) {
				std::printf("FAIL indexed BIT %02X CB 7F %02X %s: F=%02X WZ=%04X "
				            "(expected F=%02X WZ=28FF)\n",
				            c.prefix, c.op, profile == Profile::Zilog ? "Zilog" : "Upd9002",
				            cpu.reg.pair.F, cpu.reg.WZ, expected);
				++failures;
			}
		}
	}
}

} // namespace

int main() {
	RunProbeTable();
	RunDaaHighStep();
	RunIndexedBitWz();
#if defined(VAEG_M101_QA_DIR)
	RunRealMachineDumps();
#endif
#if defined(VAEG_M102_QA_DIR)
	RunRealMachineCbprb();
#endif
	RunAdcSbc();
	if (failures != 0) {
		std::printf("flag-profile: %d failure(s)\n", failures);
		return 1;
	}
	std::printf("flag-profile: PASS\n");
	return 0;
}
