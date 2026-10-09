/*
 * PC-88VA canonical CPU-visible I/O map.
 * Built-in VA devices register their exact ports here. C-bus devices use the
 * same map after their own lifecycle tier binds them; cbuscore is not a
 * second CPU I/O address space. The reset/build/bind order is deliberate.
 * Evidence: docs/agents/reports/m96_va_only_structural_cleanup.md, section 11.
 */

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "sgp.h"
#include "subsystemmx.h"
#include "va91.h"
#include "cmt.h"
#include "upd9002_regs.h"
#include "upd9002_trace.h"
#if defined(VAEG_UPD9002_SSTS_TESTING)
#include "tests/upd9002/direct_harness.h"
#endif

_DMAC dmac;
_EMSIO emsio;
_FDC fdc;
_KEYBRD keybrd;
_MOUSEIF mouseif;
_NP2SYSP np2sysp;
_PIC pic;
_PIT pit;
_RS232C rs232c;
_UPD4990 uPD4990;

/*
 * The V3 machine exposes one 16-bit I/O address space. Each high-byte entry
 * initially shares the default page and becomes private when a device binds a
 * port in that page.
 */
enum {
	IOFUNC_EXT = 0x01
};

typedef struct {
	IOOUT ioout[256];
	IOINP ioinp[256];
	UINT type;
	UINT port;
} _IOFUNC, *IOFUNC;

typedef struct {
	IOFUNC base[256];
	LISTARRAY iotbl;
} _IOMAP, *IOMAP;

typedef struct {
	_IOMAP map;
	UINT busclock;
} _IOCORE, *IOCORE;

static UINT8 iova_unhandled_out[0x2000];
static _IOCORE iocore;

static void trace_unhandled_out(UINT port) {
	UINT idx;
	UINT bit;

	port &= 0xffff;
	idx = port >> 3;
	bit = 1 << (port & 7);
	if (!(iova_unhandled_out[idx] & bit)) {
		iova_unhandled_out[idx] |= (UINT8)bit;
		fdc_trace_iova_unhandled(port);
	}
}

static void IOOUTCALL defout8(UINT port, REG8 dat) {
	trace_unhandled_out(port);
	TRACEOUT(("defout8 - %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
}

static REG8 IOINPCALL definp8(UINT port) {
	TRACEOUT(("definp8 - %x %.4x:%.4x", port, CPU_CS, CPU_IP));
	return (0xff);
}

static void attachout(IOFUNC iof, UINT port, IOOUT func) {
	if (func) {
		iof->ioout[port] = func;
	}
}

static void attachinp(IOFUNC iof, UINT port, IOINP func) {
	if (func) {
		iof->ioinp[port] = func;
	}
}

static IOFUNC getextiofunc(UINT port) {
	IOFUNC iof;
	IOMAP map;

	map = &iocore.map;
	iof = map->base[(port >> 8) & 0xff];
	if (!(iof->type & IOFUNC_EXT)) {
		iof = (IOFUNC)listarray_append(map->iotbl, iof);
		if (iof != NULL) {
			map->base[(port >> 8) & 0xff] = iof;
			iof->type |= IOFUNC_EXT;
			iof->port = port & 0xff00;
		}
	}
	return (iof);
}

BOOL iocore_attachout(UINT port, IOOUT func) {
	IOFUNC iof;

	iof = getextiofunc(port);
	if (iof) {
		attachout(iof, port & 0xff, func);
		return (SUCCESS);
	}
	return (FAILURE);
}

BOOL iocore_attachinp(UINT port, IOINP func) {
	IOFUNC iof;

	iof = getextiofunc(port);
	if (iof) {
		attachinp(iof, port & 0xff, func);
		return (SUCCESS);
	}
	return (FAILURE);
}

void iocore_create(void) {
	ZeroMemory(&iocore, sizeof(iocore));
	ZeroMemory(iova_unhandled_out, sizeof(iova_unhandled_out));
}

void iocore_destroy(void) {
	listarray_destroy(iocore.map.iotbl);
	iocore.map.iotbl = NULL;
}

BOOL iocore_build(void) {
	IOFUNC base;
	IOMAP map;
	LISTARRAY iotbl;
	int i;

	map = &iocore.map;
	listarray_destroy(map->iotbl);
	iotbl = listarray_new(sizeof(_IOFUNC), 32);
	map->iotbl = iotbl;
	if (iotbl == NULL) {
		return (FAILURE);
	}

	base = (IOFUNC)listarray_append(iotbl, NULL);
	if (base == NULL) {
		return (FAILURE);
	}
	for (i = 0; i < 256; i++) {
		base->ioout[i] = defout8;
		base->ioinp[i] = definp8;
		map->base[i] = base;
	}
	return (SUCCESS);
}

/*
 * Reset shared device state before VA-specific controllers consume it. The
 * binding table below exposes only native VA and separately owned expansion
 * interfaces.
 */
static const IOCBFN resetfn[] = {
    dmac_reset,         fdc_reset,       keyboard_reset, pic_reset,     rs232c_reset,
    uPD4990_reset,      itimer_reset,    mouseif_reset,  np2sysp_reset, emsio_reset,
    memctrlva_reset,    tsp_reset,       sgp_reset,      videova_reset, subsystemmx_reset,
    systemportva_reset, mouseifva_reset, gactrlva_reset, cgromva_reset, va91_reset,
    upd9002_regs_reset, cmt_reset,
};

static const IOCBFN bindfn[] = {
    dmac_bind,     fdc_bind,     keyboard_bind,    pic_bind,          rs232c_bind,
    itimer_bind,   np2sysp_bind, emsio_bind,       memctrlva_bind,    tsp_bind,
    sgp_bind,      videova_bind, subsystemmx_bind, systemportva_bind, mouseifva_bind,
    gactrlva_bind, cgromva_bind, va91_bind,        upd9002_regs_bind,
};

void iocore_cb(const IOCBFN *cbfn, UINT count) {
	while (count--) {
		(*cbfn)();
		cbfn++;
	}
}

void iocore_reset(void) {
	iocore_cb(resetfn, sizeof(resetfn) / sizeof(IOCBFN));
}

void iocore_bind(void) {
	iocore.busclock = pccore.multiple;
	iocore_cb(bindfn, sizeof(bindfn) / sizeof(IOCBFN));
}

void IOOUTCALL iocore_out8(UINT port, REG8 dat) {
	IOFUNC iof;

#if defined(VAEG_UPD9002_SSTS_TESTING)
	if (upd9002_ssts_io_active()) {
		upd9002_ssts_io_write((uint16_t)port, (uint8_t)dat);
		return;
	}
#endif

	upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "io-write", (uint32_t)port, (uint32_t)dat, 1);
	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.map.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, dat);
}

