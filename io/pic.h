
typedef struct {
	UINT8 icw[4];

	UINT8 imr; // ocw1
	UINT8 isr;
	UINT8 irr;
	UINT8 ocw3;

	UINT8 pry;
	UINT8 writeicw;
	UINT8 padding[2];
} _PICITEM, *PICITEM;

typedef struct {
	_PICITEM pi[2];
} _PIC, *PIC;

/*
 * 8214 mode (V1/V2): a uPD8214-compatible controller is the ICU's slave on
 * IR7 (BNN manual 5.2.2). Reset selects this mode; port 158H selects 8259
 * mode until the next reset. Saved as its own state section.
 */
typedef struct {
	UINT8 mode8259; /* 0: 8214 mode, 1: 8259 mode */
	UINT8 status;   /* E4h: bit 3 XSGS, bits 2-0 current status */
	UINT8 inte;     /* set by an E4h write, cleared by acceptance */
	UINT8 mask;     /* E6h: bit 2 RXRDY, bit 1 VRTC, bit 0 timer 2; 1 = on */
	UINT8 irr;      /* latched requests INT0-INT7 */
	UINT8 pending;  /* level offered to compatible code, or PIC8214_NONE */
	UINT8 padding[2];
} _PIC8214;

enum {
	PIC8214_NONE = 0xff,
	PIC8214_RXRDY = 0,
	PIC8214_VRTC = 1,
	PIC8214_TIMER2 = 2,
	PIC8214_SOUND = 4,
	PIC8214_TIMER3 = 6,
	PIC8214_SGP = 7
};

enum {
	PIC_SYSTEMTIMER = 0x01,
	PIC_KEYBOARD = 0x02,
	PIC_CRTV = 0x04,
	PIC_INT0 = 0x08,
	PIC_RS232C = 0x10,
	PIC_INT1 = 0x20,
	PIC_INT2 = 0x40,
	PIC_SLAVE = 0x80,

	PIC_PRINTER = 0x01,
	PIC_SGP = 0x01,
	// PIC_PRINTERはどこにも使われていないようだ。
	PIC_INT3 = 0x02,
	PIC_INT41 = 0x04,
	PIC_INT42 = 0x08,
	PIC_INT5 = 0x10,
	PIC_INT6 = 0x20,
	PIC_NDP = 0x40,

	IRQ_INT0 = 0x03,
	IRQ_INT1 = 0x05,
	IRQ_INT2 = 0x06,
	IRQ_INT3 = 0x09,
	IRQ_INT41 = 0x0a,
	IRQ_INT42 = 0x0b,
	IRQ_INT5 = 0x0c,
	IRQ_INT6 = 0x0d,
	IRQ_NDP = 0x0e
};

#define PICEXISTINTR ((pic.pi[0].irr & (~pic.pi[0].imr)) || (pic.pi[1].irr & (~pic.pi[1].imr)))

#ifdef __cplusplus
extern "C" {
#endif

void pic_irq(void);
void pic_setirq(REG8 irq);
/* Drive a device interrupt as a level-aware input.  Edge-triggered PICs
 * latch only the low-to-high transition; level-triggered PICs re-expose the
 * request while the input remains asserted. */
void pic_setirq_level(REG8 irq, BOOL asserted);
void pic_resetirq(REG8 irq);

void picmask(NEVENTITEM item);

extern _PIC8214 pic8214;
/* Raise an 8214-mode level; ignored in 8259 mode. */
void pic8214_request(REG8 level);
/* Acknowledge from compatible code: returns the uPD780 vector. */
REG8 pic8214_acknowledge_compat(void);
/* Port 158H: leave 8214 mode for 8259 mode until the next reset. */
void pic_select_8259_mode(void);
/* 600 Hz general timer 2 (8801-compatible), an 8214-mode-only source. */
void pic8214_timer2(NEVENTITEM item);

void pic_reset(void);
void pic_bind(void);

#ifdef __cplusplus
}
#endif
