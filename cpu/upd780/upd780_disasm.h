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

#ifndef CPU_UPD780_UPD780_DISASM_H
#define CPU_UPD780_UPD780_DISASM_H

#include <cstdint>

using VaegUpd780DisasmRead = std::uint8_t (*)(void *opaque, std::uint16_t address);

enum VaegUpd780DisasmStatus : std::uint8_t {
	VAEG_UPD780_DISASM_OK = 0,
	VAEG_UPD780_DISASM_INVALID_READER = 1,
	VAEG_UPD780_DISASM_PREFIX_LIMIT = 2
};

struct VaegUpd780DisasmResult {
	std::uint16_t next_pc;
	std::uint8_t length;
	std::uint8_t status;
};

VaegUpd780DisasmResult VaegUpd780Disassemble(std::uint16_t pc, char *destination,
                                             std::uint32_t capacity, VaegUpd780DisasmRead read,
                                             void *opaque);

#endif
