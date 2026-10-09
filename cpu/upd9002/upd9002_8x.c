#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "upd9002_ops.mcr"

// -------------------------------------------------------- opecode 0x80,1,2,3

// ----- reg8

UPD9002_8X _add_r8_i(BYTE *p) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = *p;
	ADDBYTE(res, dst, src);
	*p = (BYTE)res;
}

UPD9002_8X _or_r8_i(BYTE *p) {
	UINT src;
	UINT dst;

	GET_PCBYTE(src)
	dst = *p;
	ORBYTE(dst, src);
	*p = (BYTE)dst;
}

UPD9002_8X _adc_r8_i(BYTE *p) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = *p;
	ADCBYTE(res, dst, src);
	*p = (BYTE)res;
}

UPD9002_8X _sbb_r8_i(BYTE *p) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = *p;
	SBBBYTE(res, dst, src);
	*p = (BYTE)res;
}

UPD9002_8X _and_r8_i(BYTE *p) {
	UINT src;
	UINT dst;

	GET_PCBYTE(src)
	dst = *p;
	ANDBYTE(dst, src);
	*p = (BYTE)dst;
}

UPD9002_8X _sub_r8_i(BYTE *p) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = *p;
	SUBBYTE(res, dst, src);
	*p = (BYTE)res;
}

UPD9002_8X _xor_r8_i(BYTE *p) {
	UINT src;
	UINT dst;

	GET_PCBYTE(src)
	dst = *p;
	XORBYTE(dst, src);
	*p = (BYTE)dst;
}

UPD9002_8X _cmp_r8_i(BYTE *p) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = *p;
	SUBBYTE(res, dst, src);
}

// ----- ext8

UPD9002_8X _add_ext8_i(UINT32 madr) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = upd9002_memoryread(madr);
	ADDBYTE(res, dst, src);
	upd9002_memorywrite(madr, (REG8)res);
}

UPD9002_8X _or_ext8_i(UINT32 madr) {
	UINT src;
	UINT dst;

	GET_PCBYTE(src)
	dst = upd9002_memoryread(madr);
	ORBYTE(dst, src);
	upd9002_memorywrite(madr, (REG8)dst);
}

UPD9002_8X _adc_ext8_i(UINT32 madr) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = upd9002_memoryread(madr);
	ADCBYTE(res, dst, src);
	upd9002_memorywrite(madr, (REG8)res);
}

UPD9002_8X _sbb_ext8_i(UINT32 madr) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = upd9002_memoryread(madr);
	SBBBYTE(res, dst, src);
	upd9002_memorywrite(madr, (REG8)res);
}

UPD9002_8X _and_ext8_i(UINT32 madr) {
	UINT src;
	UINT dst;

	GET_PCBYTE(src)
	dst = upd9002_memoryread(madr);
	ANDBYTE(dst, src);
	upd9002_memorywrite(madr, (REG8)dst);
}

UPD9002_8X _sub_ext8_i(UINT32 madr) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = upd9002_memoryread(madr);
	SUBBYTE(res, dst, src);
	upd9002_memorywrite(madr, (REG8)res);
}

UPD9002_8X _xor_ext8_i(UINT32 madr) {
	UINT src;
	UINT dst;

	GET_PCBYTE(src)
	dst = upd9002_memoryread(madr);
	XORBYTE(dst, src);
	upd9002_memorywrite(madr, (REG8)dst);
}

UPD9002_8X _cmp_ext8_i(UINT32 madr) {
	UINT src;
	UINT dst;
	UINT res;

	GET_PCBYTE(src)
	dst = upd9002_memoryread(madr);
	SUBBYTE(res, dst, src);
}

const UPD9002OP8XREG8 c_op8xreg8_table[] = {_add_r8_i, _or_r8_i,  _adc_r8_i, _sbb_r8_i,
                                            _and_r8_i, _sub_r8_i, _xor_r8_i, _cmp_r8_i};

