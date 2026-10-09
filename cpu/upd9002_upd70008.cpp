/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
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
#include "compiler.h"
#include "machine/pccore.h"
#include "cpucore.h"
#include "io/iocore.h"
#include "io/upd9002_regs.h"
#include "io/memctrlva.h"
#include "cpu/upd9002_upd70008.h"
#include "cpu/z80_compat_cpu.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

// Interrupt acknowledge cycles read this virtual port, outside the 16-bit
// I/O space, so the 8214 vector never comes from a real port.
constexpr std::uint32_t kInterruptAcknowledgePort = 0x10000;

// VAEG_UPD70008_TRACE=N records N compatible-mode events (default 4096);
// VAEG_UPD70008_TRACE_SKIP=M first passes over M events, so a bounded window
// can be placed late in a long run.
static std::uint32_t compat_trace_slot() {
	static bool initialized = false;
	static std::uint32_t limit = 0;
	static std::uint32_t count = 0;
	static unsigned long long skip = 0;
	if (!initialized) {
		const char *skip_value = std::getenv("VAEG_UPD70008_TRACE_SKIP");
		if (skip_value != nullptr && skip_value[0] != '\0') {
			char *end = nullptr;
			unsigned long long parsed = std::strtoull(skip_value, &end, 10);
			if (end != skip_value && *end == '\0') {
				skip = parsed;
			}
		}
		const char *value = std::getenv("VAEG_UPD70008_TRACE");
		if (value != nullptr && value[0] != '\0') {
			char *end = nullptr;
			unsigned long parsed = std::strtoul(value, &end, 10);
			if (end != value && *end == '\0' && parsed != 0) {
				limit = parsed > 1000000UL ? 1000000U : static_cast<std::uint32_t>(parsed);
			} else {
				limit = 4096;
			}
		}
		initialized = true;
	}
	if (count >= limit) {
		return UINT32_MAX;
	}
	if (skip != 0) {
		skip--;
		return UINT32_MAX;
	}
	return count++;
}

static void compat_trace(const char *event, std::uint32_t slot, std::uint8_t op0,
                         std::uint8_t op1) {
	if (slot == UINT32_MAX) {
		return;
	}
	std::fprintf(
	    stderr,
	    "m76-compat-trace event=%s slot=%u cs=%04x ip=%04x "
	    "op=%02x/%02x af=%02x%02x bc=%04x de=%04x hl=%04x "
	    "ix=%04x iy=%04x sp=%04x nsp=%04x ss=%04x stk=%04x/%04x/%04x "
	    "flags=%04x rem=%d\n",
	    event, slot, CPU_CS, CPU_IP, op0, op1, CPU_AL, static_cast<unsigned>(CPU_FLAG & 0xff),
	    CPU_CX, CPU_DX, CPU_BX, CPU_SI, CPU_DI, CPU_BP, CPU_SP, CPU_SS,
	    static_cast<unsigned>(mem[(SS_BASE + CPU_SP) & CPU_ADRSMASK]) |
	        (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 1) & 0xffff)) & CPU_ADRSMASK]) << 8),
	    static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 2) & 0xffff)) & CPU_ADRSMASK]) |
	        (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 3) & 0xffff)) & CPU_ADRSMASK]) << 8),
	    static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 4) & 0xffff)) & CPU_ADRSMASK]) |
	        (static_cast<unsigned>(mem[(SS_BASE + ((CPU_SP + 5) & 0xffff)) & CPU_ADRSMASK]) << 8),
	    CPU_FLAG, CPU_REMCLOCK);
}

class CompatCounter final : public IClockCounter {
  public:
	void IFCALL past(std::int32_t clocks) override {
		// CPU speed (machine/pccore.c): 100 / percent machine clocks a cycle.
		remain_ -= static_cast<std::int32_t>(
		    clockscale_apply(&pccore_compat_scale, static_cast<UINT32>(clocks)));
	}

	std::int32_t IFCALL GetRemainclock() override {
		return remain_;
	}

	void IFCALL SetRemainclock(std::int32_t clocks) override {
		remain_ = clocks;
	}

  private:
	std::int32_t remain_ = 0;
};

class CompatClock final : public IClock {
  public:
	std::uint32_t IFCALL now() override {
		return CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	}
};

class Upd9002Upd70008Compat final : public IMemoryAccess, public IIOAccess {
  public:
	void Reset() {
		if (initialized_) {
			upd70008_.Reset();
		}
		upd70008_.SetMemoryBases(0, 0);
		initialized_ = false;
		have_compatible_state_ = false;
		counter_.SetRemainclock(0);
	}

