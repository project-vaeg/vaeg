#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "sound.h"
#include "sasiio.h"

#include "iocoreva.h"

// Change this condition to zero to enable DMA trace output.
#if 1
#undef TRACEOUT
#define TRACEOUT(arg)
#endif

void DMACCALL dma_dummyout(REG8 data) {
	(void)data;
}

REG8 DMACCALL dma_dummyin(void) {
	return (0xff);
}

REG8 DMACCALL dma_dummyproc(REG8 func) {
	(void)func;
	return (0);
}

static const DMAPROC dmaproc[] = {
    {dma_dummyout, dma_dummyin, dma_dummyproc},    // NONE
    {fdc_datawrite, fdc_dataread, fdc_dmafunc},    // 2HD
    {fdc_datawrite, fdc_dataread, fdc_dmafunc},    // 2DD
    {sasi_datawrite, sasi_dataread, sasi_dmafunc}, // SASI
    {dma_dummyout, dma_dummyin, dma_dummyproc},    // SCSI
};

// ----

void dmac_check(void) {
	BOOL workchg;
	DMACH ch;
	REG8 bit;

	workchg = FALSE;
	ch = dmac.dmach;
	bit = 1;
	do {
		if ((!(dmac.mask & bit)) && (ch->ready)) {
			if (!(dmac.work & bit)) {
				dmac.work |= bit;
				if (ch->proc.extproc(DMAEXT_START)) {
					dmac.stat &= ~bit;
					dmac.working |= bit;
					workchg = TRUE;
				}
			}
		} else {
			if (dmac.work & bit) {
				dmac.work &= ~bit;
				dmac.working &= ~bit;
				ch->proc.extproc(DMAEXT_BREAK);
				workchg = TRUE;
			}
		}
		bit <<= 1;
		ch++;
	} while (bit & 0x0f);
	if (workchg) {
		nevent_forceexit();
	}
}

UINT dmac_getdatas(DMACH dmach, BYTE *buf, UINT size) {
	UINT leng;
	UINT32 addr;
	UINT i;

	leng = min(dmach->leng.w, size);
	if (leng) {
		addr = dmach->adrs.d;        // + mask
		if (!(dmach->mode & 0x20)) { // dir +
			for (i = 0; i < leng; i++) {
				buf[i] = MEMP_READ8(addr + i);
			}
			dmach->adrs.d += leng;
		} else { // dir -
			for (i = 0; i < leng; i++) {
				buf[i] = MEMP_READ8(addr - i);
			}
			dmach->adrs.d -= leng;
		}
		dmach->leng.w -= leng;
		if (dmach->leng.w == 0) {
			dmach->proc.extproc(DMAEXT_END);
		}
	}
	return (leng);
}

// ---- I/O

/* Port 160H master-control register: bit 0 resets the DMA controller. */
static void IOOUTCALL dmacva_o160(UINT port, REG8 dat) {
	fdc_trace_text("dmatrace port=%03x val=%02x", port, dat);
	TRACEOUT(("dmac: o160: 0x%02x", dat));
	if (dat & 0x01) {
		TRACEOUT(("dmac: reset"));
		dmac.mask = 0x0f; // Mask DMA requests on all channels.
		dmac.selch = 0;   // Reset value is undocumented; initialize to zero.
		dmac.base = 0;    // Reset value is undocumented; initialize to zero.
	}
}

/* Port 161H channel selector: bit 2 selects base registers; bits 1-0
 * select the channel. */
static void IOOUTCALL dmacva_o161(UINT port, REG8 dat) {
	fdc_trace_text("dmatrace port=%03x val=%02x", port, dat);
	TRACEOUT(("dmac: o161: 0x%02x", dat));
	dmac.selch = dat & 0x03;
	dmac.base = dat & 0x04;
	TRACEOUT(("dmac: selch=%d base=%d", dmac.selch, dmac.base));
}

/* Port 161H readback: bit 4 reports base-register selection and bits 3-0
 * form a one-hot channel indication. */
static REG8 IOINPCALL dmacva_i161(UINT port) {
	BYTE ret;

	ret = (0x01 << dmac.selch) | (dmac.base ? 0x10 : 0x00);
	TRACEOUT(("dmac: i161: 0x%02x", ret));
	return ret;
}

