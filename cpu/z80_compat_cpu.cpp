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

#include "z80_compat_cpu.h"

#include "z80_compat_state.h"
#include "z80.hpp"

#include <cstring>
#include <limits>
#include <new>

namespace {

constexpr std::uint8_t kIff1 = 0x01;
constexpr std::uint8_t kIff2 = 0x04;
constexpr std::uint8_t kIffNmi = 0x40;
constexpr std::uint8_t kIffHalt = 0x80;

bool IsOpcodePrefix(std::uint8_t value) {
	return value == 0xcb || value == 0xdd || value == 0xed || value == 0xfd;
}

std::uint16_t MakeWord(std::uint8_t high, std::uint8_t low) {
	return static_cast<std::uint16_t>((static_cast<std::uint16_t>(high) << 8) | low);
}

void SplitWord(std::uint16_t value, std::uint8_t *high, std::uint8_t *low) {
	*high = static_cast<std::uint8_t>(value >> 8);
	*low = static_cast<std::uint8_t>(value);
}

std::int32_t SignedFromBits(std::uint32_t value) {
	if (value <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
		return static_cast<std::int32_t>(value);
	}
	return static_cast<std::int32_t>(static_cast<std::int64_t>(value) - (INT64_C(1) << 32));
}

std::int32_t AddClockDelta(std::int32_t balance, std::uint32_t delta) {
	return SignedFromBits(static_cast<std::uint32_t>(balance) + delta);
}

Z80CompatReg ExportRegisters(const Z80::Register &source) {
	Z80CompatReg result{};
	result.af = MakeWord(source.pair.A, source.pair.F);
	result.hl = MakeWord(source.pair.H, source.pair.L);
	result.de = MakeWord(source.pair.D, source.pair.E);
	result.bc = MakeWord(source.pair.B, source.pair.C);
	result.ix = source.IX;
	result.iy = source.IY;
	result.sp = source.SP;
	result.r_af = MakeWord(source.back.A, source.back.F);
	result.r_hl = MakeWord(source.back.H, source.back.L);
	result.r_de = MakeWord(source.back.D, source.back.E);
	result.r_bc = MakeWord(source.back.B, source.back.C);
	result.pc = source.PC;
	result.ireg = source.I;
	result.rreg = source.R & 0x7f;
	result.rreg7 = source.R & 0x80;
	result.intmode = source.interrupt & 0x03;
	result.iff1 = (source.IFF & kIff1) != 0;
	result.iff2 = (source.IFF & kIff2) != 0;
	return result;
}

void ImportRegisters(const Z80CompatReg &source, bool halted, bool ei_inhibited,
                     Z80::Register *destination) {
	std::memset(destination, 0, sizeof(*destination));
	SplitWord(source.af, &destination->pair.A, &destination->pair.F);
	SplitWord(source.hl, &destination->pair.H, &destination->pair.L);
	SplitWord(source.de, &destination->pair.D, &destination->pair.E);
	SplitWord(source.bc, &destination->pair.B, &destination->pair.C);
	SplitWord(source.r_af, &destination->back.A, &destination->back.F);
	SplitWord(source.r_hl, &destination->back.H, &destination->back.L);
	SplitWord(source.r_de, &destination->back.D, &destination->back.E);
	SplitWord(source.r_bc, &destination->back.B, &destination->back.C);
	destination->PC = source.pc;
	destination->SP = source.sp;
	destination->IX = source.ix;
	destination->IY = source.iy;
	destination->R = static_cast<std::uint8_t>((source.rreg & 0x7f) | (source.rreg7 & 0x80));
	destination->I = source.ireg;
	destination->IFF = static_cast<std::uint8_t>(
	    (source.iff1 ? kIff1 : 0) | (source.iff2 ? kIff2 : 0) | (halted ? kIffHalt : 0));
	destination->interrupt = source.intmode & 0x03;
	destination->execEI = ei_inhibited ? 1 : 0;
}

} // namespace

struct Z80CompatCpu::Impl {
	explicit Impl(Z80CompatCpu *owner)
	    : cpu(&Z80CompatCpu::ReadMemory, &Z80CompatCpu::WriteMemory, &Z80CompatCpu::Input,
	          &Z80CompatCpu::Output, owner, false) {
		cpu.setConsumeClockCallback(&Z80CompatCpu::ConsumeClock);
		cpu.setInterruptAcknowledgeCallback(&Z80CompatCpu::Acknowledge);
		cpu.upd9002VectorRead = &Z80CompatCpu::NativeVector;
	}

