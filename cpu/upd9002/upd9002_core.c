#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "dmap.h"
#include "upd9002_diagnostic.h"
#include "upd9002_state.h"
#include "upd9002_trace.h"
#include "upd9002_perf.h"
#if defined(VAEG_UPD9002_SSTS_TESTING)
#include "tests/upd9002/direct_harness.h"
#endif
#include "upd9002_ops.mcr"

Upd9002CoreContext upd9002_core_context;
Upd9002AltRegs upd9002_alt_regs;
BOOL upd9002_alt_regs_loaded;
UINT16 upd9002_step_start_cs;
UINT16 upd9002_step_start_ip;
UINT8 upd9002_current_opcode;
static Upd9002CompatHooks upd9002_compat_hooks;
/*
 * A native CALLN can be interrupted before its final IRET.  Keep the
 * native-stack position of the CALLN frame so an interrupt IRET cannot be
 * mistaken for the compatibility return.
 */
static UINT16 upd9002_compat_return_sp;

const UINT8 iflags[512] = { // Z_FLAG, S_FLAG, P_FLAG
    0x44, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
    0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84, 0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80, 0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80, 0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84, 0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80, 0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84, 0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84, 0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
    0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80, 0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
    0x45, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01, 0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
    0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05, 0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
    0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05, 0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
    0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01, 0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
    0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05, 0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
    0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01, 0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
    0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01, 0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05,
    0x01, 0x05, 0x05, 0x01, 0x05, 0x01, 0x01, 0x05, 0x05, 0x01, 0x01, 0x05, 0x01, 0x05, 0x05, 0x01,
    0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85, 0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
    0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81, 0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
    0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81, 0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
    0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85, 0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
    0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81, 0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85,
    0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85, 0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
    0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85, 0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81,
    0x85, 0x81, 0x81, 0x85, 0x81, 0x85, 0x85, 0x81, 0x81, 0x85, 0x85, 0x81, 0x85, 0x81, 0x81, 0x85};

// ----

#if !defined(MEMOPTIMIZE)
UINT8 _szpflag16[0x10000];
#endif

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
UINT8 *_reg8_b53[256];
UINT8 *_reg8_b20[256];
#endif
#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
UINT16 *_reg16_b53[256];
UINT16 *_reg16_b20[256];
#endif

void upd9002_core_initialize(void) {
#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
	UINT i;
#endif

#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
	for (i = 0; i < 0x100; i++) {
		int pos;
#if defined(BYTESEX_LITTLE)
		pos = ((i & 0x20) ? 1 : 0);
#else
		pos = ((i & 0x20) ? 0 : 1);
#endif
		pos += ((i >> 3) & 3) * 2;
		_reg8_b53[i] = ((UINT8 *)&UPD9002_REG) + pos;
#if defined(BYTESEX_LITTLE)
		pos = ((i & 0x4) ? 1 : 0);
#else
		pos = ((i & 0x4) ? 0 : 1);
#endif
		pos += (i & 3) * 2;
		_reg8_b20[i] = ((UINT8 *)&UPD9002_REG) + pos;
#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
		_reg16_b53[i] = ((UINT16 *)&UPD9002_REG) + ((i >> 3) & 7);
		_reg16_b20[i] = ((UINT16 *)&UPD9002_REG) + (i & 7);
#endif
	}
#endif

#if !defined(MEMOPTIMIZE)
	for (i = 0; i < 0x10000; i++) {
		REG8 f;
		UINT bit;
		f = P_FLAG;
		for (bit = 0x80; bit; bit >>= 1) {
			if (i & bit) {
				f ^= P_FLAG;
			}
		}
		if (!i) {
			f |= Z_FLAG;
		}
		if (i & 0x8000) {
			f |= S_FLAG;
		}
		_szpflag16[i] = f;
	}
#endif
#if !defined(MEMOPTIMIZE) || (MEMOPTIMIZE < 2)
	upd9002_ea_initialize();
#endif
	ZeroMemory(&upd9002_core_context, sizeof(upd9002_core_context));
	upd9002_compat_return_sp = 0;
	upd9002_diagnostic_clear();
	upd9002_state_initialize();
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
}