	// Shared by Enter() and StateLoad() so that a state loaded before the
	// first mode entry also gets the native vector reader (M102 fix).
	bool InitializeCore() {
		upd70008_.SetFlagProfile(Z80CompatFlagProfile::kUpd9002);
		// Undefined ED 00-3F/74/75/77 read the native-side physical
		// page 0 (the x86 IVT); measured on a real PC-88VA2 (M102).
		upd70008_.SetNativeVectorRead(
		    [](void *, std::uint8_t index) -> std::uint8_t {
			    return static_cast<std::uint8_t>(upd9002_memoryread(index));
		    },
		    nullptr);
		initialized_ = upd70008_.Init(this, this, &clock_, &counter_, kInterruptAcknowledgePort);
		return initialized_;
	}

	void Enter() {
		if (!initialized_) {
			if (!InitializeCore()) {
				return;
			}
		}
		SetMemoryBases();
		UPD70008Reg state = *upd70008_.GetReg();
		state.af = static_cast<std::uint16_t>((static_cast<std::uint16_t>(CPU_AL) << 8) |
		                                      (CPU_FLAG & 0xff));
		state.bc = CPU_CX;
		state.de = CPU_DX;
		state.hl = CPU_BX;
		state.ix = CPU_SI;
		state.iy = CPU_DI;
		state.sp = CPU_BP;
		state.pc = CPU_IP;
		state.iff1 = (CPU_FLAG & I_FLAG) != 0;
		state.iff2 = state.iff1;
		if (have_compatible_state_) {
			upd70008_.SetMainReg(state);
		} else {
			upd70008_.SetReg(state);
			have_compatible_state_ = true;
		}
		LoadAltFromNative();
		counter_.SetRemainclock(CPU_REMCLOCK);
		const std::uint32_t trace_slot = compat_trace_slot();
		compat_trace("enter", trace_slot, 0, 0);
	}

	void Step() {
		if (!initialized_) {
			return;
		}
		counter_.SetRemainclock(CPU_REMCLOCK);
		const std::uint16_t pc = static_cast<std::uint16_t>(upd70008_.GetPC());
		const std::uint32_t code_address = (CPU_CS << 4) + pc;
		const std::uint8_t op0 = static_cast<std::uint8_t>(upd9002_memoryread(code_address));
		const std::uint8_t op1 = static_cast<std::uint8_t>(upd9002_memoryread(code_address + 1));
		const std::uint32_t trace_slot = compat_trace_slot();
		compat_trace("before", trace_slot, op0, op1);

		bool input = op0 == 0xdb;
		bool plain_io = input || op0 == 0xd3;
		UINT8 port = op1;
		// ED 40/41..78/79: IN r,(C) / OUT (C),r. ED A2/A3/AA/AB/B2/B3/BA/BB:
		// block I/O, which the VA2 ROM trap handler decodes and repeats itself.
		if (op0 == 0xed && (((op1 & 0xc6) == 0x40) || ((op1 & 0xe6) == 0xa2))) {
			plain_io = true;
			input = (op1 & 1) == 0;
			port = static_cast<UINT8>(upd70008_.GetReg()->bc);
		}
		if (plain_io && (upd9002_iotrap.control & (input ? 1 : 2))) {
			for (unsigned range = 0; range < 8; range += 4) {
				// Compatible I/O uses the 8-bit port number. Native DX matching
				// and FFEF bit 4 belong to the separate native interception path.
				if (port >= upd9002_iotrap.ranges[range] &&
				    port <= upd9002_iotrap.ranges[range + 2]) {
					SyncToNative();
					upd9002_core_compat_iotrap(input ? 0x7c : 0x7d, pc);
					counter_.SetRemainclock(CPU_REMCLOCK);
					compat_trace(input ? "io-trap-in" : "io-trap-out", trace_slot, op0, op1);
					return;
				}
			}
		}

		if ((op0 == 0xed) && (op1 == 0xed)) {
			const std::uint8_t vector =
			    static_cast<std::uint8_t>(upd9002_memoryread(code_address + 2));
			upd70008_.SetPC(static_cast<std::uint16_t>(pc + 3));
			SyncToNative();
			upd9002_core_compat_calln(vector, static_cast<REG16>(pc + 3));
			counter_.SetRemainclock(CPU_REMCLOCK);
			compat_trace("calln", trace_slot, op0, op1);
			return;
		}
		if ((op0 == 0xed) && (op1 == 0xfd)) {
			upd70008_.SetPC(static_cast<std::uint16_t>(pc + 2));
			SyncToNative();
			upd9002_core_compat_retem();
			counter_.SetRemainclock(CPU_REMCLOCK);
			compat_trace("retem", trace_slot, op0, op1);
			return;
		}

		const bool was_enabled = upd70008_.GetReg()->iff1 != 0;
		upd70008_.ExecOne();
		SyncToNative();
		CPU_REMCLOCK = counter_.GetRemainclock();
		// The machine arbitrates interrupts only between CPU time slices
		// (pic_irq). As the native STI does, end the slice one instruction
		// after EI (or a RETI/RETN restoring IFF1) when a request is waiting,
		// so it is taken in the enabled window instead of the next slice.
		if (!was_enabled && (upd70008_.GetReg()->iff1 != 0) && PICEXISTINTR && (CPU_REMCLOCK > 1)) {
			CPU_BASECLOCK += 1 - CPU_REMCLOCK;
			CPU_REMCLOCK = 1;
		}
		compat_trace("after", trace_slot, op0, op1);
	}

