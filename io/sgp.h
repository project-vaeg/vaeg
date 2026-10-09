/*
 * sgp.h: PC-88VA Super Graphic Processor
 *
 */

typedef struct {
	int scrnmode;   // Pixel format selector.
	int dot;        // Starting packed-pixel position.
	UINT16 width;   // Block width in pixels.
	UINT16 height;  // Block height in pixels.
	SINT16 fbw;     // Signed framebuffer pitch in bytes.
	UINT32 address; // Even physical start address.

	UINT32 lineaddress;
	UINT32 nextaddress;
	int dotcount;
	UINT16 buf;
	UINT16 xcount;
	UINT16 ycount;
} _SGP_BLOCK, *SGP_BLOCK;

typedef struct {
	UINT32 initialpc;
	UINT32 pc; // Command-list program counter.
	UINT32 workmem;
	UINT8 ctrl; // Bit 2 enables interrupts; bit 1 requests an abort.
	UINT8 busy; // Bit 0 reports busy state.

	UINT8 intreq; // Pending interrupt request.
	UINT8 dummy;
	UINT32 lastclock;
	SINT32 remainclock;
	UINT16 color; // Packed color selected by SET COLOR.

	//void (*func)();
	UINT16 func;

	_SGP_BLOCK src;
	_SGP_BLOCK dest;
	UINT16 newval;
	UINT16 newvalmask;
	UINT16 bltmode;

	UINT32 clsaddr;  // Current CLS address.
	UINT32 clscount; // Remaining CLS word count.

	UINT16 lineslopedenominator;
	UINT16 lineslopenumerator;
	UINT32 lineslopecount;

	UINT8 dummy2[64];
} _SGP, *SGP;

enum {
	SGP_SPEED_MODEL_DEFAULT = 0,
	SGP_SPEED_FOLLOW_CPU = 1,
	SGP_SPEED_CUSTOM = 2,
	SGP_SPEED_MODE_COUNT = 3,
	SGP_SPEED_MULTIPLIER_MAX = 16,

	SGP_INTF = 0x04,
	SGP_ABORT = 0x02,

	SGP_BUSY = 0x01,

	SGP_BLTMODE_SF = 0x1000,
	SGP_BLTMODE_VD = 0x0800,
	SGP_BLTMODE_HD = 0x0400,
	SGP_BLTMODE_TP = 0x0300,
	SGP_BLTMODE_OP = 0x000f,

	SGP_BLTMODE_LINE_VD = SGP_BLTMODE_VD,
	SGP_BLTMODE_LINE_HD = SGP_BLTMODE_HD,
};

#ifdef __cplusplus
extern "C" {
#endif

void sgp_step(void);
BOOL sgp_speed_mode_valid(UINT mode);
BOOL sgp_speed_multiplier_valid(UINT multiple);
BOOL sgp_speed_ratio(UINT mode, UINT custom_multiple, UINT cpu_multiple, UINT32 *numerator,
                     UINT32 *denominator);
UINT32 sgp_model_clock(UINT model_va);
UINT32 sgp_effective_clock(void);
void sgp_configure_speed(void);
UINT64 sgp_scale_elapsed(UINT32 elapsed);
BOOL sgp_manual_commands_selftest(void);

void sgp_reset(void);
void sgp_bind(void);

extern _SGP sgp;

#ifdef __cplusplus
}
#endif
