#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "upd9002_trace.h"
#include "upd9002_perf.h"
#include "upd9002_diagnostic.h"
#include "upd8087/upd8087.h"
#include "dmap.h"
#include "upd9002_regs.h"
#include "upd9002_ops.mcr"
#if defined(VAEG_UPD9002_M46_TESTING)
#include <stdlib.h>
#endif

#define MAX_PREFIX 8

#define NEXT_OPCODE                                                                                \
	if (UPD9002_REMCLOCK < 1) {                                                                    \
		UPD9002_BASECLOCK += (1 - UPD9002_REMCLOCK);                                               \
		UPD9002_REMCLOCK = 1;                                                                      \
	}

#define REMAIN_ADJUST(c)                                                                           \
	if (UPD9002_REMCLOCK != (c)) {                                                                 \
		UPD9002_BASECLOCK += ((c) - UPD9002_REMCLOCK);                                             \
		UPD9002_REMCLOCK = (c);                                                                    \
	}

static UINT16 upd9002_repnc_ipbak;
static UINT16 upd9002_repc_ipbak;

static UINT16 _materialize_pushf_image(void);
static UINT8 _shift8(UINT8 value, UINT count, UINT subform);
static UINT16 _shift16(UINT16 value, UINT count, UINT subform);
static UINT8 _ea8_read(UINT op, UINT32 *madr);
static void _ea8_write(UINT op, UINT32 madr, UINT8 value);
static UINT16 _ea16_read(UINT op, UINT32 *madr);
static void _ea16_write(UINT op, UINT32 madr, UINT16 value);
static void _adjust_flags(UINT8 value, BOOL adjust_low, BOOL adjust_high, UINT overflow);

static UINT8 upd9002_8087_read8(void *opaque, UINT32 address) {
	(void)opaque;
	return (UINT8)upd9002_memoryread(address);
}

static void upd9002_8087_write8(void *opaque, UINT32 address, UINT8 value) {
	(void)opaque;
	upd9002_memorywrite(address, value);
}

static UINT8 upd9002_8087_primary_opcode(void) {
	UINT16 ip = upd9002_step_start_ip;
	UINT8 opcode;

	opcode = (UINT8)upd9002_memoryread(CS_BASE + ip);
	while ((opcode == 0x26) || (opcode == 0x2e) || (opcode == 0x36) ||
	       (opcode == 0x3e) || (opcode == 0xf2) || (opcode == 0xf3)) {
		ip++;
		opcode = (UINT8)upd9002_memoryread(CS_BASE + ip);
	}
	return opcode;
}

// ----

static const UINT8 shiftbase16[256] = {
    0,  1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15, 16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15};

static const UINT8 shiftbase09[256] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4,
    5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1,
    2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6,
    7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2,
    3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3};

static const UINT8 shiftbase17[256] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,  2,  3,  4,  5,  6,
    7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
    14, 15, 16, 17, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,  2,  3,
    4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,  2,  3,  4,  5,  6,  7,
    8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,  2,  3,  4,
    5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
    12, 13, 14, 15, 16, 17, 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 1,
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17};

