#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "upd9002_ops.mcr"

// ---------------------------------------------------------------------- ins

UPD9002EXT upd9002_rep_insb(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		do {
			REG8 dat = iocore_inp8(UPD9002_DX);
			upd9002_memorywrite(UPD9002_DI + ES_BASE, dat);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(4);
		} while (--UPD9002_CX);
	}
}

UPD9002EXT upd9002_rep_insw(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		do {
			REG16 dat = iocore_inp16(UPD9002_DX);
			upd9002_memorywrite_w(UPD9002_DI + ES_BASE, dat);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(4);
		} while (--UPD9002_CX);
	}
}

// ---------------------------------------------------------------------- outs

UPD9002EXT upd9002_rep_outsb(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		do {
			REG8 dat = upd9002_memoryread(UPD9002_SI + DS_FIX);
			UPD9002_SI += stp;
			iocore_out8(UPD9002_DX, (BYTE)dat);
			UPD9002_WORKCLOCK(4);
		} while (--UPD9002_CX);
	}
}

UPD9002EXT upd9002_rep_outsw(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		do {
			REG16 dat = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
			UPD9002_SI += stp;
			iocore_out16(UPD9002_DX, (UINT16)dat);
			UPD9002_WORKCLOCK(4);
		} while (--UPD9002_CX);
	}
}

// ---------------------------------------------------------------------- movs