REG8 IOINPCALL iocore_inp8(UINT port) {
	IOFUNC iof;
	REG8 ret;

#if defined(VAEG_UPD9002_SSTS_TESTING)
	if (upd9002_ssts_io_active()) {
		return upd9002_ssts_io_read((uint16_t)port);
	}
#endif

	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.map.base[(port >> 8) & 0xff];
	ret = iof->ioinp[port & 0xff](port);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "io-read", (uint32_t)port, (uint32_t)ret, 1);
	return (ret);
}

/*
 * V3 word-capable ports occupy adjacent byte addresses in Tekumani. Preserve
 * the native VA low-byte-then-high-byte dispatch, including a 00FFH-to-0100H
 * page crossing.
 */
void IOOUTCALL iocore_out16(UINT port, REG16 dat) {
	IOFUNC iof;

#if defined(VAEG_UPD9002_SSTS_TESTING)
	if (upd9002_ssts_io_active()) {
		upd9002_ssts_io_write((uint16_t)port, (uint8_t)dat);
		upd9002_ssts_io_write((uint16_t)(port + 1), (uint8_t)(dat >> 8));
		return;
	}
#endif

	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.map.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, (UINT8)dat);
	port++;
	iof = iocore.map.base[(port >> 8) & 0xff];
	iof->ioout[port & 0xff](port, (UINT8)(dat >> 8));
}

REG16 IOINPCALL iocore_inp16(UINT port) {
	IOFUNC iof;
	REG8 ret;

#if defined(VAEG_UPD9002_SSTS_TESTING)
	if (upd9002_ssts_io_active()) {
		REG16 low;

		low = upd9002_ssts_io_read((uint16_t)port);
		return (REG16)(low | (upd9002_ssts_io_read((uint16_t)(port + 1)) << 8));
	}
#endif

	CPU_REMCLOCK -= iocore.busclock;
	iof = iocore.map.base[(port >> 8) & 0xff];
	ret = iof->ioinp[port & 0xff](port);
	port++;
	iof = iocore.map.base[(port >> 8) & 0xff];
	return ((UINT16)((iof->ioinp[port & 0xff](port) << 8) + ret));
}

void IOOUTCALL iocore_out32(UINT port, UINT32 dat) {
	CPU_REMCLOCK -= iocore.busclock;
	iocore_out16(port, (UINT16)dat);
	iocore_out16(port + 2, (UINT16)(dat >> 16));
}

UINT32 IOINPCALL iocore_inp32(UINT port) {
	UINT32 ret;

	CPU_REMCLOCK -= iocore.busclock;
	ret = iocore_inp16(port);
	return (ret + (iocore_inp16(port + 2) << 16));
}
