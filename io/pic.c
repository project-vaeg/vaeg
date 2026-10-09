#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "scsiio.h"

#include "iocoreva.h"

enum {
	PIC_OCW2_L = 0x07,
	PIC_OCW2_EOI = 0x20,
	PIC_OCW2_SL = 0x40,
	PIC_OCW2_R = 0x80,

	PIC_OCW3_RIS = 0x01,
	PIC_OCW3_RR = 0x02,
	PIC_OCW3_P = 0x04,
	PIC_OCW3_SMM = 0x20,
	PIC_OCW3_ESMM = 0x40
};

enum {
	PIC_ICW4_AEOI = 0x02
};

static const _PICITEM def_master = {{0x11, 0x08, 0x80, 0x1d}, 0x7d, 0, 0, 0, 0, 0};

static const _PICITEM def_slave = {{0x11, 0x10, 0x07, 0x09}, 0x71, 0, 0, 0, 0, 0};

/* The historical PIC state image stores the controller registers verbatim.
 * Keep the external input levels separate so adding level-aware NDP routing
 * does not change that compatibility image. */
static REG8 pic_level_state[2];

static void pic_apply_level_requests(void) {
	unsigned controller;

	for (controller = 0; controller < 2; controller++) {
		if (pic.pi[controller].icw[0] & 0x08) {
			pic.pi[controller].irr |= pic_level_state[controller];
		}
	}
}

// ---- 8214 mode (V1/V2)

_PIC8214 pic8214;

/* E6h mask bits of the maskable levels 0-2 (BNN manual, port 00E6H). */
static const REG8 pic8214_maskbit[3] = {0x04, 0x02, 0x01};

/*
 * Highest-priority request (lowest level) that the controller would offer,
 * or -1. Intel uPD8214 rule, consistent with the VA manual's E4h
 * description: after acceptance nothing is offered until E4h is written
 * again; with XSGS clear a request must be above the current status.
 */
static int pic8214_select(void) {
	int level;

	if (!pic8214.inte || (pic8214.pending != PIC8214_NONE)) {
		return -1;
	}
	for (level = 0; level < 8; level++) {
		if (pic8214.irr & (1 << level)) {
			if ((pic8214.status & 0x08) || (level < (pic8214.status & 0x07))) {
				return level;
			}
			return -1;
		}
	}
	return -1;
}

/* The 8214 INT output is the ICU master's IR7 request line. */
static void pic8214_update(void) {
	if (pic8214.mode8259) {
		return;
	}
	if (pic8214_select() >= 0) {
		pic.pi[0].irr |= PIC_SLAVE;
	} else {
		pic.pi[0].irr &= (REG8)~PIC_SLAVE;
	}
}

static void pic8214_accept(int level) {
	pic8214.irr &= (REG8) ~(1 << level);
	pic8214.inte = 0;
	pic8214_update();
}

void pic8214_request(REG8 level) {
	if (pic8214.mode8259) {
		return;
	}
	level &= 7;
	if ((level <= 2) && !(pic8214.mask & pic8214_maskbit[level])) {
		return;
	}
	pic8214.irr |= (REG8)(1 << level);
	pic8214_update();
}

REG8 pic8214_acknowledge_compat(void) {
	int level;

	level = pic8214.pending;
	if (level == PIC8214_NONE) {
		return 0xff; /* no pending level: the idle data bus */
	}
	pic8214.pending = PIC8214_NONE;
	pic8214_accept(level);
	return (REG8)(level << 1);
}

/* 8259-mode IR numbers that are also 8214 levels (BNN manual 5.2.1/5.2.2). */
static int pic8214_level_of(REG8 irq) {
	switch (irq) {
	case 0x02:
		return PIC8214_VRTC;
	case 0x03:
		return 3; /* UINT0 */
	case 0x05:
		return 5; /* UINT1 */
	case 0x08:
		return PIC8214_SGP;
	case 0x0c:
		return PIC8214_SOUND;
	case 0x0d:
		return PIC8214_TIMER3;
	default:
		return -1;
	}
}