#if 1
UPD9002EXT upd9002_rep_movsb(void) {
	UINT16 r_cx;
	int stp;
	UINT16 r_si;
	UINT16 r_di;

	UPD9002_WORKCLOCK(5);
	r_cx = UPD9002_CX;
	if (r_cx) {
		stp = STRING_DIR;
		r_si = UPD9002_SI;
		r_di = UPD9002_DI;
		while (1) {
			REG8 dat = upd9002_memoryread(DS_FIX + r_si);
			upd9002_memorywrite(ES_BASE + r_di, dat);
			r_si += stp;
			r_di += stp;
			UPD9002_WORKCLOCK(4);
			r_cx--;
			if (!r_cx) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
		UPD9002_CX = r_cx;
		UPD9002_SI = r_si;
		UPD9002_DI = r_di;
	}
}

UPD9002EXT upd9002_rep_movsw(void) {
	UINT16 r_cx;
	int stp;
	UINT16 r_si;
	UINT16 r_di;

	UPD9002_WORKCLOCK(5);
	r_cx = UPD9002_CX;
	if (r_cx) {
		stp = STRING_DIRx2;
		r_si = UPD9002_SI;
		r_di = UPD9002_DI;
		while (1) {
			REG16 dat = upd9002_memoryread_seg_w(DS_FIX, r_si);
			upd9002_memorywrite_seg_w(ES_BASE, r_di, dat);
			r_si += stp;
			r_di += stp;
			UPD9002_WORKCLOCK(4);
			r_cx--;
			if (!r_cx) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
		UPD9002_CX = r_cx;
		UPD9002_SI = r_si;
		UPD9002_DI = r_di;
	}
}
#else
UPD9002EXT upd9002_rep_movsb(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		while (1) {
			REG8 dat = upd9002_memoryread(UPD9002_SI + DS_FIX);
			upd9002_memorywrite(UPD9002_DI + ES_BASE, dat);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(4);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

UPD9002EXT upd9002_rep_movsw(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		while (1) {
			REG16 dat = upd9002_memoryread_seg_w(DS_FIX, UPD9002_SI);
			upd9002_memorywrite_seg_w(ES_BASE, UPD9002_DI, dat);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(4);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}
#endif

// ---------------------------------------------------------------------- lods

UPD9002EXT upd9002_rep_lodsb(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		while (1) {
			UPD9002_AL = upd9002_memoryread(UPD9002_SI + DS_FIX);
			UPD9002_SI += stp;
			UPD9002_WORKCLOCK(4);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

UPD9002EXT upd9002_rep_lodsw(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		while (1) {
			UPD9002_AX = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
			UPD9002_SI += stp;
			UPD9002_WORKCLOCK(4);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------- stos

UPD9002EXT upd9002_rep_stosb(void) {
	UPD9002_WORKCLOCK(4);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		while (1) {
			upd9002_memorywrite(UPD9002_DI + ES_BASE, UPD9002_AL);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(3);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

UPD9002EXT upd9002_rep_stosw(void) {
	UPD9002_WORKCLOCK(4);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		while (1) {
			upd9002_memorywrite_w(UPD9002_DI + ES_BASE, UPD9002_AX);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(3);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------- cmps

UPD9002EXT upd9002_repe_cmpsb(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		do {
			UINT res;
			UINT dst = upd9002_memoryread(UPD9002_SI + DS_FIX);
			UINT src = upd9002_memoryread(UPD9002_DI + ES_BASE);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(9);
			SUBBYTE(res, dst, src)
			UPD9002_CX--;
		} while ((UPD9002_CX) && (UPD9002_FLAGL & Z_FLAG));
	}
}

UPD9002EXT upd9002_repne_cmpsb(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		do {
			UINT res;
			UINT dst = upd9002_memoryread(UPD9002_SI + DS_FIX);
			UINT src = upd9002_memoryread(UPD9002_DI + ES_BASE);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(9);
			SUBBYTE(res, dst, src)
			UPD9002_CX--;
		} while ((UPD9002_CX) && (!(UPD9002_FLAGL & Z_FLAG)));
	}
}

UPD9002EXT upd9002_repe_cmpsw(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		do {
			UINT32 res;
			UINT32 dst = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
			UINT32 src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(9);
			SUBWORD(res, dst, src)
			UPD9002_CX--;
		} while ((UPD9002_CX) && (UPD9002_FLAGL & Z_FLAG));
	}
}

UPD9002EXT upd9002_repne_cmpsw(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		do {
			UINT32 res;
			UINT32 dst = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
			UINT32 src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
			UPD9002_SI += stp;
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(9);
			SUBWORD(res, dst, src)
			UPD9002_CX--;
		} while ((UPD9002_CX) && (!(UPD9002_FLAGL & Z_FLAG)));
	}
}

// ---------------------------------------------------------------------- scas

UPD9002EXT upd9002_repe_scasb(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		UINT dst = UPD9002_AL;
		do {
			UINT res;
			UINT src = upd9002_memoryread(UPD9002_DI + ES_BASE);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(8);
			SUBBYTE(res, dst, src)
			UPD9002_CX--;
		} while ((UPD9002_CX) && (UPD9002_FLAGL & Z_FLAG));
	}
}

UPD9002EXT upd9002_repne_scasb(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIR;
		UINT dst = UPD9002_AL;
		do {
			UINT res;
			UINT src = upd9002_memoryread(UPD9002_DI + ES_BASE);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(8);
			SUBBYTE(res, dst, src)
			UPD9002_CX--;
		} while ((UPD9002_CX) && (!(UPD9002_FLAGL & Z_FLAG)));
	}
}

UPD9002EXT upd9002_repe_scasw(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		UINT32 dst = UPD9002_AX;
		do {
			UINT32 res;
			UINT32 src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(8);
			SUBWORD(res, dst, src)
			UPD9002_CX--;
		} while ((UPD9002_CX) && (UPD9002_FLAGL & Z_FLAG));
	}
}

UPD9002EXT upd9002_repne_scasw(void) {
	UPD9002_WORKCLOCK(5);
	if (UPD9002_CX) {
		int stp = STRING_DIRx2;
		UINT32 dst = UPD9002_AX;
		do {
			UINT32 res;
			UINT32 src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
			UPD9002_DI += stp;
			UPD9002_WORKCLOCK(8);
			SUBWORD(res, dst, src)
			UPD9002_CX--;
		} while ((UPD9002_CX) && (!(UPD9002_FLAGL & Z_FLAG)));
	}
}

// ------------------------------------------------------------ repnc / repc

typedef enum {
	UPD9002_CARRY_STRING_MOVSB,
	UPD9002_CARRY_STRING_MOVSW,
	UPD9002_CARRY_STRING_CMPSB,
	UPD9002_CARRY_STRING_CMPSW,
	UPD9002_CARRY_STRING_STOSB,
	UPD9002_CARRY_STRING_STOSW,
	UPD9002_CARRY_STRING_LODSB,
	UPD9002_CARRY_STRING_LODSW,
	UPD9002_CARRY_STRING_SCASB,
	UPD9002_CARRY_STRING_SCASW
} UPD9002_CARRY_STRING_OP;

static BOOL upd9002_carry_repeat_continue(BOOL repeat_on_carry) {
	return (((UPD9002_FLAGL & C_FLAG) != 0) == repeat_on_carry);
}

static BOOL upd9002_carry_string_uses_carry(UPD9002_CARRY_STRING_OP op) {
	switch (op) {
	case UPD9002_CARRY_STRING_CMPSB:
	case UPD9002_CARRY_STRING_CMPSW:
	case UPD9002_CARRY_STRING_SCASB:
	case UPD9002_CARRY_STRING_SCASW:
		return (TRUE);

	default:
		return (FALSE);
	}
}

static int upd9002_carry_string_start_clock(UPD9002_CARRY_STRING_OP op) {
	switch (op) {
	case UPD9002_CARRY_STRING_STOSB:
	case UPD9002_CARRY_STRING_STOSW:
		return (4);

	default:
		return (5);
	}
}

static int upd9002_carry_string_iteration_clock(UPD9002_CARRY_STRING_OP op) {
	switch (op) {
	case UPD9002_CARRY_STRING_CMPSB:
	case UPD9002_CARRY_STRING_CMPSW:
		return (9);

	case UPD9002_CARRY_STRING_SCASB:
	case UPD9002_CARRY_STRING_SCASW:
		return (8);

	case UPD9002_CARRY_STRING_STOSB:
	case UPD9002_CARRY_STRING_STOSW:
		return (3);

	default:
		return (4);
	}
}

static void upd9002_carry_string_one(UPD9002_CARRY_STRING_OP op) {
	int stp;
	UINT res;
	UINT32 res32;
	UINT dst;
	UINT src;
	UINT32 dst32;
	UINT32 src32;

	switch (op) {
	case UPD9002_CARRY_STRING_MOVSB:
		stp = STRING_DIR;
		src = upd9002_memoryread(DS_FIX + UPD9002_SI);
		upd9002_memorywrite(ES_BASE + UPD9002_DI, (REG8)src);
		UPD9002_SI = (UINT16)(UPD9002_SI + stp);
		UPD9002_DI = (UINT16)(UPD9002_DI + stp);
		break;

	case UPD9002_CARRY_STRING_MOVSW:
		stp = STRING_DIRx2;
		src32 = upd9002_memoryread_seg_w(DS_FIX, UPD9002_SI);
		upd9002_memorywrite_seg_w(ES_BASE, UPD9002_DI, (REG16)src32);
		UPD9002_SI = (UINT16)(UPD9002_SI + stp);
		UPD9002_DI = (UINT16)(UPD9002_DI + stp);
		break;

	case UPD9002_CARRY_STRING_CMPSB:
		stp = STRING_DIR;
		dst = upd9002_memoryread(DS_FIX + UPD9002_SI);
		src = upd9002_memoryread(ES_BASE + UPD9002_DI);
		UPD9002_SI = (UINT16)(UPD9002_SI + stp);
		UPD9002_DI = (UINT16)(UPD9002_DI + stp);
		SUBBYTE(res, dst, src)
		break;

	case UPD9002_CARRY_STRING_CMPSW:
		stp = STRING_DIRx2;
		dst32 = upd9002_memoryread_seg_w(DS_FIX, UPD9002_SI);
		src32 = upd9002_memoryread_seg_w(ES_BASE, UPD9002_DI);
		UPD9002_SI = (UINT16)(UPD9002_SI + stp);
		UPD9002_DI = (UINT16)(UPD9002_DI + stp);
		SUBWORD(res32, dst32, src32)
		break;

	case UPD9002_CARRY_STRING_STOSB:
		stp = STRING_DIR;
		upd9002_memorywrite(ES_BASE + UPD9002_DI, UPD9002_AL);
		UPD9002_DI = (UINT16)(UPD9002_DI + stp);
		break;

	case UPD9002_CARRY_STRING_STOSW:
		stp = STRING_DIRx2;
		upd9002_memorywrite_seg_w(ES_BASE, UPD9002_DI, UPD9002_AX);
		UPD9002_DI = (UINT16)(UPD9002_DI + stp);
		break;

	case UPD9002_CARRY_STRING_LODSB:
		stp = STRING_DIR;
		UPD9002_AL = upd9002_memoryread(DS_FIX + UPD9002_SI);
		UPD9002_SI = (UINT16)(UPD9002_SI + stp);
		break;

	case UPD9002_CARRY_STRING_LODSW:
		stp = STRING_DIRx2;
		UPD9002_AX = upd9002_memoryread_seg_w(DS_FIX, UPD9002_SI);
		UPD9002_SI = (UINT16)(UPD9002_SI + stp);
		break;

	case UPD9002_CARRY_STRING_SCASB:
		stp = STRING_DIR;
		dst = UPD9002_AL;
		src = upd9002_memoryread(ES_BASE + UPD9002_DI);
		UPD9002_DI = (UINT16)(UPD9002_DI + stp);
		SUBBYTE(res, dst, src)
		break;

	case UPD9002_CARRY_STRING_SCASW:
		stp = STRING_DIRx2;
		dst32 = UPD9002_AX;
		src32 = upd9002_memoryread_seg_w(ES_BASE, UPD9002_DI);
		UPD9002_DI = (UINT16)(UPD9002_DI + stp);
		SUBWORD(res32, dst32, src32)
		break;
	}
	UPD9002_WORKCLOCK(upd9002_carry_string_iteration_clock(op));
}

static void upd9002_repeat_carry_string(UPD9002_CARRY_STRING_OP op, BOOL repeat_on_carry) {
	UPD9002_WORKCLOCK(upd9002_carry_string_start_clock(op));
	if (UPD9002_CX) {
		while (1) {
			upd9002_carry_string_one(op);
			UPD9002_CX--;
			if (!UPD9002_CX) {
				break;
			}
			if (upd9002_carry_string_uses_carry(op) &&
			    !upd9002_carry_repeat_continue(repeat_on_carry)) {
				break;
			}
			if (UPD9002_REMCLOCK <= 0) {
				UPD9002_IP -= UPD9002_PREFIX + 1;
				break;
			}
		}
	}
}

#define UPD9002_CARRY_REPEAT_WRAPPER(name, op, repeat_on_carry)                                    \
	UPD9002EXT name(void) {                                                                        \
		upd9002_repeat_carry_string(op, repeat_on_carry);                                          \
	}

UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_movsb, UPD9002_CARRY_STRING_MOVSB, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_movsb, UPD9002_CARRY_STRING_MOVSB, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_movsw, UPD9002_CARRY_STRING_MOVSW, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_movsw, UPD9002_CARRY_STRING_MOVSW, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_cmpsb, UPD9002_CARRY_STRING_CMPSB, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_cmpsb, UPD9002_CARRY_STRING_CMPSB, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_cmpsw, UPD9002_CARRY_STRING_CMPSW, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_cmpsw, UPD9002_CARRY_STRING_CMPSW, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_stosb, UPD9002_CARRY_STRING_STOSB, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_stosb, UPD9002_CARRY_STRING_STOSB, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_stosw, UPD9002_CARRY_STRING_STOSW, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_stosw, UPD9002_CARRY_STRING_STOSW, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_lodsb, UPD9002_CARRY_STRING_LODSB, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_lodsb, UPD9002_CARRY_STRING_LODSB, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_lodsw, UPD9002_CARRY_STRING_LODSW, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_lodsw, UPD9002_CARRY_STRING_LODSW, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_scasb, UPD9002_CARRY_STRING_SCASB, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_scasb, UPD9002_CARRY_STRING_SCASB, TRUE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repnc_scasw, UPD9002_CARRY_STRING_SCASW, FALSE)
UPD9002_CARRY_REPEAT_WRAPPER(upd9002_repc_scasw, UPD9002_CARRY_STRING_SCASW, TRUE)
