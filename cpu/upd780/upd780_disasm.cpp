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

#include "upd780_disasm.h"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

constexpr unsigned kMaximumIndexPrefixes = 32;

enum class IndexMode {
	none,
	ix,
	iy
};

std::string Format(const char *format, ...) {
	std::array<char, 160> buffer{};
	va_list arguments;
	va_start(arguments, format);
	const int count = std::vsnprintf(buffer.data(), buffer.size(), format, arguments);
	va_end(arguments);
	if (count < 0) {
		return "<format-error>";
	}
	return std::string(buffer.data(),
	                   std::min<std::size_t>(static_cast<std::size_t>(count), buffer.size() - 1));
}

std::string Hex8(std::uint8_t value) {
	return Format("0x%02x", static_cast<unsigned>(value));
}

std::string Hex16(std::uint16_t value) {
	return Format("0x%04x", static_cast<unsigned>(value));
}

const char *IndexName(IndexMode index) {
	return index == IndexMode::iy ? "iy" : "ix";
}

std::string IndexMemory(IndexMode index, std::uint8_t displacement) {
	const int signed_displacement =
	    displacement < 0x80 ? static_cast<int>(displacement) : static_cast<int>(displacement) - 256;
	if (signed_displacement < 0) {
		return Format("(%s-0x%02x)", IndexName(index), static_cast<unsigned>(-signed_displacement));
	}
	return Format("(%s+0x%02x)", IndexName(index), static_cast<unsigned>(signed_displacement));
}

std::string RegisterName(unsigned code, IndexMode index, bool substitute_index_halves = true) {
	static const char *const names[] = {"b", "c", "d", "e", "h", "l", "(hl)", "a"};
	if (index != IndexMode::none && substitute_index_halves) {
		if (code == 4) {
			return std::string(IndexName(index)) + "h";
		}
		if (code == 5) {
			return std::string(IndexName(index)) + "l";
		}
	}
	return names[code & 7U];
}

std::string PairName(unsigned pair, IndexMode index, bool af_pair = false) {
	static const char *const regular[] = {"bc", "de", "hl", "sp"};
	static const char *const stack[] = {"bc", "de", "hl", "af"};
	if (pair == 2 && index != IndexMode::none) {
		return IndexName(index);
	}
	return af_pair ? stack[pair & 3U] : regular[pair & 3U];
}

const char *ConditionName(unsigned condition) {
	static const char *const names[] = {"nz", "z", "nc", "c", "po", "pe", "p", "m"};
	return names[condition & 7U];
}

std::string AluOperation(unsigned operation, const std::string &operand) {
	switch (operation & 7U) {
	case 0:
		return "add a, " + operand;
	case 1:
		return "adc a, " + operand;
	case 2:
		return "sub " + operand;
	case 3:
		return "sbc a, " + operand;
	case 4:
		return "and " + operand;
	case 5:
		return "xor " + operand;
	case 6:
		return "or " + operand;
	default:
		return "cp " + operand;
	}
}

class Decoder {
  public:
	Decoder(std::uint16_t pc, VaegUpd780DisasmRead read, void *opaque)
	    : start_(pc), cursor_(pc), read_(read), opaque_(opaque), byte_count_(0),
	      status_(VAEG_UPD780_DISASM_OK) {
	}

	std::string Decode() {
		IndexMode index = IndexMode::none;
		unsigned prefix_count = 0;
		std::uint8_t opcode = Read();

		while (opcode == 0xdd || opcode == 0xfd) {
			index = opcode == 0xdd ? IndexMode::ix : IndexMode::iy;
			++prefix_count;
			opcode = Read();
			if (prefix_count == kMaximumIndexPrefixes && (opcode == 0xdd || opcode == 0xfd)) {
				status_ = VAEG_UPD780_DISASM_PREFIX_LIMIT;
				return "<invalid-prefix-sequence>";
			}
		}

		if (opcode == 0xcb) {
			if (index == IndexMode::none) {
				return DecodeCb(Read());
			}
			const std::uint8_t displacement = Read();
			return DecodeIndexedCb(index, displacement, Read());
		}
		if (opcode == 0xed) {
			return DecodeEd(Read());
		}
		return DecodeBase(opcode, index);
	}

	VaegUpd780DisasmResult Result() const {
		if (status_ != VAEG_UPD780_DISASM_OK) {
			return {start_, 0, status_};
		}
		return {cursor_, static_cast<std::uint8_t>(byte_count_), status_};
	}

  private:
	std::uint8_t Read() {
		const std::uint8_t value = read_(opaque_, cursor_);
		if (byte_count_ < bytes_.size()) {
			bytes_[byte_count_] = value;
		}
		++byte_count_;
		cursor_ = static_cast<std::uint16_t>(cursor_ + 1U);
		return value;
	}

	std::uint16_t ReadWord() {
		const std::uint8_t low = Read();
		const std::uint8_t high = Read();
		return static_cast<std::uint16_t>(static_cast<std::uint16_t>(low) |
		                                  (static_cast<std::uint16_t>(high) << 8));
	}