/*
Count register, low byte.
*/
static void IOOUTCALL dmacva_o162(UINT port, REG8 dat) {
	DMACH ch;

	fdc_trace_text("dmatrace port=%03x val=%02x", port, dat);
	TRACEOUT(("dmac: o162: 0x%02x", dat));

	ch = &dmac.dmach[dmac.selch];
	ch->lengorg.b[DMA16_LOW] = dat; // Update the base register image.
	TRACEOUT(("dmac: set count reg L (base): ch=%d count L=0x%02x", dmac.selch, dat));
	/* Base-to-current reload is not implemented;
	 * update the current register image on every register write. */
	ch->leng.b[DMA16_LOW] = dat; // Update the current register image.
	dmac.stat &= ~(1 << dmac.selch);
	TRACEOUT(("dmac: set count reg L (current): ch=%d count L=0x%02x", dmac.selch, dat));
}

/*
Count register, high byte.
*/
static void IOOUTCALL dmacva_o163(UINT port, REG8 dat) {
	DMACH ch;

	fdc_trace_text("dmatrace port=%03x val=%02x", port, dat);
	TRACEOUT(("dmac: o163: 0x%02x", dat));

	ch = &dmac.dmach[dmac.selch];
	ch->lengorg.b[DMA16_HIGH] = dat; // Update the base register image.
	TRACEOUT(("dmac: set count reg H (base): ch=%d count=0x%04x", dmac.selch, ch->lengorg.w));
	/* Base-to-current reload is not implemented;
	 * update the current register image on every register write. */
	ch->leng.b[DMA16_HIGH] = dat; // Update the current register image.
	dmac.stat &= ~(1 << dmac.selch);
	TRACEOUT(("dmac: set count reg H (current): ch=%d count=0x%04x", dmac.selch, ch->leng.w));
}

/*
Count register, low byte.
*/
static REG8 IOINPCALL dmacva_i162(UINT port) {
	BYTE ret;
	if (dmac.base) {
		// Read the base register image.
		ret = dmac.dmach[dmac.selch].lengorg.b[DMA16_LOW];
	} else {
		// Read the current register image.
		ret = dmac.dmach[dmac.selch].leng.b[DMA16_LOW];
	}
	TRACEOUT(("dmac: i162: ch=%d base=%d count L=0x%02x ", dmac.selch, dmac.base, ret));
	return ret;
}

/*
Count register, high byte.
*/
static REG8 IOINPCALL dmacva_i163(UINT port) {
	BYTE ret;
	if (dmac.base) {
		// Read the base register image.
		ret = dmac.dmach[dmac.selch].lengorg.b[DMA16_HIGH];
	} else {
		// Read the current register image.
		ret = dmac.dmach[dmac.selch].leng.b[DMA16_HIGH];
	}
	TRACEOUT(("dmac: i163: ch=%d base=%d count H=0x%02x ", dmac.selch, dmac.base, ret));
	return ret;
}

static const int intel2idx[3] = {
    DMA32_LOW + DMA16_LOW,
    DMA32_LOW + DMA16_HIGH,
    DMA32_HIGH + DMA16_LOW,
};

/*
Address register.
*/
static void IOOUTCALL dmacva_o164(UINT port, REG8 dat) {
	DMACH ch;
	int idx;
	int intelidx;

	fdc_trace_text("dmatrace port=%03x val=%02x", port, dat);
	TRACEOUT(("dmac: o%03x: 0x%02x", port, dat));

	intelidx = port - 0x164;
	idx = intel2idx[intelidx];
	ch = &dmac.dmach[dmac.selch];
	ch->adrsorg.xb[idx] = dat; // Update the base register image.
	TRACEOUT(("dmac: set adrs reg (base): ch=%d adrs[%d]=0x%02x", dmac.selch, intelidx, dat));
	/* Base-to-current reload is not implemented;
	 * update the current register image on every register write. */
	ch->adrs.b[idx] = dat; // Update the current register image.
	TRACEOUT(("dmac: set adrs reg (current): ch=%d adrs[%d]=0x%02x", dmac.selch, intelidx, dat));
}