	void SyncToNative() {
		const UPD70008Reg *state = upd70008_.GetReg();
		StoreAltToNative();
		CPU_AL = static_cast<UINT8>(state->af >> 8);
		CPU_FLAG = static_cast<UINT16>((CPU_FLAG & 0xfc00) | (state->af & 0xff) |
		                               (state->iff1 ? I_FLAG : 0));
		CPU_CX = state->bc;
		CPU_DX = state->de;
		CPU_BX = state->hl;
		CPU_SI = state->ix;
		CPU_DI = state->iy;
		CPU_BP = state->sp;
		CPU_IP = state->pc;
	}

	void Resume() {
		SetMemoryBases();
		UPD70008Reg state = *upd70008_.GetReg();
		state.af = static_cast<std::uint16_t>((static_cast<std::uint16_t>(CPU_AL) << 8) |
		                                      (CPU_FLAG & 0xff));
		state.bc = CPU_CX;
		state.de = CPU_DX;
		state.hl = CPU_BX;
		state.ix = CPU_SI;
		state.iy = CPU_DI;
		state.sp = CPU_BP;
		state.pc = CPU_IP;
		state.iff1 = (CPU_FLAG & I_FLAG) != 0;
		state.iff2 = state.iff1;
		upd70008_.SetMainReg(state);
		LoadAltFromNative();
		counter_.SetRemainclock(CPU_REMCLOCK);
	}

	// M103a: the alternate set lives in machine-owned storage
	// (upd9002_alt_regs); the compat core holds a working copy.
	void LoadAltFromNative() {
		UPD70008Reg state = *upd70008_.GetReg();
		state.r_af = upd9002_alt_regs.af;
		state.r_bc = upd9002_alt_regs.bc;
		state.r_de = upd9002_alt_regs.de;
		state.r_hl = upd9002_alt_regs.hl;
		upd70008_.SetAltReg(state);
	}

	void StoreAltToNative() {
		const UPD70008Reg *state = upd70008_.GetReg();
		upd9002_alt_regs.af = state->r_af;
		upd9002_alt_regs.bc = state->r_bc;
		upd9002_alt_regs.de = state->r_de;
		upd9002_alt_regs.hl = state->r_hl;
	}

	void Irq(bool asserted) {
		if (initialized_) {
			upd70008_.IRQ(0, asserted ? 1 : 0);
		}
	}

	int StateSave(UINT8 *buffer, UINT size) {
		if ((buffer == NULL) || (size != UPD9002_COMPAT_STATE_SIZE) || !initialized_) {
			return FAILURE;
		}
		counter_.SetRemainclock(CPU_REMCLOCK);
		return upd70008_.SaveStatus(buffer) ? SUCCESS : FAILURE;
	}

	int StateLoad(const UINT8 *buffer, UINT size) {
		if ((buffer == NULL) || (size != UPD9002_COMPAT_STATE_SIZE)) {
			return FAILURE;
		}
		if (!initialized_) {
			if (!InitializeCore()) {
				return FAILURE;
			}
		}
		if (!upd70008_.LoadStatus(buffer)) {
			return FAILURE;
		}
		SetMemoryBases();
		have_compatible_state_ = true;
		// The UPD9ALT section precedes this blob in a state file and marks
		// upd9002_alt_regs_loaded; then the machine-owned storage is the
		// authority. A file written before M103a has no such section, so
		// the blob's working copy becomes the storage.
		if (upd9002_alt_regs_loaded) {
			LoadAltFromNative();
		} else {
			StoreAltToNative();
		}
		CPU_REMCLOCK = counter_.GetRemainclock();
		return SUCCESS;
	}