	std::string RelativeTarget() {
		const std::uint8_t encoded = Read();
		const int displacement =
		    encoded < 0x80 ? static_cast<int>(encoded) : static_cast<int>(encoded) - 256;
		const std::uint16_t target =
		    static_cast<std::uint16_t>(static_cast<std::uint32_t>(cursor_) + displacement);
		return Hex16(target);
	}

	std::string ByteDirective() const {
		std::string result = "db ";
		for (std::size_t i = 0; i < byte_count_ && i < bytes_.size(); ++i) {
			if (i != 0) {
				result += ", ";
			}
			result += Hex8(bytes_[i]);
		}
		return result;
	}

	std::string DecodeBase(std::uint8_t opcode, IndexMode index) {
		const unsigned x = opcode >> 6;
		const unsigned y = (opcode >> 3) & 7U;
		const unsigned z = opcode & 7U;
		const unsigned p = y >> 1;
		const unsigned q = y & 1U;

		if (x == 0) {
			switch (z) {
			case 0:
				if (y == 0) {
					return "nop";
				}
				if (y == 1) {
					return "ex af, af'";
				}
				if (y == 2) {
					return "djnz " + RelativeTarget();
				}
				if (y == 3) {
					return "jr " + RelativeTarget();
				}
				return std::string("jr ") + ConditionName(y - 4) + ", " + RelativeTarget();
			case 1:
				if (q == 0) {
					return "ld " + PairName(p, index) + ", " + Hex16(ReadWord());
				}
				return "add " + PairName(2, index) + ", " + PairName(p, index);
			case 2:
				if (q == 0) {
					switch (p) {
					case 0:
						return "ld (bc), a";
					case 1:
						return "ld (de), a";
					case 2:
						return "ld (" + Hex16(ReadWord()) + "), " + PairName(2, index);
					default:
						return "ld (" + Hex16(ReadWord()) + "), a";
					}
				}
				switch (p) {
				case 0:
					return "ld a, (bc)";
				case 1:
					return "ld a, (de)";
				case 2:
					return "ld " + PairName(2, index) + ", (" + Hex16(ReadWord()) + ")";
				default:
					return "ld a, (" + Hex16(ReadWord()) + ")";
				}
			case 3:
				return std::string(q == 0 ? "inc " : "dec ") + PairName(p, index);
			case 4:
			case 5: {
				std::string operand;
				if (index != IndexMode::none && y == 6) {
					operand = IndexMemory(index, Read());
				} else {
					operand = RegisterName(y, index);
				}
				return std::string(z == 4 ? "inc " : "dec ") + operand;
			}
			case 6: {
				std::string destination;
				if (index != IndexMode::none && y == 6) {
					destination = IndexMemory(index, Read());
				} else {
					destination = RegisterName(y, index);
				}
				return "ld " + destination + ", " + Hex8(Read());
			}
			default: {
				static const char *const operations[] = {"rlca", "rrca", "rla", "rra",
				                                         "daa",  "cpl",  "scf", "ccf"};
				return operations[y];
			}
			}
		}

		if (x == 1) {
			if (y == 6 && z == 6) {
				return "halt";
			}
			if (index != IndexMode::none && (y == 6 || z == 6)) {
				const std::string memory = IndexMemory(index, Read());
				const std::string destination = y == 6 ? memory : RegisterName(y, index, false);
				const std::string source = z == 6 ? memory : RegisterName(z, index, false);
				return "ld " + destination + ", " + source;
			}
			return "ld " + RegisterName(y, index) + ", " + RegisterName(z, index);
		}

		if (x == 2) {
			const std::string operand = index != IndexMode::none && z == 6
			                                ? IndexMemory(index, Read())
			                                : RegisterName(z, index);
			return AluOperation(y, operand);
		}

		switch (z) {
		case 0:
			return std::string("ret ") + ConditionName(y);
		case 1:
			if (q == 0) {
				return "pop " + PairName(p, index, true);
			}
			switch (p) {
			case 0:
				return "ret";
			case 1:
				return "exx";
			case 2:
				return "jp (" + PairName(2, index) + ")";
			default:
				return "ld sp, " + PairName(2, index);
			}
		case 2:
			return std::string("jp ") + ConditionName(y) + ", " + Hex16(ReadWord());
		case 3:
			switch (y) {
			case 0:
				return "jp " + Hex16(ReadWord());
			case 1:
				return "<unexpected-cb-prefix>";
			case 2:
				return "out (" + Hex8(Read()) + "), a";
			case 3:
				return "in a, (" + Hex8(Read()) + ")";
			case 4:
				return "ex (sp), " + PairName(2, index);
			case 5:
				return "ex de, hl";
			case 6:
				return "di";
			default:
				return "ei";
			}
		case 4:
			return std::string("call ") + ConditionName(y) + ", " + Hex16(ReadWord());
		case 5:
			if (q == 0) {
				return "push " + PairName(p, index, true);
			}
			if (p == 0) {
				return "call " + Hex16(ReadWord());
			}
			return "<unexpected-prefix>";
		case 6:
			return AluOperation(y, Hex8(Read()));
		default:
			return "rst " + Hex8(static_cast<std::uint8_t>(y * 8U));
		}
	}

