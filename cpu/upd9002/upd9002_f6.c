#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "upd9002_ops.mcr"

// ------------------------------------------------------------ opecode 0xf6,7

UPD9002_F6 _test_ea8_data8(UINT op) {
	UINT src;
	UINT dst;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		dst = *(REG8_B20(op));
	} else {
		UPD9002_WORKCLOCK(6);
		dst = upd9002_memoryread(CALC_EA(op));
	}
	GET_PCBYTE(src)
	ANDBYTE(dst, src)
}

UPD9002_F6 _not_ea8(UINT op) {
	UINT32 madr;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		*(REG8_B20(op)) ^= 0xff;
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			REG8 value = upd9002_memoryread(madr);
			value ^= 0xff;
			upd9002_memorywrite(madr, value);
			return;
		}
		*(mem + madr) ^= 0xff;
	}
}

UPD9002_F6 _neg_ea8(UINT op) {
	UINT8 *out;
	UINT src;
	UINT dst;
	UINT32 madr;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			src = upd9002_memoryread(madr);
			NEGBYTE(dst, src)
			upd9002_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	src = *out;
	NEGBYTE(dst, src)
	*out = (UINT8)dst;
}

UPD9002_F6 _mul_ea8(UINT op) {
	BYTE src;
	UINT res;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(13);
		src = *(REG8_B20(op));
	} else {
		UPD9002_WORKCLOCK(16);
		src = upd9002_memoryread(CALC_EA(op));
	}
	BYTE_MUL(res, UPD9002_AL, src)
	UPD9002_AX = (UINT16)res;
}

UPD9002_F6 _imul_ea8(UINT op) {
	BYTE src;
	SINT32 res;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(13);
		src = *(REG8_B20(op));
	} else {
		UPD9002_WORKCLOCK(16);
		src = upd9002_memoryread(CALC_EA(op));
	}
	BYTE_IMUL(res, UPD9002_AL, src)
	UPD9002_AX = (UINT16)res;
}

UPD9002_F6 _div_ea8(UINT op) {
	UINT16 dividend;
	UINT flag_result;
	UINT8 src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(14);
		src = *(REG8_B20(op));
	} else {
		UPD9002_WORKCLOCK(17);
		src = upd9002_memoryread(CALC_EA(op));
	}
	dividend = UPD9002_AX;
	SUBBYTE(flag_result, UPD9002_AH, src)
	UPD9002_FLAGL |= 0x02;
	if (!src || (UPD9002_AH >= src)) {
		INT_NUM(0, UPD9002_IP); // uPD9002
		return;
	}
	UPD9002_AL = dividend / src;
	UPD9002_AH = dividend % src;
}

UPD9002_F6 _idiv_ea8(UINT op) {
	SINT32 dividend;
	SINT32 divisor;
	UINT32 dividend_magnitude;
	UINT32 divisor_magnitude;
	UINT32 quotient;
	UINT32 remainder;
	UINT flag_result;
	UINT8 src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(17);
		src = *(REG8_B20(op));
	} else {
		UPD9002_WORKCLOCK(20);
		src = upd9002_memoryread(CALC_EA(op));
	}
	dividend = (SINT16)UPD9002_AX;
	divisor = (SINT8)src;
	dividend_magnitude = (UINT32)((dividend < 0) ? -dividend : dividend);
	divisor_magnitude = (UINT32)((divisor < 0) ? -divisor : divisor);
	SUBBYTE(flag_result, dividend_magnitude >> 8, divisor_magnitude)
	UPD9002_FLAGL |= 0x02;
	if (!divisor_magnitude || ((dividend_magnitude >> 8) >= divisor_magnitude)) {
		INT_NUM(0, UPD9002_IP); // uPD9002
		return;
	}
	quotient = dividend_magnitude / divisor_magnitude;
	remainder = dividend_magnitude % divisor_magnitude;
	UPD9002_OV = 0;
	UPD9002_FLAGL = BYTESZPF(quotient) | 0x02;
	if (quotient >= 0x80) {
		INT_NUM(0, UPD9002_IP); // uPD9002
		return;
	}
	UPD9002_AL = (UINT8)(((dividend < 0) != (divisor < 0)) ? (0U - quotient) : quotient);
	UPD9002_AH = (UINT8)((dividend < 0) ? (0U - remainder) : remainder);
}

UPD9002_F6 _test_ea16_data16(UINT op) {
	UINT32 src;
	UINT32 dst;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		dst = *(REG16_B20(op));
	} else {
		UPD9002_WORKCLOCK(6);
		dst = upd9002_memoryread_w(CALC_EA(op));
	}
	GET_PCWORD(src)
	ANDWORD(dst, src)
}

