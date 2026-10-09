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

#ifndef CPU_Z80_COMPAT_REGISTERS_H
#define CPU_Z80_COMPAT_REGISTERS_H

#include <cstdint>

struct UPD780Reg {
	std::uint16_t af;
	std::uint16_t hl;
	std::uint16_t de;
	std::uint16_t bc;
	std::uint16_t ix;
	std::uint16_t iy;
	std::uint16_t sp;
	std::uint16_t r_af;
	std::uint16_t r_hl;
	std::uint16_t r_de;
	std::uint16_t r_bc;
	std::uint16_t pc;
	std::uint8_t ireg;
	std::uint8_t rreg;
	std::uint8_t rreg7;
	std::uint8_t intmode;
	bool iff1;
	bool iff2;
};

using Z80CompatReg = UPD780Reg;
using UPD70008Reg = UPD780Reg;

#endif