/*
Address register.
*/
static REG8 IOINPCALL dmacva_i164(UINT port) {
	int idx;
	int intelidx;
	BYTE ret;

	intelidx = port - 0x164;
	idx = intel2idx[intelidx];

	if (dmac.base) {
		// Read the base register image.
		ret = dmac.dmach[dmac.selch].adrsorg.xb[idx];
	} else {
		// Read the current register image.
		ret = dmac.dmach[dmac.selch].adrs.b[idx];
	}
	TRACEOUT(("dmac: i%03x: ch=%d base=%d count[%d]=0x%02x ", port, dmac.selch, dmac.base, intelidx,
	          ret));
	return ret;
}

#if 0
/*
Address register, bits 7-0.
*/
static void IOOUTCALL dmacva_o164(UINT port, REG8 dat) {
	DMACH ch;

	TRACEOUT(("dmac: o164: 0x%02x", dat));

	ch = &dmac.dmach[dmac.selch];
	ch->adrsorg.xb[DMA32_LOW + DMA16_LOW] = dat;		// Update the base register image.
	TRACEOUT(("dmac: set adrs reg bit7-0 (base): ch=%d adrs(7-0)=0x%02x", dmac.selch, dat));
	/* Base-to-current reload is not implemented;
	 * update the current register image on every register write. */
	ch->adrs.b[DMA32_LOW + DMA16_LOW] = dat;		// Update the current register image.
	TRACEOUT(("dmac: set adrs reg bit7-0 (current): ch=%d adrs(7-0)=0x%02x", dmac.selch, dat));

}

/*
Address register, bits 15-8.
*/
static void IOOUTCALL dmacva_o165(UINT port, REG8 dat) {
	DMACH ch;

	TRACEOUT(("dmac: o165: 0x%02x", dat));

	ch = &dmac.dmach[dmac.selch];
	ch->adrsorg.xb[DMA32_LOW + DMA16_HIGH] = dat;		// Update the base register image.
	TRACEOUT(("dmac: set adrs reg bit15-8 (base): ch=%d adrs(15-8)=0x%02x", dmac.selch, dat));
	/* Base-to-current reload is not implemented;
	 * update the current register image on every register write. */
	ch->adrs.b[DMA32_LOW + DMA16_HIGH] = dat;		// Update the current register image.
	TRACEOUT(("dmac: set adrs reg bit15-8 (current): ch=%d adrs(15-8)=0x%02x", dmac.selch, dat));

}

/*
Address register, bits 19-16.
*/
static void IOOUTCALL dmacva_o166(UINT port, REG8 dat) {
	DMACH ch;

	TRACEOUT(("dmac: o166: 0x%02x", dat));

	dat &= 0x0f;
	ch = &dmac.dmach[dmac.selch];
	ch->adrsorg.xb[DMA32_HIGH + DMA16_LOW] = dat;		// Update the base register image.
	TRACEOUT(("dmac: set adrs reg bit19-16 (base): ch=%d adrs(19-16)=0x%02x", dmac.selch, dat));
	/* Base-to-current reload is not implemented;
	 * update the current register image on every register write. */
	ch->adrs.b[DMA32_HIGH + DMA16_LOW] = dat;		// Update the current register image.
	TRACEOUT(("dmac: set adrs reg bit19-16 (current): ch=%d adrs(19-16)=0x%02x", dmac.selch, dat));

}
#endif

/*
 * Tekumani defines the 0168H/0169H device-control register for DMA disable,
 * priority rotation, bus hold, and verify waits. No handlers are implemented.
 */

/* 016AH mode control: transfer mode, address direction, auto-initialize,
 * transfer direction, and byte/word selection. */
static void IOOUTCALL dmacva_o16a(UINT port, REG8 dat) {
	fdc_trace_text("dmatrace port=%03x val=%02x", port, dat);
	TRACEOUT(("dmac: o16a: 0x%02x", dat));
	TRACEOUT(("dmac: set mode: ch=%d mode=0x%02x", dmac.selch, dat));
	dmac.dmach[dmac.selch].mode = dat;
	/* VA leaves bit 1 unused. Bit 0 is W/XB and is fixed to byte mode
     * (zero) on VA; the current transfer engine does not inspect it. */
}

/* Read back the selected channel 016AH mode-control latch. */
static REG8 IOINPCALL dmacva_i16a(UINT port) {
	BYTE ret = dmac.dmach[dmac.selch].mode;
	TRACEOUT(("dmac: i16a: ch=%d mode=0x%02x", dmac.selch, ret));
	return ret;
}