	Z80 cpu;
};

Z80CompatCpu::Z80CompatCpu()
    : impl_(nullptr), memory_(nullptr), bus_(nullptr), clock_(nullptr), clockcounter_(nullptr),
      lastclock_(0), acknowledge_port_(0), external_wait_(false), irq_asserted_(false),
      instruction_fetch_started_(false), prefix_fetch_pending_(false), first_opcode_(0),
      prefixed_opcode_(0), restore_iff1_after_instruction_(false),
      materialize_i_flags_after_instruction_(false), materialize_r_flags_after_instruction_(false),
      code_base_(0), data_base_(0), flag_profile_(Z80CompatFlagProfile::kZilog),
      public_registers_{} {
}

Z80CompatCpu::~Z80CompatCpu() {
	delete impl_;
}

bool Z80CompatCpu::Init(IMemoryAccess *memory, IIOAccess *bus, IClock *clock,
                        IClockCounter *clockcounter, int interrupt_acknowledge_port) {
	if (memory == nullptr || bus == nullptr || clock == nullptr || clockcounter == nullptr) {
		return false;
	}

	delete impl_;
	impl_ = nullptr;
	memory_ = memory;
	bus_ = bus;
	clock_ = clock;
	clockcounter_ = clockcounter;
	acknowledge_port_ = interrupt_acknowledge_port;
	impl_ = new (std::nothrow) Impl(this);
	if (impl_ == nullptr) {
		return false;
	}
	Reset();
	ApplyFlagProfile();
	return true;
}

void Z80CompatCpu::Exec() {
	if (impl_ == nullptr || clock_ == nullptr || clockcounter_ == nullptr) {
		return;
	}

	const std::uint32_t now = clock_->now();
	const std::uint32_t delta = now - static_cast<std::uint32_t>(lastclock_);
	clockcounter_->SetRemainclock(AddClockDelta(clockcounter_->GetRemainclock(), delta));

	if (external_wait_) {
		const std::int32_t remaining = clockcounter_->GetRemainclock();
		if (remaining > 0) {
			clockcounter_->past(remaining);
		}
	} else {
		while (clockcounter_->GetRemainclock() > 0) {
			ExecuteOne();
		}
	}

	SynchronizePublicMirror();
	lastclock_ = SignedFromBits(now);
}

void Z80CompatCpu::ExecOne() {
	if (impl_ == nullptr || clockcounter_ == nullptr) {
		return;
	}
	if (external_wait_) {
		const std::int32_t remaining = clockcounter_->GetRemainclock();
		if (remaining > 0) {
			clockcounter_->past(remaining);
		}
	} else {
		ExecuteOne();
	}
	SynchronizePublicMirror();
}

void IOCALL Z80CompatCpu::Reset(std::uint32_t, std::uint32_t) {
	if (impl_ == nullptr) {
		public_registers_ = Z80CompatReg{};
		return;
	}

	std::memset(&impl_->cpu.reg, 0, sizeof(impl_->cpu.reg));
	impl_->cpu.setIRQLine(false);
	external_wait_ = false;
	irq_asserted_ = false;
	if (clockcounter_ != nullptr) {
		clockcounter_->SetRemainclock(0);
	}
	lastclock_ = clock_ != nullptr ? SignedFromBits(clock_->now()) : 0;
	SynchronizePublicMirror();
}

void IOCALL Z80CompatCpu::IRQ(std::uint32_t, std::uint32_t asserted) {
	irq_asserted_ = asserted != 0;
	if (impl_ != nullptr) {
		impl_->cpu.setIRQLine(irq_asserted_);
	}
}

