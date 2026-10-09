#include "compiler.h"
#include "cpucore.h"
#include "commng.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "cmt.h"
#include "machine/keystat.h"

#if defined(SUPPORT_OPRECORD)
#include "oprecord.h"
#endif

/*
  ToDo: 
	Port 197H keyboard-sub-CPU commands; only the operation-class RESET command is implemented.
*/

#include "iocoreva.h"

/*
Map scan codes to key-matrix port index (high nibble) and bit index (low nibble).
*/
static UINT8 scantomap[0x80] = {
    // ESC,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   -,   ^,   \,  BS, TAB,
    0x97,
    0x61,
    0x62,
    0x63,
    0x64,
    0x65,
    0x66,
    0x67,
    0x70,
    0x71,
    0x60,
    0x57,
    0x56,
    0x54,
    0xc5,
    0xa0,
    //   Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   @,   [, RET,   A,   S,   D,
    0x41,
    0x47,
    0x25,
    0x42,
    0x44,
    0x51,
    0x45,
    0x31,
    0x37,
    0x40,
    0x20,
    0x53,
    0xe0,
    0x21,
    0x43,
    0x24,
    //   F,   G,   H,   J,   K,   L,   ;,   :,   ],   Z,   X,   C,   V,   B,   N,   M,
    0x26,
    0x27,
    0x30,
    0x32,
    0x33,
    0x34,
    0x73,
    0x72,
    0x55,
    0x52,
    0x50,
    0x23,
    0x46,
    0x22,
    0x36,
    0x35,
    //   <,   >,   /,   _, SPC,CONV, RUP,RDOWN,INS, DEL,  UP,LEFT,RIGHT,DOWN, CLR,HELP,
    0x74,
    0x75,
    0x76,
    0x77,
    0xf6,
    0xd0,
    0xb0,
    0xb1,
    0xc6,
    0xc7,
    0x81,
    0xa2,
    0x82,
    0xa1,
    0x80,
    0xa3,
    //   -,   /,   7,   8,   9,   *,   4,   5,   6,   +,   1,   2,   3,   =,   0,   ,,
    0xa5,
    0xa6,
    0x07,
    0x10,
    0x11,
    0x12,
    0x04,
    0x05,
    0x06,
    0x13,
    0x01,
    0x02,
    0x03,
    0x14,
    0x00,
    0x15,
    //   .,DECIDE,
    0x16,
    0xd1,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    //STOP,COPY,  F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9, F10
    0x90,
    0xa4,
    0xf1,
    0xf2,
    0xf3,
    0xf4,
    0xf5,
    0xc0,
    0xc1,
    0xc2,
    0xc3,
    0xc4,
    0xff,
    0xff,
    0xff,
    0xff,
    //LSFT,CAPS,KANA,GRPH,CTRL,    ,    ,    ,RSFT, RET,  PC,FULLWIDTH
    0xe2,
    0xa7,
    0x85,
    0x84,
    0x87,
    0xff,
    0xff,
    0xff,
    0xe3,
    0xe1,
    0xd2,
    0xd3,
    0xff,
    0xff,
    0xff,
    0xff,
};

/*
	The following matrix positions are emulator-internal and cannot be read through I/O.
	0xf1-0xf5: F1-F5
	0xf6     : SPACE
*/

static void setmapbit(UINT8 pos, BOOL brkflag) {
	UINT8 bit;
	UINT8 port;

	if (pos == 0xff)
		return;

	bit = (1 << (pos & 0x07));
	port = pos >> 4;
	if (brkflag) {
		// break
		keybrd.keymap[port] |= bit;
	} else {
		// make
		keybrd.keymap[port] &= ~bit;
	}
}

static void updatekeymap(UINT8 scancode) {
	UINT8 pos;
	BOOL brkflg;

	pos = scantomap[scancode & 0x7f];
	brkflg = scancode & 0x80;
	setmapbit(pos, brkflg);

	// SHIFT
	setmapbit(0x86, !((~keybrd.keymap[0x0e] & 0x0c) | // SFT RT, SFT LT
	                  (~keybrd.keymap[0x0c] & 0x5f)   // INS, F10-F6
	                  ));

	// INSDEL
	setmapbit(0x83, !(~keybrd.keymap[0x0c] & 0xe0)); // INS,DEL,BS

	// RETURN
	setmapbit(0x17, !(~keybrd.keymap[0x0e] & 0x03)); // RET 10, RET FK

	// SPACES
	setmapbit(0x96, !((~keybrd.keymap[0x0f] & 0x40) | // SPACE
	                  (~keybrd.keymap[0x0d] & 0x03)   // DECIDE and CONVERT
	                  ));

	// F1-F5
	keybrd.keymap[0x09] =
	    (keybrd.keymap[0x09] & 0xc1) | ((keybrd.keymap[0x0c] << 1) & keybrd.keymap[0x0f] & 0x3e);
}