void upd9002_core_deinitialize(void) {
	if (CPU_EXTMEM) {
		_MFREE(CPU_EXTMEM);
		CPU_EXTMEM = NULL;
		CPU_EXTMEMSIZE = 0;
	}
	upd9002_compat_return_sp = 0;
}

static void upd9002_initreg(void) {
	UPD9002_CS = 0xf000;
	CS_BASE = 0xf0000;
	UPD9002_IP = 0xfff0;
	UPD9002_ADRSMASK = 0xfffff;
}

static void upd9002_core_initreg(void) {
	upd9002_initreg();
	UPD9002_FLAG = 0xf002;
}

void upd9002_core_step(void) {
	if (CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008) {
		if (upd9002_compat_hooks.step != NULL) {
			upd9002_compat_hooks.step();
		}
		return;
	}

	UINT opcode;
	BOOL preserve_state;
	Upd9002RuntimeState state_before;

	if (upd9002_diagnostic_pending()) {
		return;
	}

	upd9002_guest_trace_step_begin();
	upd9002_trace_step_begin();
	upd9002_step_start_cs = UPD9002_CS;
	upd9002_step_start_ip = UPD9002_IP;
	opcode = upd9002_memoryread(CS_BASE + UPD9002_IP);
	upd9002_current_opcode = (UINT8)opcode;
	upd9002_perf_record_step(CS_BASE, UPD9002_IP, (UINT8)opcode,
	                         mem[(CS_BASE + (UINT16)(UPD9002_IP + 1)) & UPD9002_ADRSMASK]);
	preserve_state = (opcode == 0x26) || (opcode == 0x2e) || (opcode == 0x36) || (opcode == 0x3e) ||
	                 (opcode == 0xf2) || (opcode == 0xf3);
	if (preserve_state) {
		state_before = upd9002_core_context.s;
	}
	UPD9002_OV = UPD9002_FLAG & O_FLAG;
	UPD9002_FLAG &= ~(O_FLAG);

	UPD9002_IP++;
	upd9002op[opcode]();

	UPD9002_FLAG &= ~(O_FLAG);
	if (UPD9002_OV) {
		UPD9002_FLAG |= (O_FLAG);
	}
	if (upd9002_diagnostic_pending()) {
		/*
		 * Every active path to the diagnostic starts with one of the
		 * prefixes above. Restore the complete runtime image so the
		 * unresolved encoding has no architectural effect.
		 */
		if (preserve_state) {
			upd9002_core_context.s = state_before;
		}
		upd9002_guest_trace_step_end();
		upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "diagnostic-stop-rep0f", CS_BASE + UPD9002_IP,
		                    opcode, 1);
		upd9002_trace_step_end();
		return;
	}
	upd9002_dmap();
	upd9002_guest_trace_step_end();
	upd9002_trace_step_end();
}

void upd9002_core_reset(void) {
#if defined(VAEG_UPD9002_M46_TESTING)
	upd9002_dispatch_test_require_immutable();
#endif
	ZeroMemory(&upd9002_core_context.s, sizeof(upd9002_core_context.s));
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
	upd9002_diagnostic_clear();
	upd9002_core_initreg();
	upd9002_state_reset();
	/* All ones: a real PC-88VA2 shows AF' = BC' = FFFFh in V1/V2 MON after
	 * firmware that never writes them (CPU document 17.2; M103c). */
	upd9002_alt_regs.af = 0xffff;
	upd9002_alt_regs.bc = 0xffff;
	upd9002_alt_regs.de = 0xffff;
	upd9002_alt_regs.hl = 0xffff;
	upd9002_alt_regs_loaded = FALSE;
	CPU_COMPAT_MODE = UPD9002_COMPAT_NATIVE;
	CPU_COMPAT_RETURN_PENDING = 0;
	upd9002_compat_return_sp = 0;
	if (upd9002_compat_hooks.reset != NULL) {
		upd9002_compat_hooks.reset();
	}
}

void upd9002_core_shut(void) {
	/*
	 * ADR-0012 preserves this CPU_SHUT-only 286-style register result.
	 * It is a regression fixture exception, not an 80286 execution mode.
	 */
	ZeroMemory(&upd9002_core_context.s, offsetof(Upd9002RuntimeState, cpu_type));
	upd9002_diagnostic_clear();
	upd9002_initreg();
	upd9002_state_shut();
}

