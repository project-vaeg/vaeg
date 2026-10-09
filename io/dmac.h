
#ifndef DMACCALL
#define DMACCALL
#endif

enum {
	DMAEXT_START = 0,
	DMAEXT_END = 1,
	DMAEXT_BREAK = 2,
	DMAEXT_DRQ = 3,

	DMA_INITSIGNALONLY = 1,

	DMADEV_NONE = 0,
	DMADEV_2HD = 1,
	DMADEV_2DD = 2,
	DMADEV_SASI = 3,
	DMADEV_SCSI = 4
};

#if defined(BYTESEX_LITTLE)
enum {
	DMA16_LOW = 0,
	DMA16_HIGH = 1,
	DMA32_LOW = 0,
	DMA32_HIGH = 2
};
#elif defined(BYTESEX_BIG)
enum {
	DMA16_LOW = 1,
	DMA16_HIGH = 0,
	DMA32_LOW = 2,
	DMA32_HIGH = 0
};
#endif

typedef struct {
	void(DMACCALL *outproc)(REG8 data);
	REG8(DMACCALL *inproc)(void);
	REG8(DMACCALL *extproc)(REG8 action);
} DMAPROC;

typedef struct {
	union {
		BYTE b[4];
		UINT16 w[2];
		UINT32 d;
	} adrs; // Current address register.
	union {
		BYTE b[2];
		UINT16 w;
	} leng;    // Current count register.
	BYTE dmy1; // Padding that keeps adrsorg on a four-byte boundary.
	BYTE dmy2;
	union {
		BYTE xb[4];
		UINT16 xw[2];
		UINT32 xd;
	} adrsorg; // Base address register.
	union {
		BYTE b[2];
		UINT16 w;
	} lengorg; // Base count register.
	UINT8 bound;
	UINT8 action;
	DMAPROC proc;
	UINT8 mode;
	UINT8 sreq;
	UINT8 ready;
	UINT8 mask;
} _DMACH, *DMACH;

typedef struct {
	UINT8 device;
	UINT8 channel;
} DMADEV;

typedef struct {
	_DMACH dmach[4];
	int lh;
	UINT8 work;
	UINT8 working;
	UINT8 mask; // Bits 3-0: one masks the corresponding DMA request.
	UINT8 stat; // Bits 7-4: DMA request status; not yet composed by the model.
	            // Bits 3-0: one indicates END or terminal count.
	UINT devices;
	DMADEV device[8];

	UINT8 selch; // Channel selected for register access.
	BOOL base;   // Zero selects current reads and current/base writes.
	             // One selects base-register access.

} _DMAC, *DMAC;

#ifdef __cplusplus
extern "C" {
#endif

void DMACCALL dma_dummyout(REG8 data);
REG8 DMACCALL dma_dummyin(void);
REG8 DMACCALL dma_dummyproc(REG8 func);

void dmac_reset(void);
void dmac_bind(void);
void dmac_extbind(void);

void dmac_check(void);
UINT dmac_getdatas(DMACH dmach, BYTE *buf, UINT size);

void dmac_procset(void);
void dmac_attach(REG8 device, REG8 channel);
void dmac_detach(REG8 device);

#ifdef __cplusplus
}
#endif