	std::string DecodeCb(std::uint8_t opcode) {
		const unsigned x = opcode >> 6;
		const unsigned y = (opcode >> 3) & 7U;
		const unsigned z = opcode & 7U;
		static const char *const shifts[] = {"rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl"};
		if (x == 0) {
			return std::string(shifts[y]) + " " + RegisterName(z, IndexMode::none);
		}
		if (x == 1) {
			return Format("bit %u, %s", y, RegisterName(z, IndexMode::none).c_str());
		}
		return Format("%s %u, %s", x == 2 ? "res" : "set", y,
		              RegisterName(z, IndexMode::none).c_str());
	}

	std::string DecodeIndexedCb(IndexMode index, std::uint8_t displacement, std::uint8_t opcode) {
		const unsigned x = opcode >> 6;
		const unsigned y = (opcode >> 3) & 7U;
		const unsigned z = opcode & 7U;
		static const char *const shifts[] = {"rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl"};
		const std::string memory = IndexMemory(index, displacement);
		if (x == 1) {
			return Format("bit %u, %s", y, memory.c_str());
		}
		const std::string operation = x == 0 ? shifts[y] : (x == 2 ? "res" : "set");
		const std::string bit = x == 0 ? "" : Format(" %u,", y);
		const std::string target = z == 6 ? "" : ", " + RegisterName(z, IndexMode::none);
		return operation + bit + (x == 0 ? " " : " ") + memory + target;
	}

	std::string DecodeEd(std::uint8_t opcode) {
		const unsigned x = opcode >> 6;
		const unsigned y = (opcode >> 3) & 7U;
		const unsigned z = opcode & 7U;
		const unsigned p = y >> 1;
		const unsigned q = y & 1U;

		if (x == 1) {
			switch (z) {
			case 0:
				if (y == 6) {
					return "in (c)";
				}
				return "in " + RegisterName(y, IndexMode::none) + ", (c)";
			case 1:
				if (y == 6) {
					return "out (c), 0";
				}
				return "out (c), " + RegisterName(y, IndexMode::none);
			case 2:
				return std::string(q == 0 ? "sbc hl, " : "adc hl, ") + PairName(p, IndexMode::none);
			case 3: {
				const std::uint16_t address = ReadWord();
				if (q == 0) {
					return "ld (" + Hex16(address) + "), " + PairName(p, IndexMode::none);
				}
				return "ld " + PairName(p, IndexMode::none) + ", (" + Hex16(address) + ")";
			}
			case 4:
				return "neg";
			case 5:
				return opcode == 0x4d ? "reti" : "retn";
			case 6: {
				static const unsigned modes[] = {0, 0, 1, 2, 0, 0, 1, 2};
				return Format("im %u", modes[y]);
			}
			default: {
				static const char *const operations[] = {"ld i, a", "ld r, a", "ld a, i", "ld a, r",
				                                         "rrd",     "rld",     nullptr,   nullptr};
				if (operations[y] != nullptr) {
					return operations[y];
				}
				return ByteDirective();
			}
			}
		}

		if (x == 2 && y >= 4 && z <= 3) {
			static const char *const block[4][4] = {{"ldi", "ldd", "ldir", "lddr"},
			                                        {"cpi", "cpd", "cpir", "cpdr"},
			                                        {"ini", "ind", "inir", "indr"},
			                                        {"outi", "outd", "otir", "otdr"}};
			return block[z][y - 4];
		}
		return ByteDirective();
	}

	std::uint16_t start_;
	std::uint16_t cursor_;
	VaegUpd780DisasmRead read_;
	void *opaque_;
	std::array<std::uint8_t, kMaximumIndexPrefixes + 8> bytes_{};
	std::size_t byte_count_;
	std::uint8_t status_;
};

void CopyOutput(const std::string &text, char *destination, std::uint32_t capacity) {
	if (destination == nullptr || capacity == 0) {
		return;
	}
	const std::size_t available = static_cast<std::size_t>(capacity - 1U);
	const std::size_t count = std::min(text.size(), available);
	if (count != 0) {
		std::memcpy(destination, text.data(), count);
	}
	destination[count] = '\0';
}

} // namespace

VaegUpd780DisasmResult VaegUpd780Disassemble(std::uint16_t pc, char *destination,
                                             std::uint32_t capacity, VaegUpd780DisasmRead read,
                                             void *opaque) {
	if (read == nullptr) {
		CopyOutput("<invalid-reader>", destination, capacity);
		return {pc, 0, VAEG_UPD780_DISASM_INVALID_READER};
	}
	Decoder decoder(pc, read, opaque);
	const std::string text = decoder.Decode();
	CopyOutput(text, destination, capacity);
	return decoder.Result();
}