void upd9002_core_set_ext_size(UINT32 size) {
	if (CPU_EXTMEMSIZE != size) {
		if (CPU_EXTMEM) {
			_MFREE(CPU_EXTMEM);
			CPU_EXTMEM = NULL;
		}
		if (size) {
			CPU_EXTMEM = (BYTE *)_MALLOC(size + 16, "EXTMEM");
			if (CPU_EXTMEM == NULL) {
				size = 0;
			}
		}
		CPU_EXTMEMSIZE = size;
	}
	upd9002_core_context.e.ems[0] = mem + 0xc0000;
	upd9002_core_context.e.ems[1] = mem + 0xc4000;
	upd9002_core_context.e.ems[2] = mem + 0xc8000;
	upd9002_core_context.e.ems[3] = mem + 0xcc000;
}

void upd9002_core_set_emm(UINT frame, UINT32 addr) {
	BYTE *ptr;

	frame &= 3;
	if (addr < USE_HIMEM) {
		ptr = mem + addr;
	} else if ((addr - 0x100000 + 0x4000) <= CPU_EXTMEMSIZE) {
		ptr = CPU_EXTMEM + (addr - 0x100000);
	} else {
		ptr = mem + 0xc0000 + (frame << 14);
	}
	upd9002_core_context.e.ems[frame] = ptr;
}

static UINT16 upd9002_materialize_interrupt_saved_flags(void) {
	return (UINT16)((UPD9002_FLAG & (UINT16)~O_FLAG) | (UPD9002_OV ? O_FLAG : 0));
}

void upd9002_core_set_compat_hooks(const Upd9002CompatHooks *hooks) {
	if (hooks == NULL) {
		ZeroMemory(&upd9002_compat_hooks, sizeof(upd9002_compat_hooks));
	} else {
		upd9002_compat_hooks = *hooks;
	}
}

int upd9002_core_compat_state_save(UINT8 *buffer, UINT size) {
	if ((buffer == NULL) || (size != UPD9002_COMPAT_STATE_SIZE)) {
		return FAILURE;
	}
	ZeroMemory(buffer, size);
	if (((CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008) || (CPU_COMPAT_RETURN_PENDING != 0)) &&
	    (upd9002_compat_hooks.state_save != NULL)) {
		return upd9002_compat_hooks.state_save(buffer, size);
	}
	return SUCCESS;
}

void upd9002_core_mode_state_save(UINT8 *buffer) {
	buffer[0] = CPU_COMPAT_MODE;
	buffer[1] = CPU_COMPAT_RETURN_PENDING;
	STOREINTELWORD(buffer + 2, upd9002_compat_return_sp);
}

BOOL upd9002_core_mode_state_valid(const UINT8 *buffer) {
	return ((buffer[0] == UPD9002_COMPAT_NATIVE) || (buffer[0] == UPD9002_COMPAT_UPD70008)) &&
	       (buffer[1] <= 1) && !((buffer[0] == UPD9002_COMPAT_UPD70008) && buffer[1]);
}

/* Must follow UPD9CPU, whose import clears these fields, and precede UPD9Z80,
 * whose loader runs only when compatible state is active. */
int upd9002_core_mode_state_load(const UINT8 *buffer) {
	if (!upd9002_core_mode_state_valid(buffer)) {
		return FAILURE;
	}
	CPU_COMPAT_MODE = buffer[0];
	CPU_COMPAT_RETURN_PENDING = buffer[1];
	upd9002_compat_return_sp = LOADINTELWORD(buffer + 2);
	return SUCCESS;
}

int upd9002_core_compat_state_load(const UINT8 *buffer, UINT size) {
	if ((buffer == NULL) || (size != UPD9002_COMPAT_STATE_SIZE)) {
		return FAILURE;
	}
	if (((CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008) || (CPU_COMPAT_RETURN_PENDING != 0)) &&
	    (upd9002_compat_hooks.state_load != NULL)) {
		return upd9002_compat_hooks.state_load(buffer, size);
	}
	return SUCCESS;
}

static UINT16 upd9002_vector_offset(REG8 vect) {
	return LOADINTELWORD(mem + ((UINT)vect << 2));
}