const UPD9002OP8XEXT8 c_op8xext8_table[] = {_add_ext8_i, _or_ext8_i,  _adc_ext8_i, _sbb_ext8_i,
                                            _and_ext8_i, _sub_ext8_i, _xor_ext8_i, _cmp_ext8_i};

// -------------------------------------------------------------------------

// ----- reg16

UPD9002_8X _add_r16_i(UINT16 *p, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = *p;
	ADDWORD(res, dst, src);
	*p = (UINT16)res;
}

UPD9002_8X _or_r16_i(UINT16 *p, UINT32 src) {
	UINT32 dst;

	dst = *p;
	ORWORD(dst, src);
	*p = (UINT16)dst;
}

UPD9002_8X _adc_r16_i(UINT16 *p, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = *p;
	ADCWORD(res, dst, src);
	*p = (UINT16)res;
}

UPD9002_8X _sbb_r16_i(UINT16 *p, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = *p;
	SBBWORD(res, dst, src);
	*p = (UINT16)res;
}

UPD9002_8X _and_r16_i(UINT16 *p, UINT32 src) {
	UINT32 dst;

	dst = *p;
	ANDWORD(dst, src);
	*p = (UINT16)dst;
}

UPD9002_8X _sub_r16_i(UINT16 *p, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = *p;
	SUBWORD(res, dst, src);
	*p = (UINT16)res;
}

UPD9002_8X _xor_r16_i(UINT16 *p, UINT32 src) {
	UINT32 dst;

	dst = *p;
	XORWORD(dst, src);
	*p = (UINT16)dst;
}

UPD9002_8X _cmp_r16_i(UINT16 *p, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = *p;
	SUBWORD(res, dst, src);
}

// ----- ext16

UPD9002_8X _add_ext16_i(UINT32 madr, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = upd9002_memoryread_w(madr);
	ADDWORD(res, dst, src);
	upd9002_memorywrite_w(madr, (REG16)res);
}

UPD9002_8X _or_ext16_i(UINT32 madr, UINT32 src) {
	UINT32 dst;

	dst = upd9002_memoryread_w(madr);
	ORWORD(dst, src);
	upd9002_memorywrite_w(madr, (REG16)dst);
}

UPD9002_8X _adc_ext16_i(UINT32 madr, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = upd9002_memoryread_w(madr);
	ADCWORD(res, dst, src);
	upd9002_memorywrite_w(madr, (REG16)res);
}

UPD9002_8X _sbb_ext16_i(UINT32 madr, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = upd9002_memoryread_w(madr);
	SBBWORD(res, dst, src);
	upd9002_memorywrite_w(madr, (REG16)res);
}

UPD9002_8X _and_ext16_i(UINT32 madr, UINT32 src) {
	UINT32 dst;

	dst = upd9002_memoryread_w(madr);
	ANDWORD(dst, src);
	upd9002_memorywrite_w(madr, (REG16)dst);
}

UPD9002_8X _sub_ext16_i(UINT32 madr, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = upd9002_memoryread_w(madr);
	SUBWORD(res, dst, src);
	upd9002_memorywrite_w(madr, (REG16)res);
}

UPD9002_8X _xor_ext16_i(UINT32 madr, UINT32 src) {
	UINT32 dst;

	dst = upd9002_memoryread_w(madr);
	XORWORD(dst, src);
	upd9002_memorywrite_w(madr, (REG16)dst);
}

UPD9002_8X _cmp_ext16_i(UINT32 madr, UINT32 src) {
	UINT32 dst;
	UINT32 res;

	dst = upd9002_memoryread_w(madr);
	SUBWORD(res, dst, src);
}

const UPD9002OP8XREG16 c_op8xreg16_table[] = {_add_r16_i, _or_r16_i,  _adc_r16_i, _sbb_r16_i,
                                              _and_r16_i, _sub_r16_i, _xor_r16_i, _cmp_r16_i};

const UPD9002OP8XEXT16 c_op8xext16_table[] = {_add_ext16_i, _or_ext16_i,  _adc_ext16_i,
                                              _sbb_ext16_i, _and_ext16_i, _sub_ext16_i,
                                              _xor_ext16_i, _cmp_ext16_i};