/*
 * Key-matrix pacing. V1/V2 software reads the matrix (ports 00h-0Eh) once
 * per VRTC, but the frontend can press and release a key within one host
 * event (US-layout taps and chords). A release is therefore applied only
 * after the key has been visible across KB_MATRIX_HOLD VRTC boundaries;
 * later events wait behind it so the order is kept. Events that need no
 * delay are applied at once, as before. The FIFO to the keyboard
 * controller (V3) is not affected. Transient input state, not saved.
 */
enum {
	KB_MATRIX_QUEUE = 64,
	KB_MATRIX_HOLD = 2
};

static UINT8 matrix_queue[KB_MATRIX_QUEUE];
static UINT matrix_queued;
static UINT8 matrix_tick;
static UINT8 matrix_made_tick[0x80];
static UINT8 matrix_held[0x80 / 8];

static BOOL matrix_release_waits(UINT8 data) {
	const UINT code = data & 0x7f;
	return (data & 0x80) && (matrix_held[code >> 3] & (1 << (code & 7))) &&
	       ((UINT8)(matrix_tick - matrix_made_tick[code]) < KB_MATRIX_HOLD);
}

static void updatekeymap(UINT8 scancode);

static void matrix_apply(UINT8 data) {
	const UINT code = data & 0x7f;
	if (data & 0x80) {
		matrix_held[code >> 3] &= (UINT8) ~(1 << (code & 7));
	} else {
		matrix_held[code >> 3] |= (UINT8)(1 << (code & 7));
		matrix_made_tick[code] = matrix_tick;
	}
	updatekeymap(data);
}

static void matrix_flush(void) {
	UINT i;
	for (i = 0; i < matrix_queued; i++) {
		matrix_apply(matrix_queue[i]);
	}
	matrix_queued = 0;
}

static void matrix_send(UINT8 data) {
	if ((matrix_queued == 0) && !matrix_release_waits(data)) {
		matrix_apply(data);
		return;
	}
	if (matrix_queued >= KB_MATRIX_QUEUE) {
		matrix_flush(); /* never lose an event; give up pacing instead */
		matrix_apply(data);
		return;
	}
	matrix_queue[matrix_queued++] = data;
}

void keyboard_matrix_tick(void) {
	UINT done;

	matrix_tick++;
	for (done = 0; done < matrix_queued; done++) {
		if (matrix_release_waits(matrix_queue[done])) {
			break;
		}
		matrix_apply(matrix_queue[done]);
	}
	if (done) {
		memmove(matrix_queue, matrix_queue + done, matrix_queued - done);
		matrix_queued -= done;
	}
}

static REG8 convertmodeldependent(REG8 data) {
	REG8 code;

	code = data & 0x7f;
	if (code >= 0x52) {
		if (code < 0x58) {
			return 0xff;
		} else if (code < 0x5c) {
			code += 0x20; // Right SHIFT, keypad RETURN, PC, and FULLWIDTH.
		} else if (code < 0x60) {
			return 0xff;
		} else if (code >= 0x75) {
			return 0xff;
		}
		data = (data & 0x80) | code;
	}
	return data;
}

// ---- Keyboard

void keyboard_callback(NEVENTITEM item) {
	if (item->flag & NEVENT_SETEVENT) {
		if ((keybrd.ctrls) || (keybrd.buffers)) {
			if (!(keybrd.status & 2)) {
				keybrd.status |= 2;
				if (keybrd.ctrls) {
					keybrd.ctrls--;
					keybrd.data = keybrd.ctr[keybrd.ctrpos];
					keybrd.ctrpos = (keybrd.ctrpos + 1) & KB_CTRMASK;
				} else if (keybrd.buffers) {
					keybrd.buffers--;
					keybrd.data = keybrd.buf[keybrd.bufpos];
					keybrd.bufpos = (keybrd.bufpos + 1) & KB_BUFMASK;
					//updatekeymap(keybrd.data);
				}
				//TRACEOUT(("recv -> %02x", keybrd.data));
			}
			pic_setirq(1);
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock, keyboard_callback, NEVENT_RELATIVE);
		}
	}
}