static UINT16 upd9002_vector_segment(REG8 vect) {
	return LOADINTELWORD(mem + ((UINT)vect << 2) + 2);
}

void CPUCALL upd9002_core_brkem(REG8 vect) {
	UINT16 return_ip;

	if (CPU_COMPAT_MODE != UPD9002_COMPAT_NATIVE) {
		return;
	}
	return_ip = UPD9002_IP;
	REGPUSH0(upd9002_materialize_interrupt_saved_flags())
	REGPUSH0(UPD9002_CS)
	UPD9002_CS = upd9002_vector_segment(vect);
	CS_BASE = UPD9002_CS << 4;
	REGPUSH0(return_ip)
	UPD9002_IP = upd9002_vector_offset(vect);
	CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
	CPU_COMPAT_RETURN_PENDING = 0;
	UPD9002_WORKCLOCK(20);
	if (upd9002_compat_hooks.enter != NULL) {
		upd9002_compat_hooks.enter();
	}
}

void CPUCALL upd9002_core_compat_calln(REG8 vect, REG16 return_ip) {
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) {
		return;
	}
	REGPUSH0(upd9002_materialize_interrupt_saved_flags())
	REGPUSH0(UPD9002_CS)
	UPD9002_CS = upd9002_vector_segment(vect);
	CS_BASE = UPD9002_CS << 4;
	REGPUSH0(return_ip)
	upd9002_compat_return_sp = UPD9002_SP;
	UPD9002_IP = upd9002_vector_offset(vect);
	CPU_COMPAT_MODE = UPD9002_COMPAT_NATIVE;
	CPU_COMPAT_RETURN_PENDING = 1;
	UPD9002_WORKCLOCK(20);
}

void CPUCALL upd9002_core_compat_iotrap(REG8 vect, REG16 instruction_ip) {
	if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) {
		return;
	}
	upd9002_core_compat_calln(vect, instruction_ip);
	UPD9002_FLAG &= ~(I_FLAG | T_FLAG);
	UPD9002_TRAP = 0;
}

void CPUCALL upd9002_core_compat_irq(BOOL asserted) {
	if (upd9002_compat_hooks.irq != NULL) {
		upd9002_compat_hooks.irq(asserted);
	}
}

BOOL CPUCALL upd9002_core_compat_iret_is_return(void) {
	return (CPU_COMPAT_RETURN_PENDING != 0) && (UPD9002_SP == upd9002_compat_return_sp);
}

void CPUCALL upd9002_core_compat_retem(void) {
	UINT16 flag;

	if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008) {
		return;
	}
	REGPOP0(UPD9002_IP)
	REGPOP0(UPD9002_CS)
	REGPOP0(flag)
	CS_BASE = UPD9002_CS << 4;
	flag = (flag & 0x0fd7) | 0xf002;
	UPD9002_OV = flag & O_FLAG;
	UPD9002_FLAG = flag & (0xfff ^ O_FLAG);
	UPD9002_TRAP = ((flag & T_FLAG) != 0);
	CPU_COMPAT_MODE = UPD9002_COMPAT_NATIVE;
	CPU_COMPAT_RETURN_PENDING = 0;
	UPD9002_WORKCLOCK(31);
	if (upd9002_compat_hooks.leave != NULL) {
		upd9002_compat_hooks.leave();
	}
}

void CPUCALL upd9002_core_compat_iret_resume(void) {
	CPU_COMPAT_MODE = UPD9002_COMPAT_UPD70008;
	if (upd9002_compat_hooks.resume != NULL) {
		upd9002_compat_hooks.resume();
	}
}

void CPUCALL upd9002_intnum(UINT vect, REG16 IP) {
	upd9002_perf_record_exception((UINT8)vect);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "exception", (uint32_t)vect, (uint32_t)IP, 2);
#if defined(VAEG_UPD9002_SSTS_TESTING)
	upd9002_ssts_interrupt((uint8_t)vect);