/* Deliver the 8214 request offered on master IR7 (cascade line `bit`). */
static void pic8214_deliver(REG8 bit) {
	int level;

	level = pic8214_select();
	pic.pi[0].irr &= (REG8)~bit;
	if (level < 0) {
		return;
	}
	if (!(pic.pi[0].icw[3] & PIC_ICW4_AEOI)) {
		pic.pi[0].isr |= bit;
	}
	if (CPU_COMPAT_MODE == UPD9002_COMPAT_UPD70008) {
		/* uPD780 vector: offered now, taken by the compatible core's next
		 * interrupt acknowledge (pic8214_acknowledge_compat). */
		pic8214.pending = (UINT8)level;
		upd9002_core_compat_irq(TRUE);
	} else {
		pic8214_accept(level);
		CPU_INTERRUPT((REG8)(0x40 + level), 0);
	}
}

static UINT32 pic8214_timer2_period(void) {
	return pccore.realclock / 600;
}

/*
 * The event runs only while E6h enables the level: the interrupt is not
 * observable otherwise, and an idle timer must not perturb V3 scheduling.
 * Only the phase of the first tick after enabling is a modelling choice.
 */
void pic8214_timer2(NEVENTITEM item) {
	if (pic8214.mode8259 || !(pic8214.mask & pic8214_maskbit[PIC8214_TIMER2])) {
		return;
	}
	if (item->flag & NEVENT_SETEVENT) {
		pic8214_request(PIC8214_TIMER2);
	}
	nevent_set(NEVENT_GENTIMER2, (SINT32)pic8214_timer2_period(), pic8214_timer2, NEVENT_RELATIVE);
}

static void IOOUTCALL pic8214_oe4(UINT port, REG8 dat) {
	pic8214.status = dat & 0x0f;
	pic8214.inte = 1;
	pic8214_update();
	nevent_forceexit();
	(void)port;
}

static void IOOUTCALL pic8214_oe6(UINT port, REG8 dat) {
	pic8214.mask = dat & 0x07;
	if (!pic8214.mode8259 && (pic8214.mask & pic8214_maskbit[PIC8214_TIMER2]) &&
	    !nevent_iswork(NEVENT_GENTIMER2)) {
		nevent_set(NEVENT_GENTIMER2, (SINT32)pic8214_timer2_period(), pic8214_timer2,
		           NEVENT_ABSOLUTE);
	}
	(void)port;
}

void pic_select_8259_mode(void) {
	if (!pic8214.mode8259) {
		pic8214.mode8259 = 1;
		pic8214.irr = 0;
		pic8214.inte = 0;
		pic8214.pending = PIC8214_NONE;
		pic.pi[0].irr &= (REG8)~PIC_SLAVE;
		if (nevent_iswork(NEVENT_GENTIMER2)) {
			nevent_reset(NEVENT_GENTIMER2);
		}
	}
}

static void IOOUTCALL pic8214_o158(UINT port, REG8 dat) {
	pic_select_8259_mode();
	(void)port;
	(void)dat;
}

// ----