static REG8 IOINPCALL keyboard_i41(UINT port) {
	(void)port;
	keybrd.status &= ~2;
	pic_resetirq(1);
	//	TRACEOUT(("in41 -> %02x %.4x:%.8x", keybrd.data, CPU_CS, CPU_IP));
	return (keybrd.data);
}

static REG8 IOINPCALL keyboardva_i000(UINT port) {
	(void)port;
	return (keybrd.keymap[port]);
}

static void IOOUTCALL keyboardva_o197(UINT port, REG8 dat) {
	TRACEOUT(("keyboard: o197 -> %02x %.4x:%.4x", dat, CPU_CS, CPU_IP));
	if ((dat & 0x40) == 0x40) {
		/* Tekumani shows bit 7 set for both command classes, while the VA ROM
         * writes 01b and 10b in bits 7-6. Ignore bit 7 and use bit 6 as the
         * class selector. */
		// Mode command; its option bits are not implemented.
	} else {
		// Operation command.
		if (dat & 0x01) {
			// RESET
			keyboard_resetsignal();
		}
	}

	(void)port;
}

// ----

void keyboard_reset(void) {
	UINT8 mapbkup[KB_MAP];

	// Preserve the host-maintained key matrix across a device reset.
	matrix_flush();
	CopyMemory(mapbkup, keybrd.keymap, sizeof(mapbkup));

	ZeroMemory(&keybrd, sizeof(keybrd));
	keybrd.data = 0xff;
	keybrd.mode = 0x5e;

	CopyMemory(keybrd.keymap, mapbkup, sizeof(keybrd.keymap));
	/*
	{
		int i;

		for (i = 0; i < KB_MAP; i++) {
			keybrd.keymap[i] = 0xff;
		}
	}
	*/
}

void keyboard_bind(void) {
	keystat_ctrlreset();
	keybrd.xferclock = pccore.realclock / 1920;

	{
		int i;
		for (i = 0; i < 0x0f; i++) {
			iocore_attachinp(i, keyboardva_i000);
		}
		iocore_attachinp(0x1c1, keyboard_i41);
		iocore_attachout(0x197, keyboardva_o197);
	}
}

void keyboard_resetsignal(void) {
	int i;

	for (i = 0; i < KB_MAP; i++) {
		keybrd.keymap[i] = 0xff;
	}

	nevent_reset(NEVENT_KEYBOARD);
	keybrd.cmd = 0;
	keybrd.status = 0;
	keybrd.ctrls = 0;
	keybrd.buffers = 0;
	keystat_ctrlreset();
	keystat_resendstat();
}

void keyboard_ctrl(REG8 data) {
	if ((data == 0xfa) || (data == 0xfc)) {
		keybrd.ctrls = 0;
	}
	if (keybrd.ctrls < KB_CTR) {
		keybrd.ctr[(keybrd.ctrpos + keybrd.ctrls) & KB_CTRMASK] = data;
		keybrd.ctrls++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock, keyboard_callback, NEVENT_ABSOLUTE);
		}
	}
}

void keyboard_send(REG8 data) {
#if defined(SUPPORT_OPRECORD)
	oprecord_record_key(data);
#endif

	data = convertmodeldependent(data);
	if (data == 0xff)
		return;
	matrix_send(data);

	if (keybrd.buffers < KB_BUF) {
		keybrd.buf[(keybrd.bufpos + keybrd.buffers) & KB_BUFMASK] = data;
		keybrd.buffers++;
		if (!nevent_iswork(NEVENT_KEYBOARD)) {
			nevent_set(NEVENT_KEYBOARD, keybrd.xferclock, keyboard_callback, NEVENT_ABSOLUTE);
		}
	} else {
		keybrd.status |= 0x10;
	}
}

// ---- RS-232C

COMMNG cm_rs232c;

void rs232c_construct(void) {
	cm_rs232c = NULL;
}

void rs232c_destruct(void) {
	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
}