	std::uint32_t IFCALL Read8(std::uint32_t address) override {
		return upd9002_memoryread(address & CPU_ADRSMASK);
	}

	void IFCALL Write8(std::uint32_t address, std::uint32_t data) override {
		upd9002_memorywrite(address & CPU_ADRSMASK, static_cast<UINT8>(data));
	}

	std::uint32_t IFCALL In(std::uint32_t port) override {
		if (port == kInterruptAcknowledgePort) {
			upd70008_.IRQ(0, 0);
			return pic8214_acknowledge_compat();
		}
		const SINT32 before = CPU_REMCLOCK;
		const UINT8 value = iocore_inp8(static_cast<UINT>(port & 0xffff));
		const SINT32 elapsed = before - CPU_REMCLOCK;
		if (elapsed > 0) {
			counter_.past(elapsed);
		}
		return value;
	}

	void IFCALL Out(std::uint32_t port, std::uint32_t data) override {
		const SINT32 before = CPU_REMCLOCK;
		iocore_out8(static_cast<UINT>(port & 0xffff), static_cast<UINT8>(data));
		const SINT32 elapsed = before - CPU_REMCLOCK;
		if (elapsed > 0) {
			counter_.past(elapsed);
		}
	}

  private:
	void SetMemoryBases() {
		upd70008_.SetMemoryBases(static_cast<std::uint32_t>(CPU_CS) << 4,
		                         static_cast<std::uint32_t>(CPU_DS) << 4);
	}

	UPD70008C upd70008_;
	CompatCounter counter_;
	CompatClock clock_;
	bool initialized_ = false;
	bool have_compatible_state_ = false;
};

Upd9002Upd70008Compat compat;

void compat_reset() {
	compat.Reset();
}
void compat_enter() {
	compat.Enter();
	memctrlva_nmode_compat_entry();
}
void compat_step() {
	compat.Step();
}
void compat_sync_to_native() {
	compat.SyncToNative();
}
void compat_leave() {
}
void compat_resume() {
	const std::uint32_t trace_slot = compat_trace_slot();
	compat_trace("resume-before", trace_slot, 0, 0);
	compat.Resume();
	compat_trace("resume-after", trace_slot, 0, 0);
}
int compat_state_save(UINT8 *buffer, UINT size) {
	return compat.StateSave(buffer, size);
}
int compat_state_load(const UINT8 *buffer, UINT size) {
	return compat.StateLoad(buffer, size);
}
void compat_irq(BOOL asserted) {
	compat.Irq(asserted != FALSE);
}

} // namespace

extern "C" void upd9002_upd70008_register(void) {
	static bool registered = false;
	if (registered) {
		return;
	}
	Upd9002CompatHooks hooks{};
	hooks.reset = compat_reset;
	hooks.enter = compat_enter;
	hooks.step = compat_step;
	hooks.sync_to_native = compat_sync_to_native;
	hooks.leave = compat_leave;
	hooks.resume = compat_resume;
	hooks.state_save = compat_state_save;
	hooks.state_load = compat_state_load;
	hooks.irq = compat_irq;
	upd9002_core_set_compat_hooks(&hooks);
	registered = true;
}

#if defined(VAEG_UPD9002_M76_TESTING)
static unsigned test_io_accesses;
static REG8 IOINPCALL test_trapped_in(UINT) {
	test_io_accesses++;
	return 0x99;
}
static void IOOUTCALL test_trapped_out(UINT, REG8) {
	test_io_accesses++;
}