#if 0 // Disabled implementation: slave arbitration is incorrect.
void pic_irq(void) {
	PIC		p;
	REG8	mir;
	REG8	sir;
	REG8	dat;
	REG8	num;
	REG8	bit;
	REG8	slave;

	// Do not arbitrate while maskable interrupts are disabled.
	if (!CPU_isEI) {
		return;
	}
	p = &pic;

	mir = p->pi[0].irr & (~p->pi[0].imr);
	sir = p->pi[1].irr & (~p->pi[1].imr);
	if ((mir == 0) && (sir == 0)) {
		return;
	}

	slave = 1 << (p->pi[1].icw[2] & 7);
	dat = mir;
	if (sir) {
		dat |= slave & (~p->pi[0].imr);
	}
	if (!(p->pi[0].ocw3 & PIC_OCW3_SMM)) {
		dat |= p->pi[0].isr;
	}
	num = p->pi[0].pry;
	bit = 1 << num;
	while(!(dat & bit)) {
		num = (num + 1) & 7;
		bit = 1 << num;
	}
	if (p->pi[0].icw[2] & bit) {					// Slave cascade.
		dat = sir;
		if (!(p->pi[1].ocw3 & PIC_OCW3_SMM)) {
			dat |= p->pi[1].isr;
		}
		num = p->pi[1].pry;
		bit = 1 << num;
		while(!(dat & bit)) {
			num = (num + 1) & 7;
			bit = 1 << num;
		}
		if (!(p->pi[1].isr & bit)) {
			p->pi[0].isr |= slave;
			p->pi[0].irr &= ~slave;
			p->pi[1].isr |= bit;
			p->pi[1].irr &= ~bit;
			TRACEOUT(("hardware-int %.2x", (p->pi[1].icw[1] & 0xf8) | num));
			CPU_INTERRUPT((REG8)((p->pi[1].icw[1] & 0xf8) | num), 0);
		}
	}
	else if (!(p->pi[0].isr & bit)) {				// Master request.
		p->pi[0].isr |= bit;
		p->pi[0].irr &= ~bit;
		if (num == 0) {
			nevent_reset(NEVENT_PICMASK);
		}
		TRACEOUT(("hardware-int %.2x [%.4x:%.4x]", (p->pi[0].icw[1] & 0xf8) | num, CPU_CS, CPU_IP));
		CPU_INTERRUPT((REG8)((p->pi[0].icw[1] & 0xf8) | num), 0);
	}
}
#else
void pic_irq(void) { // ver0.78

	PIC p;
	REG8 mir;
	REG8 sir;
	REG8 num;
	REG8 bit;
	REG8 slave;

#if 1 // Shinra log
	// Do not arbitrate while maskable interrupts are disabled.
	if (!CPU_isEI) {
		//TRACEOUT(("pic: !CPU_EI"));
		return;
	}
#endif

	p = &pic;
	pic_apply_level_requests();

	if (!pic8214.mode8259) {
		pic8214_update();
		sir = 0; /* no uPD8259 slave in 8214 mode */
	} else {
		sir = p->pi[1].irr & (~p->pi[1].imr);
	}
	slave = 1 << (p->pi[1].icw[2] & 7);
	mir = p->pi[0].irr;
	if (sir) {
		mir |= slave;
	}
	mir &= (~p->pi[0].imr);
	if (mir == 0) {
		return;
	}
#if 0 // Shinra log
	// Do not arbitrate while maskable interrupts are disabled.
	if (!CPU_isEI) {
		TRACEOUT(("pic: !CPU_EI"));
		return;
	}
#endif
	if (!(p->pi[0].ocw3 & PIC_OCW3_SMM)) {
		mir |= p->pi[0].isr;
	}
	num = p->pi[0].pry;
	bit = 1 << num;
	while (!(mir & bit)) {
		num = (num + 1) & 7;
		bit = 1 << num;
	}
	if (p->pi[0].icw[2] & bit) { // Slave cascade.
		if (!pic8214.mode8259) {
			pic8214_deliver(bit);
			return;
		}
		if (sir == 0) {
			return;
		}
		if (!(p->pi[1].ocw3 & PIC_OCW3_SMM)) {
			sir |= p->pi[1].isr;
		}
		num = p->pi[1].pry;
		bit = 1 << num;
		while (!(sir & bit)) {
			num = (num + 1) & 7;
			bit = 1 << num;
		}
		if (!(p->pi[1].isr & bit)) {
			if (!(p->pi[0].icw[3] & PIC_ICW4_AEOI)) {
				p->pi[0].isr |= slave;
			}
			p->pi[0].irr &= ~slave;
			if (!(p->pi[1].icw[3] & PIC_ICW4_AEOI)) {
				p->pi[1].isr |= bit;
			}
			p->pi[1].irr &= ~bit;
			//			TRACEOUT(("pic: hardware-int %.2x: [%.4x:%.4x]", (p->pi[1].icw[1] & 0xf8) | num, CPU_CS, CPU_IP));
			CPU_INTERRUPT((REG8)((p->pi[1].icw[1] & 0xf8) | num), 0);
		}
	} else if (!(p->pi[0].isr & bit)) { // Master request.
		if (!(p->pi[0].icw[3] & PIC_ICW4_AEOI)) {
			p->pi[0].isr |= bit;
		}
		p->pi[0].irr &= ~bit;
		if (num == 0) {
			nevent_reset(NEVENT_PICMASK);
		}
		//		TRACEOUT(("hardware-int %.2x [%.4x:%.4x]", (p->pi[0].icw[1] & 0xf8) | num, CPU_CS, CPU_IP));
		CPU_INTERRUPT((REG8)((p->pi[0].icw[1] & 0xf8) | num), 0);
	}
}
#endif