void rs232c_open(void) {
	if (cm_rs232c == NULL) {
		cm_rs232c = commng_create(COMCREATE_SERIAL);
		cm_rs232c->msg(cm_rs232c, COMMSG_SETRSFLAG, rs232c.cmd & 0x22); /* RTS, DTR */
	}
}

void rs232c_callback(void) {
	BOOL interrupt;

	interrupt = FALSE;
	if ((cm_rs232c) && (cm_rs232c->read(cm_rs232c, &rs232c.data))) {
		rs232c.result |= 2;
		/* 8214 mode: RXRDY is level 0; E6h bit 2 masks it. */
		pic8214_request(PIC8214_RXRDY);
		if (sysportva.c & 1) {
			interrupt = TRUE;
		}
	} else {
		rs232c.result &= (BYTE)~2;
	}
	if (sysportva.c & 4) {
		if (rs232c.send) {
			rs232c.send = 0;
			interrupt = TRUE;
		}
	}
	if (interrupt) {
		pic_setirq(4);
	}
}

BYTE rs232c_stat(void) {
	if (cm_rs232c == NULL) {
		rs232c_open();
	}
	return (cm_rs232c->getstat(cm_rs232c));
}

void rs232c_midipanic(void) {
	if (cm_rs232c) {
		cm_rs232c->msg(cm_rs232c, COMMSG_MIDIRESET, 0);
	}
}

// ----

static void IOOUTCALL rs232c_o30(UINT port, REG8 dat) {
	if (cmt_selected()) {
		cmt_write(dat);
		return;
	}
	if (cm_rs232c) {
		cm_rs232c->write(cm_rs232c, (UINT8)dat);
	}
	if (sysportva.c & 4) {
		rs232c.send = 0;
		pic_setirq(4);
	} else {
		rs232c.send = 1;
	}
	(void)port;
}

static void IOOUTCALL rs232c_o32(UINT port, REG8 dat) {
	if (!(dat & 0xfd)) {
		rs232c.dummyinst++;
	} else {
		if ((rs232c.dummyinst >= 3) && (dat == 0x40)) {
			rs232c.pos = 0;
		}
		rs232c.dummyinst = 0;
	}
	switch (rs232c.pos) {
	case 0x00: // reset
		rs232c.pos++;
		break;

	case 0x01: // mode
		if (!(dat & 0x03)) {
			rs232c.mul = 10 * 16;
		} else {
			rs232c.mul = ((dat >> 1) & 6) + 10;
			if (dat & 0x10) {
				rs232c.mul += 2;
			}
			switch (dat & 0xc0) {
			case 0x80:
				rs232c.mul += 3;
				break;
			case 0xc0:
				rs232c.mul += 4;
				break;
			default:
				rs232c.mul += 2;
				break;
			}
			switch (dat & 0x03) {
			case 0x01:
				rs232c.mul >>= 1;
				break;
			case 0x03:
				rs232c.mul *= 32;
				break;
			default:
				rs232c.mul *= 8;
				break;
			}
		}
		rs232c.pos++;
		break;

	case 0x02: // cmd
		rs232c.cmd = dat;
		cmt_command(dat);
		if (cm_rs232c) {
			cm_rs232c->msg(cm_rs232c, COMMSG_SETRSFLAG, dat & 0x22); /* RTS, DTR */
		}
		break;
	}
	(void)port;
}

static REG8 IOINPCALL rs232c_i30(UINT port) {
	(void)port;
	if (cmt_selected()) {
		return cmt_read();
	}
	return (rs232c.data);
}

static REG8 IOINPCALL rs232c_i32(UINT port) {
	if (cmt_selected()) {
		return cmt_status(rs232c.cmd & 0x04);
	}
	if (!(rs232c_stat() & 0x20)) {
		return (rs232c.result | 0x80);
	} else {
		(void)port;
		return (rs232c.result);
	}
}

// ----

void rs232c_reset(void) {
	commng_destroy(cm_rs232c);
	cm_rs232c = NULL;
	rs232c.result = 0x05;
	rs232c.data = 0xff;
	rs232c.send = 1;
	rs232c.pos = 0;
	rs232c.dummyinst = 0;
	rs232c.mul = 10 * 16;
	rs232c.cmd = 0;
}

void rs232c_bind(void) {
	iocore_attachout(0x020, rs232c_o30);
	iocore_attachout(0x021, rs232c_o32);
	iocore_attachinp(0x020, rs232c_i30);
	iocore_attachinp(0x021, rs232c_i32);
}