#endif

	const BYTE *ptr;

	REGPUSH0(upd9002_materialize_interrupt_saved_flags())
	REGPUSH0(UPD9002_CS)
	REGPUSH0(IP)

	UPD9002_FLAG &= ~(T_FLAG | I_FLAG);
	UPD9002_TRAP = 0;

	ptr = mem + (vect * 4);
	UPD9002_IP = LOADINTELWORD(ptr + 0); // real mode!
	UPD9002_CS = LOADINTELWORD(ptr + 2); // real mode!
	CS_BASE = UPD9002_CS << 4;
	UPD9002_WORKCLOCK(20);
}

void CPUCALL upd9002_core_interrupt(REG8 vect) {
	upd9002_perf_record_interrupt(vect);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_DEVICE, "interrupt", (uint32_t)vect,
	                    (uint32_t)UPD9002_IP, 2);

	UINT op;
	const BYTE *ptr;

	if (CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008) {
		if (upd9002_compat_hooks.sync_to_native != NULL) {
			upd9002_compat_hooks.sync_to_native();
		}
		REGPUSH0(upd9002_materialize_interrupt_saved_flags())
		REGPUSH0(UPD9002_CS)
		REGPUSH0(UPD9002_IP)
		upd9002_compat_return_sp = UPD9002_SP;
		UPD9002_FLAG &= ~(T_FLAG | I_FLAG);
		UPD9002_TRAP = 0;
		ptr = mem + (vect * 4);
		UPD9002_IP = LOADINTELWORD(ptr + 0);
		UPD9002_CS = LOADINTELWORD(ptr + 2);
		CS_BASE = UPD9002_CS << 4;
		CPU_COMPAT_MODE = UPD9002_COMPAT_NATIVE;
		CPU_COMPAT_RETURN_PENDING = 1;
		UPD9002_WORKCLOCK(20);
		return;
	}

	op = upd9002_memoryread(UPD9002_IP + CS_BASE);
	if (op == 0xf4) { // hlt
		UPD9002_IP++;
	}
	REGPUSH0(REAL_FLAGREG) // preserve legacy interrupt flags
	REGPUSH0(UPD9002_CS)
	REGPUSH0(UPD9002_IP)

	UPD9002_FLAG &= ~(T_FLAG | I_FLAG);
	UPD9002_TRAP = 0;

	ptr = mem + (vect * 4);
	UPD9002_IP = LOADINTELWORD(ptr + 0); // real mode!
	UPD9002_CS = LOADINTELWORD(ptr + 2); // real mode!
	CS_BASE = UPD9002_CS << 4;
	UPD9002_WORKCLOCK(20);
}

// ---- test

#if defined(UPD9002_TEST)
BYTE BYTESZPF(UINT r) {
	if (r & (~0xff)) {
		TRACEOUT(("BYTESZPF bound error: %x", r));
	}
	return (iflags[r & 0xff]);
}

BYTE BYTESZPCF(UINT r) {
	if (r & (~0x1ff)) {
		TRACEOUT(("BYTESZPCF bound error: %x", r));
	}
	return (iflags[r & 0x1ff]);
}

BYTE WORDSZPF(UINT32 r) {
	BYTE f1;
	BYTE f2;

	if (r & (~0xffff)) {
		TRACEOUT(("WORDSZPF bound error: %x", r));
	}
	f1 = _szpflag16[r & 0xffff];
	f2 = iflags[r & 0xff] & P_FLAG;
	f2 += (r) ? 0 : Z_FLAG;
	f2 += (r >> 8) & S_FLAG;
	if (f1 != f2) {
		TRACEOUT(("word flag error: %.2x %.2x", f1, f2));
	}
	return (f1);
}

BYTE WORDSZPCF(UINT32 r) {
	BYTE f1;
	BYTE f2;

	if ((r & 0xffff0000) && (!(r & 0x00010000))) {
		TRACEOUT(("WORDSZPCF bound error: %x", r));
	}
	f1 = (r >> 16) & 1;
	f1 += _szpflag16[LOW16(r)];

	f2 = iflags[r & 0xff] & P_FLAG;
	f2 += (LOW16(r)) ? 0 : Z_FLAG;
	f2 += (r >> 8) & S_FLAG;
	f2 += (r >> 16) & 1;

	if (f1 != f2) {
		TRACEOUT(("word flag error: %.2x %.2x", f1, f2));
	}
	return (f1);
}
#endif