// Deferred system-timer request handling.
void picmask(NEVENTITEM item) {
	PICITEM pi;

	if (item->flag & NEVENT_SETEVENT) {
		pi = &pic.pi[0];
		pi->irr &= ~(pi->imr & PIC_SYSTEMTIMER);
	}
}

void pic_setirq(REG8 irq) {
	PICITEM pi;
	REG8 bit;

	pi = pic.pi;
	bit = 1 << (irq & 7);
	if (!pic8214.mode8259) {
		const int level = pic8214_level_of(irq);
		if (level >= 0) {
			pic8214_request((REG8)level);
		}
		if (irq & 8) { /* the uPD8259 slave is absent in 8214 mode */
			scsiio_trace_pic_irq(irq, TRUE);
			return;
		}
	}
	if (!(irq & 8)) {
		pi[0].irr |= bit;
		if (pi[0].imr & bit) {
			if (bit & PIC_SYSTEMTIMER) {
				if ((pit.ch[0].ctrl & 0x0c) == 0x04) {
					SINT32 cnt; // ver0.29
					if (pit.ch[0].value > 8) {
						cnt = pccore.multiple * pit.ch[0].value;
						cnt >>= 2;
					} else {
						cnt = pccore.multiple << (16 - 2);
					}
					nevent_set(NEVENT_PICMASK, cnt, picmask, NEVENT_ABSOLUTE);
				}
			}
		}
		if (pi[0].isr & bit) {
			if (bit & PIC_CRTV) {
				pi[0].irr &= ~PIC_CRTV;
			}
		}
	} else {
		pi[1].irr |= bit;
	}
	scsiio_trace_pic_irq(irq, TRUE);
}

void pic_setirq_level(REG8 irq, BOOL asserted) {
	PICITEM pi;
	REG8 bit;
	REG8 previous;
	REG8 controller;

	controller = (REG8)((irq >> 3) & 1);
	pi = pic.pi + controller;
	bit = (REG8)(1U << (irq & 7));
	previous = pic_level_state[controller] & bit;
	if (asserted) {
		pic_level_state[controller] |= bit;
		/* LTIM=0 is the VA observed edge mode.  A held NDP INT must
		 * not manufacture another edge on every CPU instruction. */
		if ((pi->icw[0] & 0x08) || !previous) {
			pi->irr |= bit;
		}
	} else {
		pic_level_state[controller] &= (REG8)~bit;
		/* In level mode IRR follows the deasserted input.  In edge mode
		 * a request already latched in IRR belongs to the controller and
		 * is intentionally left for normal PIC acknowledgement/EOI. */
		if (pi->icw[0] & 0x08) {
			pi->irr &= (REG8)~bit;
		}
	}
	if (asserted != (previous != 0)) {
		scsiio_trace_pic_irq(irq, asserted);
	}
}

void pic_resetirq(REG8 irq) {
	PICITEM pi;
	REG8 controller;
	REG8 bit;

	controller = (REG8)((irq >> 3) & 1);
	pi = pic.pi + controller;
	bit = (REG8)(1U << (irq & 7));
	pic_level_state[controller] &= (REG8)~bit;
	pi->irr &= (REG8)~bit;
}

// ---- I/O