void IOCALL Z80CompatCpu::NMI(std::uint32_t, std::uint32_t) {
	if (impl_ == nullptr || memory_ == nullptr || clockcounter_ == nullptr) {
		return;
	}

	const std::uint16_t mirrored_pc = public_registers_.pc;
	Z80::Register &reg = impl_->cpu.reg;
	reg.R = static_cast<std::uint8_t>(((reg.R + 1) & 0x7f) | (reg.R & 0x80));
	if ((reg.IFF & kIff1) != 0) {
		reg.IFF |= kIff2;
	} else {
		reg.IFF &= static_cast<std::uint8_t>(~kIff2);
	}
	reg.IFF = static_cast<std::uint8_t>((reg.IFF & ~kIffHalt) | kIffNmi);
	reg.IFF &= static_cast<std::uint8_t>(~kIff1);
	const std::uint16_t pc = reg.PC;
	reg.SP = static_cast<std::uint16_t>(reg.SP - 1);
	memory_->Write8(TranslateDataAddress(reg.SP), static_cast<std::uint8_t>(pc >> 8));
	reg.SP = static_cast<std::uint16_t>(reg.SP - 1);
	memory_->Write8(TranslateDataAddress(reg.SP), static_cast<std::uint8_t>(pc));
	reg.PC = 0x0066;
	clockcounter_->past(11);
	SynchronizePublicMirror();
	public_registers_.pc = mirrored_pc;
}

void Z80CompatCpu::Wait(bool asserted) {
	external_wait_ = asserted;
}

std::uint32_t IFCALL Z80CompatCpu::GetStatusSize() {
	return static_cast<std::uint32_t>(vaeg::z80_compat::kRevision1Size);
}

bool IFCALL Z80CompatCpu::SaveStatus(std::uint8_t *status) {
	if (impl_ == nullptr || clockcounter_ == nullptr || status == nullptr) {
		return false;
	}

	vaeg::z80_compat::LegacyState state;
	state.registers = ExportRegisters(impl_->cpu.reg);
	state.halted = (impl_->cpu.reg.IFF & kIffHalt) != 0;
	state.external_wait = external_wait_;
	state.irq_asserted = irq_asserted_;
	state.ei_inhibited = impl_->cpu.reg.execEI != 0;
	state.remainclock = clockcounter_->GetRemainclock();
	state.lastclock = lastclock_;
	vaeg::z80_compat::EncodeRevision1(state, status);
	return true;
}

bool IFCALL Z80CompatCpu::LoadStatus(const std::uint8_t *status) {
	if (impl_ == nullptr || clockcounter_ == nullptr || status == nullptr) {
		return false;
	}

	vaeg::z80_compat::LegacyState state;
	if (!vaeg::z80_compat::DecodeRevision1(status, &state)) {
		return false;
	}

	ImportRegisters(state.registers, state.halted, state.ei_inhibited, &impl_->cpu.reg);
	ApplyFlagProfile();
	external_wait_ = state.external_wait;
	irq_asserted_ = state.irq_asserted;
	impl_->cpu.setIRQLine(irq_asserted_);
	clockcounter_->SetRemainclock(state.remainclock);
	lastclock_ = state.lastclock;
	SynchronizePublicMirror();
	return true;
}

std::uint32_t Z80CompatCpu::GetPC() {
	return impl_ != nullptr ? impl_->cpu.reg.PC : 0;
}

void Z80CompatCpu::SetPC(std::uint32_t new_pc) {
	const std::uint16_t pc = static_cast<std::uint16_t>(new_pc);
	if (impl_ != nullptr) {
		impl_->cpu.reg.PC = pc;
	}
	public_registers_.pc = pc;
}

void Z80CompatCpu::SetReg(const Z80CompatReg &source) {
	if (impl_ == nullptr) {
		public_registers_ = source;
		return;
	}
	ImportRegisters(source, false, false, &impl_->cpu.reg);
	ApplyFlagProfile();
	impl_->cpu.setIRQLine(irq_asserted_);
	SynchronizePublicMirror();
}

void Z80CompatCpu::SetMainReg(const Z80CompatReg &source) {
	if (impl_ == nullptr) {
		return;
	}
	SplitWord(source.af, &impl_->cpu.reg.pair.A, &impl_->cpu.reg.pair.F);
	SplitWord(source.hl, &impl_->cpu.reg.pair.H, &impl_->cpu.reg.pair.L);
	SplitWord(source.de, &impl_->cpu.reg.pair.D, &impl_->cpu.reg.pair.E);
	SplitWord(source.bc, &impl_->cpu.reg.pair.B, &impl_->cpu.reg.pair.C);
	impl_->cpu.reg.PC = source.pc;
	impl_->cpu.reg.SP = source.sp;
	impl_->cpu.reg.IX = source.ix;
	impl_->cpu.reg.IY = source.iy;
	impl_->cpu.reg.I = source.ireg;
	impl_->cpu.reg.R = static_cast<std::uint8_t>((source.rreg & 0x7f) | (source.rreg7 & 0x80));
	impl_->cpu.reg.IFF = static_cast<std::uint8_t>(
	    (impl_->cpu.reg.IFF & kIffHalt) | (source.iff1 ? kIff1 : 0) | (source.iff2 ? kIff2 : 0));
	impl_->cpu.reg.interrupt = source.intmode & 0x03;
	ApplyFlagProfile();
	SynchronizePublicMirror();
}

