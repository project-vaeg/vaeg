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

#ifndef CPU_Z80_COMPAT_BUS_H
#define CPU_Z80_COMPAT_BUS_H

#include <cstdint>

#ifndef IFCALL
#if defined(_MSC_VER)
#define IFCALL __stdcall
#else
#define IFCALL
#endif
#endif

#ifndef IOCALL
#if defined(_MSC_VER)
#define IOCALL __stdcall
#else
#define IOCALL
#endif
#endif

struct IMemoryAccess {
	virtual std::uint32_t IFCALL Read8(std::uint32_t address) = 0;
	virtual void IFCALL Write8(std::uint32_t address, std::uint32_t data) = 0;
};

struct IIOAccess {
	virtual std::uint32_t IFCALL In(std::uint32_t port) = 0;
	virtual void IFCALL Out(std::uint32_t port, std::uint32_t data) = 0;
};

struct IClock {
	virtual std::uint32_t IFCALL now() = 0;
};

struct IClockCounter {
	virtual void IFCALL past(std::int32_t clocks) = 0;
	virtual std::int32_t IFCALL GetRemainclock() = 0;
	virtual void IFCALL SetRemainclock(std::int32_t clocks) = 0;
};

#endif