static int compat_entry_selftest(UINT8 entry_opcode) {
	const UINT16 code_segment = 0x2000;
	const UINT16 native_stack_segment = 0x3000;
	const UINT16 code_offset = 0x0100;
	const UINT16 compatible_offset = 0x1000;
	const UINT16 native_offset = 0x3000;
	const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
	const UINT32 native_stack_base = static_cast<UINT32>(native_stack_segment) << 4;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_upd70008_register();
	upd9002_core_reset();

	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = native_stack_segment;
	CPU_IP = code_offset;
	CPU_SP = 0x0100;
	CPU_BP = 0x0200;
	CPU_FLAG = 0xf202;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = native_stack_base;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_CLOCK = 0;

	mem[(0x00e1U * 4) + 0] = 0x00;
	mem[(0x00e1U * 4) + 1] = 0x10;
	mem[(0x00e1U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e1U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	mem[(0x00e0U * 4) + 0] = static_cast<UINT8>(native_offset);
	mem[(0x00e0U * 4) + 1] = static_cast<UINT8>(native_offset >> 8);
	mem[(0x00e0U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e0U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	const UINT16 native_interrupt_offset = 0x3100;
	mem[(0x00e2U * 4) + 0] = static_cast<UINT8>(native_interrupt_offset);
	mem[(0x00e2U * 4) + 1] = static_cast<UINT8>(native_interrupt_offset >> 8);
	mem[(0x00e2U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e2U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);

	mem[code_base + code_offset + 0] = 0x0f;
	mem[code_base + code_offset + 1] = entry_opcode;
	mem[code_base + code_offset + 2] = 0xe1;
	mem[code_base + compatible_offset + 0] = 0x3e;
	mem[code_base + compatible_offset + 1] = 0x42;
	mem[code_base + compatible_offset + 2] = 0x18;
	mem[code_base + compatible_offset + 3] = 0x03;
	mem[code_base + compatible_offset + 4] = 0x3e;
	mem[code_base + compatible_offset + 5] = 0x99;
	mem[code_base + compatible_offset + 6] = 0x00;
	mem[code_base + compatible_offset + 7] = 0xdd;
	mem[code_base + compatible_offset + 8] = 0x21;
	mem[code_base + compatible_offset + 9] = 0x78;
	mem[code_base + compatible_offset + 10] = 0x56;
	mem[code_base + compatible_offset + 11] = 0xfd;
	mem[code_base + compatible_offset + 12] = 0x21;
	mem[code_base + compatible_offset + 13] = 0xbc;
	mem[code_base + compatible_offset + 14] = 0x9a;
	mem[code_base + compatible_offset + 15] = 0xed;
	mem[code_base + compatible_offset + 16] = 0xed;
	mem[code_base + compatible_offset + 17] = 0xe0;
	mem[code_base + compatible_offset + 18] = 0x21;
	mem[code_base + compatible_offset + 19] = 0x34;
	mem[code_base + compatible_offset + 20] = 0x12;
	mem[code_base + compatible_offset + 21] = 0xed;
	mem[code_base + compatible_offset + 22] = 0xfd;
	mem[code_base + native_offset] = 0xcf;
	mem[code_base + native_interrupt_offset] = 0xcf;

	upd9002_core_step();
	if ((CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) || (CPU_CS != code_segment) ||
	    (CPU_IP != compatible_offset) || (CPU_SP != 0x00fa) || (CPU_DS != code_segment) ||
	    (CPU_SS != native_stack_segment) ||
	    (upd9002_memoryread_w(native_stack_base + 0x00fa) != code_offset + 3) ||
	    (upd9002_memoryread_w(native_stack_base + 0x00fc) != code_segment) ||
	    (upd9002_memoryread_w(native_stack_base + 0x00fe) != 0xf202)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if (CPU_AL != 0x42 || CPU_IP != compatible_offset + 2) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	// Trap immediate and ordinary ED I/O before any device side effect.
	iocore_create();
	if (iocore_build() != SUCCESS) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	iocore_attachinp(0x50, test_trapped_in);
	iocore_attachout(0x50, test_trapped_out);
	iocore_attachinp(0x00, test_trapped_in);
	iocore_attachout(0x00, test_trapped_out);
	upd9002_iotrap = {};
	upd9002_iotrap.ranges[0] = 0x50;
	upd9002_iotrap.ranges[2] = 0x50;
	upd9002_iotrap.control = 3;
	test_io_accesses = 0;
	static const UINT8 block_io[8] = {0xa2, 0xa3, 0xaa, 0xab, 0xb2, 0xb3, 0xba, 0xbb};
	for (unsigned form = 0; form < 26; form++) {
		const bool output = (form & 1) != 0;
		const unsigned vector = output ? 0x7d : 0x7c;
		upd9002_iotrap.ranges[0] = form < 2 ? 0x50 : 0x00;
		upd9002_iotrap.ranges[2] = upd9002_iotrap.ranges[0];
		upd9002_memorywrite_w(vector * 4, native_offset);
		upd9002_memorywrite_w(vector * 4 + 2, code_segment);
		mem[code_base + compatible_offset + 2] = form < 2 ? (output ? 0xd3 : 0xdb) : 0xed;
		mem[code_base + compatible_offset + 3] =
		    form < 2    ? 0x50
		    : form < 18 ? static_cast<UINT8>(0x40 + ((form - 2) / 2) * 8 + (form & 1))
		                : block_io[form - 18];
		upd9002_core_step();
		bool ok = CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE && CPU_IP == native_offset &&
		          CPU_SP == 0x00f4 && !(CPU_FLAG & I_FLAG) && test_io_accesses == 0 &&
		          upd9002_memoryread_w(native_stack_base + 0xf4) == compatible_offset + 2;
		// The synthetic IRET handler deliberately retries the same IP so the
		// next I/O form and then the original JR can reuse this location.
		if (ok) {
			upd9002_core_step();
		}
		ok = ok && CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008 && CPU_IP == compatible_offset + 2 &&
		     CPU_AL == 0x42 && CPU_SP == 0xfa;
		if (!ok) {
			upd9002_iotrap = {};
			iocore_destroy();
			upd9002_core_deinitialize();
			return FAILURE;
		}
	}
	upd9002_iotrap = {};
	iocore_destroy();
	mem[code_base + compatible_offset + 2] = 0x18;
	mem[code_base + compatible_offset + 3] = 0x03;
	upd9002_core_step();
	if (CPU_IP != compatible_offset + 7) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if (CPU_SI != 0x5678 || CPU_IP != compatible_offset + 11) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if (CPU_DI != 0x9abc || CPU_IP != compatible_offset + 15) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	UINT8 saved_compat[UPD9002_COMPAT_STATE_SIZE];
	if (upd9002_core_compat_state_save(saved_compat, sizeof(saved_compat)) != SUCCESS) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if ((CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE) || (CPU_IP != native_offset) ||
	    (CPU_SP != 0x00f4)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_interrupt(0xe2);
	upd9002_core_step();
	if ((CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE) || (CPU_IP != native_offset) ||
	    (CPU_SP != 0x00f4) || (CPU_COMPAT_RETURN_PENDING == 0)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if ((CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) || (CPU_IP != compatible_offset + 18) ||
	    (CPU_SP != 0x00fa)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	if (CPU_BX != 0x1234 || CPU_IP != compatible_offset + 21) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_step();
	int passed = (CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE) && (CPU_CS == code_segment) &&
	             (CPU_IP == code_offset + 3) && (CPU_SP == 0x0100) && (CPU_AL == 0x42) &&
	             (CPU_BX == 0x1234) && (CPU_SI == 0x5678) && (CPU_DI == 0x9abc);
	if (passed) {
		upd9002_core_reset();
		CPU_CS = code_segment;
		CPU_DS = code_segment;
		CPU_SS = native_stack_segment;
		CPU_SP = 0x00fa;
		CPU_BP = 0x0200;
		CPU_FLAG = 0xf202;
		CS_BASE = code_base;
		DS_BASE = code_base;
		SS_BASE = native_stack_base;
		CPU_REMCLOCK = 100000;
		CPU_BASECLOCK = 100000;
		CPU_COMPAT_RETURN_PENDING = 1;
		if (upd9002_core_compat_state_load(saved_compat, sizeof(saved_compat)) != SUCCESS) {
			passed = 0;
		} else {
			CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
			upd9002_core_step();
			passed = (CPU_COMPAT_MODE == UPD9002_COMPAT_NATIVE) && (CPU_IP == native_offset) &&
			         (CPU_SP == 0x00f4);
			upd9002_core_step();
			passed = passed && (CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008) &&
			         (CPU_IP == compatible_offset + 18);
		}
	}
	upd9002_core_deinitialize();
	return passed ? SUCCESS : FAILURE;
}

extern "C" int upd9002_upd70008_compat_selftest(void) {
	const UINT8 encodings[] = {0xff, 0xfe};
	for (UINT8 encoding : encodings) {
		if (compat_entry_selftest(encoding) != SUCCESS) {
			std::fprintf(stderr, "compatible entry selftest failed: 0F %02X\n", encoding);
			return FAILURE;
		}
	}
	return SUCCESS;
}

// M105: the CPU speed scales the compatible core's cycles (ten NOPs).
static SINT32 compat_nop_clocks(UINT percent) {
	const UINT16 code_segment = 0x2000;
	const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
	SINT32 before;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_upd70008_register();
	upd9002_core_reset();
	pccore_set_cpu_speed(percent);
	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = 0x3000;
	CPU_IP = 0x0100;
	CPU_SP = 0x0100;
	CPU_FLAG = 0xf202;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = 0x30000;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_CLOCK = 0;
	mem[(0x00e1U * 4) + 0] = 0x00;
	mem[(0x00e1U * 4) + 1] = 0x10;
	mem[(0x00e1U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e1U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	mem[code_base + 0x0100] = 0x0f; // BRKEM E1h
	mem[code_base + 0x0101] = 0xff;
	mem[code_base + 0x0102] = 0xe1;
	upd9002_core_step();
	before = CPU_REMCLOCK;
	for (int i = 0; i < 10; i++) {
		upd9002_core_step(); // NOP (memory is zero)
	}
	before -= CPU_REMCLOCK;
	pccore_set_cpu_speed(100);
	upd9002_core_deinitialize();
	return before;
}

extern "C" int upd9002_upd70008_cpu_speed_selftest(void) {
	const SINT32 full = compat_nop_clocks(100);
	const SINT32 half = compat_nop_clocks(50);

	if ((full <= 0) || (half != full * 2)) {
		std::fprintf(stderr, "compatible CPU speed selftest failed: 100%% %d, 50%% %d clocks\n",
		             static_cast<int>(full), static_cast<int>(half));
		return FAILURE;
	}
	return SUCCESS;
}

// M103a: alternate register storage and the load-before-enter vector reader.
extern "C" int upd9002_upd70008_alt_regs_selftest(void) {
	const UINT16 code_segment = 0x2000;
	const UINT16 native_stack_segment = 0x3000;
	const UINT16 code_offset = 0x0100;
	const UINT16 compatible_offset = 0x1000;
	const UINT16 native_offset = 0x3000;
	const UINT32 code_base = static_cast<UINT32>(code_segment) << 4;
	const UINT32 native_stack_base = static_cast<UINT32>(native_stack_segment) << 4;

	upd9002_core_initialize();
	ZeroMemory(mem, 0x100000);
	upd9002_upd70008_register();
	upd9002_core_reset();

	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = native_stack_segment;
	CPU_IP = code_offset;
	CPU_SP = 0x0100;
	CPU_BP = 0x0200;
	CPU_FLAG = 0xf202;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = native_stack_base;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_CLOCK = 0;

	// IVT: vector E1 = BRKEM target, E0 = CALLN target; bytes 30h/31h are
	// the word the undefined ED 20 reads with C = 30h.
	mem[(0x00e1U * 4) + 0] = static_cast<UINT8>(compatible_offset);
	mem[(0x00e1U * 4) + 1] = static_cast<UINT8>(compatible_offset >> 8);
	mem[(0x00e1U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e1U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	mem[(0x00e0U * 4) + 0] = static_cast<UINT8>(native_offset);
	mem[(0x00e0U * 4) + 1] = static_cast<UINT8>(native_offset >> 8);
	mem[(0x00e0U * 4) + 2] = static_cast<UINT8>(code_segment);
	mem[(0x00e0U * 4) + 3] = static_cast<UINT8>(code_segment >> 8);
	mem[0x30] = 0x00;
	mem[0x31] = 0x28;

	mem[code_base + code_offset + 0] = 0x0f; // BRKEM E1h
	mem[code_base + code_offset + 1] = 0xff;
	mem[code_base + code_offset + 2] = 0xe1;
	static const UINT8 program[] = {
	    0xaf,             // XOR A            F = 44h: N (bit 1) clear
	    0x01, 0x11, 0x11, // LD BC,1111h
	    0xd9,             // EXX
	    0x01, 0x22, 0x22, // LD BC,2222h
	    0xd9,             // EXX              BC=1111h, BC'=2222h
	    0xed, 0xed, 0xe0, // CALLN E0h
	    0xd9,             // EXX              BC = storage value
	    0x0e, 0x30,       // LD C,30h
	    0xed, 0x20,       // undefined: HL = IVT word at 30h
	    0xed, 0xfd,       // RETEM
	};
	for (size_t i = 0; i < sizeof(program); ++i) {
		mem[code_base + compatible_offset + i] = program[i];
	}
	mem[code_base + native_offset] = 0xcf; // IRET

	auto fail = [](int line) {
		fprintf(
		    stderr,
		    "alt-regs selftest failed at line %d: mode=%d CX=%04X BX=%04X alt.bc=%04X IP=%04X\n",
		    line, (int)CPU_COMPAT_MODE, CPU_CX, CPU_BX, upd9002_alt_regs.bc, CPU_IP);
		upd9002_core_deinitialize();
		return FAILURE;
	};

	upd9002_core_step(); // BRKEM
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008)
		return fail(__LINE__);
	upd9002_core_step(); // XOR A
	if ((CPU_FLAG & 0xff) != 0x44)
		return fail(__LINE__);
	upd9002_core_step(); // LD BC,1111h
	if (CPU_CX != 0x1111)
		return fail(__LINE__);
	upd9002_core_step(); // EXX: BC takes the reset value of BC'
	if (CPU_CX != 0xffff || upd9002_alt_regs.bc != 0x1111)
		return fail(__LINE__);
	upd9002_core_step(); // LD BC,2222h
	upd9002_core_step(); // EXX
	if (CPU_CX != 0x1111 || upd9002_alt_regs.bc != 0x2222)
		return fail(__LINE__);

	UINT8 blob[UPD9002_COMPAT_STATE_SIZE];
	if (upd9002_core_compat_state_save(blob, sizeof(blob)) != SUCCESS)
		return fail(__LINE__);

	upd9002_core_step(); // CALLN -> native
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE)
		return fail(__LINE__);
	// UPD9MODE: the return-pending SP is what lets IRET resume compatible
	// mode. Losing it (as UPD9CPU import alone did) breaks the return.
	UINT8 mode_state[UPD9002_MODE_STATE_SIZE];
	upd9002_core_mode_state_save(mode_state);
	static const UINT8 lost_sp[UPD9002_MODE_STATE_SIZE] = {0, 1, 0, 0};
	static const UINT8 invalid[][UPD9002_MODE_STATE_SIZE] = {
	    {2, 0, 0, 0}, {0, 2, 0, 0}, {1, 1, 0, 0}};
	for (const auto &bad : invalid) {
		if (upd9002_core_mode_state_valid(bad) || upd9002_core_mode_state_load(bad) == SUCCESS)
			return fail(__LINE__);
	}
	if (mode_state[0] != UPD9002_COMPAT_NATIVE || mode_state[1] != 1 ||
	    upd9002_core_mode_state_load(lost_sp) != SUCCESS || upd9002_core_compat_iret_is_return())
		return fail(__LINE__);
	if (upd9002_core_mode_state_load(mode_state) != SUCCESS ||
	    !upd9002_core_compat_iret_is_return())
		return fail(__LINE__);
	// Native code cannot address the alternate set, but the machine-owned
	// storage is the authority: a change there is what the compat core sees
	// when it resumes.
	upd9002_alt_regs.bc = 0x3333;
	upd9002_core_step(); // IRET -> resumes compatible mode
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008 || (CPU_FLAG & 0xff) != 0x44 || CPU_AL != 0)
		return fail(__LINE__);
	upd9002_core_step(); // EXX
	if (CPU_CX != 0x3333 || upd9002_alt_regs.bc != 0x1111)
		return fail(__LINE__);
	upd9002_core_step(); // LD C,30h
	upd9002_core_step(); // ED 20
	if (CPU_BX != 0x2800)
		return fail(__LINE__);
	upd9002_core_step(); // RETEM
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE)
		return fail(__LINE__);

	// Hardware reset sets the storage to all ones (M103c).
	upd9002_core_reset();
	if (upd9002_alt_regs.af != 0xffff || upd9002_alt_regs.bc != 0xffff ||
	    upd9002_alt_regs.de != 0xffff || upd9002_alt_regs.hl != 0xffff || upd9002_alt_regs_loaded)
		return fail(__LINE__);

	// Old state file: compat blob without a UPD9ALT section. The blob's
	// alternate set (BC' = 2222h) becomes the storage.
	CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
	if (upd9002_core_compat_state_load(blob, sizeof(blob)) != SUCCESS)
		return fail(__LINE__);
	if (upd9002_alt_regs.bc != 0x2222)
		return fail(__LINE__);

	// New state file: UPD9ALT loaded first, then the blob. The storage wins
	// and the load-before-enter path still reads the IVT (reader installed).
	upd9002_core_reset();
	upd9002_upd70008_register();
	CPU_CS = code_segment;
	CPU_DS = code_segment;
	CPU_SS = native_stack_segment;
	CS_BASE = code_base;
	DS_BASE = code_base;
	SS_BASE = native_stack_base;
	CPU_REMCLOCK = 100000;
	CPU_BASECLOCK = 100000;
	CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
	upd9002_alt_regs.bc = 0x4444;
	upd9002_alt_regs_loaded = TRUE;
	if (upd9002_core_compat_state_load(blob, sizeof(blob)) != SUCCESS)
		return fail(__LINE__);
	// The blob's PC points at the CALLN; replace that code with EXX / LD C /
	// ED 20 / RETEM for this check.
	static const UINT8 tail[] = {0xd9, 0x0e, 0x30, 0xed, 0x20, 0xed, 0xfd};
	for (size_t i = 0; i < sizeof(tail); ++i) {
		mem[code_base + compatible_offset + 9 + i] = tail[i];
	}
	upd9002_core_step(); // EXX
	if (CPU_CX != 0x4444)
		return fail(__LINE__);
	upd9002_core_step(); // LD C,30h
	upd9002_core_step(); // ED 20 -- zero without the StateLoad reader fix
	if (CPU_BX != 0x2800)
		return fail(__LINE__);

	upd9002_core_deinitialize();
	return SUCCESS;
}
#endif
