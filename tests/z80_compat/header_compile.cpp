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

#include <array>
#include <cstdint>

#if defined(VAEG_Z80_COMPAT_REQUIRE_DEBUG) && defined(Z80_DISABLE_DEBUG)
#error "the debug/default compile target must retain the debug API"
#endif

#if defined(VAEG_Z80_COMPAT_REQUIRE_RELEASE) && !defined(Z80_DISABLE_DEBUG)
#error "the release compile target must disable the debug API"
#endif

namespace {

struct Bus {
	std::array<std::uint8_t, 65536> memory{};
	std::uint8_t acknowledge = 0x7f;
};

unsigned char read_memory(void *opaque, unsigned short address) {
	Bus *bus = static_cast<Bus *>(opaque);
	return bus->memory[address];
}

void write_memory(void *opaque, unsigned short address, unsigned char value) {
	Bus *bus = static_cast<Bus *>(opaque);
	bus->memory[address] = value;
}

unsigned char input(void *, unsigned short) {
	return 0xff;
}

void output(void *, unsigned short, unsigned char) {
}

unsigned char acknowledge(void *opaque) {
	Bus *bus = static_cast<Bus *>(opaque);
	return bus->acknowledge;
}

#ifndef Z80_DISABLE_DEBUG
void debug_message(void *, const char *) {
}
#endif

} // namespace

int main() {
	Bus bus;
	Z80 cpu(read_memory, write_memory, input, output, &bus);
	cpu.setInterruptAcknowledgeCallback(acknowledge);
	cpu.setIRQLine(true);
	if (!cpu.isIRQLineAsserted()) {
		return 1;
	}
	cpu.setIRQLine(false);
#ifndef Z80_DISABLE_DEBUG
	cpu.setDebugMessage(debug_message);
	cpu.resetDebugMessage();
#endif
	cpu.reg.PC = 0;
	cpu.execute(1);
	return cpu.reg.PC == 1 ? 0 : 1;
}