static void IOOUTCALL pic_o00(UINT port, REG8 dat) {
	PICITEM picp;
	REG8 level;
	UINT8 ocw3;

	//	TRACEOUT(("pic %x %x", port, dat));
	picp = &pic.pi[(port >> 3) & 1];
	picp->writeicw = 0;
	switch (dat & 0x18) {
	case 0x00: // ocw2
		if (dat & PIC_OCW2_SL) {
			level = dat & PIC_OCW2_L;
		} else {
			if (!picp->isr) {
				break;
			}
			level = picp->pry;
			while (!(picp->isr & (1 << level))) {
				level = (level + 1) & 7;
			}
		}
		if (dat & PIC_OCW2_R) {
			picp->pry = (level + 1) & 7;
		}
		if (dat & PIC_OCW2_EOI) {
			picp->isr &= ~(1 << level);
			scsiio_trace_pic_irq((REG8)(((port >> 3) & 1) * 8 + level), FALSE);
			pic_apply_level_requests();
		}
		nevent_forceexit(); // mainloop exit
		break;

	case 0x08: // ocw3
		ocw3 = picp->ocw3;
		if (!(dat & PIC_OCW3_RR)) {
			dat &= PIC_OCW3_RIS;
			dat |= (ocw3 & PIC_OCW3_RIS);
		}
		if (!(dat & PIC_OCW3_ESMM)) {
			dat &= ~PIC_OCW3_SMM;
			dat |= (ocw3 & PIC_OCW3_SMM);
		}
		picp->ocw3 = dat;
		break;

	default:
		picp->icw[0] = dat;
		picp->imr = 0;
		picp->irr = 0;
		picp->ocw3 = 0;
#if 0
			picp->levels = 0;
			picp->isr = 0;
#endif
		picp->pry = 0;
		picp->writeicw = 1;
		break;
	}
}

static void IOOUTCALL pic_o02(UINT port, REG8 dat) {
	PICITEM picp;

	//	TRACEOUT(("pic %x %x", port, dat));
	picp = &pic.pi[(port >> 3) & 1];
	if (!picp->writeicw) {
#if 1
		UINT8 set;
		set = picp->imr & (~dat);
		// Re-evaluate requests exposed by the cleared mask bits.
		if ((CPU_isDI) || (!(picp->irr & set))) {
			picp->imr = dat;
			return;
		}
#endif
		picp->imr = dat;
	} else {
		picp->icw[picp->writeicw] = dat;
		picp->writeicw++;
		if (picp->writeicw >= (3 + (picp->icw[0] & 1))) {
			picp->writeicw = 0;
		}
		pic_apply_level_requests();
	}
	nevent_forceexit();
}

static REG8 IOINPCALL pic_i00(UINT port) {
	PICITEM picp;

	picp = &pic.pi[(port >> 3) & 1];
	if (!(picp->ocw3 & PIC_OCW3_RIS)) {
		return (picp->irr); // read irr
	} else {
		return (picp->isr); // read isr
	}
}

static REG8 IOINPCALL pic_i02(UINT port) {
	PICITEM picp;

	picp = &pic.pi[(port >> 3) & 1];
	return (picp->imr);
}

//slave
static void IOOUTCALL picva_o184(UINT port, REG8 dat) {
	pic_o00(0x08, dat);
}
static void IOOUTCALL picva_o186(UINT port, REG8 dat) {
	pic_o02(0x0a, dat);
}
static REG8 IOINPCALL picva_i184(UINT port) {
	return pic_i00(0x08);
}
static REG8 IOINPCALL picva_i186(UINT port) {
	return pic_i02(0x0a);
}

//master
static void IOOUTCALL picva_o188(UINT port, REG8 dat) {
	pic_o00(0x00, dat);
}
static void IOOUTCALL picva_o18a(UINT port, REG8 dat) {
	pic_o02(0x02, dat);
}
static REG8 IOINPCALL picva_i188(UINT port) {
	return pic_i00(0x00);
}
static REG8 IOINPCALL picva_i18a(UINT port) {
	return pic_i02(0x02);
}

// ---- I/F

void pic_reset(void) {
	pic.pi[0] = def_master;
	pic.pi[1] = def_slave;
	ZeroMemory(pic_level_state, sizeof(pic_level_state));
	ZeroMemory(&pic8214, sizeof(pic8214));
	pic8214.pending = PIC8214_NONE;
}

void pic_bind(void) {
	// slave
	iocore_attachout(0x184, picva_o184);
	iocore_attachout(0x186, picva_o186);
	iocore_attachinp(0x184, picva_i184);
	iocore_attachinp(0x186, picva_i186);
	// master
	iocore_attachout(0x188, picva_o188);
	iocore_attachout(0x18a, picva_o18a);
	iocore_attachinp(0x188, picva_i188);
	iocore_attachinp(0x18a, picva_i18a);
	// 8214 mode and the mode switch
	iocore_attachout(0x0e4, pic8214_oe4);
	iocore_attachout(0x0e6, pic8214_oe6);
	iocore_attachout(0x158, pic8214_o158);
}