UPD9002_F6 _not_ea16(UINT op) {
	UINT32 madr;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		*(REG16_B20(op)) ^= 0xffff;
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (!(INHIBIT_WORDP(madr))) {
			REG16 value = LOADINTELWORD(mem + madr);
			value = (REG16)~value;
			STOREINTELWORD(mem + madr, value);
		} else {
			REG16 value = upd9002_memoryread_w(madr);
			value = ~value;
			upd9002_memorywrite_w(madr, value);
		}
	}
}

UPD9002_F6 _neg_ea16(UINT op) {
	UINT16 *out;
	UINT32 src;
	UINT32 dst;
	UINT32 madr;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			src = upd9002_memoryread_w(madr);
			NEGWORD(dst, src)
			upd9002_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	src = *out;
	NEGWORD(dst, src)
	*out = (UINT16)dst;
}

UPD9002_F6 _mul_ea16(UINT op) {
	UINT16 src;
	UINT32 res;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(21);
		src = *(REG16_B20(op));
	} else {
		UPD9002_WORKCLOCK(24);
		src = upd9002_memoryread_w(CALC_EA(op));
	}
	WORD_MUL(res, UPD9002_AX, src)
	UPD9002_AX = (UINT16)res;
	UPD9002_DX = (UINT16)(res >> 16);
}

UPD9002_F6 _imul_ea16(UINT op) {
	SINT16 src;
	SINT32 res;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(21);
		src = *(REG16_B20(op));
	} else {
		UPD9002_WORKCLOCK(24);
		src = upd9002_memoryread_w(CALC_EA(op));
	}
	WORD_IMUL(res, UPD9002_AX, src)
	UPD9002_AX = (UINT16)res;
	UPD9002_DX = (UINT16)(res >> 16);
}

static UINT16 _div_read_ea16(UINT op) {
	UINT offset;
	UINT32 segment;

	offset = GET_EA(op, &segment);
	return (UINT16)(upd9002_memoryread(segment + offset) |
	                (upd9002_memoryread(segment + LOW16(offset + 1)) << 8));
}

UPD9002_F6 _div_ea16(UINT op) {
	UINT32 dividend;
	UINT32 src;
	UINT32 flag_result;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(22);
		src = *(REG16_B20(op));
	} else {
		UPD9002_WORKCLOCK(25);
		src = _div_read_ea16(op);
	}
	dividend = ((UINT32)UPD9002_DX << 16) | UPD9002_AX;
	SUBWORD(flag_result, UPD9002_DX, src)
	UPD9002_FLAGL |= 0x02;
	if (!src || (UPD9002_DX >= src)) {
		INT_NUM(0, UPD9002_IP); // uPD9002
		return;
	}
	UPD9002_AX = dividend / src;
	UPD9002_DX = dividend % src;
}

UPD9002_F6 _idiv_ea16(UINT op) {
	SINT64 dividend;
	SINT64 divisor;
	UINT64 dividend_magnitude;
	UINT32 divisor_magnitude;
	UINT32 quotient;
	UINT32 remainder;
	UINT32 flag_result;
	UINT16 src;

	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(25);
		src = *(REG16_B20(op));
	} else {
		UPD9002_WORKCLOCK(28);
		src = _div_read_ea16(op);
	}
	dividend = (SINT32)(((UINT32)UPD9002_DX << 16) | UPD9002_AX);
	divisor = (SINT16)src;
	dividend_magnitude = (UINT64)((dividend < 0) ? -dividend : dividend);
	divisor_magnitude = (UINT32)((divisor < 0) ? -divisor : divisor);
	SUBWORD(flag_result, dividend_magnitude >> 16, divisor_magnitude)
	UPD9002_FLAGL |= 0x02;
	if (!divisor_magnitude || ((dividend_magnitude >> 16) >= divisor_magnitude)) {
		INT_NUM(0, UPD9002_IP); // uPD9002
		return;
	}
	quotient = (UINT32)(dividend_magnitude / divisor_magnitude);
	remainder = (UINT32)(dividend_magnitude % divisor_magnitude);
	UPD9002_OV = 0;
	UPD9002_FLAGL = WORDSZPF(quotient) | 0x02;
	if (quotient >= 0x8000) {
		INT_NUM(0, UPD9002_IP); // uPD9002
		return;
	}
	UPD9002_AX = (UINT16)(((dividend < 0) != (divisor < 0)) ? (0U - quotient) : quotient);
	UPD9002_DX = (UINT16)((dividend < 0) ? (0U - remainder) : remainder);
}

const UPD9002OPF6 c_ope0xf6_table[] = {_test_ea8_data8, _test_ea8_data8, _not_ea8, _neg_ea8,
                                       _mul_ea8,        _imul_ea8,       _div_ea8, _idiv_ea8};

const UPD9002OPF6 c_ope0xf7_table[] = {_test_ea16_data16, _test_ea16_data16, _not_ea16, _neg_ea16,
                                       _mul_ea16,         _imul_ea16,        _div_ea16, _idiv_ea16};