UPD9002FN _add_ea_r8(void) { // 00: add EA, REG8

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;
	UINT32 madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			ADDBYTE(res, dst, src);
			upd9002_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ADDBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _add_ea_r16(void) { // 01: add EA, REG16

	UINT16 *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;
	UINT32 madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			ADDWORD(res, dst, src);
			upd9002_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ADDWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _add_r8_ea(void) { // 02: add REG8, EA

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ADDBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _add_r16_ea(void) { // 03: add REG16, EA

	UINT16 *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ADDWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _add_al_data8(void) { // 04: add al, DATA8

	UINT src;
	UINT res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	ADDBYTE(res, UPD9002_AL, src);
	UPD9002_AL = (BYTE)res;
}

UPD9002FN _add_ax_data16(void) { // 05: add ax, DATA16

	UINT src;
	UINT res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	ADDWORD(res, UPD9002_AX, src);
	UPD9002_AX = (UINT16)res;
}

UPD9002FN _push_es(void) { // 06: push es

	REGPUSH(UPD9002_ES, 3);
}

UPD9002FN _pop_es(void) { // 07: pop es

	UINT tmp;

	REGPOP(tmp, 5)
	UPD9002_ES = tmp;
	ES_BASE = SEGSELECT(tmp);
}

UPD9002FN _or_ea_r8(void) { // 08: or EA, REG8

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT32 madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			ORBYTE(dst, src);
			upd9002_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ORBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _or_ea_r16(void) { // 09: or EA, REG16

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			ORWORD(dst, src);
			upd9002_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ORWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _or_r8_ea(void) { // 0a: or REG8, EA

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ORBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _or_r16_ea(void) { // 0b: or REG16, EA

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ORWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _or_al_data8(void) { // 0c: or al, DATA8

	UINT src;
	UINT dst;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = UPD9002_AL;
	ORBYTE(dst, src);
	UPD9002_AL = (BYTE)dst;
}

UPD9002FN _or_ax_data16(void) { // 0d: or ax, DATA16

	UINT32 src;
	UINT32 dst;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = UPD9002_AX;
	ORWORD(dst, src);
	UPD9002_AX = (UINT16)dst;
}

UPD9002FN _push_cs(void) { // 0e: push cs

	REGPUSH(UPD9002_CS, 3);
}

UPD9002FN _adc_ea_r8(void) { // 10: adc EA, REG8

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;
	UINT32 madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			ADCBYTE(res, dst, src);
			upd9002_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ADCBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _adc_ea_r16(void) { // 11: adc EA, REG16

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 res;
	UINT32 madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			ADCWORD(res, dst, src);
			upd9002_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ADCWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _adc_r8_ea(void) { // 12: adc REG8, EA

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ADCBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _adc_r16_ea(void) { // 13: adc REG16, EA

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ADCWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _adc_al_data8(void) { // 14: adc al, DATA8

	UINT src;
	UINT res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	ADCBYTE(res, UPD9002_AL, src);
	UPD9002_AL = (BYTE)res;
}

UPD9002FN _adc_ax_data16(void) { // 15: adc ax, DATA16

	UINT32 src;
	UINT32 res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	ADCWORD(res, UPD9002_AX, src);
	UPD9002_AX = (UINT16)res;
}

UPD9002FN _push_ss(void) { // 16: push ss

	REGPUSH(UPD9002_SS, 3);
}

UPD9002FN _pop_ss(void) { // 17: pop ss

	UINT tmp;

	REGPOP(tmp, 5)
	UPD9002_SS = tmp;
	SS_BASE = SEGSELECT(tmp);
	SS_FIX = SS_BASE;
	NEXT_OPCODE
}

UPD9002FN _sbb_ea_r8(void) { // 18: sbb EA, REG8

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;
	UINT32 madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			SBBBYTE(res, dst, src);
			upd9002_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	SBBBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _sbb_ea_r16(void) { // 19: sbb EA, REG16

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 res;
	UINT32 madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			SBBWORD(res, dst, src);
			upd9002_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	SBBWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _sbb_r8_ea(void) { // 1a: sbb REG8, EA

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT32 res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	SBBBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _sbb_r16_ea(void) { // 1b: sbb REG16, EA

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	SBBWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _sbb_al_data8(void) { // 1c: adc al, DATA8

	UINT src;
	UINT res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	SBBBYTE(res, UPD9002_AL, src);
	UPD9002_AL = (BYTE)res;
}

UPD9002FN _sbb_ax_data16(void) { // 1d: adc ax, DATA16

	UINT32 src;
	UINT32 res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	SBBWORD(res, UPD9002_AX, src);
	UPD9002_AX = (UINT16)res;
}

UPD9002FN _push_ds(void) { // 1e: push ds

	REGPUSH(UPD9002_DS, 3);
}

UPD9002FN _pop_ds(void) { // 1f: pop ds

	UINT tmp;

	REGPOP(tmp, 5)
	UPD9002_DS = tmp;
	DS_BASE = SEGSELECT(tmp);
	DS_FIX = DS_BASE;
}

UPD9002FN _and_ea_r8(void) { // 20: and EA, REG8

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT32 madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			ANDBYTE(dst, src);
			upd9002_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	ANDBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _and_ea_r16(void) { // 21: and EA, REG16

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			ANDWORD(dst, src);
			upd9002_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	ANDWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _and_r8_ea(void) { // 22: and REG8, EA

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	ANDBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _and_r16_ea(void) { // 23: and REG16, EA

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	ANDWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _and_al_data8(void) { // 24: and al, DATA8

	UINT src;
	UINT dst;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = UPD9002_AL;
	ANDBYTE(dst, src);
	UPD9002_AL = (BYTE)dst;
}

UPD9002FN _and_ax_data16(void) { // 25: and ax, DATA16

	UINT32 src;
	UINT32 dst;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = UPD9002_AX;
	ANDWORD(dst, src);
	UPD9002_AX = (UINT16)dst;
}

UPD9002FN _segprefix_es(void) { // 26: es:

	SS_FIX = ES_BASE;
	DS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _daa(void) { // 27:	DAA

	const UINT8 value = UPD9002_AL;
	const BOOL adjust_low = ((UPD9002_FLAGL & A_FLAG) || ((value & 0x0f) > 9));
	const BOOL adjust_high = ((UPD9002_FLAGL & C_FLAG) || (value > 0x9f) ||
	                          ((value > 0x99) && !(UPD9002_FLAGL & A_FLAG)));
	const UINT8 delta = (UINT8)((adjust_low ? 6 : 0) + (adjust_high ? 0x60 : 0));
	const UINT8 result = (UINT8)(value + delta);

	UPD9002_WORKCLOCK(3);
	UPD9002_AL = result;
	_adjust_flags(result, adjust_low, adjust_high,
	              (UINT)((~(value ^ delta) & (value ^ result)) & 0x80));
}

UPD9002FN _sub_ea_r8(void) { // 28: sub EA, REG8

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;
	UINT32 madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			SUBBYTE(res, dst, src);
			upd9002_memorywrite(madr, (REG8)res);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	SUBBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _sub_ea_r16(void) { // 29: sub EA, REG16

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 res;
	UINT32 madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			SUBWORD(res, dst, src);
			upd9002_memorywrite_w(madr, (REG16)res);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	SUBWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _sub_r8_ea(void) { // 2a: sub REG8, EA

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	SUBBYTE(res, dst, src);
	*out = (BYTE)res;
}

UPD9002FN _sub_r16_ea(void) { // 2b: sub REG16, EA

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 res;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	SUBWORD(res, dst, src);
	*out = (UINT16)res;
}

UPD9002FN _sub_al_data8(void) { // 2c: sub al, DATA8

	UINT src;
	UINT res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	SUBBYTE(res, UPD9002_AL, src);
	UPD9002_AL = (BYTE)res;
}

UPD9002FN _sub_ax_data16(void) { // 2d: sub ax, DATA16

	UINT32 src;
	UINT32 res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	SUBWORD(res, UPD9002_AX, src);
	UPD9002_AX = (UINT16)res;
}

UPD9002FN _segprefix_cs(void) { // 2e: cs:

	SS_FIX = CS_BASE;
	DS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _das(void) { // 2F:	DAS

	const UINT8 value = UPD9002_AL;
	const BOOL adjust_low = ((UPD9002_FLAGL & A_FLAG) || ((value & 0x0f) > 9));
	const BOOL adjust_high = ((UPD9002_FLAGL & C_FLAG) || (value > 0x9f) ||
	                          ((value > 0x99) && !(UPD9002_FLAGL & A_FLAG)));
	const UINT8 delta = (UINT8)((adjust_low ? 6 : 0) + (adjust_high ? 0x60 : 0));
	const UINT8 result = (UINT8)(value - delta);

	UPD9002_WORKCLOCK(3);
	UPD9002_AL = result;
	_adjust_flags(result, adjust_low, adjust_high,
	              (UINT)(((value ^ delta) & (value ^ result)) & 0x80));
}

UPD9002FN _xor_ea_r8(void) { // 30: xor EA, REG8

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT32 madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			dst = upd9002_memoryread(madr);
			XORBYTE(dst, src);
			upd9002_memorywrite(madr, (REG8)dst);
			return;
		}
		out = mem + madr;
	}
	dst = *out;
	XORBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _xor_ea_r16(void) { // 31: xor EA, REG16

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			dst = upd9002_memoryread_w(madr);
			XORWORD(dst, src);
			upd9002_memorywrite_w(madr, (REG16)dst);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	dst = *out;
	XORWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _xor_r8_ea(void) { // 32: xor REG8, EA

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;

	PREPART_REG8_EA(op, src, out, 2, 7);
	dst = *out;
	XORBYTE(dst, src);
	*out = (BYTE)dst;
}

UPD9002FN _xor_r16_ea(void) { // 33: or REG16, EA

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;

	PREPART_REG16_EA(op, src, out, 2, 7);
	dst = *out;
	XORWORD(dst, src);
	*out = (UINT16)dst;
}

UPD9002FN _xor_al_data8(void) { // 34: or al, DATA8

	UINT src;
	UINT dst;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	dst = UPD9002_AL;
	XORBYTE(dst, src);
	UPD9002_AL = (BYTE)dst;
}

UPD9002FN _xor_ax_data16(void) { // 35: or ax, DATA16

	UINT32 src;
	UINT32 dst;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	dst = UPD9002_AX;
	XORWORD(dst, src);
	UPD9002_AX = (UINT16)dst;
}

UPD9002FN _segprefix_ss(void) { // 36: ss:

	SS_FIX = SS_BASE;
	DS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _aaa(void) { // 37:	AAA

	const UINT8 value = UPD9002_AL;
	const BOOL adjust = ((UPD9002_FLAGL & A_FLAG) || ((value & 0x0f) > 9));
	UINT8 result = value;

	UPD9002_WORKCLOCK(3);
	if (adjust) {
		result = (UINT8)(value + 6);
		UPD9002_AH++;
	}
	UPD9002_AL = result & 0x0f;
	_adjust_flags(result, adjust, adjust, (UINT)((~(value ^ 6) & (value ^ result)) & 0x80));
}

UPD9002FN _cmp_ea_r8(void) { // 38: cmp EA, REG8

	UINT op;
	UINT src;
	UINT dst;
	UINT res;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		dst = *(REG8_B20(op));
		SUBBYTE(res, dst, src);
	} else {
		UPD9002_WORKCLOCK(7);
		dst = upd9002_memoryread(CALC_EA(op));
		SUBBYTE(res, dst, src);
	}
}

UPD9002FN _cmp_ea_r16(void) { // 39: cmp EA, REG16

	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 res;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		dst = *(REG16_B20(op));
		SUBWORD(res, dst, src);
	} else {
		UPD9002_WORKCLOCK(7);
		dst = upd9002_memoryread_w(CALC_EA(op));
		SUBWORD(res, dst, src);
	}
}

UPD9002FN _cmp_r8_ea(void) { // 3a: cmp REG8, EA

	BYTE *out;
	UINT op;
	UINT src;
	UINT dst;
	UINT res;

	PREPART_REG8_EA(op, src, out, 2, 6);
	dst = *out;
	SUBBYTE(res, dst, src);
}

UPD9002FN _cmp_r16_ea(void) { // 3b: cmp REG16, EA

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 dst;
	UINT32 res;

	PREPART_REG16_EA(op, src, out, 2, 6);
	dst = *out;
	SUBWORD(res, dst, src);
}

UPD9002FN _cmp_al_data8(void) { // 3c: cmp al, DATA8

	UINT src;
	UINT res;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src);
	SUBBYTE(res, UPD9002_AL, src);
}

UPD9002FN _cmp_ax_data16(void) { // 3d: cmp ax, DATA16

	UINT32 src;
	UINT32 res;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src);
	SUBWORD(res, UPD9002_AX, src);
}

UPD9002FN _segprefix_ds(void) { // 3e: ds:

	SS_FIX = DS_BASE;
	DS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _aas(void) { // 3F:	AAS

	const UINT8 value = UPD9002_AL;
	const BOOL adjust = ((UPD9002_FLAGL & A_FLAG) || ((value & 0x0f) > 9));
	UINT8 result = value;

	UPD9002_WORKCLOCK(3);
	if (adjust) {
		result = (UINT8)(value - 6);
		UPD9002_AH--;
	}
	UPD9002_AL = result & 0x0f;
	_adjust_flags(result, adjust, adjust, (UINT)(((value ^ 6) & (value ^ result)) & 0x80));
}

UPD9002FN _inc_ax(void) INCWORD2(UPD9002_AX, 2)     // 40:	inc		ax
    UPD9002FN _inc_cx(void) INCWORD2(UPD9002_CX, 2) // 41:	inc		cx
    UPD9002FN _inc_dx(void) INCWORD2(UPD9002_DX, 2) // 42:	inc		dx
    UPD9002FN _inc_bx(void) INCWORD2(UPD9002_BX, 2) // 43:	inc		bx
    UPD9002FN _inc_sp(void) INCWORD2(UPD9002_SP, 2) // 44:	inc		sp
    UPD9002FN _inc_bp(void) INCWORD2(UPD9002_BP, 2) // 45:	inc		bp
    UPD9002FN _inc_si(void) INCWORD2(UPD9002_SI, 2) // 46:	inc		si
    UPD9002FN _inc_di(void) INCWORD2(UPD9002_DI, 2) // 47:	inc		di
    UPD9002FN _dec_ax(void) DECWORD2(UPD9002_AX, 2) // 48:	dec		ax
    UPD9002FN _dec_cx(void) DECWORD2(UPD9002_CX, 2) // 49:	dec		cx
    UPD9002FN _dec_dx(void) DECWORD2(UPD9002_DX, 2) // 4a:	dec		dx
    UPD9002FN _dec_bx(void) DECWORD2(UPD9002_BX, 2) // 4b:	dec		bx
    UPD9002FN _dec_sp(void) DECWORD2(UPD9002_SP, 2) // 4c:	dec		sp
    UPD9002FN _dec_bp(void) DECWORD2(UPD9002_BP, 2) // 4d:	dec		bp
    UPD9002FN _dec_si(void) DECWORD2(UPD9002_SI, 2) // 4e:	dec		si
    UPD9002FN _dec_di(void) DECWORD2(UPD9002_DI, 2) // 4f:	dec		di

    UPD9002FN _push_ax(void) REGPUSH(UPD9002_AX, 3) // 50:	push	ax
    UPD9002FN _push_cx(void) REGPUSH(UPD9002_CX, 3) // 51:	push	cx
    UPD9002FN _push_dx(void) REGPUSH(UPD9002_DX, 3) // 52:	push	dx
    UPD9002FN _push_bx(void) REGPUSH(UPD9002_BX, 3) // 53:	push	bx
    UPD9002FN _push_sp(void) REGPUSH(UPD9002_SP, 3) // 54: push sp
    UPD9002FN _push_bp(void) REGPUSH(UPD9002_BP, 3) // 55:	push	bp
    UPD9002FN _push_si(void) REGPUSH(UPD9002_SI, 3) // 56:	push	si
    UPD9002FN _push_di(void) REGPUSH(UPD9002_DI, 3) // 57:	push	di
    UPD9002FN _pop_ax(void) REGPOP(UPD9002_AX, 5)   // 58:	pop		ax
    UPD9002FN _pop_cx(void) REGPOP(UPD9002_CX, 5)   // 59:	pop		cx
    UPD9002FN _pop_dx(void) REGPOP(UPD9002_DX, 5)   // 5A:	pop		dx
    UPD9002FN _pop_bx(void) REGPOP(UPD9002_BX, 5)   // 5B:	pop		bx
    UPD9002FN _pop_sp(void) SP_POP(UPD9002_SP, 5)   // 5C:	pop		sp
    UPD9002FN _pop_bp(void) REGPOP(UPD9002_BP, 5)   // 5D:	pop		bp
    UPD9002FN _pop_si(void) REGPOP(UPD9002_SI, 5)   // 5E:	pop		si
    UPD9002FN _pop_di(void) REGPOP(UPD9002_DI, 5)   // 5F:	pop		di

#if (defined(ARM) || defined(X11)) && defined(BYTESEX_LITTLE)

    UPD9002FN _pusha(void) { // 60:	pusha

	REG16 tmp;
	UINT32 addr;

	UPD9002_WORKCLOCK(17);
	tmp = UPD9002_SP;
	addr = tmp + SS_BASE;
	if ((tmp < 16) || (INHIBIT_WORDP(addr))) {
		REGPUSH0(UPD9002_AX)
		REGPUSH0(UPD9002_CX)
		REGPUSH0(UPD9002_DX)
		REGPUSH0(UPD9002_BX)
		REGPUSH0(tmp)
		REGPUSH0(UPD9002_BP)
		REGPUSH0(UPD9002_SI)
		REGPUSH0(UPD9002_DI)
	} else {
		*(UINT16 *)(mem + addr - 2) = UPD9002_AX;
		*(UINT16 *)(mem + addr - 4) = UPD9002_CX;
		*(UINT16 *)(mem + addr - 6) = UPD9002_DX;
		*(UINT16 *)(mem + addr - 8) = UPD9002_BX;
		*(UINT16 *)(mem + addr - 10) = tmp;
		*(UINT16 *)(mem + addr - 12) = UPD9002_BP;
		*(UINT16 *)(mem + addr - 14) = UPD9002_SI;
		*(UINT16 *)(mem + addr - 16) = UPD9002_DI;
		UPD9002_SP -= 16;
	}
}

UPD9002FN _popa(void) { // 61:	popa

	UINT tmp;
	UINT32 addr;

	UPD9002_WORKCLOCK(19);
	tmp = UPD9002_SP + 16;
	addr = tmp + SS_BASE;
	if ((tmp >= 0x10000) || (INHIBIT_WORDP(addr))) {
		UPD9002_DI = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_SI = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_BP = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_SP += 2;
		UPD9002_BX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_DX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_CX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
		UPD9002_AX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
		UPD9002_SP += 2;
	} else {
		UPD9002_DI = *(UINT16 *)(mem + addr - 16);
		UPD9002_SI = *(UINT16 *)(mem + addr - 14);
		UPD9002_BP = *(UINT16 *)(mem + addr - 12);
		UPD9002_BX = *(UINT16 *)(mem + addr - 8);
		UPD9002_DX = *(UINT16 *)(mem + addr - 6);
		UPD9002_CX = *(UINT16 *)(mem + addr - 4);
		UPD9002_AX = *(UINT16 *)(mem + addr - 2);
		UPD9002_SP = tmp;
	}
}

#else

    UPD9002FN _pusha(void) { // 60:	pusha

	REG16 tmp;

	tmp = UPD9002_SP;
	REGPUSH0(UPD9002_AX)
	REGPUSH0(UPD9002_CX)
	REGPUSH0(UPD9002_DX)
	REGPUSH0(UPD9002_BX)
	REGPUSH0(tmp)
	REGPUSH0(UPD9002_BP)
	REGPUSH0(UPD9002_SI)
	REGPUSH0(UPD9002_DI)
	UPD9002_WORKCLOCK(17);
}

UPD9002FN _popa(void) { // 61:	popa

	UPD9002_DI = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_SI = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_BP = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_SP += 2;
	UPD9002_BX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_DX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_CX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_AX = upd9002_memoryread_seg_w(SS_BASE, UPD9002_SP);
	UPD9002_SP += 2;
	UPD9002_WORKCLOCK(19);
}

#endif

UPD9002FN _bound(void) { // 62:	bound

	UINT vect = 0;
	UINT op;
	UINT32 madr;
	SINT16 reg;
	SINT16 lower;
	SINT16 upper;

	UPD9002_WORKCLOCK(13); // ToDo
	GET_PCBYTE(op);
	if (op < 0xc0) {
		reg = (SINT16) * (REG16_B53(op));
		madr = CALC_EA(op);
		lower = (SINT16)upd9002_memoryread_w(madr);
		madr += 2; // ToDo
		upper = (SINT16)upd9002_memoryread_w(madr);
		if ((reg >= lower) && (reg <= upper)) {
			return;
		}
		vect = 5;
	} else {
		vect = 6;
	}
	INT_NUM(vect, UPD9002_IP);
}

UPD9002FN _push_data16(void) { // 68:	push	DATA16

	UINT16 tmp;

	GET_PCWORD(tmp)
	REGPUSH(tmp, 3)
}

UPD9002FN _imul_reg_ea_data16(void) { // 69:	imul	REG, EA, DATA16

	UINT16 *out;
	UINT op;
	SINT16 src;
	SINT16 dst;
	SINT32 res;

	PREPART_REG16_EA(op, src, out, 21, 24)
	GET_PCWORD(dst)
	WORD_IMUL(res, dst, src)
	*out = (UINT16)res;
}

UPD9002FN _push_data8(void) { // 6A:	push	DATA8

	UINT16 tmp;

	GET_PCBYTES(tmp)
	REGPUSH(tmp, 3)
}

UPD9002FN _imul_reg_ea_data8(void) { // 6B:	imul	REG, EA, DATA8

	UINT16 *out;
	UINT op;
	SINT16 src;
	SINT16 dst;
	SINT32 res;

	PREPART_REG16_EA(op, src, out, 21, 24)
	GET_PCBYTES(dst)
	WORD_IMUL(res, dst, src)
	*out = (UINT16)res;
}

UPD9002FN _insb(void) { // 6C:	insb

	REG8 dat;

	UPD9002_WORKCLOCK(5);
	dat = iocore_inp8(UPD9002_DX);
	upd9002_memorywrite(UPD9002_DI + ES_BASE, dat);
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _insw(void) { // 6D:	insw

	REG16 dat;

	UPD9002_WORKCLOCK(5);
	dat = iocore_inp16(UPD9002_DX);
	upd9002_memorywrite_w(UPD9002_DI + ES_BASE, dat);
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _outsb(void) { // 6E:	outsb

	REG8 dat;

	UPD9002_WORKCLOCK(3);
	dat = upd9002_memoryread(UPD9002_SI + DS_FIX);
	UPD9002_SI += STRING_DIR;
	iocore_out8(UPD9002_DX, (BYTE)dat);
}

UPD9002FN _outsw(void) { // 6F:	outsw

	REG16 dat;

	UPD9002_WORKCLOCK(3);
	dat = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
	UPD9002_SI += STRING_DIRx2;
	iocore_out16(UPD9002_DX, (UINT16)dat);
}

UPD9002FN _jo_short(void) { // 70:	jo short

	if (!UPD9002_OV)
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jno_short(void) { // 71:	jno short

	if (UPD9002_OV)
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jc_short(void) { // 72:	jnae/jb/jc short

	if (!(UPD9002_FLAGL & C_FLAG))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnc_short(void) { // 73:	jae/jnb/jnc short

	if (UPD9002_FLAGL & C_FLAG)
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jz_short(void) { // 74:	je/jz short

	if (!(UPD9002_FLAGL & Z_FLAG))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnz_short(void) { // 75:	jne/jnz short

	if (UPD9002_FLAGL & Z_FLAG)
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jna_short(void) { // 76:	jna/jbe short

	if (!(UPD9002_FLAGL & (Z_FLAG | C_FLAG)))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _ja_short(void) { // 77:	ja/jnbe short
	if (UPD9002_FLAGL & (Z_FLAG | C_FLAG))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _js_short(void) { // 78:	js short

	if (!(UPD9002_FLAGL & S_FLAG))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jns_short(void) { // 79:	jns short

	if (UPD9002_FLAGL & S_FLAG)
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jp_short(void) { // 7A:	jp/jpe short

	if (!(UPD9002_FLAGL & P_FLAG))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnp_short(void) { // 7B:	jnp/jpo short

	if (UPD9002_FLAGL & P_FLAG)
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jl_short(void) { // 7C:	jl/jnge short

	if (((UPD9002_FLAGL & S_FLAG) == 0) == (UPD9002_OV == 0))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnl_short(void) { // 7D:	jnl/jge short

	if (((UPD9002_FLAGL & S_FLAG) == 0) != (UPD9002_OV == 0))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jle_short(void) { // 7E:	jle/jng short

	if ((!(UPD9002_FLAGL & Z_FLAG)) && (((UPD9002_FLAGL & S_FLAG) == 0) == (UPD9002_OV == 0)))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _jnle_short(void) { // 7F:	jg/jnle short

	if ((UPD9002_FLAGL & Z_FLAG) || (((UPD9002_FLAGL & S_FLAG) == 0) != (UPD9002_OV == 0)))
		JMPNOP(2) else JMPSHORT(7)
}

UPD9002FN _calc_ea8_i8(void) { // 80:	op		EA8, DATA8
	                           // 82:	op		EA8, DATA8
	BYTE *out;
	UINT op;
	UINT32 madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			c_op8xext8_table[(op >> 3) & 7](madr);
			return;
		}
		out = mem + madr;
	}
	c_op8xreg8_table[(op >> 3) & 7](out);
}

UPD9002FN _calc_ea16_i16(void) { // 81:	op		EA16, DATA16

	UINT16 *out;
	UINT op;
	UINT32 madr;
	UINT32 src;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		{
			UINT32 seg;
			UINT off;

			off = GET_EA(op, &seg);
			madr = seg + off;
			if ((LOW16(off) == 0xffff) || INHIBIT_WORDP(madr)) {
				UINT16 tmp;

				GET_PCWORD(src);
				tmp = upd9002_memoryread_seg_w(seg, off);
				c_op8xreg16_table[(op >> 3) & 7](&tmp, src);
				if (((op >> 3) & 7) != 7) {
					upd9002_memorywrite_seg_w(seg, off, tmp);
				}
				return;
			}
		}
		if (INHIBIT_WORDP(madr)) {
			GET_PCWORD(src);
			c_op8xext16_table[(op >> 3) & 7](madr, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCWORD(src);
	c_op8xreg16_table[(op >> 3) & 7](out, src);
}

UPD9002FN _calc_ea16_i8(void) { // 83:	op		EA16, DATA8

	UINT16 *out;
	UINT op;
	UINT32 madr;
	UINT32 src;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		{
			UINT32 seg;
			UINT off;

			off = GET_EA(op, &seg);
			madr = seg + off;
			if ((LOW16(off) == 0xffff) || INHIBIT_WORDP(madr)) {
				UINT16 tmp;

				GET_PCBYTES(src);
				tmp = upd9002_memoryread_seg_w(seg, off);
				c_op8xreg16_table[(op >> 3) & 7](&tmp, src);
				if (((op >> 3) & 7) != 7) {
					upd9002_memorywrite_seg_w(seg, off, tmp);
				}
				return;
			}
		}
		if (INHIBIT_WORDP(madr)) {
			GET_PCBYTES(src);
			c_op8xext16_table[(op >> 3) & 7](madr, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCBYTES(src);
	c_op8xreg16_table[(op >> 3) & 7](out, src);
}

UPD9002FN _test_ea_r8(void) { // 84:	test	EA, REG8

	BYTE *out;
	UINT op;
	UINT src;
	UINT tmp;
	UINT32 madr;

	PREPART_EA_REG8(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(6);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			tmp = upd9002_memoryread(madr);
			ANDBYTE(tmp, src);
			return;
		}
		out = mem + madr;
	}
	tmp = *out;
	ANDBYTE(tmp, src);
}

UPD9002FN _test_ea_r16(void) { // 85:	test	EA, REG16

	UINT16 *out;
	UINT op;
	UINT32 src;
	UINT32 tmp;
	UINT32 madr;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			tmp = upd9002_memoryread_w(madr);
			ANDWORD(tmp, src);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	tmp = *out;
	ANDWORD(tmp, src);
}

UPD9002FN _xchg_ea_r8(void) { // 86:	xchg	EA, REG8

	BYTE *out;
	BYTE *src;
	UINT op;
	UINT32 madr;

	PREPART_EA_REG8P(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(5);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			BYTE tmp = upd9002_memoryread(madr);
			upd9002_memorywrite(madr, *src);
			*src = tmp;
			return;
		}
		out = mem + madr;
	}
	SWAPBYTE(*out, *src);
}

UPD9002FN _xchg_ea_r16(void) { // 87:	xchg	EA, REG16

	UINT16 *out;
	UINT16 *src;
	UINT op;
	UINT32 madr;

	PREPART_EA_REG16P(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(3);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(5);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			UINT16 tmp = upd9002_memoryread_w(madr);
			upd9002_memorywrite_w(madr, *src);
			*src = tmp;
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	SWAPWORD(*out, *src);
}

UPD9002FN _mov_ea_r8(void) { // 88:	mov		EA, REG8

	BYTE src;
	UINT op;
	UINT32 madr;

	PREPART_EA_REG8(op, src)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		*(REG8_B20(op)) = src;
	} else {
		UPD9002_WORKCLOCK(3);
		madr = CALC_EA(op);
		upd9002_memorywrite(madr, src);
	}
}

UPD9002FN _mov_ea_r16(void) { // 89:	mov		EA, REG16

	UINT16 src;
	UINT op;

	PREPART_EA_REG16(op, src);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		*(REG16_B20(op)) = src;
	} else {
		UPD9002_WORKCLOCK(3);
		upd9002_memorywrite_w(CALC_EA(op), src);
	}
}

UPD9002FN _mov_r8_ea(void) { // 8A:	mov		REG8, EA

	BYTE *out;
	BYTE src;
	UINT op;

	PREPART_REG8_EA(op, src, out, 2, 5);
	*out = src;
}

UPD9002FN _mov_r16_ea(void) { // 8B:	mov		REG16, EA

	UINT16 *out;
	UINT16 src;
	UINT op;

	PREPART_REG16_EA(op, src, out, 2, 5);
	*out = src;
}

UPD9002FN _mov_ea_seg(void) { // 8C:	mov		EA, segreg

	UINT op;
	UINT16 tmp;

	GET_PCBYTE(op);
	tmp = *SEGMENTPTR((op >> 3) & 3);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		*(REG16_B20(op)) = tmp;
	} else {
		UPD9002_WORKCLOCK(3);
		upd9002_memorywrite_w(CALC_EA(op), tmp);
	}
}

UPD9002FN _lea_r16_ea(void) { // 8D:	lea		REG16, EA

	UINT op;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		*(REG16_B53(op)) = CALC_LEA(op);
	} else {
		INT_NUM(6, UPD9002_SP - 2);
	}
}

UPD9002FN _pop_ea(void) { // 8F:	pop		EA

	UINT op;
	UINT16 tmp;

	UPD9002_WORKCLOCK(5);
	REGPOP0(tmp)

	GET_PCBYTE(op)
	if (op < 0xc0) {
		upd9002_memorywrite_w(CALC_EA(op), tmp);
	} else {
		*(REG16_B20(op)) = tmp;
	}
}

UPD9002FN _nop(void) { // 90: nop
	UPD9002_WORKCLOCK(3);
}

UPD9002FN _xchg_ax_cx(void) { // 91:	xchg	ax, cx

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_CX);
}

UPD9002FN _xchg_ax_dx(void) { // 92:	xchg	ax, dx

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_DX);
}

UPD9002FN _xchg_ax_bx(void) { // 93:	xchg	ax, bx

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_BX);
}

UPD9002FN _xchg_ax_sp(void) { // 94:	xchg	ax, sp

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_SP);
}

UPD9002FN _xchg_ax_bp(void) { // 95:	xchg	ax, bp

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_BP);
}

UPD9002FN _xchg_ax_si(void) { // 96:	xchg	ax, si

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_SI);
}

UPD9002FN _xchg_ax_di(void) { // 97:	xchg	ax, di

	UPD9002_WORKCLOCK(3);
	SWAPWORD(UPD9002_AX, UPD9002_DI);
}

UPD9002FN _cbw(void) { // 98:	cbw

	UPD9002_WORKCLOCK(2);
	UPD9002_AX = __CBW(UPD9002_AL);
}

UPD9002FN _cwd(void) { // 99:	cwd

	UPD9002_WORKCLOCK(2);
	UPD9002_DX = ((UPD9002_AH & 0x80) ? 0xffff : 0x0000);
}

UPD9002FN _call_far(void) { // 9A:	call far

	UINT16 newip;

	UPD9002_WORKCLOCK(13);
	REGPUSH0(UPD9002_CS)
	GET_PCWORD(newip)
	GET_PCWORD(UPD9002_CS)
	CS_BASE = SEGSELECT(UPD9002_CS);
	REGPUSH0(UPD9002_IP)
	UPD9002_IP = newip;
}

UPD9002FN _wait(void) { // 9B:	wait

	UPD9002_WORKCLOCK(2);
	if (upd8087_wait_blocked(&upd8087)) {
		/* Let machine events and PIC arbitration run before the native
		 * decoder fetches another instruction.  The 8087 request is sticky
		 * and is cleared only by guest recovery such as FCLEX. */
		UPD9002_REMCLOCK = 0;
	}
}

UPD9002FN _pushf(void) { // 9C:	pushf

	UPD9002_WORKCLOCK(3);
	UPD9002_SP -= 2;
	upd9002_memorywrite_seg_w(SS_BASE, UPD9002_SP, _materialize_pushf_image());
}

UPD9002FN _popf(void) { // 9D:	popf

	UINT flag;

	UPD9002_WORKCLOCK(5);
	REGPOP0(flag)
	flag = (flag & 0x0ed5) | 0xf002;
	UPD9002_OV = flag & O_FLAG;
	UPD9002_FLAG = flag & (UINT16)~O_FLAG;
	UPD9002_TRAP = ((flag & 0x300) == 0x300);
	UPD9002_IRQCHECKTERM
}

UPD9002FN _sahf(void) { // 9E:	sahf

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAGL = (UINT8)((UPD9002_AH & 0xd5) | 0x02);
}

UPD9002FN _lahf(void) { // 9F:	lahf

	UPD9002_WORKCLOCK(2);
	UPD9002_AH = UPD9002_FLAGL;
}

UPD9002FN _mov_al_m8(void) { // A0:	mov		al, m8

	UINT op;

	UPD9002_WORKCLOCK(5);
	GET_PCWORD(op)
	UPD9002_AL = upd9002_memoryread(DS_FIX + op);
}

UPD9002FN _mov_ax_m16(void) { // A1:	mov		ax, m16

	UINT op;

	UPD9002_WORKCLOCK(5);
	GET_PCWORD(op)
	UPD9002_AX = upd9002_memoryread_w(DS_FIX + op);
}

UPD9002FN _mov_m8_al(void) { // A2:	mov		m8, al

	UINT op;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(op)
	upd9002_memorywrite(DS_FIX + op, UPD9002_AL);
}

UPD9002FN _mov_m16_ax(void) { // A3:	mov		m16, ax

	UINT op;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(op);
	upd9002_memorywrite_w(DS_FIX + op, UPD9002_AX);
}

UPD9002FN _movsb(void) { // A4:	movsb

	BYTE tmp;

	UPD9002_WORKCLOCK(5);
	tmp = upd9002_memoryread(UPD9002_SI + DS_FIX);
	upd9002_memorywrite(UPD9002_DI + ES_BASE, tmp);
	UPD9002_SI += STRING_DIR;
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _movsw(void) { // A5:	movsw

	UINT16 tmp;

	UPD9002_WORKCLOCK(5);
	tmp = upd9002_memoryread_seg_w(DS_FIX, UPD9002_SI);
	upd9002_memorywrite_seg_w(ES_BASE, UPD9002_DI, tmp);
	UPD9002_SI += STRING_DIRx2;
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _cmpsb(void) { // A6:	cmpsb

	UINT src;
	UINT dst;
	UINT res;

	UPD9002_WORKCLOCK(8);
	dst = upd9002_memoryread(UPD9002_SI + DS_FIX);
	src = upd9002_memoryread(UPD9002_DI + ES_BASE);
	SUBBYTE(res, dst, src)
	UPD9002_SI += STRING_DIR;
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _cmpsw(void) { // A7:	cmpsw

	UINT32 src;
	UINT32 dst;
	UINT32 res;

	UPD9002_WORKCLOCK(8);
	dst = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
	src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
	SUBWORD(res, dst, src)
	UPD9002_SI += STRING_DIRx2;
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _test_al_data8(void) { // A8:	test	al, DATA8

	UINT src;
	UINT dst;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(src)
	dst = UPD9002_AL;
	ANDBYTE(dst, src)
}

UPD9002FN _test_ax_data16(void) { // A9:	test	ax, DATA16

	UINT32 src;
	UINT32 dst;

	UPD9002_WORKCLOCK(3);
	GET_PCWORD(src)
	dst = UPD9002_AX;
	ANDWORD(dst, src)
}

UPD9002FN _stosb(void) { // AA:	stosw

	UPD9002_WORKCLOCK(3);
	upd9002_memorywrite(UPD9002_DI + ES_BASE, UPD9002_AL);
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _stosw(void) { // AB:	stosw

	UPD9002_WORKCLOCK(3);
	upd9002_memorywrite_w(UPD9002_DI + ES_BASE, UPD9002_AX);
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _lodsb(void) { // AC:	lodsb

	UPD9002_WORKCLOCK(5);
	UPD9002_AL = upd9002_memoryread(UPD9002_SI + DS_FIX);
	UPD9002_SI += STRING_DIR;
}

UPD9002FN _lodsw(void) { // AD:	lodsw

	UPD9002_WORKCLOCK(5);
	UPD9002_AX = upd9002_memoryread_w(UPD9002_SI + DS_FIX);
	UPD9002_SI += STRING_DIRx2;
}

UPD9002FN _scasb(void) { // AE:	scasb

	UINT src;
	UINT dst;
	UINT res;

	UPD9002_WORKCLOCK(7);
	src = upd9002_memoryread(UPD9002_DI + ES_BASE);
	dst = UPD9002_AL;
	SUBBYTE(res, dst, src)
	UPD9002_DI += STRING_DIR;
}

UPD9002FN _scasw(void) { // AF:	scasw

	UINT32 src;
	UINT32 dst;
	UINT32 res;

	UPD9002_WORKCLOCK(7);
	src = upd9002_memoryread_w(UPD9002_DI + ES_BASE);
	dst = UPD9002_AX;
	SUBWORD(res, dst, src)
	UPD9002_DI += STRING_DIRx2;
}

UPD9002FN _mov_al_imm(void) MOVIMM8(UPD9002_AL)      // B0:	mov		al, imm8
    UPD9002FN _mov_cl_imm(void) MOVIMM8(UPD9002_CL)  // B1:	mov		cl, imm8
    UPD9002FN _mov_dl_imm(void) MOVIMM8(UPD9002_DL)  // B2:	mov		dl, imm8
    UPD9002FN _mov_bl_imm(void) MOVIMM8(UPD9002_BL)  // B3:	mov		bl, imm8
    UPD9002FN _mov_ah_imm(void) MOVIMM8(UPD9002_AH)  // B4:	mov		ah, imm8
    UPD9002FN _mov_ch_imm(void) MOVIMM8(UPD9002_CH)  // B5:	mov		ch, imm8
    UPD9002FN _mov_dh_imm(void) MOVIMM8(UPD9002_DH)  // B6:	mov		dh, imm8
    UPD9002FN _mov_bh_imm(void) MOVIMM8(UPD9002_BH)  // B7:	mov		bh, imm8
    UPD9002FN _mov_ax_imm(void) MOVIMM16(UPD9002_AX) // B8:	mov		ax, imm16
    UPD9002FN _mov_cx_imm(void) MOVIMM16(UPD9002_CX) // B9:	mov		cx, imm16
    UPD9002FN _mov_dx_imm(void) MOVIMM16(UPD9002_DX) // BA:	mov		dx, imm16
    UPD9002FN _mov_bx_imm(void) MOVIMM16(UPD9002_BX) // BB:	mov		bx, imm16
    UPD9002FN _mov_sp_imm(void) MOVIMM16(UPD9002_SP) // BC:	mov		sp, imm16
    UPD9002FN _mov_bp_imm(void) MOVIMM16(UPD9002_BP) // BD:	mov		bp, imm16
    UPD9002FN _mov_si_imm(void) MOVIMM16(UPD9002_SI) // BE:	mov		si, imm16
    UPD9002FN _mov_di_imm(void) MOVIMM16(UPD9002_DI) // BF:	mov		di, imm16

    UPD9002FN _shift_ea8_data8(void) { // C0:	shift	EA8, DATA8

	UINT8 *out;
	UINT op;
	UINT32 madr = 0;
	REG8 cl;

	GET_PCBYTE(op)
	if (op & 0x20) {
		UINT8 value;

		UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 8);
		value = _ea8_read(op, &madr);
		GET_PCBYTE(cl)
		UPD9002_WORKCLOCK(cl);
		if (cl) {
			_ea8_write(op, madr, _shift8(value, cl, (op >> 3) & 7));
		}
		return;
	}
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			GET_PCBYTE(cl)
			if ((op & 0x30) == 0x10) { // rotate with carry
				cl = shiftbase09[cl];
			} else {
				cl = shiftbase16[cl];
			}
			UPD9002_WORKCLOCK(cl);
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	GET_PCBYTE(cl)
	if ((op & 0x30) == 0x10) { // rotate with carry
		cl = shiftbase09[cl];
	} else {
		cl = shiftbase16[cl];
	}
	UPD9002_WORKCLOCK(cl);
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN _shift_ea16_data8(void) { // C1:	shift	EA16, DATA8

	UINT16 *out;
	UINT op;
	UINT32 madr = 0;
	REG8 cl;

	GET_PCBYTE(op)
	if (op & 0x20) {
		UINT16 value;

		UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 8);
		value = _ea16_read(op, &madr);
		GET_PCBYTE(cl);
		UPD9002_WORKCLOCK(cl);
		if (cl) {
			_ea16_write(op, madr, _shift16(value, cl, (op >> 3) & 7));
		}
		return;
	}
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			GET_PCBYTE(cl);
			if ((op & 0x30) == 0x10) { // rotate with carry
				cl = shiftbase17[cl];
			} else {
				cl = shiftbase16[cl];
			}
			UPD9002_WORKCLOCK(cl);
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	GET_PCBYTE(cl);
	if ((op & 0x30) == 0x10) { // rotate with carry
		cl = shiftbase17[cl];
	} else {
		cl = shiftbase16[cl];
	}
	UPD9002_WORKCLOCK(cl);
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN _ret_near_data16(void) { // C2:	ret near DATA16

	UINT16 ad;

	UPD9002_WORKCLOCK(11);
	GET_PCWORD(ad)
	REGPOP0(UPD9002_IP)
	UPD9002_SP += ad;
}

UPD9002FN _ret_near(void) { // C3:	ret near

	UPD9002_WORKCLOCK(11);
	REGPOP0(UPD9002_IP)
}

UPD9002FN _les_r16_ea(void) { // C4:	les		REG16, EA

	UINT op;
	UINT32 seg;
	UINT ad;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		ad = GET_EA(op, &seg);
		*(REG16_B53(op)) = upd9002_memoryread_seg_w(seg, ad);
		UPD9002_ES = upd9002_memoryread_seg_w(seg, LOW16(ad + 2));
		ES_BASE = SEGSELECT(UPD9002_ES);
	} else {
		INT_NUM(6, UPD9002_IP - 2);
	}
}

UPD9002FN _lds_r16_ea(void) { // C5:	lds		REG16, EA

	UINT op;
	UINT32 seg;
	UINT ad;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(op)
	if (op < 0xc0) {
		ad = GET_EA(op, &seg);
		*(REG16_B53(op)) = upd9002_memoryread_seg_w(seg, ad);
		UPD9002_DS = upd9002_memoryread_seg_w(seg, LOW16(ad + 2));
		DS_BASE = SEGSELECT(UPD9002_DS);
		DS_FIX = DS_BASE;
	} else {
		INT_NUM(6, UPD9002_IP - 2);
	}
}

UPD9002FN _mov_ea8_data8(void) { // C6:	mov		EA8, DATA8

	UINT op;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		GET_PCBYTE(*(REG8_B20(op)))
	} else { // 03/11/23
		UINT32 ad;
		BYTE val;
		UPD9002_WORKCLOCK(3);
		ad = CALC_EA(op);
		GET_PCBYTE(val)
		upd9002_memorywrite(ad, val);
	}
}

UPD9002FN _mov_ea16_data16(void) { // C7:	mov		EA16, DATA16

	UINT op;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		GET_PCWORD(*(REG16_B20(op)))
	} else { // 03/11/23
		UINT32 ad;
		UINT16 val;
		UPD9002_WORKCLOCK(3);
		ad = CALC_EA(op);
		GET_PCWORD(val)
		upd9002_memorywrite_w(ad, val);
	}
}

UPD9002FN _enter(void) { // C8:	enter	DATA16, DATA8

	UINT16 dimsize;
	BYTE level;

	GET_PCWORD(dimsize)
	GET_PCBYTE(level)
	REGPUSH0(UPD9002_BP)
	level &= 0x1f;
	if (!level) { // enter level=0
		UPD9002_WORKCLOCK(11);
		UPD9002_BP = UPD9002_SP;
		UPD9002_SP -= dimsize;
	} else {
		level--;
		if (!level) { // enter level=1
			UINT16 tmp;
			UPD9002_WORKCLOCK(15);
			tmp = UPD9002_SP;
			REGPUSH0(tmp)
			UPD9002_BP = tmp;
			UPD9002_SP -= dimsize;
		} else { // enter level=2-31
			UINT16 bp;
			UPD9002_WORKCLOCK(12 + level * 4);
			bp = UPD9002_BP;
			UPD9002_BP = UPD9002_SP;
			while (level--) {
#if 1 // Retain the established compatibility sequence.
				REG16 val;
				bp -= 2;
				UPD9002_SP -= 2;
				val = upd9002_memoryread_seg_w(SS_BASE, bp);
				upd9002_memorywrite_seg_w(SS_BASE, UPD9002_SP, val);
#else
				UINT16 val = upd9002_memoryread_seg_w(SS_BASE, bp);
				upd9002_memorywrite_seg_w(SS_BASE, UPD9002_SP, val);
				bp -= 2;
				UPD9002_SP -= 2;
#endif
			}
			REGPUSH0(UPD9002_BP)
			UPD9002_SP -= dimsize;
		}
	}
}

UPD9002FN fleave(void) { // C9:	leave

	UPD9002_WORKCLOCK(5);
	UPD9002_SP = UPD9002_BP;
	REGPOP0(UPD9002_BP)
}

UPD9002FN _ret_far_data16(void) { // CA:	ret far	DATA16

	UINT16 ad;

	UPD9002_WORKCLOCK(15);
	GET_PCWORD(ad)
	REGPOP0(UPD9002_IP)
	REGPOP0(UPD9002_CS)
	UPD9002_SP += ad;
	CS_BASE = SEGSELECT(UPD9002_CS);
}

UPD9002FN _ret_far(void) { // CB:	ret far

	UPD9002_WORKCLOCK(15);
	REGPOP0(UPD9002_IP)
	REGPOP0(UPD9002_CS)
	CS_BASE = SEGSELECT(UPD9002_CS);
}

UPD9002FN _int_03(void) { // CC:	int		3

	UPD9002_WORKCLOCK(3);
	INT_NUM(3, UPD9002_IP);
}

UPD9002FN _int_data8(void) { // CD:	int		DATA8

	UINT vect;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(vect)
	INT_NUM(vect, UPD9002_IP);
}

UPD9002FN _into(void) { // CE:	into

	UPD9002_WORKCLOCK(4);
	if (UPD9002_OV) {
		INT_NUM(4, UPD9002_IP);
	}
}

UPD9002FN _iret(void) { // CF: iret

	UINT flag;
	BOOL return_compat;

	return_compat = upd9002_core_compat_iret_is_return();
	REGPOP0(UPD9002_IP)
	REGPOP0(UPD9002_CS)
	REGPOP0(flag)
	if (return_compat) {
		CPU_COMPAT_RETURN_PENDING = 0;
	}
	CS_BASE = UPD9002_CS << 4;
	if (return_compat) {
		/* The frame holds the compatible F in its low byte. A real PC-88VA2
		 * returns F unchanged from a CALLN 91h round trip, including bit 1
		 * (N) = 0 (M103a ALTPRB); bits 3/5 are kept the same way. */
		flag = (flag & 0x0fff) | 0xf000;
	} else {
		flag = (flag & 0x0fd7) | 0xf002;
	}
	UPD9002_OV = flag & O_FLAG;
	UPD9002_FLAG = flag & (0xfff ^ O_FLAG);
	UPD9002_TRAP = ((flag & T_FLAG) != 0);
	if (return_compat) {
		upd9002_core_compat_iret_resume();
	}
	UPD9002_WORKCLOCK(31);
	if ((UPD9002_TRAP) || ((flag & I_FLAG) && (PICEXISTINTR))) {
		UPD9002_IRQCHECKTERM
	}
}

UPD9002FN _shift_ea8_1(void) { // D0:	shift EA8, 1

	BYTE *out;
	UINT op;
	UINT32 madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			sft_e8_table[(op >> 3) & 7](madr);
			return;
		}
		out = mem + madr;
	}
	sft_r8_table[(op >> 3) & 7](out);
}

UPD9002FN _shift_ea16_1(void) { // D1:	shift EA16, 1

	UINT16 *out;
	UINT op;
	UINT32 madr;

	GET_PCBYTE(op)
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(7);
		{
			UINT32 seg;
			UINT off;

			off = GET_EA(op, &seg);
			madr = seg + off;
			if ((LOW16(off) == 0xffff) || INHIBIT_WORDP(madr)) {
				UINT16 tmp;

				tmp = upd9002_memoryread_seg_w(seg, off);
				sft_r16_table[(op >> 3) & 7](&tmp);
				upd9002_memorywrite_seg_w(seg, off, tmp);
				return;
			}
		}
		if (INHIBIT_WORDP(madr)) {
			sft_e16_table[(op >> 3) & 7](madr);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	sft_r16_table[(op >> 3) & 7](out);
}

UPD9002FN _shift_ea8_cl(void) { // D2:	shift EA8, cl

	UINT8 *out;
	UINT op;
	UINT32 madr = 0;
	REG8 cl;

	GET_PCBYTE(op)
	if (op & 0x20) {
		UINT8 value;

		UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 8);
		value = _ea8_read(op, &madr);
		cl = UPD9002_CL;
		UPD9002_WORKCLOCK(cl);
		if (cl) {
			_ea8_write(op, madr, _shift8(value, cl, (op >> 3) & 7));
		}
		return;
	}
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG8_B20(op);
	} else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (madr >= UPD9002_MEMWRITEMAX) {
			cl = UPD9002_CL;
			UPD9002_WORKCLOCK(cl);
			if ((op & 0x30) == 0x10) { // rotate with carry
				cl = shiftbase09[cl];
			} else {
				cl = shiftbase16[cl];
			}
			sft_e8cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = mem + madr;
	}
	cl = UPD9002_CL;
	UPD9002_WORKCLOCK(cl);
	if ((op & 0x30) == 0x10) { // rotate with carry
		cl = shiftbase09[cl];
	} else {
		cl = shiftbase16[cl];
	}
	sft_r8cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN _shift_ea16_cl(void) { // D3:	shift EA16, cl

	UINT16 *out;
	UINT op;
	UINT32 madr = 0;
	REG8 cl;

	GET_PCBYTE(op)
	if (op & 0x20) {
		UINT16 value;

		UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 8);
		value = _ea16_read(op, &madr);
		cl = UPD9002_CL;
		UPD9002_WORKCLOCK(cl);
		if (cl) {
			_ea16_write(op, madr, _shift16(value, cl, (op >> 3) & 7));
		}
		return;
	}
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(5);
		out = REG16_B20(op);
	} else {
		UPD9002_WORKCLOCK(8);
		madr = CALC_EA(op);
		if (INHIBIT_WORDP(madr)) {
			cl = UPD9002_CL;
			UPD9002_WORKCLOCK(cl);
			if ((op & 0x30) == 0x10) { // rotate with carry
				cl = shiftbase17[cl];
			} else {
				cl = shiftbase16[cl];
			}
			sft_e16cl_table[(op >> 3) & 7](madr, cl);
			return;
		}
		out = (UINT16 *)(mem + madr);
	}
	cl = UPD9002_CL;
	UPD9002_WORKCLOCK(cl);
	if ((op & 0x30) == 0x10) { // rotate with carry
		cl = shiftbase17[cl];
	} else {
		cl = shiftbase16[cl];
	}
	sft_r16cl_table[(op >> 3) & 7](out, cl);
}

UPD9002FN _aam(void) { // D4:	AAM

	UINT8 al;
	UINT radix;

	UPD9002_WORKCLOCK(16);
	GET_PCBYTE(radix);
	al = UPD9002_AL;
	if (radix) {
		UPD9002_AH = (UINT8)(al / radix);
		UPD9002_AL = (UINT8)(al % radix);
	} else {
		UPD9002_AH = 0xff;
	}
	UPD9002_FLAGL = (UPD9002_FLAGL & 0x02) | BYTESZPF(UPD9002_AL);
	UPD9002_OV = 0;
}

UPD9002FN _aad(void) { // D5:	AAD

	UPD9002_WORKCLOCK(14);
	UPD9002_IP++; // is 10
	UPD9002_AL += (UINT8)(UPD9002_AH * 10);
	UPD9002_AH = 0;
	UPD9002_FLAGL &= ~(S_FLAG | Z_FLAG | P_FLAG);
	UPD9002_FLAGL |= BYTESZPF(UPD9002_AL);
}

UPD9002FN _xlat(void) { // D6:	xlat

	UPD9002_WORKCLOCK(5);
	UPD9002_AL = upd9002_memoryread(LOW16(UPD9002_AL + UPD9002_BX) + DS_FIX);
}

UPD9002FN _esc(void) { // D8:	esc

	UINT op;
	UINT8 opcode;
	UINT32 segment;
	UINT offset;
	UINT32 service_ticks;
	UPD8087_OPERAND operand;

	UPD9002_WORKCLOCK(2);
	GET_PCBYTE(op)
	if (!upd8087.enabled) {
		/* The disabled-device path must release the logical input without
		 * manufacturing a vector or touching the guest PIC registers. */
		pic_setirq_level(IRQ_NDP, FALSE);
		if (op < 0xc0) {
			CALC_LEA(op);
		}
		return;
	}
	opcode = upd9002_8087_primary_opcode();

	/* The native CPU decoder owns ModR/M, displacement, segment selection,
	 * effective address, and the first operand-word bus cycle. */
	ZeroMemory(&operand, sizeof(operand));
	operand.bus.read8 = upd9002_8087_read8;
	operand.bus.write8 = upd9002_8087_write8;
	if (op < 0xc0) {
		offset = GET_EA(op, &segment);
		operand.address = (segment + offset) & 0x000fffff;
		operand.first_word = upd9002_memoryread_seg_w(segment, offset);
		operand.first_word_valid = TRUE;
		upd8087.data_address = operand.address;
	}
	upd8087.instruction_address = (CS_BASE + upd9002_step_start_ip) & 0x000fffff;
	(void)upd8087_execute(&upd8087, opcode, (UINT8)op,
	                      (op < 0xc0) ? &operand : NULL);
	service_ticks = upd8087_service_ticks(&upd8087,
		upd8087_instruction_cycles(opcode, (UINT8)op),
		pccore_cpu_clock());
	UPD9002_REMCLOCK -= (SINT32)service_ticks;
	/* Proposed VA logical route: the sole 8087 drives slave PIC IR6
	 * (logical IRQ14), and the existing PIC cascade chooses the guest vector
	 * from its programmed ICW2.  Do not inject vector 16H here. */
	pic_setirq_level(IRQ_NDP, upd8087_interrupt_pending(&upd8087) ? TRUE : FALSE);
}

UPD9002FN _loopnz(void) { // E0:	loopnz

	UPD9002_CX--;
	if ((!UPD9002_CX) || (UPD9002_FLAGL & Z_FLAG))
		JMPNOP(4) else JMPSHORT(8)
}

UPD9002FN _loopz(void) { // E1:	loopz

	UPD9002_CX--;
	if ((!UPD9002_CX) || (!(UPD9002_FLAGL & Z_FLAG)))
		JMPNOP(4) else JMPSHORT(8)
}

UPD9002FN _loop(void) { // E2:	loop

	UPD9002_CX--;
	if (!UPD9002_CX)
		JMPNOP(5) else JMPSHORT(17)
}

UPD9002FN _jcxz(void) { // E3:	jcxz

	if (UPD9002_CX)
		JMPNOP(4) else JMPSHORT(8)
}

/*
 * Native I/O trap (FFE0h-FFEFh). A match performs no access and raises
 * vector 7Ch (IN) or 7Dh (OUT) with the saved IP at the instruction start,
 * the same restart point the core uses for faults. FFEFh bit 4 selects
 * low-byte port matching; ports FFE0h-FFFFh never trap.
 */
static BOOL native_iotrap(UINT port, BOOL input) {
	const BYTE *r = upd9002_iotrap.ranges;
	const BYTE control = upd9002_iotrap.control;
	UINT match;
	UINT block;

	if (!(control & (input ? 1 : 2)) || ((port & 0xffff) >= 0xffe0)) {
		return FALSE;
	}
	match = (control & 0x10) ? (port & 0xff) : (port & 0xffff);
	for (block = 0; block < 8; block += 4) {
		if ((match >= (UINT)(r[block] | (r[block + 1] << 8))) &&
		    (match <= (UINT)(r[block + 2] | (r[block + 3] << 8)))) {
			upd9002_intnum(input ? 0x7c : 0x7d, upd9002_step_start_ip);
			return TRUE;
		}
	}
	return FALSE;
}

UPD9002FN _in_al_data8(void) { // E4:	in		al, DATA8

	UINT port;

	UPD9002_WORKCLOCK(5);
	GET_PCBYTE(port)
	if (native_iotrap(port, TRUE)) {
		return;
	}
	UPD9002_INPADRS = CS_BASE + UPD9002_IP;
	UPD9002_AL = iocore_inp8(port);
	UPD9002_INPADRS = 0;
}

UPD9002FN _in_ax_data8(void) { // E5:	in		ax, DATA8

	UINT port;

	UPD9002_WORKCLOCK(5);
	GET_PCBYTE(port)
	if (native_iotrap(port, TRUE)) {
		return;
	}
	UPD9002_AX = iocore_inp16(port);
}

UPD9002FN _out_data8_al(void) { // E6:	out		DATA8, al

	UINT port;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(port);
	if (native_iotrap(port, FALSE)) {
		return;
	}
	iocore_out8(port, UPD9002_AL);
}

UPD9002FN _out_data8_ax(void) { // E7:	out		DATA8, ax

	UINT port;

	UPD9002_WORKCLOCK(3);
	GET_PCBYTE(port);
	if (native_iotrap(port, FALSE)) {
		return;
	}
	iocore_out16(port, UPD9002_AX);
}

UPD9002FN _call_near(void) { // E8:	call near

	UINT16 ad;

	UPD9002_WORKCLOCK(7);
	GET_PCWORD(ad)
	REGPUSH0(UPD9002_IP)
	UPD9002_IP += ad;
}

UPD9002FN _jmp_near(void) { // E9:	jmp near

	UINT16 ad;

	UPD9002_WORKCLOCK(7);
	GET_PCWORD(ad)
	UPD9002_IP += ad;
}

UPD9002FN _jmp_far(void) { // EA:	jmp far

	UINT16 ad;

	UPD9002_WORKCLOCK(11);
	GET_PCWORD(ad);
	GET_PCWORD(UPD9002_CS);
	UPD9002_IP = ad;
	CS_BASE = SEGSELECT(UPD9002_CS);
}

UPD9002FN _jmp_short(void) { // EB:	jmp short

	UINT16 ad;

	UPD9002_WORKCLOCK(7);
	GET_PCBYTES(ad)
	UPD9002_IP += ad;
}

UPD9002FN _in_al_dx(void) { // EC:	in		al, dx

	UPD9002_WORKCLOCK(5);
	if (native_iotrap(UPD9002_DX, TRUE)) {
		return;
	}
	UPD9002_AL = iocore_inp8(UPD9002_DX);
}

UPD9002FN _in_ax_dx(void) { // ED:	in		ax, dx

	UPD9002_WORKCLOCK(5);
	if (native_iotrap(UPD9002_DX, TRUE)) {
		return;
	}
	UPD9002_AX = iocore_inp16(UPD9002_DX);
}

UPD9002FN _out_dx_al(void) { // EE:	out		dx, al

	UPD9002_WORKCLOCK(3);
	if (native_iotrap(UPD9002_DX, FALSE)) {
		return;
	}
	iocore_out8(UPD9002_DX, UPD9002_AL);
}

UPD9002FN _out_dx_ax(void) { // EF:	out		dx, ax

	UPD9002_WORKCLOCK(3);
	if (native_iotrap(UPD9002_DX, FALSE)) {
		return;
	}
	iocore_out16(UPD9002_DX, UPD9002_AX);
}

UPD9002FN _lock(void) { // F0:	lock
	                    // F1:	lock
	UPD9002_WORKCLOCK(2);
}

UPD9002FN _repne(void) { // F2:	repne

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repe(void) { // F3:	repe

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _hlt(void) { // F4:	hlt

	UPD9002_REMCLOCK = -1;
	UPD9002_IP--;
}

UPD9002FN _cmc(void) { // F5:	cmc

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAGL ^= C_FLAG;
}

UPD9002FN _ope0xf6(void) { // F6:

	UINT op;

	GET_PCBYTE(op);
	c_ope0xf6_table[(op >> 3) & 7](op);
}

UPD9002FN _ope0xf7(void) { // F7:

	UINT op;

	GET_PCBYTE(op);
	c_ope0xf7_table[(op >> 3) & 7](op);
}

UPD9002FN _clc(void) { // F8:	clc

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAGL &= ~C_FLAG;
}

UPD9002FN _stc(void) { // F9:	stc

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAGL |= C_FLAG;
}

UPD9002FN _cli(void) { // FA:	cli

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAG &= ~I_FLAG;
	UPD9002_TRAP = 0;
}

UPD9002FN _sti(void) { // FB:	sti

	UPD9002_WORKCLOCK(2);
#if defined(INTR_FAST)
	if (UPD9002_FLAG & I_FLAG) {
		NEXT_OPCODE;
		return; // No architectural state would change.
	}
#endif
	UPD9002_FLAG |= I_FLAG;
	UPD9002_TRAP = (UPD9002_FLAG & T_FLAG) >> 8;
#if defined(INTR_FAST)
	if ((UPD9002_TRAP) || (PICEXISTINTR)) {
		REMAIN_ADJUST(1)
	} else {
		NEXT_OPCODE;
	}
#else
	REMAIN_ADJUST(1)
#endif
}

UPD9002FN _cld(void) { // FC:	cld

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAG &= ~D_FLAG;
}

UPD9002FN _std(void) { // FD:	std

	UPD9002_WORKCLOCK(2);
	UPD9002_FLAG |= D_FLAG;
}

UPD9002FN _ope0xfe(void) { // FE:

	UINT op;

	GET_PCBYTE(op);
	c_ope0xfe_table[(op >> 3) & 1](op);
}

UPD9002FN _ope0xff(void) { // FF:

	UINT op;

	GET_PCBYTE(op);
	c_ope0xff_table[(op >> 3) & 7](op);
}

UPD9002FN _reserved_no_int(void) {
	upd9002_perf_record_reserved(UPD9002_PERF_RESERVED_PLAIN);
	UPD9002_WORKCLOCK(2);
}
UPD9002FN _mov_seg_ea(void) { // 8E:	mov		segrem, EA

	UINT op;
	UINT tmp;
	GET_PCBYTE(op);
	if (op >= 0xc0) {
		UPD9002_WORKCLOCK(2);
		tmp = *(REG16_B20(op));
	} else {
		UPD9002_WORKCLOCK(5);
		tmp = upd9002_memoryread_w(CALC_EA(op));
	}
	switch (op & 0x18) {
	case 0x00: // es
		UPD9002_ES = (UINT16)tmp;
		ES_BASE = tmp << 4;
		break;

	case 0x08: // cs
		UPD9002_CS = (UINT16)tmp;
		CS_BASE = tmp << 4;
		break;

	case 0x10: // ss
		UPD9002_SS = (UINT16)tmp;
		SS_BASE = tmp << 4;
		SS_FIX = SS_BASE;
		NEXT_OPCODE
		break;

	case 0x18: // ds
		UPD9002_DS = (UINT16)tmp;
		DS_BASE = tmp << 4;
		DS_FIX = DS_BASE;
		break;
	}
}
static UINT16 _materialize_pushf_image(void) {
	return (UINT16)((UPD9002_FLAG & (UINT16)~O_FLAG) | (UPD9002_OV ? O_FLAG : 0));
}
static UINT8 _shift8(UINT8 value, UINT count, UINT subform) {
	UINT8 result;
	UINT8 carry;

	if (!count) {
		return value;
	}
	switch (subform) {
	case 4:
	case 6:
		result = (count < 8) ? (UINT8)(value << count) : 0;
		carry = (count <= 8) ? (UINT8)((value >> (8 - count)) & 1) : 0;
		UPD9002_OV = ((result >> 7) ^ carry) & 1;
		break;

	case 5:
		result = (count < 8) ? (UINT8)(value >> count) : 0;
		carry = (count <= 8) ? (UINT8)((value >> (count - 1)) & 1) : 0;
		UPD9002_OV = ((result >> 7) ^ (result >> 6)) & 1;
		break;

	default:
		if (count < 8) {
			result = (UINT8)(value >> count);
			if (value & 0x80) {
				result |= (UINT8)(0xffU << (8 - count));
			}
		} else {
			result = (value & 0x80) ? 0xff : 0;
		}
		carry = (count <= 8) ? (UINT8)((value >> (count - 1)) & 1) : (UINT8)((value >> 7) & 1);
		UPD9002_OV = 0;
		break;
	}
	UPD9002_FLAGL = (UINT8)((UPD9002_FLAGL & 0x02) | carry | BYTESZPF(result));
	return result;
}
static UINT16 _shift16(UINT16 value, UINT count, UINT subform) {
	UINT16 result;
	UINT8 carry;

	if (!count) {
		return value;
	}
	switch (subform) {
	case 4:
	case 6:
		result = (count < 16) ? (UINT16)(value << count) : 0;
		carry = (count <= 16) ? (UINT8)((value >> (16 - count)) & 1) : 0;
		UPD9002_OV = ((result >> 15) ^ carry) & 1;
		break;

	case 5:
		result = (count < 16) ? (UINT16)(value >> count) : 0;
		carry = (count <= 16) ? (UINT8)((value >> (count - 1)) & 1) : 0;
		UPD9002_OV = ((result >> 15) ^ (result >> 14)) & 1;
		break;

	default:
		if (count < 16) {
			result = (UINT16)(value >> count);
			if (value & 0x8000) {
				result |= (UINT16)(0xffffU << (16 - count));
			}
		} else {
			result = (value & 0x8000) ? 0xffff : 0;
		}
		carry = (count <= 16) ? (UINT8)((value >> (count - 1)) & 1) : (UINT8)((value >> 15) & 1);
		UPD9002_OV = 0;
		break;
	}
	UPD9002_FLAGL = (UINT8)((UPD9002_FLAGL & 0x02) | carry | WORDSZPF(result));
	return result;
}
static void _adjust_flags(UINT8 value, BOOL adjust_low, BOOL adjust_high, UINT overflow) {
	UPD9002_FLAGL = (UINT8)((UPD9002_FLAGL & 0x02) | (adjust_low ? A_FLAG : 0) |
	                        (adjust_high ? C_FLAG : 0) | BYTESZPF(value));
	UPD9002_OV = overflow;
}
UPD9002FN _repne_0f_diagnostic_stop(void) {
	upd9002_perf_record_reserved(UPD9002_PERF_RESERVED_REP0F_DIAGNOSTIC);
	upd9002_diagnostic_raise_rep0f(0xf2, upd9002_step_start_cs, upd9002_step_start_ip);
}
UPD9002FN _repe_0f_diagnostic_stop(void) {
	upd9002_perf_record_reserved(UPD9002_PERF_RESERVED_REP0F_DIAGNOSTIC);
	upd9002_diagnostic_raise_rep0f(0xf3, upd9002_step_start_cs, upd9002_step_start_ip);
}
UPD9002FN _test1_ea8_cl(void) { // 0F 10: test1 EA8, CL

	UINT op;
	UINT32 madr = 0;
	UINT8 value;
	UINT8 mask;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 3 : 12);
	value = _ea8_read(op, &madr);
	mask = (UINT8)(1U << (UPD9002_CL & 7));
	UPD9002_OV = 0;
	UPD9002_FLAGL = BYTESZPF(value & mask);
}
UPD9002FN _test1_ea16_cl(void) { // 0F 11: test1 EA16, CL

	UINT op;
	UINT32 madr = 0;
	UINT16 value;
	UINT16 mask;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 3 : 12);
	value = _ea16_read(op, &madr);
	mask = (UINT16)(1U << (UPD9002_CL & 15));
	UPD9002_OV = 0;
	UPD9002_FLAGL = WORDSZPF(value & mask);
}
UPD9002FN _clr1_ea8_cl(void) { // 0F 12: clr1 EA8, CL

	UINT op;
	UINT32 madr = 0;
	UINT8 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 14);
	value = _ea8_read(op, &madr);
	value &= (UINT8) ~(1U << (UPD9002_CL & 7));
	_ea8_write(op, madr, value);
}
UPD9002FN _clr1_ea16_cl(void) { // 0F 13: clr1 EA16, CL

	UINT op;
	UINT32 madr = 0;
	UINT16 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 14);
	value = _ea16_read(op, &madr);
	value &= (UINT16) ~(1U << (UPD9002_CL & 15));
	_ea16_write(op, madr, value);
}
UPD9002FN _set1_ea8_cl(void) { // 0F 14: set1 EA8, CL

	UINT op;
	UINT32 madr = 0;
	UINT8 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 4 : 13);
	value = _ea8_read(op, &madr);
	value |= (UINT8)(1U << (UPD9002_CL & 7));
	_ea8_write(op, madr, value);
}
UPD9002FN _set1_ea16_cl(void) { // 0F 15: set1 EA16, CL

	UINT op;
	UINT32 madr = 0;
	UINT16 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 4 : 13);
	value = _ea16_read(op, &madr);
	value |= (UINT16)(1U << (UPD9002_CL & 15));
	_ea16_write(op, madr, value);
}
UPD9002FN _not1_ea8_cl(void) { // 0F 16: not1 EA8, CL

	UINT op;
	UINT32 madr = 0;
	UINT8 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 4 : 13);
	value = _ea8_read(op, &madr);
	value ^= (UINT8)(1U << (UPD9002_CL & 7));
	_ea8_write(op, madr, value);
}
UPD9002FN _not1_ea16_cl(void) { // 0F 17: not1 EA16, CL

	UINT op;
	UINT32 madr = 0;
	UINT16 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 4 : 13);
	value = _ea16_read(op, &madr);
	value ^= (UINT16)(1U << (UPD9002_CL & 15));
	_ea16_write(op, madr, value);
}
UPD9002FN _test1_ea8_i3(void) { // 0F 18: test1 EA8, imm3

	UINT op;
	UINT imm;
	UINT32 madr = 0;
	UINT8 value;
	UINT8 mask;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 4 : 13);
	value = _ea8_read(op, &madr);
	GET_PCBYTE(imm);
	mask = (UINT8)(1U << (imm & 7));
	UPD9002_OV = 0;
	UPD9002_FLAGL = BYTESZPF(value & mask);
}
UPD9002FN _test1_ea16_i4(void) { // 0F 19: test1 EA16, imm4

	UINT op;
	UINT imm;
	UINT32 madr = 0;
	UINT16 value;
	UINT16 mask;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 4 : 13);
	value = _ea16_read(op, &madr);
	GET_PCBYTE(imm);
	mask = (UINT16)(1U << (imm & 15));
	UPD9002_OV = 0;
	UPD9002_FLAGL = WORDSZPF(value & mask);
}
UPD9002FN _clr1_ea8_i3(void) { // 0F 1A: clr1 EA8, imm3

	UINT op;
	UINT imm;
	UINT32 madr = 0;
	UINT8 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 6 : 15);
	value = _ea8_read(op, &madr);
	GET_PCBYTE(imm);
	value &= (UINT8) ~(1U << (imm & 7));
	_ea8_write(op, madr, value);
}
UPD9002FN _clr1_ea16_i4(void) { // 0F 1B: clr1 EA16, imm4

	UINT op;
	UINT imm;
	UINT32 madr = 0;
	UINT16 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 6 : 15);
	value = _ea16_read(op, &madr);
	GET_PCBYTE(imm);
	value &= (UINT16) ~(1U << (imm & 15));
	_ea16_write(op, madr, value);
}
UPD9002FN _set1_ea8_i3(void) { // 0F 1C: set1 EA8, imm3

	UINT op;
	UINT imm;
	UINT32 madr = 0;
	UINT8 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 14);
	value = _ea8_read(op, &madr);
	GET_PCBYTE(imm);
	value |= (UINT8)(1U << (imm & 7));
	_ea8_write(op, madr, value);
}
UPD9002FN _set1_ea16_i4(void) { // 0F 1D: set1 EA16, imm4

	UINT op;
	UINT imm;
	UINT32 madr = 0;
	UINT16 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 14);
	value = _ea16_read(op, &madr);
	GET_PCBYTE(imm);
	value |= (UINT16)(1U << (imm & 15));
	_ea16_write(op, madr, value);
}
UPD9002FN _not1_ea8_i3(void) { // 0F 1E: not1 EA8, imm3

	UINT op;
	UINT imm;
	UINT32 madr = 0;
	UINT8 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 14);
	value = _ea8_read(op, &madr);
	GET_PCBYTE(imm);
	value ^= (UINT8)(1U << (imm & 7));
	_ea8_write(op, madr, value);
}
UPD9002FN _not1_ea16_i4(void) { // 0F 1F: not1 EA16, imm4

	UINT op;
	UINT imm;
	UINT32 madr = 0;
	UINT16 value;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK((op >= 0xc0) ? 5 : 14);
	value = _ea16_read(op, &madr);
	GET_PCBYTE(imm);
	value ^= (UINT16)(1U << (imm & 15));
	_ea16_write(op, madr, value);
}
static UINT8 _add8_flag(UINT8 dst, UINT8 src, UINT8 carry, UINT8 *result) {
	UINT res;

	res = dst + src + (carry & C_FLAG);
	*result = (UINT8)res;
	return (UINT8)(((res ^ dst ^ src) & A_FLAG) | BYTESZPCF(res));
}
static UINT8 _sub8_flag(UINT8 dst, UINT8 src, UINT8 borrow, UINT8 *result) {
	UINT res;

	res = dst - src - (borrow & C_FLAG);
	*result = (UINT8)res;
	return (UINT8)(((res ^ dst ^ src) & A_FLAG) | BYTESZPCF2(res));
}
static UINT8 _daa_local(UINT8 value, UINT8 flags, UINT8 *outflags) {
	const BOOL adjust_low = ((flags & A_FLAG) || ((value & 0x0f) > 9));
	const BOOL adjust_high =
	    ((flags & C_FLAG) || (value > 0x9f) || ((value > 0x99) && !(flags & A_FLAG)));
	const UINT8 result = (UINT8)(value + (adjust_low ? 6 : 0) + (adjust_high ? 0x60 : 0));

	*outflags = (UINT8)((adjust_low ? A_FLAG : 0) | (adjust_high ? C_FLAG : 0) | BYTESZPF(result));
	return result;
}
static UINT8 _das_local(UINT8 value, UINT8 flags, UINT8 *outflags) {
	const BOOL adjust_low = ((flags & A_FLAG) || ((value & 0x0f) > 9));
	const BOOL adjust_high =
	    ((flags & C_FLAG) || (value > 0x9f) || ((value > 0x99) && !(flags & A_FLAG)));
	const UINT8 result = (UINT8)(value - (adjust_low ? 6 : 0) - (adjust_high ? 0x60 : 0));

	*outflags = (UINT8)((adjust_low ? A_FLAG : 0) | (adjust_high ? C_FLAG : 0) | BYTESZPF(result));
	return result;
}
static UINT _addsub4s_extra_count(void) {
	UINT8 count;

	count = (UINT8)((UPD9002_CL + 1) >> 1);
	count = (UINT8)(count - 1);
	return count & 0x7f;
}
static void _addsub4s_finish(UINT8 flags) {
	if (flags & C_FLAG) {
		UPD9002_FLAGL = 0x93;
	} else if (flags & Z_FLAG) {
		UPD9002_FLAGL = 0x46;
	} else {
		UPD9002_FLAGL = 0x02;
	}
	UPD9002_OV = 0;
}
UPD9002FN _add4s(void) { // 0F 20: add4s

	UINT16 srcoffset;
	UINT16 dstoffset;
	UINT count;
	UINT8 src;
	UINT8 dst;
	UINT8 flags;
	UINT8 result;
	BOOL all_zero;

	UPD9002_WORKCLOCK(26);
	srcoffset = UPD9002_SI;
	dstoffset = UPD9002_DI;
	flags = 0;
	all_zero = TRUE;
	for (count = _addsub4s_extra_count() + 1; count; count--) {
		const UINT32 srcaddr = (DS_FIX + srcoffset) & CPU_ADRSMASK;
		const UINT32 dstaddr = (ES_BASE + dstoffset) & CPU_ADRSMASK;

		src = upd9002_memoryread(srcaddr);
		dst = upd9002_memoryread(dstaddr);
		flags = _add8_flag(dst, src, flags, &result);
		result = _daa_local(result, flags, &flags);
		upd9002_memorywrite(dstaddr, result);
		all_zero = (BOOL)(all_zero && !result);
		srcoffset++;
		dstoffset++;
		if (count > 1) {
			UPD9002_WORKCLOCK(19);
		}
	}
	if (all_zero) {
		flags |= Z_FLAG;
	} else {
		flags &= (UINT8)~Z_FLAG;
	}
	_addsub4s_finish(flags);
}
static void _subcmp4s(BOOL compare_only) {
	UINT16 srcoffset;
	UINT16 dstoffset;
	UINT count;
	UINT8 src;
	UINT8 dst;
	UINT8 flags;
	UINT8 result;
	BOOL all_zero;

	UPD9002_WORKCLOCK(26);
	srcoffset = UPD9002_SI;
	dstoffset = UPD9002_DI;
	flags = 0;
	all_zero = TRUE;
	for (count = _addsub4s_extra_count() + 1; count; count--) {
		const UINT32 srcaddr = (DS_FIX + srcoffset) & CPU_ADRSMASK;
		const UINT32 dstaddr = (ES_BASE + dstoffset) & CPU_ADRSMASK;

		src = upd9002_memoryread(srcaddr);
		dst = upd9002_memoryread(dstaddr);
		flags = _sub8_flag(dst, src, flags, &result);
		result = _das_local(result, flags, &flags);
		if (!compare_only) {
			upd9002_memorywrite(dstaddr, result);
		}
		all_zero = (BOOL)(all_zero && !result);
		srcoffset++;
		dstoffset++;
		if (count > 1) {
			UPD9002_WORKCLOCK(19);
		}
	}
	if (all_zero) {
		flags |= Z_FLAG;
	} else {
		flags &= (UINT8)~Z_FLAG;
	}
	_addsub4s_finish(flags);
}
UPD9002FN _sub4s(void) { // 0F 22: sub4s

	_subcmp4s(FALSE);
}
UPD9002FN _cmp4s(void) { // 0F 26: cmp4s

	_subcmp4s(TRUE);
}
UPD9002FN _rol4_ea8(void) { // 0F 28: rol4 EA8

	UINT op;
	UINT32 madr = 0;
	UINT8 value;
	UINT8 oldal;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK(25);
	value = _ea8_read(op, &madr);
	oldal = UPD9002_AL;
	_ea8_write(op, madr, (UINT8)((value << 4) | (oldal & 0x0f)));
	UPD9002_AL = (UINT8)((oldal << 4) | (value >> 4));
}
UPD9002FN _ror4_ea8(void) { // 0F 2A: ror4 EA8

	UINT op;
	UINT32 madr = 0;
	UINT8 value;
	UINT8 oldal;

	GET_PCBYTE(op);
	UPD9002_WORKCLOCK(25);
	value = _ea8_read(op, &madr);
	oldal = UPD9002_AL;
	_ea8_write(op, madr, (UINT8)((value >> 4) | ((oldal & 0x0f) << 4)));
	UPD9002_AL = value;
}
UPD9002FN _reserved_repc(void) {
	upd9002_perf_record_reserved(UPD9002_PERF_RESERVED_REPC);
	UPD9002_WORKCLOCK(2);
	UPD9002_IP = upd9002_repc_ipbak;
}
UPD9002FN _reserved_repnc(void) {
	upd9002_perf_record_reserved(UPD9002_PERF_RESERVED_REPNC);
	UPD9002_WORKCLOCK(2);
	UPD9002_IP = upd9002_repnc_ipbak;
}
UPD9002FN _repnc(void) { // 64: repnc

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		upd9002_repnc_ipbak = (UINT16)(UPD9002_IP - 1);
		GET_PCBYTE(op);
		upd9002op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repc(void) { // 65: repc

	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		upd9002_repc_ipbak = (UINT16)(UPD9002_IP - 1);
		GET_PCBYTE(op);
		upd9002op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repnc_segprefix_es(void) {
	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		GET_PCBYTE(op);
		upd9002op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repnc_segprefix_cs(void) {
	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		GET_PCBYTE(op);
		upd9002op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repnc_segprefix_ss(void) {
	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		GET_PCBYTE(op);
		upd9002op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repnc_segprefix_ds(void) {
	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		GET_PCBYTE(op);
		upd9002op_repnc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repc_segprefix_es(void) {
	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		GET_PCBYTE(op);
		upd9002op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repc_segprefix_cs(void) {
	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		GET_PCBYTE(op);
		upd9002op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repc_segprefix_ss(void) {
	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		GET_PCBYTE(op);
		upd9002op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _repc_segprefix_ds(void) {
	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;

		GET_PCBYTE(op);
		upd9002op_repc[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}
UPD9002FN _reserved_0x0f(void) {
	upd9002_perf_record_reserved(UPD9002_PERF_RESERVED_0F);
	UPD9002_WORKCLOCK(2);
}
static const UPD9002OP upd9002_ope0x0f_table[64] = {_reserved_0x0f, // 00:
                                                    _reserved_0x0f, // 01:
                                                    _reserved_0x0f, // 02:
                                                    _reserved_0x0f, // 03:
                                                    _reserved_0x0f, // 04:
                                                    _reserved_0x0f, // 05:
                                                    _reserved_0x0f, // 06:
                                                    _reserved_0x0f, // 07:
                                                    _reserved_0x0f, // 08:
                                                    _reserved_0x0f, // 09:
                                                    _reserved_0x0f, // 0A:
                                                    _reserved_0x0f, // 0B:
                                                    _reserved_0x0f, // 0C:
                                                    _reserved_0x0f, // 0D:
                                                    _reserved_0x0f, // 0E:
                                                    _reserved_0x0f, // 0F:

                                                    _test1_ea8_cl,  // 10:
                                                    _test1_ea16_cl, // 11:
                                                    _clr1_ea8_cl,   // 12:
                                                    _clr1_ea16_cl,  // 13:
                                                    _set1_ea8_cl,   // 14:
                                                    _set1_ea16_cl,  // 15:
                                                    _not1_ea8_cl,   // 16:
                                                    _not1_ea16_cl,  // 17:
                                                    _test1_ea8_i3,  // 18:
                                                    _test1_ea16_i4, // 19:
                                                    _clr1_ea8_i3,   // 1A:
                                                    _clr1_ea16_i4,  // 1B:
                                                    _set1_ea8_i3,   // 1C:
                                                    _set1_ea16_i4,  // 1D:
                                                    _not1_ea8_i3,   // 1E:
                                                    _not1_ea16_i4,  // 1F:

                                                    _add4s,         // 20:
                                                    _reserved_0x0f, // 21:
                                                    _sub4s,         // 22:
                                                    _reserved_0x0f, // 23:
                                                    _reserved_0x0f, // 24:
                                                    _reserved_0x0f, // 25:
                                                    _cmp4s,         // 26:
                                                    _reserved_0x0f, // 27:
                                                    _rol4_ea8,      // 28:
                                                    _reserved_0x0f, // 29:
                                                    _ror4_ea8,      // 2A:
                                                    _reserved_0x0f, // 2B:
                                                    _reserved_0x0f, // 2C:
                                                    _reserved_0x0f, // 2D:
                                                    _reserved_0x0f, // 2E:
                                                    _reserved_0x0f, // 2F:

                                                    _reserved_0x0f,  // 30:
                                                    _reserved_0x0f,  // 31:
                                                    _reserved_0x0f,  // 32:
                                                    _reserved_0x0f,  // 33:
                                                    _reserved_0x0f,  // 34:
                                                    _reserved_0x0f,  // 35:
                                                    _reserved_0x0f,  // 36:
                                                    _reserved_0x0f,  // 37:
                                                    _reserved_0x0f,  // 38:
                                                    _reserved_0x0f,  // 39:
                                                    _reserved_0x0f,  // 3A:
                                                    _reserved_0x0f,  // 3B:
                                                    _reserved_0x0f,  // 3C:
                                                    _reserved_0x0f,  // 3D:
                                                    _reserved_0x0f,  // 3E:
                                                    _reserved_0x0f}; // 3F:

static UINT8 _ea8_read(UINT op, UINT32 *madr) {
	if (op >= 0xc0) {
		return *REG8_B20(op);
	}
	*madr = CALC_EA(op);
	return upd9002_memoryread(*madr);
}

static void _ea8_write(UINT op, UINT32 madr, UINT8 value) {
	if (op >= 0xc0) {
		*REG8_B20(op) = value;
	} else {
		upd9002_memorywrite(madr, value);
	}
}

static UINT16 _ea16_read(UINT op, UINT32 *madr) {
	if (op >= 0xc0) {
		return *REG16_B20(op);
	}
	*madr = CALC_EA(op);
	return upd9002_memoryread_w(*madr);
}

static void _ea16_write(UINT op, UINT32 madr, UINT16 value) {
	if (op >= 0xc0) {
		*REG16_B20(op) = value;
	} else {
		upd9002_memorywrite_w(madr, value);
	}
}

UPD9002FN _ope0x0f(void) { // 0F:

	UINT op;
	UINT8 vector;

	op = upd9002_memoryread(CS_BASE + UPD9002_IP);
	upd9002_perf_record_0f((UINT8)op);
	if (op == 0xff || op == 0xfe) {
		/* BRKEM and BRKEM2 share the compatible-mode entry policy.
		 * The system memory mode is selected independently by port 153h. */
		UPD9002_IP++;
		vector = upd9002_memoryread(CS_BASE + UPD9002_IP);
		UPD9002_IP++;
		upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "compat-entry", op, vector, 1);
		upd9002_core_brkem(vector);
		return;
	}
	if (op & 0xc0) {
		_reserved_0x0f();
		return;
	}
	UPD9002_IP++;
	upd9002_ope0x0f_table[op]();
}
// -------------------------------------------------------------------------

const UPD9002OP upd9002op[] = {
    _add_ea_r8,     // 00:
    _add_ea_r16,    // 01:
    _add_r8_ea,     // 02:
    _add_r16_ea,    // 03:
    _add_al_data8,  // 04:
    _add_ax_data16, // 05:
    _push_es,       // 06:
    _pop_es,        // 07:
    _or_ea_r8,      // 08:
    _or_ea_r16,     // 09:
    _or_r8_ea,      // 0A:
    _or_r16_ea,     // 0B:
    _or_al_data8,   // 0C:
    _or_ax_data16,  // 0D:
    _push_cs,       // 0E:
    _ope0x0f,       // 0F:

    _adc_ea_r8,     // 10:
    _adc_ea_r16,    // 11:
    _adc_r8_ea,     // 12:
    _adc_r16_ea,    // 13:
    _adc_al_data8,  // 14:
    _adc_ax_data16, // 15:
    _push_ss,       // 16:
    _pop_ss,        // 17:
    _sbb_ea_r8,     // 18:
    _sbb_ea_r16,    // 19:
    _sbb_r8_ea,     // 1A:
    _sbb_r16_ea,    // 1B:
    _sbb_al_data8,  // 1C:
    _sbb_ax_data16, // 1D:
    _push_ds,       // 1E:
    _pop_ds,        // 1F:

    _and_ea_r8,     // 20:
    _and_ea_r16,    // 21:
    _and_r8_ea,     // 22:
    _and_r16_ea,    // 23:
    _and_al_data8,  // 24:
    _and_ax_data16, // 25:
    _segprefix_es,  // 26:
    _daa,           // 27:
    _sub_ea_r8,     // 28:
    _sub_ea_r16,    // 29:
    _sub_r8_ea,     // 2A:
    _sub_r16_ea,    // 2B:
    _sub_al_data8,  // 2C:
    _sub_ax_data16, // 2D:
    _segprefix_cs,  // 2E:
    _das,           // 2F:

    _xor_ea_r8,     // 30:
    _xor_ea_r16,    // 31:
    _xor_r8_ea,     // 32:
    _xor_r16_ea,    // 33:
    _xor_al_data8,  // 34:
    _xor_ax_data16, // 35:
    _segprefix_ss,  // 36:
    _aaa,           // 37:
    _cmp_ea_r8,     // 38:
    _cmp_ea_r16,    // 39:
    _cmp_r8_ea,     // 3A:
    _cmp_r16_ea,    // 3B:
    _cmp_al_data8,  // 3C:
    _cmp_ax_data16, // 3D:
    _segprefix_ds,  // 3E:
    _aas,           // 3F:

    _inc_ax, // 40:
    _inc_cx, // 41:
    _inc_dx, // 42:
    _inc_bx, // 43:
    _inc_sp, // 44:
    _inc_bp, // 45:
    _inc_si, // 46:
    _inc_di, // 47:
    _dec_ax, // 48:
    _dec_cx, // 49:
    _dec_dx, // 4A:
    _dec_bx, // 4B:
    _dec_sp, // 4C:
    _dec_bp, // 4D:
    _dec_si, // 4E:
    _dec_di, // 4F:

    _push_ax, // 50:
    _push_cx, // 51:
    _push_dx, // 52:
    _push_bx, // 53:
    _push_sp, // 54:
    _push_bp, // 55:
    _push_si, // 56:
    _push_di, // 57:
    _pop_ax,  // 58:
    _pop_cx,  // 59:
    _pop_dx,  // 5A:
    _pop_bx,  // 5B:
    _pop_sp,  // 5C:
    _pop_bp,  // 5D:
    _pop_si,  // 5E:
    _pop_di,  // 5F:

    _pusha,              // 60:
    _popa,               // 61:
    _bound,              // 62:
    _reserved_no_int,    // 63:
    _repnc,              // 64:
    _repc,               // 65:
    _reserved_no_int,    // 66:
    _reserved_no_int,    // 67:
    _push_data16,        // 68:
    _imul_reg_ea_data16, // 69:
    _push_data8,         // 6A:
    _imul_reg_ea_data8,  // 6B:
    _insb,               // 6C:
    _insw,               // 6D:
    _outsb,              // 6E:
    _outsw,              // 6F:

    _jo_short,   // 70:
    _jno_short,  // 71:
    _jc_short,   // 72:
    _jnc_short,  // 73:
    _jz_short,   // 74:
    _jnz_short,  // 75:
    _jna_short,  // 76:
    _ja_short,   // 77:
    _js_short,   // 78:
    _jns_short,  // 79:
    _jp_short,   // 7A:
    _jnp_short,  // 7B:
    _jl_short,   // 7C:
    _jnl_short,  // 7D:
    _jle_short,  // 7E:
    _jnle_short, // 7F:

    _calc_ea8_i8,   // 80:
    _calc_ea16_i16, // 81:
    _calc_ea8_i8,   // 82:
    _calc_ea16_i8,  // 83:
    _test_ea_r8,    // 84:
    _test_ea_r16,   // 85:
    _xchg_ea_r8,    // 86:
    _xchg_ea_r16,   // 87:
    _mov_ea_r8,     // 88:
    _mov_ea_r16,    // 89:
    _mov_r8_ea,     // 8A:
    _mov_r16_ea,    // 8B:
    _mov_ea_seg,    // 8C:
    _lea_r16_ea,    // 8D:
    _mov_seg_ea,    // 8E:
    _pop_ea,        // 8F:

    _nop,        // 90:
    _xchg_ax_cx, // 91:
    _xchg_ax_dx, // 92:
    _xchg_ax_bx, // 93:
    _xchg_ax_sp, // 94:
    _xchg_ax_bp, // 95:
    _xchg_ax_si, // 96:
    _xchg_ax_di, // 97:
    _cbw,        // 98:
    _cwd,        // 99:
    _call_far,   // 9A:
    _wait,       // 9B:
    _pushf,      // 9C:
    _popf,       // 9D:
    _sahf,       // 9E:
    _lahf,       // 9F:

    _mov_al_m8,      // A0:
    _mov_ax_m16,     // A1:
    _mov_m8_al,      // A2:
    _mov_m16_ax,     // A3:
    _movsb,          // A4:
    _movsw,          // A5:
    _cmpsb,          // A6:
    _cmpsw,          // A7:
    _test_al_data8,  // A8:
    _test_ax_data16, // A9:
    _stosb,          // AA:
    _stosw,          // AB:
    _lodsb,          // AC:
    _lodsw,          // AD:
    _scasb,          // AE:
    _scasw,          // AF:

    _mov_al_imm, // B0:
    _mov_cl_imm, // B1:
    _mov_dl_imm, // B2:
    _mov_bl_imm, // B3:
    _mov_ah_imm, // B4:
    _mov_ch_imm, // B5:
    _mov_dh_imm, // B6:
    _mov_bh_imm, // B7:
    _mov_ax_imm, // B8:
    _mov_cx_imm, // B9:
    _mov_dx_imm, // BA:
    _mov_bx_imm, // BB:
    _mov_sp_imm, // BC:
    _mov_bp_imm, // BD:
    _mov_si_imm, // BE:
    _mov_di_imm, // BF:

    _shift_ea8_data8,  // C0:
    _shift_ea16_data8, // C1:
    _ret_near_data16,  // C2:
    _ret_near,         // C3:
    _les_r16_ea,       // C4:
    _lds_r16_ea,       // C5:
    _mov_ea8_data8,    // C6:
    _mov_ea16_data16,  // C7:
    _enter,            // C8:
    fleave,            // C9:
    _ret_far_data16,   // CA:
    _ret_far,          // CB:
    _int_03,           // CC:
    _int_data8,        // CD:
    _into,             // CE:
    _iret,             // CF:

    _shift_ea8_1,   // D0:
    _shift_ea16_1,  // D1:
    _shift_ea8_cl,  // D2:
    _shift_ea16_cl, // D3:
    _aam,           // D4:
    _aad,           // D5:
    _xlat,          // D6:
    _xlat,          // D7:
    _esc,           // D8:
    _esc,           // D9:
    _esc,           // DA:
    _esc,           // DB:
    _esc,           // DC:
    _esc,           // DD:
    _esc,           // DE:
    _esc,           // DF:

    _loopnz,       // E0:
    _loopz,        // E1:
    _loop,         // E2:
    _jcxz,         // E3:
    _in_al_data8,  // E4:
    _in_ax_data8,  // E5:
    _out_data8_al, // E6:
    _out_data8_ax, // E7:
    _call_near,    // E8:
    _jmp_near,     // E9:
    _jmp_far,      // EA:
    _jmp_short,    // EB:
    _in_al_dx,     // EC:
    _in_ax_dx,     // ED:
    _out_dx_al,    // EE:
    _out_dx_ax,    // EF:

    _lock,    // F0:
    _lock,    // F1:
    _repne,   // F2:
    _repe,    // F3:
    _hlt,     // F4:
    _cmc,     // F5:
    _ope0xf6, // F6:
    _ope0xf7, // F7:
    _clc,     // F8:
    _stc,     // F9:
    _cli,     // FA:
    _sti,     // FB:
    _cld,     // FC:
    _std,     // FD:
    _ope0xfe, // FE:
    _ope0xff, // FF:
};

// ----------------------------------------------------------------- repe

UPD9002FN _repe_segprefix_es(void) {
	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repe_segprefix_cs(void) {
	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repe_segprefix_ss(void) {
	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repe_segprefix_ds(void) {
	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repe[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

const UPD9002OP upd9002op_repe[] = {
    _add_ea_r8,               // 00:
    _add_ea_r16,              // 01:
    _add_r8_ea,               // 02:
    _add_r16_ea,              // 03:
    _add_al_data8,            // 04:
    _add_ax_data16,           // 05:
    _push_es,                 // 06:
    _pop_es,                  // 07:
    _or_ea_r8,                // 08:
    _or_ea_r16,               // 09:
    _or_r8_ea,                // 0A:
    _or_r16_ea,               // 0B:
    _or_al_data8,             // 0C:
    _or_ax_data16,            // 0D:
    _push_cs,                 // 0E:
    _repe_0f_diagnostic_stop, // 0F:

    _adc_ea_r8,     // 10:
    _adc_ea_r16,    // 11:
    _adc_r8_ea,     // 12:
    _adc_r16_ea,    // 13:
    _adc_al_data8,  // 14:
    _adc_ax_data16, // 15:
    _push_ss,       // 16:
    _pop_ss,        // 17:
    _sbb_ea_r8,     // 18:
    _sbb_ea_r16,    // 19:
    _sbb_r8_ea,     // 1A:
    _sbb_r16_ea,    // 1B:
    _sbb_al_data8,  // 1C:
    _sbb_ax_data16, // 1D:
    _push_ds,       // 1E:
    _pop_ds,        // 1F:

    _and_ea_r8,         // 20:
    _and_ea_r16,        // 21:
    _and_r8_ea,         // 22:
    _and_r16_ea,        // 23:
    _and_al_data8,      // 24:
    _and_ax_data16,     // 25:
    _repe_segprefix_es, // 26:
    _daa,               // 27:
    _sub_ea_r8,         // 28:
    _sub_ea_r16,        // 29:
    _sub_r8_ea,         // 2A:
    _sub_r16_ea,        // 2B:
    _sub_al_data8,      // 2C:
    _sub_ax_data16,     // 2D:
    _repe_segprefix_cs, // 2E:
    _das,               // 2F:

    _xor_ea_r8,         // 30:
    _xor_ea_r16,        // 31:
    _xor_r8_ea,         // 32:
    _xor_r16_ea,        // 33:
    _xor_al_data8,      // 34:
    _xor_ax_data16,     // 35:
    _repe_segprefix_ss, // 36:
    _aaa,               // 37:
    _cmp_ea_r8,         // 38:
    _cmp_ea_r16,        // 39:
    _cmp_r8_ea,         // 3A:
    _cmp_r16_ea,        // 3B:
    _cmp_al_data8,      // 3C:
    _cmp_ax_data16,     // 3D:
    _repe_segprefix_ds, // 3E:
    _aas,               // 3F:

    _inc_ax, // 40:
    _inc_cx, // 41:
    _inc_dx, // 42:
    _inc_bx, // 43:
    _inc_sp, // 44:
    _inc_bp, // 45:
    _inc_si, // 46:
    _inc_di, // 47:
    _dec_ax, // 48:
    _dec_cx, // 49:
    _dec_dx, // 4A:
    _dec_bx, // 4B:
    _dec_sp, // 4C:
    _dec_bp, // 4D:
    _dec_si, // 4E:
    _dec_di, // 4F:

    _push_ax, // 50:
    _push_cx, // 51:
    _push_dx, // 52:
    _push_bx, // 53:
    _push_sp, // 54:
    _push_bp, // 55:
    _push_si, // 56:
    _push_di, // 57:
    _pop_ax,  // 58:
    _pop_cx,  // 59:
    _pop_dx,  // 5A:
    _pop_bx,  // 5B:
    _pop_sp,  // 5C:
    _pop_bp,  // 5D:
    _pop_si,  // 5E:
    _pop_di,  // 5F:

    _pusha,              // 60:
    _popa,               // 61:
    _bound,              // 62:
    _reserved_no_int,    // 63:
    _repnc,              // 64:
    _repc,               // 65:
    _reserved_no_int,    // 66:
    _reserved_no_int,    // 67:
    _push_data16,        // 68:
    _imul_reg_ea_data16, // 69:
    _push_data8,         // 6A:
    _imul_reg_ea_data8,  // 6B:
    upd9002_rep_insb,    // 6C:
    upd9002_rep_insw,    // 6D:
    upd9002_rep_outsb,   // 6E:
    upd9002_rep_outsb,   // 6F:

    _jo_short,   // 70:
    _jno_short,  // 71:
    _jc_short,   // 72:
    _jnc_short,  // 73:
    _jz_short,   // 74:
    _jnz_short,  // 75:
    _jna_short,  // 76:
    _ja_short,   // 77:
    _js_short,   // 78:
    _jns_short,  // 79:
    _jp_short,   // 7A:
    _jnp_short,  // 7B:
    _jl_short,   // 7C:
    _jnl_short,  // 7D:
    _jle_short,  // 7E:
    _jnle_short, // 7F:

    _calc_ea8_i8,   // 80:
    _calc_ea16_i16, // 81:
    _calc_ea8_i8,   // 82:
    _calc_ea16_i8,  // 83:
    _test_ea_r8,    // 84:
    _test_ea_r16,   // 85:
    _xchg_ea_r8,    // 86:
    _xchg_ea_r16,   // 87:
    _mov_ea_r8,     // 88:
    _mov_ea_r16,    // 89:
    _mov_r8_ea,     // 8A:
    _mov_r16_ea,    // 8B:
    _mov_ea_seg,    // 8C:
    _lea_r16_ea,    // 8D:
    _mov_seg_ea,    // 8E:
    _pop_ea,        // 8F:

    _nop,        // 90:
    _xchg_ax_cx, // 91:
    _xchg_ax_dx, // 92:
    _xchg_ax_bx, // 93:
    _xchg_ax_sp, // 94:
    _xchg_ax_bp, // 95:
    _xchg_ax_si, // 96:
    _xchg_ax_di, // 97:
    _cbw,        // 98:
    _cwd,        // 99:
    _call_far,   // 9A:
    _wait,       // 9B:
    _pushf,      // 9C:
    _popf,       // 9D:
    _sahf,       // 9E:
    _lahf,       // 9F:

    _mov_al_m8,         // A0:
    _mov_ax_m16,        // A1:
    _mov_m8_al,         // A2:
    _mov_m16_ax,        // A3:
    upd9002_rep_movsb,  // A4:
    upd9002_rep_movsw,  // A5:
    upd9002_repe_cmpsb, // A6:
    upd9002_repe_cmpsw, // A7:
    _test_al_data8,     // A8:
    _test_ax_data16,    // A9:
    upd9002_rep_stosb,  // AA:
    upd9002_rep_stosw,  // AB:
    upd9002_rep_lodsb,  // AC:
    upd9002_rep_lodsw,  // AD:
    upd9002_repe_scasb, // AE:
    upd9002_repe_scasw, // AF:

    _mov_al_imm, // B0:
    _mov_cl_imm, // B1:
    _mov_dl_imm, // B2:
    _mov_bl_imm, // B3:
    _mov_ah_imm, // B4:
    _mov_ch_imm, // B5:
    _mov_dh_imm, // B6:
    _mov_bh_imm, // B7:
    _mov_ax_imm, // B8:
    _mov_cx_imm, // B9:
    _mov_dx_imm, // BA:
    _mov_bx_imm, // BB:
    _mov_sp_imm, // BC:
    _mov_bp_imm, // BD:
    _mov_si_imm, // BE:
    _mov_di_imm, // BF:

    _shift_ea8_data8,  // C0:
    _shift_ea16_data8, // C1:
    _ret_near_data16,  // C2:
    _ret_near,         // C3:
    _les_r16_ea,       // C4:
    _lds_r16_ea,       // C5:
    _mov_ea8_data8,    // C6:
    _mov_ea16_data16,  // C7:
    _enter,            // C8:
    fleave,            // C9:
    _ret_far_data16,   // CA:
    _ret_far,          // CB:
    _int_03,           // CC:
    _int_data8,        // CD:
    _into,             // CE:
    _iret,             // CF:

    _shift_ea8_1,   // D0:
    _shift_ea16_1,  // D1:
    _shift_ea8_cl,  // D2:
    _shift_ea16_cl, // D3:
    _aam,           // D4:
    _aad,           // D5:
    _xlat,          // D6:
    _xlat,          // D7:
    _esc,           // D8:
    _esc,           // D9:
    _esc,           // DA:
    _esc,           // DB:
    _esc,           // DC:
    _esc,           // DD:
    _esc,           // DE:
    _esc,           // DF:

    _loopnz,       // E0:
    _loopz,        // E1:
    _loop,         // E2:
    _jcxz,         // E3:
    _in_al_data8,  // E4:
    _in_ax_data8,  // E5:
    _out_data8_al, // E6:
    _out_data8_ax, // E7:
    _call_near,    // E8:
    _jmp_near,     // E9:
    _jmp_far,      // EA:
    _jmp_short,    // EB:
    _in_al_dx,     // EC:
    _in_ax_dx,     // ED:
    _out_dx_al,    // EE:
    _out_dx_ax,    // EF:

    _lock,    // F0:
    _lock,    // F1:
    _repne,   // F2:
    _repe,    // F3:
    _hlt,     // F4:
    _cmc,     // F5:
    _ope0xf6, // F6:
    _ope0xf7, // F7:
    _clc,     // F8:
    _stc,     // F9:
    _cli,     // FA:
    _sti,     // FB:
    _cld,     // FC:
    _std,     // FD:
    _ope0xfe, // FE:
    _ope0xff, // FF:
};

// ----------------------------------------------------------------- repne

UPD9002FN _repne_segprefix_es(void) {
	DS_FIX = ES_BASE;
	SS_FIX = ES_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repne_segprefix_cs(void) {
	DS_FIX = CS_BASE;
	SS_FIX = CS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repne_segprefix_ss(void) {
	DS_FIX = SS_BASE;
	SS_FIX = SS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

UPD9002FN _repne_segprefix_ds(void) {
	DS_FIX = DS_BASE;
	SS_FIX = DS_BASE;
	UPD9002_PREFIX++;
	if (UPD9002_PREFIX < MAX_PREFIX) {
		UINT op;
		GET_PCBYTE(op);
		upd9002op_repne[op]();
		REMOVE_PREFIX
		UPD9002_PREFIX = 0;
	} else {
		INT_NUM(6, UPD9002_IP);
	}
}

const UPD9002OP upd9002op_repne[] = {
    _add_ea_r8,                // 00:
    _add_ea_r16,               // 01:
    _add_r8_ea,                // 02:
    _add_r16_ea,               // 03:
    _add_al_data8,             // 04:
    _add_ax_data16,            // 05:
    _push_es,                  // 06:
    _pop_es,                   // 07:
    _or_ea_r8,                 // 08:
    _or_ea_r16,                // 09:
    _or_r8_ea,                 // 0A:
    _or_r16_ea,                // 0B:
    _or_al_data8,              // 0C:
    _or_ax_data16,             // 0D:
    _push_cs,                  // 0E:
    _repne_0f_diagnostic_stop, // 0F:

    _adc_ea_r8,     // 10:
    _adc_ea_r16,    // 11:
    _adc_r8_ea,     // 12:
    _adc_r16_ea,    // 13:
    _adc_al_data8,  // 14:
    _adc_ax_data16, // 15:
    _push_ss,       // 16:
    _pop_ss,        // 17:
    _sbb_ea_r8,     // 18:
    _sbb_ea_r16,    // 19:
    _sbb_r8_ea,     // 1A:
    _sbb_r16_ea,    // 1B:
    _sbb_al_data8,  // 1C:
    _sbb_ax_data16, // 1D:
    _push_ds,       // 1E:
    _pop_ds,        // 1F:

    _and_ea_r8,          // 20:
    _and_ea_r16,         // 21:
    _and_r8_ea,          // 22:
    _and_r16_ea,         // 23:
    _and_al_data8,       // 24:
    _and_ax_data16,      // 25:
    _repne_segprefix_es, // 26:
    _daa,                // 27:
    _sub_ea_r8,          // 28:
    _sub_ea_r16,         // 29:
    _sub_r8_ea,          // 2A:
    _sub_r16_ea,         // 2B:
    _sub_al_data8,       // 2C:
    _sub_ax_data16,      // 2D:
    _repne_segprefix_cs, // 2E:
    _das,                // 2F:

    _xor_ea_r8,          // 30:
    _xor_ea_r16,         // 31:
    _xor_r8_ea,          // 32:
    _xor_r16_ea,         // 33:
    _xor_al_data8,       // 34:
    _xor_ax_data16,      // 35:
    _repne_segprefix_ss, // 36:
    _aaa,                // 37:
    _cmp_ea_r8,          // 38:
    _cmp_ea_r16,         // 39:
    _cmp_r8_ea,          // 3A:
    _cmp_r16_ea,         // 3B:
    _cmp_al_data8,       // 3C:
    _cmp_ax_data16,      // 3D:
    _repne_segprefix_ds, // 3E:
    _aas,                // 3F:

    _inc_ax, // 40:
    _inc_cx, // 41:
    _inc_dx, // 42:
    _inc_bx, // 43:
    _inc_sp, // 44:
    _inc_bp, // 45:
    _inc_si, // 46:
    _inc_di, // 47:
    _dec_ax, // 48:
    _dec_cx, // 49:
    _dec_dx, // 4A:
    _dec_bx, // 4B:
    _dec_sp, // 4C:
    _dec_bp, // 4D:
    _dec_si, // 4E:
    _dec_di, // 4F:

    _push_ax, // 50:
    _push_cx, // 51:
    _push_dx, // 52:
    _push_bx, // 53:
    _push_sp, // 54:
    _push_bp, // 55:
    _push_si, // 56:
    _push_di, // 57:
    _pop_ax,  // 58:
    _pop_cx,  // 59:
    _pop_dx,  // 5A:
    _pop_bx,  // 5B:
    _pop_sp,  // 5C:
    _pop_bp,  // 5D:
    _pop_si,  // 5E:
    _pop_di,  // 5F:

    _pusha,              // 60:
    _popa,               // 61:
    _bound,              // 62:
    _reserved_no_int,    // 63:
    _repnc,              // 64:
    _repc,               // 65:
    _reserved_no_int,    // 66:
    _reserved_no_int,    // 67:
    _push_data16,        // 68:
    _imul_reg_ea_data16, // 69:
    _push_data8,         // 6A:
    _imul_reg_ea_data8,  // 6B:
    upd9002_rep_insb,    // 6C:
    upd9002_rep_insw,    // 6D:
    upd9002_rep_outsb,   // 6E:
    upd9002_rep_outsb,   // 6F:

    _jo_short,   // 70:
    _jno_short,  // 71:
    _jc_short,   // 72:
    _jnc_short,  // 73:
    _jz_short,   // 74:
    _jnz_short,  // 75:
    _jna_short,  // 76:
    _ja_short,   // 77:
    _js_short,   // 78:
    _jns_short,  // 79:
    _jp_short,   // 7A:
    _jnp_short,  // 7B:
    _jl_short,   // 7C:
    _jnl_short,  // 7D:
    _jle_short,  // 7E:
    _jnle_short, // 7F:

    _calc_ea8_i8,   // 80:
    _calc_ea16_i16, // 81:
    _calc_ea8_i8,   // 82:
    _calc_ea16_i8,  // 83:
    _test_ea_r8,    // 84:
    _test_ea_r16,   // 85:
    _xchg_ea_r8,    // 86:
    _xchg_ea_r16,   // 87:
    _mov_ea_r8,     // 88:
    _mov_ea_r16,    // 89:
    _mov_r8_ea,     // 8A:
    _mov_r16_ea,    // 8B:
    _mov_ea_seg,    // 8C:
    _lea_r16_ea,    // 8D:
    _mov_seg_ea,    // 8E:
    _pop_ea,        // 8F:

    _nop,        // 90:
    _xchg_ax_cx, // 91:
    _xchg_ax_dx, // 92:
    _xchg_ax_bx, // 93:
    _xchg_ax_sp, // 94:
    _xchg_ax_bp, // 95:
    _xchg_ax_si, // 96:
    _xchg_ax_di, // 97:
    _cbw,        // 98:
    _cwd,        // 99:
    _call_far,   // 9A:
    _wait,       // 9B:
    _pushf,      // 9C:
    _popf,       // 9D:
    _sahf,       // 9E:
    _lahf,       // 9F:

    _mov_al_m8,          // A0:
    _mov_ax_m16,         // A1:
    _mov_m8_al,          // A2:
    _mov_m16_ax,         // A3:
    upd9002_rep_movsb,   // A4:
    upd9002_rep_movsw,   // A5:
    upd9002_repne_cmpsb, // A6:
    upd9002_repne_cmpsw, // A7:
    _test_al_data8,      // A8:
    _test_ax_data16,     // A9:
    upd9002_rep_stosb,   // AA:
    upd9002_rep_stosw,   // AB:
    upd9002_rep_lodsb,   // AC:
    upd9002_rep_lodsw,   // AD:
    upd9002_repne_scasb, // AE:
    upd9002_repne_scasw, // AF:

    _mov_al_imm, // B0:
    _mov_cl_imm, // B1:
    _mov_dl_imm, // B2:
    _mov_bl_imm, // B3:
    _mov_ah_imm, // B4:
    _mov_ch_imm, // B5:
    _mov_dh_imm, // B6:
    _mov_bh_imm, // B7:
    _mov_ax_imm, // B8:
    _mov_cx_imm, // B9:
    _mov_dx_imm, // BA:
    _mov_bx_imm, // BB:
    _mov_sp_imm, // BC:
    _mov_bp_imm, // BD:
    _mov_si_imm, // BE:
    _mov_di_imm, // BF:

    _shift_ea8_data8,  // C0:
    _shift_ea16_data8, // C1:
    _ret_near_data16,  // C2:
    _ret_near,         // C3:
    _les_r16_ea,       // C4:
    _lds_r16_ea,       // C5:
    _mov_ea8_data8,    // C6:
    _mov_ea16_data16,  // C7:
    _enter,            // C8:
    fleave,            // C9:
    _ret_far_data16,   // CA:
    _ret_far,          // CB:
    _int_03,           // CC:
    _int_data8,        // CD:
    _into,             // CE:
    _iret,             // CF:

    _shift_ea8_1,   // D0:
    _shift_ea16_1,  // D1:
    _shift_ea8_cl,  // D2:
    _shift_ea16_cl, // D3:
    _aam,           // D4:
    _aad,           // D5:
    _xlat,          // D6:
    _xlat,          // D7:
    _esc,           // D8:
    _esc,           // D9:
    _esc,           // DA:
    _esc,           // DB:
    _esc,           // DC:
    _esc,           // DD:
    _esc,           // DE:
    _esc,           // DF:

    _loopnz,       // E0:
    _loopz,        // E1:
    _loop,         // E2:
    _jcxz,         // E3:
    _in_al_data8,  // E4:
    _in_ax_data8,  // E5:
    _out_data8_al, // E6:
    _out_data8_ax, // E7:
    _call_near,    // E8:
    _jmp_near,     // E9:
    _jmp_far,      // EA:
    _jmp_short,    // EB:
    _in_al_dx,     // EC:
    _in_ax_dx,     // ED:
    _out_dx_al,    // EE:
    _out_dx_ax,    // EF:

    _lock,    // F0:
    _lock,    // F1:
    _repne,   // F2:
    _repe,    // F3:
    _hlt,     // F4:
    _cmc,     // F5:
    _ope0xf6, // F6:
    _ope0xf7, // F7:
    _clc,     // F8:
    _stc,     // F9:
    _cli,     // FA:
    _sti,     // FB:
    _cld,     // FC:
    _std,     // FD:
    _ope0xfe, // FE:
    _ope0xff, // FF:
};

// ----------------------------------------------------------------- repnc

const UPD9002OP upd9002op_repnc[] = {
    _reserved_repnc, // 00:
    _reserved_repnc, // 01:
    _reserved_repnc, // 02:
    _reserved_repnc, // 03:
    _reserved_repnc, // 04:
    _reserved_repnc, // 05:
    _reserved_repnc, // 06:
    _reserved_repnc, // 07:
    _reserved_repnc, // 08:
    _reserved_repnc, // 09:
    _reserved_repnc, // 0A:
    _reserved_repnc, // 0B:
    _reserved_repnc, // 0C:
    _reserved_repnc, // 0D:
    _reserved_repnc, // 0E:
    _reserved_repnc, // 0F:

    _reserved_repnc, // 10:
    _reserved_repnc, // 11:
    _reserved_repnc, // 12:
    _reserved_repnc, // 13:
    _reserved_repnc, // 14:
    _reserved_repnc, // 15:
    _reserved_repnc, // 16:
    _reserved_repnc, // 17:
    _reserved_repnc, // 18:
    _reserved_repnc, // 19:
    _reserved_repnc, // 1A:
    _reserved_repnc, // 1B:
    _reserved_repnc, // 1C:
    _reserved_repnc, // 1D:
    _reserved_repnc, // 1E:
    _reserved_repnc, // 1F:

    _reserved_repnc,     // 20:
    _reserved_repnc,     // 21:
    _reserved_repnc,     // 22:
    _reserved_repnc,     // 23:
    _reserved_repnc,     // 24:
    _reserved_repnc,     // 25:
    _repnc_segprefix_es, // 26:
    _reserved_repnc,     // 27:
    _reserved_repnc,     // 28:
    _reserved_repnc,     // 29:
    _reserved_repnc,     // 2A:
    _reserved_repnc,     // 2B:
    _reserved_repnc,     // 2C:
    _reserved_repnc,     // 2D:
    _repnc_segprefix_cs, // 2E:
    _reserved_repnc,     // 2F:

    _reserved_repnc,     // 30:
    _reserved_repnc,     // 31:
    _reserved_repnc,     // 32:
    _reserved_repnc,     // 33:
    _reserved_repnc,     // 34:
    _reserved_repnc,     // 35:
    _repnc_segprefix_ss, // 36:
    _reserved_repnc,     // 37:
    _reserved_repnc,     // 38:
    _reserved_repnc,     // 39:
    _reserved_repnc,     // 3A:
    _reserved_repnc,     // 3B:
    _reserved_repnc,     // 3C:
    _reserved_repnc,     // 3D:
    _repnc_segprefix_ds, // 3E:
    _reserved_repnc,     // 3F:

    _reserved_repnc, // 40:
    _reserved_repnc, // 41:
    _reserved_repnc, // 42:
    _reserved_repnc, // 43:
    _reserved_repnc, // 44:
    _reserved_repnc, // 45:
    _reserved_repnc, // 46:
    _reserved_repnc, // 47:
    _reserved_repnc, // 48:
    _reserved_repnc, // 49:
    _reserved_repnc, // 4A:
    _reserved_repnc, // 4B:
    _reserved_repnc, // 4C:
    _reserved_repnc, // 4D:
    _reserved_repnc, // 4E:
    _reserved_repnc, // 4F:

    _reserved_repnc, // 50:
    _reserved_repnc, // 51:
    _reserved_repnc, // 52:
    _reserved_repnc, // 53:
    _reserved_repnc, // 54:
    _reserved_repnc, // 55:
    _reserved_repnc, // 56:
    _reserved_repnc, // 57:
    _reserved_repnc, // 58:
    _reserved_repnc, // 59:
    _reserved_repnc, // 5A:
    _reserved_repnc, // 5B:
    _reserved_repnc, // 5C:
    _reserved_repnc, // 5D:
    _reserved_repnc, // 5E:
    _reserved_repnc, // 5F:

    _reserved_repnc, // 60:
    _reserved_repnc, // 61:
    _reserved_repnc, // 62:
    _reserved_repnc, // 63:
    _repnc,          // 64:
    _repc,           // 65:
    _reserved_repnc, // 66:
    _reserved_repnc, // 67:
    _reserved_repnc, // 68:
    _reserved_repnc, // 69:
    _reserved_repnc, // 6A:
    _reserved_repnc, // 6B:
    _reserved_repnc, // 6C:
    _reserved_repnc, // 6D:
    _reserved_repnc, // 6E:
    _reserved_repnc, // 6F:

    _reserved_repnc, // 70:
    _reserved_repnc, // 71:
    _reserved_repnc, // 72:
    _reserved_repnc, // 73:
    _reserved_repnc, // 74:
    _reserved_repnc, // 75:
    _reserved_repnc, // 76:
    _reserved_repnc, // 77:
    _reserved_repnc, // 78:
    _reserved_repnc, // 79:
    _reserved_repnc, // 7A:
    _reserved_repnc, // 7B:
    _reserved_repnc, // 7C:
    _reserved_repnc, // 7D:
    _reserved_repnc, // 7E:
    _reserved_repnc, // 7F:

    _reserved_repnc, // 80:
    _reserved_repnc, // 81:
    _reserved_repnc, // 82:
    _reserved_repnc, // 83:
    _reserved_repnc, // 84:
    _reserved_repnc, // 85:
    _reserved_repnc, // 86:
    _reserved_repnc, // 87:
    _reserved_repnc, // 88:
    _reserved_repnc, // 89:
    _reserved_repnc, // 8A:
    _reserved_repnc, // 8B:
    _reserved_repnc, // 8C:
    _reserved_repnc, // 8D:
    _reserved_repnc, // 8E:
    _reserved_repnc, // 8F:

    _reserved_repnc, // 90:
    _reserved_repnc, // 91:
    _reserved_repnc, // 92:
    _reserved_repnc, // 93:
    _reserved_repnc, // 94:
    _reserved_repnc, // 95:
    _reserved_repnc, // 96:
    _reserved_repnc, // 97:
    _reserved_repnc, // 98:
    _reserved_repnc, // 99:
    _reserved_repnc, // 9A:
    _reserved_repnc, // 9B:
    _reserved_repnc, // 9C:
    _reserved_repnc, // 9D:
    _reserved_repnc, // 9E:
    _reserved_repnc, // 9F:

    _reserved_repnc,     // A0:
    _reserved_repnc,     // A1:
    _reserved_repnc,     // A2:
    _reserved_repnc,     // A3:
    upd9002_repnc_movsb, // A4:
    upd9002_repnc_movsw, // A5:
    upd9002_repnc_cmpsb, // A6:
    upd9002_repnc_cmpsw, // A7:
    _reserved_repnc,     // A8:
    _reserved_repnc,     // A9:
    upd9002_repnc_stosb, // AA:
    upd9002_repnc_stosw, // AB:
    upd9002_repnc_lodsb, // AC:
    upd9002_repnc_lodsw, // AD:
    upd9002_repnc_scasb, // AE:
    upd9002_repnc_scasw, // AF:

    _reserved_repnc, // B0:
    _reserved_repnc, // B1:
    _reserved_repnc, // B2:
    _reserved_repnc, // B3:
    _reserved_repnc, // B4:
    _reserved_repnc, // B5:
    _reserved_repnc, // B6:
    _reserved_repnc, // B7:
    _reserved_repnc, // B8:
    _reserved_repnc, // B9:
    _reserved_repnc, // BA:
    _reserved_repnc, // BB:
    _reserved_repnc, // BC:
    _reserved_repnc, // BD:
    _reserved_repnc, // BE:
    _reserved_repnc, // BF:

    _reserved_repnc, // C0:
    _reserved_repnc, // C1:
    _reserved_repnc, // C2:
    _reserved_repnc, // C3:
    _reserved_repnc, // C4:
    _reserved_repnc, // C5:
    _reserved_repnc, // C6:
    _reserved_repnc, // C7:
    _reserved_repnc, // C8:
    _reserved_repnc, // C9:
    _reserved_repnc, // CA:
    _reserved_repnc, // CB:
    _reserved_repnc, // CC:
    _reserved_repnc, // CD:
    _reserved_repnc, // CE:
    _reserved_repnc, // CF:

    _reserved_repnc, // D0:
    _reserved_repnc, // D1:
    _reserved_repnc, // D2:
    _reserved_repnc, // D3:
    _reserved_repnc, // D4:
    _reserved_repnc, // D5:
    _reserved_repnc, // D6:
    _reserved_repnc, // D7:
    _reserved_repnc, // D8:
    _reserved_repnc, // D9:
    _reserved_repnc, // DA:
    _reserved_repnc, // DB:
    _reserved_repnc, // DC:
    _reserved_repnc, // DD:
    _reserved_repnc, // DE:
    _reserved_repnc, // DF:

    _reserved_repnc, // E0:
    _reserved_repnc, // E1:
    _reserved_repnc, // E2:
    _reserved_repnc, // E3:
    _reserved_repnc, // E4:
    _reserved_repnc, // E5:
    _reserved_repnc, // E6:
    _reserved_repnc, // E7:
    _reserved_repnc, // E8:
    _reserved_repnc, // E9:
    _reserved_repnc, // EA:
    _reserved_repnc, // EB:
    _reserved_repnc, // EC:
    _reserved_repnc, // ED:
    _reserved_repnc, // EE:
    _reserved_repnc, // EF:

    _reserved_repnc, // F0:
    _reserved_repnc, // F1:
    _repne,          // F2:
    _repe,           // F3:
    _reserved_repnc, // F4:
    _reserved_repnc, // F5:
    _reserved_repnc, // F6:
    _reserved_repnc, // F7:
    _reserved_repnc, // F8:
    _reserved_repnc, // F9:
    _reserved_repnc, // FA:
    _reserved_repnc, // FB:
    _reserved_repnc, // FC:
    _reserved_repnc, // FD:
    _reserved_repnc, // FE:
    _reserved_repnc, // FF:
};

// ----------------------------------------------------------------- repc

const UPD9002OP upd9002op_repc[] = {
    _reserved_repc, // 00:
    _reserved_repc, // 01:
    _reserved_repc, // 02:
    _reserved_repc, // 03:
    _reserved_repc, // 04:
    _reserved_repc, // 05:
    _reserved_repc, // 06:
    _reserved_repc, // 07:
    _reserved_repc, // 08:
    _reserved_repc, // 09:
    _reserved_repc, // 0A:
    _reserved_repc, // 0B:
    _reserved_repc, // 0C:
    _reserved_repc, // 0D:
    _reserved_repc, // 0E:
    _reserved_repc, // 0F:

    _reserved_repc, // 10:
    _reserved_repc, // 11:
    _reserved_repc, // 12:
    _reserved_repc, // 13:
    _reserved_repc, // 14:
    _reserved_repc, // 15:
    _reserved_repc, // 16:
    _reserved_repc, // 17:
    _reserved_repc, // 18:
    _reserved_repc, // 19:
    _reserved_repc, // 1A:
    _reserved_repc, // 1B:
    _reserved_repc, // 1C:
    _reserved_repc, // 1D:
    _reserved_repc, // 1E:
    _reserved_repc, // 1F:

    _reserved_repc,     // 20:
    _reserved_repc,     // 21:
    _reserved_repc,     // 22:
    _reserved_repc,     // 23:
    _reserved_repc,     // 24:
    _reserved_repc,     // 25:
    _repc_segprefix_es, // 26:
    _reserved_repc,     // 27:
    _reserved_repc,     // 28:
    _reserved_repc,     // 29:
    _reserved_repc,     // 2A:
    _reserved_repc,     // 2B:
    _reserved_repc,     // 2C:
    _reserved_repc,     // 2D:
    _repc_segprefix_cs, // 2E:
    _reserved_repc,     // 2F:

    _reserved_repc,     // 30:
    _reserved_repc,     // 31:
    _reserved_repc,     // 32:
    _reserved_repc,     // 33:
    _reserved_repc,     // 34:
    _reserved_repc,     // 35:
    _repc_segprefix_ss, // 36:
    _reserved_repc,     // 37:
    _reserved_repc,     // 38:
    _reserved_repc,     // 39:
    _reserved_repc,     // 3A:
    _reserved_repc,     // 3B:
    _reserved_repc,     // 3C:
    _reserved_repc,     // 3D:
    _repc_segprefix_ds, // 3E:
    _reserved_repc,     // 3F:

    _reserved_repc, // 40:
    _reserved_repc, // 41:
    _reserved_repc, // 42:
    _reserved_repc, // 43:
    _reserved_repc, // 44:
    _reserved_repc, // 45:
    _reserved_repc, // 46:
    _reserved_repc, // 47:
    _reserved_repc, // 48:
    _reserved_repc, // 49:
    _reserved_repc, // 4A:
    _reserved_repc, // 4B:
    _reserved_repc, // 4C:
    _reserved_repc, // 4D:
    _reserved_repc, // 4E:
    _reserved_repc, // 4F:

    _reserved_repc, // 50:
    _reserved_repc, // 51:
    _reserved_repc, // 52:
    _reserved_repc, // 53:
    _reserved_repc, // 54:
    _reserved_repc, // 55:
    _reserved_repc, // 56:
    _reserved_repc, // 57:
    _reserved_repc, // 58:
    _reserved_repc, // 59:
    _reserved_repc, // 5A:
    _reserved_repc, // 5B:
    _reserved_repc, // 5C:
    _reserved_repc, // 5D:
    _reserved_repc, // 5E:
    _reserved_repc, // 5F:

    _reserved_repc, // 60:
    _reserved_repc, // 61:
    _reserved_repc, // 62:
    _reserved_repc, // 63:
    _repnc,         // 64:
    _repc,          // 65:
    _reserved_repc, // 66:
    _reserved_repc, // 67:
    _reserved_repc, // 68:
    _reserved_repc, // 69:
    _reserved_repc, // 6A:
    _reserved_repc, // 6B:
    _reserved_repc, // 6C:
    _reserved_repc, // 6D:
    _reserved_repc, // 6E:
    _reserved_repc, // 6F:

    _reserved_repc, // 70:
    _reserved_repc, // 71:
    _reserved_repc, // 72:
    _reserved_repc, // 73:
    _reserved_repc, // 74:
    _reserved_repc, // 75:
    _reserved_repc, // 76:
    _reserved_repc, // 77:
    _reserved_repc, // 78:
    _reserved_repc, // 79:
    _reserved_repc, // 7A:
    _reserved_repc, // 7B:
    _reserved_repc, // 7C:
    _reserved_repc, // 7D:
    _reserved_repc, // 7E:
    _reserved_repc, // 7F:

    _reserved_repc, // 80:
    _reserved_repc, // 81:
    _reserved_repc, // 82:
    _reserved_repc, // 83:
    _reserved_repc, // 84:
    _reserved_repc, // 85:
    _reserved_repc, // 86:
    _reserved_repc, // 87:
    _reserved_repc, // 88:
    _reserved_repc, // 89:
    _reserved_repc, // 8A:
    _reserved_repc, // 8B:
    _reserved_repc, // 8C:
    _reserved_repc, // 8D:
    _reserved_repc, // 8E:
    _reserved_repc, // 8F:

    _reserved_repc, // 90:
    _reserved_repc, // 91:
    _reserved_repc, // 92:
    _reserved_repc, // 93:
    _reserved_repc, // 94:
    _reserved_repc, // 95:
    _reserved_repc, // 96:
    _reserved_repc, // 97:
    _reserved_repc, // 98:
    _reserved_repc, // 99:
    _reserved_repc, // 9A:
    _reserved_repc, // 9B:
    _reserved_repc, // 9C:
    _reserved_repc, // 9D:
    _reserved_repc, // 9E:
    _reserved_repc, // 9F:

    _reserved_repc,     // A0:
    _reserved_repc,     // A1:
    _reserved_repc,     // A2:
    _reserved_repc,     // A3:
    upd9002_repc_movsb, // A4:
    upd9002_repc_movsw, // A5:
    upd9002_repc_cmpsb, // A6:
    upd9002_repc_cmpsw, // A7:
    _reserved_repc,     // A8:
    _reserved_repc,     // A9:
    upd9002_repc_stosb, // AA:
    upd9002_repc_stosw, // AB:
    upd9002_repc_lodsb, // AC:
    upd9002_repc_lodsw, // AD:
    upd9002_repc_scasb, // AE:
    upd9002_repc_scasw, // AF:

    _reserved_repc, // B0:
    _reserved_repc, // B1:
    _reserved_repc, // B2:
    _reserved_repc, // B3:
    _reserved_repc, // B4:
    _reserved_repc, // B5:
    _reserved_repc, // B6:
    _reserved_repc, // B7:
    _reserved_repc, // B8:
    _reserved_repc, // B9:
    _reserved_repc, // BA:
    _reserved_repc, // BB:
    _reserved_repc, // BC:
    _reserved_repc, // BD:
    _reserved_repc, // BE:
    _reserved_repc, // BF:

    _reserved_repc, // C0:
    _reserved_repc, // C1:
    _reserved_repc, // C2:
    _reserved_repc, // C3:
    _reserved_repc, // C4:
    _reserved_repc, // C5:
    _reserved_repc, // C6:
    _reserved_repc, // C7:
    _reserved_repc, // C8:
    _reserved_repc, // C9:
    _reserved_repc, // CA:
    _reserved_repc, // CB:
    _reserved_repc, // CC:
    _reserved_repc, // CD:
    _reserved_repc, // CE:
    _reserved_repc, // CF:

    _reserved_repc, // D0:
    _reserved_repc, // D1:
    _reserved_repc, // D2:
    _reserved_repc, // D3:
    _reserved_repc, // D4:
    _reserved_repc, // D5:
    _reserved_repc, // D6:
    _reserved_repc, // D7:
    _reserved_repc, // D8:
    _reserved_repc, // D9:
    _reserved_repc, // DA:
    _reserved_repc, // DB:
    _reserved_repc, // DC:
    _reserved_repc, // DD:
    _reserved_repc, // DE:
    _reserved_repc, // DF:

    _reserved_repc, // E0:
    _reserved_repc, // E1:
    _reserved_repc, // E2:
    _reserved_repc, // E3:
    _reserved_repc, // E4:
    _reserved_repc, // E5:
    _reserved_repc, // E6:
    _reserved_repc, // E7:
    _reserved_repc, // E8:
    _reserved_repc, // E9:
    _reserved_repc, // EA:
    _reserved_repc, // EB:
    _reserved_repc, // EC:
    _reserved_repc, // ED:
    _reserved_repc, // EE:
    _reserved_repc, // EF:

    _reserved_repc, // F0:
    _reserved_repc, // F1:
    _repne,         // F2:
    _repe,          // F3:
    _reserved_repc, // F4:
    _reserved_repc, // F5:
    _reserved_repc, // F6:
    _reserved_repc, // F7:
    _reserved_repc, // F8:
    _reserved_repc, // F9:
    _reserved_repc, // FA:
    _reserved_repc, // FB:
    _reserved_repc, // FC:
    _reserved_repc, // FD:
    _reserved_repc, // FE:
    _reserved_repc, // FF:
};

#if defined(VAEG_UPD9002_M46_TESTING)
int upd9002_dispatch_test_verify(void) {
	if ((upd9002op[0x0f] != _ope0x0f) || (upd9002op[0x64] != _repnc) ||
	    (upd9002op[0x65] != _repc) || (upd9002op[0x66] != _reserved_no_int) ||
	    (upd9002op[0xf2] != _repne) || (upd9002op[0xf3] != _repe) ||
	    (upd9002op_repe[0x0f] != _repe_0f_diagnostic_stop) ||
	    (upd9002op_repne[0x0f] != _repne_0f_diagnostic_stop) ||
	    (upd9002op_repnc[0xa4] != upd9002_repnc_movsb) ||
	    (upd9002op_repc[0xa4] != upd9002_repc_movsb) ||
	    (upd9002_ope0x0f_table[0x10] != _test1_ea8_cl)) {
		return (FAILURE);
	}
	return (SUCCESS);
}

void upd9002_dispatch_test_require_immutable(void) {
	if (upd9002_dispatch_test_verify() != SUCCESS) {
		abort();
	}
}

UINT upd9002_dispatch_test_construction_count(void) {
	return (0);
}

UINT upd9002_dispatch_test_rejected_count(void) {
	return (0);
}
#endif