void Z80CompatCpu::SetMemoryBases(std::uint32_t code_base, std::uint32_t data_base) {
	code_base_ = code_base;
	data_base_ = data_base;
}

void Z80CompatCpu::SetAltReg(const Z80CompatReg &source) {
	if (impl_ == nullptr) {
		return;
	}
	SplitWord(source.r_af, &impl_->cpu.reg.back.A, &impl_->cpu.reg.back.F);
	SplitWord(source.r_hl, &impl_->cpu.reg.back.H, &impl_->cpu.reg.back.L);
	SplitWord(source.r_de, &impl_->cpu.reg.back.D, &impl_->cpu.reg.back.E);
	SplitWord(source.r_bc, &impl_->cpu.reg.back.B, &impl_->cpu.reg.back.C);
	ApplyFlagProfile();
	SynchronizePublicMirror();
}

void Z80CompatCpu::SetFlagProfile(Z80CompatFlagProfile profile) {
	flag_profile_ = profile;
	ApplyFlagProfile();
	SynchronizePublicMirror();
}

Z80CompatFlagProfile Z80CompatCpu::GetFlagProfile() const {
	return flag_profile_;
}

void Z80CompatCpu::SetNativeVectorRead(NativeVectorRead reader, void *opaque) {
	native_vector_read_ = reader;
	native_vector_opaque_ = opaque;
}

unsigned char Z80CompatCpu::NativeVector(void *opaque, unsigned char index) {
	Z80CompatCpu *cpu = static_cast<Z80CompatCpu *>(opaque);
	if (cpu->native_vector_read_ == nullptr) {
		return 0x00;
	}
	return cpu->native_vector_read_(cpu->native_vector_opaque_, index);
}

// Select the core profile. Setting the profile also applies the R1 storage
// rule to F and F' for kUpd9002, so every path that writes F from outside the
// core calls this afterwards.
void Z80CompatCpu::ApplyFlagProfile() {
	if (impl_ == nullptr) {
		return;
	}
	impl_->cpu.setFlagProfile(flag_profile_ == Z80CompatFlagProfile::kUpd9002
	                              ? Z80::FlagProfile::Upd9002
	                              : Z80::FlagProfile::Zilog);
}

const Z80CompatReg *Z80CompatCpu::GetReg() {
	return &public_registers_;
}

std::uint8_t Z80CompatCpu::ReadMemory(void *opaque, std::uint16_t address) {
	Z80CompatCpu *cpu = static_cast<Z80CompatCpu *>(opaque);
	const std::uint32_t translated = cpu->instruction_fetch_started_
	                                     ? cpu->TranslateDataAddress(address)
	                                     : cpu->TranslateCodeAddress(address);
	const std::uint8_t value = static_cast<std::uint8_t>(cpu->memory_->Read8(translated));
	if (!cpu->instruction_fetch_started_) {
		cpu->instruction_fetch_started_ = true;
		cpu->first_opcode_ = value;
		cpu->prefix_fetch_pending_ = IsOpcodePrefix(value);
	} else if (cpu->prefix_fetch_pending_) {
		Z80::Register &reg = cpu->impl_->cpu.reg;
		reg.R = static_cast<std::uint8_t>(((reg.R + 1) & 0x7f) | (reg.R & 0x80));
		cpu->prefixed_opcode_ = value;
		cpu->prefix_fetch_pending_ = false;
		if (cpu->first_opcode_ == 0xed) {
			if (value == 0x45 || value == 0x4d) {
				cpu->restore_iff1_after_instruction_ = true;
				if ((reg.IFF & kIff2) != 0) {
					reg.IFF |= kIff1;
				} else {
					reg.IFF &= static_cast<std::uint8_t>(~kIff1);
				}
			} else if (value == 0x57) {
				cpu->materialize_i_flags_after_instruction_ = true;
			} else if (value == 0x5f) {
				cpu->materialize_r_flags_after_instruction_ = true;
			}
		}
	}
	return value;
}

