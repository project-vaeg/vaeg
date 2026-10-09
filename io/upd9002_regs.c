/*
 * upd9002_regs.c: PC-88VA CPU port
 */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "upd9002_regs.h"

UPD9002_REGS upd9002_regs = {0};
UPD9002_IOTRAP upd9002_iotrap = {0};

static void IOOUTCALL upd9002_iotrap_range_out(UINT port, REG8 dat) {
	upd9002_iotrap.ranges[port - 0xffe0] = (BYTE)dat;
}

static void IOOUTCALL upd9002_iotrap_control_out(UINT port, REG8 dat) {
	upd9002_iotrap.control = (BYTE)dat;
	(void)port;
}

static void IOOUTCALL upd9002_offf0(UINT port, REG8 dat) {
	upd9002_regs.tcks = dat;
	pit_ontckschanged();
}

static REG8 IOINPCALL upd9002_ifff0(UINT port) {
	(void)port;
	return (upd9002_regs.tcks);
}

// ---- I/F

void upd9002_regs_reset(void) {
	ZeroMemory(&upd9002_regs, sizeof(upd9002_regs));
	ZeroMemory(&upd9002_iotrap, sizeof(upd9002_iotrap));
}

void upd9002_regs_bind(void) {
	UINT port;
	for (port = 0xffe0; port <= 0xffe7; port++) {
		iocore_attachout(port, upd9002_iotrap_range_out);
	}
	iocore_attachout(0xffef, upd9002_iotrap_control_out);
	iocore_attachout(0xfff0, upd9002_offf0);
	iocore_attachinp(0xfff0, upd9002_ifff0);
}
