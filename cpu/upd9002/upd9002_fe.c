#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "upd9002_ops.mcr"

// ------------------------------------------------------------ opecode 0xfe,f

#if 0
UPD9002_F6 _nop_int(UINT op) {
	INT_NUM(6, UPD9002_IP - 2);
}
#endif

UPD9002_F6 _inc_ea8(UINT op) {
	UINT32 madr;
	UINT8 *out;
	REG8 res;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			res = upd9002_memoryread(madr);
			INCBYTE(res)
			upd9002_memorywrite(madr, res);
			return;
		}
		out = mem + madr;
	}
	res = *out;
	INCBYTE(res)
	*out = (UINT8)res;
}

UPD9002_F6 _dec_ea8(UINT op) {
	UINT32 madr;
	UINT8 *out;
	REG8 res;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			res = upd9002_memoryread(madr);
			DECBYTE(res)
			upd9002_memorywrite(madr, res);
			return;
		}
		out = mem + madr;
	}
	res = *out;
	DECBYTE(res)
	*out = (UINT8)res;
}

UPD9002_F6 _inc_ea16(UINT op) {
	UINT32 madr;
	UINT16 *out;
	REG16 res;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			res = upd9002_memoryread_w(madr);
			INCWORD(res)
			upd9002_memorywrite_w(madr, res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	res = *out;
	INCWORD(res)
	*out = (UINT16)res;
}

UPD9002_F6 _dec_ea16(UINT op) {
	UINT32 madr;
	UINT16 *out;
	REG16 res;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			res = upd9002_memoryread_w(madr);
			DECWORD(res)
			upd9002_memorywrite_w(madr, res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	res = *out;
	DECWORD(res)
	*out = (UINT16)res;
}

UPD9002_F6 _call_ea16(UINT op) {
	UINT16 src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(7);
		src = *(REG16_B20(op));
	} else {
		UPD9002_WORKCLOCK(11);
		src = upd9002_memoryread_w(CALC_EA(op));
	}
	REGPUSH0(UPD9002_IP);
	UPD9002_IP = src;
}

UPD9002_F6 _call_far_ea16(UINT op) {
	UINT32 seg;
	UINT ad;

	UPD9002_WORKCLOCK(16);
	if (op < 0xc0) {
		ad = GET_EA(op, &seg);
		REGPUSH0(UPD9002_CS) // ToDo
		REGPUSH0(UPD9002_IP)
		UPD9002_IP = upd9002_memoryread_seg_w(seg, ad);
		UPD9002_CS = upd9002_memoryread_seg_w(seg, LOW16(ad + 2));
		CS_BASE = SEGSELECT(UPD9002_CS);
	} else {
		INT_NUM(6, UPD9002_IP - 2);
	}
}

UPD9002_F6 _jmp_ea16(UINT op) {
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(7);
		UPD9002_IP = *(REG16_B20(op));
	} else {
		UPD9002_WORKCLOCK(11);
		UPD9002_IP = upd9002_memoryread_w(CALC_EA(op));
	}
}

UPD9002_F6 _jmp_far_ea16(UINT op) {
	UINT32 seg;
	UINT ad;

	UPD9002_WORKCLOCK(11);
	if (op < 0xc0) {
		ad = GET_EA(op, &seg);
		UPD9002_IP = upd9002_memoryread_seg_w(seg, ad);
		UPD9002_CS = upd9002_memoryread_seg_w(seg, LOW16(ad + 2));
		CS_BASE = SEGSELECT(UPD9002_CS);
	} else {
		INT_NUM(6, UPD9002_IP - 2);
	}
}

UPD9002_F6 _push_ea16(UINT op) {
	UINT16 src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		if ((op & 7) == 4) {
			src = (UINT16)(UPD9002_SP - 2);
		} else {
			src = *(REG16_B20(op));
		}
	} else {
		UPD9002_WORKCLOCK(5);
		src = upd9002_memoryread_w(CALC_EA(op));
	}
	REGPUSH0(src);
}

UPD9002_F6 _push_ff7_ea16(UINT op) {
	UINT16 src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		if ((op & 7) == 4) {
			REGPUSH0(UPD9002_SP);
		} else {
			src = *(REG16_B20(op));
			REGPUSH0(src);
		}
	} else {
		UPD9002_WORKCLOCK(5);
		src = upd9002_memoryread_w(CALC_EA(op));
		REGPUSH0(src);
	}
}

const UPD9002OPF6 c_ope0xfe_table[] = {_inc_ea8, _dec_ea8};

const UPD9002OPF6 c_ope0xff_table[] = {_inc_ea16, _dec_ea16,     _call_ea16, _call_far_ea16,
                                       _jmp_ea16, _jmp_far_ea16, _push_ea16, _push_ff7_ea16};