void Z80CompatCpu::WriteMemory(void *opaque, std::uint16_t address, std::uint8_t data) {
	Z80CompatCpu *cpu = static_cast<Z80CompatCpu *>(opaque);
	cpu->memory_->Write8(cpu->TranslateDataAddress(address), data);
}

std::uint8_t Z80CompatCpu::Input(void *opaque, std::uint16_t port) {
	Z80CompatCpu *cpu = static_cast<Z80CompatCpu *>(opaque);
	return static_cast<std::uint8_t>(cpu->bus_->In(port & 0xffU));
}

void Z80CompatCpu::Output(void *opaque, std::uint16_t port, std::uint8_t data) {
	Z80CompatCpu *cpu = static_cast<Z80CompatCpu *>(opaque);
	cpu->bus_->Out(port & 0xffU, data);
}

void Z80CompatCpu::ConsumeClock(void *opaque, int clocks) {
	Z80CompatCpu *cpu = static_cast<Z80CompatCpu *>(opaque);
	cpu->clockcounter_->past(static_cast<std::int32_t>(clocks));
}

std::uint8_t Z80CompatCpu::Acknowledge(void *opaque) {
	Z80CompatCpu *cpu = static_cast<Z80CompatCpu *>(opaque);
	const std::uint8_t value = static_cast<std::uint8_t>(
	    cpu->bus_->In(static_cast<std::uint32_t>(cpu->acknowledge_port_)));
	if (IsOpcodePrefix(value)) {
		cpu->instruction_fetch_started_ = true;
		cpu->first_opcode_ = value;
		cpu->prefixed_opcode_ = 0;
		cpu->prefix_fetch_pending_ = true;
	}
	return value;
}

void Z80CompatCpu::ExecuteOne() {
	instruction_fetch_started_ = false;
	prefix_fetch_pending_ = false;
	first_opcode_ = 0;
	prefixed_opcode_ = 0;
	restore_iff1_after_instruction_ = false;
	materialize_i_flags_after_instruction_ = false;
	materialize_r_flags_after_instruction_ = false;
	if ((impl_->cpu.reg.IFF & kIffHalt) != 0) {
		impl_->cpu.reg.R =
		    static_cast<std::uint8_t>(((impl_->cpu.reg.R + 1) & 0x7f) | (impl_->cpu.reg.R & 0x80));
	}
	impl_->cpu.execute(1);
	ApplyInstructionCorrections();
}

std::uint32_t Z80CompatCpu::TranslateCodeAddress(std::uint16_t address) const {
	return code_base_ + address;
}

std::uint32_t Z80CompatCpu::TranslateDataAddress(std::uint16_t address) const {
	return data_base_ + address;
}

void Z80CompatCpu::ApplyInstructionCorrections() {
	if (impl_ == nullptr) {
		return;
	}
	Z80::Register &reg = impl_->cpu.reg;
	if (restore_iff1_after_instruction_) {
		if ((reg.IFF & kIff2) != 0) {
			reg.IFF |= kIff1;
		} else {
			reg.IFF &= static_cast<std::uint8_t>(~kIff1);
		}
	}
	if (!materialize_i_flags_after_instruction_ && !materialize_r_flags_after_instruction_) {
		return;
	}
	const std::uint8_t value = materialize_i_flags_after_instruction_ ? reg.I : reg.R;
	constexpr std::uint8_t kCarry = 0x01;
	constexpr std::uint8_t kParityOverflow = 0x04;
	constexpr std::uint8_t kFlag3 = 0x08;
	constexpr std::uint8_t kFlag5 = 0x20;
	constexpr std::uint8_t kZero = 0x40;
	constexpr std::uint8_t kSign = 0x80;
	reg.pair.F = static_cast<std::uint8_t>(
	    (reg.pair.F & kCarry) | (value & (kSign | kFlag5 | kFlag3)) | (value == 0 ? kZero : 0) |
	    ((reg.IFF & kIff2) != 0 ? kParityOverflow : 0));
	ApplyFlagProfile();
}

void Z80CompatCpu::SynchronizePublicMirror() {
	if (impl_ != nullptr) {
		public_registers_ = ExportRegisters(impl_->cpu.reg);
	}
}
