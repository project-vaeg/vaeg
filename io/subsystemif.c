/*
 * subsystemif.c: PC-88VA FD Sub System Interface
 */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"

#include "i8255.h"
#include "subsystem.h"
#include "subsystemif.h"

_SUBSYSTEMIF subsystemif;
static _I8255CFG i8255cfg;

#define SUBIF_TRACE_MAX 256

enum {
	SUBIF_MAIN_ATN = 0x80
};

static UINT8 subif_trace_main[SUBIF_TRACE_MAX];
static UINT subif_trace_main_len;
static UINT8 subif_trace_resp[SUBIF_TRACE_MAX];
static UINT subif_trace_resp_len;
static REG8 subif_trace_portb;
static REG8 subif_trace_portc;

// ----

static void subif_trace_emit(const char *dir, const UINT8 *data, UINT length) {
	char prefix[64];

	if (!length) {
		return;
	}
	(void)snprintf(prefix, sizeof(prefix), "subiftrace mode=%02x %s", fdc.fddifmode, dir);
	fdc_trace_bytes(prefix, data, length);
}

static void subif_trace_flush_main(void) {
	subif_trace_emit("main2sub", subif_trace_main, subif_trace_main_len);
	subif_trace_main_len = 0;
}

static void subif_trace_flush_resp(void) {
	subif_trace_emit("sub2main", subif_trace_resp, subif_trace_resp_len);
	subif_trace_resp_len = 0;
}

static void subif_trace_append_main(REG8 dat) {
	if (subif_trace_main_len >= SUBIF_TRACE_MAX) {
		subif_trace_flush_main();
	}
	subif_trace_main[subif_trace_main_len++] = dat;
}

static void subif_trace_append_resp(REG8 dat) {
	if (subif_trace_resp_len >= SUBIF_TRACE_MAX) {
		subif_trace_flush_resp();
	}
	subif_trace_resp[subif_trace_resp_len++] = dat;
}

static void subif_trace_set_portc(REG8 dat) {
	REG8 old;

	old = subif_trace_portc;
	subif_trace_portc = dat;
	if (!(old & SUBIF_MAIN_ATN) && (dat & SUBIF_MAIN_ATN)) {
		subif_trace_flush_resp();
		subif_trace_main_len = 0;
	}
	if ((old & SUBIF_MAIN_ATN) && !(dat & SUBIF_MAIN_ATN)) {
		subif_trace_flush_main();
	}
}

static void IOOUTCALL subsystemif_o0fc(UINT port, REG8 dat) {
	i8255_outporta(&i8255cfg, dat);
	(void)port;
}

static void IOOUTCALL subsystemif_o0fd(UINT port, REG8 dat) {
	subif_trace_portb = dat;
	subif_trace_append_main(dat);
	i8255_outportb(&i8255cfg, dat);
}

static void IOOUTCALL subsystemif_o0fe(UINT port, REG8 dat) {
	subif_trace_set_portc(dat);
	i8255_outportc(&i8255cfg, dat);
}

static void IOOUTCALL subsystemif_o0ff(UINT port, REG8 dat) {
	if (!(dat & 0x80)) {
		REG8 portc;
		REG8 mask;
		UINT bitnum;

		bitnum = (dat >> 1) & 0x07;
		mask = (REG8)(1 << bitnum);
		portc = subif_trace_portc;
		if (dat & 1) {
			portc |= mask;
		} else {
			portc &= (REG8)~mask;
		}
		subif_trace_set_portc(portc);
	}
	i8255_outctrl(&i8255cfg, dat);
}

static REG8 IOINPCALL subsystemif_i0fc(UINT port) {
	REG8 ret;

	ret = i8255_inporta(&i8255cfg);
	subif_trace_flush_main();
	subif_trace_append_resp(ret);
	return ret;
}

/* Port B input is used by the fast transfer protocol. */
static REG8 IOINPCALL subsystemif_i0fd(UINT port) {
	(void)port;
	return i8255_inportb(&i8255cfg);
}

static REG8 IOINPCALL subsystemif_i0fe(UINT port) {
	return i8255_inportc(&i8255cfg);
}

// ---- I/F (sub system)

void subsystemif_businporta(BYTE dat) {
	i8255_businporta(&i8255cfg, dat);
}

void subsystemif_businportb(BYTE dat) {
	i8255_businportb(&i8255cfg, dat);
}

void subsystemif_businportc(BYTE dat) {
	i8255_businportc(&i8255cfg, (BYTE)(dat >> 4));
}

// ---- I/F

void subsystemif_initialize(void) {
	i8255_init(&i8255cfg, &subsystemif.i8255);
	/* Cross-wired: main A <-> sub B, main B <-> sub A. */
	i8255cfg.busoutporta = subsystem_businportb;
	i8255cfg.busoutportb = subsystem_businporta;
	i8255cfg.busoutportc = subsystem_businportc;
}

void subsystemif_reset(void) {
	subif_trace_flush_main();
	subif_trace_flush_resp();
	subif_trace_main_len = 0;
	subif_trace_resp_len = 0;
	subif_trace_portb = 0;
	subif_trace_portc = 0;
	i8255_reset(&i8255cfg);
}

void subsystemif_bind(void) {
	iocore_attachout(0x0fc, subsystemif_o0fc);
	iocore_attachout(0x0fd, subsystemif_o0fd);
	iocore_attachout(0x0fe, subsystemif_o0fe);
	iocore_attachout(0x0ff, subsystemif_o0ff);

	iocore_attachinp(0x0fc, subsystemif_i0fc);
	iocore_attachinp(0x0fd, subsystemif_i0fd);
	iocore_attachinp(0x0fe, subsystemif_i0fe);
}
