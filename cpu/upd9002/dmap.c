#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "dmap.h"
#include "memoryva.h"

void upd9002_dmap(void) {
	DMACH ch;
	REG8 bit;
	REG8 dat;

	if (dmac.working) {
		ch = dmac.dmach;
		bit = 1;
		do {
			if (dmac.working & bit) {
				if (ch->proc.extproc(DMAEXT_DRQ)) {
					goto next_channel;
				}
				// DMA working !
				if (!ch->leng.w) {
					dmac.stat |= bit;
					dmac.working &= ~bit;
					ch->proc.extproc(DMAEXT_END);
				}
				ch->leng.w--;

				switch (ch->mode & 0x0c) {
				case 0x00: // verifty
					ch->proc.inproc();
					break;

				case 0x04: // port->mem
					dat = ch->proc.inproc();
					memoryva.dma_access = 0x80;
					upd9002_memorywrite(ch->adrs.d, dat);
					memoryva.dma_access = 0x00;
					break;

				default:
					memoryva.dma_access = 0x80;
					dat = upd9002_memoryread(ch->adrs.d);
					memoryva.dma_access = 0x00;
					ch->proc.outproc(dat);
					break;
				}
				ch->adrs.d += ((ch->mode & 0x20) ? -1 : 1);
			}
		next_channel:
			ch++;
			bit <<= 1;
		} while (bit & 0x0f);
	}
}