/*
Status register.
*/
static REG8 IOINPCALL dmacva_i16b(UINT port) {
	/* Bits 3-0 expose the terminal-count latches; request-status bits
	 * 7-4 are not yet modeled. */
	BYTE ret = dmac.stat;
	TRACEOUT(("dmac: i16b: stat=0x%02x", ret));
	return ret;
}

/*
Mask register.
*/
static void IOOUTCALL dmacva_o16f(UINT port, REG8 dat) {
	fdc_trace_text("dmatrace port=%03x val=%02x", port, dat);
	TRACEOUT(("dmac: o16f: 0x%02x", dat));
	TRACEOUT(("dmac: set mask: mask=0x%02x", dat));
	dmac.mask = dat;
	dmac_check();
}

static REG8 IOINPCALL dmacva_i16f(UINT port) {
	BYTE ret = dmac.mask;
	TRACEOUT(("dmac: i16f: mask=0x%02x", ret));
	return ret;
}

// ---- I/F

void dmac_reset(void) {
	ZeroMemory(&dmac, sizeof(dmac));
	dmac.lh = DMA16_LOW;
	dmac.mask = 0xf; // Mask DMA requests on all channels.
	dmac.selch = 0;  // Reset value is undocumented; initialize to zero.
	dmac.base = 0;   // Reset value is undocumented; initialize to zero.
	dmac_procset();
	//	TRACEOUT(("sizeof(_DMACH) = %d", sizeof(_DMACH)));
}

void dmac_bind(void) {
	iocore_attachout(0x160, dmacva_o160);
	iocore_attachout(0x161, dmacva_o161);
	iocore_attachinp(0x161, dmacva_i161);

	iocore_attachout(0x162, dmacva_o162);
	iocore_attachinp(0x162, dmacva_i162);
	iocore_attachout(0x163, dmacva_o163);
	iocore_attachinp(0x163, dmacva_i163);

	iocore_attachout(0x164, dmacva_o164);
	iocore_attachinp(0x164, dmacva_i164);
	iocore_attachout(0x165, dmacva_o164);
	iocore_attachinp(0x165, dmacva_i164);
	iocore_attachout(0x166, dmacva_o164);
	iocore_attachinp(0x166, dmacva_i164);

	iocore_attachout(0x16a, dmacva_o16a);
	iocore_attachinp(0x16a, dmacva_i16a);
	iocore_attachinp(0x16b, dmacva_i16b);
	iocore_attachout(0x16f, dmacva_o16f);
	iocore_attachinp(0x16f, dmacva_i16f);
}

// ----

static void dmacset(REG8 channel) {
	DMADEV *dev;
	DMADEV *devterm;
	UINT dmadev;

	dev = dmac.device;
	devterm = dev + dmac.devices;
	dmadev = DMADEV_NONE;
	while (dev < devterm) {
		if (dev->channel == channel) {
			dmadev = dev->device;
		}
		dev++;
	}
	if (dmadev >= sizeof(dmaproc) / sizeof(DMAPROC)) {
		dmadev = 0;
	}
	//	TRACEOUT(("dmac set %d - %d", channel, dmadev));
	dmac.dmach[channel].proc = dmaproc[dmadev];
}

void dmac_procset(void) {
	REG8 i;

	for (i = 0; i < 4; i++) {
		dmacset(i);
	}
}

void dmac_attach(REG8 device, REG8 channel) {
	fdc_trace_text("dmatrace attach device=%02x channel=%02x", device, channel);
	dmac_detach(device);

	if (dmac.devices < (sizeof(dmac.device) / sizeof(DMADEV))) {
		dmac.device[dmac.devices].device = device;
		dmac.device[dmac.devices].channel = channel;
		dmac.devices++;
		dmacset(channel);
	}
}

void dmac_detach(REG8 device) {
	DMADEV *dev;
	DMADEV *devterm;
	REG8 ch;

	dev = dmac.device;
	devterm = dev + dmac.devices;
	while (dev < devterm) {
		if (dev->device == device) {
			break;
		}
		dev++;
	}
	if (dev < devterm) {
		ch = dev->channel;
		dev++;
		while (dev < devterm) {
			*(dev - 1) = *dev;
			dev++;
		}
		dmac.devices--;
		dmacset(ch);
	}
}
